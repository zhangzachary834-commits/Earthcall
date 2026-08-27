#include "Engine.hpp"
#include "../../ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "../Screen/Camera.hpp"
#include "Singularity/Input/Keyboard/KeyboardHandler.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Input/Locomotion/LocomotionChannel.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreationTools.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp"
#include "Singularity/Core/SdfBuild.hpp"
#include "Singularity/Screen/GL/GluCompat.hpp"
#include "../../Person/Person.hpp"
#include "../../ZonesOfEarth/ZoneManager.hpp"
#include "../../ZonesOfEarth/Physics/Physics.hpp"
#include "../../ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "../../ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

extern ZoneManager mgr;

namespace Core {
    void Engine::fuseObjects(Object* A, Object* B) {
        if (!A || !B || A == B) return;
        const glm::mat4 Ta = A->getTransform();
        const glm::mat4 Tb = B->getTransform();
        geom::SdfNode an = objectToSdfNode(*A);
        geom::SdfNode bn = objectToSdfNode(*B);
        bn.offset = glm::vec3(glm::inverse(Ta) * glm::vec4(glm::vec3(Tb[3]), 1.0f));
        const geom::SdfOp ops[] = { geom::SdfOp::Union, geom::SdfOp::Intersect,
                                    geom::SdfOp::Subtract, geom::SdfOp::SmoothUnion,
                                    geom::SdfOp::Morph };
        auto& state = Rendering::getCreatorConsoleState();
        int oi = state.combineOp;
        if (oi < 0 || oi > 4) oi = 2;
        const float blend = (oi >= 3) ? state.combineBlend : 0.5f;
        A->setFieldShape(geom::SdfNode::binary(ops[oi], an, bn, blend), 1.6f);
        mgr.active().removeObject(B);
        if (state.selectedObject3D == B) state.selectedObject3D = A;
        if (state.combineOperandA == B) state.combineOperandA = nullptr;
        if (state.clayGrabbed == B) state.clayGrabbed = nullptr;
        if (state.clayTarget == B) state.clayTarget = nullptr;
    }

    void Engine::blendRail(const Object* o, glm::vec3& start, glm::vec3& dir, float& length) const {
        if (!o) return;
        const glm::mat4 xf = o->getTransform();
        const glm::vec3 C = glm::vec3(xf[3]);
        const float scale = (glm::length(glm::vec3(xf[0])) +
                             glm::length(glm::vec3(xf[1])) +
                             glm::length(glm::vec3(xf[2]))) / 3.0f;
        length = std::max(0.4f, scale * 0.8f);
        start = C + glm::vec3(0.0f, scale * 0.7f, 0.0f);
        dir = glm::vec3(1.0f, 0.0f, 0.0f);
        if (_camera) {
            glm::vec3 right = glm::normalize(glm::cross(_camera->getFront(), _camera->getUp()));
            if (glm::length(right) > 1e-4f) dir = right;
        }
    }

