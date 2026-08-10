#include "KeyboardHandler.hpp"
#include <algorithm>
#if !defined(IMGUI_DISABLE) && !defined(HEADLESS)
#include <imgui.h>
#endif

KeyboardHandler::KeyboardHandler() {
    _isEnabled = true;
}

KeyboardHandler::~KeyboardHandler() {
}

void KeyboardHandler::update() {
    if (!_isEnabled) return;
    
    for (auto& [key, binding] : _keyBindings) {
        if (binding.state == KeyState::JustPressed) {
            binding.state = KeyState::Pressed;
        } else if (binding.state == KeyState::Pressed) {
            binding.state = KeyState::Held;
        }
    }
}

void KeyboardHandler::handleKeyPress(int key) {
    if (!_isEnabled) return;
    
    auto it = _keyBindings.find(key);
    if (it != _keyBindings.end() && it->second.isEnabled) {
        KeyBinding& binding = it->second;
        if (binding.state == KeyState::Released) {
            binding.state = KeyState::JustPressed;
            if (binding.callback) {
                binding.callback();
            }
        }
    }
}

void KeyboardHandler::handleKeyRelease(int key) {
    if (!_isEnabled) return;
    
    auto it = _keyBindings.find(key);
    if (it != _keyBindings.end()) {
        it->second.state = KeyState::Released;
    }
}

void KeyboardHandler::bindKey(int key, const std::string& action, std::function<void()> callback) {
    KeyBinding binding;
    binding.key = key;
    binding.action = action;
    binding.callback = callback;
    binding.state = KeyState::Released;
    binding.isEnabled = true;
    
    _keyBindings[key] = binding;
    _actionToKey[action] = key;
}

void KeyboardHandler::unbindKey(int key) {
    auto it = _keyBindings.find(key);
    if (it != _keyBindings.end()) {
        _actionToKey.erase(it->second.action);
        _keyBindings.erase(it);
    }
}

void KeyboardHandler::unbindAction(const std::string& action) {
    auto it = _actionToKey.find(action);
    if (it != _actionToKey.end()) {
        _keyBindings.erase(it->second);
        _actionToKey.erase(it);
    }
}

void KeyboardHandler::clearBindings() {
    _keyBindings.clear();
    _actionToKey.clear();
}

bool KeyboardHandler::isKeyPressed(int key) const {
    auto it = _keyBindings.find(key);
    if (it != _keyBindings.end()) {
        return it->second.state == KeyState::JustPressed ||
               it->second.state == KeyState::Pressed ||
               it->second.state == KeyState::Held;
    }
    return false;
}

bool KeyboardHandler::isKeyHeld(int key) const {
    auto it = _keyBindings.find(key);
    if (it != _keyBindings.end()) {
        return it->second.state == KeyState::Held;
    }
    return false;
}

bool KeyboardHandler::isKeyJustPressed(int key) const {
    auto it = _keyBindings.find(key);
    if (it != _keyBindings.end()) {
        return it->second.state == KeyState::JustPressed;
    }
    return false;
}

bool KeyboardHandler::isActionTriggered(const std::string& action) const {
    auto it = _actionToKey.find(action);
    if (it != _actionToKey.end()) {
        return isKeyJustPressed(it->second);
    }
    return false;
}

int KeyboardHandler::getKeyForAction(const std::string& action) const {
    auto it = _actionToKey.find(action);
    if (it != _actionToKey.end()) {
        return it->second;
    }
    return -1;
}

void KeyboardHandler::setupDefaultPerspectiveBindings() {
    bindKey(GLFW_KEY_W, "move_forward", []{});
    bindKey(GLFW_KEY_S, "move_backward", []{});
    bindKey(GLFW_KEY_A, "move_left", []{});
    bindKey(GLFW_KEY_D, "move_right", []{});
    bindKey(GLFW_KEY_SPACE, "move_up", []{});
    bindKey(GLFW_KEY_LEFT_SHIFT, "move_down", []{});
    bindKey(GLFW_KEY_ESCAPE, "toggle_cursor", []{});
    bindKey(GLFW_KEY_F1, "perspective_1", []{});
    bindKey(GLFW_KEY_F2, "perspective_2", []{});
    bindKey(GLFW_KEY_F3, "perspective_3", []{});
    bindKey(GLFW_KEY_F4, "perspective_4", []{});
    bindKey(GLFW_KEY_F5, "perspective_5", []{});
}

void KeyboardHandler::setupFirstPersonBindings() {
    bindKey(GLFW_KEY_W, "walk_forward", []{});
    bindKey(GLFW_KEY_S, "walk_backward", []{});
    bindKey(GLFW_KEY_A, "strafe_left", []{});
    bindKey(GLFW_KEY_D, "strafe_right", []{});
    bindKey(GLFW_KEY_SPACE, "jump", []{});
    bindKey(GLFW_KEY_LEFT_SHIFT, "crouch", []{});
    bindKey(GLFW_KEY_LEFT_CONTROL, "sprint", []{});
    bindKey(GLFW_KEY_E, "interact", []{});
    bindKey(GLFW_KEY_Q, "use_item", []{});
    bindKey(GLFW_KEY_R, "reload", []{});
}

