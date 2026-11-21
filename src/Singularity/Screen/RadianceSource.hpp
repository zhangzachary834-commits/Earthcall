#pragma once

#include <cstdint>
#include <string>
#include <glm/glm.hpp>

namespace OntoMath { struct Piecewise; }

namespace Rendering {

// Historical surface-light bookkeeping is compatibility data, not source
// emission truth. Keep it explicitly quarantined so an old Material response or
// legacy backend can reproduce old pixels without teaching RadianceSourceBinding
// that a source intrinsically emits "diffuse" or "specular" light.
struct LegacySurfaceIncidentCompatibility {
    glm::vec3 environmentRadiance{0.2f};
    glm::vec3 diffuseIncident{0.8f};
    glm::vec3 specularIncident{1.0f};
    // Historical scalar receiver-model coefficients. These scales are needed
    // when authored chi replaces the old constant source chroma.
    glm::vec3 coefficients{0.2f, 0.8f, 1.0f};
};

// A source binding is not a new domain being. It is the renderer-facing
// projection of one authored FieldNode: source placement, canonical fallback
// emission, and borrowed OntoMath rho/chi/alpha invariants. The FieldNode
// remains the identity and authoring surface.
struct RadianceSourceBinding {
    // Stable authored producer identity carried to the already-known execution slot.
    std::string producerId;
    glm::vec3 position{0.0f};

    // Canonical constant fallback for source emission when chi is absent.
    // rho and alpha remain independent multiplicative authored laws.
    glm::vec3 fallbackChroma{1.0f};
    float fallbackIntensity = 1.0f;

    // Old surface-model envelopes survive only as compatibility projection.
    LegacySurfaceIncidentCompatibility legacySurface{};

    // Relative source time is data beside THIS source, not global shader state.
    // EngineRender currently supplies the broad compatibility Timeline to every
    // source; later ownership selection can diverge them without changing WGSL.
    double temporalCoordinate = 0.0;
    double temporalDelta = 0.0;
    bool enabled = true;

    const OntoMath::Piecewise* radianceExpr = nullptr;
    uint64_t radianceRevision = 0;
    const OntoMath::Piecewise* chromaExpr = nullptr;
    uint64_t chromaRevision = 0;
    const OntoMath::Piecewise* angularExpr = nullptr;
    uint64_t angularRevision = 0;
};

} // namespace Rendering
