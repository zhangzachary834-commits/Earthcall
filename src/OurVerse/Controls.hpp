#pragma once
#include <GLFW/glfw3.h>

// Forward declarations
namespace Core { class Game; }

namespace ControlPanel { class Controls; }

namespace ControlPanel {

class Controls {
public:
    // Initialize controls, e.g. key bindings
    void init();

    // Update controls based on input
    void update(float dt, Core::Game* game, GLFWwindow* window);

    // Handle key press events
    static void onKeyPress(int key, int action);

    // Handle mouse movement
    static void onMouseMove(double xpos, double ypos);

    // Handle mouse button events
    static void onMouseButton(int button, int action);

private:
    Controls();
};

}