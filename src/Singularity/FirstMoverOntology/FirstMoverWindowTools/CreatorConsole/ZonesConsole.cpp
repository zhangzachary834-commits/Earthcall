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
#include <unordered_map>
#include <cmath>
#include <cstdio>

static inline ImVec2 operator+(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x + b.x, a.y + b.y); }
static inline ImVec2 operator-(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x - b.x, a.y - b.y); }
static inline ImVec2 operator*(const ImVec2& a, float b) { return ImVec2(a.x * b, a.y * b); }

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

    // 2D Zone Canvas State
    static int s_zoneViewMode = 0; // 0: 2D Spatial Canvas, 1: Split View (Canvas + Details), 2: Inspector Only
    static ImVec2 s_canvasOffset = ImVec2(50.0f, 50.0f);
    static float s_canvasScale = 1.0f;
    static std::unordered_map<std::string, ImVec2> s_zonePositions;
    static int s_draggedNodeIdx = -1;
    static ImVec2 s_dragOffset = ImVec2(0.0f, 0.0f);
    static bool s_hasInitializedCanvas = false;

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

        void autoLayoutZones(const std::vector<std::shared_ptr<Zone>>& zones) {
            s_zonePositions.clear();
            if (zones.empty()) return;

            // Group into roots and children
            std::vector<size_t> roots;
            std::unordered_map<std::string, std::vector<size_t>> children;
            for (size_t i = 0; i < zones.size(); ++i) {
                if (!zones[i]) continue;
                std::string parent = zones[i]->getParentZone();
                if (parent.empty()) {
                    roots.push_back(i);
                } else {
                    children[parent].push_back(i);
                }
            }
            if (roots.empty()) {
                for (size_t i = 0; i < zones.size(); ++i) roots.push_back(i);
            }

            float curX = 60.0f;
            for (size_t rIdx : roots) {
                const auto& rz = zones[rIdx];
                if (!rz) continue;
                std::string rId = rz->getIdentifier();
                std::string rName = rz->name();
                float rootY = 60.0f;

                // Authored placement if dimensional
                if (rz->isDimensional()) {
                    float px = static_cast<float>(rz->placementAlong("x"));
                    float py = static_cast<float>(rz->placementAlong("z"));
                    if (std::abs(px) > 0.01f || std::abs(py) > 0.01f) {
                        s_zonePositions[rId] = ImVec2(px * 18.0f + 160.0f, py * 18.0f + 120.0f);
                        continue;
                    }
                }

                s_zonePositions[rId] = ImVec2(curX, rootY);

                // Collect children
                std::vector<size_t> myKids;
                if (children.find(rName) != children.end()) {
                    myKids.insert(myKids.end(), children[rName].begin(), children[rName].end());
                }
                if (children.find(rId) != children.end()) {
                    for (size_t k : children[rId]) {
                        if (std::find(myKids.begin(), myKids.end(), k) == myKids.end()) myKids.push_back(k);
                    }
                }

                float childY = rootY + 130.0f;
                float childStartX = curX;
                for (size_t kIdx : myKids) {
                    if (kIdx >= zones.size() || !zones[kIdx]) continue;
                    const auto& kz = zones[kIdx];
                    s_zonePositions[kz->getIdentifier()] = ImVec2(childStartX, childY);
                    childStartX += 230.0f;
                }

                float clusterW = std::max(230.0f, static_cast<float>(myKids.size()) * 230.0f);
                curX += clusterW + 40.0f;
            }
        }

        void fitAllZones(const std::vector<std::shared_ptr<Zone>>& zones, ImVec2 canvasSize) {
            if (zones.empty()) return;
            float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
            bool found = false;
            for (const auto& z : zones) {
                if (!z) continue;
                auto it = s_zonePositions.find(z->getIdentifier());
                if (it != s_zonePositions.end()) {
                    minX = std::min(minX, it->second.x);
                    minY = std::min(minY, it->second.y);
                    maxX = std::max(maxX, it->second.x + 200.0f);
                    maxY = std::max(maxY, it->second.y + 100.0f);
                    found = true;
                }
            }
            if (!found) return;

            float bboxW = std::max(100.0f, maxX - minX + 60.0f);
            float bboxH = std::max(100.0f, maxY - minY + 60.0f);

            float scaleX = (canvasSize.x - 40.0f) / bboxW;
            float scaleY = (canvasSize.y - 40.0f) / bboxH;
            s_canvasScale = std::clamp(std::min(scaleX, scaleY), 0.55f, 1.25f);

            float centerX = (minX + maxX) * 0.5f;
            float centerY = (minY + maxY) * 0.5f;
            s_canvasOffset.x = (canvasSize.x * 0.5f) - centerX * s_canvasScale;
            s_canvasOffset.y = (canvasSize.y * 0.5f) - centerY * s_canvasScale;
        }
    } // anonymous namespace

    // 2D Spatial Interactive Zone Canvas
    static void render2DZoneCanvas(ZoneManager& zoneMgr, const std::vector<std::shared_ptr<Zone>>& zones, float canvasHeight) {
        if (canvasHeight < 80.0f) canvasHeight = 80.0f;

        // Ensure all zones have a layout position
        bool needLayout = false;
        for (const auto& z : zones) {
            if (z && s_zonePositions.find(z->getIdentifier()) == s_zonePositions.end()) {
                needLayout = true;
                break;
            }
        }
        if (needLayout) {
            autoLayoutZones(zones);
        }

        ImGui::BeginChild("##Zone2DCanvasChild", ImVec2(0, canvasHeight), true,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

        ImVec2 canvasP0 = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        if (canvasSize.x < 100.0f) canvasSize.x = 100.0f;
        if (canvasSize.y < 80.0f) canvasSize.y = 80.0f;
        ImVec2 canvasP1 = ImVec2(canvasP0.x + canvasSize.x, canvasP0.y + canvasSize.y);

        if (!s_hasInitializedCanvas) {
            fitAllZones(zones, canvasSize);
            s_hasInitializedCanvas = true;
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // 1. Canvas Background & Infinite Grid
        drawList->AddRectFilled(canvasP0, canvasP1, IM_COL32(16, 19, 26, 255));

        const float gridStep = 40.0f * s_canvasScale;
        if (gridStep >= 12.0f) {
            float startX = std::fmod(s_canvasOffset.x, gridStep);
            float startY = std::fmod(s_canvasOffset.y, gridStep);
            if (startX < 0.0f) startX += gridStep;
            if (startY < 0.0f) startY += gridStep;

            for (float x = canvasP0.x + startX; x < canvasP1.x; x += gridStep) {
                drawList->AddLine(ImVec2(x, canvasP0.y), ImVec2(x, canvasP1.y), IM_COL32(255, 255, 255, 10));
            }
            for (float y = canvasP0.y + startY; y < canvasP1.y; y += gridStep) {
                drawList->AddLine(ImVec2(canvasP0.x, y), ImVec2(canvasP1.x, y), IM_COL32(255, 255, 255, 10));
            }
        }

        // Invisible button to capture input over entire canvas
        ImGui::InvisibleButton("##zoneCanvasArea", canvasSize,
                               ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
        bool isCanvasHovered = ImGui::IsItemHovered();
        const ImGuiIO& io = ImGui::GetIO();
        const ImVec2 mousePos = io.MousePos;

        // Pan with Right/Middle mouse or Left drag on empty space
        if (isCanvasHovered && (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f) ||
                                ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f) ||
                                (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) && s_draggedNodeIdx < 0))) {
            s_canvasOffset.x += io.MouseDelta.x;
            s_canvasOffset.y += io.MouseDelta.y;
        }

        // Zoom with Mouse Wheel
        if (isCanvasHovered && io.MouseWheel != 0.0f) {
            float prevScale = s_canvasScale;
            s_canvasScale = std::clamp(s_canvasScale + io.MouseWheel * 0.12f, 0.45f, 2.2f);
            ImVec2 mouseInCanvas = ImVec2(mousePos.x - canvasP0.x, mousePos.y - canvasP0.y);
            s_canvasOffset.x = mouseInCanvas.x - (mouseInCanvas.x - s_canvasOffset.x) * (s_canvasScale / prevScale);
            s_canvasOffset.y = mouseInCanvas.y - (mouseInCanvas.y - s_canvasOffset.y) * (s_canvasScale / prevScale);
        }

        auto worldToScreen = [&](const ImVec2& world) -> ImVec2 {
            return ImVec2(canvasP0.x + s_canvasOffset.x + world.x * s_canvasScale,
                          canvasP0.y + s_canvasOffset.y + world.y * s_canvasScale);
        };

        const float cardW = 195.0f * s_canvasScale;
        const float cardH = 86.0f * s_canvasScale;

        // Clip drawing to canvas rectangle
        drawList->PushClipRect(canvasP0, canvasP1, true);

        // 2. Draw Hierarchical Lines (Parent -> Child)
        for (size_t i = 0; i < zones.size(); ++i) {
            const auto& z = zones[i];
            if (!z) continue;
            const std::string& parent = z->getParentZone();
            if (!parent.empty()) {
                size_t pIdx = zoneMgr.findZoneIndex(parent);
                if (pIdx != static_cast<size_t>(-1) && pIdx < zones.size() && zones[pIdx]) {
                    const std::string& pId = zones[pIdx]->getIdentifier();
                    const std::string& cId = z->getIdentifier();
                    if (s_zonePositions.count(pId) && s_zonePositions.count(cId)) {
                        ImVec2 pPos = worldToScreen(s_zonePositions[pId]);
                        ImVec2 cPos = worldToScreen(s_zonePositions[cId]);
                        ImVec2 start = ImVec2(pPos.x + cardW * 0.5f, pPos.y + cardH);
                        ImVec2 end = ImVec2(cPos.x + cardW * 0.5f, cPos.y);
                        float dy = std::max(20.0f * s_canvasScale, (end.y - start.y) * 0.5f);
                        ImVec2 cp1 = ImVec2(start.x, start.y + dy);
                        ImVec2 cp2 = ImVec2(end.x, end.y - dy);
                        drawList->AddBezierCubic(start, cp1, cp2, end, IM_COL32(70, 140, 220, 160), 2.0f * s_canvasScale);
                        drawList->AddCircleFilled(end, 3.5f * s_canvasScale, IM_COL32(90, 170, 255, 230));
                    }
                }
            }
        }

        std::string filterLower(s_zoneSearchFilter);
        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

        // Track dragging release
        if (!ImGui::IsMouseDown(0)) {
            s_draggedNodeIdx = -1;
        }

        // 3. Render Zone Cards
        for (size_t i = 0; i < zones.size(); ++i) {
            const auto& z = zones[i];
            if (!z) continue;

            const std::string& zId = z->getIdentifier();
            const std::string& zName = z->name();

            // Ensure position exists
            if (s_zonePositions.find(zId) == s_zonePositions.end()) {
                s_zonePositions[zId] = ImVec2(60.0f + static_cast<float>(i) * 220.0f, 60.0f);
            }

            // Handle active dragging of this node
            if (s_draggedNodeIdx == static_cast<int>(i)) {
                s_zonePositions[zId] = ImVec2((mousePos.x - canvasP0.x - s_canvasOffset.x) / s_canvasScale + s_dragOffset.x,
                                              (mousePos.y - canvasP0.y - s_canvasOffset.y) / s_canvasScale + s_dragOffset.y);
            }

            ImVec2 cardPos = worldToScreen(s_zonePositions[zId]);
            ImVec2 cardEnd = ImVec2(cardPos.x + cardW, cardPos.y + cardH);

            // Skip off-screen cards
            if (cardEnd.x < canvasP0.x || cardPos.x > canvasP1.x ||
                cardEnd.y < canvasP0.y || cardPos.y > canvasP1.y) {
                continue;
            }

            bool isCurrentActive = (i == zoneMgr.currentIndex());
            bool isSelected = (static_cast<int>(i) == s_selectedZone);

            // Filter match
            bool matchesFilter = true;
            if (!filterLower.empty()) {
                std::string matchStr = zName + " " + zId;
                std::transform(matchStr.begin(), matchStr.end(), matchStr.begin(), ::tolower);
                if (matchStr.find(filterLower) == std::string::npos) {
                    matchesFilter = false;
                }
            }
            float alphaMul = matchesFilter ? 1.0f : 0.32f;

            // Hit test
            bool isCardHovered = isCanvasHovered &&
                                 (mousePos.x >= cardPos.x && mousePos.x <= cardEnd.x &&
                                  mousePos.y >= cardPos.y && mousePos.y <= cardEnd.y);

            // Card quick-switch button area (lower-right)
            float btnW = 56.0f * s_canvasScale;
            float btnH = 20.0f * s_canvasScale;
            ImVec2 btnMin = ImVec2(cardEnd.x - btnW - 6.0f * s_canvasScale, cardEnd.y - btnH - 6.0f * s_canvasScale);
            ImVec2 btnMax = ImVec2(cardEnd.x - 6.0f * s_canvasScale, cardEnd.y - 6.0f * s_canvasScale);
            bool isBtnHovered = isCardHovered && (mousePos.x >= btnMin.x && mousePos.x <= btnMax.x &&
                                                  mousePos.y >= btnMin.y && mousePos.y <= btnMax.y);

            // Interaction handling
            if (isCardHovered) {
                if (isBtnHovered && ImGui::IsMouseClicked(0)) {
                    if (zoneMgr.switchTo(i)) {
                        s_selectedZone = static_cast<int>(i);
                        s_zonePersistenceStatus = "Switched to Zone '" + zId + "'.";
                    } else {
                        s_zonePersistenceStatus = "Switch refused for Zone '" + zId + "'.";
                    }
                } else if (ImGui::IsMouseDoubleClicked(0)) {
                    if (zoneMgr.switchTo(i)) {
                        s_selectedZone = static_cast<int>(i);
                        s_zonePersistenceStatus = "Switched to Zone '" + zId + "'.";
                    } else {
                        s_zonePersistenceStatus = "Switch refused for Zone '" + zId + "'.";
                    }
                } else if (ImGui::IsMouseClicked(0)) {
                    s_selectedZone = static_cast<int>(i);
                    s_draggedNodeIdx = static_cast<int>(i);
                    s_dragOffset = ImVec2(s_zonePositions[zId].x - (mousePos.x - canvasP0.x - s_canvasOffset.x) / s_canvasScale,
                                          s_zonePositions[zId].y - (mousePos.y - canvasP0.y - s_canvasOffset.y) / s_canvasScale);
                } else if (ImGui::IsMouseClicked(1)) {
                    s_selectedZone = static_cast<int>(i);
                    ImGui::OpenPopup("##ZoneCardContextMenu");
                }
            }

            // Colors based on kind
            ImU32 accentCol = IM_COL32(95, 150, 225, static_cast<int>(255 * alphaMul));
            const char* kindLabel = "Zone";
            if (z->isPrimaryHome()) {
                accentCol = IM_COL32(245, 175, 45, static_cast<int>(255 * alphaMul));
                kindLabel = "Home";
            } else if (z->isCommunityHome()) {
                accentCol = IM_COL32(45, 205, 180, static_cast<int>(255 * alphaMul));
                kindLabel = "Comm Home";
            } else if (z->isOurverseGathering()) {
                accentCol = IM_COL32(255, 125, 45, static_cast<int>(255 * alphaMul));
                kindLabel = "Gathering";
            } else if (z->isDimensional()) {
                accentCol = IM_COL32(185, 110, 245, static_cast<int>(255 * alphaMul));
                kindLabel = "Dimension";
            }

            // Card background fill
            ImU32 bgCol = isCurrentActive
                ? IM_COL32(18, 40, 28, static_cast<int>(245 * alphaMul))
                : (isCardHovered
                    ? IM_COL32(32, 38, 50, static_cast<int>(245 * alphaMul))
                    : IM_COL32(23, 27, 35, static_cast<int>(235 * alphaMul)));
            drawList->AddRectFilled(cardPos, cardEnd, bgCol, 8.0f * s_canvasScale);

            // Card borders and glows
            if (isCurrentActive) {
                // Outer glow halo
                drawList->AddRect(cardPos - ImVec2(3.0f, 3.0f) * s_canvasScale,
                                  cardEnd + ImVec2(3.0f, 3.0f) * s_canvasScale,
                                  IM_COL32(50, 230, 130, static_cast<int>(75 * alphaMul)),
                                  10.0f * s_canvasScale, 0, 3.0f * s_canvasScale);
                // Sharp border
                drawList->AddRect(cardPos, cardEnd,
                                  IM_COL32(50, 240, 130, static_cast<int>(255 * alphaMul)),
                                  8.0f * s_canvasScale, 0, 2.5f * s_canvasScale);
            } else if (isSelected) {
                drawList->AddRect(cardPos, cardEnd,
                                  IM_COL32(95, 185, 255, static_cast<int>(255 * alphaMul)),
                                  8.0f * s_canvasScale, 0, 2.0f * s_canvasScale);
            } else {
                drawList->AddRect(cardPos, cardEnd,
                                  isCardHovered ? IM_COL32(150, 175, 210, static_cast<int>(200 * alphaMul))
                                                : IM_COL32(55, 65, 80, static_cast<int>(180 * alphaMul)),
                                  8.0f * s_canvasScale, 0, 1.2f * s_canvasScale);
            }

            // Accent top line
            drawList->AddLine(cardPos + ImVec2(8.0f * s_canvasScale, 0),
                              ImVec2(cardEnd.x - 8.0f * s_canvasScale, cardPos.y),
                              accentCol, 3.0f * s_canvasScale);

            // Internal contents with clipping
            drawList->PushClipRect(cardPos, cardEnd, true);

            // Top line: Kind badge & Status pill
            float padX = 8.0f * s_canvasScale;
            float curY = cardPos.y + 7.0f * s_canvasScale;

            // Kind text
            drawList->AddText(ImVec2(cardPos.x + padX, curY), accentCol, kindLabel);

            // Active or background badge
            if (isCurrentActive) {
                const char* activeText = "[ACTIVE]";
                ImVec2 textSize = ImGui::CalcTextSize(activeText);
                drawList->AddText(ImVec2(cardEnd.x - padX - textSize.x, curY),
                                  IM_COL32(60, 240, 140, static_cast<int>(255 * alphaMul)), activeText);
            } else {
                const char* bgText = "[BG]";
                ImVec2 textSize = ImGui::CalcTextSize(bgText);
                drawList->AddText(ImVec2(cardEnd.x - padX - textSize.x, curY),
                                  IM_COL32(140, 150, 165, static_cast<int>(170 * alphaMul)), bgText);
            }

            // Middle: Display Name
            curY += 18.0f * s_canvasScale;
            drawList->AddText(ImVec2(cardPos.x + padX, curY),
                              IM_COL32(240, 245, 250, static_cast<int>(255 * alphaMul)), zName.c_str());

            // Subtitle: @Identifier
            curY += 16.0f * s_canvasScale;
            std::string slugText = "@" + zId;
            drawList->AddText(ImVec2(cardPos.x + padX, curY),
                              IM_COL32(140, 160, 180, static_cast<int>(210 * alphaMul)), slugText.c_str());

            // Bottom row: Stats and Action Button
            curY += 18.0f * s_canvasScale;
            std::string statsStr = std::to_string(z->getOwnedObjects().size()) + " objs | " +
                                   std::to_string(z->formation().relations().getAll().size()) + " rels";
            drawList->AddText(ImVec2(cardPos.x + padX, curY),
                              IM_COL32(160, 175, 195, static_cast<int>(200 * alphaMul)), statsStr.c_str());

            // Switch button / Current indicator
            if (isCurrentActive) {
                const char* curLabel = "Current";
                ImVec2 sz = ImGui::CalcTextSize(curLabel);
                drawList->AddText(ImVec2(cardEnd.x - padX - sz.x, curY),
                                  IM_COL32(60, 230, 130, static_cast<int>(255 * alphaMul)), curLabel);
            } else {
                ImU32 btnBg = isBtnHovered
                    ? IM_COL32(60, 150, 245, static_cast<int>(245 * alphaMul))
                    : IM_COL32(40, 110, 190, static_cast<int>(210 * alphaMul));
                drawList->AddRectFilled(btnMin, btnMax, btnBg, 4.0f * s_canvasScale);
                drawList->AddRect(btnMin, btnMax, IM_COL32(110, 180, 255, static_cast<int>(220 * alphaMul)), 4.0f * s_canvasScale);

                const char* btnText = "Enter";
                ImVec2 tSz = ImGui::CalcTextSize(btnText);
                ImVec2 tPos = ImVec2(btnMin.x + (btnW - tSz.x) * 0.5f, btnMin.y + (btnH - tSz.y) * 0.5f);
                drawList->AddText(tPos, IM_COL32(255, 255, 255, static_cast<int>(255 * alphaMul)), btnText);
            }

            drawList->PopClipRect();

            // Rich Tooltip on Hover
            if (isCardHovered && s_draggedNodeIdx < 0 && !isBtnHovered) {
                ImGui::BeginTooltip();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", zName.c_str());
                ImGui::TextDisabled("Identifier: %s", zId.c_str());
                ImGui::Separator();
                ImGui::Text("Kind: %s | Scope: %s", kindLabel, z->scopeName().c_str());
                ImGui::Text("Owner: %s", z->owner().empty() ? "(None / Unclaimed)" : z->owner().c_str());
                if (!z->getParentZone().empty()) {
                    ImGui::Text("Parent: %s", z->getParentZone().c_str());
                }
                ImGui::Text("Owned Objects: %zu", z->getOwnedObjects().size());
                ImGui::Text("Formation Relations: %zu", z->formation().relations().getAll().size());
                const auto& tm = z->lastUpdateTiming();
                ImGui::TextDisabled("Telemetry: %.2f ms (substeps: %d)", tm.totalMs, tm.substeps);
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f),
                                   isCurrentActive ? "Active Zone (You are here)" : "Double-click or click [Enter] to switch active zone");
                ImGui::EndTooltip();
            }
        }

        // Context Menu Popup
        if (ImGui::BeginPopup("##ZoneCardContextMenu")) {
            if (s_selectedZone >= 0 && static_cast<size_t>(s_selectedZone) < zones.size() && zones[s_selectedZone]) {
                const auto& selZ = zones[s_selectedZone];
                bool isSelActive = (static_cast<size_t>(s_selectedZone) == zoneMgr.currentIndex());

                ImGui::TextColored(ImVec4(0.85f, 0.9f, 1.0f, 1.0f), "%s", selZ->name().c_str());
                ImGui::Separator();

                if (!isSelActive) {
                    if (ImGui::MenuItem("Enter / Switch to Zone")) {
                        if (zoneMgr.switchTo(static_cast<size_t>(s_selectedZone))) {
                            s_zonePersistenceStatus = "Switched to Zone '" + selZ->getIdentifier() + "'.";
                        }
                    }
                }

                if (ImGui::MenuItem(isSelActive ? "Save Active Zone" : "Save Zone")) {
                    bool saved = isSelActive ? zoneMgr.persistActiveZone() : zoneMgr.persistZone(static_cast<size_t>(s_selectedZone));
                    s_zonePersistenceStatus = saved ? "Saved Zone '" + selZ->getIdentifier() + "'." : "Save failed.";
                }

                if (!isSelActive && ImGui::MenuItem("Diff with Active Zone")) {
                    std::string activeId = zoneMgr.active().getIdentifier();
                    std::string compId = selZ->getIdentifier();
                    s_lastDiff = zoneMgr.diffZones(activeId, compId);
                    s_hasDiff = true;
                    s_zoneViewMode = 1; // Switch to split view to show diff
                }

                if (ImGui::MenuItem("Reset Node Position")) {
                    s_zonePositions.erase(selZ->getIdentifier());
                }
            }
            ImGui::EndPopup();
        }

        drawList->PopClipRect();

        // 4. Floating Mini-Toolbar in Top-Right
        {
            float tbW = 210.0f;
            float tbH = 26.0f;
            ImVec2 tbPos = ImVec2(canvasP1.x - tbW - 10.0f, canvasP0.y + 10.0f);
            ImGui::SetCursorScreenPos(tbPos);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.14f, 0.85f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 2.0f));
            if (ImGui::BeginChild("##CanvasToolbar", ImVec2(tbW, tbH), true, ImGuiWindowFlags_NoScrollbar)) {
                if (ImGui::SmallButton("⟲ Fit")) {
                    fitAllZones(zones, canvasSize);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Frame and center all Zones in view");

                ImGui::SameLine();
                if (ImGui::SmallButton("🔲 Auto")) {
                    autoLayoutZones(zones);
                    fitAllZones(zones, canvasSize);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Auto-organize Zones topologically");

                ImGui::SameLine();
                if (ImGui::SmallButton("-")) {
                    s_canvasScale = std::clamp(s_canvasScale - 0.15f, 0.45f, 2.2f);
                }
                ImGui::SameLine();
                char zoomStr[16];
                std::snprintf(zoomStr, sizeof(zoomStr), "%d%%", static_cast<int>(s_canvasScale * 100.0f));
                if (ImGui::SmallButton(zoomStr)) {
                    s_canvasScale = 1.0f;
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset zoom to 100%%");

                ImGui::SameLine();
                if (ImGui::SmallButton("+")) {
                    s_canvasScale = std::clamp(s_canvasScale + 0.15f, 0.45f, 2.2f);
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        ImGui::EndChild();
    }

    // Detail Tabs Inspector
    static void renderZoneDetailsTabs(ZoneManager& zoneMgr, const std::vector<std::shared_ptr<Zone>>& zones, float detailsHeight) {
        if (detailsHeight < 60.0f) detailsHeight = 60.0f;
        auto& state = getCreatorConsoleState();

        ImGui::BeginChild("ZoneDetailsPane", ImVec2(0, detailsHeight), true);
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
                        bool enterPressed = ImGui::InputText("##renameInput", s_renameBuf, sizeof(s_renameBuf), ImGuiInputTextFlags_EnterReturnsTrue);
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Apply Name") || enterPressed) {
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

                        std::vector<std::pair<Object*, size_t>> toTransfer;

                        std::string objFilterLower(s_objectSearchFilter);
                        std::transform(objFilterLower.begin(), objFilterLower.end(), objFilterLower.begin(), ::tolower);

                        ImGui::BeginChild("ZoneObjectScroll", ImVec2(0, 140.0f), true);
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
                                if (ImGui::SmallButton("Select")) {
                                    state.selectedObject3D = obj.get();
                                    s_zonePersistenceStatus = "Focused '" + oid + "' in 3D Tools.";
                                }
                                
                                ImGui::SameLine();
                                if (ImGui::SmallButton("0,0,0")) {
                                    obj->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
                                    s_zonePersistenceStatus = "Teleported '" + oid + "' to Origin.";
                                }

                                ImGui::SameLine();
                                if (ImGui::Button("Transfer...")) {
                                    ImGui::OpenPopup("TransferPopup");
                                }
                                
                                if (ImGui::BeginPopup("TransferPopup")) {
                                    ImGui::TextDisabled("Move to:");
                                    ImGui::Separator();
                                    for (size_t tIdx = 0; tIdx < zones.size(); ++tIdx) {
                                        if (tIdx == static_cast<size_t>(s_selectedZone)) continue;
                                        if (ImGui::MenuItem(zones[tIdx]->name().c_str())) {
                                            toTransfer.push_back({obj.get(), tIdx});
                                        }
                                    }
                                    ImGui::EndPopup();
                                }
                                
                                ImGui::PopID();
                            }
                        }
                        ImGui::EndChild();

                        // Apply Actions
                        if (!toTransfer.empty()) {
                            for (auto& pair : toTransfer) {
                                auto* tObj = pair.first;
                                size_t destIdx = pair.second;
                                std::shared_ptr<Object> sharedObj;
                                for (const auto& o : z->getOwnedObjects()) {
                                    if (o.get() == tObj) {
                                        sharedObj = o;
                                        break;
                                    }
                                }
                                if (sharedObj) {
                                    z->removeObject(tObj);
                                    zones[destIdx]->addObject(sharedObj);
                                    s_zonePersistenceStatus = "Transferred '" + tObj->getIdentifier() + "' to " + zones[destIdx]->name() + ".";
                                }
                            }
                        }

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

                        ImGui::BeginChild("ZoneRelScroll", ImVec2(0, 140.0f), true);
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
                        bool forkEnterPressed = ImGui::InputTextWithHint("##forkId", "New Fork ID...", s_forkZoneName, sizeof(s_forkZoneName), ImGuiInputTextFlags_EnterReturnsTrue);
                        ImGui::SameLine();
                        if (ImGui::Button("Fork This Zone") || forkEnterPressed) {
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
                ImGui::TextDisabled("Select a zone from the 2D canvas or list to view its workbench.");
            }
        }
        ImGui::EndChild();
    }

    void renderZonesConsole(ZoneManager& zoneMgr) {
        ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.95f, 1.0f), "Zones of Earth — First Mover Spatial Workbench");
        ImGui::Separator();

        // 1. Top Create & Store Bar
        {
            ImGui::TextDisabled("Author New Zone:");
            ImGui::SetNextItemWidth(150.0f);
            bool enterPressed = ImGui::InputTextWithHint("##newZone", "Identifier...", s_newZoneName, IM_ARRAYSIZE(s_newZoneName), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();

            const char* const kindOptions[] = { "Standard", "Home", "Community" };
            ImGui::SetNextItemWidth(100.0f);
            ImGui::Combo("##zoneKind", &s_newZoneKindIdx, kindOptions, IM_ARRAYSIZE(kindOptions));
            ImGui::SameLine();

            if (ImGui::Button("Create Zone", ImVec2(90.0f, 0)) || enterPressed) {
                std::string newId(s_newZoneName);
                if (!newId.empty()) {
                    std::string authoredKind = "";
                    if (s_newZoneKindIdx == 1) authoredKind = Zone::kHomeKind;
                    else if (s_newZoneKindIdx == 2) authoredKind = Zone::kCommunityZoneKind;

                    auto authored = zoneMgr.authorZone(newId, "first-mover", authoredKind, "");
                    if (authored) {
                        s_selectedZone = static_cast<int>(zoneMgr.zones().size()) - 1;
                        s_zonePersistenceStatus = "Created Zone '" + authored->getIdentifier() + "'.";
                        s_hasInitializedCanvas = false; // Trigger refit so newborn zone is framed
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
                s_hasInitializedCanvas = false;
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

        // Layout sizing: fill available height smoothly
        float availY = ImGui::GetContentRegionAvail().y;
        float paneHeight = std::max(200.0f, availY - 25.0f);
        float listWidth = 195.0f;

        // 2. Left Master Pane: Zone List & Search Filter (Scroll List at Side)
        ImGui::BeginChild("ZoneListPane", ImVec2(listWidth, paneHeight), true, ImGuiWindowFlags_NoScrollbar);
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
            ImGui::BeginChild("ZoneListScroll", ImVec2(0, 0), false);

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

                // Double-click row to immediately switch to that Zone!
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (zoneMgr.switchTo(i)) {
                        s_selectedZone = static_cast<int>(i);
                        s_zonePersistenceStatus = "Switched to Zone '" + zId + "'.";
                    }
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Name: %s\nID: %s\nObjects: %zu\nRelations: %zu%s\n\n💡 Double-click to switch to this Zone!",
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

        // 3. Right Area: View Mode Selector & Content (2D Canvas / Split / Inspector)
        ImGui::BeginGroup();
        {
            // View Mode Header
            {
                ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), "View Mode:");
                ImGui::SameLine();
                if (ImGui::RadioButton("🗺️ 2D Canvas", s_zoneViewMode == 0)) s_zoneViewMode = 0;
                ImGui::SameLine();
                if (ImGui::RadioButton("🔀 Split View", s_zoneViewMode == 1)) s_zoneViewMode = 1;
                ImGui::SameLine();
                if (ImGui::RadioButton("📋 Inspector", s_zoneViewMode == 2)) s_zoneViewMode = 2;
            }

            float headerH = 26.0f;
            float contentH = paneHeight - headerH;
            if (contentH < 100.0f) contentH = 100.0f;

            if (s_zoneViewMode == 0) {
                // 2D Spatial Canvas takes full height
                render2DZoneCanvas(zoneMgr, zones, contentH);
            } else if (s_zoneViewMode == 1) {
                // Split View: 2D Canvas on top, Inspector details on bottom
                float topCanvasH = contentH * 0.52f;
                float bottomTabsH = contentH - topCanvasH - 6.0f;
                render2DZoneCanvas(zoneMgr, zones, topCanvasH);
                ImGui::Spacing();
                renderZoneDetailsTabs(zoneMgr, zones, bottomTabsH);
            } else {
                // Classic Inspector Details only
                renderZoneDetailsTabs(zoneMgr, zones, contentH);
            }
        }
        ImGui::EndGroup();

        // 4. Global Action & Diagnostics Status Bar
        if (!s_zonePersistenceStatus.empty()) {
            ImGui::Spacing();
            const bool isSuccess = (s_zonePersistenceStatus.rfind("Created", 0) == 0 ||
                                    s_zonePersistenceStatus.rfind("Moved", 0) == 0 ||
                                    s_zonePersistenceStatus.rfind("Switched", 0) == 0 ||
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
