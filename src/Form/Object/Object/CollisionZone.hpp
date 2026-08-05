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

    // Quick AABB-vs-AABB test placeholder.
    bool isTouching(const CollisionZone& space) const {
        (void)space;
        return false;
    }

    // Object-aware overload — implemented out of line in Object.cpp because
    // Object is forward-declared here.
    bool isTouching(const Object& object) const;
};
