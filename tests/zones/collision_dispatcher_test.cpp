#include "ZonesOfEarth/Physics/CollisionDispatcher.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <iostream>
#include <cassert>

using namespace Physics;

int main() {
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
