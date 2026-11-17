#pragma once

#include <glm/glm.hpp>

namespace geom { class FieldNode; }

namespace Rendering {

// Compatibility reader for one radiant FieldNode. Nothing here creates a Light
// kind: these are ordinary authored properties on the Zone's existing continuous
// FieldNode. color/intensity are the canonical constant source fallback.
// ambient/diffuse/specular are historical save/backend inputs retained only to
// construct LegacySurfaceIncidentCompatibility at the rendering boundary.
struct AuthorableLightState {
    bool source = false;
    bool enabled = true;
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float ambient = 0.2f;
    float diffuse = 0.8f;
    float specular = 1.0f;
    float attenuationConstant = 1.0f;
    float attenuationLinear = 0.0f;
    float attenuationQuadratic = 0.0f;
};

// Returns true only when `light.source` exists and is authored true. Optional
// properties retain the historical renderer defaults when absent.
bool readAuthorableLight(const geom::FieldNode& field, AuthorableLightState& out);

// Canonical source fallback: one chroma times one magnitude. This is the
// source-owned constant incident envelope used when authored chi is absent.
glm::vec3 lightIncidentRadiance(const AuthorableLightState& light);

// Historical ambient/diffuse/specular projections remain only for legacy
// surface-response/backend compatibility. New source transport must not infer
// canonical incident radiance from these receiver-model channels.
glm::vec3 lightAmbientRadiance(const AuthorableLightState& light);
glm::vec3 lightDiffuseRadiance(const AuthorableLightState& light);
glm::vec3 lightSpecularRadiance(const AuthorableLightState& light);

} // namespace Rendering
