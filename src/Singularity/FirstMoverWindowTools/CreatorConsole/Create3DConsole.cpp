#include "CreatorConsoleState.hpp"
#include <imgui.h>
#include "Person/Person.hpp"
#include "ConstructedBeing/Object/Geometry/ComplexShape.hpp"

namespace Rendering {

    namespace {
        void set3DMode(Mode3D mode) {
            auto& state = getCreatorConsoleState();
            state.current3DMode = mode;
            state.combineOperandA = nullptr;
            state.clayGrabbed = nullptr;
            state.clayTarget = nullptr;
            
            if (mode == Mode3D::FacePaint) {
                state.currentTool = Tool(Tool::Type::FacePaint);
            } else if (mode == Mode3D::FaceBrush) {
                state.currentTool = Tool(Tool::Type::FaceBrush);
            }
        }

        void render3DModeButton(Mode3D mode, const char* label) {
            auto& state = getCreatorConsoleState();
            const bool active = state.current3DMode == mode;
            pushActiveButtonStyle(active, ImVec4(0.30f, 0.50f, 0.31f, 1.0f),
                                  ImVec4(0.36f, 0.62f, 0.38f, 1.0f));
            const bool pressed = ImGui::Button(label, ImVec2(118.0f, 0.0f));
            popActiveButtonStyle(active);
            if (pressed) {
                set3DMode(mode);
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

        void renderPlacementInspector() {
            // Stub for placement inspector from old_GameToolbar.cpp
            ImGui::Separator();
            ImGui::TextUnformatted("Placement Settings");
        }

        void clearSelection3D() {
            // Call into system to clear selection
        }
    }

    void render3DConsole(Person* player, Object* selectedObject3D) {
        auto& state = getCreatorConsoleState();

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
            {Mode3D::Combine, "Combine"},
            {Mode3D::Sculpt, "Clay"},
            {Mode3D::Graph, "Graph"}
        };
        ImGui::TextUnformatted("Mode");
        for (int i = 0; i < IM_ARRAYSIZE(modeDefs); ++i) {
            render3DModeButton(modeDefs[i].mode, modeDefs[i].label);
            sameLineEvery(i, 3);
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

        renderPlacementInspector();

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
    }

} // namespace Rendering
