#include "CreatorConsoleState.hpp"
#include "CreatorConsoleWindow.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Singular/Object/Creation/ObjectConcept.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include <filesystem>
#include <vector>
#include <string>
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
        auto& state = getCreatorConsoleState();
        ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.95f, 1.0f), "Creator Asset Browser");
        ImGui::Separator();

        // 1. Materials Library
        if (ImGui::CollapsingHeader("Materials & Shaders", ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto& mats = materials.getAll();
            if (mats.empty()) {
                ImGui::TextDisabled("No materials registered.");
            } else {
                for (const auto& m : mats) {
                    if (!m) continue;
                    ImGui::PushID(m.get());
                    ImVec4 col(m->baseColor.r, m->baseColor.g, m->baseColor.b, 1.0f);
                    ImGui::ColorButton("##swatch", col, ImGuiColorEditFlags_NoTooltip, ImVec2(18, 18));
                    ImGui::SameLine();
                    ImGui::Text("%s", m->name().c_str());

                    if (state.selectedObject3D) {
                        ImGui::SameLine(180.0f);
                        if (ImGui::SmallButton("Apply to Selection")) {
                            if (auto objMat = state.selectedObject3D->ownMaterial()) {
                                objMat->baseColor = m->baseColor;
                                objMat->opacity = m->opacity;
                                objMat->shininess = m->shininess;
                                objMat->specular = m->specular;
                                objMat->ambient = m->ambient;
                                objMat->diffuse = m->diffuse;
                                objMat->setName(m->name());
                            }
                        }
                    }
                    ImGui::PopID();
                }
            }
        }

        ImGui::Spacing();

        // 2. Prefabs & Concepts Library
        if (ImGui::CollapsingHeader("Captured Concepts & Prefabs", ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto& concepts = ConceptRegistry::instance().getAll();
            if (concepts.empty()) {
                ImGui::TextDisabled("No concepts captured yet. Capture concepts via Set-to-Set Creation [F9].");
            } else {
                for (const auto& c : concepts) {
                    if (!c) continue;
                    ImGui::PushID(c->getIdentifier().c_str());
                    ImGui::BulletText("%s", c->name().c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%zu members, %zu relations)", c->members().size(), c->relationTemplates().size());
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Instantiate in Zone")) {
                        glm::vec3 spawnPos(0.0f, 1.0f, -2.5f);
                        if (engine && engine->getCamera()) {
                            spawnPos = engine->getCamera()->getPos() + engine->getCamera()->getFront() * 3.0f;
                        }
                        glm::mat4 placement = glm::translate(glm::mat4(1.0f), spawnPos);
                        auto newborns = c->instantiate(placement);
                        for (auto& newborn : newborns) {
                            mgr.active().addObject(std::move(newborn));
                        }
                    }
                    ImGui::PopID();
                }
            }
        }

        ImGui::Spacing();

        // 3. Registered Laws Inspector
        if (ImGui::CollapsingHeader("Registered Laws")) {
            LawManager* laws = engine ? engine->getLawManager() : nullptr;
            if (!laws || laws->getAll().empty()) {
                ImGui::TextDisabled("No laws registered.");
            } else {
                for (const auto& law : laws->getAll()) {
                    if (!law) continue;
                    ImGui::PushID(law.get());
                    bool enabled = law->isEnabled();
                    if (ImGui::Checkbox(law->getIdentifier().c_str(), &enabled)) {
                        law->setEnabled(enabled);
                    }
                    ImGui::PopID();
                }
            }
        }

        ImGui::Spacing();

        // 4. Legacy Session Management (Recovery & Export)
        if (ImGui::CollapsingHeader("Legacy Session Management (Migration & Recovery)")) {
            static char saveName[128] = "legacy_session";
            ImGui::InputText("Session Name", saveName, IM_ARRAYSIZE(saveName));

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
            ImGui::TextUnformatted("Import Legacy Session:");
            {
                auto worlds = SaveSystem::listWorlds(SaveSystem::SaveType::WORLD);
                if (worlds.empty()) {
                    ImGui::TextDisabled("No legacy session files in saves/worlds/.");
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
            if (ImGui::Button("Open Legacy Session Manager Window")) {
                mgr.updateSaveFiles();
                mgr.getSaveLoadState().showManager = true;
                if (engine) engine->ensureCursorUnlocked();
            }
        }
    }

} // namespace Rendering
