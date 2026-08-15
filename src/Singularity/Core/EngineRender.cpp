#include "Singularity/Core/Engine.hpp"
#include "../../ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "../Screen/Camera.hpp"
#include "../Screen/Renderer.hpp"
#include "../Screen/ShadingSystem.hpp"
#include "../../ZonesOfEarth/ZoneManager.hpp"
#include "../../ZonesOfEarth/Zone/Zone.hpp"
#include "../../Person/Person.hpp"
#include "../../Person/Body/BodyPart/BodyPart.hpp"
#include "../../ConstructedBeing/Object/Object.hpp"
#include "Singularity/FirstMoverWindowTools/CreatorConsole/CreatorConsoleWindow.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

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

        // Current active zone's 3-D world
        auto& zoneWorld = mgr.active().world();
        zoneWorld.setCamera(&_camera->pos);
        zoneWorld.setPlayerEyeHeight(_player->getBody().getEyeHeight());

        // Projection
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
            currentRenderer().beginFrame(static_cast<uint32_t>(fbW), static_cast<uint32_t>(fbH),
                                         glm::vec4(0.1f, 0.1f, 0.15f, 1.0f));
        }

        // Draw all owned objects cleanly (no hardcoded baseline mutation or skipping ground)
        const auto& objects = zoneWorld.getOwnedObjects();
        for (const auto& obj : objects) {
            if (obj) {
                currentRenderer().setModel(obj->getTransform());
                obj->drawObject();
                obj->drawHighlightOutline();
            }
        }
        currentRenderer().setModel(glm::mat4(1.0f)); // back to world space

        if (_creatorConsoleOpen) {
            Rendering::renderCreatorConsole3DPreviews(_player.get(), nullptr);
        }

        // Draw player avatar and nametag when not in first-person
        if (_currentPerspective != PerspectiveMode::FirstPerson) {
            _player->draw();
            _player->drawNametag();
        }

        _mainMenu.draw(fbW, fbH);

        currentRenderer().endFrame();
    }
}
