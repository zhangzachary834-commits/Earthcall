with open("tests/singularity/frame_lag_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
"""        PropertyValue color(glm::vec3(0.0f, static_cast<float>(i)/10.0f, 1.0f));
        macro->setDynamicProperty("authored.left_half", color);
        lawManager.tick();""",
"""        PropertyValue color(glm::vec3(0.0f, static_cast<float>(i)/10.0f, 1.0f));
        double tA = glfwGetTime();
        macro->setDynamicProperty("authored.left_half", color);
        double tB = glfwGetTime();
        lawManager.tick();
        double tC = glfwGetTime();
        std::printf("  setProp: %.3f ms, tick: %.3f ms\\n", (tB - tA) * 1000.0, (tC - tB) * 1000.0);""")

with open("tests/singularity/frame_lag_test.cpp", "w") as f:
    f.write(content)
