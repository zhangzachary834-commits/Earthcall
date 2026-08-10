// GameSaveLoad.cpp – Save / Load serialization, UI dialogs, shutdown
// Split from Game.cpp during refactor.

#include "Game.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Util/SaveSystem.hpp"

#include <imgui.h>
#include <iostream>
#include <ctime>
#include <string>
#include <vector>

extern ZoneManager mgr;

namespace Core {

// ------------------------------------------------------------------
// makeSaveContext / save-load delegates to ZoneManager
// ------------------------------------------------------------------
SaveContext Game::makeSaveContext() {
    SaveContext ctx;
    ctx.camera       = &_camera;
    ctx.mouseHandler = &_mouseHandler;
    ctx.currentColor = _currentColor;
    ctx.currentTool  = &_currentTool;
    ctx.player       = &_player;
    ctx.lawManager   = &_lawManager;
    ctx.worldTime    = &_worldTime;
    return ctx;
}

void Game::saveState(const std::string& filename) {
    SaveContext ctx = makeSaveContext();
    mgr.saveState(filename, ctx);
}

void Game::loadState(const std::string& filename) {
    SaveContext ctx = makeSaveContext();
    mgr.loadState(filename, ctx);
}

void Game::saveStateWithLog(const std::string& customName) {
    SaveContext ctx = makeSaveContext();
    mgr.saveStateWithLog(customName, ctx);
}

std::vector<uint8_t> Game::buildSaveChunkFlatBuffer() {
    return mgr.buildSaveChunkFlatBuffer();
}

void Game::loadSaveChunkFlatBuffer(const std::vector<uint8_t>& buffer) {
    mgr.loadSaveChunkFlatBuffer(buffer);
}

nlohmann::json Game::buildSaveJson() const {
    SaveContext ctx = const_cast<Game*>(this)->makeSaveContext();
    return mgr.buildSaveJson(ctx);
}

// ------------------------------------------------------------------
// drawLoadWindow
// ------------------------------------------------------------------
void Game::drawLoadWindow() {
    if (!mgr.getSaveLoadState().showLoadWindow) return;
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Load Game State", &mgr.getSaveLoadState().showLoadWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Select a save file to load:");
        if (!mgr.getSaveLoadState().lastLoadReport.empty()) {
            const bool trouble =
                mgr.getSaveLoadState().lastLoadReport.find("FAIL") != std::string::npos ||
                mgr.getSaveLoadState().lastLoadReport.find("COULD NOT") != std::string::npos;
            ImGui::TextColored(trouble ? ImVec4(1.0f, 0.5f, 0.4f, 1.0f)
                                       : ImVec4(0.5f, 0.9f, 0.5f, 1.0f),
                               "%s", mgr.getSaveLoadState().lastLoadReport.c_str());
        }
        ImGui::Separator();

        // Get save metadata for better display
        auto saveMetadata = SaveSystem::getSaveMetadata(SaveSystem::SaveType::GAME);

        if (saveMetadata.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No save files found.");
        } else {
            for (const auto& meta : saveMetadata) {
                // Format timestamp for display
                std::time_t time = meta.creationTime;
                std::tm* tm = std::localtime(&time);
                char timeStr[64];
                std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);

                // Format file size
                std::string sizeStr;
                if (meta.fileSize < 1024) {
                    sizeStr = std::to_string(meta.fileSize) + " B";
                } else if (meta.fileSize < 1024 * 1024) {
                    sizeStr = std::to_string(meta.fileSize / 1024) + " KB";
                } else {
                    sizeStr = std::to_string(meta.fileSize / (1024 * 1024)) + " MB";
                }

                // Create display string
                std::string displayText = meta.customLabel.empty() ?
                    meta.filename : meta.customLabel;
                displayText += " (" + std::string(timeStr) + ", " + sizeStr + ")";

                if (ImGui::Selectable(displayText.c_str())) {
                    loadState(meta.fullPath);
                    mgr.getSaveLoadState().showLoadWindow = false;
                }

                // Show tooltip with full path
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Path: %s", meta.fullPath.c_str());
                }
            }
        }

        ImGui::Separator();

        // Add some management options
        if (ImGui::Button("Refresh")) {
            mgr.updateSaveFiles();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clean Old Saves")) {
            SaveSystem::cleanupOldSaves(SaveSystem::SaveType::GAME, 10);
            mgr.updateSaveFiles();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            mgr.getSaveLoadState().showLoadWindow = false;
        }
    }
    ImGui::End();
}

// ------------------------------------------------------------------
// drawSaveWindow
// ------------------------------------------------------------------
void Game::drawSaveWindow() {
    if (!mgr.getSaveLoadState().showSaveWindow) return;
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Save Game State", &mgr.getSaveLoadState().showSaveWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Save your current game state:");
        ImGui::Separator();

        ImGui::Text("Save Name (optional):");
        ImGui::InputText("##SaveName", mgr.getSaveLoadState().customName, sizeof(mgr.getSaveLoadState().customName));

        ImGui::Separator();

        if (ImGui::Button("Save with Timestamp")) {
            saveStateWithLog("");
            mgr.getSaveLoadState().showSaveWindow = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Save with Custom Name")) {
            saveStateWithLog(mgr.getSaveLoadState().customName);
            mgr.getSaveLoadState().showSaveWindow = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            mgr.getSaveLoadState().showSaveWindow = false;
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Saves are stored in: saves/games/");
    }
    ImGui::End();
}

