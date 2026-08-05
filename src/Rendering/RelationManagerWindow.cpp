#include "RelationManagerWindow.hpp"

#include "imgui.h"
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <algorithm>

namespace Rendering {

struct GraphState {
    std::unordered_map<std::string, ImVec2> positions;
    std::unordered_map<std::string, ImVec2> velocities;
    std::string draggedNode;
};

static GraphState g_graphState;

static ImVec2 operator+(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x + b.x, a.y + b.y); }
static ImVec2 operator-(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x - b.x, a.y - b.y); }
static ImVec2 operator*(const ImVec2& a, float b) { return ImVec2(a.x * b, a.y * b); }
static ImVec2& operator+=(ImVec2& a, const ImVec2& b) { a.x += b.x; a.y += b.y; return a; }
static ImVec2& operator-=(ImVec2& a, const ImVec2& b) { a.x -= b.x; a.y -= b.y; return a; }

static float length(const ImVec2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }
static ImVec2 normalize(const ImVec2& v) { float len = length(v); return len > 0.0001f ? v * (1.0f / len) : ImVec2(0, 0); }

void renderRelationManagerWindow(bool* open, const RelationManager& registry) {
    if (!open) return;

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Relation Manager", open)) {
        const auto& relations = registry.getAll();
        
        ImGui::TextUnformatted("Source: Physics Relation Registry (Graph View)");
        ImGui::Separator();

        static char typeFilter[64] = "";
        static char entityFilter[64] = "";
        ImGui::InputText("Type Filter", typeFilter, IM_ARRAYSIZE(typeFilter));
        ImGui::SameLine();
        ImGui::InputText("Entity Filter", entityFilter, IM_ARRAYSIZE(entityFilter));

        ImGui::Separator();

        if (relations.empty()) {
            ImGui::TextUnformatted("No relations found.");
        } else {
            // Extract distinct nodes (entities) and filtered edges
            std::vector<std::string> nodes;
            struct Edge {
                std::string a, b, type;
                bool directed;
                float weight;
            };
            std::vector<Edge> edges;

            for (const auto& rel : relations) {
                if (!rel) continue;
                if (typeFilter[0] != '\0' && rel->type.find(typeFilter) == std::string::npos) continue;
                
                bool matchA = entityFilter[0] == '\0' || rel->entityA.find(entityFilter) != std::string::npos;
                bool matchB = entityFilter[0] == '\0' || rel->entityB.find(entityFilter) != std::string::npos;
                
                if (!matchA && !matchB) continue;

                if (std::find(nodes.begin(), nodes.end(), rel->entityA) == nodes.end()) nodes.push_back(rel->entityA);
                if (std::find(nodes.begin(), nodes.end(), rel->entityB) == nodes.end()) nodes.push_back(rel->entityB);
                
                edges.push_back({rel->entityA, rel->entityB, rel->type, rel->directed, static_cast<float>(rel->weight)});
            }

            ImGui::Text("Showing %zu relations between %zu entities", edges.size(), nodes.size());

            // Graph visualization area
            ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = ImGui::GetContentRegionAvail();
            if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;
            if (canvas_size.y < 50.0f) canvas_size.y = 50.0f;
            ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_size.x, canvas_p0.y + canvas_size.y);

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(40, 40, 45, 255));
            draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 50));

            // Invisible button over canvas to capture mouse interactions (drag/drop background)
            ImGui::InvisibleButton("canvas", canvas_size);
            bool is_hovered = ImGui::IsItemHovered();
            ImVec2 mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - canvas_p0.x, ImGui::GetIO().MousePos.y - canvas_p0.y);

            // Initialize missing node positions randomly within canvas center
            ImVec2 center = canvas_p0 + canvas_size * 0.5f;
            for (const auto& node : nodes) {
                if (g_graphState.positions.find(node) == g_graphState.positions.end()) {
                    float rx = ((rand() % 1000) / 1000.0f - 0.5f) * canvas_size.x * 0.5f;
                    float ry = ((rand() % 1000) / 1000.0f - 0.5f) * canvas_size.y * 0.5f;
                    g_graphState.positions[node] = center + ImVec2(rx, ry);
                    g_graphState.velocities[node] = ImVec2(0, 0);
                }
            }

            // Remove orphaned nodes from state
            for (auto it = g_graphState.positions.begin(); it != g_graphState.positions.end(); ) {
                if (std::find(nodes.begin(), nodes.end(), it->first) == nodes.end()) {
                    g_graphState.velocities.erase(it->first);
                    it = g_graphState.positions.erase(it);
                } else {
                    ++it;
                }
            }

            // Force-directed layout step
            const float kRepulsion = 10000.0f;
            const float kSpringLength = 150.0f;
            const float kSpringK = 0.05f;
            const float kDamping = 0.85f;
            const float dt = 0.016f;

            for (const auto& node : nodes) {
                if (g_graphState.draggedNode == node) continue;
                
                ImVec2 force(0, 0);
                
                // Center attraction (gravity) to keep nodes visible
                ImVec2 toCenter = center - g_graphState.positions[node];
                force += toCenter * 0.5f;

                // Node repulsion
                for (const auto& other : nodes) {
                    if (node == other) continue;
                    ImVec2 diff = g_graphState.positions[node] - g_graphState.positions[other];
                    float d = length(diff);
                    if (d > 0.1f && d < 400.0f) {
                        force += normalize(diff) * (kRepulsion / std::max(d, 5.0f));
                    }
                }
                
                g_graphState.velocities[node] += force * dt;
            }

            // Edge attraction
            for (const auto& edge : edges) {
                ImVec2& pA = g_graphState.positions[edge.a];
                ImVec2& pB = g_graphState.positions[edge.b];
                
                ImVec2 diff = pB - pA;
                float d = length(diff);
                if (d > 0.1f) {
                    float f = (d - kSpringLength) * kSpringK;
                    ImVec2 forceVec = normalize(diff) * f;
                    
                    if (g_graphState.draggedNode != edge.a) g_graphState.velocities[edge.a] += forceVec * dt;
                    if (g_graphState.draggedNode != edge.b) g_graphState.velocities[edge.b] -= forceVec * dt;
                }
            }

            // Apply velocities
            for (const auto& node : nodes) {
                if (g_graphState.draggedNode != node) {
                    g_graphState.velocities[node] = g_graphState.velocities[node] * kDamping;
                    g_graphState.positions[node] += g_graphState.velocities[node];
                    
                    // Clamp to canvas with padding
                    const float pad = 30.0f;
                    g_graphState.positions[node].x = std::clamp(g_graphState.positions[node].x, canvas_p0.x + pad, canvas_p1.x - pad);
                    g_graphState.positions[node].y = std::clamp(g_graphState.positions[node].y, canvas_p0.y + pad, canvas_p1.y - pad);
                }
            }

            // Draw edges
            for (const auto& edge : edges) {
                ImVec2 pA = g_graphState.positions[edge.a];
                ImVec2 pB = g_graphState.positions[edge.b];
                
                // Color based on type or weight
                ImU32 edgeColor = IM_COL32(180, 180, 180, 180);
                if (edge.type == "bond") edgeColor = IM_COL32(240, 130, 80, 200);
                
                draw_list->AddLine(pA, pB, edgeColor, 2.0f);
                
                // Draw type label at center
                ImVec2 pMid = (pA + pB) * 0.5f;
                draw_list->AddText(pMid, IM_COL32(220, 220, 220, 255), edge.type.c_str());
                
                if (edge.directed) {
                    // Draw arrow tip
                    ImVec2 dir = normalize(pB - pA);
                    ImVec2 perp(-dir.y, dir.x);
                    ImVec2 tip = pB - dir * 18.0f; // offset from node center to not overlap node circle
                    draw_list->AddTriangleFilled(tip + dir * 6.0f, tip - dir * 4.0f + perp * 5.0f, tip - dir * 4.0f - perp * 5.0f, edgeColor);
                }
            }

            // Node Interaction and Drawing
            ImGuiIO& io = ImGui::GetIO();
            draw_list->PushClipRect(canvas_p0, canvas_p1, true);

            for (const auto& node : nodes) {
                ImVec2 p = g_graphState.positions[node];
                float radius = 12.0f;
                
                // Interaction
                float dist = length(io.MousePos - p);
                bool hovered = is_hovered && (dist < radius + 8.0f);
                
                if (hovered && ImGui::IsMouseClicked(0)) {
                    g_graphState.draggedNode = node;
                }
                
                if (g_graphState.draggedNode == node) {
                    if (ImGui::IsMouseDown(0)) {
                        g_graphState.positions[node] = io.MousePos;
                        g_graphState.positions[node].x = std::clamp(g_graphState.positions[node].x, canvas_p0.x, canvas_p1.x);
                        g_graphState.positions[node].y = std::clamp(g_graphState.positions[node].y, canvas_p0.y, canvas_p1.y);
                    } else {
                        g_graphState.draggedNode = "";
                    }
                }
                
                ImU32 nodeColor = (hovered || g_graphState.draggedNode == node) ? IM_COL32(255, 210, 110, 255) : IM_COL32(90, 140, 230, 255);
                draw_list->AddCircleFilled(p, radius, nodeColor);
                draw_list->AddCircle(p, radius, IM_COL32(255, 255, 255, 255), 0, 2.0f);
                
                // Node label
                ImVec2 textSize = ImGui::CalcTextSize(node.c_str());
                draw_list->AddText(p + ImVec2(-textSize.x * 0.5f, radius + 4.0f), IM_COL32(255, 255, 255, 255), node.c_str());
            }
            
            draw_list->PopClipRect();
        }
    }
    ImGui::End();
}

} // namespace Rendering
