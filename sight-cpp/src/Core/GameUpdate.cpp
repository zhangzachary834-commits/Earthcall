// GameUpdate.cpp – Game::update() logic
// Split from Game.cpp during refactor.

#include "Game.hpp"
#include "Core/Engine.hpp"
#include "Form/Object/Object.hpp"
#include "Core/SdfBuild.hpp"
#include "OurVerse/Tool.hpp"
#include "OurVerse/AdvancedFacePaint.hpp"
#include "Rendering/BrushSystem.hpp"
#include "Rendering/HighlightSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Person/PersonEvents.hpp"
#include "Core/EventBus.hpp"

#include <GLFW/glfw3.h>
#include <OpenGL/glu.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

extern ZoneManager mgr;

using glm::vec3;

namespace Core {

void Game::update(float dt) {
    // Update input handlers
    _keyboardHandler.update();
    _keyboardHandler.updateGameInput(_window);
    _mouseHandler.update();

    // Update camera front from mouse handler
    _camera.front = _mouseHandler.calculateCameraFront();

    // Check if any text input is active (ImGui)
    bool anyTextInputActive = ImGui::IsAnyItemActive() || ImGui::IsWindowFocused();

    // ----------------------------------------------------------------------------
    // Player movement: a single authoritative resolve owns _camera.pos.
    // (Horizontal input + gravity/jump + ground contact, all in stepMovement.)
    // ----------------------------------------------------------------------------
    stepMovement(dt);

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

    // Update avatar system
    _avatarManager.updateAllAvatars(dt);

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

        bool useAvatarTargets = (_current3DTarget == ToolTarget3D::AvatarBodyParts);
        auto collect3DTargets = [&](std::vector<Object*>& targets) {
            targets.clear();
            if (useAvatarTargets) {
                for (auto* part : _player.getBody().parts) {
                    if (!part) continue;
                    targets.push_back(part);
                    for (const auto& sub : part->getSubObjects()) {
                        if (sub) targets.push_back(sub.get());
                    }
                }
                return;
            }
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
            BodyPart* shapePart = nullptr;
            if (useAvatarTargets) {
                shapePart = dynamic_cast<BodyPart*>(_selectedObject3D);
            }
            Tool::ShapeGenerator3D(_window, this, mgr, shapePart);
        } else if (_current3DMode == Mode3D::Pottery) {
            collect3DTargets(toolTargets);
            glm::mat4 avatarRoot = glm::translate(glm::mat4(1.0f), _player.position);
            Tool::Pottery3D(_window, this, mgr, dt, toolTargets,
                            useAvatarTargets ? &avatarRoot : nullptr);
        } else if (_current3DMode == Mode3D::Rotation) {
            collect3DTargets(toolTargets);
            glm::mat4 avatarRoot = glm::translate(glm::mat4(1.0f), _player.position);
            Tool::Rotate3D(_window, this, mgr, dt, toolTargets,
                           useAvatarTargets ? &avatarRoot : nullptr);
        } else if (_current3DMode == Mode3D::Selection) {
            collect3DTargets(toolTargets);
            // 3D Selection: set selected object on single click. Picking (incl.
            // the locked-cursor crosshair) is handled by Tool::Selection3D using
            // the cached camera matrices.
            if (mouseLeftNow && !_mouseLeftPressedLast) {
                Tool::Selection3D(_window, this, toolTargets);
            }
            if (_selectedObject3D) {
                ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
                ImGui::Begin("SelectionHUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
                ImGui::Text("Selected: %s", _selectedObject3D->getIdentifier().c_str());
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
                const GLint*    vp = _camera.viewport;
                glm::mat4 xf = obj->getTransform();
                double winX = xpos * scaleX;
                double winY = vp[3] - ypos * scaleY; // GL y-up

                // On press: pick the nearest vertex in screen space.
                if (mouseLeftNow && !_mouseLeftPressedLast) {
                    int best = -1; double bestD = 1e18;
                    for (int i = 0; i < obj->getPolyhedronVertexCount(); ++i) {
                        glm::vec3 w = glm::vec3(xf * glm::vec4(obj->getPolyhedronVertexLocal(i), 1.0f));
                        GLdouble sx, sy, sz;
                        if (gluProject(w.x, w.y, w.z, mv, pr, vp, &sx, &sy, &sz)) {
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
                    if (gluProject(wv.x, wv.y, wv.z, mv, pr, vp, &sx, &sy, &sz)) {
                        GLdouble nx, ny, nz;
                        if (gluUnProject(winX, winY, sz, mv, pr, vp, &nx, &ny, &nz)) {
                            glm::vec3 local = glm::vec3(glm::inverse(xf) *
                                glm::vec4((float)nx, (float)ny, (float)nz, 1.0f));
                            obj->setPolyhedronVertexLocal(_morphVertexIndex, local);
                        }
                    }
                }
            } else if (obj && obj->isBinaryField()) {
                // Embodied editing: drag operand B's holographic handle to move
                // the second shape; the blend/boolean result re-forms live.
                const GLdouble* mv = _camera.modelview;
                const GLdouble* pr = _camera.projection;
                const GLint*    vp = _camera.viewport;
                glm::mat4 xf = obj->getTransform();
                double winX = xpos * scaleX;
                double winY = vp[3] - ypos * scaleY;
                glm::vec3 hw = glm::vec3(xf * glm::vec4(obj->getFieldOperandBOffset(), 1.0f));
                GLdouble sx, sy, sz;
                bool projected = gluProject(hw.x, hw.y, hw.z, mv, pr, vp, &sx, &sy, &sz);
                if (mouseLeftNow && !_mouseLeftPressedLast && projected) {
                    double d = (sx - winX) * (sx - winX) + (sy - winY) * (sy - winY);
                    if (d < 45.0 * 45.0) _fieldHandleDragging = true;
                }
                if (!mouseLeftNow) _fieldHandleDragging = false;
                if (mouseLeftNow && _fieldHandleDragging && projected) {
                    GLdouble nx, ny, nz;
                    if (gluUnProject(winX, winY, sz, mv, pr, vp, &nx, &ny, &nz)) {
                        glm::vec3 local = glm::vec3(glm::inverse(xf) *
                            glm::vec4((float)nx, (float)ny, (float)nz, 1.0f));
                        obj->setFieldOperandBOffset(local);
                    }
                }
            } else if (obj && obj->isPatch() && obj->getPatchControlCount() > 0) {
                // Drag the control points of a Bezier surface (waterbending the
                // control net; the surface re-forms live).
                const GLdouble* mv = _camera.modelview;
                const GLdouble* pr = _camera.projection;
                const GLint*    vp = _camera.viewport;
                glm::mat4 xf = obj->getTransform();
                double winX = xpos * scaleX;
                double winY = vp[3] - ypos * scaleY;
                if (mouseLeftNow && !_mouseLeftPressedLast) {
                    int best = -1; double bestD = 1e18;
                    for (int i = 0; i < obj->getPatchControlCount(); ++i) {
                        glm::vec3 w = glm::vec3(xf * glm::vec4(obj->getPatchControlLocal(i), 1.0f));
                        GLdouble sx, sy, sz;
                        if (gluProject(w.x, w.y, w.z, mv, pr, vp, &sx, &sy, &sz)) {
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
                    if (gluProject(wv.x, wv.y, wv.z, mv, pr, vp, &sx, &sy, &sz)) {
                        GLdouble nx, ny, nz;
                        if (gluUnProject(winX, winY, sz, mv, pr, vp, &nx, &ny, &nz)) {
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

            if (mouseLeftNow && !_mouseLeftPressedLast) {
                Object* pick = Tool::PickObject3D(_window, this, targets);
                if (pick) {
                    if (!_combineOperandA) {
                        _combineOperandA = pick;       // first pick = operand A
                        _selectedObject3D = pick;
                    } else if (pick != _combineOperandA) {
                        Object* A = _combineOperandA;
                        Object* B = pick;
                        glm::mat4 Ta = A->getTransform();
                        glm::mat4 Tb = B->getTransform();
                        geom::SdfNode an = objectToSdfNode(*A);
                        geom::SdfNode bn = objectToSdfNode(*B);
                        // Put B's centre in A's local frame so the op happens where B sits.
                        bn.offset = glm::vec3(glm::inverse(Ta) * glm::vec4(glm::vec3(Tb[3]), 1.0f));
                        const geom::SdfOp ops[] = {
                            geom::SdfOp::Union, geom::SdfOp::Intersect, geom::SdfOp::Subtract,
                            geom::SdfOp::SmoothUnion, geom::SdfOp::Morph };
                        int oi = _combineOp; if (oi < 0 || oi > 4) oi = 2;
                        float blend = (oi >= 3) ? _combineBlend : 0.5f;
                        geom::SdfNode node = geom::SdfNode::binary(ops[oi], an, bn, blend);
                        A->setFieldShape(node, 1.6f);  // offset baked in -> Morph handle lands on B
                        // Consume B: it now lives inside A's field tree.
                        auto& owned = mgr.active().world().getOwnedObjectsMutable();
                        owned.erase(std::remove_if(owned.begin(), owned.end(),
                                    [B](const std::unique_ptr<Object>& p){ return p.get() == B; }),
                                    owned.end());
                        _selectedObject3D = A;
                        _combineOperandA = A;          // chain: result becomes the next A
                    }
                }
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
    // Sync highlight selection
    Rendering::HighlightSystem::setSelected(_selectedObject3D);

    // NOTE: player/world collision and ground contact are resolved entirely in
    // stepMovement(). The old per-sample and per-bodypart camera shoves used to
    // live here; they fought gravity every frame and caused the ground/cube
    // jitter, so they were removed. Body parts are posed *from* the camera, not
    // the other way around.

    // Process menu hotkeys (must be after potential cursor unlock to allow selection)
    _mainMenu.processInput(_window);
    _mouseHandler.setMenuOpen(_mainMenu.isOpen());
}

// ---------------------------------------------------------------------------
// stepMovement: the single authoritative player-movement resolve.
//
// Order each frame:
//   1. horizontal intent (WASD, pitch-flattened)  -> move on XZ
//   2. resolve object collisions HORIZONTALLY ONLY -> can't walk through walls
//   3. find the support height under the feet      -> floor or object top
//   4. vertical: fly input, or gravity + jump, clamped so feet never sink
//      below the support (this is what kills the gravity-vs-collision fight)
//   5. write _camera.pos once, then pose the body parts from it
//
// Vertical contact is owned solely here; collisions never push the player up.
// ---------------------------------------------------------------------------
void Game::stepMovement(float dt) {
    const bool anyTextInputActive = ImGui::IsAnyItemActive() || ImGui::IsWindowFocused();
    const bool canMove = _mouseHandler.isCursorLocked() && !_mainMenu.isOpen() && !anyTextInputActive;
    const bool flying  = Physics::getFlying();

    const auto& objects = mgr.active().world().getOwnedObjects();
    const float eyeH = _player.getBody().getEyeHeight();
    const glm::vec3 posBefore = _camera.pos;

    float actualSpeed = _camera.speed;
    if (glfwGetKey(_window, GLFW_KEY_V) == GLFW_PRESS) actualSpeed *= 2.5f;       // sprint
    if (glfwGetKey(_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) actualSpeed *= 0.3f; // slow

    // 1. Horizontal intent (ignores pitch so WASD behaves like Minecraft).
    if (canMove) {
        glm::vec3 forwardXZ = glm::normalize(glm::vec3(_camera.front.x, 0.0f, _camera.front.z));
        if (glm::length(forwardXZ) < 1e-3f) forwardXZ = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 rightXZ = glm::normalize(glm::cross(forwardXZ, _camera.up));

        glm::vec3 move(0.0f);
        if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS) move += forwardXZ;
        if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS) move -= forwardXZ;
        if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS) move += rightXZ;
        if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS) move -= rightXZ;
        if (glm::length(move) > 1e-4f) _camera.pos += glm::normalize(move) * actualSpeed;
    }

    // 2. Resolve object collisions horizontally only. We let enforceCollisions
    //    compute the push-out, then discard its vertical component so resting on
    //    a surface never shoves the player up (that was the jitter).
    {
        constexpr float RADIUS = 0.3f;
        glm::vec3 rightVec  = glm::normalize(glm::cross(_camera.front, _camera.up));
        glm::vec3 forwardXZ = glm::normalize(glm::vec3(_camera.front.x, 0.0f, _camera.front.z));
        if (glm::length(forwardXZ) < 1e-3f) forwardXZ = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 offsets[5] = { glm::vec3(0), rightVec*RADIUS, -rightVec*RADIUS,
                                 forwardXZ*RADIUS, -forwardXZ*RADIUS };
        for (const auto& off : offsets) {
            // sample at eye and feet so both head-height and leg-height walls block us
            for (float h : { 0.0f, eyeH }) {
                glm::vec3 sample = _camera.pos + off - glm::vec3(0.0f, h, 0.0f);
                glm::vec3 before = sample;
                Physics::enforceCollisions(sample, objects); // baseline ground already skipped
                glm::vec3 d = sample - before;
                d.y = 0.0f;                                   // horizontal push-out only
                _camera.pos += d;
            }
        }
    }

    // 3. Support height under the feet: global ground, raised to the top of any
    //    object the player is standing within the XZ footprint of.
    float supportY = 0.0f; // global floor; matches World ground plane
    {
        const float feetY = _camera.pos.y - eyeH;
        const float standTol = 0.05f; // top must be at/below the feet to be a floor
        for (const auto& up : objects) {
            if (!up) continue;
            up->updateCollisionZone(up->getTransform());
            glm::vec3 mn = up->collisionZone.corners[0], mx = mn;
            for (int i = 1; i < 8; ++i) {
                mn = glm::min(mn, up->collisionZone.corners[i]);
                mx = glm::max(mx, up->collisionZone.corners[i]);
            }
            if (_camera.pos.x < mn.x || _camera.pos.x > mx.x) continue;
            if (_camera.pos.z < mn.z || _camera.pos.z > mx.z) continue;
            if (mx.y <= feetY + standTol) supportY = std::max(supportY, mx.y);
        }
    }
    const float minEyeY = supportY + eyeH;

    // 4. Vertical resolve.
    const bool jumpKeyDown = canMove && glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (flying) {
        float vy = 0.0f;
        if (jumpKeyDown) vy += actualSpeed;
        if (canMove && glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) vy -= actualSpeed;
        _camera.pos.y += vy;
        _playerVelY = 0.0f;
        _playerGrounded = false;
    } else {
        constexpr float GRAVITY    = 9.81f;
        constexpr float JUMP_SPEED = 5.0f;
        if (jumpKeyDown && !_jumpKeyDownLast && _playerGrounded) {
            _playerVelY = JUMP_SPEED;   // jump impulse
            _playerGrounded = false;
        }
        _playerVelY -= GRAVITY * dt;     // integrate gravity
        _camera.pos.y += _playerVelY * dt;
    }
    _jumpKeyDownLast = jumpKeyDown;

    // Floor constraint: feet can never sink below the support surface.
    if (_camera.pos.y <= minEyeY) {
        _camera.pos.y = minEyeY;
        if (_playerVelY < 0.0f) _playerVelY = 0.0f;
        _playerGrounded = true;
    } else {
        _playerGrounded = !flying && (_camera.pos.y - minEyeY) <= 1e-3f;
    }

    // 5. Locomotion event + animation clocks, then a single pose from the camera.
    glm::vec3 horizDelta = _camera.pos - posBefore;
    horizDelta.y = 0.0f;
    const float distance = glm::length(horizDelta);
    const bool moving = distance > 1e-5f;
    const float speedPerSec = (moving && dt > 1e-5f) ? distance / dt : 0.0f;
    Core::EventBus::instance().publish(LocomotionChanged{&_player, moving, speedPerSec});
    _player.updateBodyAutomations(dt);

    _player.position = _camera.pos - glm::vec3(0.0f, eyeH, 0.0f);
    _player.updatePose();
}

} // namespace Core
