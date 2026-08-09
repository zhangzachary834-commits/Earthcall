#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/Game.hpp"

#include "../../../imgui/imgui.h"
#include "../../../imgui/backends/imgui_impl_glfw.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// The GPU backend is selected at compile time — see Engine.hpp for why it cannot
// be a runtime switch. Everything backend-specific in this file is behind this
// one macro, so the two builds share the entire main loop structure.
#ifdef EARTHCALL_WEBGPU
#include "../../../imgui/backends/imgui_impl_wgpu.h"
#include "Rendering/Renderer.hpp"
#include "Rendering/WebGPU/WebGpuContext.hpp"
#include "Rendering/WebGPU/WebGpuRenderer.hpp"
#include "Rendering/WebGPU/WgpuDevice.hpp"
#else
#include "../../../imgui/backends/imgui_impl_opengl2.h"
#endif

#include "Singularity/Audio/AudioSystem.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Core/EventBus.hpp"

#include <iostream>

namespace Core {

#ifdef EARTHCALL_WEBGPU
struct WebGpuBackend {
    wgpu::WindowContext ctx;
    WebGpuRenderer      renderer;
    int lastFbW = 0, lastFbH = 0; // to notice resizes and reconfigure the surface
};
#endif

Engine& Engine::instance() {
    static Engine s_instance;
    return s_instance;
}

bool Engine::init(int /*argc*/, char** /*argv*/) {
    std::cout << "Engine::init starting" << std::endl;
    if (glfwInit() == GLFW_FALSE) {
        std::cerr << "⚠️  Failed to initialise GLFW!" << std::endl;
        return false;
    }

#ifdef EARTHCALL_WEBGPU
    // WebGPU owns presentation. A GL context on the same window would fight it,
    // so the window is created with no client API at all.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

    std::cout << "Engine::init creating window" << std::endl;
    _window = glfwCreateWindow(1280, 720, "Earthcall", nullptr, nullptr);
    if (!_window) {
        std::cerr << "⚠️  Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return false;
    }

    std::cout << "Engine::init window created" << std::endl;

#ifdef EARTHCALL_WEBGPU
    std::cout << "Engine::init creating WebGpuBackend" << std::endl;
    _webgpu = new WebGpuBackend();
    std::cout << "Engine::init calling createWindowContext" << std::endl;
    _webgpu->ctx = wgpu::createWindowContext(_window);
    std::cout << "Engine::init createWindowContext returned" << std::endl;
    if (!_webgpu->ctx.valid()) {
        std::cerr << "⚠️  Failed to bring up WebGPU!" << std::endl;
        glfwDestroyWindow(_window); _window = nullptr;
        glfwTerminate();
        return false;
    }

    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(_window, &fbw, &fbh);
    wgpu::configureSurface(_webgpu->ctx, static_cast<uint32_t>(fbw), static_cast<uint32_t>(fbh));
    _webgpu->lastFbW = fbw;
    _webgpu->lastFbH = fbh;

    // WebGpuRenderer wants the WgpuDevice-shaped bundle; WindowContext already
    // holds exactly those handles, just obtained via a surface-compatible adapter.
    wgpu::Device gpu;
    gpu.instance = _webgpu->ctx.instance;
    gpu.adapter  = _webgpu->ctx.adapter;
    gpu.device   = _webgpu->ctx.device;
    gpu.queue    = _webgpu->ctx.queue;
    if (!_webgpu->renderer.init(gpu, wgpu::kSurfaceFormat)) {
        std::cerr << "⚠️  Failed to initialise WebGpuRenderer!" << std::endl;
        return false;
    }
    _webgpu->renderer.attachSurface(_webgpu->ctx.surface, _webgpu->ctx.instance);

    // From here every draw in the app lands on WebGPU.
    setCurrentRenderer(&_webgpu->renderer);
#else
    glfwMakeContextCurrent(_window);
#endif

    // Lock cursor initially so camera control behaves consistently
#ifndef __EMSCRIPTEN__
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#endif
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

#ifdef EARTHCALL_WEBGPU
    // InitForOther, not InitForOpenGL: there is no GL context to tie to.
    ImGui_ImplGlfw_InitForOther(_window, true);
    ImGui_ImplWGPU_InitInfo wgpuInit;
    wgpuInit.Device = _webgpu->ctx.device;
    wgpuInit.NumFramesInFlight = 3;
    wgpuInit.RenderTargetFormat = wgpu::kSurfaceFormat;
    // No depth: imgui composites in an overlay pass that has no depth attachment.
    wgpuInit.DepthStencilFormat = WGPUTextureFormat_Undefined;
    if (!ImGui_ImplWGPU_Init(&wgpuInit)) {
        std::cerr << "⚠️  Failed to initialise ImGui WebGPU backend!" << std::endl;
        return false;
    }
#else
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL2_Init();
#endif
    _running = true;

    std::cout << "🌟 Engine initialised ("
#ifdef EARTHCALL_WEBGPU
              << "WebGPU"
#else
              << "OpenGL"
#endif
              << ")." << std::endl;

    // Initialize audio system
    Core::Audio::AudioSystem::instance().init();
    Core::Audio::AudioSystem::instance().setupAudioEventListeners();

    return true;
}

#ifdef __EMSCRIPTEN__
struct LoopContext {
    Engine* engine;
    Game* game;
    double lastTime;
};

static void emscripten_main_loop(void* arg) {
    LoopContext* ctx = static_cast<LoopContext*>(arg);
    if (!ctx->engine->running() || !ctx->engine->window() || glfwWindowShouldClose(ctx->engine->window())) {
        emscripten_cancel_main_loop();
        ctx->engine->shutdown();
        delete ctx;
        return;
    }
    
    double currentTime = glfwGetTime();
    float dt = static_cast<float>(currentTime - ctx->lastTime);
    ctx->lastTime = currentTime;
    
    ctx->engine->tick(*(ctx->game), dt);
}
#endif

void Engine::run(Game& game) {
    double lastTime = glfwGetTime();

#ifdef __EMSCRIPTEN__
    LoopContext* ctx = new LoopContext{this, &game, lastTime};
    emscripten_set_main_loop_arg(emscripten_main_loop, ctx, 0, 1);
#else
    while (_running && _window && !glfwWindowShouldClose(_window)) {
        double currentTime = glfwGetTime();
        float  dt          = static_cast<float>(currentTime - lastTime);
        lastTime           = currentTime;
        
        tick(game, dt);
    }
    // Allow Game to perform shutdown logic before engine terminates
    game.shutdown();
#endif
}

void Engine::tick(Game& game, float dt) {
        glfwPollEvents();

#ifdef EARTHCALL_WEBGPU
        // A swapchain is sized: presenting against a stale size gives a suboptimal
        // or failed surface texture, so track the framebuffer and reconfigure.
        {
            int fbw = 0, fbh = 0;
            glfwGetFramebufferSize(_window, &fbw, &fbh);
            if (fbw != _webgpu->lastFbW || fbh != _webgpu->lastFbH) {
                _webgpu->lastFbW = fbw;
                _webgpu->lastFbH = fbh;
                wgpu::configureSurface(_webgpu->ctx, static_cast<uint32_t>(fbw),
                                       static_cast<uint32_t>(fbh));
            }
            if (fbw == 0 || fbh == 0) return; // minimised: nothing to draw into
        }
        ImGui_ImplWGPU_NewFrame();
#else
        ImGui_ImplOpenGL2_NewFrame();
#endif
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Tick language modality
        Singularity::Language::LanguageSystem::instance().tick(dt);
        Core::Audio::AudioSystem::instance().tick();
#ifdef __EMSCRIPTEN__
        Core::EventBus::instance().tick();
#endif

        game.update(dt);
        game.render(); // brackets itself with Renderer begin/endFrame

        // Render ImGui
        ImGui::Render();
#ifdef EARTHCALL_WEBGPU
        // game.render() already closed the scene pass, but the acquired surface
        // texture is still held — so imgui composites in a second load-op pass on
        // top of it, and only then does the frame get presented.
        _webgpu->renderer.overlayPass([](WGPURenderPassEncoder pass) {
            ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
        });
        _webgpu->renderer.present();
#else
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(_window);
#endif
}

void Engine::shutdown() {
    if (_window) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
    // Shutdown ImGui after window destruction but before GLFW termination
#ifdef EARTHCALL_WEBGPU
    ImGui_ImplWGPU_Shutdown();
#else
    ImGui_ImplOpenGL2_Shutdown();
#endif
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

#ifdef EARTHCALL_WEBGPU
    if (_webgpu) {
        // Drop the renderer's GPU objects before the device that owns them, and
        // unhook it first so nothing can draw through a dead backend.
        setCurrentRenderer(nullptr);
        _webgpu->renderer.shutdown();
        wgpu::destroyWindowContext(_webgpu->ctx);
        delete _webgpu;
        _webgpu = nullptr;
    }
#endif

    glfwTerminate();

    // Shutdown audio system
    Core::Audio::AudioSystem::instance().shutdown();
    
    // Clear Language Lexemes
    Singularity::Language::LanguageSystem::instance().clear();

    _running = false;
    std::cout << "👋 Engine shut down." << std::endl;
}

} // namespace Core
