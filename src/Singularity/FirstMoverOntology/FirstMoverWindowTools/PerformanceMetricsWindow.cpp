#include "PerformanceMetricsWindow.hpp"

#include "imgui.h"
#include "Singularity/Core/Engine.hpp"
#include "Person/Person.hpp"

namespace Rendering {

void renderPerformanceMetricsWindow(bool* open, Core::Engine* engine) {
    if (!open || !*open || !engine) return;

    if (ImGui::Begin("Performance & Coordinates", open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        
        Person* player = engine->getPlayer();
        if (player) {
            const glm::vec3& pos = player->position();
            ImGui::Text("Coordinates: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        } else {
            ImGui::TextDisabled("Coordinates: Unknown (No Player)");
        }
    }
    ImGui::End();
}

} // namespace Rendering
