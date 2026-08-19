#include "CreatorConsoleState.hpp"
#include "CreatorConsoleWindow.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Singularity/Input/MouseHandler.hpp"
#include <cstring>
#include <filesystem>
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
            ctx.worldTime = nullptr; // filled by callers that keep a live double
            ctx.unpackForAuthoring = mgr.getSaveLoadState().unpackForAuthoring;
            return ctx;
        }
    }

    void renderSaveLoadWindows(Core::Engine* engine) {
        auto& sl = mgr.getSaveLoadState();

        if (sl.showSaveWindow) {
            ImGui::SetNextWindowSize(ImVec2(420, 180), ImGuiCond_FirstUseEver);
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
            }
            ImGui::End();
        }

        if (sl.showLoadWindow) {
            ImGui::SetNextWindowSize(ImVec2(520, 360), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Load World", &sl.showLoadWindow)) {
                if (ImGui::Button("Refresh")) mgr.updateSaveFiles();
                ImGui::SameLine();
                ImGui::Checkbox("Unpack for authoring", &sl.unpackForAuthoring);
                ImGui::Separator();
                if (sl.files.empty()) {
                    ImGui::TextDisabled("No world saves in saves/worlds/.");
                }
                for (const auto& path : sl.files) {
                    std::string label = std::filesystem::path(path).filename().string();
                    if (ImGui::Selectable(label.c_str(), sl.loadedSaveName == path)) {
                        sl.loadedSaveName = path;
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", path.c_str());
                }
                ImGui::Separator();
                if (ImGui::Button("Load selected") && engine && !sl.loadedSaveName.empty()) {
                    SaveContext ctx = makeSaveContext(engine);
                    double t = engine->getWorldTime();
                    ctx.worldTime = &t;
                    ctx.unpackForAuthoring = sl.unpackForAuthoring;
                    mgr.loadState(sl.loadedSaveName, ctx);
                    sl.showLoadWindow = false;
                }
                if (!sl.lastLoadReport.empty()) {
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
                auto meta = SaveSystem::getSaveMetadata(SaveSystem::SaveType::WORLD);
                if (meta.empty()) {
                    ImGui::TextDisabled("No world saves.");
                }
                for (const auto& m : meta) {
                    ImGui::PushID(m.fullPath.c_str());
                    ImGui::TextUnformatted(m.filename.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("%zu bytes", m.fileSize);
                    if (ImGui::SmallButton("Load") && engine) {
                        SaveContext ctx = makeSaveContext(engine);
                        double t = engine->getWorldTime();
                        ctx.worldTime = &t;
                        mgr.loadState(m.fullPath, ctx);
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Backup")) {
                        SaveSystem::createBackup(m.fullPath, SaveSystem::SaveType::WORLD);
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Delete")) {
                        std::error_code ec;
                        std::filesystem::remove(m.fullPath, ec);
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
            double t = engine->getWorldTime();
            ctx.worldTime = &t;
            mgr.saveStateWithLog(saveName, ctx);
        }
        ImGui::SameLine();
        if (ImGui::Button("Save As")) {
            auto& sl = mgr.getSaveLoadState();
            sl.showSaveWindow = true;
            if (saveName[0] != '\0') {
                std::strncpy(sl.customName, saveName, sizeof(sl.customName) - 1);
                sl.customName[sizeof(sl.customName) - 1] = '\0';
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            mgr.updateSaveFiles();
            auto& sl = mgr.getSaveLoadState();
            sl.showLoadWindow = true;
            if (saveName[0] != '\0') {
                std::strncpy(sl.customName, saveName, sizeof(sl.customName) - 1);
                sl.customName[sizeof(sl.customName) - 1] = '\0';
            }
        }
        if (ImGui::Button("Save Manager")) {
            mgr.updateSaveFiles();
            mgr.getSaveLoadState().showManager = true;
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
