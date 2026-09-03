import sys
file_path = "src/Singularity/Core/Engine.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("void Engine::tick(float dt) {", "void Engine::tick(float dt) { auto TICK_START = glfwGetTime();")
content = content.replace("if (_lawManager) _lawManager->tick();", "auto LAW_0 = glfwGetTime(); if (_lawManager) _lawManager->tick(); auto LAW_1 = glfwGetTime();")
content = content.replace("_webgpu->renderer.present();", """
        _webgpu->renderer.present();
        auto TICK_PRESENT = glfwGetTime();
        static int frameCount = 0;
        if (frameCount++ % 60 == 0) {
            printf("CPU FRAME TIMING: law_manager=%.2fms\\n",
                   (LAW_1-LAW_0)*1000.0);
            fflush(stdout);
        }
""")

with open(file_path, "w") as f:
    f.write(content)
