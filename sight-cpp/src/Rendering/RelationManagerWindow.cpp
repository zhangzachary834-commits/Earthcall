#include "RelationManagerWindow.hpp"

#include "imgui.h"
#include <cstdio>
#include <string>

namespace Rendering {

void renderRelationManagerWindow(bool* open, const RelationManager& registry) {
    if (!open) return;

    ImGui::SetNextWindowSize(ImVec2(520, 420), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Relation Manager", open)) {
        const auto& relations = registry.getAll();

        ImGui::TextUnformatted("Source: Physics Relation Registry");
        ImGui::Separator();

        static char typeFilter[64] = "";
        static char entityFilter[64] = "";
        ImGui::InputText("Type Filter", typeFilter, IM_ARRAYSIZE(typeFilter));
        ImGui::InputText("Entity Filter", entityFilter, IM_ARRAYSIZE(entityFilter));

        ImGui::Separator();

        if (relations.empty()) {
            ImGui::TextUnformatted("No relations found.");
        } else {
            size_t matchCount = 0;
            if (ImGui::BeginListBox("##RelationList", ImVec2(0.0f, 220.0f))) {
                bool anyMatch = false;
                for (const auto& rel : relations) {
                    if (!rel) continue;
                    if (typeFilter[0] != '\0') {
                        if (rel->type.find(typeFilter) == std::string::npos) continue;
                    }
                    if (entityFilter[0] != '\0') {
                        bool matchA = rel->entityA.find(entityFilter) != std::string::npos;
                        bool matchB = rel->entityB.find(entityFilter) != std::string::npos;
                        if (!matchA && !matchB) continue;
                    }
                    ++matchCount;
                    anyMatch = true;
                    char label[256];
                    std::snprintf(label, sizeof(label), "%s: %s %s %s (w=%.2f, ev=%zu)",
                                  rel->type.c_str(),
                                  rel->entityA.c_str(),
                                  rel->directed ? "->" : "<->",
                                  rel->entityB.c_str(),
                                  rel->weight,
                                  rel->events.size());
                    ImGui::Selectable(label, false);
                }
                if (!anyMatch) {
                    ImGui::TextUnformatted("<no matches>");
                }
                ImGui::EndListBox();
            }
            ImGui::Text("Showing %zu of %zu", matchCount, relations.size());
        }
    }
    ImGui::End();
}

} // namespace Rendering

