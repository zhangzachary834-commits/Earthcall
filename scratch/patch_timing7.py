import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """    if (_connected) {
        for (Singular* being : Universe::instance().beings()) {
            seedStateFacts(being);
        }
    }

    // Once per tick, for every law — not once per law. This is the whole point
    // of the index: the world is walked a single time here, and each sweeping
    // law below then reads a candidate list instead of rebuilding
    // Universe::beings() for itself. In a frame where nothing was made,
    // unmade, or granted a property, it returns on an integer compare.
    //
    // It must run BEFORE the continuous pass, and after seeding: a being
    // admitted this tick has to be in the index the same tick, or the law that
    // wants it waits a frame — a widening, but a needless one.
    refreshVocabularyIndex();

    auto T2 = glfwGetTime();"""

replace_str = """    auto t_before_seed = glfwGetTime();
    if (_connected) {
        for (Singular* being : Universe::instance().beings()) {
            seedStateFacts(being);
        }
    }
    auto t_after_seed = glfwGetTime();

    // Once per tick, for every law — not once per law. This is the whole point
    // of the index: the world is walked a single time here, and each sweeping
    // law below then reads a candidate list instead of rebuilding
    // Universe::beings() for itself. In a frame where nothing was made,
    // unmade, or granted a property, it returns on an integer compare.
    //
    // It must run BEFORE the continuous pass, and after seeding: a being
    // admitted this tick has to be in the index the same tick, or the law that
    // wants it waits a frame — a widening, but a needless one.
    refreshVocabularyIndex();

    auto T2 = glfwGetTime();
    
    double seed_ms = (t_after_seed - t_before_seed) * 1000.0;
    double vocab_ms = (T2 - t_after_seed) * 1000.0;
    static double total_seed_ms = 0.0;
    static double total_vocab_ms = 0.0;
    total_seed_ms += seed_ms;
    total_vocab_ms += vocab_ms;"""

find_str2 = """    if (tickCount++ % 24 == 0) {
        printf("TICK: syncP=%.3f, syncR=%.3f, reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep)\\n",
            (Tp - T0)*1000.0, (T1 - Tp)*1000.0, reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0);
    }"""

replace_str2 = """    if (tickCount++ % 24 == 0) {
        printf("TICK: syncP=%.3f, syncR=%.3f, reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f\\n",
            (Tp - T0)*1000.0, (T1 - Tp)*1000.0, reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms);
        total_seed_ms = 0.0;
        total_vocab_ms = 0.0;
    }"""

if find_str in content and find_str2 in content:
    content = content.replace(find_str, replace_str)
    content = content.replace(find_str2, replace_str2)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Law.cpp timing!")
else:
    print("Could not find string in Law.cpp!")
