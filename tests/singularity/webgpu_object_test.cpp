// Real Earthcall content rendered through WebGPU, headless.
//
// WHY THIS EXISTS: smoke_renderer proves each Renderer verb works, but it feeds
// them by hand. That is exactly how the "every cube is white" bug survived —
// RenderMaterial::albedoPixels was read correctly by WebGpuRenderer and set
// correctly by the test fixture, while NOTHING in the app ever populated it.
// A verb tested only through a fixture passes while the production path feeding
// it does not exist. So this test drives the genuine Object draw path instead.
//
// It also demonstrates that Objects can now be constructed with no GL context:
// texture upload goes through the Renderer boundary, so selecting the WebGPU
// backend first means Object construction never calls into OpenGL.

#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Time/timeline.hpp"
#include "Relation/Relation.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

extern MaterialManager materials;   // global Material beings (globals.cpp)

namespace {

const uint32_t W = 16, H = 16;

struct MapR { bool ok = false, done = false; };
void onMap(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* r = static_cast<MapR*>(u);
    r->ok = (s == WGPUMapAsyncStatus_Success);
    r->done = true;
}

std::shared_ptr<OntoMath::MathNode> scalarNode(double value) {
    auto node = std::make_shared<OntoMath::MathNode>();
    node->op = OntoMath::MathNode::Op::ScalarLeaf;
    node->scalarForm.terms.push_back(OntoMath::Term(value));
    return node;
}

std::shared_ptr<OntoMath::MathNode> vectorNode(double x, double y, double z) {
    auto node = std::make_shared<OntoMath::MathNode>();
    node->op = OntoMath::MathNode::Op::VectorConstruct;
    node->children.push_back(std::make_unique<OntoMath::MathNode>(*scalarNode(x)));
    node->children.push_back(std::make_unique<OntoMath::MathNode>(*scalarNode(y)));
    node->children.push_back(std::make_unique<OntoMath::MathNode>(*scalarNode(z)));
    return node;
}

// Test-only stand-in for the production radiant FieldNode. The important
// distinction is source vs receiver: rho's Timeline belongs to the Singular
// whose conditional process emits radiance, not to whichever surface is shaded.
class RadianceSourceProbe final : public Singular {
public:
    std::string getIdentifier() const override {
        return "webgpu-radiance-source-probe";
    }

protected:
    void buildProperties() override { _propertiesBuilt = true; }
};

} // namespace

