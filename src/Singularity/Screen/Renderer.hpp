#pragma once

#include "Singularity/Screen/RenderMaterial.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SmoothSurface.hpp" // geom::TessMesh

#include <cstdint>
#include <utility>
#include <vector>

namespace geom { struct SdfNode; class FieldNode; struct HeightGrid; }

// ---------------------------------------------------------------------------
// The renderer boundary (OPENGL_MIGRATION_PLAN.md, Milestone 2). Every raw GPU
// call the object draw paths need lives behind this interface. `OpenGLRenderer`
// implements it today; a `WebGpuRenderer` will implement the same contract at
// Milestone 5 — at which point RenderMaterial becomes a uniform and TessMesh a
// vertex buffer, with no change to the callers.
//
// Two draw verbs, matching Earthcall's two geometry modalities:
//   drawMesh     — explicit triangles (polyhedra, smooth/complex/patch surfaces,
//                  and today's tessellated fields).
//   drawImplicit — a field by its SDF. The GL backend tessellates and defers to
//                  drawMesh; the WebGPU backend will raymarch it exactly (M6).
// ---------------------------------------------------------------------------
// How a flat-colour draw combines with what is already in the framebuffer.
// Opaque = no blending; Alpha = src-alpha over; Additive = the glow used by the
// selection/law-candidate overlays.
enum class Blend { Opaque, Alpha, Additive };

// An opaque handle to a GPU-side texture. Under OpenGL this happens to be the GL
// texture name (which is why RenderMaterial::textureId is the same width); under
// WebGPU it is an index the backend assigns. 0 means "none".
using TextureHandle = unsigned int;

class Renderer {
public:
    virtual ~Renderer() = default;

    // Telemetry and GPU micro-mastery stats recorded during the frame.
    struct FrameStats {
        uint32_t drawCalls = 0;
        uint32_t meshDrawCalls = 0;
        uint32_t sdfDrawCalls = 0;
        uint32_t trianglesDrawn = 0;
        size_t   vramAllocatedBytes = 0;
        size_t   uniformBytesWritten = 0;
        uint32_t bufferSuballocations = 0;
        uint32_t pipelineSwitches = 0;
        uint32_t cachedMeshesCount = 0;
    };

    const FrameStats& frameStats() const { return _frameStats; }
    FrameStats& mutableFrameStats() { return _frameStats; }

    // Frame lifecycle. WebGPU needs an explicit render pass per frame; OpenGL is
    // immediate and manages its own framebuffer, so these default to no-ops and
    // only the WebGPU backend overrides them. GameRender brackets its drawing with
    // beginFrame/endFrame so the same call sequence works under either backend.
    void beginFrame(uint32_t width, uint32_t height, const glm::vec4& clearColor) {
        _frameStats = FrameStats{};
        _viewport = glm::ivec4(0, 0, static_cast<int>(width), static_cast<int>(height));
        applyBeginFrame(width, height, clearColor);
    }
    virtual void endFrame() {}

    // Transforms. OpenGL got these for free from the fixed-function matrix stack
    // (glFrustum / gluLookAt / glPushMatrix+glMultMatrixf); WebGPU has no stack, so
    // they become explicit state on the boundary. The contract is the same either
    // way: setCamera once per frame, then setModel before each object's draws.
    //
    // view and proj stay SEPARATE rather than pre-multiplied because OpenGL needs
    // them on distinct stacks — and because fixed-function lighting positions are
    // transformed by the MODELVIEW in force when they are set, so the backend must
    // be able to establish view alone. WebGPU just multiplies them.
    void setCamera(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& eyePos) {
        _view = view; _proj = proj; _eyePos = eyePos;
        applyCamera(view, proj, eyePos);
    }

