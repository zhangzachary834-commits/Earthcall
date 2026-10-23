// Witness Gap Documentation:
// `collision_dispatcher_test.cpp` proved that low-level PolyhedronSAT and GjkEpa algorithms
// detect geometric overlap between two Objects.
// However, it could NOT prove that detecting a collision in the engine actually propagates
// through the EventBus and populates the Physics relational graph (`Physics::registry()`).
// Prior to adding `Physics::setupPhysicsEventListeners()` to `BootedEngineHarness`,
// tests using `BootedEngineHarness` bypassed production physics event listener setup
// (which occurs in `EngineInit.cpp`), leaving the collision relation registry unpopulated on events.
// This test traverses the real runtime EventBus path to verify that publishing a PhysicsCollisionEvent
// records a "collision" relation in `Physics::registry()`.

#include "support/test_harness.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Core/EventBus.hpp"
#include <cassert>
#include <iostream>

int main() {
    // 1. Boot harness (which initializes event listeners matching EngineInit)
    TestSupport::BootedEngineHarness harness("WitnessTester", "humanoid", false);

    // 2. Create two test objects
    Object objA("ObjectA");
    Object objB("ObjectB");

    // Ensure no prior collision relations exist between objA and objB
    assert(Physics::registry().getRelationsBetween(objA, objB).empty());

    // 3. Publish PhysicsCollisionEvent via real EventBus
    Physics::PhysicsCollisionEvent collisionEvent(&objA, &objB, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 1.0f);
    Core::EventBus::instance().publish(collisionEvent);

    // 4. Assert that the collision relation was recorded in Physics::registry()
    auto relations = Physics::registry().getRelationsBetween(objA, objB);
    assert(!relations.empty());
    assert(relations.front()->type == "collision");

    std::cout << "PhysicsCollisionEventListenerWitnessTest passed successfully." << std::endl;
    return 0;
}
