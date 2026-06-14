// GameToolbar.cpp - unified Creator Console for creation, editing, and world tools.

#include "Game.hpp"
#include "Form/Object/Object.hpp"
#include "Rendering/BrushSystem.hpp"
#include "Rendering/RelationManagerWindow.hpp"
#include "OurVerse/Tool.hpp"
#include "OurVerse/AdvancedFacePaint.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

extern ZoneManager mgr;

namespace {
constexpr float kDefaultBrushRadius = 0.001f;
constexpr float kPencilRadius = 0.001f;
constexpr float kPenRadius = 0.0015f;
constexpr float kMarkerRadius = 0.003f;

float spacingForBrushRadius(float radius) {
    return std::max(0.0005f, radius * 0.35f);
}

void sameLineEvery(int index, int perRow) {
    if ((index + 1) % perRow != 0) {
        ImGui::SameLine();
    }
}

void pushActiveButtonStyle(bool active, const ImVec4& color, const ImVec4& hoverColor) {
    if (!active) return;
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
}

void popActiveButtonStyle(bool active) {
    if (active) {
        ImGui::PopStyleColor(2);
    }
}

void configurePaintBrushPreset(Zone& zone, Tool::Type type) {
    if (!zone.getBrushSystem()) {
        zone.initializeBrushSystem();
    }

    zone.setCloneActive(false);
    switch (type) {
        case Tool::Type::Pencil:
            zone.setBrushType(BrushSystem::BrushType::Normal);
            zone.setBrushRadius(kPencilRadius);
            zone.setBrushOpacity(1.0f);
            zone.setBrushFlow(1.0f);
            zone.setBrushSpacing(spacingForBrushRadius(kPencilRadius));
            break;
        case Tool::Type::Pen:
            zone.setBrushType(BrushSystem::BrushType::Normal);
            zone.setBrushRadius(kPenRadius);
            zone.setBrushOpacity(1.0f);
            zone.setBrushFlow(1.0f);
            zone.setBrushSpacing(spacingForBrushRadius(kPenRadius));
            break;
        case Tool::Type::Marker:
            zone.setBrushType(BrushSystem::BrushType::Normal);
            zone.setBrushRadius(kMarkerRadius);
            zone.setBrushOpacity(0.55f);
            zone.setBrushFlow(0.75f);
            zone.setBrushSpacing(spacingForBrushRadius(kMarkerRadius));
            break;
        case Tool::Type::Airbrush:
            zone.setBrushType(BrushSystem::BrushType::Airbrush);
            break;
        case Tool::Type::Chalk:
            zone.setBrushType(BrushSystem::BrushType::Chalk);
            break;
        case Tool::Type::Spray:
            zone.setBrushType(BrushSystem::BrushType::Spray);
            break;
        case Tool::Type::Smudge:
            zone.setBrushType(BrushSystem::BrushType::Smudge);
            break;
        case Tool::Type::Clone:
            zone.setBrushType(BrushSystem::BrushType::Clone);
            zone.setCloneActive(true);
            zone.setCloneOffset(glm::vec2(-0.08f, -0.08f));
            break;
        case Tool::Type::Brush:
            zone.setBrushType(BrushSystem::BrushType::Normal);
            zone.setBrushRadius(kDefaultBrushRadius);
            zone.setBrushOpacity(1.0f);
            zone.setBrushFlow(1.0f);
            zone.setBrushSpacing(spacingForBrushRadius(kDefaultBrushRadius));
            break;
        case Tool::Type::Eraser:
        case Tool::Type::Delete:
        case Tool::Type::MagicEraser:
            zone.setBrushType(BrushSystem::BrushType::Normal);
            break;
        default:
            break;
    }
}

} // namespace

