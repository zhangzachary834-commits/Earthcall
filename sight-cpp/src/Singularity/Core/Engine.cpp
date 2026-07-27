#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/Game.hpp"

#include "../../../imgui/imgui.h"
#include "../../../imgui/backends/imgui_impl_glfw.h"
#include "../../../imgui/backends/imgui_impl_opengl2.h"

#include "Singularity/Audio/AudioSystem.hpp"

#include <iostream>

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
    ImGui_ImplOpenGL2_Init();
    _running = true;

    // TODO: move OpenGL / ImGui initialisation here later.
    std::cout << "🌟 Engine initialised." << std::endl;

    // Initialize audio system
    Core::Audio::AudioSystem::instance().init();

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
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        game.update(dt);
        game.render();

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

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
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    // Shutdown audio system
    Core::Audio::AudioSystem::instance().shutdown();

    _running = false;
    std::cout << "👋 Engine shut down." << std::endl;
}

} // namespace Core 
