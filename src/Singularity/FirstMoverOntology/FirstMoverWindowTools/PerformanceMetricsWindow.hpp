#pragma once

namespace Core { class Engine; }

// FrameTimings — populated once per Engine::tick, consumed by the perf window.
// All values are in milliseconds.  Written from a single thread (the main loop);
// the perf window reads it on the same thread, so no synchronisation is needed.
struct FrameTimings {
    float input_ms        = 0.f; // keyboard + mouse handler update
    float locomotion_ms   = 0.f; // LocomotionChannel::step
    float creation_ms     = 0.f; // stepCreationTools
    float interaction_ms  = 0.f; // InteractionChannel::step
    float zone_ms         = 0.f; // Zone::update + applyFormationRelations
    float laws_ms         = 0.f; // LawManager::tick (Rete agenda drain)
    float language_ms     = 0.f; // LanguageSystem::tick
    float audio_ms        = 0.f; // AudioSystem::tick
    float render3d_ms     = 0.f; // Engine::render() — 3-D scene
    float imgui_ms        = 0.f; // ImGui::Render + swap/present
    float total_ms        = 0.f; // full tick wall-clock
};

// Global written by Engine::tick each frame; zero-initialised at startup.
// Kernel state — sits below the Law system, named here rather than omitted.
extern FrameTimings g_frameTimings;

namespace Rendering {
    void renderPerformanceMetricsWindow(bool* open, Core::Engine* engine);
}