namespace Core {

void Game::renderCreatorToolbar() {
    Zone& zone = mgr.active();
    if (!zone.getDesignSystem()) {
        zone.initializeDesignSystem();
    }

    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(540.0f, 720.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Earthcall Creator Console", &_showToolbar, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        drawLoadWindow();
        drawSaveWindow();
        drawSaveManager();
        return;
    }

    renderCreatorSectionTabs();
    ImGui::Separator();

    switch (_creatorSection) {
        case CreatorSection::Paint:
            renderPaintConsole(zone);
            break;
        case CreatorSection::Create3D:
            render3DConsole();
            break;
        case CreatorSection::Character:
            renderCharacterConsole();
            break;
        case CreatorSection::World:
            renderWorldConsole();
            break;
        case CreatorSection::Assets:
            renderAssetsConsole(zone);
            break;
        case CreatorSection::Relations:
            renderRelationsConsole(zone);
            break;
    }

    renderCreatorStatusBar();
    ImGui::End();

    if (_showRelationManager) {
        Rendering::renderRelationManagerWindow(&_showRelationManager, Physics::registry());
    }

    drawLoadWindow();
    drawSaveWindow();
    drawSaveManager();
}

void Game::renderCreatorSectionTabs() {
    renderSectionButton(CreatorSection::Paint, "Paint");
    ImGui::SameLine();
    renderSectionButton(CreatorSection::Create3D, "3D");
    ImGui::SameLine();
    renderSectionButton(CreatorSection::Character, "Avatar");
    ImGui::SameLine();
    renderSectionButton(CreatorSection::World, "World");
    ImGui::SameLine();
    renderSectionButton(CreatorSection::Assets, "Assets");
    ImGui::SameLine();
    renderSectionButton(CreatorSection::Relations, "Relations");
}

void Game::renderSectionButton(CreatorSection section, const char* label) {
    const bool active = _creatorSection == section;
    pushActiveButtonStyle(active, ImVec4(0.24f, 0.43f, 0.78f, 1.0f),
                          ImVec4(0.30f, 0.52f, 0.92f, 1.0f));
    const bool pressed = ImGui::Button(label, ImVec2(78.0f, 0.0f));
    popActiveButtonStyle(active);
    if (pressed) {
        _creatorSection = section;
    }
}

void Game::setPaintTool(Zone& zone, Tool::Type type) {
    _currentTool = Tool(type);
    zone.setDesignTool(type);
    _current3DMode = Mode3D::None;

    if (!zone.getBrushSystem()) {
        zone.initializeBrushSystem();
    }
    configurePaintBrushPreset(zone, type);
}

void Game::renderPaintToolButton(Zone& zone, Tool::Type type, const char* label) {
    const bool active = _current3DMode == Mode3D::None && _currentTool.getType() == type;
    pushActiveButtonStyle(active, ImVec4(0.30f, 0.50f, 0.31f, 1.0f),
                          ImVec4(0.36f, 0.62f, 0.38f, 1.0f));
    const bool pressed = ImGui::Button(label, ImVec2(118.0f, 0.0f));
    popActiveButtonStyle(active);
    if (pressed) {
        setPaintTool(zone, type);
    }
}

void Game::renderPaintConsole(Zone& zone) {
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
    if (ImGui::ColorEdit3("Color", _currentColor, ImGuiColorEditFlags_NoInputs)) {
        zone.setDrawColor(_currentColor[0], _currentColor[1], _currentColor[2]);
    }

    ImGui::Checkbox("Advanced 2D Brush", &_brush.useAdvanced2D);
    if (_brush.useAdvanced2D) {
        if (!zone.getBrushSystem()) {
            zone.initializeBrushSystem();
        }
        BrushSystem* brushSystem = zone.getBrushSystem();
        if (brushSystem) {
            const char* brushTypes[] = {"Normal", "Airbrush", "Chalk", "Spray", "Smudge", "Clone"};
            int currentType = static_cast<int>(brushSystem->getBrushType());
            if (ImGui::Combo("Brush Type", &currentType, brushTypes, IM_ARRAYSIZE(brushTypes))) {
                zone.setBrushType(static_cast<BrushSystem::BrushType>(currentType));
            }

            float radius = brushSystem->getRadius();
            if (ImGui::SliderFloat("Radius", &radius, 0.001f, 0.02f, "%.4f")) {
                zone.setBrushRadius(radius);
                zone.setBrushSpacing(spacingForBrushRadius(radius));
            }

            float opacity = brushSystem->getOpacity();
            if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f, "%.2f")) {
                zone.setBrushOpacity(opacity);
            }

            float flow = brushSystem->getFlow();
            if (ImGui::SliderFloat("Flow", &flow, 0.0f, 1.0f, "%.2f")) {
                zone.setBrushFlow(flow);
            }

            if (ImGui::CollapsingHeader("Dynamics")) {
                float spacing = brushSystem->getSpacing();
                if (ImGui::SliderFloat("Spacing", &spacing, 0.0005f, 0.02f, "%.4f")) {
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

                if (ImGui::Checkbox("Pressure Simulation", &_use2DPressureSimulation)) {
                    zone.setPressureSimulation(_use2DPressureSimulation);
                }
                float pressureSensitivity = brushSystem->getPressureSensitivity();
                if (ImGui::SliderFloat("Pressure Sensitivity", &pressureSensitivity, 0.01f, 5.0f, "%.2f")) {
                    zone.setPressureSensitivity(pressureSensitivity);
                }
                bool interpolate = brushSystem->getStrokeInterpolation();
                if (ImGui::Checkbox("Stroke Interpolation", &interpolate)) {
                    zone.setStrokeInterpolation(interpolate);
                }
            }

            if (ImGui::CollapsingHeader("Layers")) {
                bool useLayers = brushSystem->getUseLayers();
                if (ImGui::Checkbox("Use Layers", &useLayers)) {
                    zone.setUseLayers(useLayers);
                }
                int activeLayer = brushSystem->getActiveLayer();
                int layerMax = std::max(0, brushSystem->getLayerCount() - 1);
                if (ImGui::SliderInt("Active Layer", &activeLayer, 0, layerMax)) {
                    zone.setActiveLayer(activeLayer);
                }
                float layerOpacity = brushSystem->getLayerOpacity();
                if (ImGui::SliderFloat("Layer Opacity", &layerOpacity, 0.0f, 1.0f, "%.2f")) {
                    zone.setLayerOpacity(layerOpacity);
                }
                const char* blendModes[] = {"Normal", "Multiply", "Screen", "Overlay", "Add", "Subtract"};
                int blendMode = static_cast<int>(brushSystem->getBlendMode());
                if (ImGui::Combo("Blend Mode", &blendMode, blendModes, IM_ARRAYSIZE(blendModes))) {
                    zone.setBlendMode(static_cast<BrushSystem::BlendMode>(blendMode));
                }
                if (ImGui::Button("Add Layer")) {
                    zone.addLayer();
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete Layer")) {
                    zone.deleteLayer(brushSystem->getActiveLayer());
                }
            }
        }
    }

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Design Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
        int activeDesignLayer = zone.getActiveDesignLayer();
        int designLayerMax = std::max(0, zone.getDesignLayerCount() - 1);
        if (ImGui::SliderInt("Active Design Layer", &activeDesignLayer, 0, designLayerMax)) {
            zone.setActiveDesignLayer(activeDesignLayer);
        }
        float designOpacity = zone.getDesignLayerOpacity(activeDesignLayer);
        if (ImGui::SliderFloat("Design Opacity", &designOpacity, 0.0f, 1.0f, "%.2f")) {
            zone.setDesignLayerOpacity(activeDesignLayer, designOpacity);
        }
        if (ImGui::Button("Add Design Layer")) {
            zone.addDesignLayer();
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove Design Layer")) {
            zone.removeDesignLayer(activeDesignLayer);
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Undo")) {
        zone.undo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Redo")) {
        zone.redo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear History")) {
        zone.clearHistory();
    }
}

