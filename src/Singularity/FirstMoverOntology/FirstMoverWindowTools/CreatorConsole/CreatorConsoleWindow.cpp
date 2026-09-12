#include "CreatorConsoleWindow.hpp"
#include <imgui.h>
#include "Singularity/Core/Engine.hpp"

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

        // Track external changes to state.currentSection (hotkeys, menus, initial boot state)
        static CreatorSection s_lastActiveSection = state.currentSection;
        static bool s_sectionChangedExternally = true;

        if (state.currentSection != s_lastActiveSection) {
            s_sectionChangedExternally = true;
            s_lastActiveSection = state.currentSection;
        }

        // Menu Bar or Tab Bar for Tabs
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

        ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Creator Console [F8]", open, ImGuiWindowFlags_MenuBar)) {
            renderCreatorConsoleContent(player, selected, zoneMgr, window, engine);
        }
        ImGui::End();
    }

    void renderCreatorConsole3DPreviews(Person* player, Object* selected) {
        // Handle rendering of 3D previews
        auto& state = getCreatorConsoleState();
        if (state.currentSection == CreatorSection::Create3D && state.current3DMode == Mode3D::BrushCreate) {
            // Render primitive preview...
        }
    }

} // namespace Rendering
