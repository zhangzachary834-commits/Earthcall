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
            ctx.player = engine->getPlayer();
            ctx.lawManager = engine->getLawManager();
            ctx.worldTime = engine->worldTimePtr();
            ctx.unpackForAuthoring = mgr.getSaveLoadState().unpackForAuthoring;
            return ctx;
        }

        void loadWorld(Core::Engine* engine, const std::string& path) {
            if (!engine || path.empty()) return;
            SaveContext ctx = makeSaveContext(engine);
            mgr.loadState(path, ctx);
            // Zones from the previous world are gone. Drop Object* the
            // tools still hold or Morph/Combine/Clay run on freed memory.
            forgetStaleObjectHandles(mgr, engine->getPlayer());
        }
    }

    void renderSaveLoadWindows(Core::Engine* engine) {
        auto& sl = mgr.getSaveLoadState();

        if (sl.showSaveWindow) {
            if (engine) engine->ensureCursorUnlocked();
            ImGui::SetNextWindowSize(ImVec2(420, 180), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(80, 80), ImGuiCond_Appearing);
            ImGui::SetNextWindowFocus();
            if (ImGui::Begin("Save As", &sl.showSaveWindow)) {
                ImGui::InputText("Name", sl.customName, IM_ARRAYSIZE(sl.customName));
                ImGui::Checkbox("Unpack for authoring", &sl.unpackForAuthoring);
                if (ImGui::Button("Save") && engine) {
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
            ImGui::SetNextWindowSize(ImVec2(520, 360), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(80, 80), ImGuiCond_Appearing);
            ImGui::SetNextWindowFocus();
            if (ImGui::Begin("Load World", &sl.showLoadWindow)) {
                if (ImGui::Button("Refresh")) mgr.updateSaveFiles();
                ImGui::SameLine();
                ImGui::Checkbox("Unpack for authoring", &sl.unpackForAuthoring);
                ImGui::Separator();
                ImGui::TextDisabled("One entry per world. Binary twins, empty files, and delta chunks are not listed.");
                {
                    const std::string stash = ZoneManager::beforeLoadSnapshotPath();
                    std::error_code ec;
                    if (!stash.empty() && std::filesystem::exists(stash, ec) &&
                        std::filesystem::file_size(stash, ec) > 0) {
                        if (ImGui::Button("Restore unsaved (before last load)")) {
                            loadWorld(engine, stash);
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%s", stash.c_str());
                        }
                        ImGui::Separator();
                    }
                }
                auto worlds = SaveSystem::listWorlds(SaveSystem::SaveType::WORLD);
                if (worlds.empty()) {
                    ImGui::TextDisabled("No world saves in saves/worlds/.");
                }
                const std::string& current = sl.loadedSaveName;
                for (const auto& w : worlds) {
                    ImGui::PushID(w.path.c_str());
                    if (ImGui::Button("Load")) {
                        loadWorld(engine, w.path);
                    }
                    ImGui::SameLine();
                    if (w.label == current) {
                        ImGui::Text("%s  (loaded)", w.label.c_str());
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
            ImGui::SetNextWindowSize(ImVec2(560, 420), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Save Manager", &sl.showManager)) {
                if (ImGui::Button("Refresh##mgr")) mgr.updateSaveFiles();
                ImGui::SameLine();
                if (ImGui::Button("Cleanup old (keep 10)")) {
                    SaveSystem::cleanupOldSaves(SaveSystem::SaveType::WORLD, 10);
                    mgr.updateSaveFiles();
                }
                ImGui::Separator();
                auto worlds = SaveSystem::listWorlds(SaveSystem::SaveType::WORLD);
                if (worlds.empty()) {
                    ImGui::TextDisabled("No world saves.");
                }
                for (const auto& w : worlds) {
                    ImGui::PushID(w.path.c_str());
                    ImGui::TextUnformatted(w.label.c_str());
                    if (ImGui::SmallButton("Load") && engine) {
                        loadWorld(engine, w.path);
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
        ImGui::TextUnformatted("Assets & Save Management");
        ImGui::Separator();

        static char saveName[128] = "my_world";
        ImGui::InputText("Save Name", saveName, IM_ARRAYSIZE(saveName));

        if (ImGui::Button("Quick Save") && engine) {
            SaveContext ctx = makeSaveContext(engine);
            mgr.saveStateWithLog(saveName, ctx);
        }
        ImGui::SameLine();
        if (ImGui::Button("Save As") && engine) {
            auto& sl = mgr.getSaveLoadState();
            if (saveName[0] != '\0') {
                std::strncpy(sl.customName, saveName, sizeof(sl.customName) - 1);
                sl.customName[sizeof(sl.customName) - 1] = '\0';
            }
            SaveContext ctx = makeSaveContext(engine);
            mgr.saveStateWithLog(sl.customName, ctx);
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Load a world");
        ImGui::TextDisabled("One name per world. json and .ecsave of the same name are the same world.");
        {
            auto worlds = SaveSystem::listWorlds(SaveSystem::SaveType::WORLD);
            if (worlds.empty()) {
                ImGui::TextDisabled("No world saves in saves/worlds/ yet.");
            }
            const std::string& current = mgr.getSaveLoadState().loadedSaveName;
            for (const auto& w : worlds) {
                ImGui::PushID(w.path.c_str());
                if (ImGui::Button("Load") && engine) {
                    loadWorld(engine, w.path);
                }
                ImGui::SameLine();
                if (w.label == current) {
                    ImGui::Text("%s  (loaded)", w.label.c_str());
                } else {
                    ImGui::TextUnformatted(w.label.c_str());
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", w.path.c_str());
                ImGui::PopID();
            }
        }
        if (ImGui::Button("Save Manager")) {
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
            ImGui::Checkbox("Unpack for authoring", &sl.unpackForAuthoring);
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
