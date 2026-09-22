#pragma once

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <glm/glm.hpp>

namespace Physics {

enum class CollisionMethod {
    None,
    PolyhedronSAT,
    GjkEpa,
    SdfProbe,
    PatchNonSolid,
    AabbFallback
};

struct CollisionResult {
    bool hit = false;
    glm::vec3 normal{0.0f, 1.0f, 0.0f}; // direction to move A away from B
    float depth = 0.0f;
    glm::vec3 point{0.0f};
    CollisionMethod method = CollisionMethod::None;
};

CollisionResult dispatchCollision(const Object& a, const Object& b);

} // namespace Physics
