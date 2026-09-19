#include "CreatorConsoleState.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <imgui.h>

namespace Rendering {

    void renderRelationsConsole(ZoneManager& zoneMgr) {
        auto& state = getCreatorConsoleState();
        ImGui::TextUnformatted("Relations");
        ImGui::Separator();
        ImGui::TextDisabled("Relations are first-class beings in the active zone.");

        if (ImGui::Button("Open Law Author")) {
            state.showLawAuthor = true;
        }

        ImGui::Separator();
        const auto& rels = zoneMgr.active().formation().relations().getAll();
        if (rels.empty()) {
            ImGui::TextDisabled("No relations in the active zone.");
            return;
        }
        ImGui::TextDisabled("%zu relation(s).", rels.size());
        for (const auto& r : rels) {
            if (!r) continue;
            ImGui::TextUnformatted(r->getIdentifier().c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(%s%s%s)",
                                r->aId().c_str(),
                                r->directed ? " -> " : " <-> ",
                                r->bId().c_str());
        }
    }

} // namespace Rendering
