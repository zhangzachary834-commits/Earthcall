#pragma once

#ifdef USE_GL3_RENDERER

#include <string>
#include <vector>
#include <glm/glm.hpp>

// Forward declare GLFWwindow to avoid heavy includes here
struct GLFWwindow;

// Minimal OpenGL 3.3+ renderer used for initial migration testing.
// Draws a simple triangle using VAO/VBO and a tiny shader program.
// This allows validating the modern pipeline without removing legacy GL2 code.
class GL3Renderer {
public:
    GL3Renderer() = default;
    ~GL3Renderer() = default;

    bool init(GLFWwindow* window, const char* glslVersion = "#version 330 core");
    void render(int framebufferWidth, int framebufferHeight);
    void shutdown();

private:
    unsigned int _programId = 0; // GL shader program
    unsigned int _vao = 0;       // Vertex Array Object
    unsigned int _vbo = 0;       // Vertex Buffer Object
    unsigned int _ebo = 0;       // Element Buffer Object

    int _uProjection = -1;       // uniform locations
    int _uModelView = -1;

    bool createShaders(const char* glslVersion);
    bool createTriangleMesh();
    void destroyGLResources();
};

#endif // USE_GL3_RENDERER