void KeyboardHandler::setupThirdPersonBindings() {
    bindKey(GLFW_KEY_W, "move_forward", []{});
    bindKey(GLFW_KEY_S, "move_backward", []{});
    bindKey(GLFW_KEY_A, "turn_left", []{});
    bindKey(GLFW_KEY_D, "turn_right", []{});
    bindKey(GLFW_KEY_SPACE, "jump", []{});
    bindKey(GLFW_KEY_LEFT_SHIFT, "crouch", []{});
    bindKey(GLFW_KEY_LEFT_CONTROL, "sprint", []{});
    bindKey(GLFW_KEY_E, "interact", []{});
    bindKey(GLFW_KEY_Q, "use_item", []{});
    bindKey(GLFW_KEY_R, "reload", []{});
    bindKey(GLFW_KEY_TAB, "switch_target", []{});
}

void KeyboardHandler::setupFreeCameraBindings() {
    bindKey(GLFW_KEY_W, "camera_forward", []{});
    bindKey(GLFW_KEY_S, "camera_backward", []{});
    bindKey(GLFW_KEY_A, "camera_left", []{});
    bindKey(GLFW_KEY_D, "camera_right", []{});
    bindKey(GLFW_KEY_SPACE, "camera_up", []{});
    bindKey(GLFW_KEY_LEFT_SHIFT, "camera_down", []{});
    bindKey(GLFW_KEY_LEFT_CONTROL, "camera_fast", []{});
    bindKey(GLFW_KEY_LEFT_ALT, "camera_slow", []{});
    bindKey(GLFW_KEY_R, "reset_camera", []{});
    bindKey(GLFW_KEY_F, "focus_target", []{});
}

void KeyboardHandler::setupMenuBindings() {
    bindKey(GLFW_KEY_M, "toggle_menu", []{});
    bindKey(GLFW_KEY_ESCAPE, "toggle_cursor_lock", []{});
    bindKey(GLFW_KEY_H, "toggle_chat", []{});
    bindKey(GLFW_KEY_I, "toggle_integration_ui", []{});
    bindKey(GLFW_KEY_T, "toggle_toolbar", []{});
}

void KeyboardHandler::setupCameraBindings() {
    bindKey(GLFW_KEY_W, "camera_forward", []{});
    bindKey(GLFW_KEY_S, "camera_backward", []{});
    bindKey(GLFW_KEY_A, "camera_left", []{});
    bindKey(GLFW_KEY_D, "camera_right", []{});
    bindKey(GLFW_KEY_LEFT_SHIFT, "camera_down", []{});
    bindKey(GLFW_KEY_SPACE, "camera_up", []{});
    bindKey(GLFW_KEY_V, "camera_sprint", []{});
    bindKey(GLFW_KEY_LEFT_ALT, "camera_slow", []{});
    
    bindKey(GLFW_KEY_RIGHT, "manual_offset_right", []{});
    bindKey(GLFW_KEY_LEFT, "manual_offset_left", []{});
    bindKey(GLFW_KEY_PAGE_UP, "manual_offset_up", []{});
    bindKey(GLFW_KEY_PAGE_DOWN, "manual_offset_down", []{});
    bindKey(GLFW_KEY_UP, "manual_offset_forward", []{});
    bindKey(GLFW_KEY_DOWN, "manual_offset_backward", []{});
}

void KeyboardHandler::setupToolBindings() {
    bindKey(GLFW_KEY_1, "perspective_first_person", []{});
    bindKey(GLFW_KEY_2, "perspective_second_person", []{});
    bindKey(GLFW_KEY_3, "perspective_third_person", []{});
    bindKey(GLFW_KEY_F, "toggle_flight", []{});
    bindKey(GLFW_KEY_C, "switch_to_character_zone", []{});
    bindKey(GLFW_KEY_O, "toggle_avatar_demo", []{});
}

void KeyboardHandler::setupPerspectiveBindings() {
    bindKey(GLFW_KEY_1, "perspective_first_person", []{});
    bindKey(GLFW_KEY_2, "perspective_second_person", []{});
    bindKey(GLFW_KEY_3, "perspective_third_person", []{});
}

void KeyboardHandler::setupUtilityBindings() {
    bindKey(GLFW_KEY_Z, "undo", []{});
    bindKey(GLFW_KEY_Y, "redo", []{});
    bindKey(GLFW_KEY_LEFT_SHIFT, "straight_line_mode", []{});
    bindKey(GLFW_KEY_RIGHT_SHIFT, "straight_line_mode", []{});
}

void KeyboardHandler::setupGameBindings() {
    setupMenuBindings();
    setupCameraBindings();
    setupToolBindings();
    setupPerspectiveBindings();
    setupUtilityBindings();
}

