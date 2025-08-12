#include "ShadingSystem.hpp"

#include <GLFW/glfw3.h>
#include <OpenGL/glu.h>

bool ShadingSystem::s_enabled = true;

void ShadingSystem::init() {
    // Basic depth test for correct rendering order
    glEnable(GL_DEPTH_TEST);

    // Enable lighting if requested
    if (!s_enabled) return;

    glEnable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);          // Use Gouraud shading by default

    // Allow vertex colors to act as material diffuse/ambient
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // --- Configure a single white directional light (GL_LIGHT0) ---
    GLfloat ambient[]  = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat diffuse[]  = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    glEnable(GL_LIGHT0);

    // Global material shininess (affects specular highlight size)
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);
}

void ShadingSystem::update(const glm::vec3& cameraPos) {
    if (!s_enabled) return;

    // Keep light a bit above and behind the camera for consistent illumination
    GLfloat position[] = {
        cameraPos.x + 2.0f,
        cameraPos.y + 5.0f,
        cameraPos.z + 2.0f,
        1.0f // positional light
    };
    glLightfv(GL_LIGHT0, GL_POSITION, position);
}

void ShadingSystem::setEnabled(bool enabled) {
    s_enabled = enabled;
    if (enabled) {
        glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING);
    }
}

bool ShadingSystem::isEnabled() {
    return s_enabled;
} 