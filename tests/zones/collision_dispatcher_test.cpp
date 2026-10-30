// Collision Dispatcher & Physics Runtime Event Witness Test.
//
// WITNESS DOCUMENTATION:
// What the old test proved:
//   That low-level Polyhedron SAT and GJK EPA algorithms in Physics::dispatchCollision(a, b)
//   return hit == true and identify the correct CollisionMethod for intersecting shapes.
// What the old test COULD NOT prove:
//   That two colliding objects in Physics::updateBodies() actually publish PhysicsCollisionEvent,
//   "collision", "contact-began", or "contact-ended" events to EventBus, or that
//   Physics::setupPhysicsEventListeners() (initialized in production boot at EngineInit.cpp:264)
//   receives PhysicsCollisionEvent and records the "collision" relation in Physics::registry().

#include "ZonesOfEarth/Physics/CollisionDispatcher.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <iostream>
#include <vector>
#include <memory>

namespace {
bool require(bool condition, const char* message) {
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}
}

using namespace Physics;

int main() {
    std::cout << "=== Running CollisionDispatcher & Physics Runtime Witness Test ===" << std::endl;

    // 1. Polyhedron SAT
    {
        Object a("A"); a.setShape(ObjectTypes::ShapeKind::Cube);
        Object b("B"); b.setShape(ObjectTypes::ShapeKind::Cube);
        glm::mat4 t = glm::mat4(1.0f); t[3] = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f);
        b.setTransform(t);
        CollisionResult r = dispatchCollision(a, b);
        if (!require(r.hit == true, "r.hit == true")) return 1;
        if (!require(r.method == CollisionMethod::PolyhedronSAT, "r.method == CollisionMethod::PolyhedronSAT")) return 1;
    }

    // 3. GJK EPA
    {
        Object a("A"); a.setShape(ObjectTypes::ShapeKind::Sphere);
        Object b("B"); b.setShape(ObjectTypes::ShapeKind::Sphere);
        glm::mat4 t = glm::mat4(1.0f); t[3] = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f);
        b.setTransform(t);
        CollisionResult r = dispatchCollision(a, b);
        if (!require(r.hit == true, "r.hit == true")) return 1;
        if (!require(r.method == CollisionMethod::GjkEpa, "r.method == CollisionMethod::GjkEpa")) return 1;
    }

    // --- Real Application Physics Runtime Event & Registry Witness Verification ---
    {
        std::cout << "--- Real Application Physics Runtime Event & Registry Witness Verification ---" << std::endl;

        // Reset state & wire physics listeners as production boot does (EngineInit.cpp:264)
        Physics::resetRigidBodies();
        Physics::registry() = RelationManager();
        Physics::setupPhysicsEventListeners();

        int physicsCollisionEventCount = 0;
        int ecaCollisionEventCount = 0;
        int contactBeganEventCount = 0;
        int contactEndedEventCount = 0;

        const auto physicsToken = Core::EventBus::instance().subscribe<PhysicsCollisionEvent>([&](const PhysicsCollisionEvent&) {
            ++physicsCollisionEventCount;
        });

        const auto ecaToken = Core::EventBus::instance().subscribe<ECA::Event>([&](const ECA::Event& ev) {
            if (ev.type == "collision") ++ecaCollisionEventCount;
            if (ev.type == "contact-began") ++contactBeganEventCount;
            if (ev.type == "contact-ended") ++contactEndedEventCount;
        });

        // Instantiate two overlapping objects in a physics update set
        auto objA = std::make_shared<Object>("ObjectA");
        objA->setShape(ObjectTypes::ShapeKind::Cube);
        objA->setTransform(glm::mat4(1.0f));

        auto objB = std::make_shared<Object>("ObjectB");
        objB->setShape(ObjectTypes::ShapeKind::Cube);
        glm::mat4 tB = glm::mat4(1.0f);
        tB[3] = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f);
        objB->setTransform(tB);

        std::vector<std::shared_ptr<Object>> objects{objA, objB};

        // Frame 1: Collide objects via updateBodies
        Physics::updateBodies(objects, 0.016f, 9.81f, 0.1f, -10.0f);

        if (!require(physicsCollisionEventCount == 1, "expected one typed collision event")) return 1;
        if (!require(ecaCollisionEventCount == 1, "expected one ECA collision level event")) return 1;
        if (!require(contactBeganEventCount == 1, "expected one contact-began edge")) return 1;

        // Verify Physics::registry() was populated via setupPhysicsEventListeners() -> recordCollision()
        bool relationFound = false;
        for (const auto& rel : Physics::registry().getAll()) {
            if (rel && rel->type == "collision" &&
                ((rel->a() == objA.get() && rel->b() == objB.get()) ||
                 (rel->a() == objB.get() && rel->b() == objA.get()))) {
                relationFound = true;
                break;
            }
        }
        if (!require(relationFound, "relationFound")) return 1;

        // Frame 2: Separate objects and updateBodies to trigger contact-ended
        glm::mat4 tB_far = glm::mat4(1.0f);
        tB_far[3] = glm::vec4(100.0f, 0.0f, 0.0f, 1.0f);
        objB->setTransform(tB_far);

        Physics::updateBodies(objects, 0.016f, 9.81f, 0.1f, -10.0f);
        if (!require(physicsCollisionEventCount == 1 && ecaCollisionEventCount == 1 &&
                     contactBeganEventCount == 1 && contactEndedEventCount == 1,
                     "expected no new collision and exactly one contact-ended edge")) return 1;
        if (!require(Core::EventBus::instance().unsubscribe(physicsToken),
                     "physics witness subscription cleanup failed")) return 1;
        if (!require(Core::EventBus::instance().unsubscribe(ecaToken),
                     "ECA witness subscription cleanup failed")) return 1;

        std::cout << "  ok: Physics::updateBodies dispatches PhysicsCollisionEvent, contact-began, contact-ended, and records collision relation in registry" << std::endl;
    }

    std::cout << "All CollisionDispatcher tests passed" << std::endl;
    return 0;
}
