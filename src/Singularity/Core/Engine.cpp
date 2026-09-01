#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/PerformanceMetricsWindow.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Screen/Camera.hpp"
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
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/WebGPU/WebGpuContext.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#else
#include "../../../imgui/backends/imgui_impl_opengl2.h"
#endif

#include "Singularity/Audio/AudioSystem.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Network/WebSocketServer.hpp"
#include "Person/Person.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include "Singularity/Input/Keyboard/KeyboardHandler.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CursorTools.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/Chat.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/ElementalToolHandler.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleWindow.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp"
#include "Singularity/Screen/LawGraphWindow.hpp"
#include "Singularity/Screen/DeveloperToolsWindow.hpp"
#include "Singularity/Screen/CreationWindow.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Core/Logger.hpp"

#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleWindow.hpp"
#include <iostream>
#include <filesystem>
#include <vector>
#include <climits>
#include <chrono>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

extern ZoneManager mgr;

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

namespace {
std::filesystem::path findRepoRoot() {
    std::error_code ec;
    std::vector<std::filesystem::path> seeds;
    if (auto cwd = std::filesystem::current_path(ec); !ec) seeds.push_back(cwd);
#ifdef __APPLE__
    char buf[PATH_MAX];
    uint32_t sz = sizeof(buf);
    if (_NSGetExecutablePath(buf, &sz) == 0) {
        auto exe = std::filesystem::weakly_canonical(std::filesystem::path(buf), ec);
        if (!ec) seeds.push_back(exe.parent_path());
    }
#endif
    for (auto dir : seeds) {
        for (int i = 0; i < 8 && !dir.empty() && dir != dir.root_path(); ++i) {
            if (std::filesystem::exists(dir / "AGENTS.md", ec) &&
                std::filesystem::is_directory(dir / "saves", ec)) {
                return dir;
            }
            dir = dir.parent_path();
        }
    }
    return {};
}
} // namespace

bool Engine::init(int /*argc*/, char** /*argv*/) {
    ECA::Logger::instance().log(ECA::LogCategory::System, "SYSTEM", "Engine::init starting");
    std::cout << "Engine::init starting" << std::endl;
    {
        const auto root = findRepoRoot();
        if (!root.empty()) {
            std::error_code ec;
            std::filesystem::current_path(root, ec);
            const auto saves = (root / "saves").string();
            SaveSystem::setSaveRoot(saves);
            std::cout << "[Engine] repo root: " << root.string()
                      << "  saves: " << saves << std::endl;
        }
    }
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

    ECA::Logger::instance().log(ECA::LogCategory::System, "SYSTEM", "Engine initialised successfully");
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

    initLogic();

    return true;
}

#ifdef __EMSCRIPTEN__
struct LoopContext {
    Engine* engine;
    double lastTime;
};

static void emscripten_main_loop(void* arg) {
    LoopContext* ctx = static_cast<LoopContext*>(arg);
    if (!ctx->engine->running() || !ctx->engine->window() || glfwWindowShouldClose(ctx->engine->window())) {
        emscripten_cancel_main_loop();
        // Mirror the native loop-exit path below (Engine::run's #else arm):
        // Game must get a chance to flush, final-save, or unsubscribe events
        // before the engine tears down. This call was missing here, so any
        // such teardown work Game does was silently skipped on close, but
        // only in the browser -- entry.cpp's own post-loop call is
        // unreachable in this build (emscripten_set_main_loop_arg with
        // simulate_infinite_loop=1 never returns, it unwinds the stack).
        ctx->engine->shutdown();
        delete ctx;
        return;
    }
    
    double currentTime = glfwGetTime();
    float dt = static_cast<float>(currentTime - ctx->lastTime);
    ctx->lastTime = currentTime;
    
    ctx->engine->tick(dt);
}
#endif

void Engine::run() {
    double lastTime = glfwGetTime();

#ifdef __EMSCRIPTEN__
    LoopContext* ctx = new LoopContext{this, lastTime};
    emscripten_set_main_loop_arg(emscripten_main_loop, ctx, 0, 1);
#else
    while (_running && _window && !glfwWindowShouldClose(_window)) {
        double currentTime = glfwGetTime();
        float  dt          = static_cast<float>(currentTime - lastTime);
        lastTime           = currentTime;
        
        tick(dt);
    }
    // Allow Game to perform shutdown logic before engine terminates
    
#endif
}