void Game::set3DMode(Mode3D mode) {
    _current3DMode = mode;
    if (mode == Mode3D::FacePaint) {
        _currentTool = Tool(Tool::Type::FacePaint);
    } else if (mode == Mode3D::FaceBrush) {
        _currentTool = Tool(Tool::Type::FaceBrush);
    }
}

void Game::render3DModeButton(Mode3D mode, const char* label) {
    const bool active = _current3DMode == mode;
    pushActiveButtonStyle(active, ImVec4(0.30f, 0.50f, 0.31f, 1.0f),
                          ImVec4(0.36f, 0.62f, 0.38f, 1.0f));
    const bool pressed = ImGui::Button(label, ImVec2(118.0f, 0.0f));
    popActiveButtonStyle(active);
    if (pressed) {
        set3DMode(mode);
    }
}

void Game::renderPrimitiveButton(Object::ShapeKind kind, const char* label) {
    const bool active = _polyhedron.shapeKind == kind;
    pushActiveButtonStyle(active, ImVec4(0.30f, 0.50f, 0.31f, 1.0f),
                          ImVec4(0.36f, 0.62f, 0.38f, 1.0f));
    const bool pressed = ImGui::Button(label, ImVec2(118.0f, 0.0f));
    popActiveButtonStyle(active);
    if (pressed) {
        _polyhedron.shapeKind = kind;
    }
}