    bool Engine::handleFieldGizmos(Object* o, bool pressEdge, bool mouseDown, double winX, double winY) {
        auto& state = Rendering::getCreatorConsoleState();
        if (!o || !o->isBinaryField() || !_camera) {
            state.blendHandleDragging = false;
            state.fieldHandleDragging = false;
            return false;
        }
        const GLdouble* mv = _camera->getModelview();
        const GLdouble* pr = _camera->getProjection();
        const int* vp = _camera->getViewport();
        const glm::mat4 xf = o->getTransform();

        const glm::vec3 hbW = glm::vec3(xf * glm::vec4(o->getFieldOperandBOffset(), 1.0f));
        GLdouble bx, by, bz;
        const bool bProj = ecgl::project(hbW.x, hbW.y, hbW.z, mv, pr, vp, &bx, &by, &bz);

        const bool blendable = o->isMorphField();
        glm::vec3 rs(0.0f), rd(0.0f); float rl = 1.0f;
        GLdouble cx = 0, cy = 0, cz = 0; bool cProj = false;
        if (blendable) {
            blendRail(o, rs, rd, rl);
            const glm::vec3 beadW = rs + rd * (o->getMorphParam() * rl);
            cProj = ecgl::project(beadW.x, beadW.y, beadW.z, mv, pr, vp, &cx, &cy, &cz);
        }

        if (!mouseDown) {
            state.blendHandleDragging = false;
            state.fieldHandleDragging = false;
        }
        if (pressEdge) {
            const double dB = bProj ? (bx - winX) * (bx - winX) + (by - winY) * (by - winY) : 1e18;
            const double dC = cProj ? (cx - winX) * (cx - winX) + (cy - winY) * (cy - winY) : 1e18;
            const double R = 22.0 * 22.0;
            if (blendable && dC < R && dC <= dB) state.blendHandleDragging = true;
            else if (dB < R) state.fieldHandleDragging = true;
        }
        if (mouseDown && state.blendHandleDragging && blendable) {
            GLdouble nx, ny, nz;
            if (ecgl::unProject(winX, winY, cz, mv, pr, vp, &nx, &ny, &nz)) {
                const glm::vec3 P((float)nx, (float)ny, (float)nz);
                const float t = glm::clamp(glm::dot(P - rs, rd) / (rl > 1e-6f ? rl : 1.0f), 0.0f, 1.0f);
                o->setMorphParam(t);
            }
            return true;
        }
        if (mouseDown && state.fieldHandleDragging && bProj) {
            GLdouble nx, ny, nz;
            if (ecgl::unProject(winX, winY, bz, mv, pr, vp, &nx, &ny, &nz)) {
                const glm::vec3 local = glm::vec3(glm::inverse(xf) * glm::vec4((float)nx, (float)ny, (float)nz, 1.0f));
                o->setFieldOperandBOffset(local);
            }
            return true;
        }
        return state.blendHandleDragging || state.fieldHandleDragging;
    }
    
    void Engine::update(float dt) {
        if (!_keyboardHandler || !_mouseHandler || !_camera || !_player || !_lawManager) return;

        // Update input handlers
        if (_mainMenu.isOpen()) {
            _mainMenu.processInput(_window);
        }
        
        _keyboardHandler->update();
        _mouseHandler->update();

        // Update camera front from mouse handler
        _camera->front = _mouseHandler->calculateCameraFront();

        // Check if any text input is active (ImGui)
        bool anyTextInputActive = ImGui::IsAnyItemActive() || ImGui::IsWindowFocused();

        // Vessel movement — Input first mover, not Person.
        const bool canMove = _mouseHandler->isCursorLocked() && !_mainMenu.isOpen() && !anyTextInputActive;
        const bool flying  = Physics::getFlying();
        if (auto* locomotion = Singularity::Input::LocomotionChannel::find(*_lawManager)) {
            locomotion->step(*_player, *_camera, _window, mgr, dt, flying, canMove);
        }

        // Creation first mover — sense placement, honour L, push the
        // console's live selection onto the channel, actuate the armed
        // tool. Used to run inside render3DConsole / DeveloperToolsWindow,
        // so collapsing the console froze every 3D tool.
        Rendering::stepCreationTools(_window, this, mgr, dt, _creatorConsoleOpen);

        // Interaction first mover — pick the being under the pointer, publish
        // the click/scroll/focus edges, drive hover. Stepped here and not from
        // a render function, for the reason above it: a channel that only runs
        // while a window is on screen is a channel that freezes when the
        // window collapses.
        //
        // WantCaptureMouse is the foreign-surface veto: while an ImGui panel
        // owns the pointer, the world must see no pointer at all, or the
        // Person clicks a menu and a button behind it fires too.
        if (auto* interaction = Singularity::Input::InteractionChannel::find(*_lawManager)) {
            interaction->step(_window, *_camera, mgr, ImGui::GetIO().WantCaptureMouse);
        }

        // Update world (physics etc.)
        mgr.active().update(dt);
        mgr.active().applyFormationRelations();

        // Evict unreferenced smooth tessellation caches from destroyed or modified objects
        Object::gcSmoothTessellationCache();

        // Advance time
        _worldTime += static_cast<double>(dt);
        Universe::instance().setClock(_worldTime, static_cast<double>(dt));
    }
}
