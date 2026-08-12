#include "ShadingSystem.hpp"

#include "Singularity/Screen/Renderer.hpp"

bool ShadingSystem::s_enabled = true;

// The scene's lighting policy. It used to configure GL_LIGHT0 directly; now it
// only decides WHAT the light is and hands that to the renderer boundary, which
// decides HOW to install it (GL_LIGHT0 under OpenGL, WGSL uniforms under WebGPU).
// The values below are unchanged from the fixed-function setup.
namespace {
const glm::vec3 kAmbient(0.2f, 0.2f, 0.2f);
const glm::vec3 kDiffuse(0.8f, 0.8f, 0.8f);
const glm::vec3 kSpecular(1.0f, 1.0f, 1.0f);
} // namespace

void ShadingSystem::init() {
    currentRenderer().setLightingEnabled(s_enabled);
    if (!s_enabled) return;
    currentRenderer().setLight(glm::vec3(2.0f, 5.0f, 2.0f), kAmbient, kDiffuse, kSpecular);
}

void ShadingSystem::update(const glm::vec3& cameraPos) {
    if (!s_enabled) return;

    // Keep light a bit above and behind the camera for consistent illumination
    currentRenderer().setLight(cameraPos + glm::vec3(2.0f, 5.0f, 2.0f),
                               kAmbient, kDiffuse, kSpecular);
}

void ShadingSystem::setEnabled(bool enabled) {
    s_enabled = enabled;
    currentRenderer().setLightingEnabled(enabled);
}

bool ShadingSystem::isEnabled() {
    return s_enabled;
}
