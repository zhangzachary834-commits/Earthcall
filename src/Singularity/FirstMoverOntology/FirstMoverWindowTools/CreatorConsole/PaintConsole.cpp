#include "CreatorConsoleState.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Screen/BrushSystem.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include <imgui.h>
#include <algorithm>

namespace Rendering {

    namespace {
        constexpr float kDefaultBrushRadius = 0.001f;
        constexpr float kPencilRadius = 0.001f;
        constexpr float kPenRadius = 0.0015f;
        constexpr float kMarkerRadius = 0.003f;

        float spacingForBrushRadius(float radius) {
            return std::max(0.0005f, radius * 0.35f);
        }

        void configurePaintBrushPreset(CreatorConsoleState& /*state*/, BrushSystem* brushSys, Tool::Type type) {
            if (!brushSys) return;

            brushSys->setCloneActive(false);
            switch (type) {
                case Tool::Type::Pencil:
                    brushSys->setBrushType(BrushSystem::BrushType::Normal);
                    brushSys->setRadius(kPencilRadius);
                    brushSys->setOpacity(1.0f);
                    brushSys->setFlow(1.0f);
                    brushSys->setSpacing(spacingForBrushRadius(kPencilRadius));
                    break;
                case Tool::Type::Pen:
                    brushSys->setBrushType(BrushSystem::BrushType::Normal);
                    brushSys->setRadius(kPenRadius);
                    brushSys->setOpacity(1.0f);
                    brushSys->setFlow(1.0f);
                    brushSys->setSpacing(spacingForBrushRadius(kPenRadius));
                    break;
                case Tool::Type::Marker:
                    brushSys->setBrushType(BrushSystem::BrushType::Normal);
                    brushSys->setRadius(kMarkerRadius);
                    brushSys->setOpacity(0.55f);
                    brushSys->setFlow(0.75f);
                    brushSys->setSpacing(spacingForBrushRadius(kMarkerRadius));
                    break;
                case Tool::Type::Airbrush:
                    brushSys->setBrushType(BrushSystem::BrushType::Airbrush);
                    break;
                case Tool::Type::Chalk:
                    brushSys->setBrushType(BrushSystem::BrushType::Chalk);
                    break;
                case Tool::Type::Spray:
                    brushSys->setBrushType(BrushSystem::BrushType::Spray);
                    break;
                case Tool::Type::Smudge:
                    brushSys->setBrushType(BrushSystem::BrushType::Smudge);
                    break;
                case Tool::Type::Clone:
                    brushSys->setBrushType(BrushSystem::BrushType::Clone);
                    brushSys->setCloneActive(true);
                    brushSys->setCloneOffset(glm::vec2(-0.08f, -0.08f));
                    break;
                case Tool::Type::Brush:
                    brushSys->setBrushType(BrushSystem::BrushType::Normal);
                    brushSys->setRadius(kDefaultBrushRadius);
                    brushSys->setOpacity(1.0f);
                    brushSys->setFlow(1.0f);
                    brushSys->setSpacing(spacingForBrushRadius(kDefaultBrushRadius));
                    break;
                case Tool::Type::Eraser:
                case Tool::Type::Delete:
                case Tool::Type::MagicEraser:
                    brushSys->setBrushType(BrushSystem::BrushType::Normal);
                    break;
                default:
                    break;
            }
        }

        void setPaintTool(CreatorConsoleState& state, BrushSystem* brushSys, Tool::Type type) {
            state.currentTool = Tool(type);
            state.current3DMode = Mode3D::None;
            if (auto* channel = Singularity::Core::CreationChannel::find(*Core::Engine::instance().getLawManager())) {
                channel->activeTool = state.currentTool.getTypeName();
            }
            configurePaintBrushPreset(state, brushSys, type);
        }

