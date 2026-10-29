// Witness 👁️ Test: Physics Collision EventBus Listener Real Runtime Path
//
// WHAT THE OLD TESTS COULD PROVE:
// Existing tests (such as collision_dispatcher_test.cpp and default_physics_laws_test.cpp)
// proved that low-level SAT/GJK algorithms detect shape intersections, and that updateBodies()
// integrates body positions and resolves overlaps.
//
// WHAT THE OLD TESTS COULD NOT PROVE:
// EngineInit.cpp explicitly registers EventBus listeners during application startup via
// Physics::setupPhysicsEventListeners(). However, no test in the test suite ever called
// setupPhysicsEventListeners(). Consequently, the real application path—where
// PhysicsCollisionEvent published during updateBodies() triggers the EventBus listener
// to record a "collision" Relation in Physics::registry()—was completely unexercised.
//
// THIS TEST PROVES:
// Traversing the real application path by calling Physics::setupPhysicsEventListeners()
// before updateBodies() successfully records the "collision" relation in Physics::registry()
// upon collision.

#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/RelationManager.hpp"
#include <iostream>
#include <vector>
#include <memory>

int main() {
    // 1. Traverse real application setup path (as performed in EngineInit.cpp line 264)
    Physics::setupPhysicsEventListeners();
    Physics::resetRigidBodies();

    // 2. Create two overlapping objects
    std::vector<std::shared_ptr<Object>> objects;
    auto a = std::make_shared<Object>("BoxA");
    a->setShape(ObjectTypes::ShapeKind::Cube);
    a->setTransform(glm::mat4(1.0f));

    auto b = std::make_shared<Object>("BoxB");
    b->setShape(ObjectTypes::ShapeKind::Cube);
    glm::mat4 tB = glm::mat4(1.0f);
    tB[3] = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f); // overlapping with a
    b->setTransform(tB);

    objects.push_back(a);
    objects.push_back(b);

    // Ensure collision logic is active
    Physics::setLegacyEngineEnabled(true);

    // Each CTest executable starts with a fresh registry. Fail in Release too:
    // C assert() would silently disappear under NDEBUG.
    if (!Physics::registry().getAll().empty()) {
        std::cerr << "Physics registry was not empty before the witness" << std::endl;
        return 1;
    }

    // 3. Step the real physics update path that publishes PhysicsCollisionEvent.
    Physics::updateBodies(objects, 0.016f, 9.81f, 0.01f, -10.0f);

    // 4. Witness exactly one collision relation, with the actual two endpoints.
    // An unrelated relation (or a duplicate subscription) must not pass.
    const auto& relations = Physics::registry().getAll();
    if (relations.size() != 1 || !relations.front() ||
        relations.front()->type != "collision") {
        std::cerr << "Expected exactly one collision relation, got "
                  << relations.size() << std::endl;
        return 1;
    }
    const auto& collision = relations.front();
    if (!((collision->a() == a.get() && collision->b() == b.get()) ||
          (collision->a() == b.get() && collision->b() == a.get()))) {
        std::cerr << "Collision relation has incorrect endpoints" << std::endl;
        return 1;
    }

    std::cout << "Witness 👁️: physics_event_listener_witness_test passed successfully!" << std::endl;
    return 0;
}
