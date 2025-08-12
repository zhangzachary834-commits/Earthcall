#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Core/EventBus.hpp"
#include "Form/Object/Object.hpp"
#include <iostream>

// Example of how to use the hybrid physics/EventBus system
void setupPhysicsEventSystem() {
    // 1. Set up the default physics event listeners
    Physics::setupPhysicsEventListeners();
    
    // 2. Add custom listeners for specific collision types
    auto& eventBus = Core::EventBus::instance();
    
    // Listen for collisions involving specific object types
    eventBus.subscribe<Physics::PhysicsCollisionEvent>([](const Physics::PhysicsCollisionEvent& event) {
        // Check if this is a player-object collision
        if (event.objectA && event.objectA->getIdentifier().find("Player") != std::string::npos) {
            std::cout << "Player hit something!" << std::endl;
            // Trigger player-specific collision response
        }
        
        // Check for high-impact collisions
        if (event.impactForce > 10.0f) {
            std::cout << "High impact collision detected!" << std::endl;
            // Trigger special effects for high-impact collisions
        }
    });
    
    // Listen for collisions in specific zones
    eventBus.subscribe<Physics::PhysicsCollisionEvent>([](const Physics::PhysicsCollisionEvent& event) {
        // Check if collision happened in a specific area
        if (event.collisionPoint.y > 5.0f) {
            std::cout << "Collision happened in the upper area" << std::endl;
            // Trigger zone-specific responses
        }
    });
}

// Example of how the physics system works with events
void demonstrateHybridPhysics() {
    // The physics system continues to work exactly as before
    // But now it also publishes events that other systems can listen to
    
    // Your existing physics code remains unchanged:
    // - updateBodies() still handles all physics calculations
    // - Collision detection still works the same way
    // - All existing functionality is preserved
    
    // But now when collisions happen, events are automatically published
    // and any registered listeners will be notified
    
    std::cout << "Physics system is now hybrid: hard-wired + event-driven!" << std::endl;
}

int main() {
    // Initialize the event system
    setupPhysicsEventSystem();
    
    // Demonstrate the hybrid approach
    demonstrateHybridPhysics();
    
    return 0;
} 