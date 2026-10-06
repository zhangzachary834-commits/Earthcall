#include "CreatorConsoleState.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Object/ObjectTypes.hpp"
#include "Relation/Relation.hpp"
#include "Relation/Formation/Formation.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <algorithm>

namespace Rendering {

    static char s_newZoneName[128] = "";
    static int s_newZoneKindIdx = 0; // 0: standard, 1: home, 2: community-zone
    static char s_zoneSearchFilter[64] = "";
    static char s_renameBuf[128] = "";
    static char s_forkZoneName[128] = "";
    static char s_objectSearchFilter[64] = "";
    static int s_selectedZone = -1;
    static int s_lastSelectedZoneForRename = -2;
    static std::string s_zonePersistenceStatus;
    static bool s_hasDiff = false;
    static nlohmann::json s_lastDiff;

    namespace {
        const char* shapeKindToString(ObjectTypes::ShapeKind kind) {
            switch (kind) {
                case ObjectTypes::ShapeKind::Cube: return "Cube";
                case ObjectTypes::ShapeKind::Polyhedron: return "Polyhedron";
                case ObjectTypes::ShapeKind::Sphere: return "Sphere";
                case ObjectTypes::ShapeKind::Cylinder: return "Cylinder";
                case ObjectTypes::ShapeKind::Cone: return "Cone";
                case ObjectTypes::ShapeKind::Ellipsoid: return "Ellipsoid";
                case ObjectTypes::ShapeKind::Ovoid: return "Ovoid";
                case ObjectTypes::ShapeKind::Paraboloid: return "Paraboloid";
                case ObjectTypes::ShapeKind::Torus: return "Torus";
                case ObjectTypes::ShapeKind::RoundedBox: return "RoundedBox";
                case ObjectTypes::ShapeKind::Field: return "Field";
                case ObjectTypes::ShapeKind::Patch: return "Patch";
                case ObjectTypes::ShapeKind::Shape2D: return "Shape2D";
                case ObjectTypes::ShapeKind::Text2D: return "Text2D";
                default: return "Geometry";
            }
        }
    }

