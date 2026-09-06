#include "PerformanceMetricsWindow.hpp"

#include "imgui.h"
#include "Singularity/Core/Engine.hpp"
#include "Person/Person.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

extern ZoneManager mgr;

FrameTimings g_frameTimings{};

namespace Rendering {

void renderPerformanceMetricsWindow(bool* open, Core::Engine* engine) {
    if (!open || !*open || !engine) return;

    if (ImGui::Begin("Performance & Coordinates", open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Core Metrics");
        ImGui::Text("FPS (instant): %.1f", ImGui::GetIO().Framerate);

        // 5-second rolling average FPS
        static float frameTimes[2048] = {0};
        static int ftIdx = 0;
        static int ftCount = 0;
        
        float dt = ImGui::GetIO().DeltaTime;
        frameTimes[ftIdx] = dt;
        ftIdx = (ftIdx + 1) % 2048;
        if (ftCount < 2048) ftCount++;
        
        float sumDt = 0.0f;
        int framesIn5s = 0;
        for (int i = 0; i < ftCount; ++i) {
            int idx = (ftIdx - 1 - i + 2048) % 2048;
            sumDt += frameTimes[idx];
            framesIn5s++;
            if (sumDt >= 5.0f) break;
        }
        float avgFps5s = (sumDt > 0.0f) ? (framesIn5s / sumDt) : 0.0f;
        ImGui::Text("FPS (5s avg):  %.1f", avgFps5s);

        Person* person = engine->getPerson();
        if (person) {
            const glm::vec3& pos = person->position();
            ImGui::Text("Player Pos: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        } else {
            ImGui::TextDisabled("Player Pos: Unknown");
        }

        ImGui::Separator();
        Zone& activeZone = mgr.active();
        size_t zoneShapes = activeZone.objects().size();
        size_t zoneSingulars = zoneShapes + activeZone.formation().getMembers().size() + activeZone.joys().getMembers().size();

        ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "Current Zone: %s", activeZone.name().c_str());
        ImGui::Text("Zone Singulars: %zu", zoneSingulars);
        ImGui::Text("Zone Visual Shapes: %zu", zoneShapes);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "CPU Substrate Metrics");
        ImGui::Text("Total Singulars (CPU allocs): %d", Singular::getAliveCount());
        ImGui::Text("Total Visual Shapes (CPU allocs): %d", Object::getAliveCount());

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
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "GPU Submission Stats");

        const auto& stats = currentRenderer().frameStats();
        ImGui::Text("Total Draw Calls: %u", stats.drawCalls);
        ImGui::Text("  Mesh Draws: %u", stats.meshDrawCalls);
        ImGui::Text("  SDF Draws: %u", stats.sdfDrawCalls);
        ImGui::Text("Triangles Drawn: %u", stats.trianglesDrawn);
        ImGui::Text("Pipeline Switches: %u", stats.pipelineSwitches);
        ImGui::Text("VRAM Allocations: %u", stats.bufferSuballocations);
        ImGui::Text("VRAM Uniform Bytes: %zu", stats.uniformBytesWritten);
        if (stats.gpuMainPassTimingSupported && stats.gpuMainPassTimingValid) {
            ImGui::Text("GPU main render pass (delayed): %.2f ms", stats.gpuMainPassMs);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "GPU timestamp-query result from an earlier submitted frame. It brackets\n"
                    "the main render pass before ImGui's overlay; the delay avoids blocking\n"
                    "the queue merely to measure it.");
            }
        } else if (stats.gpuMainPassTimingSupported) {
            ImGui::TextDisabled("GPU main render pass: waiting for first timestamp sample");
        } else {
            ImGui::TextDisabled("GPU main render pass: timestamps unsupported by this adapter");
        }

        // The raymarcher's cost is very close to linear in PIXELS and nothing
        // else — measured 2026-08-31 on the Perlin floor at one fixed horizon
        // camera: 5.4 ms at 512x512, 14.4 at 1024x1024, 35 at 2048x1152, ~90 at
        // 2880x1800, i.e. a flat ~17 ns/pixel. So the same view costs wildly
        // different milliseconds on different displays or window sizes, and
        // "the horizon was 300 ms, then it was 100 ms" is answered by this line
        // before it is answered by anything else. Derived from ImGui's own
        // backend numbers rather than a new renderer accessor: DisplaySize is
        // logical points, DisplayFramebufferScale the retina factor, and their
        // product is the real framebuffer the fragment shader actually ran on.
        {
            const ImGuiIO& io = ImGui::GetIO();
            const float fbW = io.DisplaySize.x * io.DisplayFramebufferScale.x;
            const float fbH = io.DisplaySize.y * io.DisplayFramebufferScale.y;
            ImGui::Text("Framebuffer: %.0f x %.0f  (%.2f Mpx)",
                        fbW, fbH, (fbW * fbH) / 1.0e6f);
        }

        // ---- Render toggles ----------------------------------------------
        // These are not window-local checkboxes: each one writes the SAME
        // @screen-channel property a Law would write, and EngineRender.cpp
        // pushes it to the renderer every frame. So a Person can flip it here
        // or author it in law text, and both are the one source of truth —
        // this window is a First Mover's hand on the existing property, not a
        // second way to hold the same state (NO_BLACK_BOX.md §2).
        if (LawManager* lm = engine->getLawManager()) {
            if (auto* sc = Singularity::Screen::ScreenChannel::find(*lm)) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Render Toggles");
                ImGui::BeginDisabled();
                ImGui::Checkbox("Height-grid DDA skip (temporarily quarantined)",
                                &sc->heightGridDdaEnabled);
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(
                        "Native Metal parity found a grazing-root mismatch in DDA traversal,\n"
                        "so this optimization is suspended rather than allowed to erase a\n"
                        "rendered surface. The Perlin floor reads p.y and was already on\n"
                        "the generic exact marcher. Engine::tick timing remains CPU\n"
                        "wall-clock; an available GPU timestamp is shown separately above.");
                }
                ImGui::Checkbox("Wireframe (@screen-channel.wireframe)", &sc->wireframe);
            }
        }

        // ---- Engine::tick() Frame Timings Breakdown ----
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Engine::tick()  %.2f ms", g_frameTimings.total_ms);
        ImGui::Text("  Input:           %6.2f ms", g_frameTimings.input_ms);
        ImGui::Text("  Locomotion:      %6.2f ms", g_frameTimings.locomotion_ms);
        ImGui::Text("  Creation Tools:  %6.2f ms", g_frameTimings.creation_ms);
        ImGui::Text("  Interaction:     %6.2f ms", g_frameTimings.interaction_ms);
        ImGui::Text("  Zone Update:     %6.2f ms", g_frameTimings.zone_ms);
        ImGui::Text("  Laws (Rete):     %6.2f ms", g_frameTimings.laws_ms);
        ImGui::Text("  Language:        %6.2f ms", g_frameTimings.language_ms);
        ImGui::Text("  Audio:           %6.2f ms", g_frameTimings.audio_ms);
        // WebGPU submission is asynchronous. These are main-thread wall-clock
        // durations around recording, surface acquisition, and queue submission;
        // a late surface/submit stall often reports accumulated queue pressure,
        // not the execution time of just this frame's SDF draw.
        float actual_3d = g_frameTimings.render3d_ms - g_frameTimings.wait_surface_ms - g_frameTimings.wait_submit_ms;
        if (actual_3d < 0.0f) actual_3d = 0.0f;
        ImGui::Text("  3D phase (CPU wall-clock): %6.2f ms", g_frameTimings.render3d_ms);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "This is not a GPU execution timestamp. It includes command recording\n"
                "plus any blocking surface-acquisition or queue-submit work observed\n"
                "by the main thread. If this adapter supports timestamp queries, the GPU\n"
                "main-render-pass sample above is the separate execution measurement.");
        }
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "    |- Command recording:     %6.2f ms", actual_3d);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "    |- Surface acquire wait:  %6.2f ms", g_frameTimings.wait_surface_ms);
        if (g_frameTimings.wait_submit_ms > 0.1f) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "    |- Queue-submit wait:     %6.2f ms", g_frameTimings.wait_submit_ms);
        }
        ImGui::Text("  ImGui / Present: %6.2f ms", g_frameTimings.imgui_ms);

        // ---- Cold-to-Warm Convergence Telemetry (First 120 Frames) ----
        struct ColdWarmSample {
            float recordingMs;
            float acquireWaitMs;
            float submitWaitMs;
            float total3dMs;
        };
        static std::vector<ColdWarmSample> s_telemetryHistory;
        if (s_telemetryHistory.size() < 120) {
            s_telemetryHistory.push_back({actual_3d, g_frameTimings.wait_surface_ms, g_frameTimings.wait_submit_ms, g_frameTimings.render3d_ms});
        }
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.9f, 1.0f), "Cold-to-Warm Telemetry (%zu/120 frames):", s_telemetryHistory.size());
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset Telemetry")) {
            s_telemetryHistory.clear();
        }

        if (!s_telemetryHistory.empty()) {
            std::vector<float> coldTotals, warmTotals;
            for (size_t i = 0; i < s_telemetryHistory.size(); ++i) {
                if (i < 30) coldTotals.push_back(s_telemetryHistory[i].total3dMs);
                else if (i >= 60) warmTotals.push_back(s_telemetryHistory[i].total3dMs);
            }
            auto calcP = [](std::vector<float>& v, float pct) -> float {
                if (v.empty()) return 0.0f;
                std::sort(v.begin(), v.end());
                size_t idx = static_cast<size_t>(pct * (v.size() - 1));
                return v[idx];
            };

            if (!coldTotals.empty()) {
                float coldP50 = calcP(coldTotals, 0.50f);
                float coldP95 = calcP(coldTotals, 0.95f);
                ImGui::Text("  Cold  (1-30):   p50: %5.1f ms | p95: %5.1f ms", coldP50, coldP95);
            }
            if (!warmTotals.empty()) {
                float warmP50 = calcP(warmTotals, 0.50f);
                float warmP95 = calcP(warmTotals, 0.95f);
                ImGui::Text("  Warm (60-120):  p50: %5.1f ms | p95: %5.1f ms", warmP50, warmP95);
            }
            int steadyFrame = -1;
            for (size_t i = 0; i < s_telemetryHistory.size(); ++i) {
                if (s_telemetryHistory[i].total3dMs <= 20.0f) {
                    steadyFrame = static_cast<int>(i + 1);
                    break;
                }
            }
            if (steadyFrame > 0) {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "  Time-to-steady-state: Frame %d", steadyFrame);
            } else if (s_telemetryHistory.size() >= 30) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "  Time-to-steady-state: Pending convergence");
            }
        }

        // ---- LawManager tick() timing breakdown ----
        if (LawManager* lm = engine->getLawManager()) {
            const auto& t = lm->lastTickTiming();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "LawManager::tick()  %.1f ms", t.totalMs);
            ImGui::Text("  Rete Sync:       %6.2f ms", t.syncMs);
            ImGui::Text("  Seed State:      %6.2f ms", t.seedMs);
            ImGui::Text("  Eval + Sweep:    %6.2f ms", t.evalMs);
            ImGui::Text("  Drive Sessions:  %6.2f ms", t.driveMs);
            ImGui::Text("  Reap Unmade:     %6.2f ms", t.reapMs);
        }
    }
    ImGui::End();
}

} // namespace Rendering
