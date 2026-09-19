#include "KeyboardHandler.hpp"
#include <algorithm>
#if !defined(IMGUI_DISABLE) && !defined(HEADLESS)
#include <imgui.h>
#endif

namespace {
// Shell navigation is not world input. These keys open/close Earthcall's
// first-mover surfaces and should remain available while a panel merely has
// keyboard focus. Letter shortcuts still defer to an active text editor so
// typing "home" into Chat does not close Chat on the H.
bool isShellToggleShortcut(int key) {
    switch (key) {
        case GLFW_KEY_M:              // main menu
        case GLFW_KEY_H:              // chat
        case GLFW_KEY_K:              // keymap
        case GLFW_KEY_F3:             // performance metrics
        case GLFW_KEY_F8:             // creator console
        case GLFW_KEY_F9:             // singular set-to-set creation
        case GLFW_KEY_F10:            // IDE mode
        case GLFW_KEY_GRAVE_ACCENT:   // developer tools
            return true;
        default:
            return false;
    }
}
}

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

    bool imguiWantsKeyboard = false;
    bool imguiWantsTextInput = false;
#if !defined(IMGUI_DISABLE) && !defined(HEADLESS)
    // Tests and non-window utilities may construct KeyboardHandler without an
    // ImGui context. The input layer should still be usable there.
    if (ImGui::GetCurrentContext()) {
        const ImGuiIO& io = ImGui::GetIO();
        imguiWantsKeyboard = io.WantCaptureKeyboard;
        imguiWantsTextInput = io.WantTextInput;
    }
#endif

    const bool escape = key == GLFW_KEY_ESCAPE;
    const bool shellToggle = isShellToggleShortcut(key);
    const bool letterShellToggle = key == GLFW_KEY_M || key == GLFW_KEY_H || key == GLFW_KEY_K;

    // Distinguish "a shell window owns keyboard navigation" from "the Person
    // is typing text". The old code blocked H/K/M whenever ImGui captured the
    // keyboard, so Chat could be opened with H and then refuse the same H used
    // to close it. Function-key shell toggles remain globally reachable as
    // before; letter toggles remain reachable unless a text field actually
    // wants characters.
    if (imguiWantsKeyboard) {
        const bool mayPassShell = shellToggle && (!letterShellToggle || !imguiWantsTextInput);
        if (!escape && !mayPassShell) return;
    }

    // A toggle has to be able to undo itself. Previously M opened the menu,
    // then `_menuOpen` blocked M before its toggle callback could close it.
    // Other bindings still stay out of the custom menu; its own Menu input
    // router owns those keys while it is open.
    if (_menuOpen && key != GLFW_KEY_ESCAPE && key != GLFW_KEY_M) {
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
