// Legacy primitive render test (OPENGL_MIGRATION_PLAN.md, Milestone 2).
//
// The GeometryType::Cube/Sphere/Cylinder/Cone paths were immediate-mode GL; they
// are now built as TessMeshes and drawn through the Renderer like everything else.
// No other test touches these paths (geometry_cache_test uses the *topology*
// smooth-surface path via setShape, not the legacy primitives). This verifies each
// primitive still reaches the framebuffer, and that the primitive path — like the
// mesh paths — honours materials (a dim material dims the cube).

#include "Form/Object/Object.hpp"
#include "Form/Material/MaterialManager.hpp"
#include "Rendering/Renderer.hpp"

#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <cassert>
#include <cstdio>
#include <vector>

extern MaterialManager materials;

namespace {

const int bg[3] = {64, 128, 191}; // clear colour 0.25/0.5/0.75

void setupFrame() {
    glViewport(0, 0, 64, 64);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -2.0, 2.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

std::vector<unsigned char> readback() {
    glFinish();
    std::vector<unsigned char> px(64 * 64 * 4);
    glReadPixels(0, 0, 64, 64, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    return px;
}

size_t coverage(Object& obj) {
    setupFrame();
    obj.drawObject();
    auto px = readback();
    size_t covered = 0;
    for (size_t i = 0; i < px.size(); i += 4) {
        bool background = std::abs(int(px[i])     - bg[0]) < 20
                       && std::abs(int(px[i + 1]) - bg[1]) < 20
                       && std::abs(int(px[i + 2]) - bg[2]) < 20;
        if (!background) ++covered;
    }
    return covered;
}

long long brightness(Object& obj) {
    setupFrame();
    obj.drawObject();
    auto px = readback();
    long long sum = 0;
    for (size_t i = 0; i < px.size(); i += 4) sum += px[i] + px[i + 1] + px[i + 2];
    return sum;
}

} // namespace

int main() {
    if (!glfwInit()) { std::fprintf(stderr, "primitive_render_test: glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "primitive_render_test", nullptr, nullptr);
    if (!window) { std::fprintf(stderr, "primitive_render_test: no GL context\n"); glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    // Each legacy primitive reaches the framebuffer through the renderer boundary.
    {
        struct { Object::GeometryType t; const char* name; } cases[] = {
            {Object::GeometryType::Cube,     "cube"},
            {Object::GeometryType::Sphere,   "sphere"},
            {Object::GeometryType::Cylinder, "cylinder"},
            {Object::GeometryType::Cone,     "cone"},
        };
        for (auto& c : cases) {
            Object obj;
            obj.setGeometryType(c.t);
            size_t px = coverage(obj);
            assert(px > 0);
            std::printf("  %-8s drew %zu px through the renderer\n", c.name, px);
        }
    }

    // Polyhedron: variable-geometry path with a per-face mesh cache. Also confirm
    // the cache follows an edit (tetra -> icosa changes coverage, i.e. rebuilt).
    {
        Object obj;
        obj.createTetrahedron();
        size_t tetra = coverage(obj);
        assert(tetra > 0);

        obj.createIcosahedron();
        size_t icosa = coverage(obj);
        assert(icosa > 0);
        assert(icosa != tetra); // the cached face meshes rebuilt on the geometry change
        std::printf("  polyhedron drew %zu px (tetra) -> %zu px (icosa); cache follows edits\n",
                    tetra, icosa);
    }

    // The primitive path honours materials: a dim material dims the cube.
    {
        Object obj; // default geometry is Cube
        obj.setGeometryType(Object::GeometryType::Cube);
        long long bright = brightness(obj);

        auto dim = materials.create("prim_dim");
        dim->baseColor = glm::vec3(0.25f, 0.25f, 0.25f);
        obj.setMaterialId("material.prim_dim");
        long long dimmed = brightness(obj);

        assert(dimmed < bright);
        std::printf("  material: cube brightness %lld -> dim %lld (material reaches the primitive path)\n",
                    bright, dimmed);
    }

    // Overlay verbs (the selection highlight) reach the framebuffer through the
    // renderer, just like surfaces do.
    {
        Renderer& r = currentRenderer();

        // drawLines: a yellow wireframe quad.
        setupFrame();
        std::vector<std::pair<glm::vec3, glm::vec3>> edges = {
            {{-0.5f,-0.5f,0.0f}, { 0.5f,-0.5f,0.0f}}, {{ 0.5f,-0.5f,0.0f}, { 0.5f, 0.5f,0.0f}},
            {{ 0.5f, 0.5f,0.0f}, {-0.5f, 0.5f,0.0f}}, {{-0.5f, 0.5f,0.0f}, {-0.5f,-0.5f,0.0f}},
        };
        r.drawLines(edges, glm::vec4(1.0f, 0.9f, 0.2f, 0.8f), 3.0f, Blend::Alpha);
        auto px = readback();
        size_t lineCov = 0;
        for (size_t i = 0; i < px.size(); i += 4) {
            bool bgpix = std::abs(int(px[i]) - bg[0]) < 20 && std::abs(int(px[i+1]) - bg[1]) < 20
                      && std::abs(int(px[i+2]) - bg[2]) < 20;
            if (!bgpix) ++lineCov;
        }
        assert(lineCov > 0);

        // drawOverlay: a translucent additive shell over a 2-triangle quad mesh.
        setupFrame();
        geom::TessMesh quad;
        geom::TessVertex a, b, c, d;
        a.pos = {-0.5f,-0.5f,0.0f}; b.pos = {0.5f,-0.5f,0.0f};
        c.pos = { 0.5f, 0.5f,0.0f}; d.pos = {-0.5f,0.5f,0.0f};
        a.normal = b.normal = c.normal = d.normal = glm::vec3(0,0,1);
        quad.tris = {a, b, c, a, c, d};
        r.drawOverlay(quad, glm::vec4(1.0f, 0.2f, 0.2f, 0.5f), 1.0f, true);
        px = readback();
        size_t overlayCov = 0;
        for (size_t i = 0; i < px.size(); i += 4)
            if (int(px[i]) > bg[0] + 20) ++overlayCov; // additive red pushes red up
        assert(overlayCov > 0);
        std::printf("  overlay: drawLines %zu px, drawOverlay %zu px through the renderer\n",
                    lineCov, overlayCov);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("primitive_render_test: ALL OK\n");
    return 0;
}
