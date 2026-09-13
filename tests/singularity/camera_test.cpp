#include "Singularity/Screen/Camera.hpp"
#include <cstdio>
#include <glm/glm.hpp>

namespace {
    int g_failures = 0;

    void check(bool condition, const char* msg) {
        if (!condition) {
            std::printf("  FAILED: %s\n", msg);
            g_failures++;
        } else {
            std::printf("  ok: %s\n", msg);
        }
    }
}

int main() {
    std::printf("Running camera_test...\n");

    // Ensure clean state
    Core::Camera::shutdown();

    // Access singleton
    Core::Camera& camera = Core::Camera::instance();

    // Verify defaults
    check(camera.getPos() == glm::vec3(0.0f, 0.0f, 3.0f), "Default position is (0, 0, 3)");
    check(camera.getFront() == glm::vec3(0.0f, 0.0f, -1.0f), "Default front is (0, 0, -1)");
    check(camera.getUp() == glm::vec3(0.0f, 1.0f, 0.0f), "Default up is (0, 1, 0)");

    // Test accessors
    camera.setPos(glm::vec3(1.0f, 2.0f, 3.0f));
    check(camera.getPos() == glm::vec3(1.0f, 2.0f, 3.0f), "setPos updates position correctly");

    camera.setFront(glm::vec3(4.0f, 5.0f, 6.0f));
    check(camera.getFront() == glm::vec3(4.0f, 5.0f, 6.0f), "setFront updates front correctly");

    camera.setUp(glm::vec3(7.0f, 8.0f, 9.0f));
    check(camera.getUp() == glm::vec3(7.0f, 8.0f, 9.0f), "setUp updates up correctly");

    // Test matrix accessors
    GLdouble testMatrix[16];
    for (int i = 0; i < 16; i++) {
        testMatrix[i] = static_cast<GLdouble>(i);
    }
    camera.setModelview(testMatrix);
    const GLdouble* mv = camera.getModelview();
    bool mvOk = true;
    for (int i = 0; i < 16; i++) {
        if (mv[i] != testMatrix[i]) { mvOk = false; break; }
    }
    check(mvOk, "setModelview correctly updates the modelview matrix");

    for (int i = 0; i < 16; i++) {
        testMatrix[i] = static_cast<GLdouble>(i * 2);
    }
    camera.setProjection(testMatrix);
    const GLdouble* proj = camera.getProjection();
    bool projOk = true;
    for (int i = 0; i < 16; i++) {
        if (proj[i] != testMatrix[i]) { projOk = false; break; }
    }
    check(projOk, "setProjection correctly updates the projection matrix");

    int testViewport[4] = { 10, 20, 800, 600 };
    camera.setViewport(testViewport);
    const int* vp = camera.getViewport();
    bool vpOk = (vp[0] == 10 && vp[1] == 20 && vp[2] == 800 && vp[3] == 600);
    check(vpOk, "setViewport correctly updates the viewport");

    // Clean up
    Core::Camera::shutdown();

    if (g_failures > 0) {
        std::printf("camera_test FAILED with %d failures.\n", g_failures);
        return 1;
    }

    std::printf("camera_test PASSED.\n");
    return 0;
}
