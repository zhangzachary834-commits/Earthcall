#include "MouseHandler.hpp"
#include <algorithm>
#include <imgui.h>

MouseHandler::MouseHandler() {
    _currentState.buttonStates[MouseButton::Left] = ButtonState::Released;
    _currentState.buttonStates[MouseButton::Right] = ButtonState::Released;
    _currentState.buttonStates[MouseButton::Middle] = ButtonState::Released;
    _currentState.buttonStates[MouseButton::Button4] = ButtonState::Released;
    _currentState.buttonStates[MouseButton::Button5] = ButtonState::Released;
    
    _previousState = _currentState;
    
    _cursorX = 0.0f;
    _cursorY = 0.0f;
    _cursorLocked = true;
    _cameraRotation = CameraRotationState();
    _menuOpen = false;
}

MouseHandler::~MouseHandler() {
}

void MouseHandler::primeCursorBaseline(GLFWwindow* window) {
    double xpos = _cameraRotation.lastX;
    double ypos = _cameraRotation.lastY;
    if (window) {
        glfwGetCursorPos(window, &xpos, &ypos);
    }

    _currentState.position = {static_cast<float>(xpos), static_cast<float>(ypos)};
    _previousState.position = _currentState.position;
    _currentState.delta = {0.0f, 0.0f};
    _previousState.delta = {0.0f, 0.0f};
    _cursorX = static_cast<float>(xpos);
    _cursorY = static_cast<float>(ypos);
    _cameraRotation.lastX = xpos;
    _cameraRotation.lastY = ypos;
    _cameraRotation.firstMouse = true;
}

void MouseHandler::update() {
    if (!_isEnabled) return;
    
    for (auto& [button, state] : _currentState.buttonStates) {
        if (state == ButtonState::JustPressed) {
            state = ButtonState::Pressed;
        }
    }
    
    _previousState = _currentState;
    _currentState.delta = {0.0f, 0.0f};
    _currentState.scroll = {0.0f, 0.0f};
}

void MouseHandler::handleMouseMove(double xpos, double ypos) {
    if (!_isEnabled) return;
    
    if (std::isnan(xpos) || std::isnan(ypos) || std::isinf(xpos) || std::isinf(ypos)) {
        return;
    }
    
    _currentState.position.x = static_cast<float>(xpos);
    _currentState.position.y = static_cast<float>(ypos);
    
    _currentState.delta.x = _currentState.position.x - _previousState.position.x;
    _currentState.delta.y = _currentState.position.y - _previousState.position.y;
    
    _currentState.delta *= _sensitivity;
    
    _cursorX = static_cast<float>(xpos);
    _cursorY = static_cast<float>(ypos);
    
    if (_cursorLocked && !_menuOpen) {
        if (_cameraRotation.firstMouse) {
            _cameraRotation.lastX = xpos;
            _cameraRotation.lastY = ypos;
            _cameraRotation.firstMouse = false;
        }

        float xoffset = static_cast<float>((xpos - _cameraRotation.lastX) * _cameraRotation.sensitivity);
        float yoffset = static_cast<float>((_cameraRotation.lastY - ypos) * _cameraRotation.sensitivity);
        _cameraRotation.lastX = xpos;
        _cameraRotation.lastY = ypos;

        _cameraRotation.yaw += xoffset;
        _cameraRotation.pitch += yoffset;
        
        if (_cameraRotation.pitch > 89.0f) _cameraRotation.pitch = 89.0f;
        if (_cameraRotation.pitch < -89.0f) _cameraRotation.pitch = -89.0f;
        
        while (_cameraRotation.yaw > 360.0f) _cameraRotation.yaw -= 360.0f;
        while (_cameraRotation.yaw < -360.0f) _cameraRotation.yaw += 360.0f;
    }
}

void MouseHandler::handleMouseButton(int button, int action, int mods) {
    (void)mods;
    if (!_isEnabled) return;
    
    MouseButton mouseButton = static_cast<MouseButton>(button);
    
    if (_currentState.buttonStates.find(mouseButton) == _currentState.buttonStates.end()) {
        _currentState.buttonStates[mouseButton] = ButtonState::Released;
    }
    
    if (action == GLFW_PRESS) {
        _currentState.buttonStates[mouseButton] = ButtonState::JustPressed;
        
        auto it = _buttonBindings.find(mouseButton);
        if (it != _buttonBindings.end() && it->second.isEnabled && it->second.callback) {
            it->second.callback(_currentState.position);
        }
    } else if (action == GLFW_RELEASE) {
        _currentState.buttonStates[mouseButton] = ButtonState::Released;
    }
}

void MouseHandler::handleMouseScroll(double xoffset, double yoffset) {
    if (!_isEnabled) return;
    
    _currentState.scroll.x = static_cast<float>(xoffset);
    _currentState.scroll.y = static_cast<float>(yoffset);
}

void MouseHandler::bindButton(MouseButton button, const std::string& action, std::function<void(const glm::vec2&)> callback) {
    MouseBinding binding;
    binding.button = button;
    binding.action = action;
    binding.callback = callback;
    binding.state = ButtonState::Released;
    binding.isEnabled = true;
    
    _buttonBindings[button] = binding;
    _actionToButton[action] = button;
}

