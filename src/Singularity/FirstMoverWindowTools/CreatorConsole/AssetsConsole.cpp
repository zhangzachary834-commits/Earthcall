#include "CreatorConsoleState.hpp"
#include <imgui.h>
#include "Singularity/Core/Engine.hpp"

namespace Rendering {

    void renderAssetsConsole() {
        ImGui::TextUnformatted("Assets & Save Management");
        ImGui::Separator();
        
        static char saveName[128] = "my_world";
        ImGui::InputText("Save Name", saveName, IM_ARRAYSIZE(saveName));
        
        if (ImGui::Button("Save World")) {
            // auto& engine = Core::Engine::get();
            // engine.saveWorld(saveName);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load World")) {
            // auto& engine = Core::Engine::get();
            // engine.loadWorld(saveName);
        }

        ImGui::Separator();
        ImGui::TextDisabled("Asset Browser (Placeholder)");
        if (ImGui::TreeNode("Materials")) {
            ImGui::Text("material.clay");
            ImGui::Text("material.stone");
            ImGui::Text("material.wood");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Scripts")) {
            ImGui::Text("law.gravity");
            ImGui::Text("law.collision");
            ImGui::TreePop();
        }
    }

} // namespace Rendering
