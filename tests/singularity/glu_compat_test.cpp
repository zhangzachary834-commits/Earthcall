// GLU replacement correctness (OPENGL_MIGRATION_PLAN.md, Milestone 3).
//
// ecgl::project / unProject replace gluProject / gluUnProject, which drove mouse
// picking. No other test exercises that math, and a bug there breaks picking
// silently (a ray that lands slightly wrong still "works", just on the wrong
// object). So this pins the two invariants that matter:
//   1. project and unProject are inverses (screen<->world round trip).
//   2. project matches a hand-computed result for a known simple projection,
//      so the two functions can't be wrong in a way that cancels out.

#include "Singularity/Screen/GL/GluCompat.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

bool neard(double a, double b, double eps = 1e-4) { return std::fabs(a - b) < eps; }

// GL matrices are column-major GLdouble[16]; glm is column-major too, so a copy
// of the raw storage is the right conversion.
void toArray(const glm::dmat4& m, double out[16]) {
    const double* p = glm::value_ptr(m);
    for (int i = 0; i < 16; ++i) out[i] = p[i];
}

} // namespace

int main() {
    const int viewport[4] = {0, 0, 800, 600};

    // A realistic camera: perspective projection + a modelview that pushes the
    // world back and tilts it, so the test isn't accidentally axis-aligned.
    glm::dmat4 proj = glm::perspective(glm::radians(45.0), 800.0 / 600.0, 0.1, 100.0);
    glm::dmat4 model = glm::translate(glm::dmat4(1.0), glm::dvec3(0.0, 0.0, -5.0))
                     * glm::rotate(glm::dmat4(1.0), 0.6, glm::dvec3(0.0, 1.0, 0.0));
    double projA[16], modelA[16];
    toArray(proj, projA);
    toArray(model, modelA);

    // --- 1. Round trip: world -> screen -> world recovers the point ----------
    {
        const glm::dvec3 pts[] = {
            {0.0, 0.0, 0.0}, {1.0, 0.5, -0.3}, {-0.7, 1.2, 0.9}, {2.0, -1.0, 0.4}
        };
        for (const auto& p : pts) {
            double wx, wy, wz;
            bool ok = ecgl::project(p.x, p.y, p.z, modelA, projA, viewport, &wx, &wy, &wz);
            assert(ok);
            double rx, ry, rz;
            ok = ecgl::unProject(wx, wy, wz, modelA, projA, viewport, &rx, &ry, &rz);
            assert(ok);
            assert(neard(rx, p.x) && neard(ry, p.y) && neard(rz, p.z));
        }
        std::printf("  round-trip: project->unProject recovers 4 world points\n");
    }

    // --- 2. Known case: a point dead ahead lands at the screen center --------
    // Identity modelview, so the object point IS the eye-space point. A point on
    // the -Z axis must project to the center of the viewport (400, 300).
    {
        double idA[16];
        toArray(glm::dmat4(1.0), idA);
        double wx, wy, wz;
        bool ok = ecgl::project(0.0, 0.0, -10.0, idA, projA, viewport, &wx, &wy, &wz);
        assert(ok);
        assert(neard(wx, 400.0) && neard(wy, 300.0));
        // And it must sit between the near and far planes in window Z (0..1).
        assert(wz > 0.0 && wz < 1.0);
        std::printf("  known:      on-axis point projects to center (%.1f, %.1f), z=%.3f\n",
                    wx, wy, wz);
    }

    // --- 3. Degenerate matrix is reported, not silently wrong ----------------
    {
        double zero[16] = {0};
        double ox, oy, oz;
        bool ok = ecgl::unProject(400, 300, 0.5, modelA, zero, viewport, &ox, &oy, &oz);
        assert(!ok); // singular proj*model -> inverse degenerate -> false, like GLU
        std::printf("  degenerate: singular matrix returns false\n");
    }

    std::printf("glu_compat_test: ALL OK\n");
    return 0;
}
