import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """    std::vector<Law::ApplicationRecord> records;
    static int total_agenda_size = 0;
    for (int round = 0; round < _maxChainRounds && _dirty; ++round) { if(round == 7) std::cout << "MAX CHAIN ROUNDS HIT!" << std::endl;
        _dirty = false;
        // Facts asserted before this round are consumed by it; facts asserted
        // DURING it (laws firing events from applyTo) survive into the next
        // round — that's how law chains resolve, bounded by _maxChainRounds.
        const std::size_t consumed = _rete.facts().size();
        // Straight to the drain: the agenda is already complete. (An
        // _rete.evaluate() call sat here whose result was discarded — the
        // last trace of the rebuild-every-frame design.)
        std::vector<ReteActivation> agenda = _rete.drainAgenda();
        total_agenda_size += agenda.size();
        for (const auto& activation : agenda) {"""

replace_str = """    std::vector<Law::ApplicationRecord> records;
    static int total_agenda_size = 0;
    static double total_agenda_loop_ms = 0.0;
    for (int round = 0; round < _maxChainRounds && _dirty; ++round) { if(round == 7) std::cout << "MAX CHAIN ROUNDS HIT!" << std::endl;
        _dirty = false;
        // Facts asserted before this round are consumed by it; facts asserted
        // DURING it (laws firing events from applyTo) survive into the next
        // round — that's how law chains resolve, bounded by _maxChainRounds.
        const std::size_t consumed = _rete.facts().size();
        // Straight to the drain: the agenda is already complete. (An
        // _rete.evaluate() call sat here whose result was discarded — the
        // last trace of the rebuild-every-frame design.)
        std::vector<ReteActivation> agenda = _rete.drainAgenda();
        total_agenda_size += agenda.size();
        auto t_agenda_start = glfwGetTime();
        for (const auto& activation : agenda) {"""

find_str2 = """        _rete.retractFirst(consumed);
    }

    // ------------------------------------------------------------------"""

replace_str2 = """        total_agenda_loop_ms += (glfwGetTime() - t_agenda_start) * 1000.0;
        _rete.retractFirst(consumed);
    }

    // ------------------------------------------------------------------"""

find_str3 = """    const std::vector<std::shared_ptr<Law>> continuousLaws = _laws;
    for (const auto& law : continuousLaws) {"""

replace_str3 = """    const std::vector<std::shared_ptr<Law>> continuousLaws = _laws;
    auto t_laws_start = glfwGetTime();
    for (const auto& law : continuousLaws) {"""

find_str4 = """        totalSweepTime += (glfwGetTime() - tS1);
    }"""

replace_str4 = """        totalSweepTime += (glfwGetTime() - tS1);
    }
    static double total_laws_loop_ms = 0.0;
    total_laws_loop_ms += (glfwGetTime() - t_laws_start) * 1000.0;"""


find_str5 = """    if (tickCount++ % 24 == 0) {
        printf("TICK: agenda_size=%d, reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f, evaluateDirty=%.3f, restOfTick=%.3f\\n",
            total_agenda_size, reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms, total_eval_dirty_ms, total_rest_of_tick);
        total_agenda_size = 0;
        total_seed_ms = 0.0;"""

replace_str5 = """    if (tickCount++ % 24 == 0) {
        printf("TICK: agenda_size=%d, agenda_loop=%.3f, laws_loop=%.3f, reteCount=%d (%.3f ms rete, %.3f ms apply), sweepCount=%d (%.3f ms sweep), seed=%.3f, vocab=%.3f, evaluateDirty=%.3f, restOfTick=%.3f\\n",
            total_agenda_size, total_agenda_loop_ms, total_laws_loop_ms, reteCount, totalReteTime*1000.0, totalApplyTime*1000.0, sweepCount, totalSweepTime*1000.0, total_seed_ms, total_vocab_ms, total_eval_dirty_ms, total_rest_of_tick);
        total_agenda_size = 0;
        total_agenda_loop_ms = 0.0;
        total_laws_loop_ms = 0.0;
        total_seed_ms = 0.0;"""

if find_str in content and find_str2 in content and find_str3 in content and find_str4 in content and find_str5 in content:
    content = content.replace(find_str, replace_str)
    content = content.replace(find_str2, replace_str2)
    content = content.replace(find_str3, replace_str3)
    content = content.replace(find_str4, replace_str4)
    content = content.replace(find_str5, replace_str5)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Law.cpp timing 13!")
else:
    print("Could not find string in Law.cpp!")
