#include "CreatorConsoleState.hpp"
#include "Person/Person.hpp"
#include <imgui.h>

namespace Rendering {

    void renderCharacterConsole(Person* player) {
        ImGui::TextUnformatted("Character");
        ImGui::Separator();

        if (!player) {
            ImGui::TextDisabled("No Person in this instance.");
            return;
        }

        ImGui::Text("Person: %s", player->getIdentifier().c_str());
        ImGui::TextDisabled("Skeleton reset and model import are not wired yet.");
    }

} // namespace Rendering
