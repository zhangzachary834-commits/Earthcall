#pragma once

#include <GLFW/glfw3.h>

namespace Core {

// Centralised application driver. Responsible for window/context creation,
// main-loop timing, and global shutdown.  (Pure skeleton – implementation
// in Engine.cpp.)
class Engine {
public:
    // Singleton accessor (simple for now; can be replaced later)
    static Engine& instance();

    // Lifecycle -----------------------------------------------------------
    bool init(int argc = 0, char** argv = nullptr);
    void run(class Game& game);
    void shutdown();

    // Accessors -----------------------------------------------------------
    GLFWwindow* window() const { return _window; }
    bool running()   const { return _running; }

private:
    Engine() = default;                       // use instance()
    ~Engine() = default;
    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;

    GLFWwindow* _window = nullptr;
    bool        _running = false;
};

} // namespace Core 