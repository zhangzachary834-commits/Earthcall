#include "Singularity/Screen/Renderer.hpp"

#include "Singularity/Screen/GL/OpenGLRenderer.hpp"

#include <stdexcept>

namespace {
// The active backend. Null until first use, then the default OpenGL backend —
// constructed lazily so it never runs before a GL context exists. At Milestone 5
// setCurrentRenderer swaps in the WebGPU backend and this static goes unused.
Renderer* g_current = nullptr;
} // namespace

Renderer& currentRenderer() {
    if (!g_current) {
#ifndef __EMSCRIPTEN__
        static OpenGLRenderer s_defaultGL;
        g_current = &s_defaultGL;
#endif
    }
    if (!g_current) {
        // On wasm there is no lazy default: the WebGPU backend is installed
        // by Engine::init calling setCurrentRenderer() before the first draw
        // (see Singularity/Core/Engine.cpp). Getting here means either a call
        // happened before that (static init, or a draw racing engine setup)
        // or Engine::init itself failed. A bare `return *g_current` used to
        // dereference null in exactly this situation, silently and only on
        // the platform/timing that hits it. Fail loudly and specifically
        // instead of crashing with no diagnostic.
        throw std::runtime_error(
            "currentRenderer(): no renderer is set. There is no lazy default "
            "backend on this platform (wasm has none; native's OpenGLRenderer "
            "above would already have been installed). This means either a "
            "draw call happened before setCurrentRenderer() ran, or "
            "Engine::init failed before installing one.");
    }
    return *g_current;
}

void setCurrentRenderer(Renderer* r) { g_current = r; }