    void renderZonesConsole(ZoneManager& zoneMgr) {
        auto& state = getCreatorConsoleState();

        ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.95f, 1.0f), "Zones of Earth — First Mover Spatial Workbench");
        ImGui::Separator();

        // 1. Top Create & Store Bar
        {
            ImGui::TextDisabled("Author New Zone:");
            ImGui::SetNextItemWidth(150.0f);
            ImGui::InputTextWithHint("##newZone", "Identifier...", s_newZoneName, IM_ARRAYSIZE(s_newZoneName));
            ImGui::SameLine();

            const char* const kindOptions[] = { "Standard", "Home", "Community" };
            ImGui::SetNextItemWidth(100.0f);
            ImGui::Combo("##zoneKind", &s_newZoneKindIdx, kindOptions, IM_ARRAYSIZE(kindOptions));
            ImGui::SameLine();

            if (ImGui::Button("Create Zone", ImVec2(90.0f, 0))) {
                std::string newId(s_newZoneName);
                if (!newId.empty()) {
                    std::string authoredKind = "";
                    if (s_newZoneKindIdx == 1) authoredKind = Zone::kHomeKind;
                    else if (s_newZoneKindIdx == 2) authoredKind = Zone::kCommunityZoneKind;

                    auto authored = zoneMgr.authorZone(newId, "first-mover", authoredKind, "");
                    if (authored) {
                        s_selectedZone = static_cast<int>(zoneMgr.zones().size()) - 1;
                        s_zonePersistenceStatus = "Created Zone '" + authored->getIdentifier() + "'.";
                    } else {
                        s_zonePersistenceStatus = "Refused: Zone '" + newId + "' collision or invalid author.";
                    }
                    s_newZoneName[0] = '\0';
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Reload Store")) {
                zoneMgr.hydrateFromZoneStore();
                s_zonePersistenceStatus = "Hydrated zones from disk store.";
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Reload all Zone identities (saves/zones/*) from disk.");
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        const auto& zones = zoneMgr.zones();
        if (zones.empty()) {
            ImGui::TextDisabled("No Zones loaded in manager.");
            return;
        }

        if (s_selectedZone < 0 || static_cast<size_t>(s_selectedZone) >= zones.size()) {
            s_selectedZone = static_cast<int>(zoneMgr.currentIndex());
        }

        // Layout sizing: determine height dynamically with sensible bounds
        float availY = ImGui::GetContentRegionAvail().y;
        float paneHeight = (availY > 260.0f) ? (availY - 30.0f) : 260.0f;
        float listWidth = 190.0f;

        // 2. Left Master Pane: Zone List & Search Filter
        ImGui::BeginChild("ZoneListPane", ImVec2(listWidth, paneHeight), true);
        {
            ImGui::SetNextItemWidth(listWidth - 45.0f);
            ImGui::InputTextWithHint("##zoneFilter", "Filter...", s_zoneSearchFilter, sizeof(s_zoneSearchFilter));
            ImGui::SameLine();
            if (ImGui::SmallButton("X##clrFilter")) {
                s_zoneSearchFilter[0] = '\0';
            }

            std::string filterLower(s_zoneSearchFilter);
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

            ImGui::Separator();
            ImGui::BeginChild("ZoneListScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

            for (size_t i = 0; i < zones.size(); ++i) {
                const auto& z = zones[i];
                if (!z) continue;

                std::string zName = z->name();
                std::string zId = z->getIdentifier();
                std::string matchStr = zName + " " + zId;
                std::transform(matchStr.begin(), matchStr.end(), matchStr.begin(), ::tolower);

                if (!filterLower.empty() && matchStr.find(filterLower) == std::string::npos) {
                    continue;
                }

                const bool selected = (static_cast<int>(i) == s_selectedZone);
                const bool isActive = (i == zoneMgr.currentIndex());

                std::string label = "";
                if (isActive) label += "[*] ";
                if (z->isPrimaryHome()) label += "[H] ";
                else if (z->isOurverseGathering()) label += "[G] ";
                else if (z->isDimensional()) label += "[D] ";

                label += zName;
                label += " (" + std::to_string(z->getOwnedObjects().size()) + ")";

                if (isActive) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.9f, 0.5f, 1.0f));
                }
                if (ImGui::Selectable(label.c_str(), selected)) {
                    s_selectedZone = static_cast<int>(i);
                }
                if (isActive) {
                    ImGui::PopStyleColor();
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Name: %s\nID: %s\nObjects: %zu\nRelations: %zu%s",
                                      zName.c_str(),
                                      zId.c_str(),
                                      z->getOwnedObjects().size(),
                                      z->formation().relations().getAll().size(),
                                      isActive ? "\n[CURRENTLY ACTIVE]" : "");
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // 3. Right Detail Pane: Rich Tabbed Inspector & Workbench
        ImGui::BeginChild("ZoneDetailsPane", ImVec2(0, paneHeight), true);
        {
            if (s_selectedZone >= 0 && static_cast<size_t>(s_selectedZone) < zones.size() && zones[s_selectedZone]) {
                const auto& z = zones[static_cast<size_t>(s_selectedZone)];
                bool isActive = (static_cast<size_t>(s_selectedZone) == zoneMgr.currentIndex());

                // Sync rename buffer on zone switch
                if (s_selectedZone != s_lastSelectedZoneForRename) {
                    std::snprintf(s_renameBuf, sizeof(s_renameBuf), "%s", z->name().c_str());
                    s_lastSelectedZoneForRename = s_selectedZone;
                    s_hasDiff = false;
                }

                // Zone Header Banner
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", z->name().c_str());
                ImGui::SameLine();
                if (isActive) {
                    ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.4f, 1.0f), "[ACTIVE]");
                } else {
                    ImGui::TextDisabled("[Background]");
                }

                // Badges
                if (z->isPrimaryHome()) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "[Primary Home]");
                } else if (z->isCommunityHome()) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "[Community Home]");
                } else if (z->isOurverseGathering()) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1.0f), "[Gathering]");
                }
                if (z->isDimensional()) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), "[Dimensional]");
                }

                ImGui::Separator();

                // Tab Bar for Zone Workspace
                if (ImGui::BeginTabBar("##ZoneWorkspaceTabs")) {

                    // TAB 1: OVERVIEW & IDENTITY
                    if (ImGui::BeginTabItem("Overview")) {
                        ImGui::Spacing();
                        ImGui::TextDisabled("Identifier:");
                        ImGui::SameLine();
                        ImGui::Text("%s", z->getIdentifier().c_str());

                        // Inline Display Name editing
                        ImGui::TextDisabled("Display Name:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(140.0f);
                        ImGui::InputText("##renameInput", s_renameBuf, sizeof(s_renameBuf));
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Apply Name")) {
                            if (s_renameBuf[0] != '\0') {
                                z->setName(s_renameBuf);
                                s_zonePersistenceStatus = "Renamed display to '" + std::string(s_renameBuf) + "'.";
                            }
                        }

                        ImGui::Spacing();
                        ImGui::TextDisabled("Owner:");
                        ImGui::SameLine();
                        ImGui::Text("%s", z->owner().empty() ? "(None / Unclaimed)" : z->owner().c_str());

                        ImGui::TextDisabled("Scope:");
                        ImGui::SameLine();
                        ImGui::Text("%s", z->scopeName().c_str());

                        // Hierarchy & Bounds
                        const std::string& parent = z->getParentZone();
                        ImGui::TextDisabled("Within (Parent):");
                        ImGui::SameLine();
                        if (!parent.empty()) {
                            ImGui::Text("%s", parent.c_str());
                            size_t pIdx = zoneMgr.findZoneIndex(parent);
                            if (pIdx != static_cast<size_t>(-1)) {
                                ImGui::SameLine();
                                if (ImGui::SmallButton("Jump to Parent")) {
                                    s_selectedZone = static_cast<int>(pIdx);
                                }
                            }
                        } else {
                            ImGui::TextDisabled("(Dimensional Root / Continuum)");
                        }

                        ImGui::Spacing();
                        ImGui::Separator();

                        // Live Performance Profiling Telemetry
                        const auto& timing = z->lastUpdateTiming();
                        ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Kernel Telemetry (Last Tick):");
                        ImGui::Text("Total Tick: %.3f ms | Substeps: %d", timing.totalMs, timing.substeps);
                        ImGui::Text("Physics: %.3f ms | Ground Scan: %.3f ms", timing.physicsMs, timing.groundScanMs);
                        ImGui::Text("Automation: %.3f ms", timing.automationMs);

                        ImGui::EndTabItem();
                    }

                    // TAB 2: BEINGS & OBJECTS
                    if (ImGui::BeginTabItem("Objects")) {
                        const auto& objs = z->getOwnedObjects();
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.8f, 0.9f, 0.95f, 1.0f), "Owned Objects (%zu):", objs.size());
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(120.0f);
                        ImGui::InputTextWithHint("##objFilter", "Filter...", s_objectSearchFilter, sizeof(s_objectSearchFilter));

                        std::string objFilterLower(s_objectSearchFilter);
                        std::transform(objFilterLower.begin(), objFilterLower.end(), objFilterLower.begin(), ::tolower);

                        ImGui::BeginChild("ZoneObjectScroll", ImVec2(0, 140.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                        if (objs.empty()) {
                            ImGui::TextDisabled("No objects residing in this zone.");
                        } else {
                            for (const auto& obj : objs) {
                                if (!obj) continue;
                                const std::string& oid = obj->getIdentifier();
                                std::string matchStr = oid;
                                std::transform(matchStr.begin(), matchStr.end(), matchStr.begin(), ::tolower);

                                if (!objFilterLower.empty() && matchStr.find(objFilterLower) == std::string::npos) {
                                    continue;
                                }

                                ImGui::PushID(obj.get());
                                glm::vec3 pos = obj->getPosition();
                                const char* shapeName = shapeKindToString(obj->getShapeKind());

                                ImGui::BulletText("%s", oid.c_str());
                                ImGui::SameLine();
                                ImGui::TextDisabled("[%s] (%.1f, %.1f, %.1f)", shapeName, pos.x, pos.y, pos.z);

                                ImGui::SameLine();
                                if (ImGui::SmallButton("Select 3D")) {
                                    state.selectedObject3D = obj.get();
                                    s_zonePersistenceStatus = "Focused '" + oid + "' in 3D Tools.";
                                }
                                ImGui::PopID();
                            }
                        }
                        ImGui::EndChild();

                        // Additional Substrate
                        ImGui::Spacing();
                        ImGui::Text("Spatial Root: %s", z->spatialRoot() ? "Active" : "None");
                        ImGui::SameLine();
                        ImGui::Text("| Extra Fields: %zu", z->additionalSpatialFields().size());
                        ImGui::SameLine();
                        ImGui::Text("| Stored Singulars: %zu", z->storedSingulars().size());

                        ImGui::EndTabItem();
                    }

                    // TAB 3: RELATIONS & FORMATIONS
                    if (ImGui::BeginTabItem("Relations")) {
                        ImGui::Spacing();
                        const auto& rels = z->formation().relations().getAll();
                        ImGui::TextColored(ImVec4(0.8f, 0.9f, 0.95f, 1.0f), "Formation Relations (%zu):", rels.size());

                        bool joyOk = z->satisfiesJoyBounds();
                        ImGui::Text("Joy Bounds: ");
                        ImGui::SameLine();
                        ImGui::TextColored(joyOk ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f) : ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
                                           "%s", joyOk ? "Harmonious (Satisfied)" : "Unsettled");

                        ImGui::BeginChild("ZoneRelScroll", ImVec2(0, 140.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                        if (rels.empty()) {
                            ImGui::TextDisabled("No relations established in this zone formation.");
                        } else {
                            for (const auto& r : rels) {
                                if (!r) continue;
                                ImGui::BulletText("%s", r->type.c_str());
                                ImGui::SameLine();
                                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", r->aId().c_str());
                                ImGui::SameLine();
                                ImGui::TextDisabled("%s", r->directed ? "->" : "<->");
                                ImGui::SameLine();
                                ImGui::TextColored(ImVec4(0.9f, 0.75f, 0.5f, 1.0f), "%s", r->bId().c_str());
                            }
                        }
                        ImGui::EndChild();

                        ImGui::EndTabItem();
                    }

                    // TAB 4: FIRST MOVER OPERATIONS (FORK, DIFF, PERSIST)
                    if (ImGui::BeginTabItem("Operations")) {
                        ImGui::Spacing();

                        // Navigation & Persistence
                        float halfBtnW = responsiveItemWidth(2, 100.0f);
                        if (halfBtnW < 110.0f) halfBtnW = 110.0f;

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
                            bool saved = isActive ? zoneMgr.persistActiveZone() : zoneMgr.persistZone(index);
                            if (saved) {
                                s_zonePersistenceStatus = "Saved Zone '" + id + "'.";
                            } else {
                                s_zonePersistenceStatus = "Save failed for Zone '" + id + "'.";
                            }
                        }

                        ImGui::Spacing();
                        ImGui::Separator();

                        // Fork / Clone Zone
                        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.5f, 1.0f), "Fork Zone (Branching & Cloning):");
                        ImGui::SetNextItemWidth(150.0f);
                        ImGui::InputTextWithHint("##forkId", "New Fork ID...", s_forkZoneName, sizeof(s_forkZoneName));
                        ImGui::SameLine();
                        if (ImGui::Button("Fork This Zone")) {
                            std::string srcId = z->getIdentifier();
                            std::string newId(s_forkZoneName);
                            if (!newId.empty()) {
                                if (zoneMgr.forkZone(srcId, newId)) {
                                    s_zonePersistenceStatus = "Forked '" + srcId + "' -> '" + newId + "'.";
                                    s_selectedZone = static_cast<int>(zoneMgr.zones().size()) - 1;
                                    s_forkZoneName[0] = '\0';
                                } else {
                                    s_zonePersistenceStatus = "Fork refused: identifier exists or invalid.";
                                }
                            }
                        }

                        ImGui::Spacing();
                        ImGui::Separator();

                        // Diff Against Active Zone
                        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.5f, 1.0f), "Diff Against Active Zone:");
                        if (isActive) {
                            ImGui::TextDisabled("This is already the active zone.");
                        } else {
                            if (ImGui::Button("Compare with Active Zone")) {
                                std::string activeId = zoneMgr.active().getIdentifier();
                                std::string compId = z->getIdentifier();
                                s_lastDiff = zoneMgr.diffZones(activeId, compId);
                                s_hasDiff = true;
                            }

                            if (s_hasDiff && !s_lastDiff.is_null()) {
                                size_t sharedCount = s_lastDiff.value("shared", nlohmann::json::array()).size();
                                size_t onlyActive = s_lastDiff.value("onlyInA", nlohmann::json::array()).size();
                                size_t onlyTarget = s_lastDiff.value("onlyInB", nlohmann::json::array()).size();
                                ImGui::Text("Shared Entities: %zu", sharedCount);
                                ImGui::Text("Only in Active: %zu | Only in Selected: %zu", onlyActive, onlyTarget);
                            }
                        }

                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
            } else {
                ImGui::TextDisabled("Select a zone from the list to view its workbench.");
            }
        }
        ImGui::EndChild();

        // 4. Global Action & Diagnostics Status Bar
        if (!s_zonePersistenceStatus.empty()) {
            ImGui::Spacing();
            const bool isSuccess = (s_zonePersistenceStatus.rfind("Created", 0) == 0 ||
                                    s_zonePersistenceStatus.rfind("Moved", 0) == 0 ||
                                    s_zonePersistenceStatus.rfind("Saved", 0) == 0 ||
                                    s_zonePersistenceStatus.rfind("Forked", 0) == 0 ||
                                    s_zonePersistenceStatus.rfind("Renamed", 0) == 0 ||
                                    s_zonePersistenceStatus.rfind("Hydrated", 0) == 0 ||
                                    s_zonePersistenceStatus.rfind("Focused", 0) == 0);
            ImGui::TextColored(isSuccess ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                               "%s", s_zonePersistenceStatus.c_str());
        }
    }

} // namespace Rendering
