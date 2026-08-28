#pragma once

#include "Singularity/Screen/Renderer.hpp"

// Fixed-function OpenGL implementation of the renderer boundary. This is the
// current backend; it preserves exactly what the immediate-mode draw paths did.
//
// Honest limitation: in fixed-function GL, glColorMaterial ties glColor to
// ambient+diffuse and the app never set a material specular, so there are no
// specular highlights today. To keep the default material pixel-identical this
// backend applies baseColor + shininess only; specular / ambient / diffuse
// COEFFICIENTS ride on RenderMaterial for the WGSL shader (M5), where per-
// material lighting is actually evaluated.
class OpenGLRenderer : public Renderer {
public:
    // Transforms are pushed straight onto the fixed-function stacks, so the result
    // is bit-identical to the glFrustum / gluLookAt / glMultMatrixf the call sites
    // used to do themselves. `view` is cached because GL has no separate model
    // matrix: setModel loads view*model into MODELVIEW.



    void drawMesh(const geom::TessMesh& mesh, const RenderMaterial& material) override;
    void drawImplicit(const geom::SdfNode& field, const glm::vec3& extent,
                      const RenderMaterial& material,
                      const geom::FieldNode* fieldNode = nullptr,
                      uint64_t memoId = 0,
                      uint32_t memoRevision = 0) override;
    void drawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>& segments,
                   const glm::vec4& color, float width, Blend blend) override;
    void setWireframe(bool on) override;
    void drawOverlay(const geom::TessMesh& mesh, const glm::vec4& color,
                     float scale, bool additive) override;

    // Flat-colour primitives — a direct translation of the immediate-mode blocks
    // the call sites used to write inline, including the same push/pop of enable
    // state so nothing leaks into the next draw.
    void drawSolid(const std::vector<glm::vec3>& tris, const glm::vec4& color,
                   Blend blend, bool depthWrite) override;
    void begin2D(uint32_t width, uint32_t height) override;
    void end2D() override;
    void drawTris2D(const std::vector<glm::vec2>& tris, const glm::vec4& color) override;
    void drawLines2D(const std::vector<glm::vec2>& segments,
                     const glm::vec4& color, float width) override;
    void drawImage2D(const uint8_t* rgba, uint32_t width, uint32_t height,
                     const glm::vec4& rect, const glm::vec4& tint) override;
    TextureHandle uploadTexture(TextureHandle handle, const uint8_t* rgba,
                                uint32_t width, uint32_t height) override;
    void releaseTexture(TextureHandle handle) override;

protected:
    void applyBeginFrame(uint32_t width, uint32_t height,
                         const glm::vec4& clearColor) override;
    void applyLight() override;
    void applyLightingEnabled(bool on) override;
    void applyCamera(const glm::mat4& view, const glm::mat4& proj,
                     const glm::vec3& eyePos) override;
    void applyModel(const glm::mat4& model) override;
};
