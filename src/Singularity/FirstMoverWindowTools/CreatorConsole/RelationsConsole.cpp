#include "CreatorConsoleState.hpp"
#include <imgui.h>

namespace Rendering {

    void renderRelationsConsole() {
        ImGui::TextUnformatted("Relations Window");
        ImGui::Separator();
        
        ImGui::TextDisabled("Relations are first-class beings.");
        ImGui::Button("Create New Relation");
        
        if (ImGui::CollapsingHeader("Active Relations", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Relation: Player <-> Zone_1");
            ImGui::Text("Relation: Player <-> Sun");
        }
    }

} // namespace Rendering