void Engine::tick(float dt) {
    using clock = std::chrono::steady_clock;
    auto tickStart = clock::now();
    auto getMs = [](clock::time_point start, clock::time_point end) {
        return std::chrono::duration<float, std::milli>(end - start).count();
    };

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

    // 1. Process physical & logical simulation (populates input, locomotion, creation, interaction, zone)
    update(dt);

    // 2. Tick language modality
    auto tLang0 = clock::now();
    Singularity::Language::LanguageSystem::instance().tick(dt);
    auto tLang1 = clock::now();
    g_frameTimings.language_ms = getMs(tLang0, tLang1);

    // 3. Audio & event modality
    auto tAudio0 = clock::now();
    Core::Audio::AudioSystem::instance().tick();
#ifdef __EMSCRIPTEN__
    Core::EventBus::instance().tick();
#endif
    auto tAudio1 = clock::now();
    g_frameTimings.audio_ms = getMs(tAudio0, tAudio1);

    // The Person is the canonical source of "where the Person is looking
    // from" (see tests/basic_cube_law_test.cpp's comment on why placement
    // law text reads player.cameraPos, not the Camera object) -- but
    // Camera is what the render path actually moves, so it has to be
    // copied across each frame or Person's copy goes stale at its
    // construction-time default.
    if (Person* p = getPerson()) {
        if (Camera* cam = getCamera()) {
            p->cameraPos = cam->getPos();
            p->cameraForward = cam->getFront();
        }
    }

    // 4. Laws: LawManager::tick() drains the Rete agenda queued by events published this frame.
    auto tLaws0 = clock::now();
    if (_lawManager) _lawManager->tick();
    auto tLaws1 = clock::now();
    g_frameTimings.laws_ms = getMs(tLaws0, tLaws1);

    // Interaction reticle
    if (_lawManager) {
        if (auto* interaction = Singularity::Input::InteractionChannel::find(*_lawManager)) {
            if (interaction->pointerLocked) {
                ImGuiIO& rio = ImGui::GetIO();
                const ImVec2 center(rio.DisplaySize.x * 0.5f, rio.DisplaySize.y * 0.5f);
                ImDrawList* dl = ImGui::GetForegroundDrawList();
                constexpr float half = 6.0f;
                const ImU32 col = IM_COL32(255, 255, 255, 200);
                dl->AddLine(ImVec2(center.x - half, center.y), ImVec2(center.x + half, center.y), col, 1.5f);
                dl->AddLine(ImVec2(center.x, center.y - half), ImVec2(center.x, center.y + half), col, 1.5f);
            }
        }
    }

    // 5. ImGui Windows
    Rendering::renderDeveloperToolsWindow(&_devToolsWindowOpen, _window, this);
    Rendering::renderPerformanceMetricsWindow(&_performanceMetricsWindowOpen, this);

    if (_creationConsoleOpen) {
        Rendering::renderCreationWindow(&_creationConsoleOpen, *_person, nullptr, mgr.active());
    }

    if (_creatorConsoleOpen) {
        Rendering::renderCreatorConsoleWindow(
            &_creatorConsoleOpen, _person.get(),
            Rendering::getCreatorConsoleState().selectedObject3D,
            mgr, _window, this);
    }

    Rendering::renderSaveLoadWindows(this);

    if (_showKeymapWindow) {
        ImGui::SetNextWindowSize(ImVec2(420, 420), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Controls / Keymap", &_showKeymapWindow,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("Core");
            ImGui::Separator();
            ImGui::BulletText("M: Toggle Main Menu");
            ImGui::BulletText("Esc: Toggle Cursor Lock");
            ImGui::BulletText("H: Toggle Chat");
            ImGui::BulletText("K: Controls / Keymap");
            ImGui::BulletText("`: Toggle Dev Tools");
            ImGui::BulletText("F8: Creator Console");
            ImGui::BulletText("F9: Singular Set-to-Set Creation");
            ImGui::BulletText("C: Character Architect Forge Zone");
            ImGui::Separator();
            ImGui::TextUnformatted("Saves");
            ImGui::Separator();
            ImGui::BulletText("S: Quick Save (from the menu)");
            ImGui::BulletText("A: Save As...  L: Load  G: Save Manager");
            ImGui::Separator();
            ImGui::TextUnformatted("Camera");
            ImGui::Separator();
            ImGui::BulletText("WASD: Move");
            ImGui::BulletText("Space: Up");
            ImGui::BulletText("Shift: Down");
            ImGui::BulletText("V: Sprint");
            ImGui::BulletText("Alt: Slow");
            ImGui::Separator();
            ImGui::TextUnformatted("Create");
            ImGui::Separator();
            ImGui::BulletText("L: Arm 3D create law (when the menu is closed)");
            ImGui::BulletText("F4: 3D Create tab   F5: 3D Select tab");
        }
        ImGui::End();
    }

    if (_showChatWindow && _chat) {
        _chat->renderUI(&_showChatWindow);
    }

    if (_showImGuiDemo) {
        ImGui::ShowDemoWindow(&_showImGuiDemo);
    }

    if (Rendering::getCreatorConsoleState().showLawAuthor) {
        Singular* testSubject = _lawManager
            ? Singularity::Core::CreationChannel::find(*_lawManager)
            : nullptr;
        Rendering::renderLawGraphWindow(&Rendering::getCreatorConsoleState().showLawAuthor, *_lawManager, *_person, testSubject);
    }

    // 6. Render 3D scene (must happen before ImGui::Render composites over it)
    auto tRender0 = clock::now();
    render();
    auto tRender1 = clock::now();
    g_frameTimings.render3d_ms = getMs(tRender0, tRender1);

    // 7. Render ImGui & Present / Buffer Swap
    auto tGui0 = clock::now();
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
    auto tGui1 = clock::now();
    g_frameTimings.imgui_ms = getMs(tGui0, tGui1);

    // Total tick elapsed time
    auto tickEnd = clock::now();
    g_frameTimings.total_ms = getMs(tickStart, tickEnd);
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

#ifndef __EMSCRIPTEN__
    // Shutdown WebSocket server
    Singularity::Network::WebSocketServer::instance().stop();
#endif
    
    // Clear Language Lexemes
    Singularity::Language::LanguageSystem::instance().clear();

    _running = false;
    ECA::Logger::instance().log(ECA::LogCategory::System, "SYSTEM", "Engine shut down");
    std::cout << "👋 Engine shut down." << std::endl;
}

} // namespace Core

