#include "CreatorConsoleWindow.hpp"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include "Singularity/Core/Engine.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Screen/CreationWindow.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreationTools.hpp"
#include "Singularity/Core/CreationChannel.hpp"

// Include the individual console tabs
namespace Rendering {
    void renderPaintConsole(ZoneManager& zoneMgr);
    void render3DConsole(Person* player, Object* selectedObject3D, ZoneManager& zoneMgr, GLFWwindow* window, Core::Engine* engine);
    void renderCharacterConsole(Person* player);
    void renderWorldConsole(Core::Engine* engine);
    void renderAssetsConsole(Core::Engine* engine);
    void renderRelationsConsole(ZoneManager& zoneMgr);
    void renderZonesConsole(ZoneManager& zoneMgr);
}

namespace Rendering {

    void renderCreatorConsoleContent(Person* player, Object* selected, ZoneManager& zoneMgr, GLFWwindow* window, Core::Engine* engine) {
        if (!engine) engine = &Core::Engine::instance();
        if (!window && engine) window = engine->window();

        auto& state = getCreatorConsoleState();
        auto* channel = (engine && engine->getLawManager())
            ? Singularity::Core::CreationChannel::find(*engine->getLawManager())
            : nullptr;

        // Ergonomic Keyboard Shortcuts when Console has focus
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            if (ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)) {
                state.performUndo(zoneMgr);
            } else if ((ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) ||
                       (ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))) {
                state.performRedo(zoneMgr);
            }
        }

        // 1. Top Status & Ergonomics Bar
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.09f, 0.11f, 0.85f));
            if (ImGui::BeginChild("##CreatorConsoleHeaderBar", ImVec2(0, 26.0f), false, ImGuiWindowFlags_NoScrollbar)) {
                ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "Zone: %s", zoneMgr.active().name().c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("|");
                ImGui::SameLine();
                Object* liveSel = selected ? selected : state.selectedObject3D;
                if (liveSel) {
                    ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 1.0f), "Target: %s", liveSel->getIdentifier().c_str());
                } else {
                    ImGui::TextDisabled("Target: (None)");
                }

                // Undo / Redo Actions in Top Header
                float availW = ImGui::GetContentRegionAvail().x;
                float undoPos = ImGui::GetCursorPosX() + availW - 130.0f;
                if (undoPos > ImGui::GetCursorPosX()) {
                    ImGui::SameLine(undoPos);
                } else {
                    ImGui::SameLine();
                }

                bool canU = state.canUndo();
                if (!canU) ImGui::BeginDisabled();
                if (ImGui::SmallButton("↶ Undo")) {
                    state.performUndo(zoneMgr);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Undo (Ctrl+Z): %s", state.canUndo() ? state.nextUndoDesc().c_str() : "Nothing to undo");
                }
                if (!canU) ImGui::EndDisabled();

                ImGui::SameLine();
                bool canR = state.canRedo();
                if (!canR) ImGui::BeginDisabled();
                if (ImGui::SmallButton("↷ Redo")) {
                    state.performRedo(zoneMgr);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Redo (Ctrl+Y): %s", state.canRedo() ? state.nextRedoDesc().c_str() : "Nothing to redo");
                }
                if (!canR) ImGui::EndDisabled();
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        // Track external changes to state.currentSection (hotkeys, menus, initial boot state)
        static CreatorSection s_lastActiveSection = state.currentSection;
        static bool s_sectionChangedExternally = true;

        if (state.currentSection != s_lastActiveSection) {
            s_sectionChangedExternally = true;
            s_lastActiveSection = state.currentSection;
        }

        // Menu Bar or Tab Bar for Tabs (including unified Concepts tab)
        if (ImGui::BeginMenuBar()) {
            auto renderMenuItem = [&](const char* label, CreatorSection sec) {
                if (ImGui::MenuItem(label, nullptr, state.currentSection == sec)) {
                    state.currentSection = sec;
                    s_lastActiveSection = sec;
                    s_sectionChangedExternally = true;
                }
            };
            renderMenuItem("Paint", CreatorSection::Paint);
            renderMenuItem("3D Tools", CreatorSection::Create3D);
            renderMenuItem("Concepts", CreatorSection::Concepts);
            renderMenuItem("Character", CreatorSection::Character);
            renderMenuItem("World", CreatorSection::World);
            renderMenuItem("Assets", CreatorSection::Assets);
            renderMenuItem("Relations", CreatorSection::Relations);
            renderMenuItem("Zones", CreatorSection::Zones);
            ImGui::EndMenuBar();
        } else {
            // Fallback for dock/child containers without menu bar
            if (ImGui::BeginTabBar("##CreatorConsoleTabs", ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_TabListPopupButton)) {
                auto renderTab = [&](const char* label, CreatorSection sec) {
                    ImGuiTabItemFlags flags = 0;
                    if (s_sectionChangedExternally && state.currentSection == sec) {
                        flags |= ImGuiTabItemFlags_SetSelected;
                    }
                    if (ImGui::BeginTabItem(label, nullptr, flags)) {
                        state.currentSection = sec;
                        s_lastActiveSection = sec;
                        ImGui::EndTabItem();
                    }
                };

                renderTab("Paint", CreatorSection::Paint);
                renderTab("3D Tools", CreatorSection::Create3D);
                renderTab("Concepts [F9]", CreatorSection::Concepts);
                renderTab("Character", CreatorSection::Character);
                renderTab("World", CreatorSection::World);
                renderTab("Assets", CreatorSection::Assets);
                renderTab("Relations", CreatorSection::Relations);
                renderTab("Zones", CreatorSection::Zones);

                s_sectionChangedExternally = false;
                ImGui::EndTabBar();
            }
        }

        // Dispatch based on selected tab
        switch (state.currentSection) {
            case CreatorSection::Paint:
                renderPaintConsole(zoneMgr);
                break;
            case CreatorSection::Create3D:
                render3DConsole(player, selected, zoneMgr, window, engine);
                break;
            case CreatorSection::Concepts:
                if (player) {
                    renderCreationContent(*player, selected ? selected : state.selectedObject3D, zoneMgr.active());
                } else {
                    ImGui::TextDisabled("No Person present to author concepts.");
                }
                break;
            case CreatorSection::Character:
                renderCharacterConsole(player);
                break;
            case CreatorSection::World:
                renderWorldConsole(engine);
                break;
            case CreatorSection::Assets:
                renderAssetsConsole(engine);
                break;
            case CreatorSection::Relations:
                renderRelationsConsole(zoneMgr);
                break;
            case CreatorSection::Zones:
                renderZonesConsole(zoneMgr);
                break;
        }
    }

    void renderCreatorConsoleWindow(bool* open, Person* player, Object* selected, ZoneManager& zoneMgr, GLFWwindow* window, Core::Engine* engine) {
        if (!open || !*open) return;

        if (!engine) engine = &Core::Engine::instance();
        if (!window && engine) window = engine->window();

        ImGui::SetNextWindowSize(ImVec2(440, 660), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Creator Console [F8]", open, ImGuiWindowFlags_MenuBar)) {
            renderCreatorConsoleContent(player, selected, zoneMgr, window, engine);
        }
        ImGui::End();
    }

    void renderCreatorConsole3DPreviews(Person* player, Object* selected) {
        (void)player;
        (void)selected;
        auto& state = getCreatorConsoleState();
        if (state.currentSection == CreatorSection::Create3D && state.current3DMode == Mode3D::BrushCreate) {
            const char* kindStr = "Cube";
            switch (state.polyhedron.shapeKind) {
                case ObjectTypes::ShapeKind::Cube: kindStr = "Cube"; break;
                case ObjectTypes::ShapeKind::Polyhedron: kindStr = "Polyhedron"; break;
                case ObjectTypes::ShapeKind::Sphere: kindStr = "Sphere"; break;
                case ObjectTypes::ShapeKind::Ellipsoid: kindStr = "Ellipsoid"; break;
                case ObjectTypes::ShapeKind::Ovoid: kindStr = "Ovoid"; break;
                case ObjectTypes::ShapeKind::Paraboloid: kindStr = "Paraboloid"; break;
                case ObjectTypes::ShapeKind::Torus: kindStr = "Torus"; break;
                case ObjectTypes::ShapeKind::Cylinder: kindStr = "Cylinder"; break;
                case ObjectTypes::ShapeKind::Cone: kindStr = "Cone"; break;
                case ObjectTypes::ShapeKind::RoundedBox: kindStr = "Rounded Box"; break;
                default: break;
            }

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Preview: [%s]", kindStr);
            ImGui::SameLine();
            ImGui::ColorButton("##previewColor", ImVec4(state.createColor.x, state.createColor.y, state.createColor.z, 1.0f),
                               ImGuiColorEditFlags_NoTooltip, ImVec2(16, 16));
        }
    }

} // namespace Rendering
