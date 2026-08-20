#include "CreatorConsoleState.hpp"
#include <imgui.h>

namespace Rendering {

    static CreatorConsoleState g_consoleState;

    CreatorConsoleState& getCreatorConsoleState() {
        return g_consoleState;
    }

    void pushActiveButtonStyle(bool active, const ImVec4& color, const ImVec4& hoverColor) {
        if (!active) return;
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, hoverColor);
    }

    void popActiveButtonStyle(bool active) {
        if (active) {
            ImGui::PopStyleColor(3);
        }
    }

    void sameLineEvery(int index, int perRow) {
        if ((index + 1) % perRow != 0) {
            ImGui::SameLine();
        }
    }

} // namespace Rendering
