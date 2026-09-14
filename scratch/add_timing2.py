with open("tests/singularity/frame_lag_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
"""        double ms = (t1 - t0) * 1000.0;
        if (ms > maxMs) maxMs = ms;""",
"""        double ms = (t1 - t0) * 1000.0;
        std::printf("Iteration %d: %.3f ms\\n", i, ms);
        if (i > 0 && ms > maxMs) maxMs = ms; // Skip first iteration for maxMs
        else if (i == 0) maxMs = ms; // wait, let's just see what it prints""")

with open("tests/singularity/frame_lag_test.cpp", "w") as f:
    f.write(content)