    // The recorded camera, for call sites that project a world point to the screen
    // themselves — nametags, the brush cursor, pick rays. They used to glGetDoublev
    // the fixed-function stack back out, which only works under OpenGL and only if
    // nothing had disturbed the stack since. Reading it here is portable and exact.
    const glm::mat4&  view() const     { return _view; }
    const glm::mat4&  proj() const     { return _proj; }
    const glm::vec3&  eyePos() const   { return _eyePos; }
    // {x, y, width, height} of the frame, recorded by beginFrame.
    const glm::ivec4& viewport() const { return _viewport; }

    // The scene's single light, in WORLD space. OpenGL configures GL_LIGHT0 —
    // which is why setCamera must run first, since fixed-function transforms a
    // light position by whatever MODELVIEW is in force when it is set. WebGPU
    // feeds the same numbers to its WGSL lighting uniforms.
    void setLight(const glm::vec3& worldPos, const glm::vec3& ambient,
                  const glm::vec3& diffuse, const glm::vec3& specular) {
        _lightPos = worldPos; _lightAmbient = ambient;
        _lightDiffuse = diffuse; _lightSpecular = specular;
        applyLight();
    }
    void setLightingEnabled(bool on) { _lightingOn = on; applyLightingEnabled(on); }

    const glm::vec3& lightPos() const      { return _lightPos; }
    const glm::vec3& lightAmbient() const  { return _lightAmbient; }
    const glm::vec3& lightDiffuse() const  { return _lightDiffuse; }
    const glm::vec3& lightSpecular() const { return _lightSpecular; }
    bool lightingEnabled() const           { return _lightingOn; }

    // The object-to-world transform, as a stack. setModel replaces it outright;
    // pushModel/popModel compose a child transform onto its parent for the nested
    // draws — a Body's parts, a Formation's members — exactly as glPushMatrix +
    // glMultMatrixf + glPopMatrix did. Non-virtual and implemented once here in
    // terms of applyModel, so a backend only ever implements the flat case.
    void setModel(const glm::mat4& model) {
        _modelStack.assign(1, model);
        applyModel(model);
    }
    void pushModel(const glm::mat4& local) {
        _modelStack.push_back(_modelStack.back() * local);
        applyModel(_modelStack.back());
    }
    void popModel() {
        if (_modelStack.size() > 1) _modelStack.pop_back();
        applyModel(_modelStack.back());
    }
    // The accumulated transform, for call sites that need to project a local point
    // into world space themselves (nametags, pick rays).
    const glm::mat4& currentModel() const { return _modelStack.back(); }

    virtual void drawMesh(const geom::TessMesh& mesh, const RenderMaterial& material) = 0;
    virtual void drawImplicit(const geom::SdfNode& field, const glm::vec3& extent,
                              const RenderMaterial& material,
                              const geom::FieldNode* fieldNode = nullptr,
                              uint64_t memoId = 0,
                              uint32_t memoRevision = 0,
                              const geom::HeightGrid* heightGrid = nullptr) = 0;

