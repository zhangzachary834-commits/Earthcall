#include "../src/Core/Game.hpp"
#include "../src/Perspective/KeyboardHandler.hpp"
#include <iostream>

// Example demonstrating the refactored keyboard handling
// This shows how keyboard functionality was moved from Game.cpp to KeyboardHandler

int main() {
    std::cout << "=== Keyboard Handler Refactor Example ===" << std::endl;
    
    // Create a game instance
    Core::Game game;
    
    // Initialize the game (this will set up the keyboard handler)
    if (!game.init()) {
        std::cout << "❌ Failed to initialize game" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Game initialized successfully" << std::endl;
    
    // Access the keyboard handler
    KeyboardHandler& keyboard = game.getKeyboardHandler();
    
    std::cout << "\n📋 Keyboard Handler Status:" << std::endl;
    std::cout << "  - Enabled: " << (keyboard.isEnabled() ? "Yes" : "No") << std::endl;
    std::cout << "  - Handler Count: " << keyboard.getHandlerCount() << std::endl;
    
    // Show registered handlers
    std::cout << "\n🔧 Registered Key Bindings:" << std::endl;
    const auto& bindings = keyboard.getKeyBindings();
    for (const auto& [key, binding] : bindings) {
        std::cout << "  - Key " << key << " -> " << binding.action << std::endl;
    }
    
    // Demonstrate key state tracking
    std::cout << "\n🎮 Key State Tracking:" << std::endl;
    std::cout << "  - M Pressed Last: " << (keyboard.getMPressedLast() ? "Yes" : "No") << std::endl;
    std::cout << "  - Escape Pressed Last: " << (keyboard.getEscapePressedLast() ? "Yes" : "No") << std::endl;
    std::cout << "  - H Pressed Last: " << (keyboard.getHPressedLast() ? "Yes" : "No") << std::endl;
    std::cout << "  - I Pressed Last: " << (keyboard.getIPressedLast() ? "Yes" : "No") << std::endl;
    std::cout << "  - F Pressed Last: " << (keyboard.getFPressedLast() ? "Yes" : "No") << std::endl;
    
    // Demonstrate action queries
    std::cout << "\n🔍 Action Queries:" << std::endl;
    std::cout << "  - 'toggle_menu' key: " << keyboard.getKeyForAction("toggle_menu") << std::endl;
    std::cout << "  - 'toggle_cursor_lock' key: " << keyboard.getKeyForAction("toggle_cursor_lock") << std::endl;
    std::cout << "  - 'perspective_first_person' key: " << keyboard.getKeyForAction("perspective_first_person") << std::endl;
    
    // Demonstrate handler management
    std::cout << "\n⚙️ Handler Management:" << std::endl;
    
    // Add a custom handler
    keyboard.bindKey(65, "custom_action", []() {
        std::cout << "  🎯 Custom action triggered!" << std::endl;
    });
    
    std::cout << "  - Added custom handler for key 65 (A)" << std::endl;
    std::cout << "  - New handler count: " << keyboard.getHandlerCount() << std::endl;
    
    // Check if handler exists
    std::cout << "  - Has 'custom_action': " << (keyboard.hasHandler("custom_action") ? "Yes" : "No") << std::endl;
    
    // Remove the custom handler
    keyboard.removeHandler("custom_action");
    std::cout << "  - Removed custom handler" << std::endl;
    std::cout << "  - Final handler count: " << keyboard.getHandlerCount() << std::endl;
    
    std::cout << "\n✅ Keyboard Handler Refactor Example Completed!" << std::endl;
    std::cout << "\n📝 Summary of Changes:" << std::endl;
    std::cout << "  - Moved keyboard state tracking from Game.cpp to KeyboardHandler" << std::endl;
    std::cout << "  - Centralized all key bindings in KeyboardHandler" << std::endl;
    std::cout << "  - Added callback system for key actions" << std::endl;
    std::cout << "  - Maintained all existing functionality" << std::endl;
    std::cout << "  - Fixed M key conflict (menu vs camera slow)" << std::endl;
    
    return 0;
} 