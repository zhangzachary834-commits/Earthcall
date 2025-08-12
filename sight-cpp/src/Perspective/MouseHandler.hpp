#pragma once
#include <glm/glm.hpp>
#include <functional>
#include <map>
#include <string>
#include <GLFW/glfw3.h>

// Forward declarations
namespace Core { class Game; }

class MouseHandler {
public:
    enum class MouseButton {
        Left = GLFW_MOUSE_BUTTON_LEFT,
        Right = GLFW_MOUSE_BUTTON_RIGHT,
        Middle = GLFW_MOUSE_BUTTON_MIDDLE,
        Button4 = GLFW_MOUSE_BUTTON_4,
        Button5 = GLFW_MOUSE_BUTTON_5
    };

    enum class ButtonState {
        Released,
        Pressed,
        Held,
        JustPressed
    };

    struct MouseState {
        glm::vec2 position{0.0f, 0.0f};
        glm::vec2 delta{0.0f, 0.0f};
        glm::vec2 scroll{0.0f, 0.0f};
        std::map<MouseButton, ButtonState> buttonStates;
        bool isCaptured = false;
    };

    struct MouseBinding {
        MouseButton button;
        std::string action;
        std::function<void(const glm::vec2&)> callback;
        ButtonState state = ButtonState::Released;
        bool isEnabled = true;
    };

    // Camera rotation state for cursor functionality
    struct CameraRotationState {
        float yaw = -90.0f;
        float pitch = 0.0f;
        bool firstMouse = true;
        double lastX = 250.0;
        double lastY = 250.0;
        float sensitivity = 0.1f;
    };

private:
    MouseState _currentState;
    MouseState _previousState;
    std::map<MouseButton, MouseBinding> _buttonBindings;
    std::map<std::string, MouseButton> _actionToButton;
    bool _isEnabled = true;
    float _sensitivity = 1.0f;

    // Cursor functionality
    float _cursorX = 0.0f;
    float _cursorY = 0.0f;
    bool _cursorLocked = true;
    bool _menuOpen = false;
    CameraRotationState _cameraRotation;
    Core::Game* _gameInstance = nullptr;

public:
    MouseHandler();
    ~MouseHandler();

    // Core functionality
    void update();
    void handleMouseMove(double xpos, double ypos);
    void handleMouseButton(int button, int action, int mods);
    void handleMouseScroll(double xoffset, double yoffset);
    
    // Button binding management
    void bindButton(MouseButton button, const std::string& action, std::function<void(const glm::vec2&)> callback);
    void unbindButton(MouseButton button);
    void unbindAction(const std::string& action);
    void clearBindings();
    
    // State queries
    bool isButtonPressed(MouseButton button) const;
    bool isButtonHeld(MouseButton button) const;
    bool isButtonJustPressed(MouseButton button) const;
    bool isActionTriggered(const std::string& action) const;
    
    // Mouse state
    const glm::vec2& getPosition() const { return _currentState.position; }
    const glm::vec2& getDelta() const { return _currentState.delta; }
    const glm::vec2& getScroll() const { return _currentState.scroll; }
    
    // Utility
    void enable() { _isEnabled = true; }
    void disable() { _isEnabled = false; }
    bool isEnabled() const { return _isEnabled; }
    void setSensitivity(float sensitivity) { _sensitivity = sensitivity; }
    float getSensitivity() const { return _sensitivity; }
    
    // Capture control
    void captureMouse(GLFWwindow* window);
    void releaseMouse(GLFWwindow* window);
    bool isMouseCaptured() const { return _currentState.isCaptured; }
    
    // Getters
    const MouseState& getCurrentState() const { return _currentState; }
    const std::map<MouseButton, MouseBinding>& getButtonBindings() const { return _buttonBindings; }
    MouseButton getButtonForAction(const std::string& action) const;
    
    // Common mouse bindings for perspectives
    void setupDefaultPerspectiveBindings();
    void setupFirstPersonBindings();
    void setupThirdPersonBindings();
    void setupFreeCameraBindings();

    // Cursor functionality (moved from Game.cpp)
    // Cursor position
    float getCursorX() const { return _cursorX; }
    float getCursorY() const { return _cursorY; }
    void setCursorX(float x) { _cursorX = x; }
    void setCursorY(float y) { _cursorY = y; }
    
    // Cursor lock/unlock
    bool isCursorLocked() const { return _cursorLocked; }
    void setCursorLocked(bool locked) { _cursorLocked = locked; }
    void toggleCursorLock(GLFWwindow* window);
    
    // Camera rotation
    float getYaw() const { return _cameraRotation.yaw; }
    float getPitch() const { return _cameraRotation.pitch; }
    void setYaw(float yaw) { _cameraRotation.yaw = yaw; }
    void setPitch(float pitch) { _cameraRotation.pitch = pitch; }
    void setCameraSensitivity(float sensitivity) { _cameraRotation.sensitivity = sensitivity; }
    float getCameraSensitivity() const { return _cameraRotation.sensitivity; }
    
    // Camera rotation calculation
    glm::vec3 calculateCameraFront() const;
    
    // Window focus handling
    void onWindowFocus(int focused);
    
    // Game instance management
    void setGameInstance(Core::Game* game) { _gameInstance = game; }
    Core::Game* getGameInstance() const { return _gameInstance; }
    
    // Cursor state management
    void resetFirstMouse() { _cameraRotation.firstMouse = true; }
    bool isFirstMouse() const { return _cameraRotation.firstMouse; }
    
    // Menu state check
    bool isMenuOpen() const;
    void setMenuOpen(bool open);
};