#include "CreatorConsoleState.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include <imgui.h>
#include <string>

namespace Rendering {

    static char s_newZoneName[128] = "";
    static int s_selectedZone = -1;
    static std::string s_zonePersistenceStatus;

    void renderZonesConsole(ZoneManager& zoneMgr) {
        ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.95f, 1.0f), "Zones of Earth");
        ImGui::Separator();

        // 1. Create Zone
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputTextWithHint("##newZone", "New Zone Identifier...", s_newZoneName, IM_ARRAYSIZE(s_newZoneName));
        ImGui::SameLine();
        if (ImGui::Button("Create Zone")) {
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
            ImGui::TextDisabled("No Zones loaded in manager.");
            return;
        }

        if (s_selectedZone < 0 || static_cast<size_t>(s_selectedZone) >= zones.size()) {
            s_selectedZone = static_cast<int>(zoneMgr.currentIndex());
        }

        ImGui::TextDisabled(
            "%zu active Zone(s). Ordinary persistence is Zone-native.",
            zones.size());

        // 2. Zone List
        ImGui::Spacing();
        for (size_t i = 0; i < zones.size(); ++i) {
            const auto& z = zones[i];
            if (!z) continue;
            const bool selected = (static_cast<int>(i) == s_selectedZone);
            const bool isActive = (i == zoneMgr.currentIndex());

            std::string label = z->name();
            if (isActive) label += "  [Active]";

            if (ImGui::Selectable(label.c_str(), selected)) {
                s_selectedZone = static_cast<int>(i);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("ID: %s\nObjects: %zu\nRelations: %zu",
                                  z->getIdentifier().c_str(),
                                  z->getOwnedObjects().size(),
                                  z->formation().relations().getAll().size());
            }
        }

        ImGui::Separator();

        // 3. Zone Actions
        const bool selectedValid =
            s_selectedZone >= 0 && static_cast<size_t>(s_selectedZone) < zones.size() &&
            zones[static_cast<size_t>(s_selectedZone)] != nullptr;

        const float halfBtnW = responsiveItemWidth(2, 90.0f);

        if (!selectedValid) ImGui::BeginDisabled();
        if (ImGui::Button("Move to Zone", ImVec2(halfBtnW, 26.0f)) && selectedValid) {
            const size_t index = static_cast<size_t>(s_selectedZone);
            const std::string id = zones[index]->getIdentifier();
            if (zoneMgr.switchTo(index)) {
                s_zonePersistenceStatus = "Moved to Zone '" + id + "'.";
            } else {
                s_zonePersistenceStatus = "Move refused for Zone '" + id + "'.";
            }
        }
        if (!selectedValid) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Save Active Zone", ImVec2(halfBtnW, 26.0f))) {
            const std::string id = zones[zoneMgr.currentIndex()]->getIdentifier();
            if (zoneMgr.persistActiveZone()) {
                s_zonePersistenceStatus = "Saved active Zone '" + id + "'.";
            } else {
                s_zonePersistenceStatus = "Save failed for Zone '" + id + "'.";
            }
        }

        if (!s_zonePersistenceStatus.empty()) {
            ImGui::Spacing();
            const bool isSuccess = (s_zonePersistenceStatus.rfind("Created", 0) == 0 ||
                                    s_zonePersistenceStatus.rfind("Moved", 0) == 0 ||
                                    s_zonePersistenceStatus.rfind("Saved", 0) == 0);
            ImGui::TextColored(isSuccess ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                               "%s", s_zonePersistenceStatus.c_str());
        }
    }

} // namespace Rendering
