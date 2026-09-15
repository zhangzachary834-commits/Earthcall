#include "CreatorConsoleState.hpp"
#include "CreatorConsoleWindow.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include <cstring>
#include <filesystem>
#include <vector>
#include <imgui.h>

extern ZoneManager mgr;
extern MaterialManager materials;

namespace Rendering {

    namespace {
        SaveContext makeSaveContext(Core::Engine* engine) {
            SaveContext ctx;
            if (!engine) return ctx;
            ctx.camera = engine->getCamera();
            ctx.mouseHandler = engine->getMouseHandler();
            ctx.currentColor = getCreatorConsoleState().currentColor;
            ctx.person = engine->getPerson();
            ctx.lawManager = engine->getLawManager();
            ctx.ourverse = &engine->getOurverse();
            ctx.worldTime = engine->worldTimePtr();
            ctx.unpackForAuthoring = mgr.getSaveLoadState().unpackForAuthoring;
            return ctx;
        }

        void loadWorld(Core::Engine* engine, const std::string& path) {
            if (!engine || path.empty()) return;
            SaveContext ctx = makeSaveContext(engine);
            mgr.loadState(path, ctx);
            // Session pose changed; identity-stable Zones (Home, …) were
            // kept. Drop Object* only if that being is no longer live.
            forgetStaleObjectHandles(mgr, engine->getPerson());
        }
    }

