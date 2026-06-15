#include "Core/Engine.hpp"
#include "Core/Game.hpp"

#include "../../imgui/imgui.h"
#include "../../imgui/backends/imgui_impl_glfw.h"
#ifdef USE_GL3_RENDERER
#  include "../../imgui/backends/imgui_impl_opengl3.h"
#else
#  include "../../imgui/backends/imgui_impl_opengl2.h"
#endif

#include <iostream>
#include <chrono>

namespace Core {

Engine& Engine::instance() {
    static Engine s_instance;
    return s_instance;
}

bool Engine::init(int /*argc*/, char** /*argv*/) {
    if (glfwInit() == GLFW_FALSE) {
        std::cerr << "⚠️  Failed to initialise GLFW!" << std::endl;
        return false;
    }

    // Request a modern OpenGL 3.3 core profile when GL3 renderer is enabled
#ifdef USE_GL3_RENDERER
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#  if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#  endif
#endif

    _window = glfwCreateWindow(1280, 720, "Earthcall", nullptr, nullptr);
    if (!_window) {
        std::cerr << "⚠️  Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(_window);
    // Lock cursor initially so camera control behaves consistently
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // ------------------------------
    // ImGui context initialisation
    // ------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef ImGuiConfigFlags_DockingEnable
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

    ImGui_ImplGlfw_InitForOpenGL(_window, true);
#ifdef USE_GL3_RENDERER
    ImGui_ImplOpenGL3_Init("#version 330 core");
#else
    ImGui_ImplOpenGL2_Init();
#endif
    _running = true;

    // TODO: move OpenGL / ImGui initialisation here later.
    std::cout << "🌟 Engine initialised." << std::endl;
    return true;
}

void Engine::run(Game& game) {
    double lastTime = glfwGetTime();

    while (_running && _window && !glfwWindowShouldClose(_window)) {
        double currentTime = glfwGetTime();
        float  dt          = static_cast<float>(currentTime - lastTime);
        lastTime           = currentTime;

        glfwPollEvents();

        // Start a new ImGui frame
#ifdef USE_GL3_RENDERER
        ImGui_ImplOpenGL3_NewFrame();
#else
        ImGui_ImplOpenGL2_NewFrame();
#endif
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // TEMP frame profiler: prints avg update/render ms once per second.
        using clk = std::chrono::high_resolution_clock;
        static double accUpd = 0.0, accRen = 0.0, accSec = 0.0; static int frames = 0;
        auto t0 = clk::now();
        game.update(dt);
        auto t1 = clk::now();
        game.render();
        auto t2 = clk::now();
        accUpd += std::chrono::duration<double, std::milli>(t1 - t0).count();
        accRen += std::chrono::duration<double, std::milli>(t2 - t1).count();
        accSec += dt; ++frames;
        if (accSec >= 1.0 && frames > 0) {
            fprintf(stderr, "[PERF] %d fps | update %.2f ms | render %.2f ms\n",
                    frames, accUpd / frames, accRen / frames);
            accUpd = accRen = accSec = 0.0; frames = 0;
        }

        // Render ImGui
        ImGui::Render();
#ifdef USE_GL3_RENDERER
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#else
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
#endif

        glfwSwapBuffers(_window);
    }

    // Allow Game to perform shutdown logic before engine terminates
    game.shutdown();
}

void Engine::shutdown() {
    if (_window) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
    // Shutdown ImGui after window destruction but before GLFW termination
#ifdef USE_GL3_RENDERER
    ImGui_ImplOpenGL3_Shutdown();
#else
    ImGui_ImplOpenGL2_Shutdown();
#endif
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    _running = false;
    std::cout << "👋 Engine shut down." << std::endl;
}

} // namespace Core 