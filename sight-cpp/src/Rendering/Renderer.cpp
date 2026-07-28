#include "Rendering/Renderer.hpp"

#include "Rendering/GL/OpenGLRenderer.hpp"

namespace {
// The active backend. Null until first use, then the default OpenGL backend —
// constructed lazily so it never runs before a GL context exists. At Milestone 5
// setCurrentRenderer swaps in the WebGPU backend and this static goes unused.
Renderer* g_current = nullptr;
} // namespace

Renderer& currentRenderer() {
    if (!g_current) {
        static OpenGLRenderer s_defaultGL;
        g_current = &s_defaultGL;
    }
    return *g_current;
}

void setCurrentRenderer(Renderer* r) { g_current = r; }
