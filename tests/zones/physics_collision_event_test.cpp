#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <cassert>
#include <memory>
#include <vector>

void testPhysicsCollisionEvent() {
    bool collisionEventFired = false;

    // Subscribe to PhysicsCollisionEvent on EventBus
    Core::EventBus::instance().subscribe<Physics::PhysicsCollisionEvent>(
        [&collisionEventFired](const Physics::PhysicsCollisionEvent& event) {
            collisionEventFired = true;
            assert(event.objectA != nullptr);
            assert(event.objectB != nullptr);
        });

    // Create two overlapping objects using standard Object constructor
    auto objA = std::make_shared<Object>();
    auto objB = std::make_shared<Object>();

    std::vector<std::shared_ptr<Object>> objects = {objA, objB};

    // Run physics update to trigger collision detection and event publishing
    Physics::updateBodies(objects, 0.016f);

    // Verify PhysicsCollisionEvent was published without setupPhysicsEventListeners()
    assert(collisionEventFired);
}

int main() {
    testPhysicsCollisionEvent();
    return 0;
}
