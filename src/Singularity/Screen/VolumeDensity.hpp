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
    // FieldNode placement. The established particle/FieldNode convention uses
    // local coordinates in [-1,+1], so scale is the origin-centred box
    // half-span: authored world bounds are origin ± abs(scale). V0 must not
    // silently redefine the same FieldNode property as a full span.
    glm::vec3 origin{0.0f};
    glm::vec3 scale{1.0f};

    // Independent D(p,t). It must never alias source rho merely because both
    // are scalar Piecewise expressions.
    const OntoMath::Piecewise* densityExpr = nullptr;
    uint64_t densityRevision = 0;

    // Independent V1 extinction sigma_t(p,t). Null means the explicit
    // compatibility law sigma_t = 0.5 * D; it never means "borrow rho".
    const OntoMath::Piecewise* extinctionExpr = nullptr;
    uint64_t extinctionRevision = 0;

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
    if (field.volumeExtinction && !field.volumeExtinction->pieces.empty()) {
        next.extinctionExpr = field.volumeExtinction.get();
        const std::string extinctionJson = field.volumeExtinction->toJson().dump();
        next.extinctionRevision =
            static_cast<uint64_t>(std::hash<std::string>{}(extinctionJson));
    }
    next.temporalCoordinate = temporalCoordinate;
    next.temporalDelta = temporalDelta;
    out = next;
    return true;
}

} // namespace Rendering
