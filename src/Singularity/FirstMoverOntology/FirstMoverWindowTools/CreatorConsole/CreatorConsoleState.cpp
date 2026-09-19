#include "CreatorConsoleState.hpp"
#include "Person/Person.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Singularity/Screen/HighlightSystem.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include <imgui.h>
#include <algorithm>

namespace Rendering {

    static CreatorConsoleState g_consoleState;

    CreatorConsoleState& getCreatorConsoleState() {
        return g_consoleState;
    }

    void CreatorConsoleState::performUndo(ZoneManager& zoneMgr) {
        if (undoStack.empty()) return;
        HistoryRecord rec = std::move(undoStack.back());
        undoStack.pop_back();

        if (rec.type == HistoryActionType::Spawn) {
            if (rec.object) {
                zoneMgr.active().removeObject(rec.object.get());
                if (selectedObject3D == rec.object.get()) {
                    selectedObject3D = nullptr;
                    HighlightSystem::setSelected(nullptr);
                    HighlightSystem::setSelectedIds({});
                }
            }
            redoStack.push_back(std::move(rec));
        } else if (rec.type == HistoryActionType::Delete) {
            if (rec.object) {
                zoneMgr.active().addObject(rec.object);
                selectedObject3D = rec.object.get();
                HighlightSystem::setSelected(rec.object.get());
            }
            redoStack.push_back(std::move(rec));
        }
    }

    void CreatorConsoleState::performRedo(ZoneManager& zoneMgr) {
        if (redoStack.empty()) return;
        HistoryRecord rec = std::move(redoStack.back());
        redoStack.pop_back();

        if (rec.type == HistoryActionType::Spawn) {
            if (rec.object) {
                zoneMgr.active().addObject(rec.object);
                selectedObject3D = rec.object.get();
                HighlightSystem::setSelected(rec.object.get());
            }
            undoStack.push_back(std::move(rec));
        } else if (rec.type == HistoryActionType::Delete) {
            if (rec.object) {
                zoneMgr.active().removeObject(rec.object.get());
                if (selectedObject3D == rec.object.get()) {
                    selectedObject3D = nullptr;
                    HighlightSystem::setSelected(nullptr);
                    HighlightSystem::setSelectedIds({});
                }
            }
            undoStack.push_back(std::move(rec));
        }
    }

    void forgetStaleObjectHandles(ZoneManager& mgr, Person* player) {
        const auto live = [&](Object* p) -> Object* {
            if (!p) return nullptr;
            for (const auto& z : mgr.zones()) {
                if (!z) continue;
                for (const auto& o : z->getOwnedObjects()) {
                    if (o.get() == p) return p;
                }
            }
            return nullptr;
        };
        auto& s = g_consoleState;
        s.selectedObject3D = live(s.selectedObject3D);
        s.combineOperandA = live(s.combineOperandA);
        s.clayGrabbed = live(s.clayGrabbed);
        s.clayTarget = live(s.clayTarget);
        s.lastBrushObject = live(s.lastBrushObject);
        HighlightSystem::setSelected(live(HighlightSystem::getSelected()));
        if (!s.selectedObject3D) HighlightSystem::setSelectedIds({});
        if (s.selectedCharacterPart && player) {
            bool still = false;
            for (auto* part : player->getBody().parts) {
                if (part == s.selectedCharacterPart) { still = true; break; }
            }
            if (!still) s.selectedCharacterPart = nullptr;
        }

        const auto pruneStack = [&](std::vector<HistoryRecord>& stack) {
            stack.erase(std::remove_if(stack.begin(), stack.end(),
                                       [&](const HistoryRecord& r) {
                                           return r.object && !live(r.object.get());
                                       }),
                        stack.end());
        };
        pruneStack(s.undoStack);
        pruneStack(s.redoStack);
    }

    void pushActiveButtonStyle(bool active, const ImVec4& color, const ImVec4& hoverColor) {
        if (!active) return;
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, hoverColor);
    }

    void popActiveButtonStyle(bool active) {
        if (active) {
            ImGui::PopStyleColor(3);
        }
    }

    void sameLineEvery(int index, int perRow) {
        if ((index + 1) % perRow != 0) {
            ImGui::SameLine();
        }
    }

    float responsiveItemWidth(int columns, float minWidth) {
        if (columns <= 1) return ImGui::GetContentRegionAvail().x;
        const float avail = ImGui::GetContentRegionAvail().x;
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        float w = (avail - static_cast<float>(columns - 1) * spacing) / static_cast<float>(columns);
        return (w > minWidth) ? w : minWidth;
    }

    void responsiveSameLine(int index, int columns) {
        if (columns > 1 && ((index + 1) % columns != 0)) {
            ImGui::SameLine();
        }
    }

} // namespace Rendering