    // Unlit, blended overlays — the selection/law-candidate highlight. These are
    // deliberately separate verbs from drawMesh: colour-only, no lighting or
    // texture, and they own their own blend/depth state.
    //   drawLines   — anti-aliased line segments (RGBA, width).
    //   drawOverlay — a translucent shell over a mesh; `scale` inflates it about
    //                 the origin, `additive` selects additive vs alpha blending,
    //                 depth test stays on but depth WRITE is disabled.
    virtual void drawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>& segments,
                           const glm::vec4& color, float width, Blend blend) = 0;

    // WebGPU's clip space has z in [0,1]; OpenGL's is [-1,1]. A projection matrix
    // built for one is wrong for the other — geometry silently clips or z-fights.
    // The backend declares which convention it needs so callers can pick
    // glm::frustumZO vs glm::frustumNO, rather than the whole program having to
    // agree on a GLM_FORCE_DEPTH_ZERO_TO_ONE macro (which cannot vary per backend
    // inside one binary anyway).
    virtual bool zeroToOneDepth() const { return false; }

    // Whether drawImplicit renders a field EXACTLY (raymarching it) rather than by
    // tessellating. Callers that hold a cached mesh use this to decide which is
    // better: WebGPU marches the field itself, so the mesh is a needless
    // approximation there — while the OpenGL implementation of drawImplicit
    // tessellates on EVERY call, so routing a cached-mesh caller through it would
    // be a large regression. Hence a query rather than always preferring one.
    virtual bool rendersImplicitExactly() const { return false; }

    // Draw subsequent meshes as edges instead of filled triangles. This wraps an
    // arbitrary draw — the BrushCreate hologram sets it, then calls the ordinary
    // Object draw path — which is why it is render STATE rather than a draw verb.
    // OpenGL maps it to glPolygonMode; WebGPU swaps in a line-list pipeline.
    virtual void setWireframe(bool /*on*/) {}

    // Governs the min/max heightfield DDA skip (rendering-optimization Phase
    // C, @screen-channel.heightGridDdaEnabled). Same shape as setWireframe:
    // OpenGL has no marcher to skip anything in, so it stays a no-op there;
    // WebGPU overrides it to gate whether drawImplicit's heightGrid argument
    // is honoured.
    virtual void setHeightGridDdaEnabled(bool /*on*/) {}
    virtual void drawOverlay(const geom::TessMesh& mesh, const glm::vec4& color,
                             float scale, bool additive) = 0;

    // -----------------------------------------------------------------------
    // Flat-colour primitives. Everything the app still drew with glBegin/glVertex
    // — gizmo cubes, drag handles, ghost previews, 2D panels, strokes, and the
    // stb_easy_font text quads — is one of these two shapes: unlit triangles in
    // world space, or unlit triangles/lines in screen space.
    // -----------------------------------------------------------------------

    // Unlit triangles in the current model space (setModel applies). No lighting,
    // no texture, just colour. depthWrite=false is how the translucent shells
    // avoid occluding each other, exactly as glDepthMask(GL_FALSE) did.
    virtual void drawSolid(const std::vector<glm::vec3>& tris, const glm::vec4& color,
                           Blend blend, bool depthWrite) = 0;

    // Screen space. begin2D installs an orthographic projection with (0,0) at the
    // TOP-LEFT and (width,height) at the bottom-right, depth testing off and alpha
    // blending on — i.e. the glOrtho/glPushMatrix/glDisable(GL_DEPTH_TEST) preamble
    // that every 2D overlay used to repeat by hand. end2D restores the 3D camera
    // from the last setCamera. Calls do not nest.
    virtual void begin2D(uint32_t width, uint32_t height) = 0;
    virtual void end2D() = 0;
    virtual void drawTris2D(const std::vector<glm::vec2>& tris, const glm::vec4& color) = 0;
    virtual void drawLines2D(const std::vector<glm::vec2>& segments,
                             const glm::vec4& color, float width) = 0;

    // An RGBA8 image blit into `rect` = {x0, y0, x1, y1} in screen space, tinted by
    // `tint`. Pixels are consumed during the call; callers regenerate them each
    // frame (the brush canvas), so backends upload per draw rather than caching.
    virtual void drawImage2D(const uint8_t* rgba, uint32_t width, uint32_t height,
                             const glm::vec4& rect, const glm::vec4& tint) = 0;

    // -----------------------------------------------------------------------
    // Persistent textures — the per-face albedo the Face Brush paints. Unlike
    // drawImage2D's transient blit, these live across frames and are re-uploaded
    // only when the paint changes.
    //
    // uploadTexture creates a texture when `handle` is 0, otherwise replaces the
    // contents of an existing one, and returns the handle to keep. Square RGBA8.
    // A backend that consumes RenderMaterial::albedoPixels directly rather than
    // holding GPU-side textures may return 0 — callers must treat 0 as "no handle
    // to keep", not as failure.
    virtual TextureHandle uploadTexture(TextureHandle handle, const uint8_t* rgba,
                                        uint32_t width, uint32_t height) = 0;
    virtual void releaseTexture(TextureHandle handle) = 0;

