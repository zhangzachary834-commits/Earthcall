#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#ifndef GLdouble
typedef double GLdouble;
#endif

namespace Core {

class Camera {
public:
    // Position and orientation
    glm::vec3 pos   {0.0f, 0.0f, 3.0f};
    glm::vec3 front {0.0f, 0.0f, -1.0f};
    glm::vec3 up    {0.0f, 1.0f, 0.0f};
    float speed = 0.1f;

    // Matrices and viewport
    GLdouble modelview[16]  {};
    GLdouble projection[16] {};
    int      viewport[4]    {0, 0, 0, 0};
    
    Camera();
    
    // Accessors
    glm::vec3 getPos() const { return pos; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    
    void setPos(const glm::vec3& p) { pos = p; }
    void setFront(const glm::vec3& f) { front = f; }
    void setUp(const glm::vec3& u) { up = u; }
    
    const GLdouble* getModelview() const { return modelview; }
    const GLdouble* getProjection() const { return projection; }
    const int* getViewport() const { return viewport; }
    
    void setModelview(const GLdouble mv[16]);
    void setProjection(const GLdouble p[16]);
    void setViewport(const int vp[4]);
    
    // Static singleton access
    static Camera& instance();
    static void initialize();
    static void shutdown();

private:
    static Camera* _instance;
};

} // namespace Core
