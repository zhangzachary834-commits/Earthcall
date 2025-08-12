#include "../src/Core/EventBus.hpp"
#include "../src/Relation/RelationManager.hpp"
#include "../src/Relation/Relation.hpp"
#include <iostream>

// Example event handler for RelationCreatedEvent
void handleRelationCreated(const RelationCreatedEvent& event) {
    std::cout << "=== New Relation Created! ===" << std::endl;
    std::cout << "Type: " << event.relation.type << std::endl;
    std::cout << "Between: " << event.relation.entityA << " and " << event.relation.entityB << std::endl;
    std::cout << "Directed: " << (event.relation.directed ? "Yes" : "No") << std::endl;
    std::cout << "Weight: " << event.relation.weight << std::endl;
    std::cout << "Timestamp: " << event.timestamp << std::endl;
    std::cout << "=============================" << std::endl;
}

int main() {
    // Subscribe to the RelationCreatedEvent
    Core::EventBus::instance().subscribe<RelationCreatedEvent>(handleRelationCreated);
    
    // Create a relation manager
    RelationManager manager;
    
    // Create some test relations - these will trigger the event
    Relation friendship("friend", "Alice", "Bob", false, 1.0f);
    Relation ownership("owns", "Alice", "Car", true, 2.0f);
    Relation location("at", "Bob", "Home", true, 1.5f);
    
    std::cout << "Adding relations..." << std::endl;
    
    // Add relations to the manager - this will trigger events
    manager.add(friendship);
    manager.add(ownership);
    manager.add(location);
    
    // Try adding the same relation again - this should NOT trigger an event
    std::cout << "\nAdding duplicate relation..." << std::endl;
    manager.add(friendship); // This should not trigger an event since it already exists
    
    std::cout << "\nTotal relations in manager: " << manager.getAll().size() << std::endl;
    
    return 0;
} 