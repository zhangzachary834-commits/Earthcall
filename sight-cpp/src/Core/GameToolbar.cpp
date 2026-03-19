// GameToolbar.cpp – Game::renderCreatorToolbar() and all its sub-windows
// Split from Game.cpp during refactor. This was the biggest chunk (~1400 lines).

#include "Game.hpp"
#include "Core/Engine.hpp"
#include "Form/Object/Object.hpp"
#include "Form/Object/AngleTools.hpp"
#include "Form/Object/Contour.hpp"
#include "Rendering/BrushSystem.hpp"
#include "Rendering/DesignSystem.hpp"
#include "Rendering/RelationManagerWindow.hpp"
#include "OurVerse/Tool.hpp"
#include "OurVerse/AdvancedFacePaint.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

extern ZoneManager mgr;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Core {

void Game::renderCreatorToolbar() {
    // ------------------------------------------------------------------
    // Small host window containing a menu to toggle individual tool panes
    // ------------------------------------------------------------------

    static bool showPaint  = true;
    static bool show3D     = true;
    static bool showWorld  = true;
    static bool showAssets = true;
    static bool showBonds  = true;
    static bool showRelations = true;
    static bool showCursor = true;

    ImGui::SetNextWindowSize(ImVec2(550, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin(u8"\xF0\x9F\x9B\xA0 Earthcall Creator", nullptr,
                 ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Windows")) {
            ImGui::MenuItem("Paint",  nullptr, &showPaint);
            ImGui::MenuItem("3D",     nullptr, &show3D);
            ImGui::MenuItem("World",  nullptr, &showWorld);
            ImGui::MenuItem("Assets", nullptr, &showAssets);
            ImGui::MenuItem("Bonds",  nullptr, &showBonds);
            ImGui::MenuItem("Relations", nullptr, &showRelations);
            ImGui::MenuItem("Cursor Tools",  nullptr, &showCursor);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

#ifdef ImGuiConfigFlags_DockingEnable
    static ImGuiID dockspace_id = 0;
    if (dockspace_id == 0) dockspace_id = ImGui::GetID("CreatorDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
#endif

    ImGui::End(); // End host window

    // ------------------------------------------------------------------
    // Paint window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (showPaint) {
        if (ImGui::Begin(u8"\xF0\x9F\x8E\xA8 Professional 2D Design", &showPaint)) {
            Zone& zone = mgr.active();

            // Ensure design system is initialized
            if (!zone.getDesignSystem()) {
                zone.initializeDesignSystem();
            }

            // Tool Categories
            if (ImGui::BeginTabBar("DesignTools")) {

                // Drawing Tools Tab
                if (ImGui::BeginTabItem(u8"\xF0\x9F\x96\x8C Drawing")) {
                    ImGui::BeginGroup();

                    if (ImGui::Button(u8"\xF0\x9F\x96\x8C Brush")) {
                        _currentTool = Tool(Tool::Type::Brush);
                        zone.setDesignTool(Tool::Type::Brush);
                        _current3DMode = Mode3D::None;
                    }
            ImGui::SameLine();
                    if (ImGui::Button(u8"\xE2\x9C\x8F\xEF\xB8\x8F Pencil")) {
                        _currentTool = Tool(Tool::Type::Pencil);
                        zone.setDesignTool(Tool::Type::Pencil);
                        _current3DMode = Mode3D::None;
                    }
            ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x96\x8A Pen")) {
                        _currentTool = Tool(Tool::Type::Pen);
                        zone.setDesignTool(Tool::Type::Pen);
                        _current3DMode = Mode3D::None;
                    }

                    if (ImGui::Button(u8"\xF0\x9F\x92\xA8 Airbrush")) {
                        _currentTool = Tool(Tool::Type::Airbrush);
                        zone.setDesignTool(Tool::Type::Airbrush);
                        _current3DMode = Mode3D::None;
                    }
            ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x96\xBC Chalk")) {
                        _currentTool = Tool(Tool::Type::Chalk);
                        zone.setDesignTool(Tool::Type::Chalk);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x8E\xA8 Spray")) {
                        _currentTool = Tool(Tool::Type::Spray);
                        zone.setDesignTool(Tool::Type::Spray);
                        _current3DMode = Mode3D::None;
                    }

                    if (ImGui::Button(u8"\xF0\x9F\x91\x86 Smudge")) {
                        _currentTool = Tool(Tool::Type::Smudge);
                        zone.setDesignTool(Tool::Type::Smudge);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x93\x8B Clone")) {
                        _currentTool = Tool(Tool::Type::Clone);
                        zone.setDesignTool(Tool::Type::Clone);
                        _current3DMode = Mode3D::None;
                    }

                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }

                // Erasing Tools Tab
                if (ImGui::BeginTabItem(u8"\xF0\x9F\xA7\xBD Erasing")) {
                    ImGui::BeginGroup();

                    if (ImGui::Button(u8"\xF0\x9F\xA7\xBD Eraser")) {
                        _currentTool = Tool(Tool::Type::Eraser);
                        zone.setDesignTool(Tool::Type::Eraser);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xE2\x9C\xA8 Magic Eraser")) {
                        _currentTool = Tool(Tool::Type::MagicEraser);
                        zone.setDesignTool(Tool::Type::MagicEraser);
                        _current3DMode = Mode3D::None;
                    }

                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }

                // Selection Tools Tab
                if (ImGui::BeginTabItem(u8"\xE2\xAC\x9C Selection")) {
                    ImGui::BeginGroup();

                    if (ImGui::Button(u8"\xE2\xAC\x9C Selection")) {
                        _currentTool = Tool(Tool::Type::Selection);
                        zone.setDesignTool(Tool::Type::Selection);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x94\x97 Lasso")) {
                        _currentTool = Tool(Tool::Type::Lasso);
                        zone.setDesignTool(Tool::Type::Lasso);
                        _current3DMode = Mode3D::None;
                    }

                    if (ImGui::Button(u8"\xF0\x9F\xAA\x84 Magic Wand")) {
                        _currentTool = Tool(Tool::Type::MagicWand);
                        zone.setDesignTool(Tool::Type::MagicWand);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x93\xA6 Marquee")) {
                        _currentTool = Tool(Tool::Type::Marquee);
                        zone.setDesignTool(Tool::Type::Marquee);
                        _current3DMode = Mode3D::None;
                    }

                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }

                // Shape Tools Tab
                if (ImGui::BeginTabItem(u8"\xF0\x9F\x94\xB7 Shapes")) {
                    ImGui::BeginGroup();

                    if (ImGui::Button(u8"\xE2\xAC\x9C Rectangle")) {
                        _currentTool = Tool(Tool::Type::Rectangle);
                        zone.setDesignTool(Tool::Type::Rectangle);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xE2\xAD\x95 Ellipse")) {
                        _currentTool = Tool(Tool::Type::Ellipse);
                        zone.setDesignTool(Tool::Type::Ellipse);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x94\xB7 Polygon")) {
                        _currentTool = Tool(Tool::Type::Polygon);
                        zone.setDesignTool(Tool::Type::Polygon);
                        _current3DMode = Mode3D::None;
                    }

                    if (ImGui::Button(u8"\xE2\x9E\x96 Line")) {
                        _currentTool = Tool(Tool::Type::Line);
                        zone.setDesignTool(Tool::Type::Line);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xE2\x9E\xA1\xEF\xB8\x8F Arrow")) {
                        _currentTool = Tool(Tool::Type::Arrow);
                        zone.setDesignTool(Tool::Type::Arrow);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xE2\xAD\x90 Star")) {
                        _currentTool = Tool(Tool::Type::Star);
                        zone.setDesignTool(Tool::Type::Star);
                        _current3DMode = Mode3D::None;
                    }

                    if (ImGui::Button(u8"\xE2\x9D\xA4\xEF\xB8\x8F Heart")) {
                        _currentTool = Tool(Tool::Type::Heart);
                        zone.setDesignTool(Tool::Type::Heart);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x94\xB6 Custom")) {
                        _currentTool = Tool(Tool::Type::CustomShape);
                        zone.setDesignTool(Tool::Type::CustomShape);
                        _current3DMode = Mode3D::None;
                    }

                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }

                // Text Tools Tab
                if (ImGui::BeginTabItem("T Text")) {
                    ImGui::BeginGroup();

                    if (ImGui::Button(u8"T Text")) {
                        _currentTool = Tool(Tool::Type::Text);
                        zone.setDesignTool(Tool::Type::Text);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"T\xE2\x86\x95\xEF\xB8\x8F Vertical")) {
                        _currentTool = Tool(Tool::Type::TextVertical);
                        zone.setDesignTool(Tool::Type::TextVertical);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"T\xE3\x80\xB0\xEF\xB8\x8F Path")) {
                        _currentTool = Tool(Tool::Type::TextPath);
                        zone.setDesignTool(Tool::Type::TextPath);
                        _current3DMode = Mode3D::None;
                    }

                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }

                // Transform Tools Tab
                if (ImGui::BeginTabItem(u8"\xF0\x9F\x94\x84 Transform")) {
                    ImGui::BeginGroup();

                    if (ImGui::Button(u8"\xE2\x9C\x8B Move")) {
                        _currentTool = Tool(Tool::Type::Move);
                        zone.setDesignTool(Tool::Type::Move);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x94\x8D Scale")) {
                        _currentTool = Tool(Tool::Type::Scale);
                        zone.setDesignTool(Tool::Type::Scale);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x94\x84 Rotate")) {
                        _currentTool = Tool(Tool::Type::Rotate);
                        zone.setDesignTool(Tool::Type::Rotate);
                        _current3DMode = Mode3D::None;
                    }

                    if (ImGui::Button(u8"\xF0\x9F\x93\x90 Skew")) {
                        _currentTool = Tool(Tool::Type::Skew);
                        zone.setDesignTool(Tool::Type::Skew);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x94\x80 Distort")) {
                        _currentTool = Tool(Tool::Type::Distort);
                        zone.setDesignTool(Tool::Type::Distort);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x8F\x97\xEF\xB8\x8F Perspective")) {
                        _currentTool = Tool(Tool::Type::Perspective);
                        zone.setDesignTool(Tool::Type::Perspective);
                        _current3DMode = Mode3D::None;
                    }

                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }

                // Effects Tools Tab
                if (ImGui::BeginTabItem(u8"\xF0\x9F\x8E\xA8 Effects")) {
                    ImGui::BeginGroup();

                    if (ImGui::Button(u8"\xF0\x9F\x8C\xAB\xEF\xB8\x8F Blur")) {
                        _currentTool = Tool(Tool::Type::Blur);
                        zone.setDesignTool(Tool::Type::Blur);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x94\xAA Sharpen")) {
                        _currentTool = Tool(Tool::Type::Sharpen);
                        zone.setDesignTool(Tool::Type::Sharpen);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x93\xBB Noise")) {
                        _currentTool = Tool(Tool::Type::Noise);
                        zone.setDesignTool(Tool::Type::Noise);
                        _current3DMode = Mode3D::None;
                    }

                    if (ImGui::Button(u8"\xF0\x9F\x8F\x9B\xEF\xB8\x8F Emboss")) {
                        _currentTool = Tool(Tool::Type::Emboss);
                        zone.setDesignTool(Tool::Type::Emboss);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x92\xA1 Glow")) {
                        _currentTool = Tool(Tool::Type::Glow);
                        zone.setDesignTool(Tool::Type::Glow);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x91\xA4 Shadow")) {
                        _currentTool = Tool(Tool::Type::Shadow);
                        zone.setDesignTool(Tool::Type::Shadow);
                        _current3DMode = Mode3D::None;
                    }

                    if (ImGui::Button(u8"\xF0\x9F\x8C\x88 Gradient")) {
                        _currentTool = Tool(Tool::Type::Gradient);
                        zone.setDesignTool(Tool::Type::Gradient);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x94\xB2 Pattern")) {
                        _currentTool = Tool(Tool::Type::Pattern);
                        zone.setDesignTool(Tool::Type::Pattern);
                        _current3DMode = Mode3D::None;
                    }

                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }

                // Utility Tools Tab
                if (ImGui::BeginTabItem(u8"\xF0\x9F\x94\xA7 Utility")) {
                    ImGui::BeginGroup();

                    if (ImGui::Button(u8"\xF0\x9F\x8E\xAF Color Picker")) {
                        _currentTool = Tool(Tool::Type::ColorPicker);
                        zone.setDesignTool(Tool::Type::ColorPicker);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x92\x89 Eyedropper")) {
                        _currentTool = Tool(Tool::Type::Eyedropper);
                        zone.setDesignTool(Tool::Type::Eyedropper);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xE2\x9C\x8B Hand")) {
                        _currentTool = Tool(Tool::Type::Hand);
                        zone.setDesignTool(Tool::Type::Hand);
                        _current3DMode = Mode3D::None;
                    }

                    if (ImGui::Button(u8"\xF0\x9F\x94\x8D Zoom")) {
                        _currentTool = Tool(Tool::Type::Zoom);
                        zone.setDesignTool(Tool::Type::Zoom);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xE2\x9C\x82\xEF\xB8\x8F Crop")) {
                        _currentTool = Tool(Tool::Type::Crop);
                        zone.setDesignTool(Tool::Type::Crop);
                        _current3DMode = Mode3D::None;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x94\xAA Slice")) {
                        _currentTool = Tool(Tool::Type::Slice);
                        zone.setDesignTool(Tool::Type::Slice);
                        _current3DMode = Mode3D::None;
                    }

                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::Separator();

            // Color and Properties Panel
            ImGui::BeginGroup();
            ImGui::Text("Color & Properties:");
            ImGui::SameLine();
            if (ImGui::ColorEdit3("##MainColor", _currentColor, ImGuiColorEditFlags_NoInputs)) {
                zone.setDrawColor(_currentColor[0], _currentColor[1], _currentColor[2]);
            }

            // Layer Management
            ImGui::Separator();
            ImGui::Text("Layer Management:");
            if (ImGui::Button("Add Layer")) {
                zone.addDesignLayer();
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Layer")) {
                zone.removeDesignLayer(0);
            }

            // Legacy compatibility
            ImGui::Separator();
            ImGui::Checkbox("Use Advanced 2D Brush", &_useAdvanced2DBrush);
            if (_useAdvanced2DBrush) {
                ImGui::SameLine();
                if (ImGui::Button("Advanced Settings")) {
                    _show2DBrushPanel = !_show2DBrushPanel;
                }
            }

            // Show current tool status
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Current Tool: %s", Tool(_currentTool).getTypeName().c_str());

            ImGui::EndGroup();
        }
        ImGui::End();
    }

    // ------------------------------------------------------------------
    // 2D Advanced Brush Panel
    // ------------------------------------------------------------------
    if (_show2DBrushPanel && _useAdvanced2DBrush) {
        if (ImGui::Begin("Advanced 2D Brush", &_show2DBrushPanel)) {
            Zone& zone = mgr.active();

            if (!zone.getBrushSystem()) {
                zone.initializeBrushSystem();
            }

            BrushSystem* brushSystem = zone.getBrushSystem();

            if (brushSystem) {
                const char* brushTypes[] = {"Normal", "Airbrush", "Chalk", "Spray", "Smudge", "Clone"};
                int currentType = static_cast<int>(brushSystem->getBrushType());
                if (ImGui::Combo("Brush Type", &currentType, brushTypes, 6)) {
                    zone.setBrushType(static_cast<BrushSystem::BrushType>(currentType));
                }

                ImGui::Text("Brush System Status: Active");
                ImGui::Text("Active Layer: %d", brushSystem->getActiveLayer());
                ImGui::Text("Layer Count: %d", brushSystem->getLayerCount());

                ImGui::Separator();
                ImGui::Text("Basic Settings:");
                float radius = brushSystem->getRadius();
                if (ImGui::SliderFloat("Radius", &radius, 0.01f, 2.0f, "%.3f")) {
                    zone.setBrushRadius(radius);
                }

                float opacity = brushSystem->getOpacity();
                if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 3.0f, "%.2f")) {
                    zone.setBrushOpacity(opacity);
                }

                float flow = brushSystem->getFlow();
                if (ImGui::SliderFloat("Flow", &flow, 0.0f, 3.0f, "%.2f")) {
                    zone.setBrushFlow(flow);
                }

                ImGui::Separator();
                ImGui::Text("Advanced Dynamics:");
                float spacing = brushSystem->getSpacing();
                if (ImGui::SliderFloat("Spacing", &spacing, 0.01f, 2.0f, "%.3f")) {
                    zone.setBrushSpacing(spacing);
                }

                float density = brushSystem->getDensity();
                if (ImGui::SliderFloat("Density", &density, 0.1f, 5.0f, "%.2f")) {
                    zone.setBrushDensity(density);
                }

                float strength = brushSystem->getStrength();
                if (ImGui::SliderFloat("Strength", &strength, 0.0f, 5.0f, "%.2f")) {
                    zone.setBrushStrength(strength);
                }

                ImGui::Separator();
                ImGui::Text("Pressure Simulation:");
                bool usePressure = brushSystem->getUseLayers();
                if (ImGui::Checkbox("Enable Pressure", &usePressure)) {
                    zone.setPressureSimulation(usePressure);
                }

                ImGui::Separator();
                ImGui::Text("Stroke Settings:");
                bool useInterpolation = true;
                if (ImGui::Checkbox("Stroke Interpolation", &useInterpolation)) {
                    zone.setStrokeInterpolation(useInterpolation);
                }

                ImGui::Separator();
                ImGui::Text("Layer System:");
                bool useLayers = brushSystem->getUseLayers();
                if (ImGui::Checkbox("Use Layers", &useLayers)) {
                    zone.setUseLayers(useLayers);
                }

                if (useLayers) {
                    int layerCount = brushSystem->getLayerCount();
                    ImGui::Text("Layers: %d", layerCount);

                    if (ImGui::Button("Add Layer")) {
                        zone.addLayer();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Delete Layer")) {
                        zone.deleteLayer(brushSystem->getActiveLayer());
                    }

                    int activeLayer = brushSystem->getActiveLayer();
                    if (ImGui::SliderInt("Active Layer", &activeLayer, 0, std::max(0, layerCount - 1))) {
                        zone.setActiveLayer(activeLayer);
                    }
                }

                if (currentType == 5) { // Clone
                    ImGui::Separator();
                    ImGui::Text("Clone Tool:");
                    bool cloneActive = brushSystem->getCloneActive();
                    if (ImGui::Checkbox("Clone Active", &cloneActive)) {
                        zone.setCloneActive(cloneActive);
                    }

                    if (cloneActive) {
                        static glm::vec2 cloneOffset(0.0f, 0.0f);
                        if (ImGui::SliderFloat2("Clone Offset", &cloneOffset.x, -1.0f, 1.0f, "%.2f")) {
                            zone.setCloneOffset(cloneOffset);
                        }
                    }
                }

                ImGui::Separator();
                ImGui::Text("History:");
                if (ImGui::Button("Undo (Ctrl+Z)")) {
                    zone.undo();
                }
                ImGui::SameLine();
                if (ImGui::Button("Redo (Ctrl+Y)")) {
                    zone.redo();
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear History")) {
                    zone.clearHistory();
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Brush System failed to initialize!");
                if (ImGui::Button("Retry Initialization")) {
                    zone.initializeBrushSystem();
                }
            }
        }
        ImGui::End();
    }

    // ------------------------------------------------------------------
    // 3D window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (show3D) {
        if (ImGui::Begin(u8"\xF0\x9F\x94\xB3 3D", &show3D)) {
            int modeIdx = static_cast<int>(_current3DMode);
            const char* modeNames[] = {"Face Fill", "Face Brush", "Shape Generator", "Pottery", "Selection"};
            if (ImGui::Combo("SubMode", &modeIdx, modeNames, IM_ARRAYSIZE(modeNames))) {
                _current3DMode = static_cast<Mode3D>(modeIdx);
            }

            ImGui::Separator();
            int targetIdx = static_cast<int>(_current3DTarget);
            const char* targetNames[] = {"World Objects", "Avatar Body Parts"};
            if (ImGui::Combo("Target", &targetIdx, targetNames, IM_ARRAYSIZE(targetNames))) {
                _current3DTarget = static_cast<ToolTarget3D>(targetIdx);
                _selectedObject3D = nullptr;
            }
            if (_current3DTarget == ToolTarget3D::AvatarBodyParts && _current3DMode == Mode3D::BrushCreate) {
                if (_selectedObject3D && dynamic_cast<BodyPart*>(_selectedObject3D)) {
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                                       "Adding sub-objects to: %s", _selectedObject3D->getIdentifier().c_str());
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f),
                                       "Select a body part first (use Selection tool).");
                }
            }

            // Advanced Face Paint Options (only in Face Fill mode)
            if (_current3DMode == Mode3D::FacePaint) {
                ImGui::Separator();
                ImGui::TextUnformatted(u8"\xF0\x9F\x8E\xA8 Advanced Face Paint Options");

                if (ImGui::Checkbox("Enable Advanced Face Paint", &_useAdvancedFacePaint)) {
                    if (_useAdvancedFacePaint) {
                        AdvancedFacePaint::initializeAdvancedPainter();
                    }
                }

                if (_useAdvancedFacePaint) {
                    ImGui::Indent();

                    // Gradient Options
                    if (ImGui::CollapsingHeader("Gradient Options", ImGuiTreeNodeFlags_DefaultOpen)) {
                        AdvancedFacePaint::GradientSettings& gradSettings = _currentGradientSettings;

                        const char* gradientTypes[] = {"Linear", "Radial", "Angular", "Diamond", "Noise", "Custom"};
                        int gradTypeIdx = static_cast<int>(gradSettings.type);
                        if (ImGui::Combo("Gradient Type", &gradTypeIdx, gradientTypes, IM_ARRAYSIZE(gradientTypes))) {
                            gradSettings.type = static_cast<AdvancedFacePaint::GradientType>(gradTypeIdx);
                        }

                        ImGui::ColorEdit4("Start Color", &gradSettings.startColor.x);
                        ImGui::ColorEdit4("End Color", &gradSettings.endColor.x);

                        ImGui::SliderFloat2("Start Point", &gradSettings.startPoint.x, 0.0f, 1.0f, "%.2f");
                        ImGui::SliderFloat2("End Point", &gradSettings.endPoint.x, 0.0f, 1.0f, "%.2f");

                        ImGui::SliderFloat("Angle", &gradSettings.angle, 0.0f, 360.0f, "%.1f\xC2\xB0");

                        if (gradSettings.type == AdvancedFacePaint::GradientType::Noise) {
                            ImGui::SliderFloat("Noise Scale", &gradSettings.noiseScale, 0.1f, 10.0f, "%.2f");
                            ImGui::SliderInt("Noise Octaves", &gradSettings.noiseOctaves, 1, 8);
                            ImGui::SliderFloat("Noise Persistence", &gradSettings.noisePersistence, 0.1f, 1.0f, "%.2f");
                            ImGui::SliderFloat("Noise Lacunarity", &gradSettings.noiseLacunarity, 1.0f, 4.0f, "%.2f");
                        }

                        ImGui::Checkbox("Use Alpha", &gradSettings.useAlpha);
                        if (gradSettings.useAlpha) {
                            ImGui::SliderFloat("Alpha Blend", &gradSettings.alphaBlend, 0.0f, 1.0f, "%.2f");
                        }
                    }

                    // Smudge Options
                    if (ImGui::CollapsingHeader("Smudge Options", ImGuiTreeNodeFlags_DefaultOpen)) {
                        AdvancedFacePaint::SmudgeSettings& smudgeSettings = _currentSmudgeSettings;

                        const char* smudgeTypes[] = {"Normal", "Directional", "Radial", "Spiral", "Noise", "Custom"};
                        int smudgeTypeIdx = static_cast<int>(smudgeSettings.type);
                        if (ImGui::Combo("Smudge Type", &smudgeTypeIdx, smudgeTypes, IM_ARRAYSIZE(smudgeTypes))) {
                            smudgeSettings.type = static_cast<AdvancedFacePaint::SmudgeType>(smudgeTypeIdx);
                        }

                        ImGui::SliderFloat("Strength", &smudgeSettings.strength, 0.0f, 1.0f, "%.2f");
                        ImGui::SliderFloat("Radius", &smudgeSettings.radius, 0.01f, 1.0f, "%.2f");
                        ImGui::SliderFloat("Softness", &smudgeSettings.softness, 0.1f, 2.0f, "%.2f");
                        ImGui::SliderFloat("Pressure", &smudgeSettings.pressure, 0.1f, 2.0f, "%.2f");

                        if (smudgeSettings.type == AdvancedFacePaint::SmudgeType::Directional) {
                            ImGui::SliderFloat2("Direction", &smudgeSettings.direction.x, -1.0f, 1.0f, "%.2f");
                            ImGui::SliderFloat("Directional Strength", &smudgeSettings.directionalStrength, 0.0f, 1.0f, "%.2f");
                        }

                        if (smudgeSettings.type == AdvancedFacePaint::SmudgeType::Spiral) {
                            ImGui::SliderFloat("Speed", &smudgeSettings.speed, 0.1f, 5.0f, "%.2f");
                            ImGui::SliderFloat("Turbulence", &smudgeSettings.turbulence, 0.01f, 1.0f, "%.2f");
                            ImGui::SliderFloat("Spiral Turns", &smudgeSettings.spiralTurns, 0.5f, 5.0f, "%.2f");
                        }

                        if (smudgeSettings.type == AdvancedFacePaint::SmudgeType::Noise) {
                            ImGui::SliderFloat("Noise Intensity", &smudgeSettings.noiseIntensity, 0.0f, 1.0f, "%.2f");
                            ImGui::SliderFloat("Noise Scale", &smudgeSettings.noiseScale, 0.1f, 10.0f, "%.2f");
                        }

                        ImGui::Checkbox("Use Pressure", &smudgeSettings.usePressure);
                    }

                    ImGui::Separator();
                    ImGui::TextUnformatted("Preview & Apply");

                    if (ImGui::Button("Preview Gradient")) {
                        _showAdvancedFacePaintPanel = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Preview Smudge")) {
                        _showAdvancedFacePaintPanel = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Apply to Selected Face")) {
                        // Apply current settings to selected face
                    }

                    ImGui::Unindent();
                }
            }

            ImGui::Separator();
            int primitiveIdx = static_cast<int>(_currentPrimitive);
            const char* primitiveNames[] = {"Cube", "Sphere", "Cylinder", "Cone", "Polyhedron"};
            if (ImGui::Combo("Shape", &primitiveIdx, primitiveNames, IM_ARRAYSIZE(primitiveNames))) {
                _currentPrimitive = static_cast<Object::GeometryType>(primitiveIdx);
            }

            // Enhanced Polyhedron Generator (only in Shape Generator mode)
            if (_currentPrimitive == Object::GeometryType::Polyhedron && _current3DMode == Mode3D::BrushCreate) {
                ImGui::Separator();
                ImGui::TextUnformatted(u8"\xF0\x9F\x94\xB7 Polyhedron Generator");

                ImGui::TextUnformatted("Regular Polyhedrons:");
                if (ImGui::Button("Tetrahedron (4)")) { _currentPolyhedronType = 4; }
                ImGui::SameLine();
                if (ImGui::Button("Octahedron (8)")) { _currentPolyhedronType = 8; }
                ImGui::SameLine();
                if (ImGui::Button("Dodecahedron (12)")) { _currentPolyhedronType = 12; }
                ImGui::SameLine();
                if (ImGui::Button("Icosahedron (20)")) { _currentPolyhedronType = 20; }

                ImGui::Separator();
                ImGui::TextUnformatted("Advanced Options:");

                static int customFaceCount = 4;
                if (ImGui::SliderInt("Custom Face Count", &customFaceCount, 3, 50)) {
                    _currentPolyhedronType = customFaceCount;
                }

                if (ImGui::Button(u8"\xF0\x9F\x8E\xB2 Random Polyhedron")) {
                    _currentPolyhedronType = 4 + (rand() % 17);
                }
                ImGui::SameLine();
                if (ImGui::Button(u8"\xF0\x9F\x8E\xB2 Random Complex")) {
                    _currentPolyhedronType = 8 + (rand() % 13);
                }

                ImGui::Separator();
                ImGui::TextUnformatted("Quick Presets:");
                if (ImGui::Button("Simple (4-8)")) { _currentPolyhedronType = 4 + (rand() % 5); }
                ImGui::SameLine();
                if (ImGui::Button("Medium (8-12)")) { _currentPolyhedronType = 8 + (rand() % 5); }
                ImGui::SameLine();
                if (ImGui::Button("Complex (12-20)")) { _currentPolyhedronType = 12 + (rand() % 9); }

                ImGui::Separator();
                ImGui::Text("Selected: %d faces", _currentPolyhedronType);

                const char* polyhedronNames[] = {
                    "Unknown", "Unknown", "Unknown", "Unknown", "Tetrahedron",
                    "Unknown", "Unknown", "Unknown", "Octahedron", "Unknown",
                    "Unknown", "Unknown", "Dodecahedron", "Unknown", "Unknown",
                    "Unknown", "Unknown", "Unknown", "Unknown", "Icosahedron"
                };

                if (_currentPolyhedronType >= 4 && _currentPolyhedronType <= 20) {
                    ImGui::Text("Type: %s", polyhedronNames[_currentPolyhedronType]);
                }

                if (_currentPolyhedronType > 12) {
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), u8"\xE2\x9A\xA0 Complex polyhedron - may affect performance");
                }

                // Convex/Concave Polyhedron Controls
                ImGui::Separator();
                ImGui::TextUnformatted(u8"\xF0\x9F\x94\xB7 Convex/Concave Variants:");

                static int concaveType = 0;
                const char* concaveTypes[] = {"Regular", "Concave", "Star", "Crater"};
                if (ImGui::Combo("Variant", &concaveType, concaveTypes, IM_ARRAYSIZE(concaveTypes))) {
                    _currentConcaveType = concaveType;
                }

                if (concaveType == 1) {
                    static float concavity = 0.3f;
                    if (ImGui::SliderFloat("Concavity", &concavity, 0.1f, 0.8f, "%.2f")) {
                        _concavityAmount = concavity;
                    }
                } else if (concaveType == 2) {
                    static float spikeLength = 0.3f;
                    if (ImGui::SliderFloat("Spike Length", &spikeLength, 0.1f, 1.0f, "%.2f")) {
                        _spikeLength = spikeLength;
                    }
                } else if (concaveType == 3) {
                    static float craterDepth = 0.2f;
                    if (ImGui::SliderFloat("Crater Depth", &craterDepth, 0.1f, 0.5f, "%.2f")) {
                        _craterDepth = craterDepth;
                    }
                }

                // Custom polyhedron generation
                ImGui::Separator();
                ImGui::TextUnformatted("Custom Polyhedron:");
                ImGui::Checkbox("Use Custom Polyhedron", &_useCustomPolyhedron);

                if (_useCustomPolyhedron) {
                    if (ImGui::SliderInt("Vertex Count", &_customPolyhedronVertexCount, 3, 20)) {
                        _generateCustomPolyhedron();
                    }
                    if (ImGui::SliderInt("Face Count", &_customPolyhedronFaceCount, 3, 20)) {
                        _generateCustomPolyhedron();
                    }

                    if (ImGui::Button(u8"\xF0\x9F\x94\x84 Regenerate Custom")) {
                        _generateCustomPolyhedron();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\xF0\x9F\x92\xBE Save Custom")) {
                        ImGui::OpenPopup("Custom Polyhedron Saved");
                    }

                    ImGui::Text("Custom: %d vertices, %d faces", _customPolyhedronVertexCount, _customPolyhedronFaceCount);
                }

                // ---- Irregular Polyhedra ----
                ImGui::Separator();
                if (ImGui::CollapsingHeader("Irregular Polyhedra")) {
                    const char* irregularNames[] = {
                        "None (use Regular)", "Prism", "Antiprism",
                        "Pyramid", "Bipyramid", "Frustum"
                    };
                    if (ImGui::Combo("Irregular Shape", &_irregularType,
                                     irregularNames, IM_ARRAYSIZE(irregularNames))) {
                    }

                    if (_irregularType > 0) {
                        ImGui::SliderInt("Base Sides", &_irregularBaseSides, 3, 20);
                        ImGui::SliderFloat("Height", &_irregularHeight, 0.1f, 3.0f, "%.2f");

                        if (_irregularType == 5) {
                            ImGui::SliderFloat("Top Scale", &_frustumTopScale, 0.05f, 0.95f, "%.2f");
                        }

                        const char* shapeDesc[] = {
                            "", "Extruded polygon (flat top & bottom, straight sides)",
                            "Twisted polygon pair (alternating triangles)",
                            "Polygon base with a pointed apex",
                            "Double pyramid (diamond shape)",
                            "Pyramid with its top sliced off"
                        };
                        ImGui::TextWrapped("%s", shapeDesc[_irregularType]);
                    }
                }

                // ---- Shape Operations (Topological Modifiers) ----
                ImGui::Separator();
                if (ImGui::CollapsingHeader("Shape Operations")) {
                    ImGui::Checkbox("Truncate (slice off vertices)", &_applyTruncation);
                    if (_applyTruncation) {
                        ImGui::SliderFloat("Truncation Amount", &_truncationAmount,
                                           0.05f, 0.45f, "%.2f");
                        ImGui::TextWrapped("Slices every vertex to create new faces. "
                                           "A truncated icosahedron is a soccer ball!");
                    }

                    ImGui::Checkbox("Dual (swap faces & vertices)", &_applyDual);
                    if (_applyDual) {
                        ImGui::TextWrapped("Creates the dual polyhedron: faces become "
                                           "vertices and vertices become faces. "
                                           "The dual of a cube is an octahedron.");
                    }
                }

                // ---- Contour & Angle Analysis ----
                ImGui::Separator();
                if (ImGui::CollapsingHeader("Contour & Angle Info")) {
                    Object::PolyhedronData previewData = buildCurrentPolyhedron();

                    ImGui::TextUnformatted("Contour Types (flat vs round):");
                    int flatCount = 0, roundCount = 0;
                    for (auto ct : previewData.contourTypes) {
                        if (ct == Object::PolyhedronData::ContourType::Flat) flatCount++;
                        else roundCount++;
                    }
                    ImGui::Text("  Flat contours: %d", flatCount);
                    ImGui::Text("  Round contours: %d", roundCount);

                    ImGui::Separator();
                    ImGui::TextUnformatted("Topology:");
                    int V = previewData.getVertexCount();
                    int F = previewData.getFaceCount();
                    int E = static_cast<int>(previewData.edgeInfos.size());
                    ImGui::Text("  Vertices: %d", V);
                    ImGui::Text("  Edges: %d", E);
                    ImGui::Text("  Faces: %d", F);
                    ImGui::Text("  Euler (V-E+F): %d", AngleTools::eulerCharacteristic(V, E, F));
                    ImGui::Text("  Convex: %s", previewData.isConvex ? "Yes" : "No");

                    if (!previewData.dihedralAngles.empty()) {
                        ImGui::Separator();
                        ImGui::TextUnformatted("Dihedral Angles (between adjacent faces):");
                        float minDA = 999.0f, maxDA = -999.0f, sumDA = 0.0f;
                        for (const auto& da : previewData.dihedralAngles) {
                            float deg = da.angleDegrees();
                            minDA = std::min(minDA, deg);
                            maxDA = std::max(maxDA, deg);
                            sumDA += deg;
                        }
                        float avgDA = sumDA / static_cast<float>(previewData.dihedralAngles.size());
                        ImGui::Text("  Min: %.1f deg", minDA);
                        ImGui::Text("  Max: %.1f deg", maxDA);
                        ImGui::Text("  Avg: %.1f deg", avgDA);
                        ImGui::Text("  Count: %d", static_cast<int>(previewData.dihedralAngles.size()));
                    }

                    if (!previewData.edgeInfos.empty()) {
                        ImGui::Separator();
                        ImGui::TextUnformatted("Edge Lengths:");
                        float minLen = 999.0f, maxLen = 0.0f;
                        for (const auto& ei : previewData.edgeInfos) {
                            minLen = std::min(minLen, ei.length);
                            maxLen = std::max(maxLen, ei.length);
                        }
                        ImGui::Text("  Min: %.4f", minLen);
                        ImGui::Text("  Max: %.4f", maxLen);
                        bool isRegular = (maxLen - minLen) < 0.001f;
                        ImGui::Text("  Uniform: %s", isRegular ? "Yes (regular)" : "No (irregular)");
                    }

                    float totalDeficit = AngleTools::totalAngleDeficit(
                        previewData.vertices, previewData.faces);
                    ImGui::Separator();
                    ImGui::Text("Total angle deficit: %.2f rad (%.1f deg)",
                                totalDeficit, totalDeficit * 180.0f / static_cast<float>(M_PI));
                    ImGui::TextWrapped("(Should be ~12.57 rad / 720 deg for any closed "
                                       "convex polyhedron -- Descartes' theorem)");
                }
            } else if (_currentPrimitive == Object::GeometryType::Polyhedron) {
                ImGui::Separator();
                ImGui::TextUnformatted("Polyhedron Type:");
                if (ImGui::Button("Tetrahedron")) { _currentPolyhedronType = 4; }
                ImGui::SameLine();
                if (ImGui::Button("Octahedron")) { _currentPolyhedronType = 8; }
                ImGui::SameLine();
                if (ImGui::Button("Dodecahedron")) { _currentPolyhedronType = 12; }
                ImGui::SameLine();
                if (ImGui::Button("Icosahedron")) { _currentPolyhedronType = 20; }
                ImGui::Text("Selected: %d faces", _currentPolyhedronType);
            }

            ImGui::SliderFloat("Uniform Size", &_brushSize, 0.1f, 10.0f, "%.2f");

            // Pottery specific controls
            if (_current3DMode == Mode3D::Pottery) {
                ImGui::Separator();
                ImGui::TextUnformatted("Pottery Tool:");
                bool isChisel = _currentPotteryTool == PotteryTool::Chisel;
                if (ImGui::RadioButton("Chisel", isChisel)) _currentPotteryTool = PotteryTool::Chisel;
                ImGui::SameLine();
                bool isExpand = _currentPotteryTool == PotteryTool::Expand;
                if (ImGui::RadioButton("Expand", isExpand)) _currentPotteryTool = PotteryTool::Expand;
                ImGui::SliderFloat("Strength", &_potteryStrength, 0.01f, 2.0f, "%.2f");
            }

            // Placement mode controls
            ImGui::Separator();
            int placeIdx = static_cast<int>(_placementMode);
            const char* placeNames[] = {"In Front", "Manual Distance", "Cursor Snap"};
            if (ImGui::Combo("Placement", &placeIdx, placeNames, IM_ARRAYSIZE(placeNames))) {
                _placementMode = static_cast<BrushPlacementMode>(placeIdx);
            }
            if (_placementMode == BrushPlacementMode::ManualDistance && _prevPlacementMode != BrushPlacementMode::ManualDistance) {
                _manualAnchorPos      = _cameraPos + _cameraFront * 2.0f;
                _manualAnchorRight    = glm::normalize(glm::cross(_cameraFront, _cameraUp));
                _manualAnchorUp       = _cameraUp;
                _manualAnchorForward  = _cameraFront;
                _manualAnchorValid    = true;
            }
            _prevPlacementMode = _placementMode;
            if (_placementMode == BrushPlacementMode::ManualDistance) {
                ImGui::SliderFloat3("Offset XYZ", &_manualOffset.x, -20.0f, 20.0f, "%.2f");
                ImGui::TextUnformatted("X = right, Y = up, Z = forward");
            }

            // Face Brush settings
            if (_current3DMode == Mode3D::FaceBrush) {
                ImGui::Separator();

                ImGui::Text("Brush Type:");
                const char* brushTypeNames[] = {"Normal", "Airbrush", "Chalk", "Spray", "Smudge", "Clone"};
                int brushTypeIdx = static_cast<int>(_currentBrushType);
                if (ImGui::Combo("##BrushType", &brushTypeIdx, brushTypeNames, IM_ARRAYSIZE(brushTypeNames))) {
                    _currentBrushType = static_cast<BrushType>(brushTypeIdx);
                }

                // Brush Presets
                ImGui::Separator();
                ImGui::Text("Brush Presets:");
                if (ImGui::Button("Save Preset")) {
                    BrushPreset preset;
                    preset.name = "Custom " + std::to_string(_brushPresets.size() + 1);
                    preset.type = _currentBrushType;
                    preset.radius = _faceBrushRadius;
                    preset.softness = _faceBrushSoftness;
                    preset.opacity = _brushOpacity;
                    preset.flow = _brushFlow;
                    preset.spacing = _brushSpacing;
                    preset.density = _brushDensity;
                    preset.strength = _brushStrength;
                    _brushPresets.push_back(preset);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load Preset") && !_brushPresets.empty()) {
                    if (_currentPreset >= 0 && _currentPreset < static_cast<int>(_brushPresets.size())) {
                        const BrushPreset& preset = _brushPresets[_currentPreset];
                        _currentBrushType = preset.type;
                        _faceBrushRadius = preset.radius;
                        _faceBrushSoftness = preset.softness;
                        _brushOpacity = preset.opacity;
                        _brushFlow = preset.flow;
                        _brushSpacing = preset.spacing;
                        _brushDensity = preset.density;
                        _brushStrength = preset.strength;
                    }
                }

                if (!_brushPresets.empty()) {
                    std::vector<const char*> presetNames;
                    for (const auto& preset : _brushPresets) {
                        presetNames.push_back(preset.name.c_str());
                    }
                    ImGui::Combo("##PresetSelect", &_currentPreset, presetNames.data(), static_cast<int>(presetNames.size()));
                }

                ImGui::Separator();
                ImGui::Text("Basic Settings:");
                ImGui::SliderFloat("Brush Radius", &_faceBrushRadius, 0.01f, 2.0f, "%.2f");
                ImGui::SliderFloat("Softness", &_faceBrushSoftness, 0.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("Opacity", &_brushOpacity, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("Flow", &_brushFlow, 0.0f, 1.0f, "%.2f");

                ImGui::Separator();
                ImGui::Text("Advanced Dynamics:");
                ImGui::SliderFloat("Spacing", &_brushSpacing, 0.01f, 0.5f, "%.2f");
                ImGui::SliderFloat("Density", &_brushDensity, 0.1f, 1.0f, "%.2f");
                ImGui::SliderFloat("Strength", &_brushStrength, 0.0f, 1.0f, "%.2f");

                ImGui::Separator();
                ImGui::Text("Pressure Simulation:");
                ImGui::Checkbox("Enable Pressure", &_usePressureSimulation);
                if (_usePressureSimulation) {
                    ImGui::SliderFloat("Sensitivity", &_pressureSensitivity, 0.1f, 5.0f, "%.2f");
                    ImGui::SliderFloat("Current Pressure", &_currentPressure, 0.1f, 1.0f, "%.2f");
                }

                ImGui::Separator();
                ImGui::Text("Stroke Settings:");
                ImGui::Checkbox("Stroke Interpolation", &_useStrokeInterpolation);
                ImGui::Checkbox("Show Brush Cursor", &_showBrushCursor);
                ImGui::Checkbox("Show Brush Preview", &_showBrushPreview);

                if (_currentBrushType == BrushType::Clone) {
                    ImGui::Separator();
                    ImGui::Text("Clone Tool:");
                    ImGui::Checkbox("Clone Active", &_cloneToolActive);
                    if (_cloneToolActive) {
                        ImGui::SliderFloat2("Clone Offset", &_cloneOffset.x, -1.0f, 1.0f, "%.2f");
                        if (ImGui::Button("Set Source Point")) {
                            _cloneSourceUV = _brushCursorPos;
                        }
                    }
                }

                ImGui::Separator();
                ImGui::Text("Layer System:");
                ImGui::Checkbox("Use Layers", &_useLayers);
                if (_useLayers) {
                    ImGui::SliderInt("Active Layer", &_activeLayer, 0, 10);
                    ImGui::SliderFloat("Layer Opacity", &_layerOpacity, 0.0f, 1.0f, "%.2f");

                    const char* blendModeNames[] = {"Normal", "Multiply", "Screen", "Overlay", "Add", "Subtract"};
                    ImGui::Combo("Blend Mode", &_blendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames));

                    if (ImGui::Button("Add Layer")) {
                        const auto& objects = mgr.active().world().getOwnedObjects();
                        for (const auto& up : objects) {
                            Object* obj = up.get();
                            obj->addTextureLayer(0);
                            break;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Delete Layer")) {
                        const auto& objects = mgr.active().world().getOwnedObjects();
                        for (const auto& up : objects) {
                            Object* obj = up.get();
                            obj->deleteTextureLayer(0, _activeLayer);
                            break;
                        }
                    }
                }

                ImGui::Separator();
                ImGui::Text("UV Controls:");
                ImGui::SliderFloat("U Offset", &_faceBrushUOffset, -2.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("V Offset", &_faceBrushVOffset, -2.0f, 2.0f, "%.2f");
                const char* axisNames[] = {"X","Y","Z"};
                ImGui::Combo("Axis 1", &_faceBrushUAxis, axisNames, 3);
                ImGui::Combo("Axis 2", &_faceBrushVAxis, axisNames, 3);
                if(_faceBrushVAxis == _faceBrushUAxis) {
                    ImGui::TextColored(ImVec4(1,0,0,1), "Axis 1 and Axis 2 must differ!");
                }
                ImGui::Checkbox("Invert Axis 1", &_faceBrushInvertU);
                ImGui::SameLine();
                ImGui::Checkbox("Invert Axis 2", &_faceBrushInvertV);

                ImGui::Separator();
                ImGui::Text("History:");
                if (ImGui::Button("Undo (Ctrl+Z)")) {
                    const auto& objects = mgr.active().world().getOwnedObjects();
                    for (const auto& up : objects) {
                        Object* obj = up.get();
                        obj->undoStroke(0);
                        break;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Redo (Ctrl+Y)")) {
                    // Redo placeholder
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear History")) {
                    const auto& objects = mgr.active().world().getOwnedObjects();
                    for (const auto& up : objects) {
                        Object* obj = up.get();
                        obj->clearStrokeHistory(0);
                        break;
                    }
                }
            }
        }
        ImGui::End();
    }

    // ------------------------------------------------------------------
    // World window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (showWorld) {
        if (ImGui::Begin(u8"\xF0\x9F\x8C\x8D World", &showWorld)) {
            _world.renderModeUI();
        }
        ImGui::End();
    }

    // ------------------------------------------------------------------
    // Cursor Tools window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (showCursor) {
        bool open = true;
        _cursorTools.renderUI(open);
        if (!open) showCursor = false;
    }

    // ------------------------------------------------------------------
    // Assets window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (showAssets) {
        if (ImGui::Begin(u8"\xF0\x9F\x92\xBE Assets", &showAssets)) {
            if (ImGui::Button(u8"\xF0\x9F\x92\xBE Quick Save")) {
                saveStateWithLog();
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"\xF0\x9F\x92\xBE Save As...")) {
                _showSaveWindow = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"\xF0\x9F\x93\x82 Load")) {
                updateSaveFiles();
                _showLoadWindow = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"\xF0\x9F\x93\x81 Save Manager")) {
                _showSaveManager = true;
            }
        }
        ImGui::End();
    }

    // ------------------------------------------------------------------
    // Bonds window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (showBonds) {
        if (ImGui::Begin(u8"\xF0\x9F\x94\x97 Bonds", &showBonds)) {
            auto& zoneWorld = mgr.active().world();
            const auto& objs = zoneWorld.getOwnedObjects();

            static int objAIdx = 0;
            static int objBIdx = 1;

            std::vector<std::string> labels;
            labels.reserve(objs.size());
            for (size_t i = 0; i < objs.size(); ++i) {
                char buf[32];
                snprintf(buf, sizeof(buf), "Obj %zu", i);
                labels.emplace_back(buf);
            }
            std::vector<const char*> cstrs;
            for (auto& s : labels) cstrs.push_back(s.c_str());

            if (!labels.empty()) {
                ImGui::Combo("Object A", &objAIdx, cstrs.data(), static_cast<int>(labels.size()));
                ImGui::Combo("Object B", &objBIdx, cstrs.data(), static_cast<int>(labels.size()));
                if (ImGui::Button("Create Bond") && objAIdx != objBIdx && objAIdx < static_cast<int>(labels.size()) && objBIdx < static_cast<int>(labels.size())) {
                    Physics::addBond(objs[objAIdx].get(), objs[objBIdx].get());
                }
            } else {
                ImGui::TextUnformatted("No objects available.");
            }

            ImGui::Separator();

            ImGui::Text("Auto Bond Rules (shape pairs):");
            static int shapeAIdx = 0;
            static int shapeBIdx = 1;
            const char* shapeNames[] = {"Cube", "Sphere", "Cylinder", "Cone"};
            ImGui::Combo("Shape A", &shapeAIdx, shapeNames, IM_ARRAYSIZE(shapeNames));
            ImGui::Combo("Shape B", &shapeBIdx, shapeNames, IM_ARRAYSIZE(shapeNames));

            bool enabled = Physics::getAutoBond(static_cast<Object::GeometryType>(shapeAIdx), static_cast<Object::GeometryType>(shapeBIdx));
            if (ImGui::Checkbox("Bonded##Enabled", &enabled)) {
                Physics::setAutoBond(static_cast<Object::GeometryType>(shapeAIdx), static_cast<Object::GeometryType>(shapeBIdx), enabled);
            }

            ImGui::Separator();

            ImGui::Text("Existing Bonds:");
            const auto& bonds = Physics::getBonds();

            if (bonds.empty()) {
                ImGui::TextUnformatted("<none>");
            } else {
                static int selectedBond = -1;
                if (ImGui::BeginListBox("##BondList", ImVec2(-FLT_MIN, 120))) {
                    for (int i = 0; i < static_cast<int>(bonds.size()); ++i) {
                        int idxA = -1, idxB = -1;
                        for (size_t j = 0; j < objs.size(); ++j) {
                            if (objs[j].get() == bonds[i].a) idxA = static_cast<int>(j);
                            if (objs[j].get() == bonds[i].b) idxB = static_cast<int>(j);
                        }
                        char label[64];
                        snprintf(label, sizeof(label), "%d: Obj %d <-> Obj %d", i, idxA, idxB);
                        if (ImGui::Selectable(label, selectedBond == i)) {
                            selectedBond = i;
                        }
                    }
                    ImGui::EndListBox();
                }

                if (selectedBond >= 0 && selectedBond < static_cast<int>(bonds.size())) {
                    auto& bond = bonds[selectedBond];
                    float restLen = bond.restLength;
                    float strength = bond.strength;
                    if (ImGui::DragFloat("Rest Length", &restLen, 0.05f, 0.0f, 10.0f, "%.2f")) {
                        Physics::setBondParams(bond.a, bond.b, restLen, strength);
                    }
                    if (ImGui::DragFloat("Strength", &strength, 0.5f, 0.0f, 100.0f, "%.1f")) {
                        Physics::setBondParams(bond.a, bond.b, restLen, strength);
                    }
                    if (ImGui::Button("Remove Bond")) {
                        Physics::removeBond(bond.a, bond.b);
                        selectedBond = -1;
                    }
                }
            }
        }
        ImGui::End();
    }

    // ------------------------------------------------------------------
    // Relation Manager window
    // ------------------------------------------------------------------
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
#endif
    if (showRelations) {
        Rendering::renderRelationManagerWindow(&showRelations, Physics::registry());
    }

    // Show save/load dialogs if requested
    drawLoadWindow();
    drawSaveWindow();
    drawSaveManager();
}

} // namespace Core
