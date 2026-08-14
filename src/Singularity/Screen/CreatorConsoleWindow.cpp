#include "CreatorConsoleWindow.hpp"
#include <imgui.h>
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "ConstructedBeing/Object/Object.hpp"

namespace Rendering {

    static CreatorConsoleState g_consoleState;

    CreatorConsoleState& getCreatorConsoleState() {
        return g_consoleState;
    }

    static void pushActiveButtonStyle(bool active, ImVec4 defaultCol, ImVec4 hoveredCol) {
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, defaultCol);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredCol);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, hoveredCol);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        }
    }

    static void popActiveButtonStyle(bool) {
        ImGui::PopStyleColor(3);
    }

    static void renderSectionButton(CreatorSection section, const char* label) {
        const bool active = g_consoleState.currentSection == section;
        pushActiveButtonStyle(active, ImVec4(0.24f, 0.43f, 0.78f, 1.0f),
                              ImVec4(0.30f, 0.52f, 0.92f, 1.0f));
        const bool pressed = ImGui::Button(label, ImVec2(78.0f, 0.0f));
        popActiveButtonStyle(active);
        if (pressed) {
            g_consoleState.currentSection = section;
        }
    }

    static void renderCreatorSectionTabs() {
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
        ImGui::SameLine();
        renderSectionButton(CreatorSection::Zones, "Zones");
    }

    static void render3DModeButton(Mode3D mode, const char* label) {
        const bool active = g_consoleState.current3DMode == mode;
        pushActiveButtonStyle(active, ImVec4(0.30f, 0.50f, 0.31f, 1.0f),
                              ImVec4(0.36f, 0.62f, 0.38f, 1.0f));
        const bool pressed = ImGui::Button(label, ImVec2(118.0f, 0.0f));
        popActiveButtonStyle(active);
        if (pressed) {
            g_consoleState.current3DMode = mode;
        }
    }

    static void renderPrimitiveButton(Object::ShapeKind kind, const char* label) {
        const bool active = g_consoleState.currentShapeKind == kind;
        pushActiveButtonStyle(active, ImVec4(0.30f, 0.50f, 0.31f, 1.0f),
                              ImVec4(0.36f, 0.62f, 0.38f, 1.0f));
        const bool pressed = ImGui::Button(label, ImVec2(118.0f, 0.0f));
        popActiveButtonStyle(active);
        if (pressed) {
            g_consoleState.currentShapeKind = kind;
        }
    }

    static void render3DConsole(Person* player, Object* selected) {
        struct Mode3DDef {
            Mode3D mode;
            const char* label;
        };

        const Mode3DDef modes[] = {
            { Mode3D::Selection, "Selection" },
            { Mode3D::BrushCreate, "Create" },
            { Mode3D::Rotation, "Rotation" },
            { Mode3D::Pottery, "Pottery" },
            { Mode3D::FaceBrush, "Face Brush" },
            { Mode3D::FacePaint, "Face Fill" },
            { Mode3D::Combine, "Combine" },
            { Mode3D::Morph, "Morph" },
            { Mode3D::Clay, "Clay" }
        };

        ImGui::TextUnformatted("Tools");
        for (int i = 0; i < IM_ARRAYSIZE(modes); ++i) {
            render3DModeButton(modes[i].mode, modes[i].label);
            if (i % 3 != 2) ImGui::SameLine();
        }
        ImGui::NewLine();

        ImGui::Separator();
        
        if (g_consoleState.current3DMode == Mode3D::BrushCreate) {
            ImGui::TextUnformatted("Primitives");
            renderPrimitiveButton(Object::ShapeKind::Sphere, "Sphere");
            ImGui::SameLine();
            renderPrimitiveButton(Object::ShapeKind::Cube, "Cube");
            ImGui::SameLine();
            renderPrimitiveButton(Object::ShapeKind::Cylinder, "Cylinder");
            
            renderPrimitiveButton(Object::ShapeKind::Cone, "Cone");

            ImGui::Separator();
            ImGui::ColorEdit3("Material Color", &g_consoleState.createColor.x);
            ImGui::Checkbox("Wireframe", &g_consoleState.wireframe);
        } else if (g_consoleState.current3DMode == Mode3D::Selection) {
            ImGui::TextUnformatted("Selected Object");
            if (selected) {
                ImGui::Text("ID: %s", selected->getIdentifier().c_str());
                if (ImGui::Button("Snap Rotation")) {
                    selected->setRotationEulerDegrees(selected->getTargetRotationEulerDegrees());
                }
            } else {
                ImGui::TextDisabled("No object selected.");
            }
        }
    }

    static void renderAssetsConsole(ZoneManager& zoneMgr) {
        if (ImGui::Button("Quick Save")) {
            // zoneMgr.saveStateWithLog("", ctx); // Will need context
        }
        ImGui::SameLine();
        if (ImGui::Button("Save As")) {
            zoneMgr.getSaveLoadState().showSaveWindow = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            zoneMgr.updateSaveFiles();
            zoneMgr.getSaveLoadState().showLoadWindow = true;
        }
        if (ImGui::Button("Save Manager")) {
            zoneMgr.getSaveLoadState().showManager = true;
        }

        ImGui::Separator();
        ImGui::Text("Zone: %s", zoneMgr.active().name().c_str());
        ImGui::Text("Objects: %d", static_cast<int>(zoneMgr.active().world().getOwnedObjects().size()));
    }

    static void renderRelationsConsole(ZoneManager& zoneMgr) {
        const auto& objs = zoneMgr.active().world().getOwnedObjects();
        ImGui::TextUnformatted("Object Bonds");
        static int objAIdx = 0;
        static int objBIdx = 1;

        std::vector<std::string> labels;
        labels.reserve(objs.size());
        for (size_t i = 0; i < objs.size(); ++i) {
            std::string label = "Obj " + std::to_string(i);
            if (objs[i]) label += " - " + objs[i]->getIdentifier();
            labels.push_back(label);
        }

        std::vector<const char*> labelPtrs;
        labelPtrs.reserve(labels.size());
        for (const auto& label : labels) labelPtrs.push_back(label.c_str());

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
                    zoneMgr.active().syncFormationMembers({parent, child});
                    auto relation = std::make_shared<Relation>("attachment", *parent, *child, true, 1.0f);
                    relation->attachment.enabled = true;
                    relation->attachment.localOffset = glm::inverse(parent->getTransform()) * child->getTransform();
                    relation->attachment.parentAnchor = parent->getCenter();
                    relation->attachment.childAnchor = child->getCenter();
                    zoneMgr.active().formation().addRelation(relation);
                }
            }
        } else {
            ImGui::TextDisabled("No objects available.");
        }
        
        ImGui::Separator();
        const auto& bonds = Physics::getBonds();
        ImGui::Text("Bonds: %d", static_cast<int>(bonds.size()));
    }

    static void renderZonesConsole(ZoneManager& zoneMgr) {
        if (ImGui::BeginChild("ZonesArea", ImVec2(0, 0), true)) {
            ImGui::Text("Zone Management");
            ImGui::Separator();
            
            const auto& zones = zoneMgr.zones();
            size_t currentIdx = zoneMgr.currentIndex();
            
            if (ImGui::BeginListBox("Available Zones", ImVec2(-FLT_MIN, 150))) {
                for (size_t i = 0; i < zones.size(); ++i) {
                    const bool isSelected = (currentIdx == i);
                    std::string label = zones[i]->getIdentifier();
                    if (label.empty()) label = "Zone " + std::to_string(i);
                    
                    if (ImGui::Selectable(label.c_str(), isSelected)) {
                        zoneMgr.switchTo(i);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndListBox();
            }
        }
        ImGui::EndChild();
    }

    static void renderPaintConsole(ZoneManager& zoneMgr) {
        ImGui::TextUnformatted("Paint Tool Area (WIP)");
        ImGui::TextDisabled("Brush system requires update.");
    }

    static void renderCharacterConsole(Person* player, Object*& selected) {
        if (!player) return;
        Body& body = player->getBody();

        if (!selected && !body.parts.empty()) {
            selected = body.parts.front();
        }

        ImGui::Checkbox("Design Lock", &g_consoleState.characterDesignLocked);

        if (ImGui::BeginTabBar("CharacterTabs")) {
            if (ImGui::BeginTabItem("Body Parts")) {
                ImGui::TextUnformatted("Body Parts");
                for (auto* part : body.parts) {
                    if (!part) continue;
                    const bool isSelected = part == selected;
                    if (ImGui::Selectable(part->getName().c_str(), isSelected)) {
                        selected = part;
                    }
                }

                if (selected) {
                    BodyPart* part = dynamic_cast<BodyPart*>(selected);
                    if (part) {
                        ImGui::Separator();
                        ImGui::BeginDisabled(g_consoleState.characterDesignLocked);

                        ImGui::Text("Editing: %s", part->getName().c_str());

                        const char* shapeNames[] = {"Cube", "Polyhedron", "Sphere", "Cylinder", "Cone"};
                        int currentShape = static_cast<int>(part->getPrimaryShape());
                        // Ensure it's within bounds for the simple combo
                        if (currentShape > 4) currentShape = 0; 

                        if (ImGui::Combo("Shape", &currentShape, shapeNames, IM_ARRAYSIZE(shapeNames))) {
                            part->setPrimaryShape(static_cast<ObjectTypes::ShapeKind>(currentShape));
                        }

                        glm::vec3 dims = part->getDimensions();
                        float dimArr[3] = {dims.x, dims.y, dims.z};
                        if (ImGui::SliderFloat3("Dimensions", dimArr, 0.05f, 1.0f, "%.2f")) {
                            part->setDimensions({dimArr[0], dimArr[1], dimArr[2]});
                            part->setTransform(part->getTransform());
                        }

                        float color[3] = {
                            part->getColor()[0],
                            part->getColor()[1],
                            part->getColor()[2]
                        };
                        if (ImGui::ColorEdit3("Color", color)) {
                            part->setColor(color[0], color[1], color[2]);
                        }
                        ImGui::EndDisabled();
                    }
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    static void renderWorldConsole(ZoneManager& zoneMgr) {
        World& world = zoneMgr.active().world();
        ImGui::TextUnformatted("World Modes");
        const char* modeNames[] = {"Creative", "Survival", "Spectator"};
        int currentMode = static_cast<int>(world.getMode());
        if (ImGui::Combo("Mode", &currentMode, modeNames, IM_ARRAYSIZE(modeNames))) {
            world.setMode(static_cast<World::Mode>(currentMode));
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Cursor Picking");
        if (ImGui::Button("Open Cursor Tools")) {
            g_consoleState.cursorToolsOpen = true;
        }
        if (g_consoleState.cursorToolsOpen) {
            // Placeholder for CursorTools
            ImGui::TextDisabled("Cursor tools active");
        }
    }

    void renderCreatorConsoleWindow(bool* open, Person* player, Object* selected, ZoneManager& zoneMgr) {
        ImGui::SetNextWindowSize(ImVec2(600, 680), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Creator Console", open)) {
            ImGui::End();
            return;
        }

        renderCreatorSectionTabs();
        ImGui::Separator();

        switch (g_consoleState.currentSection) {
            case CreatorSection::Paint:
                renderPaintConsole(zoneMgr);
                break;
            case CreatorSection::Create3D:
                render3DConsole(player, selected);
                break;
            case CreatorSection::Character:
                renderCharacterConsole(player, selected);
                break;
            case CreatorSection::World:
                renderWorldConsole(zoneMgr);
                break;
            case CreatorSection::Assets:
                renderAssetsConsole(zoneMgr);
                break;
            case CreatorSection::Relations:
                renderRelationsConsole(zoneMgr);
                break;
            case CreatorSection::Zones:
                renderZonesConsole(zoneMgr);
                break;
        }

        ImGui::End();
    }

    // This will be called from EngineRender.cpp at the end of 3D scene rendering
    void renderCreatorConsole3DPreviews(Person* player, Object* selected) {
        if (!player) return;

        if (g_consoleState.current3DMode == Mode3D::BrushCreate) {
            // Use the Law system's evaluated spawn transform for the preview
            glm::mat4 previewT = glm::translate(glm::mat4(1.0f), player->cameraPos + player->cameraForward * 4.0f);

            // Render as translucent wireframe so it does not occlude view
            currentRenderer().setWireframe(true);
            currentRenderer().setModel(previewT);
            
            // Draw primitive outline using the selected shape
            Object temp;
            temp.setShape(g_consoleState.currentShapeKind, {1.0f, 1.0f, 1.0f});

            // We can set a temporary transparent material based on createColor
            temp.setFaceColor(0, g_consoleState.createColor.x, g_consoleState.createColor.y, g_consoleState.createColor.z);

            temp.drawObject();
            temp.drawHighlightOutline();

            currentRenderer().setModel(glm::mat4(1.0f));
            currentRenderer().setWireframe(false);
        }
    }

} // namespace Rendering
