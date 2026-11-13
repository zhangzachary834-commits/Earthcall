// Physics Collision Event Listener Witness Test.
//
// WITNESS DOCUMENTATION:
// What the old test (collision_dispatcher_test.cpp) proved:
//   That SAT and GJK narrowphase mathematical algorithms (dispatchCollision) correctly
//   detected geometry intersection between two isolated Object instances.
// What the old test COULD NOT prove:
//   That during production runtime execution (Physics::updateBodies), detected collisions
//   actually published PhysicsCollisionEvent to the EventBus, and that
//   Physics::setupPhysicsEventListeners() handled those events to write a "collision" Relation
//   between the two colliding Singular objects into the Physics Relation Registry (Physics::registry()).

#include "support/test_harness.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Relation.hpp"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

static void check(bool cond, const std::string& description) {
    if (!cond) {
        std::printf("  FAILED: %s\n", description.c_str());
        std::exit(1);
    }
    std::printf("  ok: %s\n", description.c_str());
}

int main() {
    std::printf("Running physics_collision_event_listener_test...\n");

    TestSupport::BootedEngineHarness harness("Player", "humanoid", false);

    // Create two cube objects that overlap in space
    auto objA = std::make_shared<Object>("ObjA");
    objA->setObjectID("object-a");
    objA->setShape(ObjectTypes::ShapeKind::Cube);
    objA->setPosition(glm::vec3(0.0f, 1.0f, 0.0f));

    auto objB = std::make_shared<Object>("ObjB");
    objB->setObjectID("object-b");
    objB->setShape(ObjectTypes::ShapeKind::Cube);
    objB->setPosition(glm::vec3(0.5f, 1.0f, 0.0f)); // 0.5 unit offset = clear overlap

    std::vector<std::shared_ptr<Object>> objects{objA, objB};

    // Verify registry is clear before physics step
    check(Physics::registry().getAll().empty(), "physics relation registry is initially empty");

    // Enable physics collision processing and register event listeners as EngineInit.cpp does
    Physics::setLegacyEngineEnabled(true);
    Physics::setupPhysicsEventListeners();

    // Perform physics tick (Physics::updateBodies)
    float deltaTime = 0.016f;
    float gravityAccel = 9.8f;
    float airResistance = 0.01f;
    float groundY = 0.0f;

    Physics::updateBodies(objects, deltaTime, gravityAccel, airResistance, groundY);

    // Verify that PhysicsCollisionEvent was emitted and captured by Physics::setupPhysicsEventListeners()
    const auto& relations = Physics::registry().getAll();
    check(!relations.empty(), "physics collision listener recorded a collision relation upon updateBodies()");

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

    check(foundCollisionRelation, "recorded collision relation links ObjA and ObjB");

    std::printf("physics_collision_event_listener_test PASSED!\n");
    return 0;
}
