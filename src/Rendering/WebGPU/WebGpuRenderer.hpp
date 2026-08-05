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
#include <functional>
#include <string>
#include <map>
#include <tuple>
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

    // Attach the window surface. Once set, the Renderer-interface beginFrame path
    // acquires a texture from it each frame instead of doing nothing. Passing the
    // instance too because resizing has to reconfigure the surface.
    void attachSurface(WGPUSurface surface, WGPUInstance instance) {
        _surface = surface; _instance = instance;
    }

    // Draw on top of the frame just finished, WITHOUT clearing it — this is how
    // Dear ImGui gets composited. Must be called after endFrame() and before
    // present(): endFrame closes the scene pass, but the acquired surface texture
    // is deliberately still held so a second pass can load-and-draw over it.
    // `record` receives the overlay pass encoder.
    void overlayPass(const std::function<void(WGPURenderPassEncoder)>& record);

    // Present the acquired surface texture and release it. Ends the live frame.
    // No-op for offscreen frames (there is no surface to present).
    void present();

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
    // Declaring the 2-arg overload above would otherwise HIDE the boundary's
    // 3-arg setCamera(view, proj, eye) for anyone holding a WebGpuRenderer by its
    // concrete type — a silent "too many arguments" at the call site.
    using Renderer::setCamera;

    // Renderer interface.
    void drawMesh(const geom::TessMesh& mesh, const RenderMaterial& material) override;
    void drawImplicit(const geom::SdfNode& field, float extent,
                      const RenderMaterial& material) override;
    void drawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>& segments,
                   const glm::vec4& color, float width, Blend blend) override;
    void drawOverlay(const geom::TessMesh& mesh, const glm::vec4& color,
                     float scale, bool additive) override;

    // Flat-colour primitives.
    void drawSolid(const std::vector<glm::vec3>& tris, const glm::vec4& color,
                   Blend blend, bool depthWrite) override;
    void begin2D(uint32_t width, uint32_t height) override;
    void end2D() override;
    void drawTris2D(const std::vector<glm::vec2>& tris, const glm::vec4& color) override;
    void drawLines2D(const std::vector<glm::vec2>& segments,
                     const glm::vec4& color, float width) override;
    void drawImage2D(const uint8_t* rgba, uint32_t width, uint32_t height,
                     const glm::vec4& rect, const glm::vec4& tint) override;
    void setWireframe(bool on) override { _wireframe = on; }
    bool zeroToOneDepth() const override { return true; }
    bool rendersImplicitExactly() const override { return true; }

    // Persistent GPU textures for face paint. FaceTexture calls this only when the
    // paint actually changes, so holding the texture means a repainted surface
    // costs one upload instead of one per face per frame.
    TextureHandle uploadTexture(TextureHandle handle, const uint8_t* rgba,
                                uint32_t width, uint32_t height) override;
    void releaseTexture(TextureHandle handle) override;

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

    // ---- Flat-colour pipelines (unlit: overlays, gizmos, wireframe, all 2D) ----
    // OpenGL could change blend and depth state between draws; WebGPU bakes them
    // into the pipeline, so every (topology, blend, depth) combination the app uses
    // is a distinct object. They are built on first use and cached rather than
    // enumerated up front — the full cross product is 18, of which the app uses ~7.
    enum class DepthMode {
        TestWrite,  // ordinary solid geometry
        TestOnly,   // translucent shells: read depth, never occlude each other
        None,       // screen space: no depth interaction at all
    };
    struct FlatKey {
        WGPUPrimitiveTopology topo;
        Blend blend;
        DepthMode depth;
        bool operator<(const FlatKey& o) const {
            return std::tie(topo, blend, depth) < std::tie(o.topo, o.blend, o.depth);
        }
    };
    WGPUBindGroupLayout _flatBgl = nullptr;
    WGPUShaderModule    _flatShader = nullptr;  // kept alive for lazy pipeline builds
    WGPUPipelineLayout  _flatLayout = nullptr;
    std::map<FlatKey, WGPURenderPipeline> _flatPipes;
    WGPURenderPipeline flatPipeline(WGPUPrimitiveTopology topo, Blend blend, DepthMode depth);

    // Textured screen-space quad (the brush canvas blit).
    WGPUBindGroupLayout _imageBgl    = nullptr;
    WGPUPipelineLayout  _imageLayout = nullptr;
    WGPUShaderModule    _imageShader = nullptr;
    WGPURenderPipeline  _imagePipe   = nullptr;

    // Screen-space state, established by begin2D.
    bool      _in2D = false;
    glm::mat4 _ortho2D{1.0f};

    // ---- Raymarched SDF fields (Milestone 6) ----
    // One pipeline per TREE SHAPE, keyed on the generated WGSL. Numeric parameters
    // live in a buffer, so changing a radius or a blend reuses the pipeline and a
    // slider drag costs no shader compiles.
    struct SdfPipeline {
        WGPURenderPipeline pipe = nullptr;
        WGPUBindGroupLayout bgl = nullptr;
    };
    std::map<std::string, SdfPipeline> _sdfPipes;
    WGPUBuffer _sdfCubeVerts = nullptr; // unit bounding cube, shared by every field
    const SdfPipeline* sdfPipeline(const std::string& wgsl);

    // setWireframe: meshes draw as edges instead of filled triangles.
    bool _wireframe = false;

    // Depth buffer, recreated when the target size changes.
    WGPUTexture     _depthTex  = nullptr;
    WGPUTextureView _depthView = nullptr;
    uint32_t _depthW = 0, _depthH = 0;
    void ensureDepth(uint32_t width, uint32_t height);

    // Textures this backend owns, keyed by the handle it hands out. Handles are
    // dense counters rather than pointers so a stale one is inert (a failed
    // lookup) instead of a dangling dereference.
    struct OwnedTexture { WGPUTexture tex = nullptr; WGPUTextureView view = nullptr; uint32_t size = 0; };
    std::map<TextureHandle, OwnedTexture> _textures;
    TextureHandle _nextTexture = 1; // 0 means "none"

    // Albedo sampling: one shared sampler + a 1×1 white fallback for untextured
    // materials (so the bind group layout is always satisfied).
    WGPUSampler     _sampler   = nullptr;
    WGPUTexture     _whiteTex  = nullptr;
    WGPUTextureView _whiteView = nullptr;

    // Window surface, when running live (null for offscreen/tests).
    WGPUSurface  _surface  = nullptr;
    WGPUInstance _instance = nullptr;
    // The surface texture acquired for THIS frame. Held from beginFrame until
    // present() so an overlay pass can run between them.
    WGPUTexture     _surfaceTex  = nullptr;
    WGPUTextureView _surfaceView = nullptr;

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
