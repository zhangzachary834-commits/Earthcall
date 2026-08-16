#include "CreatorConsoleWindow.hpp"
#include <imgui.h>

// Include the individual console tabs
namespace Rendering {
    void renderPaintConsole(ZoneManager& zoneMgr);
    void render3DConsole(Person* player, Object* selectedObject3D, Core::Engine* engine);
    void renderCharacterConsole(Person* player);
    void renderWorldConsole();
    void renderAssetsConsole();
    void renderRelationsConsole();
    void renderZonesConsole();
}

namespace Rendering {

    void renderCreatorConsoleWindow(bool* open, Person* player, Object* selected, ZoneManager& zoneMgr, Core::Engine* engine) {
        ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Creator Console [F8]", open, ImGuiWindowFlags_MenuBar)) {
            
            auto& state = getCreatorConsoleState();

            // Menu Bar for Tabs
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
            }

            // Dispatch based on selected tab
            switch (state.currentSection) {
                case CreatorSection::Paint:
                    renderPaintConsole(zoneMgr);
                    break;
                case CreatorSection::Create3D:
                    render3DConsole(player, selected, engine);
                    break;
                case CreatorSection::Character:
                    renderCharacterConsole(player);
                    break;
                case CreatorSection::World:
                    renderWorldConsole();
                    break;
                case CreatorSection::Assets:
                    renderAssetsConsole();
                    break;
                case CreatorSection::Relations:
                    renderRelationsConsole();
                    break;
                case CreatorSection::Zones:
                    renderZonesConsole();
                    break;
            }
        }
        ImGui::End();
    }

    void renderCreatorConsole3DPreviews(Person* player, Object* selected) {
        // Handle rendering of 3D previews
        auto& state = getCreatorConsoleState();
        if (state.currentSection == CreatorSection::Create3D && state.current3DMode == Mode3D::BrushCreate) {
            // Render primitive preview...
            // Note: Keep it as hardcoded OpenGL fixed-function drawing or similar
            // per user request: "If youre talking about the 3D tool's visual representation shape preview, it should just be the old hardcoded preview logic"
        }
    }

} // namespace Rendering
