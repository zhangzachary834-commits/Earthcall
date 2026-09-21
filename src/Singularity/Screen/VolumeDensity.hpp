#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"

#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <string>

namespace Rendering {

// Renderer-facing projection of one authored participating-medium density field.
// This is NOT a Medium kind and carries no ontological identity of its own. The
// FieldNode remains the authored Singular; this value is the Screen channel's
// bounded view of the truths needed by volumetric transport.
struct VolumeDensityBinding {
    // FieldNode placement. scale is the authored full box span used by the
    // existing FieldNode particle placement convention; origin is its center.
    glm::vec3 origin{0.0f};
    glm::vec3 scale{1.0f};

    // Independent D(p,t). It must never alias source rho merely because both
    // are scalar Piecewise expressions.
    const OntoMath::Piecewise* densityExpr = nullptr;
    uint64_t densityRevision = 0;

    // Relative medium time is data beside THIS medium. EngineRender currently
    // supplies the broad compatibility Timeline to each binding until authored
    // Timeline ownership selection has a production resolver. WebGPU never
    // decides which Timeline owns this coordinate.
    double temporalCoordinate = 0.0;
    double temporalDelta = 0.0;
};

// Resolve authored density from one FieldNode into the renderer-facing bounded
// projection. Sourcehood is intentionally irrelevant: fog need not illuminate,
// and a radiant source need not be participating medium.
inline bool readVolumeDensity(const geom::FieldNode& field,
                              double temporalCoordinate,
                              double temporalDelta,
                              VolumeDensityBinding& out) {
    if (!field.volumeDensity || field.volumeDensity->pieces.empty()) return false;

    VolumeDensityBinding next;
    next.origin = field.origin;
    next.scale = field.scale;
    next.densityExpr = field.volumeDensity.get();
    const std::string json = field.volumeDensity->toJson().dump();
    next.densityRevision =
        static_cast<uint64_t>(std::hash<std::string>{}(json));
    next.temporalCoordinate = temporalCoordinate;
    next.temporalDelta = temporalDelta;
    out = next;
    return true;
}

} // namespace Rendering
