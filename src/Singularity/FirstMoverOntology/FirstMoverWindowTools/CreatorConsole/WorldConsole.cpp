#include "CreatorConsoleState.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CursorTools.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Singularity/Screen/Camera.hpp"
#include <imgui.h>
#include <vector>
#include <string>

namespace Rendering {

    void renderWorldConsole(Core::Engine* engine) {
        auto& state = getCreatorConsoleState();
        ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.95f, 1.0f), "World & Environment Governance");
        ImGui::Separator();

        // 1. Cursor & Interaction Tools
        if (ImGui::CollapsingHeader("Cursor & Viewport Tools", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("Open Cursor Tools Window")) {
                state.cursorToolsOpen = true;
            }
            ImGui::SameLine();
            ImGui::Checkbox("Visible##CursorTools", &state.cursorToolsOpen);

            if (state.cursorToolsOpen) {
                if (engine && engine->getCursorTools()) {
                    engine->getCursorTools()->renderUI(state.cursorToolsOpen);
                } else {
                    ImGui::TextDisabled("Cursor tools instance not found.");
                }
            }
        }

        ImGui::Spacing();

        // 2. Day/Night Cycle & Atmosphere
        if (ImGui::CollapsingHeader("Atmosphere & Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
            static float timeOfDayHours = 12.0f; // 0.0 - 24.0
            static bool autoDayNightCycle = false;
            static float cycleSpeed = 1.0f;

            if (ImGui::SliderFloat("Time of Day", &timeOfDayHours, 0.0f, 24.0f, "%.1f hrs")) {
                if (engine) {
                    double t = static_cast<double>(timeOfDayHours * 3600.0f);
                    engine->setWorldTime(t);
                }
            }

            float thirdBtnW = responsiveItemWidth(3, 60.0f);
            if (ImGui::Button("Dawn (6am)", ImVec2(thirdBtnW, 0))) {
                timeOfDayHours = 6.0f;
                if (engine) engine->setWorldTime(6.0 * 3600.0);
            }
            ImGui::SameLine();
            if (ImGui::Button("Noon (12pm)", ImVec2(thirdBtnW, 0))) {
                timeOfDayHours = 12.0f;
                if (engine) engine->setWorldTime(12.0 * 3600.0);
            }
            ImGui::SameLine();
            if (ImGui::Button("Dusk (18pm)", ImVec2(thirdBtnW, 0))) {
                timeOfDayHours = 18.0f;
                if (engine) engine->setWorldTime(18.0 * 3600.0);
            }

            ImGui::Checkbox("Auto Day/Night Cycle", &autoDayNightCycle);
            if (autoDayNightCycle) {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120.0f);
                ImGui::SliderFloat("Cycle Speed", &cycleSpeed, 0.1f, 10.0f, "%.1fx");
            }

            static float ambientLight[3] = {0.25f, 0.25f, 0.30f};
            ImGui::ColorEdit3("Ambient Illumination", ambientLight);
        }

        ImGui::Spacing();

        // 3. Physics & Gravity Governance
        if (ImGui::CollapsingHeader("Physics & Gravity Governance", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool flying = Physics::getFlying();
            if (ImGui::Checkbox("Player Flight Mode [F]", &flying)) {
                Physics::setFlying(flying);
            }
            ImGui::SameLine();

            bool gravViz = Physics::getGravityVisualization();
            if (ImGui::Checkbox("Gravity Field Viz [F6]", &gravViz)) {
                Physics::setGravityVisualization(gravViz);
            }

            bool legacyPhysics = Physics::getLegacyEngineEnabled();
            if (ImGui::Checkbox("Legacy Physics Engine", &legacyPhysics)) {
                Physics::setLegacyEngineEnabled(legacyPhysics);
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.55f, 1.0f), "Active Physics Laws");

            const auto& laws = Physics::getLaws();
            if (laws.empty()) {
                ImGui::TextDisabled("No modular physics laws registered.");
            } else {
                for (const auto& law : laws) {
                    ImGui::PushID(law.id);
                    bool isEnabled = law.enabled;
                    if (ImGui::Checkbox(law.name.c_str(), &isEnabled)) {
                        Physics::setLawEnabled(law.id, isEnabled);
                    }
                    if (isEnabled) {
                        ImGui::SameLine(180.0f);
                        float strength = law.strength;
                        ImGui::SetNextItemWidth(120.0f);
                        if (ImGui::DragFloat("##strength", &strength, 0.1f, -50.0f, 50.0f, "%.2f")) {
                            Physics::PhysicsLaw updated = law;
                            updated.strength = strength;
                            Physics::updateLaw(law.id, updated);
                        }
                    }
                    ImGui::PopID();
                }
            }
        }
    }

} // namespace Rendering
