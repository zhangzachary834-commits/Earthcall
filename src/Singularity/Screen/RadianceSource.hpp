#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <string>

namespace OntoMath { struct Piecewise; }

namespace Rendering {

// A source binding is not a new domain being. It is the renderer-facing projection
// of one authored FieldNode: source placement/coefficient state plus borrowed
// OntoMath source invariants. The FieldNode remains the identity and authoring
// surface; this value is only the sensory channel's per-frame view of it.
struct RadianceSourceBinding {
    // Stable identity of the authored FieldNode already known at admission.
    // This is provenance only: it grants no renderer authority by itself.
    std::string producerId;

    glm::vec3 position{0.0f};
    glm::vec3 ambientRadiance{0.2f};
    glm::vec3 diffuseRadiance{0.8f};
    glm::vec3 specularRadiance{1.0f};
    glm::vec4 coefficients{1.0f, 0.2f, 0.8f, 1.0f};

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
