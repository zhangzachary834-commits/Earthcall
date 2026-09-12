import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """    Core::EventBus::instance().subscribe<ECA::PropertyChangedEvent>([this](const ECA::PropertyChangedEvent& e) {
        if (!e.subject) return;
        if (!_rete.markFactDirty(e.subject->getIdentifier(), e.property)) {"""

replace_str = """    Core::EventBus::instance().subscribe<ECA::PropertyChangedEvent>([this](const ECA::PropertyChangedEvent& e) {
        if (!e.subject) return;
        static int prop_changes = 0;
        prop_changes++;
        if (!_rete.markFactDirty(e.subject->getIdentifier(), e.property)) {"""

find_str2 = """    if (tickCount++ % 24 == 0) {
        printf("TICK: agenda_size=%d, agenda_loop=%.3f, laws_loop=%.3f, reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f, evaluateDirty=%.3f, restOfTick=%.3f\\n",
            total_agenda_size, total_agenda_loop_ms, total_laws_loop_ms, reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms, total_eval_dirty_ms, total_rest_of_tick);"""

replace_str2 = """    if (tickCount++ % 24 == 0) {
        printf("TICK: propChanges=%d, agenda_size=%d, agenda_loop=%.3f, laws_loop=%.3f, reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f, evaluateDirty=%.3f, restOfTick=%.3f\\n",
            prop_changes, total_agenda_size, total_agenda_loop_ms, total_laws_loop_ms, reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms, total_eval_dirty_ms, total_rest_of_tick);
        prop_changes = 0;"""

if find_str in content and find_str2 in content:
    content = content.replace(find_str, replace_str)
    content = content.replace(find_str2, replace_str2)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Law.cpp timing 15!")
else:
    print("Could not find string in Law.cpp!")
