#pragma once

// WebGPU backend for the Renderer boundary (Milestone 5). Implements the four
// verbs against wgpu-native. WebGPU has no matrix stack and no immediate mode, so
// this backend adds an explicit frame lifecycle (beginFrame/endFrame) and camera/
// model state that the OpenGL backend gets for free from the GL matrix stack.
// These extra methods are WebGPU-specific for now; when the app swaps renderers
// they will be lifted onto the Renderer interface (OpenGL impl = no-ops).
//
// Status: drawMesh is real (TessMesh vertex layout, camera+model+material
// uniforms, Lambert+ambient lighting, two-sided). drawImplicit tessellates and
// reuses drawMesh; drawLines/drawOverlay are stubs pending their own pipelines.

#include "Rendering/Renderer.hpp"

#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <vector>

namespace wgpu { struct Device; }

class WebGpuRenderer : public Renderer {
public:
    ~WebGpuRenderer() override { shutdown(); }

    // Build the pipelines against an already-initialised device. colorFormat must
    // match the render target: RGBA8Unorm for the offscreen tests, BGRA8Unorm for
    // a Metal window surface.
    bool init(const wgpu::Device& gpu,
              WGPUTextureFormat colorFormat = WGPUTextureFormat_RGBA8Unorm);
    void shutdown();

    // Offscreen frame: opens a pass rendering into an explicit `target` texture
    // view (used by tests and any render-to-texture). endFrame ends + submits it.
    void beginFrameOffscreen(WGPUTextureView target, uint32_t width, uint32_t height,
                             const glm::vec4& clearColor);

    // Renderer interface frame lifecycle. Live (on-screen) begin acquires the
    // window surface — TODO in the CAMetalLayer phase; a no-op until a surface is
    // configured. endFrame is shared by both begin paths.
    void endFrame() override;

    // Camera (view*proj + world eye position) is set once per frame; model is set
    // per object before its draws — together they replace the GL matrix stack. The
    // eye position drives the specular view vector.
    void setCamera(const glm::mat4& viewProj, const glm::vec3& eyePos) {
        _viewProj = viewProj; _eyePos = eyePos;
    }

    // Renderer interface.
    void drawMesh(const geom::TessMesh& mesh, const RenderMaterial& material) override;
    void drawImplicit(const geom::SdfNode& field, float extent,
                      const RenderMaterial& material) override;
    void drawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>& segments,
                   const glm::vec4& color, float width, Blend blend) override;
    void drawOverlay(const geom::TessMesh& mesh, const glm::vec4& color,
                     float scale, bool additive) override;

    // Flat-colour primitives. STUBS until Milestone 5's 2D pipeline lands — they
    // warn once rather than drawing, so the webgpu-* targets keep building while
    // the OpenGL call sites migrate onto the boundary ahead of them.
    void drawSolid(const std::vector<glm::vec3>& tris, const glm::vec4& color,
                   Blend blend, bool depthWrite) override;
    void begin2D(uint32_t width, uint32_t height) override;
    void end2D() override;
    void drawTris2D(const std::vector<glm::vec2>& tris, const glm::vec4& color) override;
    void drawLines2D(const std::vector<glm::vec2>& segments,
                     const glm::vec4& color, float width) override;
    void drawImage2D(const uint8_t* rgba, uint32_t width, uint32_t height,
                     const glm::vec4& rect, const glm::vec4& tint) override;

    // This backend uploads RenderMaterial::albedoPixels per draw instead of
    // holding persistent textures, so it keeps no handles: upload returns 0 and
    // release is a no-op. Giving it real GPU-side textures is a Milestone 5
    // caching refinement, not a correctness gap.
    TextureHandle uploadTexture(TextureHandle, const uint8_t*, uint32_t, uint32_t) override {
        return 0;
    }
    void releaseTexture(TextureHandle) override {}

protected:
    void applyBeginFrame(uint32_t width, uint32_t height,
                         const glm::vec4& clearColor) override;

    // Interface camera: view and proj stay separate for OpenGL's sake, so collapse
    // them here to the single view*proj uniform this backend actually wants.
    void applyCamera(const glm::mat4& view, const glm::mat4& proj,
                     const glm::vec3& eyePos) override {
        setCamera(proj * view, eyePos);
    }
    void applyModel(const glm::mat4& model) override { _model = model; }

private:
    WGPUDevice _device = nullptr;
    WGPUQueue  _queue  = nullptr;
    WGPUTextureFormat _colorFormat = WGPUTextureFormat_RGBA8Unorm;
    WGPURenderPipeline _meshPipeline = nullptr;
    WGPUBindGroupLayout _bgl = nullptr;

    // Flat-colour pipelines for the selection overlay/wireframe (unlit, blended).
    WGPUBindGroupLayout _flatBgl = nullptr;
    WGPURenderPipeline _overlayAddPipe   = nullptr; // additive triangles, depth-write off
    WGPURenderPipeline _overlayAlphaPipe = nullptr; // alpha triangles, depth-write off
    WGPURenderPipeline _linesPipe        = nullptr; // alpha line-list (WebGPU lines are 1px)

    // Depth buffer, recreated when the target size changes.
    WGPUTexture     _depthTex  = nullptr;
    WGPUTextureView _depthView = nullptr;
    uint32_t _depthW = 0, _depthH = 0;
    void ensureDepth(uint32_t width, uint32_t height);

    // Albedo sampling: one shared sampler + a 1×1 white fallback for untextured
    // materials (so the bind group layout is always satisfied).
    WGPUSampler     _sampler   = nullptr;
    WGPUTexture     _whiteTex  = nullptr;
    WGPUTextureView _whiteView = nullptr;

    // Current frame state.
    WGPUCommandEncoder   _encoder = nullptr;
    WGPURenderPassEncoder _pass   = nullptr;
    glm::mat4 _viewProj{1.0f};
    glm::mat4 _model{1.0f};
    glm::vec3 _eyePos{0.0f};

    // Resources created per draw must outlive the submit; released in endFrame.
    std::vector<WGPUBuffer>      _frameBuffers;
    std::vector<WGPUBindGroup>   _frameBindGroups;
    std::vector<WGPUTexture>     _frameTextures;
    std::vector<WGPUTextureView> _frameTextureViews;

    void releaseFrameResources();
    // Shared flat-colour draw for the overlay verbs: uploads positions + a {mvp,
    // color} uniform and records a draw with `pipe`.
    void drawFlat(WGPURenderPipeline pipe, const std::vector<glm::vec3>& verts,
                  const glm::mat4& mvp, const glm::vec4& color);
};