void Game::render3DConsole() {
    struct Mode3DDef {
        Mode3D mode;
        const char* label;
    };

    static const Mode3DDef modeDefs[] = {
        {Mode3D::BrushCreate, "Create"},
        {Mode3D::Selection, "Select"},
        {Mode3D::FaceBrush, "Face Brush"},
        {Mode3D::FacePaint, "Face Fill"},
        {Mode3D::Pottery, "Pottery"},
        {Mode3D::Rotation, "Rotate"}
    };
    ImGui::TextUnformatted("Mode");
    for (int i = 0; i < IM_ARRAYSIZE(modeDefs); ++i) {
        render3DModeButton(modeDefs[i].mode, modeDefs[i].label);
        sameLineEvery(i, 3);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Target");
    int targetIdx = static_cast<int>(_current3DTarget);
    if (ImGui::RadioButton("World Objects", targetIdx == static_cast<int>(ToolTarget3D::WorldObjects))) {
        _current3DTarget = ToolTarget3D::WorldObjects;
        _selectedObject3D = nullptr;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Avatar Parts", targetIdx == static_cast<int>(ToolTarget3D::AvatarBodyParts))) {
        _current3DTarget = ToolTarget3D::AvatarBodyParts;
        _selectedObject3D = nullptr;
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Shape");
    // Grouped by fundamental topological category, not by named primitive.
    ImGui::TextDisabled("Polyhedra (flat-faced)");
    renderPrimitiveButton(Object::ShapeKind::Cube, "Cube"); ImGui::SameLine();
    renderPrimitiveButton(Object::ShapeKind::Polyhedron, "Polyhedron");
    ImGui::TextDisabled("Smooth surfaces");
    renderPrimitiveButton(Object::ShapeKind::Sphere, "Sphere"); ImGui::SameLine();
    renderPrimitiveButton(Object::ShapeKind::Ellipsoid, "Ellipsoid"); ImGui::SameLine();
    renderPrimitiveButton(Object::ShapeKind::Ovoid, "Ovoid");
    renderPrimitiveButton(Object::ShapeKind::Paraboloid, "Paraboloid"); ImGui::SameLine();
    renderPrimitiveButton(Object::ShapeKind::Torus, "Torus");
    ImGui::TextDisabled("Complex (round + edges)");
    renderPrimitiveButton(Object::ShapeKind::Cylinder, "Cylinder"); ImGui::SameLine();
    renderPrimitiveButton(Object::ShapeKind::Cone, "Cone"); ImGui::SameLine();
    renderPrimitiveButton(Object::ShapeKind::RoundedBox, "Rounded Box");

    ImGui::SliderFloat("Uniform Size", &_brush.size, 0.1f, 10.0f, "%.2f");
    ImGui::SliderFloat3("Scale", &_brush.scale.x, 0.1f, 8.0f, "%.2f");
    ImGui::SliderFloat3("Rotation", &_brush.rotation.x, -180.0f, 180.0f, "%.1f");
    ImGui::Checkbox("Grid Snap", &_brush.gridSnap);
    if (_brush.gridSnap) {
        ImGui::SliderFloat("Grid Size", &_brush.gridSize, 0.1f, 5.0f, "%.2f");
    }

    renderPlacementInspector();

    if (_polyhedron.shapeKind == Object::ShapeKind::Polyhedron) {
        ImGui::Separator();
        ImGui::TextUnformatted("Polyhedron");
        if (ImGui::Button("Tetrahedron")) {
            _polyhedron.currentType = 4;
        }
        ImGui::SameLine();
        if (ImGui::Button("Octahedron")) {
            _polyhedron.currentType = 8;
        }
        ImGui::SameLine();
        if (ImGui::Button("Dodecahedron")) {
            _polyhedron.currentType = 12;
        }
        ImGui::SameLine();
        if (ImGui::Button("Icosahedron")) {
            _polyhedron.currentType = 20;
        }
        ImGui::SliderInt("Faces", &_polyhedron.currentType, 4, 50);

        const char* concaveTypes[] = {"Regular", "Concave", "Star", "Crater"};
        ImGui::Combo("Variant", &_polyhedron.concaveType, concaveTypes, IM_ARRAYSIZE(concaveTypes));
        if (_polyhedron.concaveType == 1) {
            ImGui::SliderFloat("Concavity", &_polyhedron.concavityAmount, 0.1f, 0.8f, "%.2f");
        } else if (_polyhedron.concaveType == 2) {
            ImGui::SliderFloat("Spike Length", &_polyhedron.spikeLength, 0.1f, 1.0f, "%.2f");
        } else if (_polyhedron.concaveType == 3) {
            ImGui::SliderFloat("Crater Depth", &_polyhedron.craterDepth, 0.1f, 0.5f, "%.2f");
        }

        if (ImGui::CollapsingHeader("Custom Polyhedron")) {
            ImGui::Checkbox("Use Custom", &_polyhedron.useCustom);
            if (_polyhedron.useCustom) {
                bool changed = false;
                changed |= ImGui::SliderInt("Vertices", &_polyhedron.customVertexCount, 3, 20);
                changed |= ImGui::SliderInt("Custom Faces", &_polyhedron.customFaceCount, 3, 20);
                if (changed || ImGui::Button("Regenerate")) {
                    _polyhedron.generateCustom();
                }
            }
        }
    }

    if (_current3DMode == Mode3D::FaceBrush) {
        ImGui::Separator();
        ImGui::TextUnformatted("Face Brush");
        const char* brushTypeNames[] = {"Normal", "Airbrush", "Chalk", "Spray", "Smudge", "Clone"};
        int brushTypeIdx = static_cast<int>(_brush.type);
        if (ImGui::Combo("Brush Type##3d", &brushTypeIdx, brushTypeNames, IM_ARRAYSIZE(brushTypeNames))) {
            _brush.type = static_cast<BrushType>(brushTypeIdx);
        }
        ImGui::SliderFloat("Brush Radius", &_faceBrush.radius, 0.01f, 2.0f, "%.2f");
        ImGui::SliderFloat("Softness", &_faceBrush.softness, 0.0f, 2.0f, "%.2f");
        ImGui::SliderFloat("Opacity", &_brush.opacity, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Flow", &_brush.flow, 0.0f, 1.0f, "%.2f");
        ImGui::Checkbox("Stroke Interpolation", &_brush.useStrokeInterpolation);
        ImGui::Checkbox("Show Brush Cursor", &_brush.showCursor);
        ImGui::Checkbox("Show Preview", &_brush.showPreview);

        if (ImGui::CollapsingHeader("UV Mapping")) {
            ImGui::SliderFloat("U Offset", &_faceBrush.uOffset, -2.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("V Offset", &_faceBrush.vOffset, -2.0f, 2.0f, "%.2f");
            const char* axisNames[] = {"X", "Y", "Z"};
            ImGui::Combo("Axis 1", &_faceBrush.uAxis, axisNames, IM_ARRAYSIZE(axisNames));
            ImGui::Combo("Axis 2", &_faceBrush.vAxis, axisNames, IM_ARRAYSIZE(axisNames));
            ImGui::Checkbox("Invert Axis 1", &_faceBrush.invertU);
            ImGui::SameLine();
            ImGui::Checkbox("Invert Axis 2", &_faceBrush.invertV);
        }
    }

    if (_current3DMode == Mode3D::FacePaint) {
        ImGui::Separator();
        ImGui::Checkbox("Advanced Face Paint", &_advancedFacePaint.enabled);
        if (_advancedFacePaint.enabled) {
            AdvancedFacePaint::initializeAdvancedPainter();
            ImGui::ColorEdit4("Gradient Start", &_advancedFacePaint.gradient.startColor.x);
            ImGui::ColorEdit4("Gradient End", &_advancedFacePaint.gradient.endColor.x);
            ImGui::SliderFloat("Gradient Angle", &_advancedFacePaint.gradient.angle, 0.0f, 360.0f, "%.1f");
            ImGui::SliderFloat("Smudge Strength", &_advancedFacePaint.smudge.strength, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Smudge Radius", &_advancedFacePaint.smudge.radius, 0.01f, 1.0f, "%.2f");
        }
    }

    if (_current3DMode == Mode3D::Pottery) {
        ImGui::Separator();
        bool chisel = _pottery.currentTool == PotteryTool::Chisel;
        if (ImGui::RadioButton("Chisel", chisel)) {
            _pottery.currentTool = PotteryTool::Chisel;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Expand", !chisel)) {
            _pottery.currentTool = PotteryTool::Expand;
        }
        ImGui::SliderFloat("Strength", &_pottery.strength, 0.01f, 2.0f, "%.2f");
    }

    if (_current3DMode == Mode3D::Rotation) {
        ImGui::Separator();
        const char* axisModeNames[] = {"Free XY", "X", "Y", "Z", "Authoritative Axis"};
        int axisModeIdx = static_cast<int>(_rotation.axisMode);
        if (ImGui::Combo("Axis Mode", &axisModeIdx, axisModeNames, IM_ARRAYSIZE(axisModeNames))) {
            _rotation.axisMode = static_cast<RotationAxisMode>(axisModeIdx);
        }
        ImGui::SliderFloat("Sensitivity", &_rotation.sensitivity, 0.05f, 2.0f, "%.2f");
        ImGui::SliderFloat("Smoothness", &_rotation.smoothness, 1.0f, 20.0f, "%.2f");
    }

    renderSelectionInspector();
}

void Game::renderCharacterConsole() {
    Body& body = _player.getBody();

    if (_current3DTarget == ToolTarget3D::AvatarBodyParts) {
        if (auto* pickedPart = dynamic_cast<BodyPart*>(_selectedObject3D)) {
            _selectedCharacterPart = pickedPart;
        }
    }
    if (!_selectedCharacterPart && !body.parts.empty()) {
        _selectedCharacterPart = body.parts.front();
    }

    if (ImGui::Button("Edit Avatar Parts")) {
        _current3DTarget = ToolTarget3D::AvatarBodyParts;
        _current3DMode = Mode3D::Selection;
        _currentPerspective = PerspectiveMode::ThirdPerson;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Design Lock", &_characterDesignLocked);

    if (ImGui::BeginTabBar("CharacterTabs")) {
        if (ImGui::BeginTabItem("Body Parts")) {
            ImGui::TextUnformatted("Body Parts");
            for (auto* part : body.parts) {
                if (!part) continue;
                const bool selected = part == _selectedCharacterPart;
                if (ImGui::Selectable(part->getName().c_str(), selected)) {
                    _selectedCharacterPart = part;
                    _selectedObject3D = part;
                    _current3DTarget = ToolTarget3D::AvatarBodyParts;
                }
            }

            if (_selectedCharacterPart) {
                ImGui::Separator();
                ImGui::BeginDisabled(_characterDesignLocked);

                ImGui::Text("Editing: %s", _selectedCharacterPart->getName().c_str());

                const char* shapeNames[] = {"Cube", "Sphere", "Cylinder", "Cone", "Polyhedron"};
                int currentShape = static_cast<int>(_selectedCharacterPart->getPrimaryShape());
                if (ImGui::Combo("Shape", &currentShape, shapeNames, IM_ARRAYSIZE(shapeNames))) {
                    _selectedCharacterPart->setPrimaryShape(static_cast<Object::GeometryType>(currentShape));
                }

                glm::vec3 dims = _selectedCharacterPart->getGeometry().getDimensions();
                float dimArr[3] = {dims.x, dims.y, dims.z};
                if (ImGui::SliderFloat3("Dimensions", dimArr, 0.05f, 1.0f, "%.2f")) {
                    _selectedCharacterPart->getGeometry().setDimensions({dimArr[0], dimArr[1], dimArr[2]});
                    _selectedCharacterPart->setTransform(_selectedCharacterPart->getTransform());
                }

                float color[3] = {
                    _selectedCharacterPart->getColor()[0],
                    _selectedCharacterPart->getColor()[1],
                    _selectedCharacterPart->getColor()[2]
                };
                if (ImGui::ColorEdit3("Color", color)) {
                    _selectedCharacterPart->setColor(color[0], color[1], color[2]);
                }

                ImGui::Separator();
                ImGui::Text("Sub-Objects (%zu)", _selectedCharacterPart->getSubObjectCount());
                for (size_t index = 0; index < _selectedCharacterPart->getSubObjectCount(); ++index) {
                    Object* sub = _selectedCharacterPart->getSubObject(index);
                    if (!sub) continue;

                    ImGui::PushID(static_cast<int>(index));
                    int subShape = static_cast<int>(sub->getGeometryType());
                    ImGui::SetNextItemWidth(104.0f);
                    if (ImGui::Combo("##SubShape", &subShape, shapeNames, IM_ARRAYSIZE(shapeNames))) {
                        sub->setGeometryType(static_cast<Object::GeometryType>(subShape));
                    }
                    ImGui::SameLine();

                    glm::mat4 localOffset = _selectedCharacterPart->getSubObjectLocalOffset(index);
                    float offset[3] = {localOffset[3][0], localOffset[3][1], localOffset[3][2]};
                    ImGui::SetNextItemWidth(210.0f);
                    if (ImGui::SliderFloat3("Offset", offset, -1.0f, 1.0f, "%.2f")) {
                        localOffset[3][0] = offset[0];
                        localOffset[3][1] = offset[1];
                        localOffset[3][2] = offset[2];
                        _selectedCharacterPart->setSubObjectLocalOffset(index, localOffset);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("X")) {
                        _selectedCharacterPart->removeSubObject(index);
                        ImGui::PopID();
                        break;
                    }
                    ImGui::PopID();
                }

                if (ImGui::Button("Add Sub-Object")) {
                    _selectedCharacterPart->addSubObject(Object::GeometryType::Cube, glm::mat4(1.0f));
                }

                ImGui::Separator();
                ImGui::Text("Health: %.1f/%.1f", _selectedCharacterPart->getHealth(), _selectedCharacterPart->getMaxHealth());
                if (ImGui::Button("Heal Part")) {
                    _selectedCharacterPart->heal(20.0f);
                }
                ImGui::SameLine();
                if (ImGui::Button("Damage Part")) {
                    _selectedCharacterPart->takeDamage(10.0f);
                }

                ImGui::EndDisabled();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Avatar Stats")) {
            ImGui::BeginDisabled(_characterDesignLocked);

            ImGui::Text("Health: %.1f/%.1f", _player.state.health, _player.state.maxHealth);
            ImGui::Text("Energy: %.1f/%.1f", _player.state.energy, _player.state.maxEnergy);
            ImGui::Text("Mood: %.1f", _player.state.mood);
            ImGui::Text("Level: %d (XP: %.1f)", _player.state.level, _player.state.experience);

            ImGui::Separator();
            ImGui::TextUnformatted("Skills");
            for (const auto& skill : _player.state.skills) {
                ImGui::Text("%s: %.1f", skill.first.c_str(), skill.second);
            }

            ImGui::Separator();
            if (ImGui::Button("Add Experience")) {
                _player.addExperience(50.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Heal Avatar")) {
                _player.modifyHealth(50.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Restore Energy")) {
                _player.modifyEnergy(50.0f);
            }

            ImGui::EndDisabled();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Appearance")) {
            ImGui::BeginDisabled(_characterDesignLocked);

            char hairStyle[64];
            std::snprintf(hairStyle, sizeof(hairStyle), "%s", _player.state.hairStyle.c_str());
            if (ImGui::InputText("Hair Style", hairStyle, sizeof(hairStyle))) {
                _player.setHairStyle(hairStyle);
            }

            char eyeColor[32];
            std::snprintf(eyeColor, sizeof(eyeColor), "%s", _player.state.eyeColor.c_str());
            if (ImGui::InputText("Eye Color", eyeColor, sizeof(eyeColor))) {
                _player.setEyeColor(eyeColor);
            }

            char skinTone[32];
            std::snprintf(skinTone, sizeof(skinTone), "%s", _player.state.skinTone.c_str());
            if (ImGui::InputText("Skin Tone", skinTone, sizeof(skinTone))) {
                _player.setSkinTone(skinTone);
            }

            float height = body.height;
            if (ImGui::SliderFloat("Height", &height, 0.5f, 2.5f, "%.2f m")) {
                body.setHeight(height);
                _player.state.height = height;
                _player.updatePose();
            }

            float weight = body.weight;
            if (ImGui::SliderFloat("Weight", &weight, 30.0f, 150.0f, "%.1f kg")) {
                body.setWeight(weight);
                _player.state.weight = weight;
            }

            int proportions = static_cast<int>(body.proportions);
            const char* proportionNames[] = {"Child", "Teen", "Adult", "Elder"};
            if (ImGui::Combo("Proportions", &proportions, proportionNames, IM_ARRAYSIZE(proportionNames))) {
                body.setProportions(static_cast<Body::Proportions>(proportions));
                _player.updatePose();
            }

            ImGui::EndDisabled();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Clothing")) {
            ImGui::BeginDisabled(_characterDesignLocked);

            ImGui::TextUnformatted("Equipped Clothing");
            for (auto& item : body.clothing) {
                bool equipped = item.second.isEquipped;
                if (ImGui::Checkbox(item.first.c_str(), &equipped)) {
                    if (equipped) {
                        body.equipClothing(item.first);
                    } else {
                        body.unequipClothing(item.first);
                    }
                }
                if (equipped) {
                    ImGui::SameLine();
                    ImGui::Text("(Protection: %.1f, Warmth: %.1f)",
                                item.second.protection, item.second.warmth);
                }
            }

            ImGui::Separator();
            ImGui::Text("Total Protection: %.1f", body.getTotalProtection());
            ImGui::Text("Total Warmth: %.1f", body.getTotalWarmth());

            ImGui::EndDisabled();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Inventory")) {
            ImGui::BeginDisabled(_characterDesignLocked);

            ImGui::Text("Inventory (%zu/%d items)", _player.inventory.size(), _player.maxInventorySize);
            for (size_t i = 0; i < _player.inventory.size(); ++i) {
                ImGui::Text("%zu. %s", i + 1, _player.inventory[i].c_str());
            }

            ImGui::Separator();
            static char newItem[64] = "";
            if (ImGui::InputText("Add Item", newItem, sizeof(newItem), ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (_player.addToInventory(newItem)) {
                    newItem[0] = '\0';
                }
            }

            ImGui::EndDisabled();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Presets")) {
            ImGui::BeginDisabled(_characterDesignLocked);

            static char presetName[64] = "";
            ImGui::InputText("Preset Name", presetName, sizeof(presetName));
            if (ImGui::Button("Create Preset") && presetName[0] != '\0') {
                _avatarManager.createPreset(presetName, &_player);
                presetName[0] = '\0';
            }
            ImGui::SameLine();
            if (ImGui::Button("Create Current")) {
                _avatarManager.createPreset("Current", &_player);
            }

            ImGui::EndDisabled();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void Game::renderPlacementInspector() {
    ImGui::Separator();
    int placeIdx = static_cast<int>(_placement.mode);
    const char* placeNames[] = {"In Front", "Manual Distance", "Cursor Snap"};
    if (ImGui::Combo("Placement", &placeIdx, placeNames, IM_ARRAYSIZE(placeNames))) {
        _placement.mode = static_cast<BrushPlacementMode>(placeIdx);
    }
    if (_placement.mode == BrushPlacementMode::ManualDistance &&
        _placement.prevMode != BrushPlacementMode::ManualDistance) {
        _placement.anchorPos = _camera.pos + _camera.front * 2.0f;
        _placement.anchorRight = glm::normalize(glm::cross(_camera.front, _camera.up));
        _placement.anchorUp = _camera.up;
        _placement.anchorForward = _camera.front;
        _placement.anchorValid = true;
    }
    _placement.prevMode = _placement.mode;
    if (_placement.mode == BrushPlacementMode::ManualDistance) {
        ImGui::SliderFloat3("Offset", &_placement.manualOffset.x, -20.0f, 20.0f, "%.2f");
    }
}

void Game::renderSelectionInspector() {
    ImGui::Separator();
    ImGui::TextUnformatted("Selection");
    if (_selectedObject3D) {
        ImGui::TextWrapped("%s", _selectedObject3D->getIdentifier().c_str());
        glm::vec3 center = _selectedObject3D->getCenter();
        if (ImGui::DragFloat3("Center", &center.x, 0.01f, -100.0f, 100.0f, "%.2f")) {
            _selectedObject3D->setCenter(center);
        }
        glm::vec3 targetRotation = _selectedObject3D->getTargetRotationEulerDegrees();
        if (ImGui::DragFloat3("Target Rotation", &targetRotation.x, 0.5f, -720.0f, 720.0f, "%.1f")) {
            _selectedObject3D->setTargetRotationEulerDegrees(targetRotation);
        }
        if (ImGui::Button("Snap Rotation")) {
            _selectedObject3D->setRotationEulerDegrees(_selectedObject3D->getTargetRotationEulerDegrees());
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Selection")) {
            _selectedObject3D = nullptr;
        }
    } else {
        ImGui::TextDisabled("No object selected.");
    }
}

void Game::renderWorldConsole() {
    _world.renderModeUI();

    ImGui::Separator();
    ImGui::TextUnformatted("Cursor Picking");
    if (ImGui::Button("Open Cursor Tools")) {
        _cursorToolsOpen = true;
    }
    if (_cursorToolsOpen) {
        _cursorTools.renderUI(_cursorToolsOpen);
    }
}

void Game::renderAssetsConsole(Zone& zone) {
    if (ImGui::Button("Quick Save")) {
        saveStateWithLog();
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As")) {
        _saveLoad.showSaveWindow = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        updateSaveFiles();
        _saveLoad.showLoadWindow = true;
    }
    if (ImGui::Button("Save Manager")) {
        _saveLoad.showManager = true;
    }

    ImGui::Separator();
    ImGui::Text("Zone: %s", zone.name().c_str());
    ImGui::Text("Objects: %d", static_cast<int>(zone.world().getOwnedObjects().size()));
    ImGui::Text("Tool: %s", _currentTool.getTypeName().c_str());
}

void Game::renderRelationsConsole(Zone& zone) {
    const auto& objs = zone.world().getOwnedObjects();

    ImGui::TextUnformatted("Object Bonds");
    static int objAIdx = 0;
    static int objBIdx = 1;

    std::vector<std::string> labels;
    labels.reserve(objs.size());
    for (size_t i = 0; i < objs.size(); ++i) {
        std::string label = "Obj " + std::to_string(i);
        if (objs[i]) {
            label += " - " + objs[i]->getIdentifier();
        }
        labels.push_back(label);
    }

    std::vector<const char*> labelPtrs;
    labelPtrs.reserve(labels.size());
    for (const auto& label : labels) {
        labelPtrs.push_back(label.c_str());
    }

    if (!labelPtrs.empty()) {
        objAIdx = std::clamp(objAIdx, 0, static_cast<int>(labelPtrs.size()) - 1);
        objBIdx = std::clamp(objBIdx, 0, static_cast<int>(labelPtrs.size()) - 1);
        ImGui::Combo("Object A", &objAIdx, labelPtrs.data(), static_cast<int>(labelPtrs.size()));
        ImGui::Combo("Object B", &objBIdx, labelPtrs.data(), static_cast<int>(labelPtrs.size()));
        if (ImGui::Button("Create Bond") && objAIdx != objBIdx) {
            Physics::addBond(objs[objAIdx].get(), objs[objBIdx].get());
        }
        ImGui::SameLine();
        if (ImGui::Button("Create Attachment") && objAIdx != objBIdx) {
            Object* parent = objs[objAIdx].get();
            Object* child = objs[objBIdx].get();
            if (parent && child) {
                zone.syncFormationMembers({parent, child});
                auto relation = std::make_shared<Relation>("attachment", *parent, *child, true, 1.0f);
                relation->attachment.enabled = true;
                relation->attachment.localOffset = glm::inverse(parent->getTransform()) * child->getTransform();
                relation->attachment.parentAnchor = parent->getCenter();
                relation->attachment.childAnchor = child->getCenter();
                zone.formation().addRelation(relation);
            }
        }
    } else {
        ImGui::TextDisabled("No objects available.");
    }

    ImGui::Separator();
    const auto& bonds = Physics::getBonds();
    ImGui::Text("Bonds: %d", static_cast<int>(bonds.size()));
    static int selectedBond = -1;
    if (ImGui::BeginListBox("##BondList", ImVec2(-1.0f, 120.0f))) {
        for (int i = 0; i < static_cast<int>(bonds.size()); ++i) {
            char label[96];
            std::snprintf(label, sizeof(label), "Bond %d", i);
            if (ImGui::Selectable(label, selectedBond == i)) {
                selectedBond = i;
            }
        }
        ImGui::EndListBox();
    }

    if (selectedBond >= 0 && selectedBond < static_cast<int>(bonds.size())) {
        const auto& bond = bonds[selectedBond];
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

    ImGui::Separator();
    if (ImGui::Button("Open Relation Manager")) {
        _showRelationManager = true;
    }
}

void Game::renderCreatorStatusBar() {
    ImGui::Separator();
    const char* modeLabel = "2D";
    if (_current3DMode == Mode3D::BrushCreate) modeLabel = "Create";
    if (_current3DMode == Mode3D::Selection) modeLabel = "Select";
    if (_current3DMode == Mode3D::FaceBrush) modeLabel = "Face Brush";
    if (_current3DMode == Mode3D::FacePaint) modeLabel = "Face Fill";
    if (_current3DMode == Mode3D::Pottery) modeLabel = "Pottery";
    if (_current3DMode == Mode3D::Rotation) modeLabel = "Rotate";

    ImGui::Text("Mode: %s", modeLabel);
    ImGui::SameLine();
    ImGui::Text("Tool: %s", _currentTool.getTypeName().c_str());
    ImGui::SameLine();
    ImGui::Text("Physics: %s", _world.isPhysicsEnabled() ? "On" : "Off");
}

} // namespace Core
