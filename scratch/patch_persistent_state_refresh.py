from pathlib import Path

cpp_path = Path('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp')
hpp_path = Path('src/ZonesOfEarth/AuthorsOfLaw/Law.hpp')
cpp = cpp_path.read_text()
hpp = hpp_path.read_text()

if 'void ReteNetwork::refreshStateFact(' in cpp:
    raise SystemExit('refreshStateFact already exists; refusing to double-patch')


def function_block(text: str, signature: str):
    """Return (start, end, source) for one currently-known C++ function.

    We use balanced braces instead of whitespace/tail anchors so harmless
    formatting around a function cannot masquerade as a semantic patch
    failure. The targeted Rete functions contain balanced C++ lambdas but no
    brace-bearing string literals, so this is deliberately small rather than a
    pretend C++ parser.
    """
    start = text.index(signature)
    open_brace = text.index('{', start + len(signature))
    depth = 0
    for pos in range(open_brace, len(text)):
        ch = text[pos]
        if ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0:
                end = pos + 1
                return start, end, text[start:end]
    raise SystemExit(f'unclosed function while patching: {signature}')


# Split assertion into storage admission and the existing incremental
# discrimination path. The latter is reused verbatim by refresh.
assert_sig = 'std::string ReteNetwork::assertFact(FactPtr fact)'
assert_start, assert_end, assert_func = function_block(cpp, assert_sig)
propagation_marker = '    std::vector<std::size_t> activatedAlphas;'
propagation_start = assert_func.index(propagation_marker)
return_pos = assert_func.rfind('\n    return f->id;')
if return_pos < propagation_start:
    raise SystemExit('assertFact return moved before propagation; inspect before patching')
propagation_body = assert_func[propagation_start:return_pos]
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
retract_sig = 'bool ReteNetwork::retractFact(const std::string& factId)'
retract_start, retract_end, retract_func = function_block(cpp, retract_sig)
consequence_marker = '    auto aIt = _factAlphaNodes.find(factId);'
consequence_start = retract_func.index(consequence_marker)
return_pos = retract_func.rfind('\n    return true;')
if return_pos < consequence_start:
    raise SystemExit('retractFact return moved before consequence cleanup; inspect before patching')
consequence_body = retract_func[consequence_start:return_pos]
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
eval_sig = 'void ReteNetwork::evaluateDirty()'
eval_start, eval_end, eval_func = function_block(cpp, eval_sig)
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

lines = eval_func.splitlines(keepends=True)
retract_line = next((i for i, line in enumerate(lines) if 'retractFact(fact->id);' in line), None)
value_line = next((i for i, line in enumerate(lines) if 'fact->value = newValue;' in line), None)
assert_line = next((i for i, line in enumerate(lines) if 'assertFact(fact);' in line), None)
if retract_line is None or value_line is None or assert_line is None:
    raise SystemExit('evaluateDirty refresh calls changed; inspect before patching')
if not (retract_line < value_line < assert_line <= retract_line + 6):
    raise SystemExit('evaluateDirty refresh call order changed; inspect before patching')

replace_start = retract_line
if retract_line > 0 and '// Retract' in lines[retract_line - 1]:
    replace_start -= 1
replace_end = assert_line + 1
new_dirty = (
    '            // The state fact persists. Invalidate consequences of its old\n'
    '            // value, mutate the same identity in place, then discriminate the\n'
    '            // new value. Do not erase/reappend _facts: its order is consumed\n'
    '            // by retractFirst(), and a value change is not fact death.\n'
    '            refreshStateFact(fact, std::move(newValue));\n'
)
patched_eval = ''.join(lines[:replace_start]) + new_dirty + ''.join(lines[replace_end:])
cpp = cpp[:eval_start] + refresh + patched_eval + cpp[eval_end:]

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
