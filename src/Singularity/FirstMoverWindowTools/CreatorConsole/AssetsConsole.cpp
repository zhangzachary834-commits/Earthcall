#include "CreatorConsoleState.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "Singularity/Core/Engine.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <cstring>
#include <imgui.h>

extern ZoneManager mgr;
extern MaterialManager materials;

namespace Rendering {

    void renderAssetsConsole(Core::Engine* engine) {
        ImGui::TextUnformatted("Assets & Save Management");
        ImGui::Separator();

        static char saveName[128] = "my_world";
        ImGui::InputText("Save Name", saveName, IM_ARRAYSIZE(saveName));

        if (ImGui::Button("Save World") && engine) {
            SaveContext ctx;
            ctx.camera = engine->getCamera();
            ctx.mouseHandler = engine->getMouseHandler();
            ctx.currentColor = getCreatorConsoleState().currentColor;
            ctx.player = engine->getPlayer();
            ctx.lawManager = engine->getLawManager();
            double t = engine->getWorldTime();
            ctx.worldTime = &t;
            ctx.unpackForAuthoring = mgr.getSaveLoadState().unpackForAuthoring;
            mgr.saveStateWithLog(saveName, ctx);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load World")) {
            mgr.updateSaveFiles();
            auto& sl = mgr.getSaveLoadState();
            sl.showLoadWindow = true;
            if (saveName[0] != '\0') {
                std::strncpy(sl.customName, saveName, sizeof(sl.customName) - 1);
                sl.customName[sizeof(sl.customName) - 1] = '\0';
            }
        }

        ImGui::Separator();
        if (ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto& mats = materials.getAll();
            if (mats.empty()) {
                ImGui::TextDisabled("No materials registered.");
            } else {
                for (const auto& m : mats) {
                    if (m) ImGui::TextUnformatted(m->getIdentifier().c_str());
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Laws", ImGuiTreeNodeFlags_DefaultOpen)) {
            LawManager* laws = engine ? engine->getLawManager() : nullptr;
            if (!laws || laws->getAll().empty()) {
                ImGui::TextDisabled("No laws registered.");
            } else {
                for (const auto& law : laws->getAll()) {
                    if (!law) continue;
                    ImGui::Text("%s%s", law->getIdentifier().c_str(),
                                law->isEnabled() ? "" : " (down)");
                }
            }
            ImGui::TreePop();
        }
    }

} // namespace Rendering
