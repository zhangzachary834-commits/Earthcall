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
#if !defined(__EMSCRIPTEN__) && !defined(NO_OPENGL_RENDERER) && !defined(GLFW_INCLUDE_NONE)
        static OpenGLRenderer s_defaultGL;
        g_current = &s_defaultGL;
#else
        class NullRenderer : public Renderer {
        public:
            void drawMesh(const geom::TessMesh&, const RenderMaterial&) override {}
            void drawImplicit(const geom::SdfNode&, const glm::vec3&, const RenderMaterial&,
                              const geom::FieldNode*, uint64_t, uint32_t, const geom::HeightGrid*) override {}
            void drawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>&, const glm::vec4&, float, Blend) override {}
            void drawOverlay(const geom::TessMesh&, const glm::vec4&, float, bool) override {}
            void drawSolid(const std::vector<glm::vec3>&, const glm::vec4&, Blend, bool) override {}
            void begin2D(uint32_t, uint32_t) override {}
            void end2D() override {}
            void drawTris2D(const std::vector<glm::vec2>&, const glm::vec4&) override {}
            void drawLines2D(const std::vector<glm::vec2>&, const glm::vec4&, float) override {}
            void drawImage2D(const uint8_t*, uint32_t, uint32_t, const glm::vec4&, const glm::vec4&) override {}
            TextureHandle uploadTexture(TextureHandle, const uint8_t*, uint32_t, uint32_t) override { return 0; }
            void releaseTexture(TextureHandle) override {}
        };
        static NullRenderer s_nullRenderer;
        g_current = &s_nullRenderer;
#endif
    }
    return *g_current;
}

void setCurrentRenderer(Renderer* r) { g_current = r; }
