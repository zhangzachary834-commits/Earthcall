#include "../src/Singularity/Core/EventBus.hpp"
#include "../src/Relation/RelationManager.hpp"
#include "../src/Relation/Relation.hpp"
#include "../src/Singularity/Language/Lexeme.hpp"
#include <iostream>
#include <memory>

// Example event handler for RelationCreatedEvent
void handleRelationCreated(const RelationCreatedEvent& event) {
    std::cout << "=== New Relation Created! ===" << std::endl;
    if (!event.relation) {
        std::cout << "Type: (null relation)" << std::endl;
        std::cout << "=============================" << std::endl;
        return;
    }
    std::cout << "Type: " << event.relation->type << std::endl;
    std::cout << "Between: " << event.relation->aId() << " and " << event.relation->bId() << std::endl;
    std::cout << "Directed: " << (event.relation->directed ? "Yes" : "No") << std::endl;
    std::cout << "Weight: " << event.relation->getWeight() << std::endl;
    std::cout << "Timestamp: " << event.timestamp << std::endl;
    std::cout << "=============================" << std::endl;
}

int main() {
    // Subscribe to the RelationCreatedEvent
    Core::EventBus::instance().subscribe<RelationCreatedEvent>(handleRelationCreated);
    
    // Create a relation manager
    RelationManager manager;
    
    using Singularity::Language::Lexeme;
    auto alice = std::make_shared<Lexeme>("Alice", "lexeme.alice");
    auto bob   = std::make_shared<Lexeme>("Bob", "lexeme.bob");
    auto car   = std::make_shared<Lexeme>("Car", "lexeme.car");
    auto home  = std::make_shared<Lexeme>("Home", "lexeme.home");
    auto friendship = std::make_shared<Relation>("friend", *alice, *bob, false, 1.0f);
    auto ownership = std::make_shared<Relation>("owns", *alice, *car, true, 2.0f);
    auto location = std::make_shared<Relation>("at", *bob, *home, true, 1.5f);
    
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