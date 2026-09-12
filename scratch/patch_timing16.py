import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """bool LawManager::propheticHears(const std::string& propertyName) const {
    ++_propheticCounters.asked;"""

replace_str = """bool LawManager::propheticHears(const std::string& propertyName) const {
    ++_propheticCounters.asked;
    static int prop_hears_count = 0;
    static int prop_hears_true_count = 0;
    prop_hears_count++;"""

find_str2 = """    if (_propheticRevision != Law::textRevision()) return true;
    // (2) INCOMPLETE. Some law reads through a closure, a collision test, or a
    //     condition kind this build cannot read. Nothing may be pruned around
    //     a law whose reads are not enumerable.
    if (!_prophetic.complete()) return true;
    // (3) FOREIGN ALPHA. A node bound with a hand-written predicate (the graph
    //     editor, a test, a channel) matches on whatever it likes, and no law
    //     text accounts for it. Deliberately NOT hasOpaqueBoundAlpha(), which
    //     counts every compiled condition too — an authored condition's reads
    //     are exactly the law's own text, which this index has read.
    if (_rete.hasForeignBoundAlpha()) return true;

    if (_prophetic.anyConditionReads(propertyName)) return true;
    ++_propheticCounters.filtered;
    return false;
}"""

replace_str2 = """    if (_propheticRevision != Law::textRevision()) { prop_hears_true_count++; return true; }
    // (2) INCOMPLETE. Some law reads through a closure, a collision test, or a
    //     condition kind this build cannot read. Nothing may be pruned around
    //     a law whose reads are not enumerable.
    if (!_prophetic.complete()) { prop_hears_true_count++; return true; }
    // (3) FOREIGN ALPHA. A node bound with a hand-written predicate (the graph
    //     editor, a test, a channel) matches on whatever it likes, and no law
    //     text accounts for it. Deliberately NOT hasOpaqueBoundAlpha(), which
    //     counts every compiled condition too — an authored condition's reads
    //     are exactly the law's own text, which this index has read.
    if (_rete.hasForeignBoundAlpha()) { prop_hears_true_count++; return true; }

    if (_prophetic.anyConditionReads(propertyName)) { prop_hears_true_count++; return true; }
    ++_propheticCounters.filtered;
    return false;
}"""

find_str3 = """    if (tickCount++ % 24 == 0) {
        printf("TICK: agenda_size=%d, agenda_loop=%.3f, laws_loop=%.3f, reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f, evaluateDirty=%.3f, restOfTick=%.3f\\n",
            total_agenda_size, total_agenda_loop_ms, total_laws_loop_ms, reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms, total_eval_dirty_ms, total_rest_of_tick);"""

replace_str3 = """    if (tickCount++ % 24 == 0) {
        extern int prop_hears_count;
        extern int prop_hears_true_count;
        printf("TICK: propHears=%d/%d, agenda_size=%d, agenda_loop=%.3f, laws_loop=%.3f, reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f, evaluateDirty=%.3f, restOfTick=%.3f\\n",
            prop_hears_true_count, prop_hears_count, total_agenda_size, total_agenda_loop_ms, total_laws_loop_ms, reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms, total_eval_dirty_ms, total_rest_of_tick);
        prop_hears_count = 0;
        prop_hears_true_count = 0;"""

if find_str in content and find_str2 in content and find_str3 in content:
    content = content.replace(find_str, replace_str)
    content = content.replace(find_str2, replace_str2)
    content = content.replace(find_str3, replace_str3)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Law.cpp timing 16!")
else:
    print("Could not find string in Law.cpp!")
