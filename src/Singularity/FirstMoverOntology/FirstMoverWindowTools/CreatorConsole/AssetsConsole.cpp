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
#include <map>
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

        struct WorldSave {
            std::string label;
            std::string path;
        };

        // One row per world. json + ecsave + _delta are the same save written
        // three ways; listing them as independent loads is how none of them
        // felt like they worked. Prefer the readable json; never offer delta.
        std::vector<WorldSave> listWorldSaves() {
            mgr.updateSaveFiles();
            std::map<std::string, WorldSave> byStem;
            for (const auto& path : mgr.getSaveLoadState().files) {
                std::filesystem::path p(path);
                const std::string stem = p.stem().string();
                const std::string ext = p.extension().string();
                if (stem.size() >= 6 && stem.compare(stem.size() - 6, 6, "_delta") == 0)
                    continue;
                std::error_code ec;
                if (!std::filesystem::exists(p, ec)) continue;
                if (std::filesystem::file_size(p, ec) == 0) continue;
                WorldSave row{stem, path};
                auto it = byStem.find(stem);
                if (it == byStem.end()) {
                    byStem.emplace(stem, std::move(row));
                } else if (ext == ".json") {
                    it->second.path = path;
                }
            }
            std::vector<WorldSave> out;
            out.reserve(byStem.size());
            for (auto& kv : byStem) out.push_back(std::move(kv.second));
            return out;
        }

        void loadWorld(Core::Engine* engine, const std::string& path) {
            if (!engine || path.empty()) return;
            SaveContext ctx = makeSaveContext(engine);
            mgr.loadState(path, ctx);
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
                ImGui::TextDisabled("One entry per world. Binary twins and delta chunks are not listed.");
                auto worlds = listWorldSaves();
                if (worlds.empty()) {
                    ImGui::TextDisabled("No world saves in saves/worlds/.");
                }
                for (const auto& w : worlds) {
                    ImGui::PushID(w.path.c_str());
                    if (ImGui::Button("Load")) {
                        loadWorld(engine, w.path);
                    }
                    ImGui::SameLine();
                    ImGui::TextUnformatted(w.label.c_str());
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
                auto meta = SaveSystem::getSaveMetadata(SaveSystem::SaveType::WORLD);
                if (meta.empty()) {
                    ImGui::TextDisabled("No world saves.");
                }
                for (const auto& m : meta) {
                    if (m.filename.find("_delta") != std::string::npos) continue;
                    ImGui::PushID(m.fullPath.c_str());
                    ImGui::TextUnformatted(m.filename.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("%zu bytes", m.fileSize);
                    if (ImGui::SmallButton("Load") && engine) {
                        loadWorld(engine, m.fullPath);
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
        ImGui::TextDisabled("Click Load on a name. json/ecsave/_delta of the same world are one entry.");
        {
            auto worlds = listWorldSaves();
            if (worlds.empty()) {
                ImGui::TextDisabled("No world saves in saves/worlds/ yet.");
            }
            for (const auto& w : worlds) {
                ImGui::PushID(w.path.c_str());
                if (ImGui::Button("Load") && engine) {
                    loadWorld(engine, w.path);
                }
                ImGui::SameLine();
                ImGui::TextUnformatted(w.label.c_str());
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