void MouseHandler::unbindButton(MouseButton button) {
    auto it = _buttonBindings.find(button);
    if (it != _buttonBindings.end()) {
        _actionToButton.erase(it->second.action);
        _buttonBindings.erase(it);
    }
}

void MouseHandler::unbindAction(const std::string& action) {
    auto it = _actionToButton.find(action);
    if (it != _actionToButton.end()) {
        _buttonBindings.erase(it->second);
        _actionToButton.erase(it);
    }
}

void MouseHandler::clearBindings() {
    _buttonBindings.clear();
    _actionToButton.clear();
}

bool MouseHandler::isButtonPressed(MouseButton button) const {
    auto it = _currentState.buttonStates.find(button);
    if (it != _currentState.buttonStates.end()) {
        return it->second == ButtonState::JustPressed || it->second == ButtonState::Pressed;
    }
    return false;
}

bool MouseHandler::isButtonHeld(MouseButton button) const {
    auto it = _currentState.buttonStates.find(button);
    if (it != _currentState.buttonStates.end()) {
        return it->second == ButtonState::Pressed;
    }
    return false;
}

bool MouseHandler::isButtonJustPressed(MouseButton button) const {
    auto it = _currentState.buttonStates.find(button);
    if (it != _currentState.buttonStates.end()) {
        return it->second == ButtonState::JustPressed;
    }
    return false;
}

bool MouseHandler::isActionTriggered(const std::string& action) const {
    auto it = _actionToButton.find(action);
    if (it != _actionToButton.end()) {
        return isButtonJustPressed(it->second);
    }
    return false;
}

void MouseHandler::captureMouse(GLFWwindow* window) {
    if (window && !_currentState.isCaptured) {
        primeCursorBaseline(window);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        _currentState.isCaptured = true;
        _cursorLocked = true;
        primeCursorBaseline(window);
    }
}

void MouseHandler::releaseMouse(GLFWwindow* window) {
    if (window && _currentState.isCaptured) {
        primeCursorBaseline(window);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        _currentState.isCaptured = false;
        _cursorLocked = false;
        primeCursorBaseline(window);
    }
}

void MouseHandler::toggleCursorLock(GLFWwindow* window) {
    if (!window) {
        _cursorLocked = !_cursorLocked;
        _currentState.isCaptured = _cursorLocked;
        _cameraRotation.firstMouse = true;
        return;
    }

    primeCursorBaseline(window);
    _cursorLocked = !_cursorLocked;
    if (_cursorLocked) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    _currentState.isCaptured = _cursorLocked;
    primeCursorBaseline(window);
}

glm::vec3 MouseHandler::calculateCameraFront() const {
    glm::vec3 direction;
    
    float yawRad = glm::radians(_cameraRotation.yaw);
    float pitchRad = glm::radians(_cameraRotation.pitch);
    
    pitchRad = glm::clamp(pitchRad, glm::radians(-89.0f), glm::radians(89.0f));
    
    direction.x = cos(yawRad) * cos(pitchRad);
    direction.y = sin(pitchRad);
    direction.z = sin(yawRad) * cos(pitchRad);
    
    if (glm::length(direction) > 1e-6f) {
        return glm::normalize(direction);
    } else {
        return glm::vec3(0.0f, 0.0f, -1.0f);
    }
}

void MouseHandler::onWindowFocus(int focused) {
    if (focused) {
        ImGuiIO& io = ImGui::GetIO();
        for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); ++i) io.MouseDown[i] = false;
        primeCursorBaseline(nullptr);
    }
}

MouseHandler::MouseButton MouseHandler::getButtonForAction(const std::string& action) const {
    auto it = _actionToButton.find(action);
    if (it != _actionToButton.end()) {
        return it->second;
    }
    return MouseButton::Left;
}

void MouseHandler::setupDefaultPerspectiveBindings() {
    bindButton(MouseButton::Left, "select", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Right, "context_menu", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Middle, "pan", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Button4, "previous", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Button5, "next", [](const glm::vec2& pos){(void)pos;});
}

void MouseHandler::setupFirstPersonBindings() {
    bindButton(MouseButton::Left, "primary_action", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Right, "secondary_action", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Middle, "tertiary_action", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Button4, "weapon_prev", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Button5, "weapon_next", [](const glm::vec2& pos){(void)pos;});
}

void MouseHandler::setupThirdPersonBindings() {
    bindButton(MouseButton::Left, "select_target", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Right, "move_to", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Middle, "rotate_camera", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Button4, "previous_target", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Button5, "next_target", [](const glm::vec2& pos){(void)pos;});
}

void MouseHandler::setupFreeCameraBindings() {
    bindButton(MouseButton::Left, "select_object", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Right, "orbit_camera", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Middle, "pan_camera", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Button4, "previous_object", [](const glm::vec2& pos){(void)pos;});
    bindButton(MouseButton::Button5, "next_object", [](const glm::vec2& pos){(void)pos;});
}

bool MouseHandler::isMenuOpen() const {
    return _menuOpen;
}

void MouseHandler::setMenuOpen(bool open) {
    _menuOpen = open;
}
