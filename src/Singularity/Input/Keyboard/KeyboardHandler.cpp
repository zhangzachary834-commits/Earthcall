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
    
    // Check if ImGui wants to capture keyboard (safe to call outside NewFrame/Render)
    bool imguiWantsKeyboard = false;
#if !defined(IMGUI_DISABLE) && !defined(HEADLESS)
    imguiWantsKeyboard = ImGui::GetIO().WantCaptureKeyboard;
#endif

    // Escape, F8 (Creator Console), and Grave Accent (Dev Tools) can always bypass ImGui capture
    if (imguiWantsKeyboard && 
        key != GLFW_KEY_ESCAPE && 
        key != GLFW_KEY_F8 && 
        key != GLFW_KEY_F9 && 
        key != GLFW_KEY_GRAVE_ACCENT) {
        return;
    }
    
    // Check _menuOpen logic inside the keybind itself, or just allow the callbacks to decide.
    // wait, previously, most keys (except Esc) checked `!_menuOpen`!
    // Since we don't have access to _menuOpen directly in handleKeyPress, we shouldn't worry about it,
    // the callbacks themselves can check if the menu is open! Wait, they don't right now.
    // Actually, KeyboardHandler has `_menuOpen`!
    if (_menuOpen && key != GLFW_KEY_ESCAPE) {
        return;
    }

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
    // Left empty: all edge detection has been migrated to handleKeyPress
    // as per the "Edges, not levels" architecture principle.
}
