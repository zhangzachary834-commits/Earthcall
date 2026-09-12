import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """        std::vector<ReteActivation> agenda = _rete.drainAgenda();
        for (const auto& activation : agenda) {"""

replace_str = """        std::vector<ReteActivation> agenda = _rete.drainAgenda();
        static int total_agenda_size = 0;
        total_agenda_size += agenda.size();
        for (const auto& activation : agenda) {"""

find_str2 = """    if (tickCount++ % 24 == 0) {
        printf("TICK: reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f, evaluateDirty=%.3f, restOfTick=%.3f\\n",
            reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms, total_eval_dirty_ms, total_rest_of_tick);"""

replace_str2 = """    if (tickCount++ % 24 == 0) {
        printf("TICK: agenda_size=%d, reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f, evaluateDirty=%.3f, restOfTick=%.3f\\n",
            total_agenda_size, reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms, total_eval_dirty_ms, total_rest_of_tick);
        total_agenda_size = 0;"""

if find_str in content:
    content = content.replace(find_str, replace_str)
    content = content.replace(find_str2, replace_str2)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Law.cpp timing 11!")
else:
    print("Could not find string in Law.cpp!")
