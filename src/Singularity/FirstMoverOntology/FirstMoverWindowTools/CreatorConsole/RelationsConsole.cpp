#include "CreatorConsoleState.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Relation/Relation.hpp"
#include <imgui.h>
#include <vector>
#include <string>
#include <algorithm>

namespace Rendering {

    namespace {
        Singular* resolveBeing(const std::string& id) {
            if (id.empty()) return nullptr;
            for (Singular* b : Universe::instance().beings()) {
                if (b && b->getIdentifier() == id) return b;
            }
            return nullptr;
        }
    }

    void renderRelationsConsole(ZoneManager& zoneMgr) {
        auto& state = getCreatorConsoleState();
        ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.95f, 1.0f), "Ontological Relations & Structure");
        ImGui::Separator();

        // 1. Creation of New Relations
        if (ImGui::CollapsingHeader("Author New Relation", ImGuiTreeNodeFlags_DefaultOpen)) {
            static char s_endpointA[128] = "";
            static char s_endpointB[128] = "";
            static char s_relType[64] = "instance-of";
            static bool s_directed = true;
            static std::string s_creationStatus;

            // Autofill Endpoint A from current selection if empty
            if (s_endpointA[0] == '\0' && state.selectedObject3D) {
                std::snprintf(s_endpointA, sizeof(s_endpointA), "%s", state.selectedObject3D->getIdentifier().c_str());
            }

            ImGui::SetNextItemWidth(200.0f);
            ImGui::InputText("Endpoint A", s_endpointA, sizeof(s_endpointA));
            if (state.selectedObject3D) {
                ImGui::SameLine();
                if (ImGui::SmallButton("Use Selected##A")) {
                    std::snprintf(s_endpointA, sizeof(s_endpointA), "%s", state.selectedObject3D->getIdentifier().c_str());
                }
            }

            ImGui::SetNextItemWidth(200.0f);
            ImGui::InputText("Endpoint B", s_endpointB, sizeof(s_endpointB));

            ImGui::SetNextItemWidth(200.0f);
            ImGui::InputText("Relation Kind", s_relType, sizeof(s_relType));
            ImGui::SameLine();
            ImGui::Checkbox("Directed (A -> B)", &s_directed);

            // Quick preset chips for common relation kinds
            ImGui::TextDisabled("Presets:");
            ImGui::SameLine();
            if (ImGui::SmallButton("instance-of")) std::snprintf(s_relType, sizeof(s_relType), "instance-of");
            ImGui::SameLine();
            if (ImGui::SmallButton("part-of")) std::snprintf(s_relType, sizeof(s_relType), "part-of");
            ImGui::SameLine();
            if (ImGui::SmallButton("authored-by")) std::snprintf(s_relType, sizeof(s_relType), "authored-by");
            ImGui::SameLine();
            if (ImGui::SmallButton("attachment")) std::snprintf(s_relType, sizeof(s_relType), "attachment");

            if (ImGui::Button("Create Relation in Zone")) {
                Singular* a = resolveBeing(s_endpointA);
                Singular* b = resolveBeing(s_endpointB);
                if (!a) {
                    s_creationStatus = "Failed: Endpoint A '" + std::string(s_endpointA) + "' not found in Universe.";
                } else if (!b) {
                    s_creationStatus = "Failed: Endpoint B '" + std::string(s_endpointB) + "' not found in Universe.";
                } else if (s_relType[0] == '\0') {
                    s_creationStatus = "Failed: Relation kind must not be empty.";
                } else {
                    auto newRel = std::make_shared<Relation>(s_relType, *a, *b, s_directed);
                    zoneMgr.active().formation().relations().add(newRel);
                    s_creationStatus = "Created: " + newRel->getIdentifier();
                    s_endpointB[0] = '\0';
                }
            }

            if (!s_creationStatus.empty()) {
                ImGui::TextColored(s_creationStatus.rfind("Failed", 0) == 0 ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.4f, 0.9f, 0.4f, 1.0f),
                                   "%s", s_creationStatus.c_str());
            }
        }

        ImGui::Spacing();

        // 2. Active Zone Relations List & Filter
        if (ImGui::CollapsingHeader("Relations in Active Zone", ImGuiTreeNodeFlags_DefaultOpen)) {
            static char s_filterBuf[64] = "";
            ImGui::SetNextItemWidth(220.0f);
            ImGui::InputTextWithHint("##relFilter", "Filter by kind or entity ID...", s_filterBuf, sizeof(s_filterBuf));
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear##Filter")) {
                s_filterBuf[0] = '\0';
            }

            auto& relMgr = zoneMgr.active().formation().relations();
            const auto& rels = relMgr.getAll();
            if (rels.empty()) {
                ImGui::TextDisabled("No relations currently recorded in the active zone.");
            } else {
                ImGui::TextDisabled("%zu total relation(s)", rels.size());

                std::shared_ptr<Relation> relToRemove = nullptr;
                std::string filterLower(s_filterBuf);
                std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

                for (const auto& r : rels) {
                    if (!r) continue;
                    const std::string rId = r->getIdentifier();
                    std::string rLower = rId;
                    std::transform(rLower.begin(), rLower.end(), rLower.begin(), ::tolower);

                    if (!filterLower.empty() && rLower.find(filterLower) == std::string::npos) {
                        continue;
                    }

                    ImGui::PushID(r.get());
                    ImGui::BulletText("%s", r->type.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", r->aId().c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("%s", r->directed ? " -> " : " <-> ");
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.9f, 0.75f, 0.5f, 1.0f), "%s", r->bId().c_str());

                    ImGui::SameLine();
                    if (ImGui::SmallButton("Break")) {
                        relToRemove = r;
                    }
                    ImGui::PopID();
                }

                if (relToRemove) {
                    relMgr.remove(relToRemove);
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("Open Law Author Window")) {
            state.showLawAuthor = true;
        }
    }

} // namespace Rendering
