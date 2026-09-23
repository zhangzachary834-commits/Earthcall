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

    // Independent V2 scattering coefficient sigma_s(p,t). Null preserves the
    // exact pre-V2 compatibility law sigma_s = D.
    const OntoMath::Piecewise* scatteringExpr = nullptr;
    uint64_t scatteringRevision = 0;

    // Independent V2 medium chroma C_v(p,t). Null preserves neutral white.
    // This is medium truth and never borrows source chroma or source alpha.
    const OntoMath::Piecewise* volumeChromaExpr = nullptr;
    uint64_t volumeChromaRevision = 0;

    // Independent V3 phase Phi(p,wi,wo,t). Null preserves exact V2 pixels via
    // the compatibility identity Phi=1; presence is sole phase authority.
    const OntoMath::Piecewise* phaseExpr = nullptr;
    uint64_t phaseRevision = 0;

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
    if (field.volumeScattering && !field.volumeScattering->pieces.empty()) {
        next.scatteringExpr = field.volumeScattering.get();
        const std::string scatteringJson = field.volumeScattering->toJson().dump();
        next.scatteringRevision =
            static_cast<uint64_t>(std::hash<std::string>{}(scatteringJson));
    }
    if (field.volumeChroma && !field.volumeChroma->pieces.empty()) {
        next.volumeChromaExpr = field.volumeChroma.get();
        const std::string chromaJson = field.volumeChroma->toJson().dump();
        next.volumeChromaRevision =
            static_cast<uint64_t>(std::hash<std::string>{}(chromaJson));
    }
    if (field.volumePhase && !field.volumePhase->pieces.empty()) {
        next.phaseExpr = field.volumePhase.get();
        const std::string phaseJson = field.volumePhase->toJson().dump();
        next.phaseRevision =
            static_cast<uint64_t>(std::hash<std::string>{}(phaseJson));
    }
    next.temporalCoordinate = temporalCoordinate;
    next.temporalDelta = temporalDelta;
    out = next;
    return true;
}

} // namespace Rendering
