#include "CreatorConsoleState.hpp"
#include <imgui.h>
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Singularity/Screen/BrushSystem.hpp"
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

        void configurePaintBrushPreset(Zone& zone, Tool::Type type) {
            // commented out
        }

        void setPaintTool(Zone& zone, Tool::Type type) {
            auto& state = getCreatorConsoleState();
            state.currentTool = Tool(type);
            // zone.setDesignTool(type); // doesn't exist
            state.current3DMode = Mode3D::None;
        }

        void renderPaintToolButton(Zone& zone, Tool::Type type, const char* label) {
            auto& state = getCreatorConsoleState();
            const bool active = state.current3DMode == Mode3D::None && state.currentTool.getType() == type;
            pushActiveButtonStyle(active, ImVec4(0.30f, 0.50f, 0.31f, 1.0f),
                                  ImVec4(0.36f, 0.62f, 0.38f, 1.0f));
            const bool pressed = ImGui::Button(label, ImVec2(118.0f, 0.0f));
            popActiveButtonStyle(active);
            if (pressed) {
                setPaintTool(zone, type);
            }
        }
    }

    void renderPaintConsole(ZoneManager& zoneMgr) {
        Zone& zone = zoneMgr.active();
        auto& state = getCreatorConsoleState();

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

        ImGui::TextUnformatted("Tool Belt");
        for (int i = 0; i < IM_ARRAYSIZE(drawingTools); ++i) {
            renderPaintToolButton(zone, drawingTools[i].type, drawingTools[i].label);
            sameLineEvery(i, 3);
        }

        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Shapes", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (int i = 0; i < IM_ARRAYSIZE(shapeTools); ++i) {
                renderPaintToolButton(zone, shapeTools[i].type, shapeTools[i].label);
                sameLineEvery(i, 3);
            }
        }

        if (ImGui::CollapsingHeader("Utility")) {
            for (int i = 0; i < IM_ARRAYSIZE(utilityTools); ++i) {
                renderPaintToolButton(zone, utilityTools[i].type, utilityTools[i].label);
                sameLineEvery(i, 3);
            }
        }

        if (ImGui::CollapsingHeader("Text")) {
            for (int i = 0; i < IM_ARRAYSIZE(textTools); ++i) {
                renderPaintToolButton(zone, textTools[i].type, textTools[i].label);
                sameLineEvery(i, 3);
            }
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Paint Inspector");
        if (ImGui::ColorEdit3("Color", state.currentColor, ImGuiColorEditFlags_NoInputs)) {
            // zone.setDrawColor(state.currentColor[0], state.currentColor[1], state.currentColor[2]);
        }

        ImGui::Checkbox("Advanced 2D Brush", &state.brush.useAdvanced2D);
        if (ImGui::Checkbox("Legacy 2D Tools", &state.useLegacy2DTools)) {
            // Note: zone doesn't store this, it's just state here.
        }
        
        ImGui::TextDisabled("Brush properties have been detached from Zone.");

        ImGui::Separator();
        if (ImGui::Button("Undo")) {
            // zone.undo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Redo")) {
            // zone.redo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear History")) {
            // zone.clearHistory();
        }
    }

} // namespace Rendering
