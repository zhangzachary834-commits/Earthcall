#include "CreatorConsoleState.hpp"
#include "Person/Person.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include <imgui.h>
#include <cstring>

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

        // Person ID is read-only (what this Person IS)
        if (player->hasIdentity()) {
            ImGui::Text("Person ID: %s", player->personId().toString().c_str());
        } else {
            ImGui::TextDisabled("Person ID: (unauthenticated)");
        }

        // Editable Display Name (what this Person is CALLED)
        static char nameBuf[128] = "";
        static Person* lastPerson = nullptr;
        static std::string lastSyncedName = "";
        static std::string renameStatus = "";

        if (player != lastPerson || (player->getDisplayName() != lastSyncedName && !ImGui::IsItemActive())) {
            std::strncpy(nameBuf, player->getDisplayName().c_str(), sizeof(nameBuf) - 1);
            nameBuf[sizeof(nameBuf) - 1] = '\0';
            lastPerson = player;
            lastSyncedName = player->getDisplayName();
            renameStatus.clear();
        }

        ImGui::SetNextItemWidth(200.0f);
        bool enterPressed = ImGui::InputText("Name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (ImGui::Button("Rename") || enterPressed) {
            std::string newName(nameBuf);
            if (newName.empty() || newName == "Player" || newName == "player") {
                newName = "Person";
            }
            if (newName != player->getDisplayName()) {
                player->rename(newName);
                lastSyncedName = player->getDisplayName();
                std::strncpy(nameBuf, lastSyncedName.c_str(), sizeof(nameBuf) - 1);
                nameBuf[sizeof(nameBuf) - 1] = '\0';
                renameStatus = "Renamed to " + newName;
            }
        }
        if (!renameStatus.empty()) {
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "%s", renameStatus.c_str());
        }

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
                state.selectedObject3D = part->getPrimaryObject();
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
