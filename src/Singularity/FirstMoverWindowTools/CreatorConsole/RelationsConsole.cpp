#include "CreatorConsoleState.hpp"
#include <imgui.h>

namespace Rendering {

    void renderRelationsConsole() {
        auto& state = getCreatorConsoleState();
        ImGui::TextUnformatted("Relations Window");
        ImGui::Separator();
        
        ImGui::TextDisabled("Relations are first-class beings.");
        ImGui::Button("Create New Relation");
        
        ImGui::Separator();
        if (ImGui::Button("Open Law Author")) {
            state.showLawAuthor = true;
        }
        
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Active Relations", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Relation: Player <-> Zone_1");
            ImGui::Text("Relation: Player <-> Sun");
        }
    }

} // namespace Rendering
