// GameUpdate.cpp – Game::update() logic
// Split from Game.cpp during refactor.

#include "Game.hpp"
#include "Core/Engine.hpp"
#include "Form/Object/Object.hpp"
#include "OurVerse/Tool.hpp"
#include "OurVerse/AdvancedFacePaint.hpp"
#include "Rendering/BrushSystem.hpp"
#include "Rendering/HighlightSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"

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
    _cameraFront = _mouseHandler.calculateCameraFront();

    // Check if any text input is active (ImGui)
    bool anyTextInputActive = ImGui::IsAnyItemActive() || ImGui::IsWindowFocused();

    // ----------------------------------------------------------------------------
    // Camera movement WASD + SHIFT/SPACE (continuous movement)
    // ----------------------------------------------------------------------------
    float actualSpeed = _cameraSpeed;
    if (glfwGetKey(_window, GLFW_KEY_V) == GLFW_PRESS) actualSpeed *= 2.5f; // sprint
    if (glfwGetKey(_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) actualSpeed *= 0.3f; // slow

    if (_mouseHandler.isCursorLocked() && !_mainMenu.isOpen() && !anyTextInputActive) {
        // Calculate movement vectors that ignore camera pitch so WASD behaves like Minecraft
        glm::vec3 forwardXZ = glm::normalize(glm::vec3(_cameraFront.x, 0.0f, _cameraFront.z));
        if (glm::length(forwardXZ) < 1e-3f) forwardXZ = glm::vec3(0.0f, 0.0f, -1.0f); // fallback
        glm::vec3 rightXZ   = glm::normalize(glm::cross(forwardXZ, _cameraUp));

        if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS) _cameraPos += actualSpeed * forwardXZ;
        if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS) _cameraPos -= actualSpeed * forwardXZ;
        if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS) _cameraPos -= rightXZ * actualSpeed;
        if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS) _cameraPos += rightXZ * actualSpeed;
        if (glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) _cameraPos -= actualSpeed * _cameraUp;
        if (glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS) _cameraPos += actualSpeed * _cameraUp;

        // Reset anchor if mode switched out of ManualDistance
        if(_placementMode != BrushPlacementMode::ManualDistance){ _manualAnchorValid = false; }

        // Manual offset tweak with keys when using ManualDistance - only when not typing
        if (_placementMode == BrushPlacementMode::ManualDistance && _current3DMode == Mode3D::BrushCreate && !anyTextInputActive) {
            float step = 0.1f;
            if (glfwGetKey(_window, GLFW_KEY_RIGHT) == GLFW_PRESS) _manualOffset.x += step;
            if (glfwGetKey(_window, GLFW_KEY_LEFT)  == GLFW_PRESS) _manualOffset.x -= step;
            if (glfwGetKey(_window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) _manualOffset.y += step;
            if (glfwGetKey(_window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) _manualOffset.y -= step;
            if (glfwGetKey(_window, GLFW_KEY_UP)   == GLFW_PRESS) _manualOffset.z += step;
            if (glfwGetKey(_window, GLFW_KEY_DOWN) == GLFW_PRESS) _manualOffset.z -= step;
        }
    }

    // Sync player anchor with camera position
    glm::vec3 anchor = _cameraPos - glm::vec3(0.0f, _player.getBody().getEyeHeight(), 0.0f);
    _player.position = anchor;
    _player.updatePose();

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

            Tool::use(_window, mgr, zone, currentToolType, *this);
            if (currentToolType == Tool::Type::Brush) {
                // Check for Shift key to enable straight line mode
                bool shiftPressed = glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                                   glfwGetKey(_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

                // Straight line mode (either from button or Shift+click)
                if (_straightLineMode || shiftPressed) {
                    if (mouseLeftNow && !_mouseLeftPressedLast) {
                        // Start straight line
                        _drawingStraightLine = true;
                        _straightLineStartX = mx;
                        _straightLineStartY = my;
                        zone.startStroke(mx, my);
                    } else if (_drawingStraightLine) {
                        // Update straight line preview on mouse move
                        zone.endStroke();
                        zone.startStroke(_straightLineStartX, _straightLineStartY);
                        zone.continueStroke(mx, my);
                    } else if (!mouseLeftNow && _mouseLeftPressedLast && _drawingStraightLine) {
                        // End straight line
                        zone.endStroke();
                        _drawingStraightLine = false;
                        if (shiftPressed) {
                            _straightLineMode = false;
                        }
                    }
                } else {
                    // Ensure design system is initialized
                    if (!zone.getDesignSystem()) {
                        zone.initializeDesignSystem();
                    }

                    // Debug output to see what tool is selected
                    static Tool::Type lastToolType = Tool::Type::Brush;
                    if (currentToolType != lastToolType) {
                        printf("Tool changed to: %s (%s)\n", _currentTool.getTypeName().c_str(), _currentTool.getIcon().c_str());
                        lastToolType = currentToolType;
                    }

                    // Drawing tools (including Line)
                    if (currentToolType == Tool::Type::Brush) {
                        Tool::use(_window, mgr, zone, currentToolType, *this);
                    }

                    // Utility tools
                    else if (currentToolType == Tool::Type::ColorPicker ||
                             currentToolType == Tool::Type::Eyedropper ||
                             currentToolType == Tool::Type::Hand ||
                             currentToolType == Tool::Type::Zoom ||
                             currentToolType == Tool::Type::Crop ||
                             currentToolType == Tool::Type::Slice) {

                        if (mouseLeftNow && !_mouseLeftPressedLast) {
                            switch (currentToolType) {
                                case Tool::Type::ColorPicker:
                                case Tool::Type::Eyedropper: {
                                    float r = static_cast<float>(rand()) / RAND_MAX;
                                    float g = static_cast<float>(rand()) / RAND_MAX;
                                    float b = static_cast<float>(rand()) / RAND_MAX;
                                    zone.setDrawColor(r, g, b);
                                    break;
                                }

                                case Tool::Type::Hand:
                                    printf("Hand tool: Pan view at (%.1f, %.1f)\n", mx, my);
                                    break;

                                case Tool::Type::Zoom:
                                    printf("Zoom tool: Zoom at (%.1f, %.1f)\n", mx, my);
                                    break;

                                case Tool::Type::Crop:
                                    printf("Crop tool: Start crop at (%.1f, %.1f)\n", mx, my);
                                    break;

                                case Tool::Type::Slice:
                                    printf("Slice tool: Start slice at (%.1f, %.1f)\n", mx, my);
                                    break;

                                default:
                                    break;
                            }
                        }
                    }
                    // Legacy fallback for compatibility
                    else {
                        if (_useAdvanced2DBrush) {
                            if (mouseLeftNow && !_mouseLeftPressedLast) {
                                zone.startStroke(mx, my);
                            } else if (mouseLeftNow && _mouseLeftPressedLast) {
                                zone.continueStroke(mx, my);
                            } else if (!mouseLeftNow && _mouseLeftPressedLast) {
                                zone.endStroke();
                            }
                        } else {
                            if (mouseLeftNow && !_mouseLeftPressedLast) {
                                mgr.active().startStroke(mx, my);
                            } else if (mouseLeftNow && _mouseLeftPressedLast) {
                                mgr.active().continueStroke(mx, my);
                            } else if (!mouseLeftNow && _mouseLeftPressedLast) {
                                mgr.active().endStroke();
                            }
                        }
                    }
                }
            } else if (_currentTool.getType() == Tool::Type::Eraser) {
                Tool::use(_window, mgr, zone, currentToolType, *this);

            } else if (_currentTool.getType() == Tool::Type::Rectangle) {
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
            // 3D Selection: set selected object on single click
            if (mouseLeftNow && !_mouseLeftPressedLast) {
                glGetIntegerv(GL_VIEWPORT, _cameraViewport);
                glGetDoublev(GL_MODELVIEW_MATRIX, _cameraModelview);
                glGetDoublev(GL_PROJECTION_MATRIX, _cameraProjection);
                double winX = xpos * scaleX; double winY = ypos * scaleY;
                winY = _cameraViewport[3] - winY;
                GLdouble nearX,nearY,nearZ,farX,farY,farZ;
                gluUnProject(winX, winY, 0.0, _cameraModelview, _cameraProjection, _cameraViewport, &nearX,&nearY,&nearZ);
                gluUnProject(winX, winY, 1.0, _cameraModelview, _cameraProjection, _cameraViewport, &farX,&farY,&farZ);
                glm::vec3 rayO(nearX,nearY,nearZ); glm::vec3 rayDir = glm::normalize(glm::vec3(farX,farY,farZ)-rayO);
                float nearestT = 1e9f; Object* hitObj=nullptr;
                const auto& objects = toolTargets;
                for (auto* obj : objects) {
                    if (!obj) continue;
                    float t; int face; glm::vec2 uv;
                    if (obj->raycastFace(rayO, rayDir, t, face, uv)) {
                        if (t > 0.0f && t < nearestT) { nearestT = t; hitObj = obj; }
                    }
                }
                _selectedObject3D = hitObj;
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

    // Extra collision samples around the player (simple capsule approximation)
    {
        constexpr float EYE_TO_FEET = 0.9f;
        constexpr float RADIUS = 0.3f;

        glm::vec3 rightVec = glm::normalize(glm::cross(_cameraFront, _cameraUp));
        glm::vec3 forwardXZ = glm::normalize(glm::vec3(_cameraFront.x, 0.0f, _cameraFront.z));
        if (glm::length(forwardXZ) < 1e-3f) forwardXZ = glm::vec3(0.0f,0.0f,1.0f);

        glm::vec3 offsets[5] = {
            glm::vec3(0),
            rightVec * RADIUS,
            -rightVec * RADIUS,
            forwardXZ * RADIUS,
            -forwardXZ * RADIUS
        };

        for (const auto& off : offsets) {
            glm::vec3 sampleEye  = _cameraPos + off;
            glm::vec3 sampleFeet = sampleEye  - glm::vec3(0.0f, EYE_TO_FEET, 0.0f);
            Physics::enforceCollisions(sampleEye,  mgr.active().world().getOwnedObjects());
            Physics::enforceCollisions(sampleFeet, mgr.active().world().getOwnedObjects());
            glm::vec3 resolvedCenter = sampleEye - off;
            _cameraPos = resolvedCenter;
        }
    }

    // Update body part world transforms based on (possibly corrected) player position
    _player.updatePose();

    // Per-bodypart collision refinement – single aggregate delta
    glm::vec3 totalDelta(0.0f);
    for (auto* part : _player.getBody().parts) {
        if (!part) continue;
        glm::vec3 pos = glm::vec3(part->getTransform()[3]);
        glm::vec3 corrected = pos;
        Physics::enforceCollisions(corrected, mgr.active().world().getOwnedObjects());
        totalDelta += (corrected - pos);
    }
    if (glm::length(totalDelta) > 1e-4f) {
        _cameraPos += totalDelta;
        _player.position += totalDelta;
        _player.updatePose();
    }

    // Final sync so avatar anchors exactly to camera for next frame
    _player.position = _cameraPos - glm::vec3(0.0f, _player.getBody().getEyeHeight(), 0.0f);
    _player.updatePose();

    // Process menu hotkeys (must be after potential cursor unlock to allow selection)
    _mainMenu.processInput(_window);
    _mouseHandler.setMenuOpen(_mainMenu.isOpen());
}

} // namespace Core
