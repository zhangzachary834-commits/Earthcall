// Witness analysis for physics collision relation recording on real runtime path:
//
// What existing unit tests (e.g. collision_dispatcher_test.cpp) could prove:
//   That low-level geometry intersection algorithms (SAT/GJK) return correct
//   CollisionResult structs when invoked directly on pairs of Object shapes.
//
// What existing unit tests could NOT prove:
//   That running the real production physics update loop (Physics::updateBodies)
//   inside a booted engine harness (BootedEngineHarness) actually routes the
//   published PhysicsCollisionEvent through Core::EventBus listeners to record
//   collision relations in Physics::registry().
//   Prior to wiring Physics::setupPhysicsEventListeners() in BootedEngineHarness,
//   the test harness omitted the EventBus listener setup present in EngineInit.cpp,
//   causing PhysicsCollisionEvent instances published during physics updates to be
//   silently dropped without updating Physics::registry().

#include "support/test_harness.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

int main() {
    std::cout << "=== Running Physics Collision Relation Witness Test ===" << std::endl;

    // Boot engine harness without loading existing zone store
    TestSupport::BootedEngineHarness harness("Tester", "humanoid", false);

    // Prepare two overlapping objects in space
    auto objA = std::make_shared<Object>("witness-object-a");
    objA->setShape(ObjectTypes::ShapeKind::Cube);
    objA->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));

    auto objB = std::make_shared<Object>("witness-object-b");
    objB->setShape(ObjectTypes::ShapeKind::Cube);
    objB->setPosition(glm::vec3(0.2f, 0.0f, 0.0f)); // Overlaps with ObjA

    std::vector<std::shared_ptr<Object>> objects = {objA, objB};

    // Ensure physics registry is clean before simulation
    while (!Physics::registry().getAll().empty()) {
        Physics::registry().remove(Physics::registry().getAll().back());
    }
    assert(Physics::registry().getAll().empty() && "Physics registry should start empty");

    // Execute one frame of physics simulation with collision detection enabled
    Physics::updateBodies(objects, 0.016f, 9.8f, 0.01f, -10.0f);

    // Verify that PhysicsCollisionEvent was routed via EventBus to record the collision relation
    const auto& relations = Physics::registry().getAll();
    assert(!relations.empty() && "Physics collision must be recorded in Physics::registry() during updateBodies");

    bool foundCollisionRelation = false;
    for (const auto& rel : relations) {
        if (rel && rel->type == "collision") {
            if ((rel->a() == objA.get() && rel->b() == objB.get()) ||
                (rel->a() == objB.get() && rel->b() == objA.get())) {
                foundCollisionRelation = true;
                break;
            }
        }
    }

    assert(foundCollisionRelation && "A collision relation between ObjA and ObjB must be recorded in Physics::registry()");
    std::cout << "  ok: PhysicsCollisionEvent correctly populates Physics::registry() during updateBodies" << std::endl;

    std::cout << "=== Physics Collision Relation Witness Test Passed ===" << std::endl;
    return 0;
}