        void renderPaintToolButton(CreatorConsoleState& state, BrushSystem* brushSys, Tool::Type type, const char* label) {
            const bool active = state.current3DMode == Mode3D::None && state.currentTool.getType() == type;
            pushActiveButtonStyle(active, ImVec4(0.30f, 0.50f, 0.31f, 1.0f),
                                  ImVec4(0.36f, 0.62f, 0.38f, 1.0f));
            const bool pressed = ImGui::Button(label, ImVec2(118.0f, 0.0f));
            popActiveButtonStyle(active);
            if (pressed) {
                setPaintTool(state, brushSys, type);
            }
        }
    } // namespace

    void renderPaintConsole(ZoneManager& /*zoneMgr*/) {
        auto& state = getCreatorConsoleState();
        static BrushSystem localBrushSys(512);
        BrushSystem* brushSys = &localBrushSys;

        struct PaintToolDef {
            Tool::Type type;
            const char* label;
        };

        static const PaintToolDef drawingTools[] = {
            {Tool::Type::Brush, "Brush"},
            {Tool::Type::Pencil, "Pencil"},
            {Tool::Type::Pen, "Pen"},
            {Tool::Type::Airbrush, "Airbrush"},
            {Tool::Type::Chalk, "Chalk"},
            {Tool::Type::Spray, "Spray"},
            {Tool::Type::Smudge, "Smudge"},
            {Tool::Type::Clone, "Clone"}
        };
        static const PaintToolDef shapeTools[] = {
            {Tool::Type::Rectangle, "Rectangle"},
            {Tool::Type::Ellipse, "Ellipse"},
            {Tool::Type::Polygon, "Polygon"},
            {Tool::Type::Line, "Line"},
            {Tool::Type::Arrow, "Arrow"},
            {Tool::Type::Star, "Star"},
            {Tool::Type::Heart, "Heart"},
            {Tool::Type::CustomShape, "Custom"}
        };
        static const PaintToolDef utilityTools[] = {
            {Tool::Type::Eraser, "Eraser"},
            {Tool::Type::Delete, "Delete"},
            {Tool::Type::MagicEraser, "Magic Eraser"},
            {Tool::Type::Selection, "Select"},
            {Tool::Type::Lasso, "Lasso"},
            {Tool::Type::MagicWand, "Wand"},
            {Tool::Type::Marquee, "Marquee"},
            {Tool::Type::ColorPicker, "Color"},
            {Tool::Type::Eyedropper, "Dropper"}
        };
        static const PaintToolDef textTools[] = {
            {Tool::Type::Text, "Text"},
            {Tool::Type::TextVertical, "Vertical"},
            {Tool::Type::TextPath, "Path"}
        };

        ImGui::TextUnformatted("2D Paint Tool Belt");
        ImGui::Separator();

        for (int i = 0; i < IM_ARRAYSIZE(drawingTools); ++i) {
            renderPaintToolButton(state, brushSys, drawingTools[i].type, drawingTools[i].label);
            sameLineEvery(i, 3);
        }

        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Shapes", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (int i = 0; i < IM_ARRAYSIZE(shapeTools); ++i) {
                renderPaintToolButton(state, brushSys, shapeTools[i].type, shapeTools[i].label);
                sameLineEvery(i, 3);
            }
        }

        if (ImGui::CollapsingHeader("Utility")) {
            for (int i = 0; i < IM_ARRAYSIZE(utilityTools); ++i) {
                renderPaintToolButton(state, brushSys, utilityTools[i].type, utilityTools[i].label);
                sameLineEvery(i, 3);
            }
        }

        if (ImGui::CollapsingHeader("Text")) {
            for (int i = 0; i < IM_ARRAYSIZE(textTools); ++i) {
                renderPaintToolButton(state, brushSys, textTools[i].type, textTools[i].label);
                sameLineEvery(i, 3);
            }
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Paint Inspector");

        if (ImGui::ColorEdit3("Color", &state.createColor.x, ImGuiColorEditFlags_NoInputs)) {
            if (auto* channel = Singularity::Core::CreationChannel::find(*Core::Engine::instance().getLawManager())) {
                channel->activeColor = state.createColor;
            }
        }

        ImGui::Checkbox("Advanced 2D Brush", &state.brush.useAdvanced2D);
        if (state.brush.useAdvanced2D && brushSys) {
            const char* brushTypes[] = {"Normal", "Airbrush", "Chalk", "Spray", "Smudge", "Clone"};
            int currentType = static_cast<int>(brushSys->getBrushType());
            if (ImGui::Combo("Brush Type", &currentType, brushTypes, IM_ARRAYSIZE(brushTypes))) {
                brushSys->setBrushType(static_cast<BrushSystem::BrushType>(currentType));
            }

            float radius = brushSys->getRadius();
            if (ImGui::SliderFloat("Radius", &radius, 0.001f, 0.02f, "%.4f")) {
                brushSys->setRadius(radius);
                brushSys->setSpacing(spacingForBrushRadius(radius));
            }

            float opacity = brushSys->getOpacity();
            if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f, "%.2f")) {
                brushSys->setOpacity(opacity);
            }

            float flow = brushSys->getFlow();
            if (ImGui::SliderFloat("Flow", &flow, 0.0f, 1.0f, "%.2f")) {
                brushSys->setFlow(flow);
            }

            if (ImGui::CollapsingHeader("Dynamics")) {
                float spacing = brushSys->getSpacing();
                if (ImGui::SliderFloat("Spacing", &spacing, 0.0005f, 0.02f, "%.4f")) {
                    brushSys->setSpacing(spacing);
                }

                float density = brushSys->getDensity();
                if (ImGui::SliderFloat("Density", &density, 0.1f, 5.0f, "%.2f")) {
                    brushSys->setDensity(density);
                }

                float strength = brushSys->getStrength();
                if (ImGui::SliderFloat("Strength", &strength, 0.0f, 5.0f, "%.2f")) {
                    brushSys->setStrength(strength);
                }

                static bool usePressureSim = false;
                if (ImGui::Checkbox("Pressure Simulation", &usePressureSim)) {
                    brushSys->setPressureSimulation(usePressureSim);
                }
                float pressureSensitivity = brushSys->getPressureSensitivity();
                if (ImGui::SliderFloat("Pressure Sensitivity", &pressureSensitivity, 0.01f, 5.0f, "%.2f")) {
                    brushSys->setPressureSensitivity(pressureSensitivity);
                }
                bool interpolate = brushSys->getStrokeInterpolation();
                if (ImGui::Checkbox("Stroke Interpolation", &interpolate)) {
                    brushSys->setStrokeInterpolation(interpolate);
                }
            }

            if (ImGui::CollapsingHeader("Layers")) {
                bool useLayers = brushSys->getUseLayers();
                if (ImGui::Checkbox("Use Layers", &useLayers)) {
                    brushSys->setUseLayers(useLayers);
                }
                int activeLayer = brushSys->getActiveLayer();
                int layerMax = std::max(0, brushSys->getLayerCount() - 1);
                if (ImGui::SliderInt("Active Layer", &activeLayer, 0, layerMax)) {
                    brushSys->setActiveLayer(activeLayer);
                }
                float layerOpacity = brushSys->getLayerOpacity();
                if (ImGui::SliderFloat("Layer Opacity", &layerOpacity, 0.0f, 1.0f, "%.2f")) {
                    brushSys->setLayerOpacity(layerOpacity);
                }
                const char* blendModes[] = {"Normal", "Multiply", "Screen", "Overlay", "Add", "Subtract"};
                int blendMode = static_cast<int>(brushSys->getBlendMode());
                if (ImGui::Combo("Blend Mode", &blendMode, blendModes, IM_ARRAYSIZE(blendModes))) {
                    brushSys->setBlendMode(static_cast<BrushSystem::BlendMode>(blendMode));
                }
                if (ImGui::Button("Add Layer")) {
                    brushSys->addLayer();
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete Layer")) {
                    brushSys->deleteLayer(brushSys->getActiveLayer());
                }
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Undo")) {
            if (brushSys) brushSys->undo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Redo")) {
            if (brushSys) brushSys->redo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear History")) {
            if (brushSys) brushSys->clearHistory();
        }
    }

} // namespace Rendering
