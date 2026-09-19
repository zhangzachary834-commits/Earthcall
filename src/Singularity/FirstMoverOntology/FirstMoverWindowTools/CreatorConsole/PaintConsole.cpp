#include "CreatorConsoleState.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <imgui.h>

namespace Rendering {

    void renderPaintConsole(ZoneManager& /*zoneMgr*/) {
        ImGui::TextUnformatted("Paint (Disabled)");
        ImGui::Separator();
        ImGui::TextWrapped(
            "2D design tools were detached from Zone in the Game.hpp split "
            "and have no owner. The belt is not shown because none of it fires.");
        ImGui::TextDisabled("Same situation as Professional 2D Design (Disabled).");
    }

} // namespace Rendering
