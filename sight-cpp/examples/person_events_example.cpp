#include "../src/Core/EventBus.hpp"
#include "../src/Person/Person.hpp"
#include "../src/Person/AvatarManager.hpp"
#include "../src/Person/Body/Body.hpp"
#include <iostream>
#include <thread>
#include <chrono>

// Event handlers for Person events
void handlePersonCreated(const PersonCreatedEvent& event) {
    std::cout << "\n🎉 === New Person Created! ===" << std::endl;
    std::cout << "Name: " << event.person.getSoulName() << std::endl;
    std::cout << "Level: " << event.person.state.level << std::endl;
    std::cout << "Health: " << event.person.state.health << "/" << event.person.state.maxHealth << std::endl;
    std::cout << "Timestamp: " << event.timestamp << std::endl;
    std::cout << "=============================" << std::endl;
}

void handlePersonJoined(const PersonJoinedEvent& event) {
    std::cout << "\n🚪 === Person Joined Zone! ===" << std::endl;
    std::cout << "Person: " << event.person.getSoulName() << std::endl;
    std::cout << "Zone: " << event.zoneName << std::endl;
    std::cout << "Timestamp: " << event.timestamp << std::endl;
    std::cout << "=============================" << std::endl;
}

void handlePersonLogin(const PersonLoginEvent& event) {
    std::cout << "\n🔐 === Person Logged In! ===" << std::endl;
    std::cout << "Person: " << event.person.getSoulName() << std::endl;
    std::cout << "Session: " << event.sessionId << std::endl;
    std::cout << "Timestamp: " << event.timestamp << std::endl;
    std::cout << "=============================" << std::endl;
}

void handlePersonLogout(const PersonLogoutEvent& event) {
    std::cout << "\n🚪 === Person Logged Out! ===" << std::endl;
    std::cout << "Person: " << event.person.getSoulName() << std::endl;
    std::cout << "Session: " << event.sessionId << std::endl;
    std::cout << "Timestamp: " << event.timestamp << std::endl;
    std::cout << "=============================" << std::endl;
}

int main() {
    std::cout << "=== Person Events Demo ===" << std::endl;
    
    // Subscribe to all Person events
    Core::EventBus::instance().subscribe<PersonCreatedEvent>(handlePersonCreated);
    Core::EventBus::instance().subscribe<PersonJoinedEvent>(handlePersonJoined);
    Core::EventBus::instance().subscribe<PersonLoginEvent>(handlePersonLogin);
    Core::EventBus::instance().subscribe<PersonLogoutEvent>(handlePersonLogout);
    
    // Create an avatar manager
    AvatarManager manager;
    
    std::cout << "\n--- Creating Persons ---" << std::endl;
    
    // Create some persons (this will trigger PersonCreatedEvent)
    Person* alice = manager.createAvatar("Alice", "Voxel");
    Person* bob = manager.createCustomAvatar("Bob", Body::BodyType::Humanoid, Body::Proportions::Adult);
    Person* child = manager.createChildAvatar("Little Timmy");
    
    std::cout << "\n--- Login/Logout Demo ---" << std::endl;
    
    // Login persons (this will trigger PersonLoginEvent)
    alice->login("session_alice_001");
    bob->login("session_bob_001");
    child->login(); // Uses auto-generated session ID
    
    std::cout << "\n--- Zone Management Demo ---" << std::endl;
    
    // Join zones (this will trigger PersonJoinedEvent)
    alice->joinZone("Main Plaza");
    alice->joinZone("Shopping District");
    bob->joinZone("Main Plaza");
    child->joinZone("Playground");
    
    std::cout << "\n--- Checking Status ---" << std::endl;
    
    // Check status
    std::cout << "Alice logged in: " << (alice->isLoggedIn() ? "Yes" : "No") << std::endl;
    std::cout << "Alice session: " << alice->getCurrentSession() << std::endl;
    std::cout << "Alice zones: ";
    for (const auto& zone : alice->getJoinedZones()) {
        std::cout << zone << " ";
    }
    std::cout << std::endl;
    
    std::cout << "\n--- Logout Demo ---" << std::endl;
    
    // Logout (this will trigger PersonLogoutEvent)
    alice->logout();
    bob->logout("session_bob_001");
    
    std::cout << "\n--- Final Status ---" << std::endl;
    std::cout << "Alice logged in: " << (alice->isLoggedIn() ? "Yes" : "No") << std::endl;
    std::cout << "Bob logged in: " << (bob->isLoggedIn() ? "Yes" : "No") << std::endl;
    std::cout << "Child logged in: " << (child->isLoggedIn() ? "Yes" : "No") << std::endl;
    
    std::cout << "\n=== Demo Complete ===" << std::endl;
    
    return 0;
} 