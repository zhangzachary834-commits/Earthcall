#include "CreatorConsoleState.hpp"
#include "Person/Person.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Singularity/Screen/HighlightSystem.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <imgui.h>

namespace Rendering {

    static CreatorConsoleState g_consoleState;

    CreatorConsoleState& getCreatorConsoleState() {
        return g_consoleState;
    }

    void forgetStaleObjectHandles(ZoneManager& mgr, Person* player) {
        const auto live = [&](Object* p) -> Object* {
            if (!p) return nullptr;
            for (const auto& z : mgr.zones()) {
                if (!z) continue;
                for (const auto& o : z->world().getOwnedObjects()) {
                    if (o.get() == p) return p;
                }
            }
            return nullptr;
        };
        auto& s = g_consoleState;
        s.selectedObject3D = live(s.selectedObject3D);
        s.combineOperandA = live(s.combineOperandA);
        s.clayGrabbed = live(s.clayGrabbed);
        s.clayTarget = live(s.clayTarget);
        s.lastBrushObject = live(s.lastBrushObject);
        HighlightSystem::setSelected(live(HighlightSystem::getSelected()));
        if (!s.selectedObject3D) HighlightSystem::setSelectedIds({});
        if (s.selectedCharacterPart && player) {
            bool still = false;
            for (auto* part : player->getBody().parts) {
                if (part == s.selectedCharacterPart) { still = true; break; }
            }
            if (!still) s.selectedCharacterPart = nullptr;
        }
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
