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
#include <cassert>
#include <vector>
#include <memory>

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
        assert(r.hit == true);
        assert(r.method == CollisionMethod::PolyhedronSAT);
    }

    // 3. GJK EPA
    {
        Object a("A"); a.setShape(ObjectTypes::ShapeKind::Sphere);
        Object b("B"); b.setShape(ObjectTypes::ShapeKind::Sphere);
        glm::mat4 t = glm::mat4(1.0f); t[3] = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f);
        b.setTransform(t);
        CollisionResult r = dispatchCollision(a, b);
        assert(r.hit == true);
        assert(r.method == CollisionMethod::GjkEpa);
    }

    // --- Real Application Physics Runtime Event & Registry Witness Verification ---
    {
        std::cout << "--- Real Application Physics Runtime Event & Registry Witness Verification ---" << std::endl;

        // Reset state & wire physics listeners as production boot does (EngineInit.cpp:264)
        Physics::resetRigidBodies();
        Physics::registry() = RelationManager();
        Physics::setupPhysicsEventListeners();

        bool physicsCollisionEventReceived = false;
        bool ecaCollisionEventReceived = false;
        bool contactBeganEventReceived = false;
        bool contactEndedEventReceived = false;

        Core::EventBus::instance().subscribe<PhysicsCollisionEvent>([&](const PhysicsCollisionEvent&) {
            physicsCollisionEventReceived = true;
        });

        Core::EventBus::instance().subscribe<ECA::Event>([&](const ECA::Event& ev) {
            if (ev.type == "collision") ecaCollisionEventReceived = true;
            if (ev.type == "contact-began") contactBeganEventReceived = true;
            if (ev.type == "contact-ended") contactEndedEventReceived = true;
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

        assert(physicsCollisionEventReceived);
        assert(ecaCollisionEventReceived);
        assert(contactBeganEventReceived);

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
        assert(relationFound);

        // Frame 2: Separate objects and updateBodies to trigger contact-ended
        glm::mat4 tB_far = glm::mat4(1.0f);
        tB_far[3] = glm::vec4(100.0f, 0.0f, 0.0f, 1.0f);
        objB->setTransform(tB_far);

        Physics::updateBodies(objects, 0.016f, 9.81f, 0.1f, -10.0f);
        assert(contactEndedEventReceived);

        std::cout << "  ok: Physics::updateBodies dispatches PhysicsCollisionEvent, contact-began, contact-ended, and records collision relation in registry" << std::endl;
    }

    std::cout << "All CollisionDispatcher tests passed" << std::endl;
    return 0;
}
