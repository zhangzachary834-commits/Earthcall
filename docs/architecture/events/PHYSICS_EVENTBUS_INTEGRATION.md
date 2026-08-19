# Physics EventBus Integration

This document explains how the physics system now works as a hybrid of hard-wired connections and soft-wired event-driven communication.

## What Changed

The physics system now publishes events when collisions occur, while maintaining all existing functionality. This creates a hybrid approach where:

- **Hard-wired**: Physics calculations, collision detection, and object movement remain exactly the same
- **Soft-wired**: Collision events are published to the EventBus for other systems to listen to

## New Features Added

### 1. PhysicsCollisionEvent Structure
```cpp
struct PhysicsCollisionEvent {
    Object* objectA{nullptr};        // First colliding object
    Object* objectB{nullptr};        // Second colliding object  
    glm::vec3 collisionPoint{0.0f};  // Point where collision occurred
    glm::vec3 collisionNormal{0.0f}; // Normal vector of collision
    float impactForce{0.0f};         // Force of the collision
    std::time_t timestamp{0};        // When collision happened
};
```

### 2. Automatic Event Publishing
When objects collide in the physics system, a `PhysicsCollisionEvent` is automatically published to the EventBus. This happens in the existing collision detection code without removing any functionality.

### 3. Default Event Listeners
The `Physics::setupPhysicsEventListeners()` function sets up default listeners that:
- Log collision details to console
- Record collisions in the physics registry (existing functionality)
- Provide hooks for additional collision responses

## How to Use

### Step 1: Initialize the Event System
```cpp
// Call this during your game initialization
Physics::setupPhysicsEventListeners();
```

### Step 2: Add Custom Listeners
```cpp
auto& eventBus = Core::EventBus::instance();

// Listen for all physics collisions
eventBus.subscribe<Physics::PhysicsCollisionEvent>([](const Physics::PhysicsCollisionEvent& event) {
    // Your custom collision response logic here
    std::cout << "Objects collided: " << event.objectA->getIdentifier() 
              << " and " << event.objectB->getIdentifier() << std::endl;
});
```

### Step 3: Physics System Works Unchanged
```cpp
// Your existing physics code continues to work exactly as before
std::vector<std::unique_ptr<Object>> objects = /* your objects */;
Physics::updateBodies(objects, deltaTime, gravity, airResistance, groundY);
// Collisions are automatically detected AND events are published
```

## Example Use Cases

### 1. Sound Effects
```cpp
eventBus.subscribe<Physics::PhysicsCollisionEvent>([](const Physics::PhysicsCollisionEvent& event) {
    if (event.impactForce > 5.0f) {
        playSound("collision_heavy.wav");
    } else {
        playSound("collision_light.wav");
    }
});
```

### 2. Particle Effects
```cpp
eventBus.subscribe<Physics::PhysicsCollisionEvent>([](const Physics::PhysicsCollisionEvent& event) {
    createParticleEffect(event.collisionPoint, event.impactForce);
});
```

### 3. Game Mechanics
```cpp
eventBus.subscribe<Physics::PhysicsCollisionEvent>([](const Physics::PhysicsCollisionEvent& event) {
    // Check if player hit a collectible
    if (event.objectA->getIdentifier() == "Player" && 
        event.objectB->getIdentifier().find("Collectible") != std::string::npos) {
        collectItem(event.objectB);
    }
});
```

### 4. Formation Relations
```cpp
eventBus.subscribe<Physics::PhysicsCollisionEvent>([](const Physics::PhysicsCollisionEvent& event) {
    // Add collision to formation relations
    auto& eventBus = Core::EventBus::instance();
    eventBus.addEventToFormationRelations("collision", 
                                         event.objectA->getIdentifier(),
                                         event.objectB->getIdentifier(),
                                         currentFormation);
});
```

## Benefits

1. **No Breaking Changes**: All existing physics code continues to work
2. **Modular Design**: Other systems can respond to physics events without tight coupling
3. **Extensible**: Easy to add new collision responses without modifying physics code
4. **Performance**: Events are published only when collisions actually occur
5. **Priority System**: Physics events can have high priority to ensure timely processing

## Integration with Existing Systems

The physics system now seamlessly integrates with:
- **Formation System**: Collisions can be recorded as relations
- **Zone System**: Collisions can trigger zone-specific responses  
- **UI System**: Collision events can update UI elements
- **Audio System**: Collision events can trigger sound effects
- **Particle System**: Collision events can create visual effects

This hybrid approach gives you the best of both worlds: reliable hard-wired physics calculations with flexible event-driven responses! 