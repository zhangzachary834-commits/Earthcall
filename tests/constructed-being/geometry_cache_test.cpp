// Geometry cache milestone test (OPENGL_MIGRATION_PLAN.md, Milestone 1):
//   "Geometry is rebuilt when it changes, not when it is drawn."
//
// The draw path used to call tessellateSmooth / tessellatePatch on every frame,
// rebuilding every surface in the world sixty times a second for geometry that
// changes only when a Person edits it. Those tessellations are now cached and
// built once per change by rebuildGeometryCaches().
//
// The render tessellations are private, so this drives the public observable that
// the same function fills under the same branches — getSupportCloud(). Inside a
// branch the render cache is assigned unconditionally, so a rebuilt cloud for a
// kind means that kind's render cache was rebuilt in the same pass. What matters
// is the invariant caching can break: an edit must still reach the mesh.

#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Object/Geometry/Sdf.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

bool sameCloud(const std::vector<glm::vec3>& a, const std::vector<glm::vec3>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i] != b[i]) return false;
    return true;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "geometry_cache_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "geometry_cache_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "geometry_cache_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    // --- Smooth surface: the cache must follow an edit -----------------------
    {
        Object o;
        Object::ShapeParams p; p.r = 0.5f;
        o.setShape(Object::ShapeKind::Sphere, p);
        assert(o.hasSmoothSurface());
        std::vector<glm::vec3> small = o.getSupportCloud();
        assert(!small.empty());

        p.r = 1.0f;
        o.setShape(Object::ShapeKind::Sphere, p);
        std::vector<glm::vec3> big = o.getSupportCloud();
        assert(!big.empty());
        assert(!sameCloud(small, big));
        std::printf("  smooth:  rebuilt on radius edit (%zu -> %zu pts)\n",
                    small.size(), big.size());

        Object ellipsoid;
        ellipsoid.setShapeKind(Object::ShapeKind::Ellipsoid);
        assert(ellipsoid.hasSmoothSurface());
        std::printf("  ellipsoid via setShapeKind is a SmoothSurface, not a tessellation leftover\n");

        geom::SdfNode asField = geom::sdfFromSmooth(o.getSmoothData());
        assert(asField.prim == geom::SdfPrim::Sphere);
        std::printf("  sphere quadric lowers to an SDF sphere leaf, not a UV mesh\n");
    }

    // --- Complex shape: per-patch caches must follow an edit -----------------
    {
        Object o;
        Object::ShapeParams p; p.r = 0.5f; p.halfH = 0.5f;
        o.setShape(Object::ShapeKind::Cylinder, p);
        assert(o.hasComplexShape());
        std::vector<glm::vec3> thin = o.getSupportCloud();
        assert(!thin.empty());

        p.r = 1.2f;
        o.setShape(Object::ShapeKind::Cylinder, p);
        std::vector<glm::vec3> fat = o.getSupportCloud();
        assert(!fat.empty());
        assert(!sameCloud(thin, fat));
        std::printf("  complex: rebuilt on radius edit (%zu -> %zu pts)\n",
                    thin.size(), fat.size());
    }

    // --- Field: the pre-existing cache still works --------------------------
    {
        Object o;
        geom::SdfNode n;
        n.op = geom::SdfOp::Leaf;
        n.prim = geom::SdfPrim::Sphere;
        n.dims = glm::vec3(0.5f);
        o.setFieldShape(n, 1.0f);
        assert(o.hasField());
        std::vector<glm::vec3> cloud = o.getSupportCloud();
        assert(!cloud.empty());
        std::printf("  field:   built on set (%zu pts)\n", cloud.size());
    }

    // --- Bezier patch: it must build a cache like the other topology kinds ----
    // Regression lock: setBezierPatch set _hasPatch and SpatialKind::Patch, but
    // rebuildGeometryCaches had no patch branch and returned early, so the support
    // cloud was empty and drawObject fell through to the primitive switch — a patch
    // rendered as a cube and could not be picked. An empty cloud here is that bug.
    {
        Object o;
        o.setBezierPatch(geom::makeBezierGrid(3, 3, 0.5f));
        assert(o.hasPatch());
        assert(o.getSpatialKind() == Object::SpatialKind::Patch);
        std::vector<glm::vec3> small = o.getSupportCloud();
        assert(!small.empty()); // the patch tessellation reached the cache

        o.setBezierPatch(geom::makeBezierGrid(3, 3, 1.0f));
        std::vector<glm::vec3> big = o.getSupportCloud();
        assert(!big.empty());
        assert(!sameCloud(small, big)); // and it tracks the edited control net
        std::printf("  patch:   rebuilt on control-net edit (%zu -> %zu pts)\n",
                    small.size(), big.size());
    }

    // --- Switching kind must not leave the old kind's cache in play ----------
    {
        Object o;
        Object::ShapeParams p;
        o.setShape(Object::ShapeKind::Sphere, p);
        assert(o.hasSmoothSurface());
        o.setShape(Object::ShapeKind::Cylinder, p);
        assert(o.hasComplexShape());
        assert(!o.hasSmoothSurface());
        assert(!o.getSupportCloud().empty());
        std::printf("  switch:  smooth -> complex leaves only the complex cache\n");
    }

    // --- The cached mesh must actually reach the framebuffer ----------------
    // Everything above passes even with an empty cache, because the support cloud
    // was always rebuilt. If _smoothMesh were empty the object would simply render
    // invisible. So draw offscreen and read the pixels back — and do it at two
    // radii, because coverage tracking the radius is what proves the cache holds
    // the *current* shape rather than merely something.
    {
        auto coverage = [&](float radius) -> size_t {
            Object o;
            Object::ShapeParams p; p.r = radius;
            o.setShape(Object::ShapeKind::Sphere, p);

            glViewport(0, 0, 64, 64);
            glMatrixMode(GL_PROJECTION); glLoadIdentity();
            glOrtho(-1.0, 1.0, -1.0, 1.0, -2.0, 2.0);
            glMatrixMode(GL_MODELVIEW); glLoadIdentity();
            glEnable(GL_DEPTH_TEST);
            // Not red/green/blue/black/white: initFaceTextures paints face 0 red, so
            // a red background would hide a sphere that drew correctly.
            glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            o.drawObject();
            glFinish();

            std::vector<unsigned char> px(64 * 64 * 4);
            glReadPixels(0, 0, 64, 64, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
            const int bg[3] = {64, 128, 191};
            size_t covered = 0;
            for (size_t i = 0; i < px.size(); i += 4) {
                bool background = std::abs(int(px[i])     - bg[0]) < 20
                               && std::abs(int(px[i + 1]) - bg[1]) < 20
                               && std::abs(int(px[i + 2]) - bg[2]) < 20;
                if (!background) ++covered;
            }
            return covered;
        };

        size_t smallPx = coverage(0.3f);
        size_t bigPx   = coverage(0.9f);
        assert(smallPx > 0);            // the cache is not empty
        assert(bigPx > smallPx);        // and it tracks the authored radius
        std::printf("  render:  cached mesh drew %zu px at r=0.3, %zu px at r=0.9\n",
                    smallPx, bigPx);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("geometry_cache_test: ALL OK\n");
    return 0;
}
