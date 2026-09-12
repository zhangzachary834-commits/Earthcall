import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """    if (_rete.hasDirtyFacts()) {
        _rete.evaluateDirty();
        _dirty = true;
    }"""

replace_str = """    auto t_eval_dirty_start = glfwGetTime();
    if (_rete.hasDirtyFacts()) {
        _rete.evaluateDirty();
        _dirty = true;
    }
    double eval_dirty_ms = (glfwGetTime() - t_eval_dirty_start) * 1000.0;
    static double total_eval_dirty_ms = 0.0;
    total_eval_dirty_ms += eval_dirty_ms;"""

find_str2 = """    if (tickCount++ % 24 == 0) {
        printf("TICK: syncP=%.3f, syncR=%.3f, reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f\\n",
            (Tp - T0)*1000.0, (T1 - Tp)*1000.0, reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms);
        total_seed_ms = 0.0;
        total_vocab_ms = 0.0;
    }"""

replace_str2 = """    if (tickCount++ % 24 == 0) {
        printf("TICK: reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f, evaluateDirty=%.3f\\n",
            reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms, total_eval_dirty_ms);
        total_seed_ms = 0.0;
        total_vocab_ms = 0.0;
        total_eval_dirty_ms = 0.0;
    }"""

if find_str in content and find_str2 in content:
    content = content.replace(find_str, replace_str)
    content = content.replace(find_str2, replace_str2)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Law.cpp timing 8!")
else:
    print("Could not find string in Law.cpp!")