protected:
    // The hooks a backend actually implements. The state above (model stack,
    // recorded camera, viewport) is shared and lives here.
    virtual void applyModel(const glm::mat4& /*model*/) {}
    virtual void applyCamera(const glm::mat4& /*view*/, const glm::mat4& /*proj*/,
                             const glm::vec3& /*eyePos*/) {}
    virtual void applyBeginFrame(uint32_t /*width*/, uint32_t /*height*/,
                                 const glm::vec4& /*clearColor*/) {}
    virtual void applyLight() {}
    virtual void applyLightingEnabled(bool /*on*/) {}

private:
    std::vector<glm::mat4> _modelStack{glm::mat4(1.0f)};
    glm::mat4  _view{1.0f};
    glm::mat4  _proj{1.0f};
    glm::vec3  _eyePos{0.0f};
    glm::ivec4 _viewport{0, 0, 0, 0};

    // Light defaults reproduce what ShadingSystem::init configured on GL_LIGHT0.
    glm::vec3 _lightPos{2.0f, 5.0f, 2.0f};
    glm::vec3 _lightAmbient{0.2f};
    glm::vec3 _lightDiffuse{0.8f};
    glm::vec3 _lightSpecular{1.0f};
    bool      _lightingOn = true;
    FrameStats _frameStats;
};

// Topology adapters. The fixed-function call sites emitted GL_QUADS, GL_POLYGON
// and GL_LINE_LOOP; WebGPU has none of those topologies, so the boundary speaks
// only triangles and line segments and the call sites convert on the way in.
namespace draw {

// GL_QUADS -> triangle list. 4n vertices in, 6n out.
std::vector<glm::vec2> quadsToTris(const std::vector<glm::vec2>& quads);
std::vector<glm::vec3> quadsToTris(const std::vector<glm::vec3>& quads);

// GL_POLYGON / GL_TRIANGLE_FAN -> triangle list, fanned about the first vertex.
std::vector<glm::vec2> fanToTris(const std::vector<glm::vec2>& fan);

// GL_LINE_STRIP (closed=false) / GL_LINE_LOOP (closed=true) -> segment pairs.
std::vector<glm::vec2> stripToSegments(const std::vector<glm::vec2>& pts, bool closed);

// GL_LINE_STIPPLE has no equivalent outside fixed-function GL, so dashes are cut
// on the CPU: each input segment becomes a run of shorter segments alternating
// `dash` units drawn and `gap` units skipped.
std::vector<glm::vec2> dashSegments(const std::vector<glm::vec2>& segments,
                                    float dash, float gap);

// GL_POINTS with glPointSize has no equivalent either — a point of `size` pixels
// becomes a filled square of that size centred on it.
std::vector<glm::vec2> pointsToTris(const std::vector<glm::vec2>& points, float size);

// An axis-aligned rectangle {x0, y0, x1, y1}: as two filled triangles, or as the
// four segments of its closed outline (what GL_LINE_LOOP round a quad produced).
std::vector<glm::vec2> rectTris(const glm::vec4& rect);
std::vector<glm::vec2> rectOutline(const glm::vec4& rect);

// stb_easy_font_print fills a packed buffer of 16-byte GL_QUADS vertices —
// {float x, y, z; unsigned char rgba[4]} — which the call sites used to hand
// straight to glVertexPointer(2, GL_FLOAT, 16, buf). Pull out the 2D positions
// and triangulate. `quadCount` is stb_easy_font_print's return value.
std::vector<glm::vec2> easyFontToTris(const void* buffer, int quadCount);

} // namespace draw

// The single active backend. Draw paths call currentRenderer().drawMesh(...).
// Defaults to an OpenGLRenderer; setCurrentRenderer swaps in WebGPU at M5.
Renderer& currentRenderer();
void setCurrentRenderer(Renderer* r);
