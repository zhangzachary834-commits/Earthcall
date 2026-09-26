#pragma once

// Rung 10A: zero-pixel-authority reference adapter for secondary scene hits.
// This deliberately wraps the existing CPU picking truth. It is NOT a
// production GI hot path and grants no renderer pixel authority.
#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.hpp"

namespace Rung10TestSupport {

enum class SecondaryHitState { BudgetExhausted, Miss, Hit };

struct SecondaryHit {
    SecondaryHitState state = SecondaryHitState::Miss;
    std::string objectId;
    std::string materialId;
    float distance = 0.0f;
    int face = -1;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    uint32_t bounceIndex = 0;
    uint32_t remainingBudget = 0;
};

inline SecondaryHit querySecondaryHit(const std::vector<Object*>& scene,
                                      const glm::vec3& origin,
                                      const glm::vec3& direction,
                                      uint32_t bounceIndex,
                                      uint32_t remainingBudget,
                                      uint64_t* queryCount = nullptr) {
    SecondaryHit out;
    out.bounceIndex = bounceIndex;
    out.remainingBudget = remainingBudget;

    // Constitutional compatibility gate: direct rendering (budget 0) does not
    // even consult secondary scene geometry.
    if (remainingBudget == 0) {
        out.state = SecondaryHitState::BudgetExhausted;
        return out;
    }

    if (queryCount) ++*queryCount;
    SurfaceHit surface;
    if (!pickSurface(scene, origin, direction, surface) || !surface.obj) {
        out.state = SecondaryHitState::Miss;
        return out; // fresh empty identities: no stale prior hit can survive
    }

    out.state = SecondaryHitState::Hit;
    out.objectId = surface.obj->getIdentifier();
    out.materialId = surface.obj->materialId();
    out.distance = surface.t;
    out.face = surface.face;
    out.point = surface.point;
    out.normal = surface.normal;
    return out;
}

} // namespace Rung10TestSupport
