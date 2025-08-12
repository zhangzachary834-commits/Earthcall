#include "../src/Core/EventHandler.hpp"
#include "../src/Core/EventBus.hpp"
#include <iostream>
#include <string>

// Example event types
struct PlayerJumped {
    int playerId;
    float jumpHeight;
    std::string timestamp;
};

struct ItemCollected {
    std::string itemName;
    int playerId;
    float value;
};

struct GameStateChanged {
    std::string newState;
    std::string previousState;
};

// Example event handlers
void handlePlayerJumped(const PlayerJumped& event) {
    std::cout << "🎮 Player " << event.playerId << " jumped " 
              << event.jumpHeight << " units high!" << std::endl;
}

void handleItemCollected(const ItemCollected& event) {
    std::cout << "💎 Player " << event.playerId << " collected " 
              << event.itemName << " (value: " << event.value << ")" << std::endl;
}

void handleGameStateChanged(const GameStateChanged& event) {
    std::cout << "🔄 Game state changed from '" << event.previousState 
              << "' to '" << event.newState << "'" << std::endl;
}

// High priority handler (runs first)
void handlePlayerJumpedHighPriority(const PlayerJumped& event) {
    std::cout << "🚀 [HIGH PRIORITY] Player " << event.playerId 
              << " is jumping! Processing first..." << std::endl;
}

int main() {
    std::cout << "=== Event Handler Example ===" << std::endl;
    
    // Get the event handler instance
    auto& eventHandler = Core::EventHandler::instance();
    
    // Register handlers using the new EventHandler
    eventHandler.registerHandler<PlayerJumped>("player_jump_handler", handlePlayerJumped);
    eventHandler.registerHandler<PlayerJumped>("player_jump_high_priority", handlePlayerJumpedHighPriority, 10); // Higher priority
    eventHandler.registerHandler<ItemCollected>("item_collected_handler", handleItemCollected);
    eventHandler.registerHandler<GameStateChanged>("game_state_handler", handleGameStateChanged);
    
    std::cout << "\n📋 Registered handlers: " << eventHandler.getHandlerCount() << std::endl;
    for (const auto& handler : eventHandler.getRegisteredHandlers()) {
        std::cout << "  - " << handler << std::endl;
    }
    
    // Publish events using EventHandler
    std::cout << "\n📤 Publishing events..." << std::endl;
    
    eventHandler.publish(PlayerJumped{1, 5.5f, "2024-01-15 10:30:00"});
    eventHandler.publish(ItemCollected{"Golden Sword", 1, 100.0f});
    eventHandler.publish(GameStateChanged{"Playing", "Menu"});
    
    // You can also still use EventBus directly if needed
    std::cout << "\n📤 Publishing directly through EventBus..." << std::endl;
    Core::EventBus::instance().publish(PlayerJumped{2, 3.2f, "2024-01-15 10:31:00"});
    
    // Test handler management
    std::cout << "\n🔧 Testing handler management..." << std::endl;
    std::cout << "Has 'player_jump_handler': " << (eventHandler.hasHandler("player_jump_handler") ? "Yes" : "No") << std::endl;
    
    // Remove a handler
    eventHandler.removeHandler("item_collected_handler");
    std::cout << "After removing 'item_collected_handler': " << eventHandler.getHandlerCount() << " handlers" << std::endl;
    
    // This event won't be handled since we removed the handler
    eventHandler.publish(ItemCollected{"Diamond", 2, 500.0f});
    
    std::cout << "\n✅ Example completed!" << std::endl;
    
    return 0;
} 