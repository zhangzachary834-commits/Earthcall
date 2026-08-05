#pragma once
#include <map>
#include <functional>
#include <string>
#include <GLFW/glfw3.h>

// Forward declarations
namespace Core { class Game; }

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

    // Game-specific key states for tracking last frame state
    struct GameKeyStates {
        bool mPressedLast = false;
        bool escapePressedLast = false;
        bool hPressedLast = false;
        bool iPressedLast = false;
        bool fPressedLast = false;
        bool undoPressedLast = false;
        bool redoPressedLast = false;
    };

private:
    std::map<int, KeyBinding> _keyBindings;
    std::map<std::string, int> _actionToKey;
    bool _isEnabled = true;
    
    // Game-specific state tracking
    GameKeyStates _gameKeyStates;
    Core::Game* _gameInstance = nullptr;

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
    
    // Game-specific functionality
    void setGameInstance(Core::Game* game) { _gameInstance = game; }
    Core::Game* getGameInstance() const { return _gameInstance; }
    
    // Game key state getters
    bool getMPressedLast() const { return _gameKeyStates.mPressedLast; }
    bool getEscapePressedLast() const { return _gameKeyStates.escapePressedLast; }
    bool getHPressedLast() const { return _gameKeyStates.hPressedLast; }
    bool getIPressedLast() const { return _gameKeyStates.iPressedLast; }
    bool getFPressedLast() const { return _gameKeyStates.fPressedLast; }
    bool getUndoPressedLast() const { return _gameKeyStates.undoPressedLast; }
    bool getRedoPressedLast() const { return _gameKeyStates.redoPressedLast; }
    
    // Game key state setters
    void setMPressedLast(bool state) { _gameKeyStates.mPressedLast = state; }
    void setEscapePressedLast(bool state) { _gameKeyStates.escapePressedLast = state; }
    void setHPressedLast(bool state) { _gameKeyStates.hPressedLast = state; }
    void setIPressedLast(bool state) { _gameKeyStates.iPressedLast = state; }
    void setFPressedLast(bool state) { _gameKeyStates.fPressedLast = state; }
    void setUndoPressedLast(bool state) { _gameKeyStates.undoPressedLast = state; }
    void setRedoPressedLast(bool state) { _gameKeyStates.redoPressedLast = state; }
    
    // Common key bindings for perspectives
    void setupDefaultPerspectiveBindings();
    void setupFirstPersonBindings();
    void setupThirdPersonBindings();
    void setupFreeCameraBindings();
    
    // Game-specific key bindings
    void setupGameBindings();
    void setupMenuBindings();
    void setupCameraBindings();
    void setupToolBindings();
    void setupPerspectiveBindings();
    void setupUtilityBindings();
    
    // Game update function that processes all keyboard input
    void updateGameInput(GLFWwindow* window);
};
