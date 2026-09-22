#include "Camera.hpp"
#include <algorithm>

namespace Core {

Camera* Camera::_instance = nullptr;

Camera::Camera() {
    pos = glm::vec3(0.0f, 0.0f, 3.0f);
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    up = glm::vec3(0.0f, 1.0f, 0.0f);
    speed = 0.1f;
    
    // Initialize matrices to identity
    for (int i = 0; i < 16; ++i) {
        modelview[i] = (i % 5 == 0) ? 1.0 : 0.0;
        projection[i] = (i % 5 == 0) ? 1.0 : 0.0;
    }
    viewport[0] = viewport[1] = 0;
    viewport[2] = viewport[3] = 0;
}

void Camera::setModelview(const GLdouble mv[16]) {
    std::copy(mv, mv + 16, modelview);
}

void Camera::setProjection(const GLdouble p[16]) {
    std::copy(p, p + 16, projection);
}

void Camera::setViewport(const int vp[4]) {
    std::copy(vp, vp + 4, viewport);
}

Camera& Camera::instance() {
    if (!_instance) {
        initialize();
    }
    return *_instance;
}

void Camera::initialize() {
    if (!_instance) {
        _instance = new Camera();
    }
}

void Camera::shutdown() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

} // namespace Core
