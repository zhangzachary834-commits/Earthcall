#include "CreatorConsoleState.hpp"
#include <imgui.h>
#include "Person/Person.hpp"

namespace Rendering {

    void renderCharacterConsole(Person* player) {
        auto& state = getCreatorConsoleState();
        ImGui::TextUnformatted("Character");
        ImGui::Separator();
        
        bool& designLocked = state.characterDesignLocked;
        if (ImGui::Checkbox("Lock Player Design", &designLocked)) {
            // Usually this prevents changing shapes by accident
        }

        if (!player) {
            ImGui::TextDisabled("No active character selected.");
            return;
        }
        
        if (designLocked) {
            ImGui::TextDisabled("Design is locked. Unlock to modify character.");
        } else {
            ImGui::Text("Character Editing Active.");
            // Example stub
            ImGui::Button("Reset Skeleton");
            ImGui::Button("Import Model");
        }
    }

} // namespace Rendering
