from pathlib import Path

cpp_path = Path('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp')
hpp_path = Path('src/ZonesOfEarth/AuthorsOfLaw/Law.hpp')
cpp = cpp_path.read_text()
hpp = hpp_path.read_text()

if 'void ReteNetwork::refreshStateFact(' in cpp:
    raise SystemExit('refreshStateFact already exists; refusing to double-patch')

# Split assertion into storage admission and the existing incremental
# discrimination path. The latter is reused verbatim by refresh.
assert_start = cpp.index('std::string ReteNetwork::assertFact(FactPtr fact) {')
assert_end = cpp.index('\nvoid ReteNetwork::retractFirst', assert_start)
assert_func = cpp[assert_start:assert_end]
propagation_marker = '    std::vector<std::size_t> activatedAlphas;'
propagation_start = assert_func.index(propagation_marker)
assert_tail = '\n    return f->id;\n}'
if not assert_func.endswith(assert_tail):
    raise SystemExit('assertFact tail changed; inspect before patching')
propagation_body = assert_func[propagation_start:-len(assert_tail)]
new_assert = (
    assert_func[:propagation_start]
    + '    propagateFact(f);\n'
    + '    return f->id;\n'
    + '}'
)
propagate = (
    '\n\nvoid ReteNetwork::propagateFact(const FactPtr& f) {\n'
    + propagation_body
    + '\n}'
)
cpp = cpp[:assert_start] + new_assert + propagate + cpp[assert_end:]

# Split structural storage deletion from invalidating the old Rete
# consequences. The extracted block is byte-for-byte the existing
# Alpha/Beta/agenda cleanup.
retract_start = cpp.index('bool ReteNetwork::retractFact(const std::string& factId) {')
retract_end = cpp.index('\nbool ReteNetwork::hasRelationStateFact', retract_start)
retract_func = cpp[retract_start:retract_end]
consequence_marker = '    auto aIt = _factAlphaNodes.find(factId);'
consequence_start = retract_func.index(consequence_marker)
retract_tail = '\n    return true;\n}'
if not retract_func.endswith(retract_tail):
    raise SystemExit('retractFact tail changed; inspect before patching')
consequence_body = retract_func[consequence_start:-len(retract_tail)]
new_retract = (
    retract_func[:consequence_start]
    + '    detachFactConsequences(fact);\n'
    + '\n    return true;\n'
    + '}'
)
detach = (
    'void ReteNetwork::detachFactConsequences(const FactPtr& fact) {\n'
    '    if (!fact) return;\n'
    '    const std::string& factId = fact->id;\n'
    + consequence_body
    + '\n}\n\n'
)
cpp = cpp[:retract_start] + detach + new_retract + cpp[retract_end:]

# Refresh is deliberately ordered: detach while OLD value is still in the
# shared FactPtr, mutate, then run the normal discriminator.
eval_marker = 'void ReteNetwork::evaluateDirty() {'
eval_pos = cpp.index(eval_marker)
refresh = '''void ReteNetwork::refreshStateFact(const FactPtr& fact, nlohmann::json newValue) {
    if (!fact) return;

    // A persistent property changing value is not structural deletion.
    // Detach every consequence of the OLD value before mutating the shared
    // FactPtr: Alpha/Beta memories and agenda tokens retain FactPtr identity,
    // so mutating first would rewrite history under the old memberships.
    // Fact-store residency and ordering remain unchanged; only genuine
    // retractFact() removes from _facts.
    detachFactConsequences(fact);
    fact->value = std::move(newValue);
    propagateFact(fact);
}

'''
cpp = cpp[:eval_pos] + refresh + cpp[eval_pos:]

old_dirty = '''            // Retract the old fact
            retractFact(fact->id);
            
            // Update value and re-assert
            fact->value = newValue;
            assertFact(fact);'''
if old_dirty not in cpp:
    raise SystemExit('evaluateDirty refresh block changed; inspect before patching')
new_dirty = '''            // The state fact persists. Invalidate consequences of its old
            // value, mutate the same identity in place, then discriminate the
            // new value. Do not erase/reappend _facts: its order is consumed
            // by retractFirst(), and a value change is not fact death.
            refreshStateFact(fact, std::move(newValue));'''
cpp = cpp.replace(old_dirty, new_dirty, 1)

private_anchor = '''    const AlphaNode* findAlpha(std::size_t id) const;
    AlphaNode* findAlpha(std::size_t id);'''
if private_anchor not in hpp:
    raise SystemExit('ReteNetwork private helper anchor changed')
private_replacement = private_anchor + '''
    void propagateFact(const FactPtr& fact);
    void detachFactConsequences(const FactPtr& fact);
    void refreshStateFact(const FactPtr& fact, nlohmann::json newValue);'''
hpp = hpp.replace(private_anchor, private_replacement, 1)

cpp_path.write_text(cpp)
hpp_path.write_text(hpp)
