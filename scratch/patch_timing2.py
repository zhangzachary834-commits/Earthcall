import sys

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()

content = content.replace("static int p_tick_counter = 0;", "")
content = content.replace("static double S_SYNC = 0, S_SEED = 0, S_EVAL = 0, S_DRIVE = 0, S_REAP = 0;", "")
content = content.replace("S_SYNC += _tickTiming.syncMs;", "")
content = content.replace("S_SEED += _tickTiming.seedMs;", "")
content = content.replace("S_EVAL += _tickTiming.evalMs;", "")
content = content.replace("S_DRIVE += _tickTiming.driveMs;", "")
content = content.replace("S_REAP += _tickTiming.reapMs;", "")
content = content.replace("if (++p_tick_counter == 24) {", "if (_tickTiming.totalMs > 10.0) {")
content = content.replace('printf("--- INTERNAL TICK PERF ---\\n");', 'printf("--- SLOW TICK: %.3f ms ---\\n", _tickTiming.totalMs);')
content = content.replace('printf("syncMs:  %.3f ms\\n", S_SYNC);', 'printf("sync: %.3f ms\\n", _tickTiming.syncMs);')
content = content.replace('printf("seedMs:  %.3f ms\\n", S_SEED);', 'printf("seed: %.3f ms\\n", _tickTiming.seedMs);')
content = content.replace('printf("evalMs:  %.3f ms\\n", S_EVAL);', 'printf("eval: %.3f ms\\n", _tickTiming.evalMs);')
content = content.replace('printf("driveMs: %.3f ms\\n", S_DRIVE);', 'printf("drive: %.3f ms\\n", _tickTiming.driveMs);')
content = content.replace('printf("reapMs:  %.3f ms\\n", S_REAP);', 'printf("reap: %.3f ms\\n", _tickTiming.reapMs);')

with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
