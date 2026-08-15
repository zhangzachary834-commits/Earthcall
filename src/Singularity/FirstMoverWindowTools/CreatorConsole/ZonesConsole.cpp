#include "CreatorConsoleState.hpp"
#include <imgui.h>

namespace Rendering {

    void renderZonesConsole() {
        ImGui::TextUnformatted("Zone Tool");
        ImGui::Separator();
        
        ImGui::Button("Create New Zone");
        ImGui::SameLine();
        ImGui::Button("Edit Boundaries");

        if (ImGui::CollapsingHeader("Active Zones", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Zone_Spawn");
            ImGui::Text("Zone_Wilderness");
        }
    }

} // namespace Rendering
