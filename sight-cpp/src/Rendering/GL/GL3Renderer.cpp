#include "Rendering/GL/GL3Renderer.hpp"

#ifdef USE_GL3_RENDERER

#if defined(__APPLE__)
#   include <OpenGL/gl3.h>
#   include <OpenGL/gl3ext.h>
#else
#   include <GL/glew.h>
#   include <GL/gl.h>
#endif

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

namespace {
static unsigned int compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[GL3Renderer] Shader compile error: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static unsigned int linkProgram(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[GL3Renderer] Program link error: %s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}
} // namespace

bool GL3Renderer::init(GLFWwindow* /*window*/, const char* glslVersion) {
    // Create shaders
    if (!createShaders(glslVersion)) {
        return false;
    }
    // Create test triangle VAO/VBO
    if (!createTriangleMesh()) {
        return false;
    }
    return true;
}

void GL3Renderer::shutdown() {
    destroyGLResources();
}

void GL3Renderer::destroyGLResources() {
    if (_ebo) { glDeleteBuffers(1, &_ebo); _ebo = 0; }
    if (_vbo) { glDeleteBuffers(1, &_vbo); _vbo = 0; }
    if (_vao) { glDeleteVertexArrays(1, &_vao); _vao = 0; }
    if (_programId) { glDeleteProgram(_programId); _programId = 0; }
}

bool GL3Renderer::createShaders(const char* glslVersion) {
    // Minimal GLSL 330 shaders
    const char* vsSrc = R"GLSL(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        uniform mat4 projection;
        uniform mat4 modelView;
        out vec3 vColor;
        void main(){
            vColor = aColor;
            gl_Position = projection * modelView * vec4(aPos, 1.0);
        }
    )GLSL";

    const char* fsSrc = R"GLSL(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main(){
            FragColor = vec4(vColor, 1.0);
        }
    )GLSL";

    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    if (!vs) return false;
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    if (!fs) { glDeleteShader(vs); return false; }

    _programId = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!_programId) return false;

    _uProjection = glGetUniformLocation(_programId, "projection");
    _uModelView = glGetUniformLocation(_programId, "modelView");
    return true;
}

bool GL3Renderer::createTriangleMesh() {
    // Interleaved positions and colors for a single triangle
    const float vertices[] = {
        // pos              // color
        -0.5f, -0.5f, 0.0f,  1.f, 0.f, 0.f,
         0.5f, -0.5f, 0.0f,  0.f, 1.f, 0.f,
         0.0f,  0.5f, 0.0f,  0.f, 0.f, 1.f,
    };
    const unsigned int indices[] = { 0, 1, 2 };

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
    return true;
}

void GL3Renderer::render(int framebufferWidth, int framebufferHeight) {
    glUseProgram(_programId);

    // Build a basic MVP
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
    glm::mat4 modelView  = glm::mat4(1.0f);

    glUniformMatrix4fv(_uProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(_uModelView, 1, GL_FALSE, glm::value_ptr(modelView));

    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

#endif // USE_GL3_RENDERER