// ------------------------------------------------------------------
// drawSaveManager
// ------------------------------------------------------------------
void Game::drawSaveManager() {
    if (!mgr.getSaveLoadState().showManager) return;
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Save Manager", &mgr.getSaveLoadState().showManager, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {

        // Tabs for different save types
        if (ImGui::BeginTabBar("SaveTypes")) {

            // Game saves tab
            if (ImGui::BeginTabItem("Game Saves")) {
                auto gameSaves = SaveSystem::getSaveMetadata(SaveSystem::SaveType::GAME);

                ImGui::Text("Game Saves (%zu files)", gameSaves.size());
                ImGui::Separator();

                if (gameSaves.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No game saves found.");
                } else {
                    for (const auto& meta : gameSaves) {
                        std::time_t time = meta.creationTime;
                        std::tm* tm = std::localtime(&time);
                        char timeStr[64];
                        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);

                        std::string sizeStr;
                        if (meta.fileSize < 1024) {
                            sizeStr = std::to_string(meta.fileSize) + " B";
                        } else if (meta.fileSize < 1024 * 1024) {
                            sizeStr = std::to_string(meta.fileSize / 1024) + " KB";
                        } else {
                            sizeStr = std::to_string(meta.fileSize / (1024 * 1024)) + " MB";
                        }

                        std::string displayText = meta.customLabel.empty() ?
                            meta.filename : meta.customLabel;
                        displayText += " (" + std::string(timeStr) + ", " + sizeStr + ")";
                        if (meta.isCloudOnly) displayText += " [CLOUD]";
                        else displayText += " [LOCAL]";

                        if (ImGui::Selectable(displayText.c_str())) {
                            loadState(meta.fullPath);
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Path: %s", meta.fullPath.c_str());
                        }
                    }
                }

                ImGui::Separator();
                if (ImGui::Button("Clean Old Saves")) {
                    SaveSystem::cleanupOldSaves(SaveSystem::SaveType::GAME, 10);
                }
                ImGui::SameLine();
                if (ImGui::Button("Sync & Clear Local")) {
                    std::cout << "[UI] Sync & Clear Local clicked. Cloud foundation active.\n";
                }

                ImGui::EndTabItem();
            }

            // Avatar saves tab
            if (ImGui::BeginTabItem("Avatar Saves")) {
                auto avatarSaves = SaveSystem::getSaveMetadata(SaveSystem::SaveType::AVATAR);

                ImGui::Text("Avatar Saves (%zu files)", avatarSaves.size());
                ImGui::Separator();

                if (avatarSaves.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No avatar saves found.");
                } else {
                    for (const auto& meta : avatarSaves) {
                        std::time_t time = meta.creationTime;
                        std::tm* tm = std::localtime(&time);
                        char timeStr[64];
                        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);

                        std::string sizeStr;
                        if (meta.fileSize < 1024) {
                            sizeStr = std::to_string(meta.fileSize) + " B";
                        } else if (meta.fileSize < 1024 * 1024) {
                            sizeStr = std::to_string(meta.fileSize / 1024) + " KB";
                        } else {
                            sizeStr = std::to_string(meta.fileSize / (1024 * 1024)) + " MB";
                        }

                        std::string displayText = meta.customLabel.empty() ?
                            meta.filename : meta.customLabel;
                        displayText += " (" + std::string(timeStr) + ", " + sizeStr + ")";

                        if (ImGui::Selectable(displayText.c_str())) {
                            // TODO: Load avatar save
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Path: %s", meta.fullPath.c_str());
                        }
                    }
                }

                ImGui::Separator();
                if (ImGui::Button("Clean Old Saves")) {
                    SaveSystem::cleanupOldSaves(SaveSystem::SaveType::AVATAR, 10);
                }

                ImGui::EndTabItem();
            }

            // Design saves tab
            if (ImGui::BeginTabItem("Design Saves")) {
                auto designSaves = SaveSystem::getSaveMetadata(SaveSystem::SaveType::DESIGN);

                ImGui::Text("Design Saves (%zu files)", designSaves.size());
                ImGui::Separator();

                if (designSaves.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No design saves found.");
                } else {
                    for (const auto& meta : designSaves) {
                        std::time_t time = meta.creationTime;
                        std::tm* tm = std::localtime(&time);
                        char timeStr[64];
                        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);

                        std::string sizeStr;
                        if (meta.fileSize < 1024) {
                            sizeStr = std::to_string(meta.fileSize) + " B";
                        } else if (meta.fileSize < 1024 * 1024) {
                            sizeStr = std::to_string(meta.fileSize / 1024) + " KB";
                        } else {
                            sizeStr = std::to_string(meta.fileSize / (1024 * 1024)) + " MB";
                        }

                        std::string displayText = meta.customLabel.empty() ?
                            meta.filename : meta.customLabel;
                        displayText += " (" + std::string(timeStr) + ", " + sizeStr + ")";

                        if (ImGui::Selectable(displayText.c_str())) {
                            // TODO: Load design save
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Path: %s", meta.fullPath.c_str());
                        }
                    }
                }

                ImGui::Separator();
                if (ImGui::Button("Clean Old Saves")) {
                    SaveSystem::cleanupOldSaves(SaveSystem::SaveType::DESIGN, 10);
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();
        if (ImGui::Button("Close")) {
            mgr.getSaveLoadState().showManager = false;
        }
    }
    ImGui::End();
}

} // namespace Core
