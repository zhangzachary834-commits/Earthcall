#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SdfJson.hpp"

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
    // Stable identity of the authored FieldNode already known at admission.
    // This is provenance only: it grants no transport authority by itself.
    std::string producerId;

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

    // Independent V4 self-emission E_v(p,omega,t). Null preserves exact
    // pre-V4 pixels: no self-emitted radiance. This never borrows source rho,
    // source chroma, or surface emission.
    const OntoMath::Piecewise* emissionExpr = nullptr;
    uint64_t emissionRevision = 0;

    // Optional participating-medium occluder geometry S(p) -> signed distance.
    // When present, volumetric transport evaluates path visibility between the
    // medium sample and the radiant source, carving radiance into volumetric beams.
    const geom::SdfNode* occluderSdf = nullptr;
    uint64_t occluderRevision = 0;

    // Relative medium time is data beside THIS medium. EngineRender currently
    // supplies the broad compatibility Timeline to each binding until authored
    // Timeline ownership selection has a production resolver. WebGPU never
    // decides which Timeline owns this coordinate.
    double temporalCoordinate = 0.0;
    double temporalDelta = 0.0;
};

// V5 medium-set groundwork: one canonical six-channel content fingerprint.
// World-level medium discovery and renderer memoization must agree on exactly
// which authored truths make one participating medium's content distinct.
// Keep this helper narrow: placement/time are runtime values, while the six
// authored medium expressions are content identity.
inline uint64_t volumeContentRevision(const VolumeDensityBinding& medium) {
    uint64_t combined = medium.densityRevision;
    auto combine = [&](uint64_t next) {
        combined ^= next + 0x9e3779b97f4a7c15ULL +
                    (combined << 6) + (combined >> 2);
    };
    combine(medium.extinctionRevision);
    combine(medium.scatteringRevision);
    combine(medium.volumeChromaRevision);
    combine(medium.phaseRevision);
    combine(medium.emissionRevision);
    combine(medium.occluderRevision);
    return combined;
}

// Stable ordered membership identity for the Zone-projected medium set. This is
// deliberately textual at the EngineRender boundary so membership/order and all
// six authored channel revisions remain inspectable. The renderer receives this
// already-bounded world truth and never rescans the Zone to reconstruct it.
inline void appendVolumeSetIdentity(std::string& identity,
                                    const std::string& stableId,
                                    const VolumeDensityBinding& medium) {
    identity += stableId;
    identity += ":";
    identity += std::to_string(medium.densityRevision);
    identity += ":";
    identity += std::to_string(medium.extinctionRevision);
    identity += ":";
    identity += std::to_string(medium.scatteringRevision);
    identity += ":";
    identity += std::to_string(medium.volumeChromaRevision);
    identity += ":";
    identity += std::to_string(medium.phaseRevision);
    identity += ":";
    identity += std::to_string(medium.emissionRevision);
    identity += ":";
    identity += std::to_string(medium.occluderRevision);
    identity += "\\n";
}

// Resolve authored density from one FieldNode into the renderer-facing bounded
// projection. Sourcehood is intentionally irrelevant: fog need not illuminate,
// and a radiant source need not be participating medium.
inline bool readVolumeDensity(const geom::FieldNode& field,
                              double temporalCoordinate,
                              double temporalDelta,
                              VolumeDensityBinding& out) {
    if (!field.volumeDensity || field.volumeDensity->pieces.empty()) return false;

    VolumeDensityBinding next;
    next.producerId = field.getIdentifier();
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
    if (field.volumeEmission && !field.volumeEmission->pieces.empty()) {
        next.emissionExpr = field.volumeEmission.get();
        const std::string emissionJson = field.volumeEmission->toJson().dump();
        next.emissionRevision =
            static_cast<uint64_t>(std::hash<std::string>{}(emissionJson));
    }
    if (geom::isSdfActive(field.volumeOccluder.get())) {
        next.occluderSdf = field.volumeOccluder.get();
        const std::string occluderJson = geom::sdfToJson(*field.volumeOccluder).dump();
        next.occluderRevision =
            static_cast<uint64_t>(std::hash<std::string>{}(occluderJson));
    }
    next.temporalCoordinate = temporalCoordinate;
    next.temporalDelta = temporalDelta;
    out = next;
    return true;
}

} // namespace Rendering
