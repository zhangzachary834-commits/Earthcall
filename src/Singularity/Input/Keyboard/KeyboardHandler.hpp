#pragma once
#include <map>
#include <functional>
#include <string>
#include <GLFW/glfw3.h>

class KeyboardHandler {
public:
    enum class KeyState {
        Released,
        Pressed,
        Held,
        JustPressed
    };

    struct KeyBinding {
        int key;
        std::string action;
        std::function<void()> callback;
        KeyState state = KeyState::Released;
        bool isEnabled = true;
    };

private:
    std::map<int, KeyBinding> _keyBindings;
    std::map<std::string, int> _actionToKey;
    bool _isEnabled = true;
    bool _menuOpen = false;

public:
    KeyboardHandler();
    ~KeyboardHandler();

    // Core functionality
    void update();
    void handleKeyPress(int key);
    void handleKeyRelease(int key);
    
    // Key binding management
    void bindKey(int key, const std::string& action, std::function<void()> callback);
    void unbindKey(int key);
    void unbindAction(const std::string& action);
    void clearBindings();
    
    // State queries
    bool isKeyPressed(int key) const;
    bool isKeyHeld(int key) const;
    bool isKeyJustPressed(int key) const;
    bool isActionTriggered(const std::string& action) const;
    
    // Utility
    void enable() { _isEnabled = true; }
    void disable() { _isEnabled = false; }
    bool isEnabled() const { return _isEnabled; }
    
    // Getters
    const std::map<int, KeyBinding>& getKeyBindings() const { return _keyBindings; }
    int getKeyForAction(const std::string& action) const;
    
    // Menu state management
    bool isMenuOpen() const { return _menuOpen; }
    void setMenuOpen(bool open) { _menuOpen = open; }
};
