#include "CreatorConsoleState.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <imgui.h>

namespace Rendering {

    static char s_newZoneName[128] = "";

    void renderZonesConsole(ZoneManager& zoneMgr) {
        ImGui::TextUnformatted("Zones");
        ImGui::Separator();

        ImGui::InputText("New Zone Name", s_newZoneName, IM_ARRAYSIZE(s_newZoneName));
        ImGui::SameLine();
        if (ImGui::Button("Create")) {
            std::string newId(s_newZoneName);
            if (!newId.empty()) {
                zoneMgr.authorZone(newId, "first-mover", "", "");
                s_newZoneName[0] = '\0';
            }
        }
        ImGui::Separator();

        const auto& zones = zoneMgr.zones();
        if (zones.empty()) {
            ImGui::TextDisabled("No Zones loaded.");
            return;
        }

        ImGui::TextDisabled("%zu zone(s). Click to switch. Identity lives in saves/zones/, shared across sessions.", zones.size());
        for (size_t i = 0; i < zones.size(); ++i) {
            const auto& z = zones[i];
            if (!z) continue;
            const bool active = (i == zoneMgr.currentIndex());
            if (ImGui::Selectable(z->name().c_str(), active)) {
                zoneMgr.switchTo(i);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", z->getIdentifier().c_str());
            }
        }
    }

} // namespace Rendering
