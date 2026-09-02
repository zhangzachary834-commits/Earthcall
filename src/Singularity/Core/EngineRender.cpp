#include "Singularity/Core/Engine.hpp"
#include "../../ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "../Screen/Camera.hpp"
#include "../Screen/Renderer.hpp"
#include "../Screen/ShadingSystem.hpp"
#include "../../ZonesOfEarth/ZoneManager.hpp"
#include "../../ZonesOfEarth/Zone/Zone.hpp"
#include "../../Person/Person.hpp"
#include "../../Person/Body/BodyPart/BodyPart.hpp"
#include "../../ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleWindow.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/PerformanceMetricsWindow.hpp"

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <algorithm>

extern ZoneManager mgr;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Core {
    void Engine::render() {
        if (!_window) return;

        int fbW, fbH;
        glfwGetFramebufferSize(_window, &fbW, &fbH);
        if (fbH == 0) fbH = 1;
        float aspect = static_cast<float>(fbW) / fbH;

        // Current active Zone's objects
        auto& zone = mgr.active();

        /**
         * --------------------
         * Projection
         *
         */
        float fov = 45.0f;
        float nearZ = 0.1f;
        float farZ  = 100.0f;
        float top   = tanf(fov * M_PI / 360.0f) * nearZ;
        float bottom = -top;
        float right  = top * aspect;
        float left   = -right;

        glm::mat4 proj = currentRenderer().zeroToOneDepth()
            ? glm::frustumZO(left, right, bottom, top, nearZ, farZ)
            : glm::frustumNO(left, right, bottom, top, nearZ, farZ);

        /* -------------------- */

        // Model-view (camera)
        glm::vec3 eyePos   = _camera->pos;
        glm::vec3 lookDir  = _camera->front;
        const float CAMERA_DISTANCE = 4.0f;

        if (_currentPerspective == PerspectiveMode::ThirdPerson) {
            eyePos  = _camera->pos - _camera->front * CAMERA_DISTANCE;
        } else if (_currentPerspective == PerspectiveMode::SecondPerson) {
            eyePos  = _camera->pos + _camera->front * CAMERA_DISTANCE;
        }

        glm::vec3 lookTarget = _camera->pos + lookDir;
        glm::mat4 view = glm::lookAt(eyePos, lookTarget, _camera->up);

        currentRenderer().setCamera(view, proj, eyePos);

        for (int i = 0; i < 16; ++i) {
            _camera->modelview[i]  = static_cast<GLdouble>(glm::value_ptr(view)[i]);
            _camera->projection[i] = static_cast<GLdouble>(glm::value_ptr(proj)[i]);
        }
        _camera->viewport[0] = 0;    _camera->viewport[1] = 0;
        _camera->viewport[2] = fbW;  _camera->viewport[3] = fbH;

        ShadingSystem::update(_camera->pos);

        {
            glm::vec4 clearColor(0.1f, 0.1f, 0.15f, 1.0f);
            if (_lawManager) {
                if (auto* sc = Singularity::Screen::ScreenChannel::find(*_lawManager)) {
                    clearColor = glm::vec4(sc->backgroundColor, 1.0f);
                    currentRenderer().setWireframe(sc->wireframe);
                    currentRenderer().setHeightGridDdaEnabled(sc->heightGridDdaEnabled);
                }
            }
            auto tB0 = std::chrono::steady_clock::now();
            currentRenderer().beginFrame(static_cast<uint32_t>(fbW), static_cast<uint32_t>(fbH), clearColor);
            auto tB1 = std::chrono::steady_clock::now();
            g_frameTimings.wait_surface_ms = std::chrono::duration<float, std::milli>(tB1 - tB0).count();
        }

        // Draw all owned objects cleanly (no hardcoded baseline mutation or skipping ground)
        const auto& objects = zone.getOwnedObjects();
        for (const auto& obj : objects) {
            if (obj) {
                currentRenderer().setModel(obj->getTransform());
                obj->drawObject();
                obj->drawHighlightOutline();
            }
        }
        currentRenderer().setModel(glm::mat4(1.0f)); // back to world space

        if (_creatorConsoleOpen) {
            Rendering::renderCreatorConsole3DPreviews(_person.get(), nullptr);
        }

        // Draw player avatar and nametag when not in first-person
        if (_currentPerspective != PerspectiveMode::FirstPerson) {
            _person->draw();
            _person->drawNametag();
        }

        _mainMenu.draw(fbW, fbH);

        // Draw 2D objects (Shape2D / Text2D) in screen space, after the 3D scene.
        // begin2D / end2D bracket installs the orthographic projection; objects are
        // sorted by zOrder2D so authored z-ordering is honoured.
        //
        // WINDOW POINTS, not framebuffer pixels — and the distinction is the whole
        // bug this line fixes. `x2D`/`y2D` are picked against `glfwGetCursorPos`,
        // which reports window points; opening the bracket with the FRAMEBUFFER
        // size made every Shape2D draw at 1/scale of its authored position while
        // staying clickable at the authored one, so on any Retina display the
        // visible rectangle and its hit region were in different places and no 2D
        // control could be clicked at all. Worse, the orphaned hit regions still
        // occluded the 3D pick, swallowing clicks aimed at the world behind them.
        // Invisible at scale 1, which is why it survived.
        //
        // Window points is the space to standardise on: it is what the cursor is
        // in, what ImGui is in, what Menu::draw already passes, and it is
        // resolution-independent, so an authored HUD lands in the same place on
        // every display. See Renderer::begin2D's contract.
        {
            int winW = fbW, winH = fbH;
            glfwGetWindowSize(_window, &winW, &winH);
            if (winW <= 0) winW = fbW;
            if (winH <= 0) winH = fbH;
            const uint32_t fbWu = static_cast<uint32_t>(winW);
            const uint32_t fbHu = static_cast<uint32_t>(winH);
            std::vector<Object*> objects2D;
            for (const auto& obj : objects) {
                if (obj && obj->is2D()) objects2D.push_back(obj.get());
            }
            std::stable_sort(objects2D.begin(), objects2D.end(),
                [](const Object* a, const Object* b) {
                    return a->getZOrder2D() < b->getZOrder2D();
                });
            if (!objects2D.empty()) {
                currentRenderer().begin2D(fbWu, fbHu);
                for (Object* obj : objects2D) {
                    obj->draw2DObject(fbWu, fbHu);
                }
                currentRenderer().end2D();
            }
        }

        auto tE0 = std::chrono::steady_clock::now();
        currentRenderer().endFrame();
        auto tE1 = std::chrono::steady_clock::now();
        g_frameTimings.wait_submit_ms = std::chrono::duration<float, std::milli>(tE1 - tE0).count();

        if (_lawManager) {
            if (auto* sc = Singularity::Screen::ScreenChannel::find(*_lawManager)) {
                const auto& stats = currentRenderer().frameStats();
                sc->updateMetrics(static_cast<int>(stats.drawCalls),
                                  static_cast<int>(stats.trianglesDrawn),
                                  static_cast<double>(stats.vramAllocatedBytes),
                                  static_cast<double>(stats.uniformBytesWritten),
                                  static_cast<int>(stats.bufferSuballocations),
                                  static_cast<int>(stats.pipelineSwitches),
                                  static_cast<int>(stats.cachedMeshesCount));
            }
        }
    }
}
