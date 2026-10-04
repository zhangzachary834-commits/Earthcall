#include "CreatorConsoleState.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include <imgui.h>
#include <string>

namespace Rendering {

    static char s_newZoneName[128] = "";
    static int s_selectedZone = -1;
    static std::string s_zonePersistenceStatus;

    void renderZonesConsole(ZoneManager& zoneMgr) {
        ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.95f, 1.0f), "Zones of Earth");
        ImGui::Separator();

        // 1. Create Zone Bar
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 120.0f);
        ImGui::InputTextWithHint("##newZone", "New Zone Identifier...", s_newZoneName, IM_ARRAYSIZE(s_newZoneName));
        ImGui::SameLine();
        if (ImGui::Button("Create Zone", ImVec2(110.0f, 0))) {
            std::string newId(s_newZoneName);
            if (!newId.empty()) {
                auto authored = zoneMgr.authorZone(newId, "first-mover", "", "");
                if (authored) {
                    s_selectedZone = static_cast<int>(zoneMgr.zones().size()) - 1;
                    s_zonePersistenceStatus = "Created Zone '" + authored->getIdentifier() + "'.";
                }
                s_newZoneName[0] = '\0';
            }
        }
        ImGui::Separator();

        const auto& zones = zoneMgr.zones();
        if (zones.empty()) {
            ImGui::TextDisabled("No Zones loaded in manager.");
            return;
        }

        if (s_selectedZone < 0 || static_cast<size_t>(s_selectedZone) >= zones.size()) {
            s_selectedZone = static_cast<int>(zoneMgr.currentIndex());
        }

        // Left pane: List of zones
        float listWidth = 180.0f; // Fixed width to prevent auto-resize bleeding
        float paneHeight = 200.0f;
        ImGui::BeginChild("ZoneListPane", ImVec2(listWidth, paneHeight), true, ImGuiWindowFlags_NoScrollbar);
        
        ImGui::TextDisabled("%zu Active Zones", zones.size());
        ImGui::Separator();
        
        ImGui::BeginChild("ZoneListScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        for (size_t i = 0; i < zones.size(); ++i) {
            const auto& z = zones[i];
            if (!z) continue;
            const bool selected = (static_cast<int>(i) == s_selectedZone);
            const bool isActive = (i == zoneMgr.currentIndex());
            
            std::string label = z->name();
            if (isActive) label = "[*] " + label;
            
            if (ImGui::Selectable(label.c_str(), selected)) {
                s_selectedZone = static_cast<int>(i);
            }
        }
        ImGui::EndChild();
        ImGui::EndChild();

        ImGui::SameLine();

        // Right pane: Details
        ImGui::BeginChild("ZoneDetailsPane", ImVec2(0, paneHeight), true);
        
        if (s_selectedZone >= 0 && static_cast<size_t>(s_selectedZone) < zones.size()) {
            const auto& z = zones[static_cast<size_t>(s_selectedZone)];
            if (z) {
                bool isActive = (static_cast<size_t>(s_selectedZone) == zoneMgr.currentIndex());
                
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", z->name().c_str());
                ImGui::TextDisabled("ID: %s", z->getIdentifier().c_str());
                
                ImGui::Spacing();
                if (isActive) {
                    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Currently Active Zone");
                } else {
                    ImGui::TextDisabled("Inactive Zone");
                }
                ImGui::Separator();
                
                // Stats
                ImGui::Text("Objects: %zu", z->getOwnedObjects().size());
                ImGui::Text("Relations: %zu", z->formation().relations().getAll().size());
                
                ImGui::Spacing();
                ImGui::Separator();
                
                // Actions
                ImGui::Text("Zone Actions:");
                ImGui::Spacing();
                
                float halfBtnW = responsiveItemWidth(2, 90.0f);
                if (halfBtnW < 120.0f) halfBtnW = 120.0f; // Ensure minimum decent width
                
                if (!isActive) {
                    if (ImGui::Button("Move to Zone", ImVec2(halfBtnW, 26.0f))) {
                        const size_t index = static_cast<size_t>(s_selectedZone);
                        const std::string id = zones[index]->getIdentifier();
                        if (zoneMgr.switchTo(index)) {
                            s_zonePersistenceStatus = "Moved to Zone '" + id + "'.";
                        } else {
                            s_zonePersistenceStatus = "Move refused for Zone '" + id + "'.";
                        }
                    }
                } else {
                    ImGui::BeginDisabled();
                    ImGui::Button("Already Here", ImVec2(halfBtnW, 26.0f));
                    ImGui::EndDisabled();
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button(isActive ? "Save Active Zone" : "Save Zone", ImVec2(halfBtnW, 26.0f))) {
                    const size_t index = static_cast<size_t>(s_selectedZone);
                    const std::string id = zones[index]->getIdentifier();
                    
                    bool saved = false;
                    if (isActive) {
                        saved = zoneMgr.persistActiveZone();
                    } else {
                        saved = zoneMgr.persistZone(index);
                    }
                    
                    if (saved) {
                        s_zonePersistenceStatus = "Saved Zone '" + id + "'.";
                    } else {
                        s_zonePersistenceStatus = "Save failed for Zone '" + id + "'.";
                    }
                }
                
                if (!isActive) {
                    ImGui::Spacing();
                    ImGui::TextWrapped("Note: Saving an inactive zone writes its native identity (saves/zones/%s/zone.json) without changing your current active view.", z->getIdentifier().c_str());
                }
            }
        } else {
            ImGui::TextDisabled("Select a zone to view details.");
        }
        
        ImGui::EndChild();
        
        if (!s_zonePersistenceStatus.empty()) {
            ImGui::Spacing();
            const bool isSuccess = (s_zonePersistenceStatus.rfind("Created", 0) == 0 ||
                                    s_zonePersistenceStatus.rfind("Moved", 0) == 0 ||
                                    s_zonePersistenceStatus.rfind("Saved", 0) == 0);
            ImGui::TextColored(isSuccess ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                               "%s", s_zonePersistenceStatus.c_str());
        }
    }

} // namespace Rendering