    void renderSaveLoadWindows(Core::Engine* engine) {
        auto& sl = mgr.getSaveLoadState();

        if (sl.showSaveWindow) {
            if (engine) engine->ensureCursorUnlocked();
            ImGui::SetNextWindowSize(ImVec2(480, 220), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(80, 80), ImGuiCond_Appearing);
            ImGui::SetNextWindowFocus();
            if (ImGui::Begin("Legacy Session Export", &sl.showSaveWindow)) {
                ImGui::TextWrapped(
                    "Legacy compatibility/recovery only. Ordinary authorship saves the active Zone from Creator Console -> Zones -> Save Zone.");
                ImGui::Separator();
                ImGui::InputText("Session Name", sl.customName, IM_ARRAYSIZE(sl.customName));
                ImGui::Checkbox("Unpack for authoring", &sl.unpackForAuthoring);
                if (ImGui::Button("Export Legacy Session") && engine) {
                    SaveContext ctx = makeSaveContext(engine);
                    double t = engine->getWorldTime();
                    ctx.worldTime = &t;
                    ctx.unpackForAuthoring = sl.unpackForAuthoring;
                    mgr.saveStateWithLog(sl.customName, ctx);
                    sl.showSaveWindow = false;
                }
                if (!sl.lastSaveReport.empty()) {
                    ImGui::TextWrapped("%s", sl.lastSaveReport.c_str());
                }
            }
            ImGui::End();
        }

        if (sl.showLoadWindow) {
            if (engine) engine->ensureCursorUnlocked();
            ImGui::SetNextWindowSize(ImVec2(540, 390), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(80, 80), ImGuiCond_Appearing);
            ImGui::SetNextWindowFocus();
            if (ImGui::Begin("Legacy Session Import / Recovery", &sl.showLoadWindow)) {
                ImGui::TextWrapped(
                    "Legacy session files may restore/import old working-set and pose data. Moving among living Zones belongs in Creator Console -> Zones.");
                ImGui::Separator();
                if (ImGui::Button("Refresh")) mgr.updateSaveFiles();
                ImGui::SameLine();
                ImGui::Checkbox("Unpack for authoring", &sl.unpackForAuthoring);
                ImGui::Separator();
                ImGui::TextDisabled("Legacy sessions (camera, laws, old working-set envelope). Home and other Zones remain identity-stable across imports.");
                {
                    auto zoneIds = SaveSystem::listZoneIdentities();
                    if (!zoneIds.empty()) {
                        std::string listed = "Zones of Earth: ";
                        for (size_t i = 0; i < zoneIds.size(); ++i) {
                            if (i) listed += ", ";
                            listed += zoneIds[i];
                        }
                        ImGui::TextWrapped("%s", listed.c_str());
                    }
                }
                ImGui::Separator();
                ImGui::TextDisabled("One entry per legacy session. Binary twins, empty files, and delta chunks are not listed.");
                {
                    const std::string stash = ZoneManager::beforeLoadSnapshotPath();
                    std::error_code ec;
                    if (!stash.empty() && std::filesystem::exists(stash, ec) &&
                        std::filesystem::file_size(stash, ec) > 0) {
                        if (ImGui::Button("Restore unsaved (before last legacy import)")) {
                            loadWorld(engine, stash);
                            sl.showLoadWindow = false;
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%s", stash.c_str());
                        }
                        ImGui::Separator();
                    }
                }
                auto worlds = SaveSystem::listWorlds(SaveSystem::SaveType::WORLD);
                if (worlds.empty()) {
                    ImGui::TextDisabled("No legacy session files in saves/worlds/.");
                }
                const std::string& current = sl.loadedSaveName;
                for (const auto& w : worlds) {
                    ImGui::PushID(w.path.c_str());
                    if (ImGui::Button("Import")) {
                        loadWorld(engine, w.path);
                        sl.showLoadWindow = false;
                    }
                    ImGui::SameLine();
                    if (w.label == current) {
                        ImGui::Text("%s  (imported)", w.label.c_str());
                    } else {
                        ImGui::TextUnformatted(w.label.c_str());
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", w.path.c_str());
                    ImGui::PopID();
                }
                if (!sl.lastLoadReport.empty()) {
                    ImGui::Separator();
                    ImGui::TextWrapped("%s", sl.lastLoadReport.c_str());
                }
            }
            ImGui::End();
        }

        if (sl.showManager) {
            ImGui::SetNextWindowSize(ImVec2(580, 440), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Legacy Session Manager", &sl.showManager)) {
                ImGui::TextWrapped(
                    "Migration/recovery surface for saves/worlds/. Zone identities are managed from the Zones console.");
                ImGui::Separator();
                if (ImGui::Button("Refresh##mgr")) mgr.updateSaveFiles();
                ImGui::SameLine();
                if (ImGui::Button("Cleanup old legacy sessions (keep 10)")) {
                    SaveSystem::cleanupOldSaves(SaveSystem::SaveType::WORLD, 10);
                    mgr.updateSaveFiles();
                }
                ImGui::Separator();
                auto worlds = SaveSystem::listWorlds(SaveSystem::SaveType::WORLD);
                if (worlds.empty()) {
                    ImGui::TextDisabled("No legacy session files.");
                }
                for (const auto& w : worlds) {
                    ImGui::PushID(w.path.c_str());
                    ImGui::TextUnformatted(w.label.c_str());
                    if (ImGui::SmallButton("Import") && engine) {
                        loadWorld(engine, w.path);
                        sl.showManager = false;
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Backup")) {
                        SaveSystem::createBackup(w.path, SaveSystem::SaveType::WORLD);
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Delete")) {
                        SaveSystem::removeWorld(w.label, SaveSystem::SaveType::WORLD);
                        mgr.updateSaveFiles();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::End();
        }
    }

    void renderAssetsConsole(Core::Engine* engine) {
        ImGui::TextUnformatted("Assets & Legacy Session Tools");
        ImGui::Separator();
        ImGui::TextWrapped(
            "Ordinary save/movement is Zone-native. Use Creator Console -> Zones -> Move to Zone / Save Zone. The controls below retain saves/worlds compatibility for migration and recovery.");

        if (ImGui::TreeNode("Legacy session import/export / recovery")) {
            static char saveName[128] = "legacy_session";
            ImGui::InputText("Legacy Session Name", saveName, IM_ARRAYSIZE(saveName));

            if (ImGui::Button("Export Legacy Session") && engine) {
                SaveContext ctx = makeSaveContext(engine);
                mgr.saveStateWithLog(saveName, ctx);
            }
            ImGui::SameLine();
            if (ImGui::Button("Export As...") && engine) {
                auto& sl = mgr.getSaveLoadState();
                if (saveName[0] != '\0') {
                    std::strncpy(sl.customName, saveName, sizeof(sl.customName) - 1);
                    sl.customName[sizeof(sl.customName) - 1] = '\0';
                }
                sl.showSaveWindow = true;
                if (engine) engine->ensureCursorUnlocked();
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Import a legacy session");
            {
                auto worlds = SaveSystem::listWorlds(SaveSystem::SaveType::WORLD);
                if (worlds.empty()) {
                    ImGui::TextDisabled("No legacy session files in saves/worlds/ yet.");
                }
                const std::string& current = mgr.getSaveLoadState().loadedSaveName;
                for (const auto& w : worlds) {
                    ImGui::PushID(w.path.c_str());
                    if (ImGui::Button("Import") && engine) {
                        loadWorld(engine, w.path);
                    }
                    ImGui::SameLine();
                    if (w.label == current) {
                        ImGui::Text("%s  (imported)", w.label.c_str());
                    } else {
                        ImGui::TextUnformatted(w.label.c_str());
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", w.path.c_str());
                    ImGui::PopID();
                }
            }
            if (ImGui::Button("Legacy Session Manager")) {
                mgr.updateSaveFiles();
                mgr.getSaveLoadState().showManager = true;
                if (engine) engine->ensureCursorUnlocked();
            }

            {
                auto& sl = mgr.getSaveLoadState();
                if (!sl.lastSaveReport.empty()) {
                    ImGui::TextWrapped("%s", sl.lastSaveReport.c_str());
                }
                if (!sl.lastLoadReport.empty()) {
                    ImGui::TextWrapped("%s", sl.lastLoadReport.c_str());
                }
                ImGui::Checkbox("Unpack legacy session for authoring", &sl.unpackForAuthoring);
            }
            ImGui::TreePop();
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