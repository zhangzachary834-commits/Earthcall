#pragma once

#include <glm/glm.hpp>

namespace geom { class FieldNode; }

namespace Rendering {

// The authored meaning of one radiant FieldNode, before any rendering backend
// decides how to realize it. Nothing here creates a Light kind: these are
// ordinary authored properties on the Zone's existing continuous FieldNode.
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

// Convert authored chromatic/intensity coefficients into the renderer boundary's
// three existing radiance channels. Keeping this transform in one place prevents
// OpenGL and WebGPU call sites from inventing separate interpretations.
glm::vec3 lightAmbientRadiance(const AuthorableLightState& light);
glm::vec3 lightDiffuseRadiance(const AuthorableLightState& light);
glm::vec3 lightSpecularRadiance(const AuthorableLightState& light);

} // namespace Rendering
