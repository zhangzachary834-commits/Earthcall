#include "CreatorConsoleState.hpp"
#include "Person/Person.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include <imgui.h>
#include <cstring>
#include <vector>

namespace Rendering {

    namespace {
        const char* shapeKindLabel(ObjectTypes::ShapeKind kind) {
            switch (kind) {
                case ObjectTypes::ShapeKind::Cube: return "Cube";
                case ObjectTypes::ShapeKind::Sphere: return "Sphere";
                case ObjectTypes::ShapeKind::Cylinder: return "Cylinder";
                case ObjectTypes::ShapeKind::Cone: return "Cone";
                case ObjectTypes::ShapeKind::Ellipsoid: return "Ellipsoid";
                case ObjectTypes::ShapeKind::RoundedBox: return "Rounded Box";
                default: return "Other";
            }
        }
    }

    void renderCharacterConsole(Person* player) {
        ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.95f, 1.0f), "Character Architect Forge");
        ImGui::Separator();

        if (!player) {
            ImGui::TextDisabled("No Person present in this instance.");
            return;
        }

        auto& state = getCreatorConsoleState();
        Body& body = player->getBody();

        // 1. Identity Card
        if (player->hasIdentity()) {
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Person ID: %s", player->personId().toString().c_str());
        } else {
            ImGui::TextDisabled("Person ID: (unauthenticated)");
        }

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

        ImGui::SetNextItemWidth(180.0f);
        bool enterPressed = ImGui::InputText("Display Name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue);
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

        ImGui::Spacing();
        ImGui::Checkbox("Character Design Lock", &state.characterDesignLocked);
        if (state.characterDesignLocked) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "[LOCKED]");
        }

        if (!state.selectedCharacterPart && !body.parts.empty()) {
            state.selectedCharacterPart = body.parts.front();
        }

        ImGui::Separator();

        // 2. Hierarchical Body Parts List
        if (ImGui::CollapsingHeader("Body Anatomy & Parts", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto renderPartSelectable = [&](BodyPart* part) {
                if (!part) return;
                const bool isSel = (part == state.selectedCharacterPart);
                std::string label = part->getName();
                if (ImGui::Selectable(label.c_str(), isSel)) {
                    state.selectedCharacterPart = part;
                    state.selectedObject3D = part->getPrimaryObject();
                }
            };

            // Head & Neck
            if (ImGui::TreeNodeEx("Head & Neck", ImGuiTreeNodeFlags_DefaultOpen)) {
                for (auto* part : body.parts) {
                    if (!part) continue;
                    if (part->getType() == BodyPart::Type::Head || part->getType() == BodyPart::Type::Neck) {
                        renderPartSelectable(part);
                    }
                }
                ImGui::TreePop();
            }

            // Torso & Upper Body
            if (ImGui::TreeNodeEx("Torso & Arms", ImGuiTreeNodeFlags_DefaultOpen)) {
                for (auto* part : body.parts) {
                    if (!part) continue;
                    if (part->getType() == BodyPart::Type::Torso ||
                        part->getType() == BodyPart::Type::Shoulder ||
                        part->getType() == BodyPart::Type::Arm ||
                        part->getType() == BodyPart::Type::ForeArm ||
                        part->getType() == BodyPart::Type::Hand ||
                        part->getType() == BodyPart::Type::Finger) {
                        renderPartSelectable(part);
                    }
                }
                ImGui::TreePop();
            }

            // Legs & Lower Body
            if (ImGui::TreeNodeEx("Legs & Feet", ImGuiTreeNodeFlags_DefaultOpen)) {
                for (auto* part : body.parts) {
                    if (!part) continue;
                    if (part->getType() == BodyPart::Type::Leg ||
                        part->getType() == BodyPart::Type::ForeLeg ||
                        part->getType() == BodyPart::Type::Foot) {
                        renderPartSelectable(part);
                    }
                }
                ImGui::TreePop();
            }
        }

        // 3. Selected Part Inspector
        if (state.selectedCharacterPart) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.55f, 1.0f), "Selected Part: %s", state.selectedCharacterPart->getName().c_str());

            ImGui::BeginDisabled(state.characterDesignLocked);

            // Shape Selector
            ObjectTypes::ShapeKind curShape = state.selectedCharacterPart->getPrimaryShape();
            const char* shapes[] = {"Cube", "Sphere", "Cylinder", "Cone", "Ellipsoid", "Rounded Box"};
            const ObjectTypes::ShapeKind shapeKinds[] = {
                ObjectTypes::ShapeKind::Cube,
                ObjectTypes::ShapeKind::Sphere,
                ObjectTypes::ShapeKind::Cylinder,
                ObjectTypes::ShapeKind::Cone,
                ObjectTypes::ShapeKind::Ellipsoid,
                ObjectTypes::ShapeKind::RoundedBox
            };
            int curIdx = 0;
            for (int i = 0; i < 6; ++i) {
                if (curShape == shapeKinds[i]) curIdx = i;
            }
            if (ImGui::Combo("Part Shape", &curIdx, shapes, 6)) {
                state.selectedCharacterPart->setPrimaryShape(shapeKinds[curIdx]);
            }

            // Dimensions
            glm::vec3 dims = state.selectedCharacterPart->getDimensions();
            float dimArr[3] = {dims.x, dims.y, dims.z};
            if (ImGui::SliderFloat3("Dimensions", dimArr, 0.05f, 1.5f, "%.2f")) {
                state.selectedCharacterPart->setDimensions({dimArr[0], dimArr[1], dimArr[2]});
                state.selectedCharacterPart->setTransform(state.selectedCharacterPart->getTransform());
            }

            // Color Edit
            float color[3] = {
                state.selectedCharacterPart->getColor()[0],
                state.selectedCharacterPart->getColor()[1],
                state.selectedCharacterPart->getColor()[2]
            };
            if (ImGui::ColorEdit3("Part Color", color)) {
                state.selectedCharacterPart->setColor(color[0], color[1], color[2]);
            }

            if (ImGui::Button("Reset Dimensions to (1.0, 1.0, 1.0)")) {
                state.selectedCharacterPart->setDimensions(glm::vec3(1.0f));
                state.selectedCharacterPart->setTransform(state.selectedCharacterPart->getTransform());
            }

            ImGui::EndDisabled();
        }
    }

} // namespace Rendering
