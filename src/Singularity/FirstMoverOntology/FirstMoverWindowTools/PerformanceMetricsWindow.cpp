#include "PerformanceMetricsWindow.hpp"

#include "imgui.h"
#include "Singularity/Core/Engine.hpp"
#include "Person/Person.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/Renderer.hpp"

namespace Rendering {

void renderPerformanceMetricsWindow(bool* open, Core::Engine* engine) {
    if (!open || !*open || !engine) return;

    if (ImGui::Begin("Performance & Coordinates", open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Core Metrics");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        
        Person* player = engine->getPlayer();
        if (player) {
            const glm::vec3& pos = player->position();
            ImGui::Text("Player Pos: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        } else {
            ImGui::TextDisabled("Player Pos: Unknown");
        }
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "CPU Substrate Metrics");
        
        // Calculate AST evals per frame
        static uint32_t lastAstEvals = 0;
        uint32_t currentAstEvals = OntoMath::g_astEvaluations.load(std::memory_order_relaxed);
        uint32_t astDiff = currentAstEvals - lastAstEvals;
        lastAstEvals = currentAstEvals;
        
        // Smooth it slightly for readability
        static float smoothedAstDiff = 0.0f;
        smoothedAstDiff = smoothedAstDiff * 0.9f + astDiff * 0.1f;
        ImGui::Text("AST Evaluations: %.0f / frame", smoothedAstDiff);
        ImGui::TextDisabled("Total ASTs lifetime: %u", currentAstEvals);
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "GPU Frame Stats");
        
        const auto& stats = currentRenderer().frameStats();
        ImGui::Text("Total Draw Calls: %u", stats.drawCalls);
        ImGui::Text("  Mesh Draws: %u", stats.meshDrawCalls);
        ImGui::Text("  SDF Draws: %u", stats.sdfDrawCalls);
        ImGui::Text("Triangles Drawn: %u", stats.trianglesDrawn);
        ImGui::Text("Pipeline Switches: %u", stats.pipelineSwitches);
        ImGui::Text("VRAM Allocations: %u", stats.bufferSuballocations);
        ImGui::Text("VRAM Uniform Bytes: %zu", stats.uniformBytesWritten);
    }
    ImGui::End();
}

} // namespace Rendering
