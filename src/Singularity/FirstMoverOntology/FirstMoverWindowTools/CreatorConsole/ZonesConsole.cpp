#include "CreatorConsoleState.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <imgui.h>
#include <string>

namespace Rendering {

    static char s_newZoneName[128] = "";
    static int s_selectedZone = -1;
    static std::string s_zonePersistenceStatus;

    void renderZonesConsole(ZoneManager& zoneMgr) {
        ImGui::TextUnformatted("Zones");
        ImGui::Separator();

        ImGui::InputText("New Zone Name", s_newZoneName, IM_ARRAYSIZE(s_newZoneName));
        ImGui::SameLine();
        if (ImGui::Button("Create")) {
            std::string newId(s_newZoneName);
            if (!newId.empty()) {
                auto authored = zoneMgr.authorZone(newId, "first-mover", "", "");
                if (authored) {
                    s_selectedZone = static_cast<int>(zoneMgr.zones().size()) - 1;
                    s_zonePersistenceStatus = "Created Zone '" + authored->getIdentifier() + "'.";
                }
                s_newZoneName[0] = '\0';
            }
        }
        ImGui::Separator();

        const auto& zones = zoneMgr.zones();
        if (zones.empty()) {
            ImGui::TextDisabled("No Zones loaded.");
            return;
        }

        if (s_selectedZone < 0 || static_cast<size_t>(s_selectedZone) >= zones.size()) {
            s_selectedZone = static_cast<int>(zoneMgr.currentIndex());
        }

        ImGui::TextDisabled(
            "%zu Zone(s). Select a Zone, then Move to Zone. Ordinary saving is Zone-native; legacy saves/worlds sessions are migration/recovery only.",
            zones.size());

        for (size_t i = 0; i < zones.size(); ++i) {
            const auto& z = zones[i];
            if (!z) continue;
            const bool selected = (static_cast<int>(i) == s_selectedZone);
            std::string label = z->name();
            if (i == zoneMgr.currentIndex()) label += "  [active]";
            if (ImGui::Selectable(label.c_str(), selected)) {
                s_selectedZone = static_cast<int>(i);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", z->getIdentifier().c_str());
            }
        }

        ImGui::Separator();
        const bool selectedValid =
            s_selectedZone >= 0 && static_cast<size_t>(s_selectedZone) < zones.size() &&
            zones[static_cast<size_t>(s_selectedZone)] != nullptr;

        if (!selectedValid) ImGui::BeginDisabled();
        if (ImGui::Button("Move to Zone") && selectedValid) {
            const size_t index = static_cast<size_t>(s_selectedZone);
            const std::string id = zones[index]->getIdentifier();
            if (zoneMgr.switchTo(index)) {
                s_zonePersistenceStatus = "Moved to Zone '" + id + "'.";
            } else {
                s_zonePersistenceStatus = "Move refused for Zone '" + id + "'; current Zone is unchanged.";
            }
        }
        if (!selectedValid) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Save Zone")) {
            const std::string id = zones[zoneMgr.currentIndex()]->getIdentifier();
            if (zoneMgr.persistActiveZone()) {
                s_zonePersistenceStatus = "Saved active Zone '" + id + "' only.";
            } else {
                s_zonePersistenceStatus = "Save Zone refused/failed for '" + id + "'. See console for the refusal.";
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Writes only the active Zone/Home identity and the shared Law roots it names. It does not write saves/worlds/.");
        }

        if (!s_zonePersistenceStatus.empty()) {
            ImGui::TextWrapped("%s", s_zonePersistenceStatus.c_str());
        }
    }

} // namespace Rendering