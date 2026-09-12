import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """    if (tickCount++ % 24 == 0) {
        printf("TICK: reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f, evaluateDirty=%.3f\\n",
            reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms, total_eval_dirty_ms);
        total_seed_ms = 0.0;
        total_vocab_ms = 0.0;
        total_eval_dirty_ms = 0.0;
    }

    auto T3 = glfwGetTime();"""

replace_str = """    auto T3 = glfwGetTime();
    double rest_of_tick = (T3 - T2) * 1000.0;
    static double total_rest_of_tick = 0.0;
    total_rest_of_tick += rest_of_tick;

    if (tickCount++ % 24 == 0) {
        printf("TICK: reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f, evaluateDirty=%.3f, restOfTick=%.3f\\n",
            reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms, total_eval_dirty_ms, total_rest_of_tick);
        total_seed_ms = 0.0;
        total_vocab_ms = 0.0;
        total_eval_dirty_ms = 0.0;
        total_rest_of_tick = 0.0;
    }"""

if find_str in content:
    content = content.replace(find_str, replace_str)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Law.cpp timing 10!")
else:
    print("Could not find string in Law.cpp!")
