#pragma once

#include <GLFW/glfw3.h>

namespace Core {

// The WebGPU device/surface/renderer, when built with -DEARTHCALL_WEBGPU.
// Deliberately only forward-declared: defining it here would drag webgpu.h into
// every translation unit that includes Engine.hpp, and the default OpenGL build
// must not need the WebGPU headers at all. Null in the OpenGL build.
struct WebGpuBackend;

// Centralised application driver. Responsible for window/context creation,
// main-loop timing, and global shutdown.  (Pure skeleton – implementation
// in Engine.cpp.)
//
// Which GPU backend this drives is a COMPILE-TIME choice, not a runtime one:
// a window is created either with a GL context or with GLFW_NO_API for WebGPU,
// and that cannot be changed afterwards. `make` builds the OpenGL binary and
// `make webgpu-app` builds the WebGPU one from the same sources, so the two can
// be run side by side and compared.
class Engine {
public:
    // Singleton accessor (simple for now; can be replaced later)
    static Engine& instance();

    // Lifecycle -----------------------------------------------------------
    bool init(int argc = 0, char** argv = nullptr);
    void run(class Game& game);
    void tick(class Game& game, float dt);
    void shutdown();

    // Accessors -----------------------------------------------------------
    GLFWwindow* window() const { return _window; }
    bool running()   const { return _running; }

private:
    Engine() = default;                       // use instance()
    ~Engine() = default;
    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;

    GLFWwindow*     _window  = nullptr;
    bool            _running = false;
    WebGpuBackend*  _webgpu  = nullptr; // owned; only allocated in the WebGPU build
};

} // namespace Core 