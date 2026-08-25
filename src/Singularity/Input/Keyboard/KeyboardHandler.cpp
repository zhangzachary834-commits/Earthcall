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

