with open("tests/singularity/frame_lag_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
"""        double t1 = glfwGetTime();
        double ms = (t1 - t0) * 1000.0;
        std::printf("  setProp: %.3f ms, tick: %.3f ms\\n", (tB - tA) * 1000.0, (tC - tB) * 1000.0);
        std::printf("Iteration %d: %.3f ms\\n", i, ms);
        if (i > 0 && ms > maxMs) maxMs = ms; // Skip first iteration for maxMs
        else if (i == 0) maxMs = ms; // wait, let's just see what it prints""",
"""        double t1 = glfwGetTime();
        double ms = (t1 - t0) * 1000.0;
        if (ms > maxMs) maxMs = ms;""")

content = content.replace(
"""        PropertyValue color(glm::vec3(0.0f, static_cast<float>(i)/10.0f, 1.0f));
        double tA = glfwGetTime();
        macro->setDynamicProperty("authored.left_half", color);
        double tB = glfwGetTime();
        lawManager.tick();
        double tC = glfwGetTime();""",
"""        PropertyValue color(glm::vec3(0.0f, static_cast<float>(i)/10.0f, 1.0f));
        macro->setDynamicProperty("authored.left_half", color);
        lawManager.tick();""")

with open("tests/singularity/frame_lag_test.cpp", "w") as f:
    f.write(content)
