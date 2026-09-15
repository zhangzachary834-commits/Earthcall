with open("tests/singularity/frame_lag_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
"""    if (!elevated) {
        std::printf("FAIL: ElevatePixels failed: %s\\n", reason.c_str());
        gFailures++;
        return;
    }
    
    // 3. Mutate the region property many times""",
"""    if (!elevated) {
        std::printf("FAIL: ElevatePixels failed: %s\\n", reason.c_str());
        gFailures++;
        return;
    }
    lawManager.tick(); // Flush the creation and elevation events from the change feed!
    
    // 3. Mutate the region property many times""")

with open("tests/singularity/frame_lag_test.cpp", "w") as f:
    f.write(content)
