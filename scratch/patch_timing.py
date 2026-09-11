import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("_tickTiming.totalMs = static_cast<float>((T5 - T0) * 1000.0);", """_tickTiming.totalMs = static_cast<float>((T5 - T0) * 1000.0);
    static int p_tick_counter = 0;
    static double S_SYNC = 0, S_SEED = 0, S_EVAL = 0, S_DRIVE = 0, S_REAP = 0;
    S_SYNC += _tickTiming.syncMs;
    S_SEED += _tickTiming.seedMs;
    S_EVAL += _tickTiming.evalMs;
    S_DRIVE += _tickTiming.driveMs;
    S_REAP += _tickTiming.reapMs;
    if (++p_tick_counter == 24) {
        printf("--- INTERNAL TICK PERF ---\\n");
        printf("syncMs:  %.3f ms\\n", S_SYNC);
        printf("seedMs:  %.3f ms\\n", S_SEED);
        printf("evalMs:  %.3f ms\\n", S_EVAL);
        printf("driveMs: %.3f ms\\n", S_DRIVE);
        printf("reapMs:  %.3f ms\\n", S_REAP);
    }""")

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
