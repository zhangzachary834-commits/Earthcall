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

        // Menu Bar or Tab Bar for Tabs
        if (ImGui::BeginMenuBar()) {
            if (ImGui::MenuItem("Paint", nullptr, state.currentSection == CreatorSection::Paint)) {
                state.currentSection = CreatorSection::Paint;
            }
            if (ImGui::MenuItem("3D Tools", nullptr, state.currentSection == CreatorSection::Create3D)) {
                state.currentSection = CreatorSection::Create3D;
            }
            if (ImGui::MenuItem("Character", nullptr, state.currentSection == CreatorSection::Character)) {
                state.currentSection = CreatorSection::Character;
            }
            if (ImGui::MenuItem("World", nullptr, state.currentSection == CreatorSection::World)) {
                state.currentSection = CreatorSection::World;
            }
            if (ImGui::MenuItem("Assets", nullptr, state.currentSection == CreatorSection::Assets)) {
                state.currentSection = CreatorSection::Assets;
            }
            if (ImGui::MenuItem("Relations", nullptr, state.currentSection == CreatorSection::Relations)) {
                state.currentSection = CreatorSection::Relations;
            }
            if (ImGui::MenuItem("Zones", nullptr, state.currentSection == CreatorSection::Zones)) {
                state.currentSection = CreatorSection::Zones;
            }
            ImGui::EndMenuBar();
        } else {
            // Fallback for dock/child containers without menu bar
            if (ImGui::BeginTabBar("CreatorConsoleTabs", ImGuiTabBarFlags_None)) {
                if (ImGui::BeginTabItem("Paint", nullptr, state.currentSection == CreatorSection::Paint ? ImGuiTabItemFlags_SetSelected : 0)) {
                    state.currentSection = CreatorSection::Paint;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("3D Tools", nullptr, state.currentSection == CreatorSection::Create3D ? ImGuiTabItemFlags_SetSelected : 0)) {
                    state.currentSection = CreatorSection::Create3D;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Character", nullptr, state.currentSection == CreatorSection::Character ? ImGuiTabItemFlags_SetSelected : 0)) {
                    state.currentSection = CreatorSection::Character;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("World", nullptr, state.currentSection == CreatorSection::World ? ImGuiTabItemFlags_SetSelected : 0)) {
                    state.currentSection = CreatorSection::World;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Assets", nullptr, state.currentSection == CreatorSection::Assets ? ImGuiTabItemFlags_SetSelected : 0)) {
                    state.currentSection = CreatorSection::Assets;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Relations", nullptr, state.currentSection == CreatorSection::Relations ? ImGuiTabItemFlags_SetSelected : 0)) {
                    state.currentSection = CreatorSection::Relations;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Zones", nullptr, state.currentSection == CreatorSection::Zones ? ImGuiTabItemFlags_SetSelected : 0)) {
                    state.currentSection = CreatorSection::Zones;
                    ImGui::EndTabItem();
                }
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
