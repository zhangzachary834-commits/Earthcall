// GameUpdate.cpp – Game::update() logic
// Split from Game.cpp during refactor.

#include "Game.hpp"
#include "Singularity/Core/Engine.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "Singularity/Core/SdfBuild.hpp"
#include "OurVerse/Tool.hpp"
#include "OurVerse/AdvancedFacePaint.hpp"
#include "Rendering/BrushSystem.hpp"
#include "Rendering/HighlightSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Person/PersonEvents.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/PhysicsLawBridge.hpp"
#include "Singularity/Physical/PhysicalChannel.hpp"

#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"

#include <GLFW/glfw3.h>
#include "Rendering/GL/GluCompat.hpp"
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

extern ZoneManager mgr;

using glm::vec3;

namespace Core {

// Fuse B into A in place: A becomes (A op B) as an SDF field, and B is consumed
// (removed from the world — it lives on inside A's field tree as operand B, which
// stays draggable in Morph mode). B's centre is baked into A's local frame so the
// op happens where B sits. Op + blend come from _combineOp / _combineBlend. Shared
// by the Combine tool (click A, click B) and the Clay tool (drag-to-overlap).
void Game::fuseObjects(Object* A, Object* B) {
    if (!A || !B || A == B) return;
    const glm::mat4 Ta = A->getTransform();
    const glm::mat4 Tb = B->getTransform();
    geom::SdfNode an = objectToSdfNode(*A);
    geom::SdfNode bn = objectToSdfNode(*B);
    bn.offset = glm::vec3(glm::inverse(Ta) * glm::vec4(glm::vec3(Tb[3]), 1.0f));
    const geom::SdfOp ops[] = { geom::SdfOp::Union, geom::SdfOp::Intersect,
                                geom::SdfOp::Subtract, geom::SdfOp::SmoothUnion,
                                geom::SdfOp::Morph };
    int oi = _combineOp; if (oi < 0 || oi > 4) oi = 2;
    const float blend = (oi >= 3) ? _combineBlend : 0.5f;
    A->setFieldShape(geom::SdfNode::binary(ops[oi], an, bn, blend), 1.6f);
    auto& owned = mgr.active().world().getOwnedObjectsMutable();
    owned.erase(std::remove_if(owned.begin(), owned.end(),
                [B](const std::shared_ptr<Object>& p){ return p.get() == B; }),
                owned.end());
}

// A short screen-aligned rail above the shape; the bead's position along it maps
// to the blend t in [0,1]. Computed identically here and in GameRender so the
// drawn bead and the hit-test agree.
void Game::blendRail(const Object* o, glm::vec3& start, glm::vec3& dir, float& length) const {
    const glm::mat4 xf = o->getTransform();
    const glm::vec3 C  = glm::vec3(xf[3]);
    const float scale = (glm::length(glm::vec3(xf[0])) +
                         glm::length(glm::vec3(xf[1])) +
                         glm::length(glm::vec3(xf[2]))) / 3.0f;
    const glm::vec3 f     = glm::normalize(_camera.front);
    const glm::vec3 wup   = glm::normalize(_camera.up);
    const glm::vec3 right = glm::normalize(glm::cross(f, wup));
    const glm::vec3 up    = glm::normalize(glm::cross(right, f));
    length = 1.2f * scale;
    start  = C + up * (0.95f * scale) - right * (length * 0.5f);
    dir    = right;
}

bool Game::handleFieldGizmos(Object* o, bool pressEdge, bool mouseDown, double winX, double winY) {
    if (!o || !o->isBinaryField()) { _blendHandleDragging = false; _fieldHandleDragging = false; return false; }
    const GLdouble* mv = _camera.modelview;
    const GLdouble* pr = _camera.projection;
    const int*      vp = _camera.viewport;
    const glm::mat4 xf = o->getTransform();

    // Operand-B drag handle (gold cube at operand B's offset).
    const glm::vec3 hbW = glm::vec3(xf * glm::vec4(o->getFieldOperandBOffset(), 1.0f));
    GLdouble bx, by, bz; const bool bProj = ecgl::project(hbW.x, hbW.y, hbW.z, mv, pr, vp, &bx, &by, &bz);

    // Blend bead (only meaningful for Morph / SmoothUnion fields).
    const bool blendable = o->isMorphField();
    glm::vec3 rs(0.0f), rd(0.0f); float rl = 1.0f;
    GLdouble cx = 0, cy = 0, cz = 0; bool cProj = false;
    if (blendable) {
        blendRail(o, rs, rd, rl);
        const glm::vec3 beadW = rs + rd * (o->getMorphParam() * rl);
        cProj = ecgl::project(beadW.x, beadW.y, beadW.z, mv, pr, vp, &cx, &cy, &cz);
    }

    if (!mouseDown) { _blendHandleDragging = false; _fieldHandleDragging = false; }

    if (pressEdge) {
        const double dB = bProj ? (bx - winX) * (bx - winX) + (by - winY) * (by - winY) : 1e18;
        const double dC = cProj ? (cx - winX) * (cx - winX) + (cy - winY) * (cy - winY) : 1e18;
        const double R = 22.0 * 22.0;
        if (blendable && dC < R && dC <= dB) _blendHandleDragging = true;
        else if (dB < R)                     _fieldHandleDragging = true;
    }

    if (mouseDown && _blendHandleDragging && blendable) {
        GLdouble nx, ny, nz;
        if (ecgl::unProject(winX, winY, cz, mv, pr, vp, &nx, &ny, &nz)) {
            const glm::vec3 P((float)nx, (float)ny, (float)nz);
            const float t = glm::clamp(glm::dot(P - rs, rd) / (rl > 1e-6f ? rl : 1.0f), 0.0f, 1.0f);
            o->setMorphParam(t);
        }
        return true;
    }
    if (mouseDown && _fieldHandleDragging && bProj) {
        GLdouble nx, ny, nz;
        if (ecgl::unProject(winX, winY, bz, mv, pr, vp, &nx, &ny, &nz)) {
            const glm::vec3 local = glm::vec3(glm::inverse(xf) * glm::vec4((float)nx, (float)ny, (float)nz, 1.0f));
            o->setFieldOperandBOffset(local);
        }
        return true;
    }
    return _blendHandleDragging || _fieldHandleDragging;
}

void Game::update(float dt) {
    // Update input handlers
    _keyboardHandler.update();
    _keyboardHandler.updateGameInput(_window);
    _mouseHandler.update();

    // Update camera front from mouse handler
    _camera.front = _mouseHandler.calculateCameraFront();

    // Check if any text input is active (ImGui)
    bool anyTextInputActive = ImGui::IsAnyItemActive() || ImGui::IsWindowFocused();

    // Debug Coordinates Toggle (Cmd+Shift+S)
    if (!anyTextInputActive) {
        static bool s_cmdShiftS_wasDown = false;
        bool cmdDown = glfwGetKey(_window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS || 
                       glfwGetKey(_window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;
        bool shiftDown = glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
                         glfwGetKey(_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        bool sDown = glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS;
        
        bool currentDown = cmdDown && shiftDown && sDown;
        if (currentDown && !s_cmdShiftS_wasDown) {
            _showDebugCoordinates = !_showDebugCoordinates;
        }
        s_cmdShiftS_wasDown = currentDown;
    }

    // ----------------------------------------------------------------------------
    // Player movement: a single authoritative resolve owns _camera.pos.
    // (Horizontal input + gravity/jump + ground contact, now in Person::stepMovement.)
    // ----------------------------------------------------------------------------
    const bool canMove = _mouseHandler.isCursorLocked() && !_mainMenu.isOpen() && !anyTextInputActive;
    const bool flying  = Physics::getFlying();
    _player.stepMovement(dt, _window, &_camera, &mgr, flying, canMove);

    // Tool-only key handling that previously rode along with the movement block.
    if (_mouseHandler.isCursorLocked() && !_mainMenu.isOpen() && !anyTextInputActive) {
        // Reset anchor if mode switched out of ManualDistance
        if(_placement.mode != BrushPlacementMode::ManualDistance){ _placement.anchorValid = false; }

        // Manual offset tweak with keys when using ManualDistance - only when not typing
        if (_placement.mode == BrushPlacementMode::ManualDistance && _current3DMode == Mode3D::BrushCreate && !anyTextInputActive) {
            float step = 0.1f;
            if (glfwGetKey(_window, GLFW_KEY_RIGHT) == GLFW_PRESS) _placement.manualOffset.x += step;
            if (glfwGetKey(_window, GLFW_KEY_LEFT)  == GLFW_PRESS) _placement.manualOffset.x -= step;
            if (glfwGetKey(_window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) _placement.manualOffset.y += step;
            if (glfwGetKey(_window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) _placement.manualOffset.y -= step;
            if (glfwGetKey(_window, GLFW_KEY_UP)   == GLFW_PRESS) _placement.manualOffset.z += step;
            if (glfwGetKey(_window, GLFW_KEY_DOWN) == GLFW_PRESS) _placement.manualOffset.z -= step;
        }
    }

    if (!_mainMenu.isOpen() && _current3DMode == Mode3D::BrushCreate && !anyTextInputActive) {
        const bool shiftDown = glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                               glfwGetKey(_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        const bool ctrlDown = glfwGetKey(_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                              glfwGetKey(_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        float step = 60.0f * dt;
        if (shiftDown) step *= 3.0f;
        if (ctrlDown) step *= 0.25f;

        glm::vec3 rotation = _brush.rotation;
        if (glfwGetKey(_window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)  rotation.y -= step;
        if (glfwGetKey(_window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) rotation.y += step;
        if (glfwGetKey(_window, GLFW_KEY_SEMICOLON) == GLFW_PRESS)     rotation.x -= step;
        if (glfwGetKey(_window, GLFW_KEY_APOSTROPHE) == GLFW_PRESS)    rotation.x += step;
        if (glfwGetKey(_window, GLFW_KEY_COMMA) == GLFW_PRESS)         rotation.z -= step;
        if (glfwGetKey(_window, GLFW_KEY_PERIOD) == GLFW_PRESS)        rotation.z += step;

        for (int axis = 0; axis < 3; ++axis) {
            while (rotation[axis] > 180.0f) rotation[axis] -= 360.0f;
            while (rotation[axis] < -180.0f) rotation[axis] += 360.0f;
        }
        _brush.rotation = rotation;
    }


    // Simple cube rotation animation
    _cubeAngle += 50.0f * dt; // degrees/sec
    if (_cubeAngle > 360.0f) _cubeAngle -= 360.0f;

    // --------------------------------------------------------------
    // Creation Tools
    // --------------------------------------------------------------
    {
        const ImGuiIO& io = ImGui::GetIO();
        bool uiWantsMouse = io.WantCaptureMouse || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
        bool overUI = uiWantsMouse || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered();
        if (!overUI) {
        bool mouseLeftNow = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        double xpos, ypos;
        glfwGetCursorPos(_window, &xpos, &ypos);
        int winW, winH; glfwGetWindowSize(_window,&winW,&winH);
        int fW, fH; glfwGetFramebufferSize(_window,&fW,&fH);
        float scaleX = static_cast<float>(fW)/winW;
        float scaleY = static_cast<float>(fH)/winH;
        float mx = static_cast<float>(xpos*scaleX);
        float my = static_cast<float>(ypos*scaleY);

        setCursorX(mx);
        setCursorY(my);

        if (mouseLeftNow && !_mouseLeftPressedLast) {
            Core::EventBus::instance().publish(
                ECA::Event{"mouse-clicked", &_player, nullptr, static_cast<std::time_t>(_worldTime)});
        }

        auto collect3DTargets = [&](std::vector<Object*>& targets) {
            targets.clear();
            const auto& objects = mgr.active().world().getOwnedObjects();
            for (const auto& up : objects) {
                targets.push_back(up.get());
            }
        };
        std::vector<Singular*> formationMembers;
        formationMembers.reserve(mgr.active().world().getOwnedObjects().size() + _player.getBody().parts.size() * 2);
        for (const auto& up : mgr.active().world().getOwnedObjects()) {
            if (up) formationMembers.push_back(up.get());
        }
        for (auto* part : _player.getBody().parts) {
            if (!part) continue;
            formationMembers.push_back(static_cast<Object*>(part));
            for (const auto& sub : part->getSubObjects()) {
                if (sub) formationMembers.push_back(sub.get());
            }
        }
        mgr.active().syncFormationMembers(formationMembers);
        std::vector<Object*> toolTargets;

        // 2D Creation
        if (_current3DMode == Mode3D::None) {
            // Active Zone
            Zone& zone = mgr.active();

            Tool::Type currentToolType = _currentTool.getType();

            if (currentToolType == Tool::Type::Brush) {
                // Check for Shift key to enable straight line mode
                bool shiftPressed = glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                                   glfwGetKey(_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

                // Straight line mode (either from button or Shift+click)
                if (_drawingStraightLine || _straightLineMode || shiftPressed) {
                    if (mouseLeftNow && !_mouseLeftPressedLast) {
                        _drawingStraightLine = true;
                        _straightLineStartX = mx;
                        _straightLineStartY = my;
                        _straightLineEndX = mx;
                        _straightLineEndY = my;
                        if (!zone.getBrushSystem()) {
                            zone.initializeBrushSystem();
                        }
                    } else if (_drawingStraightLine && mouseLeftNow) {
                        _straightLineEndX = mx;
                        _straightLineEndY = my;
                    } else if (_drawingStraightLine && !mouseLeftNow) {
                        zone.startStroke(_straightLineStartX, _straightLineStartY);
                        if (std::abs(_straightLineEndX - _straightLineStartX) > 0.5f ||
                            std::abs(_straightLineEndY - _straightLineStartY) > 0.5f) {
                            zone.continueStroke(_straightLineEndX, _straightLineEndY);
                        }
                        zone.endStroke();
                        _drawingStraightLine = false;
                    }
                } else {
                    // Ensure design system is initialized
                    if (!zone.getDesignSystem()) {
                        zone.initializeDesignSystem();
                    }

                    static Tool::Type lastToolType = Tool::Type::Brush;
                    if (currentToolType != lastToolType) {
                        printf("Tool changed to: %s (%s)\n", _currentTool.getTypeName().c_str(), _currentTool.getIcon().c_str());
                        lastToolType = currentToolType;
                    }

                    Tool::use(_window, mgr, zone, currentToolType, *this);
                }
            } else {
                Tool::use(_window, mgr, zone, currentToolType, *this);
            }
        }

        /* 3D Creation */
        else if (_current3DMode == Mode3D::BrushCreate) {
            // Note: C++ hardcoded Tool::ShapeGenerator3D was removed here.
            // The tool is now implemented dynamically via Law and triggers on
            // the "mouse-clicked" event. Body-part targeting, which the tool
            // did by hand off the selected part, is authored instead: the
            // spawn action's parent path resolves "cursorHoveredBodyPart".
        } else if (_current3DMode == Mode3D::Pottery) {
            collect3DTargets(toolTargets);
            Tool::Pottery3D(_window, this, mgr, dt, toolTargets, nullptr);
        } else if (_current3DMode == Mode3D::Rotation) {
            collect3DTargets(toolTargets);
            Tool::Rotate3D(_window, this, mgr, dt, toolTargets, nullptr);
        } else if (_current3DMode == Mode3D::Selection) {
            collect3DTargets(toolTargets);
            if (mouseLeftNow && !_mouseLeftPressedLast) {
                bool extendSelection =
                    glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                    glfwGetKey(_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS ||
                    glfwGetKey(_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                    glfwGetKey(_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS ||
                    glfwGetKey(_window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
                    glfwGetKey(_window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;
                selectObject3D(Tool::PickObject3D(_window, this, toolTargets), extendSelection);
            }
            if (_selectedObject3D) {
                ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
                ImGui::Begin("SelectionHUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
                ImGui::Text("Selected: %s", _selectedObject3D->getIdentifier().c_str());
                ImGui::Text("Formation: %d objects, %d relations",
                            static_cast<int>(_selectedFormation3D.getMembers().size()),
                            static_cast<int>(_selectedFormation3D.relations().getAll().size()));
                ImGui::End();
            }
        } else if (_current3DMode == Mode3D::FacePaint) {
            collect3DTargets(toolTargets);
            Tool::FacePaint(_window, this, mgr, dt, toolTargets);
        } else if (_current3DMode == Mode3D::FaceBrush) {
            collect3DTargets(toolTargets);
            Tool::FaceBrush(_window, this, mgr, dt, toolTargets);
        } else if (_current3DMode == Mode3D::Morph) {
            // Direct topology editing: drag a polyhedron's vertices ("waterbending").
            Object* obj = _selectedObject3D;
            if (obj && obj->getGeometryType() == Object::GeometryType::Polyhedron &&
                obj->getPolyhedronVertexCount() > 0) {
                const GLdouble* mv = _camera.modelview;
                const GLdouble* pr = _camera.projection;
                const int*      vp = _camera.viewport;
                glm::mat4 xf = obj->getTransform();
                double winX = xpos * scaleX;
                double winY = vp[3] - ypos * scaleY; // GL y-up

                // On press: pick the nearest vertex in screen space.
                if (mouseLeftNow && !_mouseLeftPressedLast) {
                    int best = -1; double bestD = 1e18;
                    for (int i = 0; i < obj->getPolyhedronVertexCount(); ++i) {
                        glm::vec3 w = glm::vec3(xf * glm::vec4(obj->getPolyhedronVertexLocal(i), 1.0f));
                        GLdouble sx, sy, sz;
                        if (ecgl::project(w.x, w.y, w.z, mv, pr, vp, &sx, &sy, &sz)) {
                            double d = (sx - winX) * (sx - winX) + (sy - winY) * (sy - winY);
                            if (d < bestD) { bestD = d; best = i; }
                        }
                    }
                    if (best >= 0 && bestD < 40.0 * 40.0) _morphVertexIndex = best;
                }

                // While held: drag the selected vertex to the cursor at its depth.
                if (mouseLeftNow && _morphVertexIndex >= 0 &&
                    _morphVertexIndex < obj->getPolyhedronVertexCount()) {
                    glm::vec3 wv = glm::vec3(xf * glm::vec4(obj->getPolyhedronVertexLocal(_morphVertexIndex), 1.0f));
                    GLdouble sx, sy, sz;
                    if (ecgl::project(wv.x, wv.y, wv.z, mv, pr, vp, &sx, &sy, &sz)) {
                        GLdouble nx, ny, nz;
                        if (ecgl::unProject(winX, winY, sz, mv, pr, vp, &nx, &ny, &nz)) {
                            glm::vec3 local = glm::vec3(glm::inverse(xf) *
                                glm::vec4((float)nx, (float)ny, (float)nz, 1.0f));
                            obj->setPolyhedronVertexLocal(_morphVertexIndex, local);
                        }
                    }
                }
            } else if (obj && obj->isBinaryField()) {
                // Embodied editing: drag operand B's gold handle to move the second
                // shape, or the blend bead to set the blend/smoothness; result
                // re-forms live. (Shared gizmo path with Combine and Clay.)
                handleFieldGizmos(obj, mouseLeftNow && !_mouseLeftPressedLast,
                                  mouseLeftNow, xpos * scaleX,
                                  _camera.viewport[3] - ypos * scaleY);
            } else if (obj && obj->isPatch() && obj->getPatchControlCount() > 0) {
                // Drag the control points of a Bezier surface (waterbending the
                // control net; the surface re-forms live).
                const GLdouble* mv = _camera.modelview;
                const GLdouble* pr = _camera.projection;
                const int*      vp = _camera.viewport;
                glm::mat4 xf = obj->getTransform();
                double winX = xpos * scaleX;
                double winY = vp[3] - ypos * scaleY;
                if (mouseLeftNow && !_mouseLeftPressedLast) {
                    int best = -1; double bestD = 1e18;
                    for (int i = 0; i < obj->getPatchControlCount(); ++i) {
                        glm::vec3 w = glm::vec3(xf * glm::vec4(obj->getPatchControlLocal(i), 1.0f));
                        GLdouble sx, sy, sz;
                        if (ecgl::project(w.x, w.y, w.z, mv, pr, vp, &sx, &sy, &sz)) {
                            double d = (sx - winX) * (sx - winX) + (sy - winY) * (sy - winY);
                            if (d < bestD) { bestD = d; best = i; }
                        }
                    }
                    if (best >= 0 && bestD < 40.0 * 40.0) _patchCtrlIndex = best;
                }
                if (mouseLeftNow && _patchCtrlIndex >= 0 &&
                    _patchCtrlIndex < obj->getPatchControlCount()) {
                    glm::vec3 wv = glm::vec3(xf * glm::vec4(obj->getPatchControlLocal(_patchCtrlIndex), 1.0f));
                    GLdouble sx, sy, sz;
                    if (ecgl::project(wv.x, wv.y, wv.z, mv, pr, vp, &sx, &sy, &sz)) {
                        GLdouble nx, ny, nz;
                        if (ecgl::unProject(winX, winY, sz, mv, pr, vp, &nx, &ny, &nz)) {
                            glm::vec3 local = glm::vec3(glm::inverse(xf) *
                                glm::vec4((float)nx, (float)ny, (float)nz, 1.0f));
                            obj->setPatchControlLocal(_patchCtrlIndex, local);
                        }
                    }
                }
            }
        }
        else if (_current3DMode == Mode3D::Combine) {
            // In-scene boolean/blend: click shape A, then shape B. The result
            // (A op B) replaces A in place; B is consumed into it (it becomes the
            // draggable operand-B ghost editable in Morph mode). Right-click clears A.
            std::vector<Object*> targets;
            for (const auto& up : mgr.active().world().getOwnedObjects())
                if (up && !(up->hasAttribute("baseline") &&
                            up->getAttribute("baseline") == std::string("ground")))
                    targets.push_back(up.get());

            if (glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
                _combineOperandA = nullptr;

            // Refine the current result in place: if the left-drag grabbed one of
            // the selected field's gizmos (operand B / blend bead), don't also pick.
            const bool gizmo = handleFieldGizmos(_selectedObject3D,
                                                 mouseLeftNow && !_mouseLeftPressedLast,
                                                 mouseLeftNow, xpos * scaleX,
                                                 _camera.viewport[3] - ypos * scaleY);

            if (!gizmo && mouseLeftNow && !_mouseLeftPressedLast) {
                Object* pick = Tool::PickObject3D(_window, this, targets);
                if (pick) {
                    if (!_combineOperandA) {
                        _combineOperandA = pick;       // first pick = operand A
                        setSelectedObject3D(pick);
                    } else if (pick != _combineOperandA) {
                        fuseObjects(_combineOperandA, pick); // A op B -> A (B consumed)
                        setSelectedObject3D(_combineOperandA);
                        // _combineOperandA stays = the result, so ops chain naturally.
                    }
                }
            }
        }
        else if (_current3DMode == Mode3D::Sculpt) {
            // Clay: grab a shape and drag it (cursor unlocked, like Morph); release
            // while overlapping another shape to fuse them (target op dragged).
            std::vector<Object*> targets;
            for (const auto& up : mgr.active().world().getOwnedObjects())
                if (up && !(up->hasAttribute("baseline") &&
                            up->getAttribute("baseline") == std::string("ground")))
                    targets.push_back(up.get());

            const GLdouble* mv = _camera.modelview;
            const GLdouble* pr = _camera.projection;
            const int*      vp = _camera.viewport;
            const double winX = xpos * scaleX;
            const double winY = vp[3] - ypos * scaleY;

            // Refine the current result first: if the drag grabbed a gizmo of the
            // selected field, adjust that instead of grabbing a whole shape to move.
            const bool gizmo = handleFieldGizmos(_selectedObject3D,
                                                 mouseLeftNow && !_mouseLeftPressedLast,
                                                 mouseLeftNow, winX, winY);

            if (!gizmo && mouseLeftNow && !_mouseLeftPressedLast) {
                _clayGrabbed = Tool::PickObject3D(_window, this, targets);
                if (_clayGrabbed) setSelectedObject3D(_clayGrabbed);
            }

            if (mouseLeftNow && _clayGrabbed) {
                // Slide the grabbed shape in the screen plane at its current depth.
                glm::mat4 xf = _clayGrabbed->getTransform();
                glm::vec3 c = glm::vec3(xf[3]);
                GLdouble sx, sy, sz;
                if (ecgl::project(c.x, c.y, c.z, mv, pr, vp, &sx, &sy, &sz)) {
                    GLdouble nx, ny, nz;
                    if (ecgl::unProject(winX, winY, sz, mv, pr, vp, &nx, &ny, &nz)) {
                        xf[3] = glm::vec4((float)nx, (float)ny, (float)nz, 1.0f);
                        _clayGrabbed->setTransform(xf);
                    }
                }
                // Nearest other shape whose AABB overlaps = the fuse candidate.
                _clayTarget = nullptr;
                _clayGrabbed->updateCollisionZone(_clayGrabbed->getTransform());
                glm::vec3 gmn = _clayGrabbed->collisionZone.corners[0], gmx = gmn;
                for (int i = 1; i < 8; ++i) {
                    gmn = glm::min(gmn, _clayGrabbed->collisionZone.corners[i]);
                    gmx = glm::max(gmx, _clayGrabbed->collisionZone.corners[i]);
                }
                glm::vec3 gc = glm::vec3(_clayGrabbed->getTransform()[3]);
                float bestD = 1e18f;
                for (Object* o : targets) {
                    if (o == _clayGrabbed) continue;
                    o->updateCollisionZone(o->getTransform());
                    glm::vec3 mn = o->collisionZone.corners[0], mx = mn;
                    for (int i = 1; i < 8; ++i) {
                        mn = glm::min(mn, o->collisionZone.corners[i]);
                        mx = glm::max(mx, o->collisionZone.corners[i]);
                    }
                    bool overlap = (gmn.x <= mx.x && gmx.x >= mn.x) &&
                                   (gmn.y <= mx.y && gmx.y >= mn.y) &&
                                   (gmn.z <= mx.z && gmx.z >= mn.z);
                    if (overlap) {
                        float d = glm::length(glm::vec3(o->getTransform()[3]) - gc);
                        if (d < bestD) { bestD = d; _clayTarget = o; }
                    }
                }
            }

            if (!mouseLeftNow && _clayGrabbed) {
                if (_clayTarget && _clayTarget != _clayGrabbed) {
                    Object* target = _clayTarget;
                    fuseObjects(target, _clayGrabbed); // target op grabbed -> target
                    setSelectedObject3D(target);
                }
                _clayGrabbed = nullptr;
                _clayTarget = nullptr;
            }
        }

        _mouseLeftPressedLast = mouseLeftNow;
        } else {
            _mouseLeftPressedLast = false;
        }
    }

    // Update world (physics etc.)
    mgr.active().world().update(dt);
    mgr.active().applyFormationRelations();
    syncSelectedFormationRelations(mgr.active());
    // Sync highlight selection
    Rendering::HighlightSystem::setSelected(_selectedObject3D);
    Rendering::HighlightSystem::setSelectedIds(getSelectedObjectIds3D());

    // NOTE: player/world collision and ground contact are resolved entirely in
    // Person::stepMovement(). The old per-sample and per-bodypart camera shoves used to
    // live here; they fought gravity every frame and caused the ground/cube
    // jitter, so they were removed. Body parts are posed *from* the camera, not
    // the other way around.

    // Process menu hotkeys (must be after potential cursor unlock to allow selection)
    _mainMenu.processInput(_window);
    _mouseHandler.setMenuOpen(_mainMenu.isOpen());

    // Hover events were dormant: nothing ever called updateHoverState. Pick
    // the object under the cursor once per frame; the enter/exit EDGES
    // publish the hover events (and their ECA echoes) laws bind to.
    {
        std::vector<Object*> hoverTargets;
        for (const auto& obj : mgr.active().world().getOwnedObjects()) {
            if (obj) hoverTargets.push_back(obj.get());
        }
        
        // Use the generic pickSurface instead of PickObject3D so we also get the hit point/normal
        // for the Law system's cursorHitPos/cursorHitNormal.
        glm::vec3 rayO, rayD;
        if (buildMouseRay(_window, this, rayO, rayD)) {
            SurfaceHit hit;
            if (pickSurface(hoverTargets, rayO, rayD, hit)) {
                for (Object* obj : hoverTargets) {
                    obj->updateHoverState(obj == hit.obj);
                }
                _player.cursorHitPos = hit.point;
                _player.cursorHitNormal = hit.normal;
                _player.cursorHoveredBodyPart = hit.obj ? hit.obj->getIdentifier() : "";
            } else {
                for (Object* obj : hoverTargets) {
                    obj->updateHoverState(false);
                }
                // When looking at sky, hit point could be projected far out
                _player.cursorHitPos = rayO + rayD * 1000.0f;
                _player.cursorHitNormal = -rayD;
                _player.cursorHoveredBodyPart = "";
            }

            // Law System authors are now responsible for reading `cursorHitPos`/`cameraPos`
            // and mapping them to `cursorSpawnPos`, `cursorSpawnRot`, and `cursorSpawnScale`
            // based on dynamic `placementMode` strings!
        }
    }

    // Law System Perception properties on the Person
    _player.cameraPos = _camera.pos;
    _player.cameraForward = _camera.front;

    // --- Brush/placement state the removed ShapeGenerator3D tool consumed
    // directly. A law reads it off the Person instead, so all of it has to
    // reach the Person or the law spawns default cubes at a default pose.
    // placementMode in particular was never assigned, which left every law
    // stuck on "InFront" no matter what the UI said.
    switch (_placement.mode) {
        case BrushPlacementMode::ManualDistance: _player.placementMode = "ManualDistance"; break;
        case BrushPlacementMode::CursorSnap:     _player.placementMode = "CursorSnap";     break;
        case BrushPlacementMode::InFront:
        default:                                 _player.placementMode = "InFront";        break;
    }

    // Freeze the manual anchor on entering the mode, exactly as the tool did:
    // the offset is measured from where you were standing and looking, and
    // must not drift as you turn.
    if (_placement.mode == BrushPlacementMode::ManualDistance && !_placement.anchorValid) {
        _placement.anchorPos     = _camera.pos + _camera.front * 2.0f;
        _placement.anchorRight   = glm::normalize(glm::cross(_camera.front, _camera.up));
        _placement.anchorUp      = _camera.up;
        _placement.anchorForward = _camera.front;
        _placement.anchorValid   = true;
    }
    _player.manualOffset        = _placement.manualOffset;
    _player.manualAnchorValid   = _placement.anchorValid;
    _player.manualAnchorPos     = _placement.anchorPos;
    _player.manualAnchorRight   = _placement.anchorRight;
    _player.manualAnchorUp      = _placement.anchorUp;
    _player.manualAnchorForward = _placement.anchorForward;

    _player.gridSnap     = _brush.gridSnap;
    _player.gridSnapSize = _brush.gridSize;

    // The pose half of buildBrushCreateTransform(). Without these the law
    // spawned everything unrotated at scale 1 regardless of the brush.
    _player.cursorSpawnRot   = _brush.rotation;
    _player.cursorSpawnScale = _brush.scale * _brush.size;

    // The default mapping the comment above describes: cursorHitPos/cameraPos
    // -> cursorSpawnPos, per placementMode. It runs AFTER cameraPos and
    // cameraForward are refreshed, or "ManualDistance" would place against
    // last frame's camera. Laws that want their own placement overwrite
    // cursorSpawnPos after this point and win.
    _player.updatePlacement();
    _player.activeShapeKind = static_cast<int>(_polyhedron.shapeKind);
    _player.activeColor = glm::vec3(_currentColor[0], _currentColor[1], _currentColor[2]);
    _player.activeTool = _currentTool.getTypeName();
    
    switch (_current3DMode) {
        case Mode3D::BrushCreate: _player.active3DMode = "Create"; break;
        case Mode3D::Selection:   _player.active3DMode = "Select"; break;
        case Mode3D::FaceBrush:   _player.active3DMode = "Face Brush"; break;
        case Mode3D::FacePaint:   _player.active3DMode = "Face Fill"; break;
        case Mode3D::Pottery:     _player.active3DMode = "Pottery"; break;
        case Mode3D::Rotation:    _player.active3DMode = "Rotate"; break;
        case Mode3D::Morph:       _player.active3DMode = "Morph"; break;
        case Mode3D::Combine:     _player.active3DMode = "Combine"; break;
        case Mode3D::Sculpt:      _player.active3DMode = "Clay"; break;
        case Mode3D::Graph:       _player.active3DMode = "Graph"; break;
        case Mode3D::None:        _player.active3DMode = "None"; break;
        default:                  _player.active3DMode = "Unknown"; break;
    }

    // The world clock: Singularity owns time. Laws read it through the
    // reserved paths "time" / "time.delta" / "time.sinceApplied" — this is
    // what authored change-over-time (Map/Flow/Drive of t) stands on.
    _worldTime += static_cast<double>(dt);
    Universe::instance().setClock(_worldTime, static_cast<double>(dt));

    // First movers stay legible: every engine physics law has a bridge Law
    // in the register, so gravity is governable text, not hidden machinery.
    PhysicsLawBridge::syncRegister(_lawManager);
    Singularity::Physical::PhysicalChannel::syncRegister(_lawManager);


    // Laws hear the frame: everything published above (collisions, hover
    // edges, finished clips, applied laws) has been asserted as facts; one
    // tick evaluates the network and fires whatever the events woke.
    _lawManager.tick();
}

// ---------------------------------------------------------------------------
// stepMovement: Moved to Person::stepMovement(). The Person class now owns
// its movement integration logic.
// ---------------------------------------------------------------------------

} // namespace Core
