// GameToolbar.cpp - unified Creator Console for creation, editing, and world tools.

#include "Game.hpp"
#include "Form/Object/Object.hpp"
#include "Core/SdfBuild.hpp"
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

// shapeToSdfLeaf / objectToSdfNode now live in Core/SdfBuild.hpp (shared with the
// in-scene Combine tool). Included below at file scope.

struct ShapeKindDef { Object::ShapeKind k; const char* label; };

void shapeKindCombo(const char* label, int& idx, const ShapeKindDef* kinds, int count) {
    if (idx < 0 || idx >= count) idx = 0;
    if (ImGui::BeginCombo(label, kinds[idx].label)) {
        for (int i = 0; i < count; ++i) {
            bool sel = (i == idx);
            if (ImGui::Selectable(kinds[i].label, sel)) idx = i;
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
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
    renderSectionButton(CreatorSection::Character, "Character");
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
    _combineOperandA = nullptr; // drop any pending Combine operand on mode switch
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
        {Mode3D::Rotation, "Rotate"},
        {Mode3D::Morph, "Morph"},
        {Mode3D::Combine, "Combine"}
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

    // Per-shape parameters for the selected topology shape.
    {
        Object::ShapeParams& sp = _polyhedron.shapeParams;
        switch (_polyhedron.shapeKind) {
            case Object::ShapeKind::Sphere:
                ImGui::SliderFloat("Radius", &sp.r, 0.05f, 2.0f, "%.2f");
                break;
            case Object::ShapeKind::Ellipsoid:
                ImGui::SliderFloat("Semi-axis X", &sp.r,  0.05f, 2.0f, "%.2f");
                ImGui::SliderFloat("Semi-axis Y", &sp.ry, 0.05f, 2.0f, "%.2f");
                ImGui::SliderFloat("Semi-axis Z", &sp.rz, 0.05f, 2.0f, "%.2f");
                break;
            case Object::ShapeKind::Ovoid:
                ImGui::SliderFloat("Radius", &sp.r, 0.05f, 2.0f, "%.2f");
                ImGui::SliderFloat("Asymmetry", &sp.ovoidAsym, 0.0f, 0.9f, "%.2f");
                break;
            case Object::ShapeKind::Paraboloid:
                ImGui::SliderFloat("Steepness", &sp.paraboloidA, 0.5f, 6.0f, "%.2f");
                break;
            case Object::ShapeKind::Torus:
                ImGui::SliderFloat("Major Radius", &sp.majorR, 0.1f, 1.0f, "%.2f");
                ImGui::SliderFloat("Minor Radius", &sp.minorR, 0.02f, 0.5f, "%.2f");
                break;
            case Object::ShapeKind::Cylinder:
            case Object::ShapeKind::Cone:
                ImGui::SliderFloat("Radius", &sp.r, 0.05f, 2.0f, "%.2f");
                ImGui::SliderFloat("Half-height", &sp.halfH, 0.05f, 2.0f, "%.2f");
                break;
            case Object::ShapeKind::RoundedBox:
                ImGui::SliderFloat("Fillet", &sp.fillet, 0.01f, 0.49f, "%.2f");
                break;
            default: break;
        }
    }

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
        auto selectRegularPolyhedron = [&](int faces) {
            _polyhedron.currentType = faces;
            _polyhedron.irregularType = 0;
            _polyhedron.useCustom = false;
        };
        if (ImGui::Button("Tetrahedron")) {
            selectRegularPolyhedron(4);
        }
        ImGui::SameLine();
        if (ImGui::Button("Octahedron")) {
            selectRegularPolyhedron(8);
        }
        ImGui::SameLine();
        if (ImGui::Button("Dodecahedron")) {
            selectRegularPolyhedron(12);
        }
        ImGui::SameLine();
        if (ImGui::Button("Icosahedron")) {
            selectRegularPolyhedron(20);
        }
        if (ImGui::SliderInt("Faces", &_polyhedron.currentType, 4, 50)) {
            _polyhedron.irregularType = 0;
            _polyhedron.useCustom = false;
        }

        if (ImGui::CollapsingHeader("Irregular Polyhedron")) {
            const char* irregularTypes[] = {"None", "Prism", "Antiprism", "Pyramid", "Bipyramid", "Frustum"};
            if (ImGui::Combo("Irregular Type", &_polyhedron.irregularType, irregularTypes, IM_ARRAYSIZE(irregularTypes))) {
                if (_polyhedron.irregularType > 0) {
                    _polyhedron.useCustom = false;
                    _polyhedron.concaveType = 0;
                }
            }
            if (_polyhedron.irregularType > 0) {
                ImGui::SliderInt("Base Sides", &_polyhedron.irregularBaseSides, 3, 24);
                ImGui::SliderFloat("Height", &_polyhedron.irregularHeight, 0.1f, 3.0f, "%.2f");
                if (_polyhedron.irregularType == 5) {
                    ImGui::SliderFloat("Top Scale", &_polyhedron.frustumTopScale, 0.05f, 2.0f, "%.2f");
                }
            }
        }

        if (!_polyhedron.useCustom && _polyhedron.irregularType == 0) {
            const char* concaveTypes[] = {"Regular", "Concave", "Star", "Crater"};
            if (ImGui::Combo("Variant", &_polyhedron.concaveType, concaveTypes, IM_ARRAYSIZE(concaveTypes)) &&
                _polyhedron.concaveType > 0) {
                _polyhedron.useCustom = false;
                _polyhedron.irregularType = 0;
            }
            if (_polyhedron.concaveType == 1) {
                ImGui::SliderFloat("Concavity", &_polyhedron.concavityAmount, 0.1f, 0.8f, "%.2f");
            } else if (_polyhedron.concaveType == 2) {
                ImGui::SliderFloat("Spike Length", &_polyhedron.spikeLength, 0.1f, 1.0f, "%.2f");
            } else if (_polyhedron.concaveType == 3) {
                ImGui::SliderFloat("Crater Depth", &_polyhedron.craterDepth, 0.1f, 0.5f, "%.2f");
            }
        }

        if (ImGui::CollapsingHeader("Custom Polyhedron")) {
            bool useCustom = _polyhedron.useCustom;
            if (ImGui::Checkbox("Use Custom", &useCustom)) {
                _polyhedron.useCustom = useCustom;
                if (_polyhedron.useCustom) {
                    _polyhedron.irregularType = 0;
                    _polyhedron.concaveType = 0;
                    if (_polyhedron.customVertices.empty()) {
                        _polyhedron.generateCustom();
                    }
                }
            }
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

    // --- Combine (in-scene boolean / blend) ------------------------------
    // Pick the verb here, then do the nouns IN THE SCENE: click shape A, then
    // shape B. The result (A op B) replaces A in place and B is absorbed into it.
    // This is the embodied replacement for the A/B dropdown + Create flow below.
    if (_current3DMode == Mode3D::Combine) {
        ImGui::Separator();
        ImGui::TextUnformatted("Combine  (click A, then B)");
        const char* ops[] = { "Union  (A + B)", "Intersect  (A & B)", "Subtract  (A - B)",
                              "Smooth Union", "Blend  (A <-> B)" };
        ImGui::TextUnformatted("Operation");
        for (int i = 0; i < 5; ++i) {
            bool sel = (_combineOp == i);
            pushActiveButtonStyle(sel, ImVec4(0.20f, 0.55f, 0.95f, 1.0f),
                                       ImVec4(0.30f, 0.65f, 1.00f, 1.0f));
            if (ImGui::Button(ops[i])) _combineOp = i;
            popActiveButtonStyle(sel);
            if (i != 2 && i != 4) ImGui::SameLine();
        }
        if (_combineOp == 3 || _combineOp == 4)
            ImGui::SliderFloat(_combineOp == 3 ? "Smoothness" : "Blend t",
                               &_combineBlend, 0.0f, 1.0f, "%.2f");
        ImGui::Separator();
        if (!_combineOperandA)
            ImGui::TextColored(ImVec4(0.6f,0.9f,1.0f,1.0f), "Click shape A in the scene.");
        else {
            ImGui::TextColored(ImVec4(1.0f,0.85f,0.2f,1.0f), "A: %s",
                               _combineOperandA->getIdentifier().c_str());
            ImGui::TextUnformatted("Now click shape B  (right-click cancels).");
        }
        ImGui::TextDisabled("B is absorbed into A; drag it later in Morph mode.");
    }

    // --- Morph (topology) ------------------------------------------------
    // Direct + math editing of a selected polyhedron's vertices. Drag handles in
    // the viewport (Morph mode), or type exact coordinates here. Two views of
    // one model.
    if (_current3DMode == Mode3D::Morph) {
        ImGui::Separator();
        ImGui::TextUnformatted("Morph (topology)");
        Object* o = _selectedObject3D;
        if (!o) {
            ImGui::TextDisabled("Select a shape (Select mode) first.");
        } else if (o->isBinaryField()) {
            ImGui::TextDisabled("Drag the gold handle in the viewport to move operand B.");
            glm::vec3 off = o->getFieldOperandBOffset();
            if (ImGui::DragFloat3("Operand B pos", &off.x, 0.01f, -3.0f, 3.0f, "%.3f"))
                o->setFieldOperandBOffset(off);
            if (o->isMorphField()) {
                float t = o->getMorphParam();
                if (ImGui::SliderFloat("Blend t", &t, 0.0f, 1.0f, "%.2f")) o->setMorphParam(t);
            }
        } else if (o->isPatch()) {
            ImGui::Text("Control net %dx%d (degree %d,%d)",
                        o->getPatchDegreeU() + 1, o->getPatchDegreeV() + 1,
                        o->getPatchDegreeU(), o->getPatchDegreeV());
            ImGui::TextDisabled("Drag the control handles in the viewport.");
            if (ImGui::Button("Raise degree U")) o->elevatePatchU();
            ImGui::SameLine();
            if (ImGui::Button("Raise degree V")) o->elevatePatchV();
            if (_patchCtrlIndex >= 0 && _patchCtrlIndex < o->getPatchControlCount()) {
                glm::vec3 c = o->getPatchControlLocal(_patchCtrlIndex);
                if (ImGui::DragFloat3("Control point", &c.x, 0.01f, -5.0f, 5.0f, "%.3f"))
                    o->setPatchControlLocal(_patchCtrlIndex, c);
            }
            // The same coin, algebra side: the surface's polynomial coefficients.
            // Editing these rebuilds the control net; dragging handles updates these.
            if (ImGui::CollapsingHeader("Polynomial coefficients (u^k v^l)")) {
                ImGui::TextDisabled("S(u,v) = sum  a(k,l) * u^k * v^l  (geometry <-> algebra)");
                const geom::BezierPatch& p = o->getPatchData();
                std::vector<glm::vec3> coeff = geom::patchToMonomial(p);
                int nu = p.nu();
                int editedK = -1, editedL = -1; glm::vec3 editedVal(0.0f);
                for (int l = 0; l < p.nv(); ++l)
                    for (int k = 0; k < p.nu(); ++k) {
                        glm::vec3 c = coeff[l * nu + k];
                        char label[40];
                        std::snprintf(label, sizeof(label), "a(u^%d v^%d)##coef", k, l);
                        if (ImGui::DragFloat3(label, &c.x, 0.005f, -20.0f, 20.0f, "%.3f")) {
                            editedK = k; editedL = l; editedVal = c;
                        }
                    }
                if (editedK >= 0) {
                    coeff[editedL * nu + editedK] = editedVal;
                    o->setBezierPatch(geom::monomialToPatch(coeff, p.du, p.dv));
                }
            }
        } else if (o->getGeometryType() != Object::GeometryType::Polyhedron) {
            ImGui::TextDisabled("Vertex editing is for polyhedra (for now).");
        } else {
            ImGui::Text("Vertices: %d   Selected: %d",
                        o->getPolyhedronVertexCount(), _morphVertexIndex);
            ImGui::TextDisabled("Drag a handle in the viewport, or type below.");
            if (_morphVertexIndex >= 0 && _morphVertexIndex < o->getPolyhedronVertexCount()) {
                glm::vec3 v = o->getPolyhedronVertexLocal(_morphVertexIndex);
                if (ImGui::DragFloat3("Vertex (local)", &v.x, 0.01f, -5.0f, 5.0f, "%.3f")) {
                    o->setPolyhedronVertexLocal(_morphVertexIndex, v);
                }
                // Math readouts: incident edge lengths from the polyhedron faces.
                const auto& pd = o->getPolyhedronData();
                float minE = 1e9f, maxE = 0.0f; int count = 0;
                for (const auto& face : pd.faces) {
                    for (size_t k = 0; k < face.size(); ++k) {
                        int a = face[k], b = face[(k + 1) % face.size()];
                        if (a == _morphVertexIndex || b == _morphVertexIndex) {
                            float len = glm::length(pd.vertices[a] - pd.vertices[b]);
                            minE = std::min(minE, len); maxE = std::max(maxE, len); ++count;
                        }
                    }
                }
                if (count > 0) ImGui::Text("Incident edge length: %.3f - %.3f", minE, maxE);
            }
        }
    }

    // --- Blend (A -> B) --------------------------------------------------
    // Binary: interpolate EXACTLY two shapes (lerp of their signed-distance
    // fields). A is the selected shape or a template; B is a template. If A is
    // the selected shape the result replaces it; otherwise a new shape is made.
    {
        ImGui::Separator();
        ImGui::TextUnformatted("Blend  (A -> B)");
        ImGui::TextDisabled("Interpolates two shapes. A and B are the only inputs.");
        static const ShapeKindDef kinds[] = {
            {Object::ShapeKind::Sphere, "Sphere"},   {Object::ShapeKind::Cube, "Box"},
            {Object::ShapeKind::Ellipsoid, "Ellipsoid"}, {Object::ShapeKind::RoundedBox, "Rounded Box"},
            {Object::ShapeKind::Cylinder, "Cylinder"}, {Object::ShapeKind::Cone, "Cone"},
            {Object::ShapeKind::Torus, "Torus"},     {Object::ShapeKind::Ovoid, "Ovoid"},
        };
        const int kindCount = static_cast<int>(sizeof(kinds) / sizeof(kinds[0]));
        static int aIdx = 0, bIdx = 1;
        static float blendT = 0.5f;
        static int aSource = 0; // 0 = selected shape, 1 = template
        const bool haveSel = (_selectedObject3D != nullptr);

        // Operand A — stable labels + int* overload so clicks register reliably.
        ImGui::RadioButton("A = Selected", &aSource, 0);
        ImGui::SameLine();
        ImGui::RadioButton("A = Template", &aSource, 1);
        if (!haveSel) aSource = 1; // no selection → fall back to template
        if (aSource == 0 && haveSel) ImGui::Text("   A: %s", _selectedObject3D->getIdentifier().c_str());
        else {
            if (!haveSel) ImGui::TextDisabled("   (select a shape to use 'A = Selected')");
            shapeKindCombo("A shape", aIdx, kinds, kindCount);
        }

        // Operand B
        shapeKindCombo("B shape", bIdx, kinds, kindCount);
        ImGui::SliderFloat("A  <->  B", &blendT, 0.0f, 1.0f, "%.2f");

        const bool replace = (aSource == 0 && haveSel);
        if (ImGui::Button(replace ? "Blend (replace selected)" : "Blend (create new)")) {
            geom::SdfNode a = replace ? objectToSdfNode(*_selectedObject3D)
                                      : shapeToSdfLeaf(kinds[aIdx].k, Object::ShapeParams{});
            geom::SdfNode b = shapeToSdfLeaf(kinds[bIdx].k, Object::ShapeParams{});
            geom::SdfNode node = geom::SdfNode::binary(geom::SdfOp::Morph, a, b, blendT);
            if (replace) {
                _selectedObject3D->setFieldShape(node, 1.4f);
            } else {
                auto obj = std::make_unique<Object>();
                obj->setFieldShape(node, 1.3f);
                glm::mat4 t = glm::translate(glm::mat4(1.0f), _camera.pos + _camera.front * 3.0f);
                t = glm::scale(t, _brush.scale * _brush.size);
                obj->setTransform(t);
                obj->updateCollisionZone(t);
                for (int f = 0; f < obj->getFaces(); ++f)
                    obj->setFaceColor(f, _currentColor[0], _currentColor[1], _currentColor[2]);
                mgr.active().world().addObject(std::move(obj));
            }
        }
        if (_selectedObject3D && _selectedObject3D->isMorphField()) {
            float t = _selectedObject3D->getMorphParam();
            if (ImGui::SliderFloat("Edit blend (selected)", &t, 0.0f, 1.0f, "%.2f"))
                _selectedObject3D->setMorphParam(t);
        }
    }

    // --- Boolean (A op B) ------------------------------------------------
    // Binary: combine EXACTLY two shapes by a set operation. A is the selected
    // shape or a template; B is a template placed by its offset. A = selected →
    // result replaces it; otherwise a new shape is made.
    {
        ImGui::Separator();
        ImGui::TextUnformatted("Boolean  (A op B)");
        ImGui::TextDisabled("Set op on two shapes. A and B are the only inputs.");
        static const ShapeKindDef kinds[] = {
            {Object::ShapeKind::Sphere, "Sphere"},   {Object::ShapeKind::Cube, "Box"},
            {Object::ShapeKind::Ellipsoid, "Ellipsoid"}, {Object::ShapeKind::Cylinder, "Cylinder"},
            {Object::ShapeKind::Cone, "Cone"},       {Object::ShapeKind::Torus, "Torus"},
        };
        const int kindCount = static_cast<int>(sizeof(kinds) / sizeof(kinds[0]));
        static int aIdx = 1, bIdx = 0; // Box, Sphere
        static int opIdx = 2;          // Subtract
        static float off[3] = {0.3f, 0.3f, 0.3f};
        static float smoothK = 0.1f;
        static int aSource = 0;
        const char* ops[] = {"Union (A + B)", "Intersect (A & B)", "Subtract (A - B)", "Smooth Union"};
        const geom::SdfOp opMap[] = {geom::SdfOp::Union, geom::SdfOp::Intersect,
                                     geom::SdfOp::Subtract, geom::SdfOp::SmoothUnion};
        const bool haveSel = (_selectedObject3D != nullptr);

        ImGui::RadioButton("A = Selected##b", &aSource, 0);
        ImGui::SameLine();
        ImGui::RadioButton("A = Template##b", &aSource, 1);
        if (!haveSel) aSource = 1;
        if (aSource == 0 && haveSel) ImGui::Text("   A: %s", _selectedObject3D->getIdentifier().c_str());
        else {
            if (!haveSel) ImGui::TextDisabled("   (select a shape to use 'A = Selected')");
            shapeKindCombo("A shape##b", aIdx, kinds, kindCount);
        }

        shapeKindCombo("B shape##b", bIdx, kinds, kindCount);
        ImGui::Combo("Operation", &opIdx, ops, IM_ARRAYSIZE(ops));
        ImGui::DragFloat3("B offset", off, 0.01f, -1.5f, 1.5f, "%.2f");
        if (opIdx == 3) ImGui::SliderFloat("Smoothness", &smoothK, 0.0f, 0.5f, "%.2f");

        const bool replace = (aSource == 0 && haveSel);
        if (ImGui::Button(replace ? "Boolean (replace selected)" : "Boolean (create new)")) {
            geom::SdfNode a = replace ? objectToSdfNode(*_selectedObject3D)
                                      : shapeToSdfLeaf(kinds[aIdx].k, Object::ShapeParams{});
            geom::SdfNode b = shapeToSdfLeaf(kinds[bIdx].k, Object::ShapeParams{});
            b.offset = glm::vec3(off[0], off[1], off[2]);
            float blend = (opIdx == 3) ? smoothK : 0.5f;
            geom::SdfNode node = geom::SdfNode::binary(opMap[opIdx], a, b, blend);
            if (replace) {
                _selectedObject3D->setFieldShape(node, 1.6f);
            } else {
                auto obj = std::make_unique<Object>();
                obj->setFieldShape(node, 1.6f);
                glm::mat4 t = glm::translate(glm::mat4(1.0f), _camera.pos + _camera.front * 3.0f);
                t = glm::scale(t, _brush.scale * _brush.size);
                obj->setTransform(t);
                obj->updateCollisionZone(t);
                for (int f = 0; f < obj->getFaces(); ++f)
                    obj->setFaceColor(f, _currentColor[0], _currentColor[1], _currentColor[2]);
                mgr.active().world().addObject(std::move(obj));
            }
        }
    }

    // --- Implicit (math) -------------------------------------------------
    // Type an algebraic surface f(x,y,z) = 0 (Desmos-style). The 0 iso-surface
    // is meshed by marching tetrahedra. Two sides of the same coin as the
    // control-point editor that will come later.
    {
        ImGui::Separator();
        ImGui::TextUnformatted("Implicit  f(x,y,z) = 0");
        static char buf[256] = "x*x + y*y + z*z - 0.25";
        ImGui::InputText("f(x,y,z)", buf, sizeof(buf));
        ImGui::TextDisabled("ops + - * / ^   funcs sin cos tan sqrt abs exp log   consts pi e");
        if (ImGui::SmallButton("Sphere")) std::snprintf(buf, sizeof(buf), "x*x + y*y + z*z - 0.25");
        ImGui::SameLine();
        if (ImGui::SmallButton("Gyroid")) std::snprintf(buf, sizeof(buf), "sin(8*x)*cos(8*y) + sin(8*y)*cos(8*z) + sin(8*z)*cos(8*x)");
        ImGui::SameLine();
        if (ImGui::SmallButton("Torus")) std::snprintf(buf, sizeof(buf), "(sqrt(x*x + y*y) - 0.3)^2 + z*z - 0.01");
        ImGui::SameLine();
        if (ImGui::SmallButton("Heart")) std::snprintf(buf, sizeof(buf), "(x*x + 2.25*z*z + y*y - 0.25)^3 - x*x*y*y*y - 0.1125*z*z*y*y*y");
        if (ImGui::Button("Create Implicit")) {
            geom::SdfNode node = geom::makeImplicit(buf);
            auto obj = std::make_unique<Object>();
            obj->setFieldShape(node, 1.1f);
            glm::mat4 t = glm::translate(glm::mat4(1.0f), _camera.pos + _camera.front * 3.0f);
            t = glm::scale(t, _brush.scale * _brush.size);
            obj->setTransform(t);
            obj->updateCollisionZone(t);
            for (int f = 0; f < obj->getFaces(); ++f)
                obj->setFaceColor(f, _currentColor[0], _currentColor[1], _currentColor[2]);
            mgr.active().world().addObject(std::move(obj));
        }
    }

    // --- Surface (control net) -------------------------------------------
    // Bezier control-net surface: a grid of draggable control points. Create a
    // sheet, then sculpt it in Morph mode (drag the gold/blue control handles).
    {
        ImGui::Separator();
        ImGui::TextUnformatted("Surface (control net)");
        static int du = 3, dv = 3;
        ImGui::SliderInt("Degree U", &du, 1, 6);
        ImGui::SliderInt("Degree V", &dv, 1, 6);
        if (ImGui::Button("Create Surface")) {
            auto obj = std::make_unique<Object>();
            obj->setBezierPatch(geom::makeBezierGrid(du, dv, 0.5f));
            glm::mat4 t = glm::translate(glm::mat4(1.0f), _camera.pos + _camera.front * 3.0f);
            t = glm::scale(t, _brush.scale * _brush.size);
            obj->setTransform(t);
            obj->updateCollisionZone(t);
            for (int f = 0; f < obj->getFaces(); ++f)
                obj->setFaceColor(f, _currentColor[0], _currentColor[1], _currentColor[2]);
            mgr.active().world().addObject(std::move(obj));
        }
        ImGui::TextDisabled("Then use Morph mode to drag the control points.");
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

        if (ImGui::BeginTabItem("Form")) {
            ImGui::BeginDisabled(_characterDesignLocked);

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
