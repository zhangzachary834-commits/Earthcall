#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Core {

struct CameraState {
    glm::vec3 pos   {0.0f, 0.0f, 3.0f};
    glm::vec3 front {0.0f, 0.0f, -1.0f};
    glm::vec3 up    {0.0f, 1.0f, 0.0f};
    float speed = 0.1f;

    GLdouble modelview[16]  {};
    GLdouble projection[16] {};
    int      viewport[4]    {0, 0, 0, 0};
};

} // namespace Core