int main() {
    // Unbuffered: this test can crash inside the GPU driver, and a buffered
    // stdout would swallow every progress line written before the fault,
    // making the failure look like it happened at startup.
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    wgpu::Device gpu;
    if (!gpu.init()) { std::printf("FAIL: no WebGPU device\n"); return 1; }

    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) { std::printf("FAIL: renderer init\n"); return 1; }

    // MUST precede any Object construction: Object paints its face textures on
    // construction, and the default backend is OpenGL, which would try to call
    // glGenTextures with no context.
    setCurrentRenderer(&renderer);

    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = { W, H, 1 };
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1; td.sampleCount = 1;
    WGPUTexture target = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(target, nullptr);

    WGPUBufferDescriptor rbd = {};
    rbd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    rbd.size = 256 * H;
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &rbd);

    auto readAt = [&](uint32_t px_, uint32_t py_, unsigned char out[4]) {
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUTexelCopyTextureInfo src = {};
        src.texture = target; src.aspect = WGPUTextureAspect_All; src.origin = {0,0,0};
        WGPUTexelCopyBufferInfo dst = {};
        dst.buffer = readback; dst.layout.bytesPerRow = 256; dst.layout.rowsPerImage = H;
        WGPUExtent3D ext = { W, H, 1 };
        wgpuCommandEncoderCopyTextureToBuffer(enc, &src, &dst, &ext);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuCommandEncoderRelease(enc);

        MapR m;
        WGPUBufferMapCallbackInfo ci = {};
        ci.mode = WGPUCallbackMode_AllowProcessEvents;
        ci.callback = onMap; ci.userdata1 = &m;
        wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, 256 * H, ci);
        while (!m.done) wgpuInstanceProcessEvents(gpu.instance);
        const auto* px = static_cast<const unsigned char*>(
            wgpuBufferGetConstMappedRange(readback, 0, 256 * H));
        const size_t at = size_t(py_) * 256 + size_t(px_) * 4;
        for (int i = 0; i < 4; ++i) out[i] = px[at + i];
        wgpuBufferUnmap(readback);
    };
    auto readCentre = [&](unsigned char out[4]) { readAt(W / 2, H / 2, out); };

    // Camera: straight down -Z at a unit cube, so its +Z face fills the frame.
    // frustum/perspective must be the [0,1]-depth form for WebGPU.
    const glm::vec3 eye(0.0f, 0.0f, 2.0f);
    const glm::mat4 proj = glm::perspectiveZO(glm::radians(45.0f), float(W) / H, 0.1f, 100.0f);
    const glm::mat4 view3d = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0, 1, 0));

    // --- The actual subject: an Object painted red, drawn the way the app draws it.
    // setFaceColor paints the object's OWN material — the first stroke
    // diverges it from the shared material.default it was referencing, which
    // is what makes "this object is red and that one is green" possible at
    // all now that the per-face textures live on the Material being.
    Object cube;
    cube.setShapeKind(Object::ShapeKind::Cube);
    for (int f = 0; f < cube.getFaces(); ++f)
        cube.setFaceColor(f, 1.0f, 0.0f, 0.0f); // pure red paint

    renderer.setCamera(view3d, proj, eye);
    renderer.setModel(glm::mat4(1.0f));
    renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    cube.drawObject();
    renderer.endFrame();

    unsigned char p[4];
    readCentre(p);
    std::printf("painted cube centre = (%d,%d,%d,%d)\n", p[0], p[1], p[2], p[3]);

    // Lighting scales the value, so do not assert an exact colour — assert the
    // HUE. An unpainted surface falls back to the 1x1 white texture, which makes
    // r == g == b; red paint must make red dominate. That difference is precisely
    // the bug this test exists to catch.
    assert(p[0] > 40 && "surface rendered black — nothing reached the framebuffer");
    assert(p[0] > p[1] + 40 && p[0] > p[2] + 40 &&
           "face albedo did not reach WebGPU: surface is grey/white, not red");

    // Control: a SECOND object painted a different colour must come out that
    // colour. Asserting "the first one is red" alone would still pass if every
    // surface were tinted red by some shared state; this pins the paint to the
    // object. It also catches the per-draw texture upload leaking between draws.
    Object green;
    green.setShapeKind(Object::ShapeKind::Cube);
    for (int f = 0; f < green.getFaces(); ++f)
        green.setFaceColor(f, 0.0f, 1.0f, 0.0f);

    renderer.setModel(glm::mat4(1.0f));
    renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    green.drawObject();
    renderer.endFrame();

    unsigned char q[4];
    readCentre(q);
    std::printf("green cube centre   = (%d,%d,%d,%d)\n", q[0], q[1], q[2], q[3]);
    assert(q[1] > q[0] + 40 && q[1] > q[2] + 40 &&
           "second object did not get its own paint — albedo is shared or stale");

    // An UNPAINTED object has no face textures at all — paint lives on the
    // Material being, and an object that has never been painted is still
    // sharing one rather than owning one. What it shows is that material's
    // baseColor, so assert THAT: give it a material tinted blue and read blue
    // back. (It used to seed a per-face RGB palette on construction; that
    // palette was a debug default which no longer has anywhere to live now
    // that a material is shared by name, and an object which shows its
    // material's colour is the more truthful default anyway.)
    auto tint = materials.create("webgpu_object_tint");
    tint->baseColor = glm::vec3(0.1f, 0.1f, 0.9f);

    Object plain;
    plain.setShapeKind(Object::ShapeKind::Cube);
    plain.setMaterialId("material.webgpu_object_tint");
    renderer.setModel(glm::mat4(1.0f));
    renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    plain.drawObject();
    renderer.endFrame();

    unsigned char d[4];
    readCentre(d);
    std::printf("unpainted cube centre = (%d,%d,%d,%d)\n", d[0], d[1], d[2], d[3]);
    assert(d[2] > d[0] + 40 && d[2] > d[1] + 40 &&
           "material baseColor did not reach WebGPU (+Z face should be blue)");
    // The original bug's signature: an all-white surface has r == g == b.
    assert(!(abs(int(d[0]) - int(d[1])) < 12 && abs(int(d[1]) - int(d[2])) < 12) &&
           "surface is grey — the material's colour never reached the shader");

    // And painting it now DIVERGES it: the shared tint is untouched afterwards,
    // which is the whole promise of copy-on-write over a shared being.
    plain.setFaceColor(0, 1.0f, 0.0f, 0.0f);
    assert(plain.materialId() != "material.webgpu_object_tint" &&
           "painting an object must give it its own material, not repaint the shared one");
    assert(tint->faceTextures.empty() &&
           "the shared material took paint meant for one object");

    // --- Volumetric V0 native witness ---------------------------------------
    // Use an implicit f(p)=1: it is positive everywhere, so there is NO hard
    // zero-surface inside the proxy. Any non-black pixel therefore comes from
    // participating-medium integration alone, not from surface shading.
    //
    // This is intentionally an isolated offscreen composition witness. Production
    // Zone media are not activated through this path until the renderer has a
    // depth-aware volume-composition pass that can clamp transport against opaque
    // scene depth without letting translucent samples become depth owners.
    {
        geom::SdfNode noSurface = geom::makeImplicit("1");
        geom::FieldNode medium("webgpu-volume-density-probe");
        auto densityNode = scalarNode(0.08);
        *medium.volumeDensity = OntoMath::Piecewise::continuous(densityNode);

        RenderMaterial volumeMat;
        volumeMat.baseColor = glm::vec3(0.0f);
        volumeMat.opacity = 0.0f;

        constexpr uint64_t kVolumeMemoId = 0xD3115179ULL;
        constexpr uint32_t kVolumeStructureRevision = 1u;

        renderer.setVolumeDensityTemporalCoordinate(0.0, 0.0);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.drawImplicit(noSurface, glm::vec3(1.0f), volumeMat, &medium,
                              kVolumeMemoId, kVolumeStructureRevision, nullptr,
                              /*memoParameterRevision=*/1u);
        renderer.endFrame();

        unsigned char lowDensity[4];
        readCentre(lowDensity);
        const Renderer::FrameStats lowStats = renderer.frameStats();
        std::printf("volume low density  = (%d,%d,%d,%d), compiles=%llu hits=%llu\n",
                    lowDensity[0], lowDensity[1], lowDensity[2], lowDensity[3],
                    static_cast<unsigned long long>(lowStats.sdfProgramCompiles),
                    static_cast<unsigned long long>(lowStats.sdfProgramCacheHits));

        assert(lowDensity[0] > 0 &&
               "authored volume density produced no native WebGPU contribution");
        assert(abs(int(lowDensity[0]) - int(lowDensity[1])) < 8 &&
               abs(int(lowDensity[1]) - int(lowDensity[2])) < 8 &&
               "V0 compatibility scatter should still be neutral/white");
        assert(lowStats.sdfProgramCompiles >= 1 &&
               "first authored density draw must compile its structure");

        // VALUE ONLY: mutate D in-place while keeping geometry's parameter
        // revision unchanged. Density must refresh on its own authored identity,
        // not because an unrelated geometry revision was manually advanced.
        densityNode->scalarForm.terms[0].coefficient = 0.8;

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.drawImplicit(noSurface, glm::vec3(1.0f), volumeMat, &medium,
                              kVolumeMemoId, kVolumeStructureRevision, nullptr,
                              /*memoParameterRevision=*/1u);
        renderer.endFrame();

        unsigned char highDensity[4];
        readCentre(highDensity);
        const Renderer::FrameStats highStats = renderer.frameStats();
        std::printf("volume high density = (%d,%d,%d,%d), compiles=%llu hits=%llu\n",
                    highDensity[0], highDensity[1], highDensity[2], highDensity[3],
                    static_cast<unsigned long long>(highStats.sdfProgramCompiles),
                    static_cast<unsigned long long>(highStats.sdfProgramCacheHits));

        assert(highDensity[0] > lowDensity[0] + 10 &&
               "numeric volume-density edit did not visibly change native pixels");
        assert(highStats.sdfProgramCompiles == 0 &&
               "numeric D edit regenerated WGSL instead of refreshing parameters");
        assert(highStats.sdfProgramCacheHits >= 1 &&
               "numeric D edit did not reuse the memoized volume shader");

        // STRUCTURE ONLY: replace ScalarLeaf with Add(ScalarLeaf, ScalarLeaf)
        // while geometry revisions remain untouched. Density structure itself
        // must invalidate WGSL exactly once.
        auto densityAdd = std::make_shared<OntoMath::MathNode>();
        densityAdd->op = OntoMath::MathNode::Op::Add;
        densityAdd->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.4)));
        densityAdd->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.4)));
        medium.volumeDensity->pieces[0].mathNode = densityAdd;

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.drawImplicit(noSurface, glm::vec3(1.0f), volumeMat, &medium,
                              kVolumeMemoId, kVolumeStructureRevision, nullptr,
                              /*memoParameterRevision=*/1u);
        renderer.endFrame();
        const Renderer::FrameStats densityStructureStats = renderer.frameStats();
        assert(densityStructureStats.sdfProgramCompiles == 1 &&
               "density operator-tree edit failed to regenerate WGSL structure");
    }

    // --- Volumetric V0c production-composition witness -----------------------
    // Exercise Renderer::composeVolumes(), not the isolated drawImplicit density
    // seam above. The same bounded medium is rendered twice:
    //
    //   1. against clear depth, so the centre ray integrates z=+1 -> z=-1;
    //   2. with the existing opaque green cube at the origin, whose front face
    //      is near z=+0.5 and therefore MUST truncate the medium integral there.
    //
    // If the volume pass samples stale/empty depth, runs before deferred opaque
    // draws are flushed, or writes/reads the same depth attachment illegally,
    // the red/blue fog contribution will not fall in the blocked frame.
    {
        auto compositeDensityNode = scalarNode(0.8);
        OntoMath::Piecewise compositeDensity =
            OntoMath::Piecewise::continuous(compositeDensityNode);

        Rendering::VolumeDensityBinding medium;
        medium.origin = glm::vec3(0.0f);
        // Existing FieldNode convention: scale is the box half-span, so scale=1
        // owns z in [-1,+1]. The previous V0 implementation incorrectly halved
        // this again and integrated only [-0.5,+0.5].
        medium.scale = glm::vec3(1.0f);
        medium.densityExpr = &compositeDensity;
        medium.densityRevision = 5101;
        medium.temporalCoordinate = 0.0;
        medium.temporalDelta = 0.0;

        renderer.setVolumeDensitySources({medium}, 5201);

        // No opaque geometry: the medium owns the full bounded ray interval.
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();

        unsigned char fullMedium[4];
        readCentre(fullMedium);
        std::printf("volume composite clear-depth = (%d,%d,%d,%d)\n",
                    fullMedium[0], fullMedium[1], fullMedium[2], fullMedium[3]);

        assert(fullMedium[0] > 220 && fullMedium[1] > 220 && fullMedium[2] > 220 &&
               "FieldNode scale was not honored as the established origin±scale volume domain");

        // Opaque geometry is intentionally DEFERRED by drawMesh. composeVolumes
        // must flush it before closing the world pass, then sample the finished
        // depth in its separate no-depth-attachment pass.
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        green.drawObject();
        renderer.composeVolumes();
        renderer.endFrame();

        unsigned char depthClampedMedium[4];
        readCentre(depthClampedMedium);
        std::printf("volume composite opaque-clamped = (%d,%d,%d,%d)\n",
                    depthClampedMedium[0], depthClampedMedium[1],
                    depthClampedMedium[2], depthClampedMedium[3]);

        assert(fullMedium[0] > depthClampedMedium[0] + 50 &&
               fullMedium[2] > depthClampedMedium[2] + 50 &&
               "opaque scene depth did not truncate participating-medium transport");
        assert(depthClampedMedium[1] > depthClampedMedium[0] + 20 &&
               depthClampedMedium[1] > depthClampedMedium[2] + 20 &&
               "volume composition erased or replaced the opaque green receiver");

        // V1 EXTINCTION: hold D fixed and change only sigma_t. Low
        // extinction remains bright under the temporary V0 white-scatter model;
        // high extinction visibly reduces the premultiplied scattered RGB.
        auto extinctionNode = scalarNode(0.05);
        OntoMath::Piecewise authoredExtinction =
            OntoMath::Piecewise::continuous(extinctionNode);
        Rendering::VolumeDensityBinding extinctionMedium = medium;
        extinctionMedium.extinctionExpr = &authoredExtinction;
        extinctionMedium.extinctionRevision = 5501;
        renderer.setVolumeDensitySources({extinctionMedium}, 5501);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char lowExtinction[4];
        readCentre(lowExtinction);
        const Renderer::FrameStats lowExtinctionStats = renderer.frameStats();
        assert(lowExtinctionStats.volumeProgramCompiles == 1 &&
               "first authored sigma_t structure did not compile exactly once");

        extinctionNode->scalarForm.terms[0].coefficient = 3.0;
        extinctionMedium.extinctionRevision = 5502;
        renderer.setVolumeDensitySources({extinctionMedium}, 5502);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char highExtinction[4];
        readCentre(highExtinction);
        const Renderer::FrameStats highExtinctionStats = renderer.frameStats();
        std::printf("volume extinction low=%d high=%d compiles=%u\n",
                    lowExtinction[0], highExtinction[0],
                    highExtinctionStats.volumeProgramCompiles);
        assert(lowExtinction[0] > highExtinction[0] + 60 &&
               "changing sigma_t with fixed D did not visibly change native volume transport");
        assert(highExtinctionStats.volumeProgramCompiles == 0 &&
               "numeric sigma_t edit regenerated WGSL instead of refreshing parameters");

        // SHARED-D CACHE IDENTITY: two media may share exactly one D AST while
        // carrying different sigma_t ASTs. V0's density-only memo key would make
        // these variants evict one another every frame. V1 keys the compiled
        // medium program by (D AST, sigma_t AST), so the second identical frame
        // must hit both memos without recompiling either structure.
        auto siblingExtinctionNode = scalarNode(0.2);
        OntoMath::Piecewise siblingExtinction =
            OntoMath::Piecewise::continuous(siblingExtinctionNode);
        auto siblingExtinctionAdd = std::make_shared<OntoMath::MathNode>();
        siblingExtinctionAdd->op = OntoMath::MathNode::Op::Add;
        siblingExtinctionAdd->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.1)));
        siblingExtinctionAdd->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.1)));
        siblingExtinction.pieces[0].mathNode = siblingExtinctionAdd;

        Rendering::VolumeDensityBinding sharedDensityA = medium;
        Rendering::VolumeDensityBinding sharedDensityB = medium;
        sharedDensityA.origin.x = -0.35f;
        sharedDensityB.origin.x = 0.35f;
        sharedDensityA.extinctionExpr = &authoredExtinction;
        sharedDensityA.extinctionRevision = 5601;
        sharedDensityB.extinctionExpr = &siblingExtinction;
        sharedDensityB.extinctionRevision = 5602;
        renderer.setVolumeDensitySources({sharedDensityA, sharedDensityB}, 5603);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        const Renderer::FrameStats sharedDensityFirstStats = renderer.frameStats();
        assert(sharedDensityFirstStats.volumeProgramCompiles >= 1 &&
               "shared-D extinction variants did not establish their program structures");

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        const Renderer::FrameStats sharedDensitySecondStats = renderer.frameStats();
        assert(sharedDensitySecondStats.volumeProgramCompiles == 0 &&
               sharedDensitySecondStats.volumeProgramCacheHits >= 2 &&
               "media sharing D but differing in sigma_t thrashed the volume program cache");

        // Structural sigma_t edit must compile even though the resulting value
        // stays 3.0 and D is unchanged.
        auto extinctionAdd = std::make_shared<OntoMath::MathNode>();
        extinctionAdd->op = OntoMath::MathNode::Op::Add;
        extinctionAdd->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(1.5)));
        extinctionAdd->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(1.5)));
        authoredExtinction.pieces[0].mathNode = extinctionAdd;
        extinctionMedium.extinctionRevision = 5503;
        renderer.setVolumeDensitySources({extinctionMedium}, 5503);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        const Renderer::FrameStats extinctionStructureStats = renderer.frameStats();
        assert(extinctionStructureStats.volumeProgramCompiles == 1 &&
               "sigma_t operator-tree edit failed to regenerate volume WGSL structure");

        // Timeline: sigma_t=t, while D remains exactly the same authored tree.
        auto extinctionTimeNode = std::make_shared<OntoMath::MathNode>();
        extinctionTimeNode->op = OntoMath::MathNode::Op::ValueLeaf;
        extinctionTimeNode->variableName = OntoMath::kTimeVar;
        OntoMath::Piecewise timedExtinction =
            OntoMath::Piecewise::continuous(extinctionTimeNode);
        extinctionMedium.extinctionExpr = &timedExtinction;
        extinctionMedium.extinctionRevision = 5504;
        extinctionMedium.temporalCoordinate = 0.05;
        extinctionMedium.temporalDelta = 0.05;
        renderer.setVolumeDensitySources({extinctionMedium}, 5504);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char timedExtinctionLow[4];
        readCentre(timedExtinctionLow);
        const Renderer::FrameStats timedExtinctionCompileStats = renderer.frameStats();
        assert(timedExtinctionCompileStats.volumeProgramCompiles == 1 &&
               "introducing sigma_t(p,t) did not compile exactly once");

        extinctionMedium.temporalCoordinate = 3.0;
        extinctionMedium.temporalDelta = 2.95;
        renderer.setVolumeDensitySources({extinctionMedium}, 5504);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char timedExtinctionHigh[4];
        readCentre(timedExtinctionHigh);
        const Renderer::FrameStats timedExtinctionAdvanceStats = renderer.frameStats();
        assert(timedExtinctionLow[0] > timedExtinctionHigh[0] + 60 &&
               "advancing medium time did not visibly drive sigma_t(p,t)");
        assert(timedExtinctionAdvanceStats.volumeProgramCompiles == 0 &&
               timedExtinctionAdvanceStats.volumeProgramCacheHits >= 1 &&
               "advancing extinction time failed to reuse compiled volume structure");

        // Refusal: unsupported extinction must suppress the medium rather than
        // reusing the previous valid sigma_t program.
        auto unsupportedExtinctionNode = std::make_shared<OntoMath::MathNode>();
        unsupportedExtinctionNode->op = OntoMath::MathNode::Op::Raycast;
        timedExtinction.pieces[0].mathNode = unsupportedExtinctionNode;
        extinctionMedium.extinctionRevision = 5505;
        renderer.setVolumeDensitySources({extinctionMedium}, 5505);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char refusedExtinctionPixel[4];
        readCentre(refusedExtinctionPixel);
        const Renderer::FrameStats refusedExtinctionStats = renderer.frameStats();
        assert(refusedExtinctionStats.volumeProgramRefusals >= 1 &&
               refusedExtinctionStats.volumeLastProgramRefusal.find("extinction") != std::string::npos &&
               refusedExtinctionStats.volumeLastProgramRefusal.find("Raycast") != std::string::npos &&
               "unsupported sigma_t did not produce a named extinction refusal");
        assert(refusedExtinctionPixel[0] < 12 &&
               refusedExtinctionPixel[1] < 12 &&
               refusedExtinctionPixel[2] < 12 &&
               "refused sigma_t left stale volumetric output on screen");

        // TIMELINE: D(p,t)=t is the same authored structure across both frames.
        // Only this medium's admitted Timeline coordinate changes. The second
        // frame must visibly brighten while reusing the already-compiled volume
        // program; time is runtime data, never an AST mutation.
        auto densityTimeNode = std::make_shared<OntoMath::MathNode>();
        densityTimeNode->op = OntoMath::MathNode::Op::ValueLeaf;
        densityTimeNode->variableName = OntoMath::kTimeVar;
        OntoMath::Piecewise timedDensity =
            OntoMath::Piecewise::continuous(densityTimeNode);

        Rendering::VolumeDensityBinding timedMedium = medium;
        timedMedium.densityExpr = &timedDensity;
        timedMedium.densityRevision = 5301;
        timedMedium.temporalCoordinate = 0.05;
        timedMedium.temporalDelta = 0.05;
        renderer.setVolumeDensitySources({timedMedium}, 5401);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();

        unsigned char timedDensityDim[4];
        readCentre(timedDensityDim);
        const Renderer::FrameStats timedDensityCompileStats = renderer.frameStats();
        assert(timedDensityCompileStats.volumeProgramCompiles == 1 &&
               "introducing D(p,t) did not compile its density structure exactly once");

        timedMedium.temporalCoordinate = 1.0;
        timedMedium.temporalDelta = 0.95;
        // Membership and authored density content are unchanged. Keep the set
        // and density revisions fixed so only runtime Timeline data advances.
        renderer.setVolumeDensitySources({timedMedium}, 5401);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();

        unsigned char timedDensityBright[4];
        readCentre(timedDensityBright);
        const Renderer::FrameStats timedDensityAdvanceStats = renderer.frameStats();
        std::printf("volume density timeline t=.05:%d t=1:%d compiles=%u cacheHits=%u\n",
                    timedDensityDim[0], timedDensityBright[0],
                    timedDensityAdvanceStats.volumeProgramCompiles,
                    timedDensityAdvanceStats.volumeProgramCacheHits);
        assert(timedDensityBright[0] > timedDensityDim[0] + 60 &&
               "advancing the medium Timeline did not visibly change D(p,t)");
        assert(timedDensityAdvanceStats.volumeProgramCompiles == 0 &&
               "advancing density time regenerated the volume shader");
        assert(timedDensityAdvanceStats.volumeProgramCacheHits >= 1 &&
               "advancing density time failed to reuse the memoized volume program");

        // REFUSAL: replace only D's authored structure with unsupported Raycast.
        // The production composite must surface a named refusal and render no
        // stale medium from the previously valid timed density program.
        auto unsupportedDensityNode = std::make_shared<OntoMath::MathNode>();
        unsupportedDensityNode->op = OntoMath::MathNode::Op::Raycast;
        timedDensity.pieces[0].mathNode = unsupportedDensityNode;
        timedMedium.densityRevision = 5302;
        renderer.setVolumeDensitySources({timedMedium}, 5402);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();

        unsigned char refusedDensityPixel[4];
        readCentre(refusedDensityPixel);
        const Renderer::FrameStats refusedDensityStats = renderer.frameStats();
        assert(refusedDensityStats.volumeProgramRefusals >= 1 &&
               "unsupported authored density did not surface a production volume refusal");
        assert(refusedDensityStats.volumeLastProgramRefusal.find("Raycast") != std::string::npos &&
               "production density refusal did not name the unsupported authored operation");
        assert(refusedDensityPixel[0] < 12 &&
               refusedDensityPixel[1] < 12 &&
               refusedDensityPixel[2] < 12 &&
               "refused authored density left stale volumetric output on screen");

        renderer.setVolumeDensitySources({}, 0);
    }

    // Unhook before the renderer (a stack object) goes out of scope: globals such
    // as ZoneManager are destroyed after main returns and can still reach for the
    // active backend, which by then would be a dangling pointer.
    // --- A FIELD object: raymarched, not tessellated (Milestone 6) -------------
    // The point of routing through the real Object path: drawFieldModel asks the
    // backend whether it renders implicits exactly, and only WebGPU says yes. If
    // that wiring breaks, this silently falls back to the mesh and still looks
    // plausible — so assert the SHAPE, which the bounding box would not produce.
    {
        geom::SdfNode sphere;
        sphere.op   = geom::SdfOp::Leaf;
        sphere.prim = geom::SdfPrim::Sphere;
        sphere.dims = glm::vec3(0.55f);

        Object field;
        field.setFieldShape(sphere, glm::vec3(1.0f));
        // Painted explicitly: a field is one face, and nothing gives an object
        // paint it was not given any more, so the colour under test has to be
        // put there rather than inherited from a construction-time palette.
        field.setFaceColor(0, 1.0f, 0.0f, 0.0f);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        field.drawObject();
        renderer.endFrame();
    }
    unsigned char f0[4], fcorner[4];
    readCentre(f0);
    readAt(0, 0, fcorner);
    std::printf("field centre        = (%d,%d,%d,%d)  corner = (%d,%d,%d,%d)\n",
                f0[0], f0[1], f0[2], f0[3], fcorner[0], fcorner[1], fcorner[2], fcorner[3]);
    assert(f0[0] > 30 && "raymarched field produced no surface at the centre");
    // The field's face-0 paint is RED, and tessellateSdf samples one texel, so
    // the raymarched surface must be red too — not white. This is the check
    // that catches a field silently changing colour with the backend.
    assert(f0[0] > f0[1] + 30 && f0[0] > f0[2] + 30 &&
           "field ignored its face paint — raymarch and mesh paths disagree on colour");
    assert(fcorner[0] < 12 && fcorner[1] < 12 && fcorner[2] < 12 &&
           "corner is lit — the bounding box was painted instead of the sphere traced");
    assert(currentRenderer().rendersImplicitExactly() &&
           "WebGPU should report exact implicit rendering");

    // --- Authored radiance live-edit witness ---------------------------------
    // This is a REAL Object -> drawFieldModel -> drawImplicit path, not a direct
    // shader fixture. Numeric edits to rho must change pixels while reusing the
    // compiled WGSL; operator-tree edits must recompile; unsupported authored
    // math must refuse visibly rather than leave an old shader answer on screen.
    {
        auto radiantMat = materials.create("webgpu_radiance_witness");
        radiantMat->baseColor = glm::vec3(1.0f);
        radiantMat->ambient = 0.0f;   // isolate rho on diffuse light
        radiantMat->diffuse = 1.0f;
        radiantMat->specular = 0.0f;

        Object radiant;
        radiant.setFieldShape(
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.55f)),
            glm::vec3(1.0f));
        radiant.setMaterialId("material.webgpu_radiance_witness");

        auto rhoLeaf = scalarNode(1.0);
        OntoMath::Piecewise rho = OntoMath::Piecewise::continuous(rhoLeaf);

        renderer.setLight(glm::vec3(0.0f, 0.0f, 2.0f),
                   glm::vec3(0.2f), glm::vec3(0.8f), glm::vec3(1.0f));
        renderer.setLightingEnabled(true);
        renderer.setRadianceField(&rho, 1001);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char rhoBright[4];
        readCentre(rhoBright);
        const Renderer::FrameStats firstStats = renderer.frameStats();
        assert(firstStats.sdfProgramCompiles == 1 &&
               "first authored-radiance draw must compile its SDF program");

        // VALUE ONLY: same ScalarLeaf structure, new coefficient.
        rhoLeaf->scalarForm.terms[0].coefficient = 0.15;
        renderer.setRadianceField(&rho, 1002);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char rhoDim[4];
        readCentre(rhoDim);
        const Renderer::FrameStats valueStats = renderer.frameStats();

        std::printf("radiance live edit centre bright=%d dim=%d compiles=%u cacheHits=%u\n",
                    rhoBright[0], rhoDim[0], valueStats.sdfProgramCompiles,
                    valueStats.sdfProgramCacheHits);
        assert(rhoBright[0] > rhoDim[0] + 80 &&
               "numeric authored rho edit did not change rendered radiance");
        assert(valueStats.sdfProgramCompiles == 0 &&
               "numeric authored rho edit recompiled WGSL instead of refreshing parameters");
        assert(valueStats.sdfProgramCacheHits >= 1 &&
               "numeric authored rho edit did not reuse the memoized SDF program");
        assert(valueStats.sdfParameterBytesUploaded > 0 &&
               "numeric authored rho edit reused stale GPU parameters instead of uploading refreshed values");

        // RUNG 4 TIME: make rho read the canonical temporal coordinate.
        // Timeline is RELATIVE: any Singular may own one. Here the radiance
        // SOURCE owns an ordinary Timeline; the Object below is only the
        // receiver being shaded. This prevents source time from being confused
        // with per-surface time while leaving future Law selection open.
        auto timeLeaf = std::make_shared<OntoMath::MathNode>();
        timeLeaf->op = OntoMath::MathNode::Op::ValueLeaf;
        timeLeaf->variableName = OntoMath::kTimeVar;
        rho.pieces[0].mathNode = timeLeaf;

        RadianceSourceProbe radianceSource;
        Timeline localTimeline;
        Relation localTimelineOwnership(
            "owned-by", localTimeline, radianceSource, true, 1.0f);
        assert(localTimelineOwnership.a() == &localTimeline);
        assert(localTimelineOwnership.b() == &radianceSource);
        assert(localTimelineOwnership.typeLabel() == "owned-by");

        assert(localTimeline.setClock(0.15, 0.15));
        renderer.setRadianceTemporalCoordinate(localTimeline.now(),
                                       localTimeline.delta());
        renderer.setRadianceField(&rho, 1003);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char timeDim[4];
        readCentre(timeDim);
        const Renderer::FrameStats timeCompileStats = renderer.frameStats();
        assert(timeCompileStats.sdfProgramCompiles == 1 &&
               "introducing canonical t should compile the new rho structure once");

        assert(localTimeline.setClock(1.0, 0.85));
        renderer.setRadianceTemporalCoordinate(localTimeline.now(),
                                       localTimeline.delta());
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char timeBright[4];
        readCentre(timeBright);
        const Renderer::FrameStats timeAdvanceStats = renderer.frameStats();

        std::printf("radiance timeline centre t=.15:%d t=1:%d compiles=%u cacheHits=%u paramBytes=%zu\n",
                    timeDim[0], timeBright[0], timeAdvanceStats.sdfProgramCompiles,
                    timeAdvanceStats.sdfProgramCacheHits,
                    timeAdvanceStats.sdfParameterBytesUploaded);
        assert(timeBright[0] > timeDim[0] + 80 &&
               "advancing the radiance source Timeline did not visibly change rho(p,t)");
        assert(timeAdvanceStats.sdfProgramCompiles == 0 &&
               "advancing t recompiled WGSL instead of updating the shared uniform");
        assert(timeAdvanceStats.sdfProgramCacheHits >= 1 &&
               "advancing t failed to reuse the memoized SDF program");
        assert(timeAdvanceStats.sdfParameterBytesUploaded == 0 &&
               "advancing t incorrectly rewrote the authored SDF parameter buffer");

        // STRUCTURE: ValueLeaf(t) -> Add(ScalarLeaf, ScalarLeaf).
        auto add = std::make_shared<OntoMath::MathNode>();
        add->op = OntoMath::MathNode::Op::Add;
        add->children.push_back(std::make_unique<OntoMath::MathNode>(*scalarNode(0.5)));
        add->children.push_back(std::make_unique<OntoMath::MathNode>(*scalarNode(0.5)));
        rho.pieces[0].mathNode = add;
        renderer.setRadianceField(&rho, 1004);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        const Renderer::FrameStats structureStats = renderer.frameStats();
        assert(structureStats.sdfProgramCompiles == 1 &&
               "radiance operator-tree edit failed to regenerate WGSL structure");

        // REFUSAL: Raycast is deliberately unsupported by the GPU compiler.
        auto unsupported = std::make_shared<OntoMath::MathNode>();
        unsupported->op = OntoMath::MathNode::Op::Raycast;
        rho.pieces[0].mathNode = unsupported;
        renderer.setRadianceField(&rho, 1005);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char refusedPixel[4];
        readCentre(refusedPixel);
        const Renderer::FrameStats refusalStats = renderer.frameStats();
        assert(refusalStats.sdfProgramRefusals >= 1 &&
               "unsupported authored rho did not surface an explicit renderer refusal");
        assert(refusalStats.sdfLastProgramRefusal.find("Raycast") != std::string::npos &&
               "renderer refusal did not name the unsupported authored operation");
        assert(refusedPixel[0] < 12 && refusedPixel[1] < 12 && refusedPixel[2] < 12 &&
               "refused authored rho left stale rendered radiance on screen");

        // RUNG 5 CHROMA: restore rho, then prove chi is source-side RGB
        // mathematics received by this white surface. The legacy source color is
        // GREEN on purpose; authored RED chi must replace it rather than multiply
        // it, which would otherwise collapse to black in the red channel.
        auto rhoRestored = scalarNode(1.0);
        rho.pieces[0].mathNode = rhoRestored;
        renderer.setRadianceField(&rho, 1006);
        renderer.setRadianceSourceCoefficients(1.0f, 0.2f, 0.8f, 1.0f);
        renderer.setLight(glm::vec3(0.0f, 0.0f, 2.0f),
                          glm::vec3(0.0f, 0.2f, 0.0f),
                          glm::vec3(0.0f, 0.8f, 0.0f),
                          glm::vec3(0.0f, 1.0f, 0.0f));
        renderer.setRadianceChroma(nullptr, 0);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char legacyGreen[4];
        readCentre(legacyGreen);
        assert(legacyGreen[1] > legacyGreen[0] + 60 &&
               "source without chi no longer uses legacy light.color");

        auto chiNode = vectorNode(1.0, 0.0, 0.0);
        OntoMath::Piecewise chi = OntoMath::Piecewise::continuous(chiNode);
        renderer.setRadianceChroma(&chi, 2001);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char chiRed[4];
        readCentre(chiRed);
        const Renderer::FrameStats chiCompileStats = renderer.frameStats();
        std::printf("chroma legacy G=%d authored red=(%d,%d,%d) compiles=%u\n",
                    legacyGreen[1], chiRed[0], chiRed[1], chiRed[2],
                    chiCompileStats.sdfProgramCompiles);
        assert(chiRed[0] > chiRed[1] + 60 && chiRed[0] > chiRed[2] + 60 &&
               "authored chi did not replace legacy source chroma on the receiver");
        assert(chiCompileStats.sdfProgramCompiles == 1 &&
               "introducing authored chi should compile its new structure once");

        // VALUE ONLY: red -> blue without changing VectorConstruct structure.
        chiNode->children[0]->scalarForm.terms[0].coefficient = 0.0;
        chiNode->children[2]->scalarForm.terms[0].coefficient = 1.0;
        renderer.setRadianceChroma(&chi, 2002);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char chiBlue[4];
        readCentre(chiBlue);
        const Renderer::FrameStats chiValueStats = renderer.frameStats();
        assert(chiBlue[2] > chiBlue[0] + 60 && chiBlue[2] > chiBlue[1] + 60 &&
               "numeric chi edit did not visibly recolor received illumination");
        assert(chiValueStats.sdfProgramCompiles == 0 &&
               chiValueStats.sdfProgramCacheHits >= 1 &&
               chiValueStats.sdfParameterBytesUploaded > 0 &&
               "numeric chi edit did not take the parameter-refresh cache path");

        // STRUCTURE + RELATIVE TIME: chi=(t,0,0). The SAME source-owned
        // Timeline used above drives chroma; the receiving Object owns no clock.
        auto timedChiNode = std::make_shared<OntoMath::MathNode>();
        timedChiNode->op = OntoMath::MathNode::Op::VectorConstruct;
        auto chiTime = std::make_unique<OntoMath::MathNode>();
        chiTime->op = OntoMath::MathNode::Op::ValueLeaf;
        chiTime->variableName = OntoMath::kTimeVar;
        timedChiNode->children.push_back(std::move(chiTime));
        timedChiNode->children.push_back(std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
        timedChiNode->children.push_back(std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
        chi.pieces[0].mathNode = timedChiNode;
        assert(localTimeline.setClock(0.15, 0.15));
        renderer.setRadianceTemporalCoordinate(localTimeline.now(), localTimeline.delta());
        renderer.setRadianceChroma(&chi, 2003);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char chiTimeDim[4];
        readCentre(chiTimeDim);
        const Renderer::FrameStats chiTimeCompileStats = renderer.frameStats();
        assert(chiTimeCompileStats.sdfProgramCompiles == 1 &&
               "introducing timed chi should compile the new chroma structure once");

        assert(localTimeline.setClock(1.0, 0.85));
        renderer.setRadianceTemporalCoordinate(localTimeline.now(), localTimeline.delta());
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char chiTimeBright[4];
        readCentre(chiTimeBright);
        const Renderer::FrameStats chiTimeAdvanceStats = renderer.frameStats();
        assert(chiTimeBright[0] > chiTimeDim[0] + 60 &&
               "advancing the source Timeline did not visibly change chi(p,t)");
        assert(chiTimeAdvanceStats.sdfProgramCompiles == 0 &&
               chiTimeAdvanceStats.sdfProgramCacheHits >= 1 &&
               chiTimeAdvanceStats.sdfParameterBytesUploaded == 0 &&
               "advancing chi time mutated authored parameters or recompiled WGSL");

        // REFUSAL: authored chroma exists but is malformed/unsupported. It may
        // NOT fall back to legacy green or reuse the previously compiled red chi.
        auto badChi = std::make_shared<OntoMath::MathNode>();
        badChi->op = OntoMath::MathNode::Op::VectorConstruct;
        auto badRay = std::make_unique<OntoMath::MathNode>();
        badRay->op = OntoMath::MathNode::Op::Raycast;
        badChi->children.push_back(std::move(badRay));
        badChi->children.push_back(std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
        badChi->children.push_back(std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
        chi.pieces[0].mathNode = badChi;
        renderer.setRadianceChroma(&chi, 2004);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char chiRefused[4];
        readCentre(chiRefused);
        const Renderer::FrameStats chiRefusalStats = renderer.frameStats();
        assert(chiRefusalStats.sdfProgramRefusals >= 1 &&
               chiRefusalStats.sdfLastProgramRefusal.find("chroma") != std::string::npos &&
               chiRefusalStats.sdfLastProgramRefusal.find("Raycast") != std::string::npos &&
               "unsupported authored chi did not surface a named chroma refusal");
        assert(chiRefused[0] < 12 && chiRefused[1] < 12 && chiRefused[2] < 12 &&
               "refused authored chi fell back or left stale rendered illumination");


        // RUNG 6 ANGULAR EMISSION: restore valid white chi, then shape the same
        // source by alpha(p,omega,t). At this receiver the outgoing world-space
        // source->receiver direction points approximately -Z, so -omega.z is a
        // bright lobe and a small coefficient is visibly dimmer.
        chi.pieces[0].mathNode = vectorNode(1.0, 1.0, 1.0);
        renderer.setRadianceChroma(&chi, 2005);

        auto angularScale = scalarNode(-1.0);
        auto omegaZ = std::make_unique<OntoMath::MathNode>();
        omegaZ->op = OntoMath::MathNode::Op::ValueLeaf;
        omegaZ->variableName = OntoMath::kOmegaZVar;
        auto angularNode = std::make_shared<OntoMath::MathNode>();
        angularNode->op = OntoMath::MathNode::Op::Scale;
        angularNode->children.push_back(
            std::make_unique<OntoMath::MathNode>(*angularScale));
        angularNode->children.push_back(std::move(omegaZ));
        OntoMath::Piecewise alpha = OntoMath::Piecewise::continuous(angularNode);

        renderer.setRadianceAngular(&alpha, 3001);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char alphaBright[4];
        readCentre(alphaBright);
        const Renderer::FrameStats alphaCompileStats = renderer.frameStats();
        assert(alphaCompileStats.sdfProgramCompiles == 1 &&
               "introducing authored alpha should compile its new structure once");

        // VALUE ONLY: same Scale(number, omega.z), smaller coefficient.
        angularNode->children[0]->scalarForm.terms[0].coefficient = -0.05;
        renderer.setRadianceAngular(&alpha, 3002);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char alphaDim[4];
        readCentre(alphaDim);
        const Renderer::FrameStats alphaValueStats = renderer.frameStats();
        std::printf("angular lobe centre bright=%d dim=%d compiles=%u cacheHits=%u paramBytes=%zu\n",
                    alphaBright[0], alphaDim[0], alphaValueStats.sdfProgramCompiles,
                    alphaValueStats.sdfProgramCacheHits,
                    alphaValueStats.sdfParameterBytesUploaded);
        assert(alphaBright[0] > alphaDim[0] + 60 &&
               "authored alpha(omega) did not visibly shape received illumination");
        assert(alphaValueStats.sdfProgramCompiles == 0 &&
               alphaValueStats.sdfProgramCacheHits >= 1 &&
               alphaValueStats.sdfParameterBytesUploaded > 0 &&
               "numeric alpha edit did not take the parameter-refresh cache path");

        // STRUCTURE + RELATIVE TIME: rotate a lobe axis through the XZ plane:
        // alpha = -(omega.x*sin(t) + omega.z*cos(t)). The same source-owned
        // Timeline advances the direction without changing authored parameters.
        auto omegaXTimed = std::make_unique<OntoMath::MathNode>();
        omegaXTimed->op = OntoMath::MathNode::Op::ValueLeaf;
        omegaXTimed->variableName = OntoMath::kOmegaXVar;
        auto omegaZTimed = std::make_unique<OntoMath::MathNode>();
        omegaZTimed->op = OntoMath::MathNode::Op::ValueLeaf;
        omegaZTimed->variableName = OntoMath::kOmegaZVar;

        auto sinTime = std::make_unique<OntoMath::MathNode>();
        sinTime->op = OntoMath::MathNode::Op::ScalarLeaf;
        sinTime->scalarForm =
            OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Sin,
                                                 OntoMath::kTimeVar);
        auto cosTime = std::make_unique<OntoMath::MathNode>();
        cosTime->op = OntoMath::MathNode::Op::ScalarLeaf;
        cosTime->scalarForm =
            OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Cos,
                                                 OntoMath::kTimeVar);

        auto xProjection = std::make_unique<OntoMath::MathNode>();
        xProjection->op = OntoMath::MathNode::Op::Scale;
        xProjection->children.push_back(std::move(omegaXTimed));
        xProjection->children.push_back(std::move(sinTime));
        auto zProjection = std::make_unique<OntoMath::MathNode>();
        zProjection->op = OntoMath::MathNode::Op::Scale;
        zProjection->children.push_back(std::move(omegaZTimed));
        zProjection->children.push_back(std::move(cosTime));
        auto rotatingDot = std::make_unique<OntoMath::MathNode>();
        rotatingDot->op = OntoMath::MathNode::Op::Add;
        rotatingDot->children.push_back(std::move(xProjection));
        rotatingDot->children.push_back(std::move(zProjection));
        auto rotatingAlpha = std::make_shared<OntoMath::MathNode>();
        rotatingAlpha->op = OntoMath::MathNode::Op::Scale;
        rotatingAlpha->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(-1.0)));
        rotatingAlpha->children.push_back(std::move(rotatingDot));
        alpha.pieces[0].mathNode = rotatingAlpha;

        assert(localTimeline.setClock(0.0, -1.0));
        renderer.setRadianceTemporalCoordinate(localTimeline.now(), localTimeline.delta());
        renderer.setRadianceAngular(&alpha, 3003);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char rotatingToward[4];
        readCentre(rotatingToward);
        const Renderer::FrameStats alphaTimeCompileStats = renderer.frameStats();
        assert(alphaTimeCompileStats.sdfProgramCompiles == 1 &&
               "introducing rotating alpha should compile its structure once");

        assert(localTimeline.setClock(3.141592653589793, 3.141592653589793));
        renderer.setRadianceTemporalCoordinate(localTimeline.now(), localTimeline.delta());
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char rotatingAway[4];
        readCentre(rotatingAway);
        const Renderer::FrameStats alphaTimeAdvanceStats = renderer.frameStats();
        assert(rotatingToward[0] > rotatingAway[0] + 60 &&
               "advancing the source Timeline did not visibly rotate alpha(omega,t)");
        assert(alphaTimeAdvanceStats.sdfProgramCompiles == 0 &&
               alphaTimeAdvanceStats.sdfProgramCacheHits >= 1 &&
               alphaTimeAdvanceStats.sdfParameterBytesUploaded == 0 &&
               "advancing angular time mutated authored parameters or recompiled WGSL");

        // REFUSAL: unsupported alpha must not reuse the previously bright lobe.
        auto badAlpha = std::make_shared<OntoMath::MathNode>();
        badAlpha->op = OntoMath::MathNode::Op::Raycast;
        alpha.pieces[0].mathNode = badAlpha;
        renderer.setRadianceAngular(&alpha, 3004);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char alphaRefused[4];
        readCentre(alphaRefused);
        const Renderer::FrameStats alphaRefusalStats = renderer.frameStats();
        assert(alphaRefusalStats.sdfProgramRefusals >= 1 &&
               alphaRefusalStats.sdfLastProgramRefusal.find("angular") != std::string::npos &&
               alphaRefusalStats.sdfLastProgramRefusal.find("Raycast") != std::string::npos &&
               "unsupported authored alpha did not surface a named angular refusal");
        assert(alphaRefused[0] < 12 && alphaRefused[1] < 12 && alphaRefused[2] < 12 &&
               "refused authored alpha left stale rendered illumination");

        renderer.setRadianceAngular(nullptr, 0);
        renderer.setRadianceChroma(nullptr, 0);
        renderer.setRadianceField(nullptr, 0);
        renderer.setRadianceTemporalCoordinate(0.0, 0.0);

        // RUNG 7 WORLD COMPOSITION: two independent authored sources illuminate
        // the SAME receiver. Source 0 is red, source 1 blue. The shader must sum
        // their contributions above rho/chi/alpha, not fuse the ASTs into one
        // source or let the second source overwrite the first.
        auto rhoRedNode = scalarNode(0.45);
        auto rhoBlueNode = scalarNode(0.45);
        auto chiRedNode = vectorNode(1.0, 0.0, 0.0);
        auto chiBlueNode = vectorNode(0.0, 0.0, 1.0);
        OntoMath::Piecewise rhoRed = OntoMath::Piecewise::continuous(rhoRedNode);
        OntoMath::Piecewise rhoBlue = OntoMath::Piecewise::continuous(rhoBlueNode);
        OntoMath::Piecewise chiRedSource = OntoMath::Piecewise::continuous(chiRedNode);
        OntoMath::Piecewise chiBlueSource = OntoMath::Piecewise::continuous(chiBlueNode);

        Rendering::RadianceSourceBinding redSource;
        redSource.position = glm::vec3(-0.25f, 0.0f, 2.0f);
        redSource.coefficients = glm::vec4(1.0f, 0.2f, 0.8f, 1.0f);
        redSource.radianceExpr = &rhoRed;
        redSource.radianceRevision = 4001;
        redSource.chromaExpr = &chiRedSource;
        redSource.chromaRevision = 4101;

        Rendering::RadianceSourceBinding blueSource;
        blueSource.position = glm::vec3(0.25f, 0.0f, 2.0f);
        blueSource.coefficients = glm::vec4(1.0f, 0.2f, 0.8f, 1.0f);
        blueSource.radianceExpr = &rhoBlue;
        blueSource.radianceRevision = 4002;
        blueSource.chromaExpr = &chiBlueSource;
        blueSource.chromaRevision = 4102;

        renderer.setRadianceSources({redSource, blueSource}, 4201);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char twoSourcePixel[4];
        readCentre(twoSourcePixel);
        const Renderer::FrameStats twoSourceCompileStats = renderer.frameStats();

        std::printf("two-source radiance centre=(%d,%d,%d) compiles=%u\n",
                    twoSourcePixel[0], twoSourcePixel[1], twoSourcePixel[2],
                    twoSourceCompileStats.sdfProgramCompiles);
        assert(twoSourcePixel[0] > 30 && twoSourcePixel[2] > 30 &&
               twoSourcePixel[1] + 20 < twoSourcePixel[0] &&
               twoSourcePixel[1] + 20 < twoSourcePixel[2] &&
               "two independent red/blue sources did not both contribute to the receiver");
        assert(twoSourceCompileStats.sdfProgramCompiles == 1 &&
               "entering a two-source structural world should compile exactly once");

        // VALUE ONLY OUTSIDE AST PARAMETERS: disable the blue source. This must
        // update the source storage buffer without regenerating WGSL or touching
        // the packed OntoMath parameter buffer.
        blueSource.enabled = false;
        // Enablement is source-buffer state, not authored rho/chi/alpha identity.
        // Keep the source-set revision unchanged: WebGpuRenderer must observe the
        // byte change through the persistent source buffer without recollecting
        // OntoMath parameters or recompiling WGSL.
        renderer.setRadianceSources({redSource, blueSource}, 4201);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char redOnlyPixel[4];
        readCentre(redOnlyPixel);
        const Renderer::FrameStats enableValueStats = renderer.frameStats();

        assert(twoSourcePixel[2] > redOnlyPixel[2] + 25 &&
               "disabling source 1 did not remove its blue contribution");
        assert(redOnlyPixel[0] > redOnlyPixel[2] + 30 &&
               "disabling source 1 damaged source 0's independent red contribution");
        assert(enableValueStats.sdfProgramCompiles == 0 &&
               enableValueStats.sdfProgramCacheHits >= 1 &&
               "source enablement value edit recompiled the composed shader");
        assert(enableValueStats.sdfParameterBytesUploaded == 0 &&
               "source enablement incorrectly rewrote authored OntoMath parameters");

        // RUNG 8 DERIVED VISIBILITY: geometry may stand between a source and
        // receiver without becoming part of rho/chi/alpha. This first truthful
        // baseline is deliberately scoped to geometry owned by the executing SDF
        // pipeline; the renderer leaves it disabled by default until arbitrary
        // cross-pipeline scene transport has a shared geometry representation.
        //
        // Build one authored SDF containing the receiver sphere plus a small
        // blocker on ONLY the red-source path. The camera ray through the centre
        // still hits the receiver, while the blue path remains unobstructed.
        blueSource.enabled = true;
        redSource.position = glm::vec3(-0.8f, 0.0f, 1.6f);
        blueSource.position = glm::vec3(0.8f, 0.0f, 1.6f);
        redSource.coefficients = glm::vec4(1.0f, 0.0f, 1.0f, 0.0f);
        blueSource.coefficients = glm::vec4(1.0f, 0.0f, 1.0f, 0.0f);
        renderer.setRadianceSources({redSource, blueSource}, 4201);

        const std::string rhoRedBeforeVisibility = rhoRed.toJson().dump();
        const std::string rhoBlueBeforeVisibility = rhoBlue.toJson().dump();
        const std::string chiRedBeforeVisibility = chiRedSource.toJson().dump();
        const std::string chiBlueBeforeVisibility = chiBlueSource.toJson().dump();

        auto receiver =
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.55f));

        // Establish the exact V=1 pixel baseline with the SAME source values
        // before any blocker exists. Adding off-axis blocker geometry while
        // visibility remains disabled must not perturb this receiver sample.
        radiant.setFieldShape(receiver, glm::vec3(1.5f));
        renderer.setRadianceVisibilityEnabled(false);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char visibilityBaseline[4];
        readCentre(visibilityBaseline);

        // Visibility with no blocker must be observationally identical to V=1.
        // This specifically guards against the primary marcher terminating a hair
        // inside the receiver and the secondary transport ray then shadowing the
        // receiver against itself.
        renderer.setRadianceVisibilityEnabled(true);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char receiverOnlyVisibility[4];
        readCentre(receiverOnlyVisibility);
        const Renderer::FrameStats receiverOnlyVisibilityStats = renderer.frameStats();
        assert(abs(int(receiverOnlyVisibility[0]) - int(visibilityBaseline[0])) <= 2 &&
               abs(int(receiverOnlyVisibility[1]) - int(visibilityBaseline[1])) <= 2 &&
               abs(int(receiverOnlyVisibility[2]) - int(visibilityBaseline[2])) <= 2 &&
               "visibility query self-shadowed the receiver with no blocker present");
        assert(receiverOnlyVisibilityStats.sdfProgramCompiles == 0 &&
               receiverOnlyVisibilityStats.sdfProgramCacheHits >= 1 &&
               "enabling receiver-only visibility regenerated shader structure");

        auto blocker =
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.16f));
        blocker.offset = glm::vec3(-0.4f, 0.0f, 1.075f);
        auto shadowField =
            geom::SdfNode::binary(geom::SdfOp::Union, receiver, blocker);
        radiant.setFieldShape(shadowField, glm::vec3(1.5f));

        // Compatibility law: transport disabled is exactly V=1. The blocker is
        // real geometry, but it must not alter source emission while this derived
        // query is disabled.
        renderer.setRadianceVisibilityEnabled(false);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char visibilityOff[4];
        readCentre(visibilityOff);
        const Renderer::FrameStats visibilityOffStats = renderer.frameStats();
        assert(visibilityOff[0] > 35 && visibilityOff[2] > 35 &&
               "V=1 compatibility did not preserve both direct source contributions");
        assert(abs(int(visibilityOff[0]) - int(visibilityBaseline[0])) <= 2 &&
               abs(int(visibilityOff[1]) - int(visibilityBaseline[1])) <= 2 &&
               abs(int(visibilityOff[2]) - int(visibilityBaseline[2])) <= 2 &&
               "visibility-disabled V=1 changed the pre-shadow receiver pixel");
        assert(visibilityOffStats.sdfProgramCompiles == 1 &&
               "new receiver+blocker SDF structure should compile exactly once");

        // Value-only execution change: enable derived transport. The red path
        // intersects the blocker; blue does not. Source ASTs and shader structure
        // remain untouched.
        renderer.setRadianceVisibilityEnabled(true);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char redBlocked[4];
        readCentre(redBlocked);
        const Renderer::FrameStats visibilityOnStats = renderer.frameStats();
        std::printf("Rung-8 visibility off=(%d,%d,%d) on=(%d,%d,%d) compiles=%u cacheHits=%u\n",
                    visibilityOff[0], visibilityOff[1], visibilityOff[2],
                    redBlocked[0], redBlocked[1], redBlocked[2],
                    visibilityOnStats.sdfProgramCompiles,
                    visibilityOnStats.sdfProgramCacheHits);
        assert(visibilityOff[0] > redBlocked[0] + 25 &&
               "blocker on source 0 path did not suppress the red direct contribution");
        assert(redBlocked[2] > 35 &&
               abs(int(redBlocked[2]) - int(visibilityOff[2])) < 25 &&
               "blocking source 0 damaged independent source 1 transport");
        assert(visibilityOnStats.sdfProgramCompiles == 0 &&
               visibilityOnStats.sdfProgramCacheHits >= 1 &&
               "enabling derived visibility regenerated source/SDF shader structure");

        // Move only the blocker behind the receiver. This is a geometry VALUE
        // edit with identical Union/Sphere topology. The red path must return,
        // while all four source invariants remain byte-for-byte unchanged.
        const auto blockerStructureBefore = radiant.getSdfStructureRevision();
        const auto blockerParamsBefore = radiant.getSdfParameterRevision();
        radiant.setFieldOperandBOffset(glm::vec3(-0.4f, 0.0f, -0.8f));
        assert(radiant.getSdfStructureRevision() == blockerStructureBefore &&
               "blocker value motion incorrectly invalidated SDF structure");
        assert(radiant.getSdfParameterRevision() != blockerParamsBefore &&
               "blocker value motion did not advance SDF parameter revision");
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        radiant.drawObject();
        renderer.endFrame();
        unsigned char blockerMoved[4];
        readCentre(blockerMoved);
        const Renderer::FrameStats blockerMoveStats = renderer.frameStats();
        assert(blockerMoved[0] > redBlocked[0] + 25 &&
               "moving the blocker away left a stale shadow");
        assert(blockerMoved[2] > 35 &&
               "moving the red blocker damaged the blue source contribution");
        assert(blockerMoveStats.sdfProgramCompiles == 0 &&
               blockerMoveStats.sdfProgramCacheHits >= 1 &&
               "numeric blocker motion regenerated WGSL instead of refreshing geometry values");
        assert(rhoRed.toJson().dump() == rhoRedBeforeVisibility &&
               rhoBlue.toJson().dump() == rhoBlueBeforeVisibility &&
               chiRedSource.toJson().dump() == chiRedBeforeVisibility &&
               chiBlueSource.toJson().dump() == chiBlueBeforeVisibility &&
               "derived visibility leaked blocker state into authored source invariants");

        renderer.setRadianceVisibilityEnabled(false);
        renderer.setRadianceSources({}, 0);
    }

    // --- An unpainted cube draws as ONE merged mesh; painting a single face
    // must drop it straight back to the six-face path (remediation plan Phase
    // 4.2). This is the guard the plan calls for: the merge decision reads the
    // resolved albedo fresh every draw rather than being cached on the Object.
    {
        Object mergeCube;
        mergeCube.setShapeKind(Object::ShapeKind::Cube);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        mergeCube.drawObject();
        renderer.endFrame();
        const int unpaintedDrawCalls = renderer.frameStats().drawCalls;
        std::printf("unpainted cube draw calls = %d\n", unpaintedDrawCalls);
        assert(unpaintedDrawCalls == 1 &&
               "an unpainted cube's six faces resolve identical paint and should merge to one draw");

        mergeCube.setFaceColor(0, 1.0f, 0.0f, 0.0f); // one stroke, one face

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        mergeCube.drawObject();
        renderer.endFrame();
        const int paintedDrawCalls = renderer.frameStats().drawCalls;
        std::printf("single-face-painted cube draw calls = %d\n", paintedDrawCalls);
        assert(paintedDrawCalls == 6 &&
               "a single painted face must fall back to the six-draw path, not merge over the paint");
    }

    {
        std::vector<Object> toruses(10);
        for (int i = 0; i < 10; ++i) {
            toruses[i].setShapeKind(Object::ShapeKind::Torus);
            // Force Mesh path. WebGPU raymarches Parametrics exactly (drawImplicit) which does not batch.
            // This tests the Phase 4 tessellation cache, which applies to the Mesh path.
            toruses[i].setRenderMode(Object::RenderMode::Mesh);
        }
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        for (int i = 0; i < 10; ++i) {
            renderer.setModel(glm::mat4(1.0f));
            toruses[i].drawObject();
        }
        renderer.endFrame();
        const int torusDrawCalls = renderer.frameStats().drawCalls;
        std::printf("10 identical toruses draw calls = %d\n", torusDrawCalls);
        assert(torusDrawCalls == 1 &&
               "10 identical smooth shapes must batch into 1 draw call via shared tessellation");
    }

    setCurrentRenderer(nullptr);
    renderer.shutdown();
    std::printf("webgpu_object_test: ALL OK\n");

    // _Exit, not return: bringing up a wgpu-native device inside a process that
    // also carries this app's global objects aborts during STATIC DESTRUCTION,
    // after main. Verified as unrelated to anything here — it reproduces with no
    // Objects created at all and with the texture cache disabled, while
    // smoke_renderer (same GPU bring-up, none of the app's globals) exits 0, and
    // the real earthcall_webgpu shuts down cleanly. This skips those destructors
    // so the test reports its own result rather than that fault. Every assertion
    // above has already run, so a genuine regression still aborts loudly.
    // Tracked in docs/rendering/WEBGPU_HANDOFF.md — do NOT treat this as fixed.
    std::fflush(stdout);
    std::_Exit(0);
}
