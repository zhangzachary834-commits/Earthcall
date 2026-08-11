#pragma once

#include <glm/glm.hpp>
#include <vector>

class Object;

// Axis-aligned bounding box represented by 8 world-space corners, plus a
// (currently unused) vertex list reserved for finer polyhedron collision.
// TODO: extend to polyhedron-precise collision when needed.
struct CollisionZone {
    // AABB box corners in world space.
    glm::vec3 corners[8];

    // Reserved for complex/round-faced polyhedrons that need both AABB
    // and radial/parametric checks.
    std::vector<glm::vec3> vertices;

    // Quick AABB-vs-AABB test for broad-phase rejection.
    bool isTouching(const CollisionZone& space) const {
        glm::vec3 minA = corners[0], maxA = corners[0];
        glm::vec3 minB = space.corners[0], maxB = space.corners[0];
        for (int i = 1; i < 8; ++i) {
            minA = glm::min(minA, corners[i]);
            maxA = glm::max(maxA, corners[i]);
            minB = glm::min(minB, space.corners[i]);
            maxB = glm::max(maxB, space.corners[i]);
        }
        return (minA.x <= maxB.x && maxA.x >= minB.x) &&
               (minA.y <= maxB.y && maxA.y >= minB.y) &&
               (minA.z <= maxB.z && maxA.z >= minB.z);
    }

    // Object-aware overload — implemented out of line in Object.cpp because
    // Object is forward-declared here.
    bool isTouching(const Object& object) const;
};
