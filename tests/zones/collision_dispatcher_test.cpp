#include "ZonesOfEarth/Physics/CollisionDispatcher.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Core/EventBus.hpp"
#include <iostream>
#include <cassert>

using namespace Physics;

int main() {
    // Verify PhysicsCollisionEvent is published via EventBus during collision dispatch updates
    bool eventReceived = false;
    Core::EventBus::instance().subscribe<PhysicsCollisionEvent>([&eventReceived](const PhysicsCollisionEvent& ev) {
        eventReceived = true;
        assert(ev.objectA != nullptr);
        assert(ev.objectB != nullptr);
    });

    auto objA = std::make_shared<Object>("A"); objA->setShape(ObjectTypes::ShapeKind::Cube);
    auto objB = std::make_shared<Object>("B"); objB->setShape(ObjectTypes::ShapeKind::Cube);
    glm::mat4 t = glm::mat4(1.0f); t[3] = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f);
    objB->setTransform(t);

    std::vector<std::shared_ptr<Object>> objects = { objA, objB };
    Physics::updateBodies(objects, 0.1f, 0.0f, 0.0f, 0.0f);
    assert(eventReceived == true);

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

    std::cout << "All CollisionDispatcher tests passed" << std::endl;
    return 0;
}
