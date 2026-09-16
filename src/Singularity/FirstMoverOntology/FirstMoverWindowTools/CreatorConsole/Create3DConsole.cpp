#include "CreatorConsoleState.hpp"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include "Person/Person.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/ComplexShape.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreationTools.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "Singularity/Core/SdfBuild.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Patch.hpp"
#include "Singularity/Screen/HighlightSystem.hpp"
#include "Singularity/Screen/Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace Rendering {

    namespace {
        Singularity::Core::CreationChannel* channelOf(Core::Engine* engine) {
            if (!engine || !engine->getLawManager()) return nullptr;
            return Singularity::Core::CreationChannel::find(*engine->getLawManager());
        }

        void render3DModeButton(Mode3D mode, const char* label, const char* shortcut,
                                Singularity::Core::CreationChannel* channel,
                                float btnWidth) {
            auto& state = getCreatorConsoleState();
            const bool active = state.current3DMode == mode;
            pushActiveButtonStyle(active, ImVec4(0.20f, 0.50f, 0.70f, 1.0f),
                                  ImVec4(0.26f, 0.62f, 0.85f, 1.0f));
            const bool pressed = ImGui::Button(label, ImVec2(btnWidth, 26.0f));
            popActiveButtonStyle(active);
            if (shortcut && ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s [%s]", label, shortcut);
            }
            if (pressed) {
                if (active) {
                    apply3DMode(state, channel, Mode3D::None);
                } else {
                    apply3DMode(state, channel, mode);
                }
            }
        }

        void renderPrimitiveButton(ObjectTypes::ShapeKind kind, const char* label, float btnWidth) {
            auto& state = getCreatorConsoleState();
            const bool active = state.polyhedron.shapeKind == kind;
            pushActiveButtonStyle(active, ImVec4(0.30f, 0.55f, 0.35f, 1.0f),
                                  ImVec4(0.38f, 0.68f, 0.44f, 1.0f));
            const bool pressed = ImGui::Button(label, ImVec2(btnWidth, 24.0f));
            popActiveButtonStyle(active);
            if (pressed) {
                state.polyhedron.shapeKind = kind;
                state.currentShapeKind = kind;
            }
        }

        void renderPlacementInspector(Singularity::Core::CreationChannel* channel) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.5f, 1.0f), "Placement Settings");
            if (!channel) return;

            const char* placementModes[] = {"InFront", "CursorSnap", "ManualDistance"};
            int placementIndex = 0;
            for (int i = 0; i < 3; ++i) {
                if (channel->placementMode == placementModes[i]) placementIndex = i;
            }
            if (ImGui::Combo("Placement##Create3D", &placementIndex, placementModes, 3)) {
                channel->placementMode = placementModes[placementIndex];
                channel->manualAnchorValid = false;
            }

            if (channel->placementMode == "InFront") {
                ImGui::SliderFloat("Distance##Create3D", &channel->inFrontDistance, 0.5f, 10.0f);
            } else if (channel->placementMode == "ManualDistance") {
                ImGui::SliderFloat3("Offset (right/up/fwd)##Create3D", &channel->manualOffset.x, -10.0f, 10.0f);
                if (ImGui::Button("Reset Anchor##Create3D")) channel->manualAnchorValid = false;
            }
        }

        void clearSelection3D() {
            auto& state = getCreatorConsoleState();
            state.selectedObject3D = nullptr;
            HighlightSystem::setSelected(nullptr);
            HighlightSystem::setSelectedIds({});
        }

        void paintNewObject(Object& obj, const glm::vec3& color) {
            if (auto mine = obj.ownMaterial()) {
                mine->baseColor = color;
            }
        }

        glm::mat4 spawnTransform(Singularity::Core::CreationChannel* channel,
                                 Core::Engine* engine) {
            if (channel) return channel->getCursorSpawnTransform();
            glm::vec3 pos(0.0f, 0.0f, -2.0f);
            if (engine && engine->getCamera()) {
                pos = engine->getCamera()->getPos() + engine->getCamera()->getFront() * 3.0f;
            }
            return glm::translate(glm::mat4(1.0f), pos);
        }

        Object* spawnAuthoredObject(ZoneManager& zoneMgr,
                                    Singularity::Core::CreationChannel* channel,
                                    Core::Engine* engine,
                                    const glm::vec3& color,
                                    const std::string& desc = "Spawn Object") {
            auto obj = std::make_shared<Object>();
            obj->setTransform(spawnTransform(channel, engine));
            obj->updateCollisionZone(obj->getTransform());
            paintNewObject(*obj, color);
            Object* raw = obj.get();
            zoneMgr.active().addObject(obj);
            if (channel) channel->recordProvenance("authored-by", *raw, *channel, true, 1.0f);
            getCreatorConsoleState().recordSpawn(obj, desc);
            return raw;
        }

        void renderSelectionDetails(Object* sel, ZoneManager& zoneMgr, Core::Engine* engine, bool showHeader = true) {
            if (!sel) {
                ImGui::TextDisabled("No object selected. Click Select mode [F5] to target objects.");
                return;
            }

            auto& state = getCreatorConsoleState();

            if (showHeader) {
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Target: %s", sel->getIdentifier().c_str());
            }

            // Quick Ergonomic Actions for Selected Object
            float halfBtnW = responsiveItemWidth(2, 70.0f);
            if (ImGui::Button("Duplicate [Ctrl+D]##Sel", ImVec2(halfBtnW, 24.0f)) ||
                (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D))) {
                auto dup = std::make_shared<Object>();
                dup->setShape(sel->getShapeKind(), sel->getShapeParams());
                if (sel->hasPatch()) dup->setBezierPatch(sel->getPatchData());
                if (sel->hasField()) dup->setFieldShape(sel->getFieldData(), glm::vec3(1.1f));
                glm::mat4 t = sel->getTransform();
                t = glm::translate(t, glm::vec3(1.0f, 0.0f, 0.0f));
                dup->setTransform(t);
                dup->updateCollisionZone(dup->getTransform());
                if (auto srcMat = sel->ownMaterial()) {
                    if (auto dMat = dup->ownMaterial()) {
                        dMat->baseColor = srcMat->baseColor;
                        dMat->opacity = srcMat->opacity;
                        dMat->shininess = srcMat->shininess;
                    }
                }
                zoneMgr.active().addObject(dup);
                state.recordSpawn(dup, "Duplicate " + sel->getIdentifier());
                state.selectedObject3D = dup.get();
                HighlightSystem::setSelected(dup.get());
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Duplicate the selected object with an offset (Ctrl+D)");

            ImGui::SameLine();
            if (ImGui::Button("Delete [Del]##Sel", ImVec2(halfBtnW, 24.0f)) || ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                std::shared_ptr<Object> targetShared = nullptr;
                for (const auto& o : zoneMgr.active().getOwnedObjects()) {
                    if (o.get() == sel) {
                        targetShared = o;
                        break;
                    }
                }
                if (targetShared) {
                    state.recordDelete(targetShared, "Delete " + sel->getIdentifier());
                    zoneMgr.active().removeObject(sel);
                    clearSelection3D();
                    return;
                }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove selected object from active zone (Del)");

            float thirdBtnW = responsiveItemWidth(3, 60.0f);
            if (ImGui::Button("Focus View [F]##Sel", ImVec2(thirdBtnW, 22.0f)) || ImGui::IsKeyPressed(ImGuiKey_F)) {
                if (engine && engine->getCamera()) {
                    glm::vec3 c = sel->getCenter();
                    glm::vec3 camPos = engine->getCamera()->getPos();
                    glm::vec3 dir = c - camPos;
                    if (glm::length(dir) > 0.001f) {
                        engine->getCamera()->setFront(glm::normalize(dir));
                    }
                }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Align camera view to look at selected object (F)");

            ImGui::SameLine();
            if (ImGui::Button("Snap Rot##Sel", ImVec2(thirdBtnW, 22.0f))) {
                sel->setRotationEulerDegrees(sel->getTargetRotationEulerDegrees());
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Snap current rotation to target Euler degrees");

            ImGui::SameLine();
            if (ImGui::Button("Deselect##Sel", ImVec2(thirdBtnW, 22.0f))) {
                clearSelection3D();
                return;
            }

            ImGui::Separator();
            glm::vec3 center = sel->getCenter();
            if (ImGui::DragFloat3("Center##Sel", &center.x, 0.01f, -100.0f, 100.0f, "%.2f")) {
                sel->setCenter(center);
            }
            glm::vec3 targetRotation = sel->getTargetRotationEulerDegrees();
            if (ImGui::DragFloat3("Rotation##Sel", &targetRotation.x, 0.5f, -720.0f, 720.0f, "%.1f")) {
                sel->setTargetRotationEulerDegrees(targetRotation);
            }

            // Material Color & Quick Swatches
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.55f, 1.0f), "Material & Tint");
            if (auto mat = sel->ownMaterial()) {
                ImGui::ColorEdit3("Color##SelMat", &mat->baseColor.x, ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine();

                // Quick Palette Swatches
                static const glm::vec3 kPalette[] = {
                    {1.0f, 1.0f, 1.0f},     // White
                    {0.5f, 0.5f, 0.5f},     // Grey
                    {0.15f, 0.15f, 0.15f},  // Charcoal
                    {0.9f, 0.2f, 0.2f},     // Red
                    {0.95f, 0.55f, 0.15f},  // Orange
                    {0.95f, 0.85f, 0.2f},   // Yellow
                    {0.25f, 0.75f, 0.35f},  // Green
                    {0.2f, 0.75f, 0.9f},    // Cyan
                    {0.25f, 0.45f, 0.95f},  // Blue
                    {0.7f, 0.3f, 0.85f}     // Purple
                };
                for (int i = 0; i < 10; ++i) {
                    ImGui::PushID(i);
                    ImVec4 col(kPalette[i].r, kPalette[i].g, kPalette[i].b, 1.0f);
                    if (ImGui::ColorButton("##pal", col, ImGuiColorEditFlags_NoTooltip, ImVec2(16, 16))) {
                        mat->baseColor = kPalette[i];
                    }
                    ImGui::PopID();
                    if (i < 9) ImGui::SameLine();
                }
            }
        }
    } // namespace

    void render3DConsole(Person* player, Object* selectedObject3D, ZoneManager& zoneMgr, GLFWwindow* window, Core::Engine* engine) {
        (void)player;
        (void)window;
        auto& state = getCreatorConsoleState();
        auto* channel = channelOf(engine);

        state.currentShapeKind = state.polyhedron.shapeKind;

        struct Mode3DDef {
            Mode3D mode;
            const char* label;
            const char* shortcut;
        };

        static const Mode3DDef modeDefs[] = {
            {Mode3D::BrushCreate, "Create",     "F4"},
            {Mode3D::Selection,   "Select",     "F5"},
            {Mode3D::FaceBrush,   "Face Brush", "B"},
            {Mode3D::FacePaint,   "Face Fill",  ""},
            {Mode3D::Pottery,     "Pottery",    ""},
            {Mode3D::Rotation,    "Rotate",     "R"},
            {Mode3D::Morph,       "Morph",      "M"},
            {Mode3D::Combine,     "Combine",    ""},
            {Mode3D::Sculpt,      "Clay",       ""},
            {Mode3D::Graph,       "Graph",      "G"}
        };

        // Header and Mode Toolbar
        ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.95f, 1.0f), "3D Tool Belt");
        const float availWidth = ImGui::GetContentRegionAvail().x;
        const int modeCols = (availWidth >= 320.0f) ? 3 : 2;
        const float modeBtnWidth = responsiveItemWidth(modeCols, 80.0f);

        for (int i = 0; i < IM_ARRAYSIZE(modeDefs); ++i) {
            render3DModeButton(modeDefs[i].mode, modeDefs[i].label, modeDefs[i].shortcut, channel, modeBtnWidth);
            responsiveSameLine(i, modeCols);
        }

        ImGui::Spacing();
        if (channel) {
            bool armed = channel->spawnLawArmed;
            if (ImGui::Checkbox("Spawn as law (L)", &armed)) {
                channel->spawnLawArmed = armed;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("When armed, left-click in viewport invokes the first-mover Spawn Law (L)");
            }
            if (armed) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "(Law owns click)");
            }
        }
        ImGui::SameLine();
        int targetIdx = static_cast<int>(state.current3DTarget);
        if (ImGui::RadioButton("World", targetIdx == static_cast<int>(ToolTarget3D::WorldObjects))) {
            state.current3DTarget = ToolTarget3D::WorldObjects;
            clearSelection3D();
        }

        ImGui::Separator();

        // -------------------------------------------------------------
        // Context-Sensitive Inspector based on active 3D mode
        // -------------------------------------------------------------
        switch (state.current3DMode) {
            case Mode3D::None: {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.7f, 0.75f, 0.8f, 1.0f), "Select a tool mode above or use keyboard shortcuts:");
                ImGui::BulletText("Create [F4] : Spawn primitives, implicit SDFs, and surfaces");
                ImGui::BulletText("Select [F5] : Target objects, transform, duplicate, or delete");
                ImGui::BulletText("Face Brush : Paint on 3D geometry faces");
                ImGui::BulletText("Morph / Clay : Deform topology, edit vertices, and fuse shapes");
                break;
            }

            case Mode3D::BrushCreate: {
                ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.55f, 1.0f), "Shape Primitive");
                const int primCols = (availWidth >= 320.0f) ? 3 : 2;
                const float primBtnWidth = responsiveItemWidth(primCols, 70.0f);

                ImGui::TextDisabled("Polyhedra (flat-faced)");
                renderPrimitiveButton(ObjectTypes::ShapeKind::Cube, "Cube", primBtnWidth);
                ImGui::SameLine();
                renderPrimitiveButton(ObjectTypes::ShapeKind::Polyhedron, "Polyhedron", primBtnWidth);

                ImGui::Spacing();
                ImGui::TextDisabled("Smooth surfaces");
                const ObjectTypes::ShapeKind smoothKinds[] = {
                    ObjectTypes::ShapeKind::Sphere,
                    ObjectTypes::ShapeKind::Ellipsoid,
                    ObjectTypes::ShapeKind::Ovoid,
                    ObjectTypes::ShapeKind::Paraboloid,
                    ObjectTypes::ShapeKind::Torus
                };
                const char* smoothLabels[] = {"Sphere", "Ellipsoid", "Ovoid", "Paraboloid", "Torus"};
                for (int i = 0; i < 5; ++i) {
                    renderPrimitiveButton(smoothKinds[i], smoothLabels[i], primBtnWidth);
                    responsiveSameLine(i, primCols);
                }

                ImGui::Spacing();
                ImGui::TextDisabled("Complex shapes");
                const ObjectTypes::ShapeKind complexKinds[] = {
                    ObjectTypes::ShapeKind::Cylinder,
                    ObjectTypes::ShapeKind::Cone,
                    ObjectTypes::ShapeKind::RoundedBox
                };
                const char* complexLabels[] = {"Cylinder", "Cone", "Rounded Box"};
                for (int i = 0; i < 3; ++i) {
                    renderPrimitiveButton(complexKinds[i], complexLabels[i], primBtnWidth);
                    responsiveSameLine(i, primCols);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.55f, 1.0f), "Shape Dimensions");

                auto& sp = state.polyhedron.shapeParams;
                switch (state.polyhedron.shapeKind) {
                    case ObjectTypes::ShapeKind::Sphere:
                        ImGui::SliderFloat("Radius", &sp.r, 0.05f, 2.0f, "%.2f");
                        break;
                    case ObjectTypes::ShapeKind::Ellipsoid:
                        ImGui::SliderFloat("Semi-axis X", &sp.r,  0.05f, 2.0f, "%.2f");
                        ImGui::SliderFloat("Semi-axis Y", &sp.ry, 0.05f, 2.0f, "%.2f");
                        ImGui::SliderFloat("Semi-axis Z", &sp.rz, 0.05f, 2.0f, "%.2f");
                        break;
                    case ObjectTypes::ShapeKind::Ovoid:
                        ImGui::SliderFloat("Radius", &sp.r, 0.05f, 2.0f, "%.2f");
                        ImGui::SliderFloat("Asymmetry", &sp.ovoidAsym, 0.0f, 0.9f, "%.2f");
                        break;
                    case ObjectTypes::ShapeKind::Paraboloid:
                        ImGui::SliderFloat("Steepness", &sp.paraboloidA, 0.5f, 6.0f, "%.2f");
                        break;
                    case ObjectTypes::ShapeKind::Torus:
                        ImGui::SliderFloat("Major Radius", &sp.majorR, 0.1f, 1.0f, "%.2f");
                        ImGui::SliderFloat("Minor Radius", &sp.minorR, 0.02f, 0.5f, "%.2f");
                        break;
                    case ObjectTypes::ShapeKind::Cylinder:
                    case ObjectTypes::ShapeKind::Cone:
                        ImGui::SliderFloat("Radius", &sp.r, 0.05f, 2.0f, "%.2f");
                        ImGui::SliderFloat("Half-height", &sp.halfH, 0.05f, 2.0f, "%.2f");
                        break;
                    case ObjectTypes::ShapeKind::RoundedBox:
                        ImGui::SliderFloat("Fillet", &sp.fillet, 0.01f, 0.49f, "%.2f");
                        break;
                    default: break;
                }

                ImGui::SliderFloat("Uniform Size", &state.brush.size, 0.1f, 10.0f, "%.2f");
                ImGui::SliderFloat3("Scale", &state.brush.scale.x, 0.1f, 8.0f, "%.2f");
                ImGui::SliderFloat3("Rotation", &state.brush.rotation.x, -180.0f, 180.0f, "%.1f");
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset##Rot")) {
                    state.brush.rotation = glm::vec3(0.0f);
                }
                ImGui::Checkbox("Grid Snap", &state.brush.gridSnap);
                if (state.brush.gridSnap) {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100.0f);
                    ImGui::SliderFloat("Grid Size", &state.brush.gridSize, 0.1f, 5.0f, "%.2f");
                }

                ImGui::ColorEdit3("Material Color", &state.createColor.x);

                renderPlacementInspector(channel);

                if (state.polyhedron.shapeKind == ObjectTypes::ShapeKind::Polyhedron) {
                    ImGui::Separator();
                    if (ImGui::CollapsingHeader("Polyhedron Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
                        auto selectRegular = [&](int faces) {
                            state.polyhedron.currentType = faces;
                            state.polyhedron.irregularType = 0;
                            state.polyhedron.useCustom = false;
                        };
                        float regBtnW = responsiveItemWidth(4, 50.0f);
                        if (ImGui::Button("Tetra", ImVec2(regBtnW, 0))) selectRegular(4);
                        ImGui::SameLine();
                        if (ImGui::Button("Octa", ImVec2(regBtnW, 0))) selectRegular(8);
                        ImGui::SameLine();
                        if (ImGui::Button("Dodeca", ImVec2(regBtnW, 0))) selectRegular(12);
                        ImGui::SameLine();
                        if (ImGui::Button("Icosa", ImVec2(regBtnW, 0))) selectRegular(20);

                        ImGui::SliderInt("Faces", &state.polyhedron.currentType, 4, 50);

                        const char* irregularTypes[] = {"None", "Prism", "Antiprism", "Pyramid", "Bipyramid", "Frustum"};
                        if (ImGui::Combo("Irregular Type", &state.polyhedron.irregularType, irregularTypes, IM_ARRAYSIZE(irregularTypes))) {
                            if (state.polyhedron.irregularType > 0) {
                                state.polyhedron.useCustom = false;
                                state.polyhedron.concaveType = 0;
                            }
                        }
                        if (state.polyhedron.irregularType > 0) {
                            ImGui::SliderInt("Base Sides", &state.polyhedron.irregularBaseSides, 3, 24);
                            ImGui::SliderFloat("Height", &state.polyhedron.irregularHeight, 0.1f, 3.0f, "%.2f");
                            if (state.polyhedron.irregularType == 5) {
                                ImGui::SliderFloat("Top Scale", &state.polyhedron.frustumTopScale, 0.05f, 2.0f, "%.2f");
                            }
                        }

                        if (!state.polyhedron.useCustom && state.polyhedron.irregularType == 0) {
                            const char* concaveTypes[] = {"Regular", "Concave", "Star", "Crater"};
                            if (ImGui::Combo("Variant", &state.polyhedron.concaveType, concaveTypes, IM_ARRAYSIZE(concaveTypes)) &&
                                state.polyhedron.concaveType > 0) {
                                state.polyhedron.useCustom = false;
                                state.polyhedron.irregularType = 0;
                            }
                            if (state.polyhedron.concaveType == 1) {
                                ImGui::SliderFloat("Concavity", &state.polyhedron.concavityAmount, 0.1f, 0.8f, "%.2f");
                            } else if (state.polyhedron.concaveType == 2) {
                                ImGui::SliderFloat("Spike Length", &state.polyhedron.spikeLength, 0.1f, 1.0f, "%.2f");
                            } else if (state.polyhedron.concaveType == 3) {
                                ImGui::SliderFloat("Crater Depth", &state.polyhedron.craterDepth, 0.1f, 0.5f, "%.2f");
                            }
                        }

                        bool useCustom = state.polyhedron.useCustom;
                        if (ImGui::Checkbox("Use Custom Vertices", &useCustom)) {
                            state.polyhedron.useCustom = useCustom;
                            if (state.polyhedron.useCustom) {
                                state.polyhedron.irregularType = 0;
                                state.polyhedron.concaveType = 0;
                                if (state.polyhedron.customVertices.empty()) {
                                    state.polyhedron.generateCustom();
                                }
                            }
                        }
                        if (state.polyhedron.useCustom) {
                            bool changed = false;
                            changed |= ImGui::SliderInt("Vertices", &state.polyhedron.customVertexCount, 3, 20);
                            changed |= ImGui::SliderInt("Custom Faces", &state.polyhedron.customFaceCount, 3, 20);
                            if (changed || ImGui::Button("Regenerate Custom Polyhedron")) {
                                state.polyhedron.generateCustom();
                            }
                        }
                    }
                }

                ImGui::Separator();
                if (ImGui::CollapsingHeader("Implicit SDF  f(x,y,z) = 0")) {
                    static char implicitBuf[256] = "x*x + y*y + z*z - 0.25";
                    ImGui::InputText("Formula", implicitBuf, sizeof(implicitBuf));
                    ImGui::TextDisabled("Ops: + - * / ^  Funcs: sin cos tan sqrt abs exp log  Consts: pi e");
                    if (ImGui::SmallButton("Sphere##imp")) std::snprintf(implicitBuf, sizeof(implicitBuf), "x*x + y*y + z*z - 0.25");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Gyroid")) std::snprintf(implicitBuf, sizeof(implicitBuf), "abs(sin(pi*x)*cos(pi*y) + sin(pi*y)*cos(pi*z) + sin(pi*z)*cos(pi*x)) - 0.2");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Torus##imp")) std::snprintf(implicitBuf, sizeof(implicitBuf), "(sqrt(x*x + y*y) - 0.3)^2 + z*z - 0.01");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Heart")) std::snprintf(implicitBuf, sizeof(implicitBuf), "(x*x + 2.25*z*z + y*y - 0.25)^3 - x*x*y*y*y - 0.1125*z*z*y*y*y");
                    if (ImGui::Button("Create Implicit Object")) {
                        geom::SdfNode node = geom::makeImplicit(implicitBuf);
                        Object* o = spawnAuthoredObject(zoneMgr, channel, engine, state.createColor, "Spawn Implicit");
                        if (o) {
                            o->setFieldShape(node, glm::vec3(1.1f));
                            paintNewObject(*o, state.createColor);
                        }
                    }
                }

                if (ImGui::CollapsingHeader("Surface (Bézier Patch Control Net)")) {
                    static int du = 3, dv = 3;
                    ImGui::SliderInt("Degree U", &du, 1, 6);
                    ImGui::SliderInt("Degree V", &dv, 1, 6);
                    if (ImGui::Button("Create Bézier Surface")) {
                        Object* o = spawnAuthoredObject(zoneMgr, channel, engine, state.createColor, "Spawn Bézier Surface");
                        if (o) {
                            o->setBezierPatch(geom::makeBezierGrid(du, dv, 0.5f));
                            paintNewObject(*o, state.createColor);
                        }
                    }
                    ImGui::TextDisabled("Drag control net points in Morph mode after creating.");
                }
                break;
            }

            case Mode3D::Selection: {
                Object* sel = selectedObject3D ? selectedObject3D : state.selectedObject3D;
                renderSelectionDetails(sel, zoneMgr, engine, true);
                break;
            }

            case Mode3D::Morph: {
                ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.55f, 1.0f), "Morph (Topology & Deformation)");
                Object* o = selectedObject3D ? selectedObject3D : state.selectedObject3D;
                if (!o) {
                    ImGui::TextDisabled("Select an object first (using Select mode [F5]) to edit its topology.");
                } else if (o->isBinaryField()) {
                    ImGui::TextDisabled("Drag the gold handle in viewport to offset operand B.");
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
                    ImGui::TextDisabled("Drag handles in viewport to shape the surface.");
                    if (ImGui::Button("Raise degree U")) o->elevatePatchU();
                    ImGui::SameLine();
                    if (ImGui::Button("Raise degree V")) o->elevatePatchV();
                    if (state.patchCtrlIndex >= 0 && state.patchCtrlIndex < o->getPatchControlCount()) {
                        glm::vec3 c = o->getPatchControlLocal(state.patchCtrlIndex);
                        if (ImGui::DragFloat3("Control point", &c.x, 0.01f, -5.0f, 5.0f, "%.3f"))
                            o->setPatchControlLocal(state.patchCtrlIndex, c);
                    }
                    if (ImGui::CollapsingHeader("Polynomial coefficients (u^k v^l)")) {
                        const geom::BezierPatch& p = o->getPatchData();
                        std::vector<glm::vec3> coeff = geom::patchToMonomial(p);
                        int nu = p.nu();
                        int editedK = -1, editedL = -1; glm::vec3 editedVal(0.0f);
                        for (int l = 0; l < p.nv(); ++l) {
                            for (int k = 0; k < p.nu(); ++k) {
                                glm::vec3 c = coeff[l * nu + k];
                                char label[40];
                                std::snprintf(label, sizeof(label), "a(u^%d v^%d)##coef", k, l);
                                if (ImGui::DragFloat3(label, &c.x, 0.005f, -20.0f, 20.0f, "%.3f")) {
                                    editedK = k; editedL = l; editedVal = c;
                                }
                            }
                        }
                        if (editedK >= 0) {
                            coeff[editedL * nu + editedK] = editedVal;
                            o->setBezierPatch(geom::monomialToPatch(coeff, p.du, p.dv));
                        }
                    }
                } else if (o->getShapeKind() != ObjectTypes::ShapeKind::Polyhedron) {
                    ImGui::TextDisabled("Vertex editing currently targets polyhedral meshes.");
                } else {
                    ImGui::Text("Vertices: %d   Selected: %d",
                                o->getPolyhedronVertexCount(), state.morphVertexIndex);
                    ImGui::TextDisabled("Drag vertex handles in the viewport, or adjust below:");
                    if (state.morphVertexIndex >= 0 && state.morphVertexIndex < o->getPolyhedronVertexCount()) {
                        glm::vec3 v = o->getPolyhedronVertexLocal(state.morphVertexIndex);
                        if (ImGui::DragFloat3("Vertex (local)", &v.x, 0.01f, -5.0f, 5.0f, "%.3f")) {
                            o->setPolyhedronVertexLocal(state.morphVertexIndex, v);
                        }
                    }
                }
                break;
            }

            case Mode3D::Combine:
            case Mode3D::Sculpt:
            case Mode3D::Clay: {
                const bool clay = (state.current3DMode == Mode3D::Sculpt || state.current3DMode == Mode3D::Clay);
                ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.55f, 1.0f),
                                   clay ? "Clay (Drag shape into another to fuse)"
                                        : "Combine (Click shape A, then shape B)");
                const char* ops[] = { "Union (A+B)", "Intersect (A&B)", "Subtract (A-B)",
                                      "Smooth Union", "Blend (A<->B)" };
                for (int i = 0; i < 5; ++i) {
                    bool sel = (state.combineOp == i);
                    pushActiveButtonStyle(sel, ImVec4(0.20f, 0.55f, 0.95f, 1.0f),
                                               ImVec4(0.30f, 0.65f, 1.00f, 1.0f));
                    if (ImGui::Button(ops[i])) state.combineOp = i;
                    popActiveButtonStyle(sel);
                    if (i != 2 && i != 4) ImGui::SameLine();
                }
                if (state.combineOp == 3 || state.combineOp == 4) {
                    ImGui::SliderFloat(state.combineOp == 3 ? "Smoothness" : "Blend t",
                                       &state.combineBlend, 0.0f, 1.0f, "%.2f");
                }
                ImGui::Separator();
                if (clay) {
                    if (!state.clayGrabbed)
                        ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "Drag a shape onto another, then release to fuse.");
                    else if (state.clayTarget)
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Release to fuse into: %s", state.clayTarget->getIdentifier().c_str());
                    else
                        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Dragging... overlap another shape to fuse.");
                } else {
                    if (!state.combineOperandA)
                        ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "Click operand shape A in the scene.");
                    else {
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Operand A: %s",
                                           state.combineOperandA->getIdentifier().c_str());
                        ImGui::TextUnformatted("Now click operand B in the viewport (Right-click cancels).");
                    }
                }
                break;
            }

            case Mode3D::FaceBrush: {
                ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.55f, 1.0f), "Face Brush Settings");
                const char* brushTypeNames[] = {"Normal", "Airbrush", "Chalk", "Spray", "Smudge", "Clone"};
                ImGui::Combo("Brush Type##3d", &state.faceBrushType, brushTypeNames, IM_ARRAYSIZE(brushTypeNames));
                ImGui::SliderFloat("Brush Radius", &state.faceBrushRadius, 0.01f, 2.0f, "%.2f");
                ImGui::SliderFloat("Softness", &state.faceBrushSoftness, 0.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("Opacity##fb", &state.faceBrushOpacity, 0.01f, 1.0f, "%.2f");
                ImGui::SliderFloat("Flow##fb", &state.faceBrushFlow, 0.01f, 1.0f, "%.2f");
                ImGui::ColorEdit3("Brush Color##fb", &state.createColor.x);
                if (ImGui::CollapsingHeader("UV Mapping")) {
                    ImGui::SliderFloat("U Offset", &state.faceBrushUOffset, -2.0f, 2.0f, "%.2f");
                    ImGui::SliderFloat("V Offset", &state.faceBrushVOffset, -2.0f, 2.0f, "%.2f");
                }
                break;
            }

            case Mode3D::FacePaint: {
                ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.55f, 1.0f), "Face Paint Settings");
                ImGui::Checkbox("Advanced Face Paint", &state.advancedFacePaint);
                ImGui::ColorEdit3("Fill Color", &state.createColor.x);
                ImGui::TextDisabled("Click any face on a 3D object to fill it with the selected color.");
                break;
            }

            case Mode3D::Pottery: {
                ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.55f, 1.0f), "Pottery Sculpting");
                bool chisel = state.potteryTool == 0;
                if (ImGui::RadioButton("Chisel", chisel)) state.potteryTool = 0;
                ImGui::SameLine();
                if (ImGui::RadioButton("Expand", !chisel)) state.potteryTool = 1;
                ImGui::SliderFloat("Sculpt Strength", &state.potteryStrength, 0.01f, 2.0f, "%.2f");
                break;
            }

            case Mode3D::Rotation: {
                ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.55f, 1.0f), "Object Rotation");
                const char* axisModeNames[] = {"Free XY", "X Axis", "Y Axis", "Z Axis", "Authoritative Axis"};
                ImGui::Combo("Axis Mode", &state.rotationAxisMode, axisModeNames, IM_ARRAYSIZE(axisModeNames));
                ImGui::SliderFloat("Sensitivity", &state.rotationSensitivity, 0.05f, 2.0f, "%.2f");
                ImGui::SliderFloat("Smoothness", &state.rotationSmoothness, 1.0f, 20.0f, "%.2f");
                break;
            }

            case Mode3D::Graph: {
                ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.55f, 1.0f), "Ontological Graph & Law Authoring");
                ImGui::TextDisabled("Laws govern creation, interaction, and behavior across all zones.");
                if (ImGui::Button("Open Law Author Window")) {
                    state.showLawAuthor = true;
                }
                break;
            }
        }

        // Persistent selection status footer when not actively inspecting selection
        Object* liveSel = selectedObject3D ? selectedObject3D : state.selectedObject3D;
        if (liveSel && state.current3DMode != Mode3D::Selection) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Selected: %s", liveSel->getIdentifier().c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Inspect [F5]##Footer")) {
                apply3DMode(state, channel, Mode3D::Selection);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Focus [F]##Footer")) {
                if (engine && engine->getCamera()) {
                    glm::vec3 c = liveSel->getCenter();
                    glm::vec3 dir = c - engine->getCamera()->getPos();
                    if (glm::length(dir) > 0.001f) engine->getCamera()->setFront(glm::normalize(dir));
                }
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear##Footer")) {
                clearSelection3D();
            }
        }
    }

} // namespace Rendering