// Game input update - to be refactored to use EventBus instead of Game-specific logic
// This method contains Game-specific state tracking and should be migrated
// to use EventBus for cross-component communication.
void KeyboardHandler::updateGameInput(GLFWwindow* window) {
    if (!_isEnabled || !window || _keyBindings.empty()) return;
    
    // Check if any text input is active (ImGui)
    bool anyTextInputActive = false;
#if !defined(IMGUI_DISABLE) && !defined(HEADLESS)
    anyTextInputActive = ImGui::IsAnyItemActive() || ImGui::IsWindowFocused();
#endif
    
    // Menu toggle with M (allowed even when menu is open)
    bool mPressed = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
    static bool mPressedLast = false;
    if (mPressed && !mPressedLast) {
        auto it = _keyBindings.find(GLFW_KEY_M);
        if (it != _keyBindings.end() && it->second.callback) {
            it->second.callback();
        }
    }
    mPressedLast = mPressed;

    // Cursor lock/unlock with Escape
    bool escapePressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    static bool escapePressedLast = false;
    if (!_menuOpen && escapePressed && !escapePressedLast) {
        auto it = _keyBindings.find(GLFW_KEY_ESCAPE);
        if (it != _keyBindings.end() && it->second.callback) {
            it->second.callback();
        }
    }
    escapePressedLast = escapePressed;

    // Chat window toggle with H - only when not typing
    bool hPressed = glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS;
    static bool hPressedLast = false;
    if (!_menuOpen && hPressed && !hPressedLast && !anyTextInputActive) {
        auto it = _keyBindings.find(GLFW_KEY_H);
        if (it != _keyBindings.end() && it->second.callback) {
            it->second.callback();
        }
    }
    hPressedLast = hPressed;

    // Integration UI toggle with I - only when not typing
    bool iPressed = glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS;
    static bool iPressedLast = false;
    if (!_menuOpen && iPressed && !iPressedLast && !anyTextInputActive) {
        auto it = _keyBindings.find(GLFW_KEY_I);
        if (it != _keyBindings.end() && it->second.callback) {
            it->second.callback();
        }
    }
    iPressedLast = iPressed;

    // Toolbar visibility toggle with T - only when not typing
    bool tPressed = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
    static bool tPressedLast = false;
    if (!_menuOpen && tPressed && !tPressedLast && !anyTextInputActive) {
        auto it = _keyBindings.find(GLFW_KEY_T);
        if (it != _keyBindings.end() && it->second.callback) {
            it->second.callback();
        }
    }
    tPressedLast = tPressed;

    // Perspective switching keys 1/2/3 - only when not typing
    if (!_menuOpen && !anyTextInputActive) {
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            auto it = _keyBindings.find(GLFW_KEY_1);
            if (it != _keyBindings.end() && it->second.callback) {
                it->second.callback();
            }
        }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            auto it = _keyBindings.find(GLFW_KEY_2);
            if (it != _keyBindings.end() && it->second.callback) {
                it->second.callback();
            }
        }
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
            auto it = _keyBindings.find(GLFW_KEY_3);
            if (it != _keyBindings.end() && it->second.callback) {
                it->second.callback();
            }
        }
    }

    // Flight toggle (F) only when not typing
    bool fPressed = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    static bool fPressedLast = false;
    if (!_menuOpen && fPressed && !fPressedLast && !anyTextInputActive) {
        auto it = _keyBindings.find(GLFW_KEY_F);
        if (it != _keyBindings.end() && it->second.callback) {
            it->second.callback();
        }
    }
    fPressedLast = fPressed;

    // Quick switch to Character design zone via C key - only when not typing
    bool cPressed = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
    static bool cPressedLast = false;
    if (!_menuOpen && cPressed && !cPressedLast && !anyTextInputActive) {
        auto it = _keyBindings.find(GLFW_KEY_C);
        if (it != _keyBindings.end() && it->second.callback) {
            it->second.callback();
        }
    }
    cPressedLast = cPressed;

    // Avatar demo toggle with O key - only when not typing
    bool oPressed = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;
    static bool oPressedLast = false;
    if (!_menuOpen && oPressed && !oPressedLast && !anyTextInputActive) {
        auto it = _keyBindings.find(GLFW_KEY_O);
        if (it != _keyBindings.end() && it->second.callback) {
            it->second.callback();
        }
    }
    oPressedLast = oPressed;

    // Handle keyboard shortcuts
    bool ctrlPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || 
                      glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    bool zPressed = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
    bool yPressed = glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS;
    
    static bool undoPressedLast = false;
    if (!_menuOpen && ctrlPressed && zPressed && !undoPressedLast && !anyTextInputActive) {
        auto it = _keyBindings.find(GLFW_KEY_Z);
        if (it != _keyBindings.end() && it->second.callback) {
            it->second.callback();
        }
    }
    undoPressedLast = ctrlPressed && zPressed;
    
    static bool redoPressedLast = false;
    if (!_menuOpen && ctrlPressed && yPressed && !redoPressedLast && !anyTextInputActive) {
        auto it = _keyBindings.find(GLFW_KEY_Y);
        if (it != _keyBindings.end() && it->second.callback) {
            it->second.callback();
        }
    }
    redoPressedLast = ctrlPressed && yPressed;
}
