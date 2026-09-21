#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"

#include <cstdint>
#include <glm/glm.hpp>

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

} // namespace Rendering