namespace Core {
MouseHandler* Engine::getMouseHandler() { return _mouseHandler.get(); }
Camera* Engine::getCamera() { return _camera.get(); }
Person* Engine::getPerson() { return _person.get(); }
LawManager* Engine::getLawManager() { return _lawManager.get(); }
KeyboardHandler* Engine::getKeyboardHandler() { return _keyboardHandler.get(); }
CursorTools* Engine::getCursorTools() { return _cursorTools.get(); }
void Engine::ensureCursorUnlocked() {
    if (_mouseHandler && _window && _mouseHandler->isCursorLocked()) {
        _mouseHandler->toggleCursorLock(_window);
    }
}
Engine::~Engine() {}

bool Engine::getRotateDragging() const {
    return Rendering::getCreatorConsoleState().rotateDragging;
}
void Engine::setRotateDragging(bool v) {
    Rendering::getCreatorConsoleState().rotateDragging = v;
}
double Engine::getRotateLastCursorX() const {
    return Rendering::getCreatorConsoleState().rotateLastCursorX;
}
double Engine::getRotateLastCursorY() const {
    return Rendering::getCreatorConsoleState().rotateLastCursorY;
}
void Engine::setRotateLastCursor(double x, double y) {
    auto& s = Rendering::getCreatorConsoleState();
    s.rotateLastCursorX = x;
    s.rotateLastCursorY = y;
}
bool Engine::isAdvancedFacePaintEnabled() const {
    return Rendering::getCreatorConsoleState().advancedFacePaint;
}
float Engine::getCurrentColor(int i) const {
    auto& s = Rendering::getCreatorConsoleState();
    if (i >= 0 && i < 3) return s.createColor[i];
    return 1.0f;
}
float Engine::getRotationToolSensitivity() const {
    return Rendering::getCreatorConsoleState().rotationSensitivity;
}
float Engine::getRotationToolSmoothness() const {
    return Rendering::getCreatorConsoleState().rotationSmoothness;
}
RotationAxisMode Engine::getRotationAxisMode() const {
    switch (Rendering::getCreatorConsoleState().rotationAxisMode) {
        case 1: return RotationAxisMode::X;
        case 2: return RotationAxisMode::Y;
        case 3: return RotationAxisMode::Z;
        case 4: return RotationAxisMode::AuthoritativeAxis;
        default: return RotationAxisMode::FreeXY;
    }
}
PotteryTool Engine::getCurrentPotteryTool() const {
    return Rendering::getCreatorConsoleState().potteryTool == 0
        ? PotteryTool::Pinch : PotteryTool::Expand;
}
float Engine::getPotteryStrength() const {
    return Rendering::getCreatorConsoleState().potteryStrength;
}
double Engine::getCursorX() const {
    return _mouseHandler ? _mouseHandler->getCursorX() : 0.0;
}
double Engine::getCursorY() const {
    return _mouseHandler ? _mouseHandler->getCursorY() : 0.0;
}
float Engine::getFaceBrushUOffset() const {
    return Rendering::getCreatorConsoleState().faceBrushUOffset;
}
float Engine::getFaceBrushVOffset() const {
    return Rendering::getCreatorConsoleState().faceBrushVOffset;
}
}
