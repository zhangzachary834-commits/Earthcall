// Material render integration test (OPENGL_MIGRATION_PLAN.md, Milestone 2).
//
// The unit tests prove each link: a Material is a law-addressable being
// (material_being_test), an Object references one by identifier
// (property_bridge_test), and the renderer boundary is behaviour-preserving
// (geometry_cache_test). This proves the LINKS FORM A CHAIN that reaches pixels:
// assigning a darker-tinted material to an object actually dims what draws.
// Without this, baseColor could silently stop flowing into drawMesh and every
// other test would still pass.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"

#include <GLFW/glfw3.h>
#ifndef NO_OPENGL_RENDERER
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include <cassert>
#include <cstdio>
#include <vector>

extern MaterialManager materials; // the global the renderer resolves against

#ifndef NO_OPENGL_RENDERER
namespace {

// Total red channel over the framebuffer. Geometry and background are identical
// between renders, so any difference is purely the material tint multiplying the
// (red) face texture. Lighting is left off, so glColor modulates the texture
// directly and the result is deterministic.
long long renderAndSumRed(Object& obj) {
    glViewport(0, 0, 64, 64);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -2.0, 2.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    obj.drawObject();
    glFinish();

    std::vector<unsigned char> px(64 * 64 * 4);
    glReadPixels(0, 0, 64, 64, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    long long sum = 0;
    for (size_t i = 0; i < px.size(); i += 4) sum += px[i]; // red byte
    return sum;
}

} // namespace
#endif // !NO_OPENGL_RENDERER

int main() {
#ifdef NO_OPENGL_RENDERER
    std::printf("material_render_test: SKIPPED (NO_OPENGL_RENDERER)\n");
    return 0;
#else
    if (!glfwInit()) { std::fprintf(stderr, "material_render_test: glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "material_render_test", nullptr, nullptr);
    if (!window) { std::fprintf(stderr, "material_render_test: no GL context\n"); glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    Object obj;
    Object::ShapeParams p; p.r = 0.8f;
    obj.setShape(Object::ShapeKind::Sphere, p); // smooth-surface path, face 0 red by default

    // 1. Default material: white tint, so the red texture stays bright red.
    assert(obj.materialId() == "material.default");
    long long bright = renderAndSumRed(obj);
    assert(bright > 0);

    // 2. Assign a dim grey material. baseColor (0.3) multiplies the red texture,
    //    so the sphere's red drops — same geometry, same background.
    auto dim = materials.create("dim");
    dim->baseColor = glm::vec3(0.3f, 0.3f, 0.3f);
    obj.setMaterialId("material.dim");
    long long dimmed = renderAndSumRed(obj);

    // The tint reached the framebuffer through the whole chain.
    assert(dimmed < bright);
    std::printf("  tint:    default red sum %lld -> dim material %lld (material reaches pixels)\n",
                bright, dimmed);

    // 3. Back to default resolves and restores brightness (reference is not dangling).
    obj.setMaterialId("material.default");
    long long restored = renderAndSumRed(obj);
    assert(restored > dimmed);
    std::printf("  restore: reassigning material.default brightens again (%lld)\n", restored);

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("material_render_test: ALL OK\n");
    return 0;
#endif
}
