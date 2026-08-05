#include "AvatarManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

void runAvatarDemo() {
    std::cout << "=== AVATAR SYSTEM DEMO ===" << std::endl;
    std::cout << "Creating a robust and meaningful avatar system..." << std::endl;
    
    AvatarManager manager;
    
    // Create different types of avatars
    std::cout << "\n--- Creating Avatars ---" << std::endl;
    Person* alice = manager.createAvatar("Alice", "Voxel");
    Person* bob = manager.createCustomAvatar("Bob", Body::BodyType::Humanoid, Body::Proportions::Adult);
    Person* child = manager.createChildAvatar("Little Timmy");
    Person* elder = manager.createElderAvatar("Grandpa Joe");
    
    // Customize avatars
    std::cout << "\n--- Customizing Avatars ---" << std::endl;
    alice->setHairStyle("Long");
    alice->setEyeColor("Blue");
    alice->setHeight(1.7f);
    alice->setWeight(60.0f);
    
    bob->setHairStyle("Short");
    bob->setEyeColor("Brown");
    bob->setHeight(1.85f);
    bob->setWeight(80.0f);
    
    // Add clothing to avatars
    std::cout << "\n--- Adding Clothing ---" << std::endl;
    Body::Clothing shirt{"Cotton Shirt", "torso", "cotton", 5.0f, 10.0f};
    Body::Clothing pants{"Denim Pants", "legs", "denim", 8.0f, 15.0f};
    Body::Clothing hat{"Wool Hat", "head", "wool", 2.0f, 20.0f};
    
    alice->getBody().addClothing(shirt);
    alice->getBody().addClothing(pants);
    alice->getBody().equipClothing("Cotton Shirt");
    alice->getBody().equipClothing("Denim Pants");
    
    bob->getBody().addClothing(hat);
    bob->getBody().equipClothing("Wool Hat");
    
    // Add items to inventory
    std::cout << "\n--- Adding Items to Inventory ---" << std::endl;
    alice->addToInventory("Magic Wand");
    alice->addToInventory("Health Potion");
    alice->addToInventory("Golden Key");
    
    bob->addToInventory("Sword");
    bob->addToInventory("Shield");
    
    // Create avatar groups
    std::cout << "\n--- Creating Groups ---" << std::endl;
    manager.createAvatarGroup("Adventurers", {"Alice", "Bob"});
    manager.createAvatarGroup("Family", {"Little Timmy", "Grandpa Joe"});
    
    // Position avatars near each other for interactions
    alice->position = glm::vec3(0.0f, 0.0f, 0.0f);
    bob->position = glm::vec3(2.0f, 0.0f, 0.0f);
    child->position = glm::vec3(-2.0f, 0.0f, 0.0f);
    elder->position = glm::vec3(0.0f, 0.0f, 2.0f);
    
    // Enable AI for some avatars
    std::cout << "\n--- Enabling AI ---" << std::endl;
    manager.enableAvatarAI("Bob");
    manager.enableAvatarAI("Little Timmy");
    
    // Simulate some time passing
    std::cout << "\n--- Simulating Time (5 seconds) ---" << std::endl;
    for (int i = 0; i < 50; ++i) {  // 50 frames at 0.1s each = 5 seconds
        manager.updateAllAvatars(0.1f);
        
        // Show status every second
        if (i % 10 == 0) {
            std::cout << "\nTime: " << (i / 10) << "s" << std::endl;
            std::cout << "Alice - Health: " << alice->state.health 
                      << ", Energy: " << alice->state.energy 
                      << ", Mood: " << alice->state.mood << std::endl;
            std::cout << "Bob - Health: " << bob->state.health 
                      << ", Energy: " << bob->state.energy 
                      << ", Mood: " << bob->state.mood << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Organize an event
    std::cout << "\n--- Organizing Event ---" << std::endl;
    manager.organizeEvent("Birthday Party", {"Alice", "Bob", "Little Timmy", "Grandpa Joe"});
    
    // Start some activities
    std::cout << "\n--- Starting Activities ---" << std::endl;
    manager.startActivity("Dancing", {"Alice", "Bob"});
    manager.startActivity("Storytelling", {"Grandpa Joe", "Little Timmy"});
    
    // Simulate more time
    std::cout << "\n--- More Simulation (3 seconds) ---" << std::endl;
    for (int i = 0; i < 30; ++i) {
        manager.updateAllAvatars(0.1f);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Show final statistics
    std::cout << "\n--- Final Statistics ---" << std::endl;
    std::cout << "Total Avatars: " << manager.getTotalAvatars() << std::endl;
    std::cout << "Average Health: " << manager.getAverageHealth() << std::endl;
    std::cout << "Average Level: " << manager.getAverageLevel() << std::endl;
    std::cout << "Total Experience: " << manager.getTotalExperience() << std::endl;
    
    // Show individual avatar details
    std::cout << "\n--- Individual Avatar Details ---" << std::endl;
    alice->express();
    std::cout << "Inventory: ";
    for (const auto& item : alice->inventory) {
        std::cout << item << " ";
    }
    std::cout << std::endl;
    
    bob->express();
    std::cout << "Inventory: ";
    for (const auto& item : bob->inventory) {
        std::cout << item << " ";
    }
    std::cout << std::endl;
    
    // Test body part damage and healing
    std::cout << "\n--- Testing Body Part System ---" << std::endl;
    BodyPart* aliceHead = alice->getBody().getBodyPart("Head");
    if (aliceHead) {
        std::cout << "Alice's head health: " << aliceHead->getHealth() << std::endl;
        aliceHead->takeDamage(20.0f);
        std::cout << "After damage: " << aliceHead->getHealth() << " (State: " 
                  << static_cast<int>(aliceHead->getHealthState()) << ")" << std::endl;
        aliceHead->heal(10.0f);
        std::cout << "After healing: " << aliceHead->getHealth() << std::endl;
    }
    
    // Test clothing system
    std::cout << "\n--- Clothing System ---" << std::endl;
    const Body::Clothing* aliceShirt = alice->getBody().getEquippedClothing("torso");
    if (aliceShirt) {
        std::cout << "Alice is wearing: " << aliceShirt->name 
                  << " (Protection: " << aliceShirt->protection 
                  << ", Warmth: " << aliceShirt->warmth << ")" << std::endl;
    }
    
    std::cout << "Alice's total protection: " << alice->getBody().getTotalProtection() << std::endl;
    std::cout << "Alice's total warmth: " << alice->getBody().getTotalWarmth() << std::endl;
    
    // Test animations
    std::cout << "\n--- Animation System ---" << std::endl;
    alice->playAnimation("Idle", true);
    bob->playAnimation("Walk", true);
    
    // Simulate animation for a bit
    for (int i = 0; i < 20; ++i) {
        manager.updateAllAvatars(0.1f);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Create and apply a preset
    std::cout << "\n--- Preset System ---" << std::endl;
    manager.createPreset("Warrior", bob);
    manager.applyPreset("Warrior", alice);
    
    // Save avatar states
    std::cout << "\n--- Saving Avatar States ---" << std::endl;
    manager.saveAvatarState("Alice", "alice_save.txt");
    manager.saveAvatarState("Bob", "bob_save.txt");
    
    std::cout << "\n=== DEMO COMPLETE ===" << std::endl;
    std::cout << "The avatar system now includes:" << std::endl;
    std::cout << "✓ Health, energy, mood, and experience systems" << std::endl;
    std::cout << "✓ Body part damage and healing" << std::endl;
    std::cout << "✓ Clothing system with protection and warmth" << std::endl;
    std::cout << "✓ Inventory management" << std::endl;
    std::cout << "✓ Avatar interactions and relationships" << std::endl;
    std::cout << "✓ Animation system" << std::endl;
    std::cout << "✓ Avatar groups and events" << std::endl;
    std::cout << "✓ AI behavior system" << std::endl;
    std::cout << "✓ Customization presets" << std::endl;
    std::cout << "✓ State persistence" << std::endl;
    std::cout << "✓ Body proportions and measurements" << std::endl;
    std::cout << "✓ Special effects and status conditions" << std::endl;
} 