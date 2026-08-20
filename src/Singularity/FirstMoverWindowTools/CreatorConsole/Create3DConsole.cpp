#include "CreatorConsoleState.hpp"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include "Person/Person.hpp"
#include "ConstructedBeing/Object/Geometry/ComplexShape.hpp"
#include "Singularity/FirstMoverWindowTools/Tool.hpp"
#include "Singularity/FirstMoverWindowTools/CreationTools.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Singularity/Core/SdfBuild.hpp"
#include "ConstructedBeing/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Object/Geometry/Patch.hpp"
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

        void render3DModeButton(Mode3D mode, const char* label,
                                Singularity::Core::CreationChannel* channel) {
            auto& state = getCreatorConsoleState();
            const bool active = state.current3DMode == mode;
            pushActiveButtonStyle(active, ImVec4(0.30f, 0.50f, 0.31f, 1.0f),
                                  ImVec4(0.36f, 0.62f, 0.38f, 1.0f));
            const bool pressed = ImGui::Button(label, ImVec2(118.0f, 0.0f));
            popActiveButtonStyle(active);
            if (pressed) {
                apply3DMode(state, channel, mode);
            }
        }

        void renderPrimitiveButton(ObjectTypes::ShapeKind kind, const char* label) {
            auto& state = getCreatorConsoleState();
            const bool active = state.polyhedron.shapeKind == kind;
            pushActiveButtonStyle(active, ImVec4(0.30f, 0.50f, 0.31f, 1.0f),
                                  ImVec4(0.36f, 0.62f, 0.38f, 1.0f));
            const bool pressed = ImGui::Button(label, ImVec2(118.0f, 0.0f));
            popActiveButtonStyle(active);
            if (pressed) {
                state.polyhedron.shapeKind = kind;
            }
        }

        void renderPlacementInspector(Singularity::Core::CreationChannel* channel) {
            ImGui::Separator();
            ImGui::TextUnformatted("Placement Settings");
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
            for (int f = 0; f < obj.getFaces(); ++f)
                obj.setFaceColor(f, color.x, color.y, color.z);
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
                                    const glm::vec3& color) {
            auto obj = std::make_shared<Object>();
            obj->setTransform(spawnTransform(channel, engine));
            obj->updateCollisionZone(obj->getTransform());
            paintNewObject(*obj, color);
            Object* raw = obj.get();
            zoneMgr.active().world().addObject(obj);
            if (channel) channel->recordProvenance("authored-by", *raw, *channel, true, 1.0f);
            return raw;
        }
    }

    void render3DConsole(Person* player, Object* selectedObject3D, ZoneManager& zoneMgr, GLFWwindow* window, Core::Engine* engine) {
        (void)player;
        (void)window;
        auto& state = getCreatorConsoleState();
        auto* channel = channelOf(engine);

        struct Mode3DDef {
            Mode3D mode;
            const char* label;
        };

        static const Mode3DDef modeDefs[] = {
            // Console Create: developer bypass. Does not arm the spawn law.
            {Mode3D::BrushCreate, "Create"},
            {Mode3D::Selection, "Select"},
            {Mode3D::FaceBrush, "Face Brush"},
            {Mode3D::FacePaint, "Face Fill"},
            {Mode3D::Pottery, "Pottery"},
            {Mode3D::Rotation, "Rotate"},
            {Mode3D::Morph, "Morph"},
            {Mode3D::Combine, "Combine"},
            {Mode3D::Sculpt, "Clay"},
            {Mode3D::Graph, "Graph"}
        };
        ImGui::TextUnformatted("Mode");
        for (int i = 0; i < IM_ARRAYSIZE(modeDefs); ++i) {
            render3DModeButton(modeDefs[i].mode, modeDefs[i].label, channel);
            sameLineEvery(i, 3);
        }
        if (channel) {
            bool armed = channel->spawnLawArmed;
            if (ImGui::Checkbox("Spawn as law (L)", &armed)) {
                channel->spawnLawArmed = armed;
            }
            if (armed) {
                ImGui::SameLine();
                ImGui::TextUnformatted("law owns the click");
            }
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Target");
        int targetIdx = static_cast<int>(state.current3DTarget);
        if (ImGui::RadioButton("World Objects", targetIdx == static_cast<int>(ToolTarget3D::WorldObjects))) {
            state.current3DTarget = ToolTarget3D::WorldObjects;
            clearSelection3D();
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Shape");
        ImGui::TextDisabled("Polyhedra (flat-faced)");
        renderPrimitiveButton(ObjectTypes::ShapeKind::Cube, "Cube"); ImGui::SameLine();
        renderPrimitiveButton(ObjectTypes::ShapeKind::Polyhedron, "Polyhedron");
        ImGui::TextDisabled("Smooth surfaces");
        renderPrimitiveButton(ObjectTypes::ShapeKind::Sphere, "Sphere"); ImGui::SameLine();
        renderPrimitiveButton(ObjectTypes::ShapeKind::Ellipsoid, "Ellipsoid"); ImGui::SameLine();
        renderPrimitiveButton(ObjectTypes::ShapeKind::Ovoid, "Ovoid");
        renderPrimitiveButton(ObjectTypes::ShapeKind::Paraboloid, "Paraboloid"); ImGui::SameLine();
        renderPrimitiveButton(ObjectTypes::ShapeKind::Torus, "Torus");
        ImGui::TextDisabled("Complex (round + edges)");
        renderPrimitiveButton(ObjectTypes::ShapeKind::Cylinder, "Cylinder"); ImGui::SameLine();
        renderPrimitiveButton(ObjectTypes::ShapeKind::Cone, "Cone"); ImGui::SameLine();
        renderPrimitiveButton(ObjectTypes::ShapeKind::RoundedBox, "Rounded Box");

        {
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
        }

        ImGui::SliderFloat("Uniform Size", &state.brush.size, 0.1f, 10.0f, "%.2f");
        ImGui::SliderFloat3("Scale", &state.brush.scale.x, 0.1f, 8.0f, "%.2f");
        ImGui::SliderFloat3("Rotation", &state.brush.rotation.x, -180.0f, 180.0f, "%.1f");
        ImGui::SameLine();
        if (ImGui::Button("Reset##CreateRotation")) {
            state.brush.rotation = glm::vec3(0.0f);
        }
        ImGui::Checkbox("Grid Snap", &state.brush.gridSnap);
        if (state.brush.gridSnap) {
            ImGui::SliderFloat("Grid Size", &state.brush.gridSize, 0.1f, 5.0f, "%.2f");
        }

        ImGui::ColorEdit3("Material Color", &state.createColor.x);

        renderPlacementInspector(channel);

        if (state.polyhedron.shapeKind == ObjectTypes::ShapeKind::Polyhedron) {
            ImGui::Separator();
            ImGui::TextUnformatted("Polyhedron");
            auto selectRegularPolyhedron = [&](int faces) {
                state.polyhedron.currentType = faces;
                state.polyhedron.irregularType = 0;
                state.polyhedron.useCustom = false;
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
            if (ImGui::SliderInt("Faces", &state.polyhedron.currentType, 4, 50)) {
                state.polyhedron.irregularType = 0;
                state.polyhedron.useCustom = false;
            }

            if (ImGui::CollapsingHeader("Irregular Polyhedron")) {
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

            if (ImGui::CollapsingHeader("Custom Polyhedron")) {
                bool useCustom = state.polyhedron.useCustom;
                if (ImGui::Checkbox("Use Custom", &useCustom)) {
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
                    if (changed || ImGui::Button("Regenerate")) {
                        state.polyhedron.generateCustom();
                    }
                }
            }
        }

        {
            ImGui::Separator();
            ImGui::TextUnformatted("Implicit  f(x,y,z) = 0");
            static char implicitBuf[256] = "x*x + y*y + z*z - 0.25";
            ImGui::InputText("f(x,y,z)", implicitBuf, sizeof(implicitBuf));
            ImGui::TextDisabled("ops + - * / ^   funcs sin cos tan sqrt abs exp log   consts pi e");
            if (ImGui::SmallButton("Sphere##imp")) std::snprintf(implicitBuf, sizeof(implicitBuf), "x*x + y*y + z*z - 0.25");
            ImGui::SameLine();
            // The raw gyroid sin(kx)cos(ky)+… = 0 is a paper-thin periodic sheet,
            // defined everywhere. Frequency 8 packed ~3 cells into the box and
            // the marcher, starting at the eye, hit sheets between the camera
            // and the object — a flashing cube of holes. abs(f)-t is the
            // thickened lattice people mean by "gyroid"; one cell fits the box.
            if (ImGui::SmallButton("Gyroid")) std::snprintf(implicitBuf, sizeof(implicitBuf), "abs(sin(pi*x)*cos(pi*y) + sin(pi*y)*cos(pi*z) + sin(pi*z)*cos(pi*x)) - 0.2");
            ImGui::SameLine();
            if (ImGui::SmallButton("Torus##imp")) std::snprintf(implicitBuf, sizeof(implicitBuf), "(sqrt(x*x + y*y) - 0.3)^2 + z*z - 0.01");
            ImGui::SameLine();
            if (ImGui::SmallButton("Heart")) std::snprintf(implicitBuf, sizeof(implicitBuf), "(x*x + 2.25*z*z + y*y - 0.25)^3 - x*x*y*y*y - 0.1125*z*z*y*y*y");
            ImGui::TextDisabled("Gyroid is a thickened lattice (abs(f)-t). The raw f=0 sheet is paper-thin.");
            if (ImGui::Button("Create Implicit")) {
                geom::SdfNode node = geom::makeImplicit(implicitBuf);
                Object* o = spawnAuthoredObject(zoneMgr, channel, engine, state.createColor);
                if (o) {
                    o->setFieldShape(node, 1.1f);
                    paintNewObject(*o, state.createColor);
                }
            }
        }

        {
            ImGui::Separator();
            ImGui::TextUnformatted("Surface (control net)");
            static int du = 3, dv = 3;
            ImGui::SliderInt("Degree U", &du, 1, 6);
            ImGui::SliderInt("Degree V", &dv, 1, 6);
            if (ImGui::Button("Create Surface")) {
                Object* o = spawnAuthoredObject(zoneMgr, channel, engine, state.createColor);
                if (o) {
                    o->setBezierPatch(geom::makeBezierGrid(du, dv, 0.5f));
                    paintNewObject(*o, state.createColor);
                }
            }
            ImGui::TextDisabled("Then use Morph mode to drag the control points.");
        }

        if (state.current3DMode == Mode3D::Combine || state.current3DMode == Mode3D::Sculpt) {
            const bool clay = (state.current3DMode == Mode3D::Sculpt);
            ImGui::Separator();
            ImGui::TextUnformatted(clay ? "Clay  (drag a shape into another)"
                                        : "Combine  (click A, then B)");
            const char* ops[] = { "Union  (A + B)", "Intersect  (A & B)", "Subtract  (A - B)",
                                  "Smooth Union", "Blend  (A <-> B)" };
            ImGui::TextUnformatted("Operation");
            for (int i = 0; i < 5; ++i) {
                bool sel = (state.combineOp == i);
                pushActiveButtonStyle(sel, ImVec4(0.20f, 0.55f, 0.95f, 1.0f),
                                           ImVec4(0.30f, 0.65f, 1.00f, 1.0f));
                if (ImGui::Button(ops[i])) state.combineOp = i;
                popActiveButtonStyle(sel);
                if (i != 2 && i != 4) ImGui::SameLine();
            }
            if (state.combineOp == 3 || state.combineOp == 4)
                ImGui::SliderFloat(state.combineOp == 3 ? "Smoothness" : "Blend t",
                                   &state.combineBlend, 0.0f, 1.0f, "%.2f");
            ImGui::Separator();
            if (clay) {
                if (!state.clayGrabbed)
                    ImGui::TextColored(ImVec4(0.6f,0.9f,1.0f,1.0f),
                                       "Drag a shape onto another, then release to fuse.");
                else if (state.clayTarget)
                    ImGui::TextColored(ImVec4(1.0f,0.85f,0.2f,1.0f),
                                       "Release to fuse into: %s", state.clayTarget->getIdentifier().c_str());
                else
                    ImGui::TextColored(ImVec4(0.8f,0.8f,0.8f,1.0f),
                                       "Dragging... overlap a shape to fuse.");
            } else {
                if (!state.combineOperandA)
                    ImGui::TextColored(ImVec4(0.6f,0.9f,1.0f,1.0f), "Click shape A in the scene.");
                else {
                    ImGui::TextColored(ImVec4(1.0f,0.85f,0.2f,1.0f), "A: %s",
                                       state.combineOperandA->getIdentifier().c_str());
                    ImGui::TextUnformatted("Now click shape B  (right-click cancels).");
                }
            }
            ImGui::TextDisabled("The absorbed shape stays draggable in Morph mode.");
        }

        if (state.current3DMode == Mode3D::Morph) {
            ImGui::Separator();
            ImGui::TextUnformatted("Morph (topology)");
            Object* o = selectedObject3D;
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
                if (state.patchCtrlIndex >= 0 && state.patchCtrlIndex < o->getPatchControlCount()) {
                    glm::vec3 c = o->getPatchControlLocal(state.patchCtrlIndex);
                    if (ImGui::DragFloat3("Control point", &c.x, 0.01f, -5.0f, 5.0f, "%.3f"))
                        o->setPatchControlLocal(state.patchCtrlIndex, c);
                }
                if (ImGui::CollapsingHeader("Polynomial coefficients (u^k v^l)")) {
                    ImGui::TextDisabled("S(u,v) = sum  a(k,l) * u^k * v^l  (geometry <-> algebra)");
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
                ImGui::TextDisabled("Vertex editing is for polyhedra (for now).");
            } else {
                ImGui::Text("Vertices: %d   Selected: %d",
                            o->getPolyhedronVertexCount(), state.morphVertexIndex);
                ImGui::TextDisabled("Drag a handle in the viewport, or type below.");
                if (state.morphVertexIndex >= 0 && state.morphVertexIndex < o->getPolyhedronVertexCount()) {
                    glm::vec3 v = o->getPolyhedronVertexLocal(state.morphVertexIndex);
                    if (ImGui::DragFloat3("Vertex (local)", &v.x, 0.01f, -5.0f, 5.0f, "%.3f")) {
                        o->setPolyhedronVertexLocal(state.morphVertexIndex, v);
                    }
                    const auto& pd = o->getPolyhedronData();
                    float minE = 1e9f, maxE = 0.0f; int count = 0;
                    for (const auto& face : pd.faces) {
                        for (size_t k = 0; k < face.size(); ++k) {
                            int a = face[k], b = face[(k + 1) % face.size()];
                            if (a == state.morphVertexIndex || b == state.morphVertexIndex) {
                                float len = glm::length(pd.vertices[a] - pd.vertices[b]);
                                minE = std::min(minE, len); maxE = std::max(maxE, len); ++count;
                            }
                        }
                    }
                    if (count > 0) {
                        ImGui::TextDisabled("Edges: %d (%.2f to %.2f)", count, minE, maxE);
                    }
                }
            }
        }

        if (state.current3DMode == Mode3D::FaceBrush) {
            ImGui::Separator();
            ImGui::TextUnformatted("Face Brush");
            const char* brushTypeNames[] = {"Normal", "Airbrush", "Chalk", "Spray", "Smudge", "Clone"};
            ImGui::Combo("Brush Type##3d", &state.faceBrushType, brushTypeNames, IM_ARRAYSIZE(brushTypeNames));
            ImGui::SliderFloat("Brush Radius", &state.faceBrushRadius, 0.01f, 2.0f, "%.2f");
            ImGui::SliderFloat("Softness", &state.faceBrushSoftness, 0.0f, 2.0f, "%.2f");
            if (ImGui::CollapsingHeader("UV Mapping")) {
                ImGui::SliderFloat("U Offset", &state.faceBrushUOffset, -2.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("V Offset", &state.faceBrushVOffset, -2.0f, 2.0f, "%.2f");
            }
        }

        if (state.current3DMode == Mode3D::FacePaint) {
            ImGui::Separator();
            ImGui::Checkbox("Advanced Face Paint", &state.advancedFacePaint);
        }

        if (state.current3DMode == Mode3D::Pottery) {
            ImGui::Separator();
            bool chisel = state.potteryTool == 0;
            if (ImGui::RadioButton("Chisel", chisel)) state.potteryTool = 0;
            ImGui::SameLine();
            if (ImGui::RadioButton("Expand", !chisel)) state.potteryTool = 1;
            ImGui::SliderFloat("Strength", &state.potteryStrength, 0.01f, 2.0f, "%.2f");
        }

        if (state.current3DMode == Mode3D::Rotation) {
            ImGui::Separator();
            const char* axisModeNames[] = {"Free XY", "X", "Y", "Z", "Authoritative Axis"};
            ImGui::Combo("Axis Mode", &state.rotationAxisMode, axisModeNames, IM_ARRAYSIZE(axisModeNames));
            ImGui::SliderFloat("Sensitivity", &state.rotationSensitivity, 0.05f, 2.0f, "%.2f");
            ImGui::SliderFloat("Smoothness", &state.rotationSmoothness, 1.0f, 20.0f, "%.2f");
        }

        if (state.current3DMode == Mode3D::Graph) {
            ImGui::Separator();
            ImGui::TextUnformatted("Graph");
            ImGui::TextDisabled("Law Author opens from this mode (Graph button).");
            if (ImGui::Button("Open Law Author")) state.showLawAuthor = true;
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Selection");
        Object* sel = selectedObject3D ? selectedObject3D : state.selectedObject3D;
        if (sel) {
            ImGui::TextWrapped("%s", sel->getIdentifier().c_str());
            glm::vec3 center = sel->getCenter();
            if (ImGui::DragFloat3("Center", &center.x, 0.01f, -100.0f, 100.0f, "%.2f")) {
                sel->setCenter(center);
            }
            glm::vec3 targetRotation = sel->getTargetRotationEulerDegrees();
            if (ImGui::DragFloat3("Target Rotation", &targetRotation.x, 0.5f, -720.0f, 720.0f, "%.1f")) {
                sel->setTargetRotationEulerDegrees(targetRotation);
            }
            if (ImGui::Button("Snap Rotation")) {
                sel->setRotationEulerDegrees(sel->getTargetRotationEulerDegrees());
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Selection")) {
                clearSelection3D();
            }
        } else {
            ImGui::TextDisabled("No object selected. Use Select mode and click.");
        }

        // Tools run in Rendering::stepCreationTools (Engine::update), not
        // here. Rendering this tab used to be the only thing that actuated
        // them, so collapsing the window or switching tabs froze the mode
        // the chrome still showed as armed.
    }

} // namespace Rendering
