#include "CreatorConsoleState.hpp"
#include "Person/Person.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include <imgui.h>

namespace Rendering {

    void renderCharacterConsole(Person* player) {
        ImGui::TextUnformatted("Character");
        ImGui::Separator();

        if (!player) {
            ImGui::TextDisabled("No Person in this instance.");
            return;
        }

        auto& state = getCreatorConsoleState();
        Body& body = player->getBody();

        ImGui::Text("Person: %s", player->getIdentifier().c_str());
        ImGui::Checkbox("Design Lock", &state.characterDesignLocked);

        if (!state.selectedCharacterPart && !body.parts.empty()) {
            state.selectedCharacterPart = body.parts.front();
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Body Parts");
        for (auto* part : body.parts) {
            if (!part) continue;
            const bool selected = part == state.selectedCharacterPart;
            if (ImGui::Selectable(part->getName().c_str(), selected)) {
                state.selectedCharacterPart = part;
                state.selectedObject3D = part;
            }
        }

        if (state.selectedCharacterPart) {
            ImGui::Separator();
            ImGui::BeginDisabled(state.characterDesignLocked);
            ImGui::Text("Editing: %s", state.selectedCharacterPart->getName().c_str());
            glm::vec3 dims = state.selectedCharacterPart->getDimensions();
            float dimArr[3] = {dims.x, dims.y, dims.z};
            if (ImGui::SliderFloat3("Dimensions", dimArr, 0.05f, 1.0f, "%.2f")) {
                state.selectedCharacterPart->setDimensions({dimArr[0], dimArr[1], dimArr[2]});
                state.selectedCharacterPart->setTransform(state.selectedCharacterPart->getTransform());
            }
            float color[3] = {
                state.selectedCharacterPart->getColor()[0],
                state.selectedCharacterPart->getColor()[1],
                state.selectedCharacterPart->getColor()[2]
            };
            if (ImGui::ColorEdit3("Color", color)) {
                state.selectedCharacterPart->setColor(color[0], color[1], color[2]);
            }
            ImGui::EndDisabled();
        }
    }

} // namespace Rendering
