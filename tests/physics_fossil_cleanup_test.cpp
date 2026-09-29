#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include <memory>
#include <vector>
#include <cassert>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

int main() {
    auto& eventBus = Core::EventBus::instance();

    bool receivedPhysicsCollision = false;
    bool receivedEcaCollision = false;
    bool receivedContactBegan = false;

    eventBus.subscribe<Physics::PhysicsCollisionEvent>([&](const Physics::PhysicsCollisionEvent& ev) {
        (void)ev;
        receivedPhysicsCollision = true;
    });

    eventBus.subscribe<ECA::Event>([&](const ECA::Event& ev) {
        if (ev.type == "collision") {
            receivedEcaCollision = true;
        } else if (ev.type == "contact-began") {
            receivedContactBegan = true;
        }
    });

    // Create two overlapping objects
    auto objA = std::make_shared<Object>("ObjA");
    auto objB = std::make_shared<Object>("ObjB");
    objA->setShapeKind(Object::ShapeKind::Cube);
    objB->setShapeKind(Object::ShapeKind::Cube);

    glm::mat4 tA = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 tB = glm::translate(glm::mat4(1.0f), glm::vec3(0.2f, 1.0f, 0.0f));
    objA->setTransform(tA);
    objB->setTransform(tB);

    std::vector<std::shared_ptr<Object>> objects = {objA, objB};

    Physics::updateBodies(objects, 0.016f);

    assert(receivedPhysicsCollision);
    assert(receivedEcaCollision);
    assert(receivedContactBegan);

    std::cout << "physics_fossil_cleanup_test PASSED\n";
    return 0;
}
