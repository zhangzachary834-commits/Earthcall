#include "CreatorConsoleState.hpp"
#include <imgui.h>

namespace Rendering {

    void renderWorldConsole() {
        auto& state = getCreatorConsoleState();
        ImGui::TextUnformatted("World & Environment");
        ImGui::Separator();
        
        ImGui::Checkbox("Wireframe (Global)", &state.wireframe);

        ImGui::Checkbox("Cursor Tools Open", &state.cursorToolsOpen);
        if (state.cursorToolsOpen) {
            ImGui::Indent();
            ImGui::Text("Cursor tool options...");
            ImGui::Unindent();
        }

        ImGui::Separator();
        ImGui::TextDisabled("Physics & Atmosphere");
        ImGui::Button("Reset Day/Night Cycle");
        ImGui::Button("Adjust Gravity");
    }

} // namespace Rendering
