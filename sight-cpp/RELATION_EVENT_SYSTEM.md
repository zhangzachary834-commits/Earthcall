# Relation Event System

This document explains how to use the new event system that triggers whenever a new Relation is created.

## What is it?

When a new Relation is created in the system, an event called `RelationCreatedEvent` is automatically triggered. This allows other parts of your code to react when new relationships are formed.

## How to use it

### 1. Subscribe to the event

```cpp
#include "Core/EventBus.hpp"
#include "Relation/RelationManager.hpp"

// Create a function to handle the event
void handleNewRelation(const RelationCreatedEvent& event) {
    std::cout << "New relation created: " << event.relation.type << std::endl;
    std::cout << "Between: " << event.relation.entityA << " and " << event.relation.entityB << std::endl;
}

// Subscribe to the event
Core::EventBus::instance().subscribe<RelationCreatedEvent>(handleNewRelation);
```

### 2. The event will trigger automatically

Whenever you add a new relation to a RelationManager, the event will be triggered:

```cpp
RelationManager manager;
Relation friendship("friend", "Alice", "Bob");
manager.add(friendship); // This triggers the RelationCreatedEvent
```

### 3. What information is available

The `RelationCreatedEvent` contains:
- `relation`: The complete Relation object that was created
- `timestamp`: When the event was triggered

The Relation object contains:
- `type`: The type of relationship (e.g., "friend", "owns", "at")
- `entityA` and `entityB`: The two entities being related
- `directed`: Whether the relation is directional (A→B) or bidirectional (A↔B)
- `weight`: The strength or importance of the relation

## Example Use Cases

- **Logging**: Track when new relationships are formed
- **UI Updates**: Update the interface when new connections are made
- **Analytics**: Count and analyze relationship patterns
- **Game Logic**: Trigger game events when certain relationships are created
- **Physics**: Update physics simulations when objects are connected

## Important Notes

- The event only triggers for **new** relations, not when existing relations are updated
- The event is triggered synchronously (immediately when the relation is added)
- You can have multiple handlers for the same event
- The event system is thread-safe

## Running the Example

To see the event system in action, compile and run the example:

```bash
g++ -o relation_event_example examples/relation_event_example.cpp src/Relation/RelationManager.cpp src/Relation/Relation.cpp src/Core/EventBus.cpp
./relation_event_example
```

This will show you how the event is triggered each time a new relation is created. 