#include "CreatorConsoleState.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CursorTools.hpp"
#include <imgui.h>

namespace Rendering {

    void renderWorldConsole(Core::Engine* engine) {
        auto& state = getCreatorConsoleState();
        ImGui::TextUnformatted("World & Environment");
        ImGui::Separator();

        ImGui::TextDisabled("Global wireframe is not wired to the renderer.");

        if (ImGui::Button("Open Cursor Tools")) {
            state.cursorToolsOpen = true;
        }

        ImGui::Checkbox("Cursor Tools Open", &state.cursorToolsOpen);
        if (state.cursorToolsOpen) {
            if (engine && engine->getCursorTools()) {
                engine->getCursorTools()->renderUI(state.cursorToolsOpen);
            } else {
                ImGui::TextDisabled("Cursor tools are not constructed.");
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("Day/night cycle and gravity adjust are not wired yet.");
    }

} // namespace Rendering
