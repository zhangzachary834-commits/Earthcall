#include "CreatorConsoleState.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <imgui.h>

namespace Rendering {

    void renderZonesConsole(ZoneManager& zoneMgr) {
        ImGui::TextUnformatted("Zones");
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
