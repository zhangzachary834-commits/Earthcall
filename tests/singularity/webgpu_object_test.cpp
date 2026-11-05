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
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
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
        // Admission prepares inside/outside modules from one retained evaluator.
        // Warm and numeric-edit checks below must still compile zero modules.
        assert(lowExtinctionStats.volumeProgramCompiles == 2 &&
               "first authored sigma_t structure did not prepare both camera variants");

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
        unsigned char sharedOrderAB[4];
        readCentre(sharedOrderAB);
        const Renderer::FrameStats sharedDensitySecondStats = renderer.frameStats();
        assert(sharedDensitySecondStats.volumeProgramCompiles == 0 &&
               sharedDensitySecondStats.volumeProgramCacheHits >= 2 &&
               "media sharing D but differing in sigma_t thrashed the volume program cache");

        // V5 NATIVE PERMUTATION WITNESS: these two bounded media overlap the
        // centre ray. Reversing the projected world-set order must not change
        // the integrated framebuffer answer. The old V0-V4 sequential
        // whole-medium blending path made ordering an accidental transport law.
        renderer.setVolumeDensitySources({sharedDensityB, sharedDensityA}, 5604);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char sharedOrderBA[4];
        readCentre(sharedOrderBA);
        const Renderer::FrameStats sharedDensityReverseStats = renderer.frameStats();
        const auto channelNear = [](unsigned char a, unsigned char b) {
            return std::abs(static_cast<int>(a) - static_cast<int>(b)) <= 1;
        };
        std::printf(
            "V5 overlap order AB=(%d,%d,%d) BA=(%d,%d,%d) compiles=%u\n",
            sharedOrderAB[0], sharedOrderAB[1], sharedOrderAB[2],
            sharedOrderBA[0], sharedOrderBA[1], sharedOrderBA[2],
            sharedDensityReverseStats.volumeProgramCompiles);
        assert(channelNear(sharedOrderAB[0], sharedOrderBA[0]) &&
               channelNear(sharedOrderAB[1], sharedOrderBA[1]) &&
               channelNear(sharedOrderAB[2], sharedOrderBA[2]) &&
               "V5 overlapping medium transport changed when world-set order reversed");
        assert(sharedDensityReverseStats.volumeProgramCompiles >= 1 &&
               "V5 reversed set order did not establish its fused structural program");

        // V5 LOCAL-QUALITY WITNESS: adding a distant, disjoint medium that
        // contributes exactly zero transport must not change the already-existing
        // local answer of medium A. A carries spatially varying self-emission so
        // undersampling is visible; B exists only to enlarge the fused union span.
        auto localDensityNode = scalarNode(1.0);
        OntoMath::Piecewise localDensity =
            OntoMath::Piecewise::continuous(localDensityNode);
        auto localExtinctionNode = scalarNode(0.05);
        OntoMath::Piecewise localExtinction =
            OntoMath::Piecewise::continuous(localExtinctionNode);
        auto localZeroScatteringNode = scalarNode(0.0);
        OntoMath::Piecewise localZeroScattering =
            OntoMath::Piecewise::continuous(localZeroScatteringNode);

        auto localEmissionX = std::make_unique<OntoMath::MathNode>();
        localEmissionX->op = OntoMath::MathNode::Op::ScalarLeaf;
        localEmissionX->scalarForm =
            OntoMath::ScalarForm::transcendental(
                OntoMath::TransFactor::Kind::Exp, "z",
                /*scale=*/3.0, /*shift=*/0.0, /*coefficient=*/0.05);
        auto localEmissionNode = std::make_shared<OntoMath::MathNode>();
        localEmissionNode->op = OntoMath::MathNode::Op::VectorConstruct;
        localEmissionNode->children.push_back(std::move(localEmissionX));
        localEmissionNode->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
        localEmissionNode->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
        OntoMath::Piecewise localEmission =
            OntoMath::Piecewise::continuous(localEmissionNode);

        Rendering::VolumeDensityBinding localA = medium;
        localA.origin = glm::vec3(0.0f);
        localA.scale = glm::vec3(1.0f);
        localA.densityExpr = &localDensity;
        localA.densityRevision = 5651;
        localA.extinctionExpr = &localExtinction;
        localA.extinctionRevision = 5652;
        localA.scatteringExpr = &localZeroScattering;
        localA.scatteringRevision = 5653;
        localA.volumeChromaExpr = nullptr;
        localA.volumeChromaRevision = 0;
        localA.phaseExpr = nullptr;
        localA.phaseRevision = 0;
        localA.emissionExpr = &localEmission;
        localA.emissionRevision = 5654;

        renderer.setVolumeDensitySources({localA}, 5655);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char localAAlone[4];
        readCentre(localAAlone);

        auto zeroDensityNode = scalarNode(0.0);
        OntoMath::Piecewise zeroDensity =
            OntoMath::Piecewise::continuous(zeroDensityNode);
        Rendering::VolumeDensityBinding farZeroB = localA;
        farZeroB.origin = glm::vec3(0.0f, 0.0f, -90.0f);
        farZeroB.densityExpr = &zeroDensity;
        farZeroB.densityRevision = 5656;
        farZeroB.extinctionExpr = nullptr;
        farZeroB.extinctionRevision = 0;
        farZeroB.scatteringExpr = nullptr;
        farZeroB.scatteringRevision = 0;
        farZeroB.emissionExpr = nullptr;
        farZeroB.emissionRevision = 0;

        renderer.setVolumeDensitySources({localA, farZeroB}, 5657);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char localAWithFarZeroB[4];
        readCentre(localAWithFarZeroB);

        const int localQualityDelta =
            std::abs(static_cast<int>(localAAlone[0]) -
                     static_cast<int>(localAWithFarZeroB[0]));
        std::printf(
            "V5 local quality A-alone=(%d,%d,%d) A+far-zero-B=(%d,%d,%d) delta=%d\n",
            localAAlone[0], localAAlone[1], localAAlone[2],
            localAWithFarZeroB[0], localAWithFarZeroB[1], localAWithFarZeroB[2],
            localQualityDelta);
        assert(localAAlone[0] > 20 &&
               "V5 local-quality witness did not produce a measurable A baseline");
        assert(localQualityDelta <= 2 &&
               "V5 distant zero-contribution medium degraded A's local sampling quality");

        // V5 NUMERIC-ONLY FUSED-SET WITNESS: mutate only one numeric
        // coefficient inside member A's already-admitted E_v tree. Membership,
        // expression identities, operator topology, and B remain unchanged.
        // The native answer must change through parameter refresh without
        // regenerating the fused set WGSL/pipeline structure.
        localEmissionNode->children[0]->scalarForm.terms[0].coefficient = 0.0;
        localA.emissionRevision = 5658;
        renderer.setVolumeDensitySources({localA, farZeroB}, 5658);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char localNumericDim[4];
        readCentre(localNumericDim);
        const Renderer::FrameStats localNumericStats = renderer.frameStats();
        std::printf(
            "V5 numeric fused E_v baseline=%d edited=%d compiles=%u\n",
            localAWithFarZeroB[0], localNumericDim[0],
            localNumericStats.volumeProgramCompiles);
        assert(localAWithFarZeroB[0] > localNumericDim[0] + 15 &&
               "V5 numeric member edit did not visibly refresh fused-set parameters");
        assert(localNumericStats.volumeProgramCompiles == 0 &&
               "V5 numeric member edit regenerated fused-set WGSL structure");

        // V5 TIME-ONLY FUSED-SET WITNESS: E_v=(t,0,0) belongs to member A,
        // while a second admitted member keeps this on the fused V5 path.
        // Advancing only A's relative Timeline coordinate must change native
        // pixels while reusing the already-compiled set program. Time remains
        // per-medium runtime instance data, never authored structural identity.
        auto timedSetEmissionT = std::make_unique<OntoMath::MathNode>();
        timedSetEmissionT->op = OntoMath::MathNode::Op::ValueLeaf;
        timedSetEmissionT->variableName = OntoMath::kTimeVar;
        auto timedSetEmissionNode = std::make_shared<OntoMath::MathNode>();
        timedSetEmissionNode->op = OntoMath::MathNode::Op::VectorConstruct;
        timedSetEmissionNode->children.push_back(std::move(timedSetEmissionT));
        timedSetEmissionNode->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
        timedSetEmissionNode->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
        OntoMath::Piecewise timedSetEmission =
            OntoMath::Piecewise::continuous(timedSetEmissionNode);

        Rendering::VolumeDensityBinding timedSetA = localA;
        timedSetA.emissionExpr = &timedSetEmission;
        timedSetA.emissionRevision = 5660;
        timedSetA.temporalCoordinate = 0.05;
        timedSetA.temporalDelta = 0.05;

        renderer.setVolumeDensitySources({timedSetA, farZeroB}, 5661);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char fusedTimeDim[4];
        readCentre(fusedTimeDim);
        const Renderer::FrameStats fusedTimeCompileStats = renderer.frameStats();
        assert(fusedTimeCompileStats.volumeProgramCompiles == 2 &&
               "introducing V5 E_v(t) did not prepare both fused camera variants");

        timedSetA.temporalCoordinate = 1.0;
        timedSetA.temporalDelta = 0.95;
        // Keep membership and authored content revisions fixed: only runtime
        // Timeline data changes between these two frames.
        renderer.setVolumeDensitySources({timedSetA, farZeroB}, 5661);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char fusedTimeBright[4];
        readCentre(fusedTimeBright);
        const Renderer::FrameStats fusedTimeAdvanceStats = renderer.frameStats();
        std::printf(
            "V5 fused timeline t=.05:%d t=1:%d compiles=%u cacheHits=%u\n",
            fusedTimeDim[0], fusedTimeBright[0],
            fusedTimeAdvanceStats.volumeProgramCompiles,
            fusedTimeAdvanceStats.volumeProgramCacheHits);
        assert(fusedTimeBright[0] > fusedTimeDim[0] + 80 &&
               "advancing one fused member Timeline did not visibly change E_v(p,t)");
        assert(fusedTimeAdvanceStats.volumeProgramCompiles == 0 &&
               "advancing one fused member Timeline regenerated set WGSL");
        assert(fusedTimeAdvanceStats.volumeProgramCacheHits >= 2 &&
               "advancing fused member time failed to reuse the admitted set program");

        // V5 MEMBERSHIP INVALIDATION: the already-memoized {A,B} set above
        // is the baseline. Add a third admitted medium C with independent
        // expression identities and a visible blue self-emission contribution.
        // This new membership must compile exactly one new fused structure.
        auto memberCDensityNode = scalarNode(1.0);
        OntoMath::Piecewise memberCDensity =
            OntoMath::Piecewise::continuous(memberCDensityNode);
        auto memberCExtinctionNode = scalarNode(0.05);
        OntoMath::Piecewise memberCExtinction =
            OntoMath::Piecewise::continuous(memberCExtinctionNode);
        auto memberCZeroScatteringNode = scalarNode(0.0);
        OntoMath::Piecewise memberCZeroScattering =
            OntoMath::Piecewise::continuous(memberCZeroScatteringNode);
        auto memberCEmissionNode = vectorNode(0.0, 0.0, 1.0);
        OntoMath::Piecewise memberCEmission =
            OntoMath::Piecewise::continuous(memberCEmissionNode);

        Rendering::VolumeDensityBinding memberC = timedSetA;
        memberC.origin = glm::vec3(0.0f);
        memberC.scale = glm::vec3(1.0f);
        memberC.densityExpr = &memberCDensity;
        memberC.densityRevision = 5670;
        memberC.extinctionExpr = &memberCExtinction;
        memberC.extinctionRevision = 5671;
        memberC.scatteringExpr = &memberCZeroScattering;
        memberC.scatteringRevision = 5672;
        memberC.volumeChromaExpr = nullptr;
        memberC.volumeChromaRevision = 0;
        memberC.phaseExpr = nullptr;
        memberC.phaseRevision = 0;
        memberC.emissionExpr = &memberCEmission;
        memberC.emissionRevision = 5673;
        memberC.temporalCoordinate = 0.0;
        memberC.temporalDelta = 0.0;

        renderer.setVolumeDensitySources({timedSetA, farZeroB, memberC}, 5674);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char membershipABC[4];
        readCentre(membershipABC);
        const Renderer::FrameStats membershipAddStats = renderer.frameStats();
        std::printf(
            "V5 membership AB=(%d,%d,%d) ABC=(%d,%d,%d) addCompiles=%u\n",
            fusedTimeBright[0], fusedTimeBright[1], fusedTimeBright[2],
            membershipABC[0], membershipABC[1], membershipABC[2],
            membershipAddStats.volumeProgramCompiles);
        assert(membershipABC[2] > fusedTimeBright[2] + 30 &&
               "adding visible medium C did not change the fused native answer");
        assert(membershipAddStats.volumeProgramCompiles == 2 &&
               "adding one V5 set member did not prepare both fused camera variants");

        // Remove C and return to the already-established {A,B} structural key.
        // The projected set revision changes, so this first restoration is a
        // value/content refresh, not a historical cache-hit counter event. The
        // structural proof is zero compiles plus restoration of the old pixels.
        renderer.setVolumeDensitySources({timedSetA, farZeroB}, 5675);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char membershipABRestored[4];
        readCentre(membershipABRestored);
        const Renderer::FrameStats membershipRemoveStats = renderer.frameStats();
        assert(channelNear(membershipABRestored[0], fusedTimeBright[0]) &&
               channelNear(membershipABRestored[1], fusedTimeBright[1]) &&
               channelNear(membershipABRestored[2], fusedTimeBright[2]) &&
               "removing C failed to restore the previously memoized {A,B} answer");
        assert(membershipRemoveStats.volumeProgramCompiles == 0 &&
               "removing C recompiled the already-established {A,B} fused structure");

        // An identical follow-up frame on the restored revision gives the
        // telemetry counter its strict historical meaning: unchanged content
        // reuses the admitted two-member memo directly.
        renderer.setVolumeDensitySources({timedSetA, farZeroB}, 5675);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        const Renderer::FrameStats membershipReuseStats = renderer.frameStats();
        assert(membershipReuseStats.volumeProgramCompiles == 0 &&
               membershipReuseStats.volumeProgramCacheHits >= 2 &&
               "restored {A,B} did not directly reuse its fused program memo");

        // V5 REFUSED-MEMBER / NO-STALE-OUTPUT: poison only member B's E_v
        // structure with unsupported Raycast. Even though B's density is zero,
        // the admitted member is invalid authored truth, so the WHOLE current
        // fused answer must fail closed. The previously valid bright {A,B}
        // pipeline may stay cached as an artifact but must not be scheduled.
        auto refusedMemberEmissionNode = std::make_shared<OntoMath::MathNode>();
        refusedMemberEmissionNode->op = OntoMath::MathNode::Op::Raycast;
        OntoMath::Piecewise refusedMemberEmission =
            OntoMath::Piecewise::continuous(refusedMemberEmissionNode);
        Rendering::VolumeDensityBinding refusedB = farZeroB;
        refusedB.emissionExpr = &refusedMemberEmission;
        refusedB.emissionRevision = 5676;

        renderer.setVolumeDensitySources({timedSetA, refusedB}, 5677);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char refusedSetPixel[4];
        readCentre(refusedSetPixel);
        const Renderer::FrameStats refusedSetStats = renderer.frameStats();
        std::printf(
            "V5 refused member pixel=(%d,%d,%d) refusals=%u reason=%s\n",
            refusedSetPixel[0], refusedSetPixel[1], refusedSetPixel[2],
            refusedSetStats.volumeProgramRefusals,
            refusedSetStats.volumeLastProgramRefusal.c_str());
        assert(refusedSetStats.volumeProgramRefusals >= 1 &&
               refusedSetStats.volumeLastProgramRefusal.find("volume set member 1") !=
                   std::string::npos &&
               refusedSetStats.volumeLastProgramRefusal.find("emission") !=
                   std::string::npos &&
               refusedSetStats.volumeLastProgramRefusal.find("Raycast") !=
                   std::string::npos &&
               "unsupported fused member did not produce a named member/emission refusal");
        assert(refusedSetPixel[0] < 12 &&
               refusedSetPixel[1] < 12 &&
               refusedSetPixel[2] < 12 &&
               "refused V5 member replayed stale integrated radiance");

        // Restore lawful B. Returning to the already-memoized {A,B} key must
        // recover the visible answer with no structural compile. A refusal in a
        // different set memo may not poison future lawful reuse.
        renderer.setVolumeDensitySources({timedSetA, farZeroB}, 5678);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char postRefusalRestored[4];
        readCentre(postRefusalRestored);
        const Renderer::FrameStats postRefusalStats = renderer.frameStats();
        assert(channelNear(postRefusalRestored[0], fusedTimeBright[0]) &&
               channelNear(postRefusalRestored[1], fusedTimeBright[1]) &&
               channelNear(postRefusalRestored[2], fusedTimeBright[2]) &&
               "lawful {A,B} did not recover after a refused sibling set");
        assert(postRefusalStats.volumeProgramCompiles == 0 &&
               "restoring lawful {A,B} recompiled a previously memoized fused structure");

        // Restore the original order for the remaining single-medium V1/V2/V3/V4
        // witnesses below; they replace the collection before rendering anyway.
        renderer.setVolumeDensitySources({sharedDensityA, sharedDensityB}, 5603);

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
        assert(extinctionStructureStats.volumeProgramCompiles == 2 &&
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
        assert(timedExtinctionCompileStats.volumeProgramCompiles == 2 &&
               "introducing sigma_t(p,t) did not prepare both camera variants");

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

        // V2 NATIVE PIXELS: the two media below share the EXACT SAME authored
        // D pointer and the EXACT SAME sigma_t pointer. Only medium-owned sigma_s
        // and C_v differ. Their native pixels must therefore diverge without
        // attributing the difference to density, extinction, or source chroma.
        auto v2ExtinctionNode = scalarNode(0.4);
        OntoMath::Piecewise v2Extinction =
            OntoMath::Piecewise::continuous(v2ExtinctionNode);

        auto strongScatteringNode = scalarNode(0.8);
        OntoMath::Piecewise strongScattering =
            OntoMath::Piecewise::continuous(strongScatteringNode);
        auto weakScatteringNode = scalarNode(0.15);
        OntoMath::Piecewise weakScattering =
            OntoMath::Piecewise::continuous(weakScatteringNode);

        auto redMediumChromaNode = vectorNode(1.0, 0.0, 0.0);
        OntoMath::Piecewise redMediumChroma =
            OntoMath::Piecewise::continuous(redMediumChromaNode);
        auto blueMediumChromaNode = vectorNode(0.0, 0.0, 1.0);
        OntoMath::Piecewise blueMediumChroma =
            OntoMath::Piecewise::continuous(blueMediumChromaNode);

        Rendering::VolumeDensityBinding v2RedStrong = medium;
        v2RedStrong.extinctionExpr = &v2Extinction;
        v2RedStrong.extinctionRevision = 5701;
        v2RedStrong.scatteringExpr = &strongScattering;
        v2RedStrong.scatteringRevision = 5702;
        v2RedStrong.volumeChromaExpr = &redMediumChroma;
        v2RedStrong.volumeChromaRevision = 5703;

        Rendering::VolumeDensityBinding v2BlueWeak = medium;
        v2BlueWeak.extinctionExpr = &v2Extinction;
        v2BlueWeak.extinctionRevision = 5701;
        v2BlueWeak.scatteringExpr = &weakScattering;
        v2BlueWeak.scatteringRevision = 5712;
        v2BlueWeak.volumeChromaExpr = &blueMediumChroma;
        v2BlueWeak.volumeChromaRevision = 5713;

        assert(v2RedStrong.densityExpr == v2BlueWeak.densityExpr &&
               v2RedStrong.extinctionExpr == v2BlueWeak.extinctionExpr &&
               "V2 native witness stopped sharing exact D/sigma_t truth");

        renderer.setVolumeDensitySources({v2RedStrong}, 5721);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char v2RedStrongPixel[4];
        readCentre(v2RedStrongPixel);
        const Renderer::FrameStats v2RedStrongStats = renderer.frameStats();

        renderer.setVolumeDensitySources({v2BlueWeak}, 5722);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char v2BlueWeakPixel[4];
        readCentre(v2BlueWeakPixel);
        const Renderer::FrameStats v2BlueWeakStats = renderer.frameStats();

        std::printf(
            "V2 shared-D/sigma_t red-strong=(%d,%d,%d,%d) blue-weak=(%d,%d,%d,%d) "
            "compiles=(%u,%u)\n",
            v2RedStrongPixel[0], v2RedStrongPixel[1], v2RedStrongPixel[2],
            v2RedStrongPixel[3],
            v2BlueWeakPixel[0], v2BlueWeakPixel[1], v2BlueWeakPixel[2],
            v2BlueWeakPixel[3],
            v2RedStrongStats.volumeProgramCompiles,
            v2BlueWeakStats.volumeProgramCompiles);

        assert(v2RedStrongPixel[0] > v2RedStrongPixel[1] + 80 &&
               v2RedStrongPixel[0] > v2RedStrongPixel[2] + 80 &&
               "authored red C_v did not control native medium hue");
        assert(v2BlueWeakPixel[2] > v2BlueWeakPixel[0] + 20 &&
               v2BlueWeakPixel[2] > v2BlueWeakPixel[1] + 20 &&
               "authored blue C_v did not control native medium hue");
        assert(v2RedStrongPixel[0] > v2BlueWeakPixel[2] + 80 &&
               "changing sigma_s with fixed D/sigma_t did not visibly change scattering strength");
        assert(v2RedStrongStats.volumeProgramCompiles == 2 &&
               v2BlueWeakStats.volumeProgramCompiles == 2 &&
               "distinct authored V2 medium structures were not established independently");

        // Return to the first medium. Its (D,sigma_t,sigma_s,C_v) program memo
        // must still exist after compiling the sibling variant.
        renderer.setVolumeDensitySources({v2RedStrong}, 5721);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char v2RedStrongRepeat[4];
        readCentre(v2RedStrongRepeat);
        const Renderer::FrameStats v2RedStrongRepeatStats = renderer.frameStats();
        assert(abs(int(v2RedStrongRepeat[0]) - int(v2RedStrongPixel[0])) <= 2 &&
               abs(int(v2RedStrongRepeat[1]) - int(v2RedStrongPixel[1])) <= 2 &&
               abs(int(v2RedStrongRepeat[2]) - int(v2RedStrongPixel[2])) <= 2 &&
               "V2 medium cache replay changed native pixels");
        assert(v2RedStrongRepeatStats.volumeProgramCompiles == 0 &&
               v2RedStrongRepeatStats.volumeProgramCacheHits >= 1 &&
               "shared-D/sigma_t V2 variants collided in the volume program cache");

        // V3 NATIVE PHASE: keep D / sigma_t / sigma_s / C_v byte-for-byte
        // identical and vary only Phi. A single admitted source supplies wi;
        // wo remains sample->eye. This is angular medium truth, not source alpha.
        auto phaseExtinctionNode = scalarNode(0.8);
        OntoMath::Piecewise phaseExtinction =
            OntoMath::Piecewise::continuous(phaseExtinctionNode);
        auto phaseScatteringNode = scalarNode(0.2);
        OntoMath::Piecewise phaseScattering =
            OntoMath::Piecewise::continuous(phaseScatteringNode);
        auto phaseChromaNode = vectorNode(1.0, 1.0, 1.0);
        OntoMath::Piecewise phaseChroma =
            OntoMath::Piecewise::continuous(phaseChromaNode);

        auto forwardNode =
            OntoMath::MathNode::fromLegacyExpression(
                OntoMath::ScalarForm::constant(1.0).plus(
                    OntoMath::ScalarForm::variable(
                        OntoMath::kWiZVar, 1.0, 0.8)));
        auto reverseNode =
            OntoMath::MathNode::fromLegacyExpression(
                OntoMath::ScalarForm::constant(1.0).plus(
                    OntoMath::ScalarForm::variable(
                        OntoMath::kWiZVar, 1.0, -0.8)));
        OntoMath::Piecewise forwardPhase =
            OntoMath::Piecewise::continuous(forwardNode);
        OntoMath::Piecewise reversePhase =
            OntoMath::Piecewise::continuous(reverseNode);

        Rendering::VolumeDensityBinding phaseForwardMedium = medium;
        phaseForwardMedium.extinctionExpr = &phaseExtinction;
        phaseForwardMedium.extinctionRevision = 5801;
        phaseForwardMedium.scatteringExpr = &phaseScattering;
        phaseForwardMedium.scatteringRevision = 5802;
        phaseForwardMedium.volumeChromaExpr = &phaseChroma;
        phaseForwardMedium.volumeChromaRevision = 5803;
        phaseForwardMedium.phaseExpr = &forwardPhase;
        phaseForwardMedium.phaseRevision = 5804;

        Rendering::VolumeDensityBinding phaseReverseMedium = phaseForwardMedium;
        phaseReverseMedium.phaseExpr = &reversePhase;
        phaseReverseMedium.phaseRevision = 5814;

        assert(phaseForwardMedium.densityExpr == phaseReverseMedium.densityExpr &&
               phaseForwardMedium.extinctionExpr == phaseReverseMedium.extinctionExpr &&
               phaseForwardMedium.scatteringExpr == phaseReverseMedium.scatteringExpr &&
               phaseForwardMedium.volumeChromaExpr == phaseReverseMedium.volumeChromaExpr &&
               "V3 witness stopped holding D/sigma_t/sigma_s/C_v fixed");

        Rendering::RadianceSourceBinding phaseSource;
        phaseSource.position = glm::vec3(0.0f, 0.0f, -3.0f);
        phaseSource.enabled = true;
        renderer.setRadianceSources({phaseSource}, 5820);

        renderer.setVolumeDensitySources({phaseForwardMedium}, 5821);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char phaseForwardPixel[4];
        readCentre(phaseForwardPixel);
        const Renderer::FrameStats phaseForwardStats = renderer.frameStats();

        renderer.setVolumeDensitySources({phaseReverseMedium}, 5822);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char phaseReversePixel[4];
        readCentre(phaseReversePixel);
        const Renderer::FrameStats phaseReverseStats = renderer.frameStats();

        std::printf(
            "V3 same-medium phase forward=%d reverse=%d compiles=(%u,%u)\n",
            phaseForwardPixel[0], phaseReversePixel[0],
            phaseForwardStats.volumeProgramCompiles,
            phaseReverseStats.volumeProgramCompiles);
        assert(phaseForwardPixel[0] > phaseReversePixel[0] + 45 &&
               "changing only Phi with fixed D/sigma_t/sigma_s/C_v did not change native scattering");
        assert(phaseForwardStats.volumeProgramCompiles == 2 &&
               phaseReverseStats.volumeProgramCompiles == 2 &&
               "distinct authored phase structures were not compiled independently");

        // Directionality itself is runtime input: keep the same Phi AST and move
        // only the admitted source across the medium. wi.z flips sign; no shader
        // structure is allowed to change.
        renderer.setVolumeDensitySources({phaseForwardMedium}, 5821);
        renderer.setRadianceSources({phaseSource}, 5820);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char sourceBehindPixel[4];
        readCentre(sourceBehindPixel);

        phaseSource.position = glm::vec3(0.0f, 0.0f, 3.0f);
        renderer.setRadianceSources({phaseSource}, 5820);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char sourceFrontPixel[4];
        readCentre(sourceFrontPixel);
        const Renderer::FrameStats sourceFlipStats = renderer.frameStats();
        assert(sourceBehindPixel[0] > sourceFrontPixel[0] + 45 &&
               "Phi(wi) did not respond visibly when the admitted incident direction flipped");
        assert(sourceFlipStats.volumeProgramCompiles == 0 &&
               sourceFlipStats.volumeProgramCacheHits >= 1 &&
               "incident-direction value change regenerated phase WGSL");

        // VALUE ONLY: a scalar Phi coefficient changes while the AST pointer and
        // structure remain fixed.
        auto numericPhaseNode = scalarNode(0.25);
        OntoMath::Piecewise numericPhase =
            OntoMath::Piecewise::continuous(numericPhaseNode);
        Rendering::VolumeDensityBinding numericPhaseMedium = phaseForwardMedium;
        numericPhaseMedium.phaseExpr = &numericPhase;
        numericPhaseMedium.phaseRevision = 5831;
        renderer.setVolumeDensitySources({numericPhaseMedium}, 5832);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char numericPhaseDim[4];
        readCentre(numericPhaseDim);

        numericPhaseNode->scalarForm.terms[0].coefficient = 1.1;
        numericPhaseMedium.phaseRevision = 5833;
        renderer.setVolumeDensitySources({numericPhaseMedium}, 5834);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char numericPhaseBright[4];
        readCentre(numericPhaseBright);
        const Renderer::FrameStats numericPhaseStats = renderer.frameStats();
        assert(numericPhaseBright[0] > numericPhaseDim[0] + 35 &&
               "numeric Phi edit did not visibly refresh native transport");
        assert(numericPhaseStats.volumeProgramCompiles == 0 &&
               "numeric Phi edit regenerated WGSL instead of refreshing parameters");

        // STRUCTURE: preserve the numeric value 1.1 but replace the leaf by
        // Add(0.55,0.55). The phase program must recompile exactly once.
        auto phaseAdd = std::make_shared<OntoMath::MathNode>();
        phaseAdd->op = OntoMath::MathNode::Op::Add;
        phaseAdd->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.55)));
        phaseAdd->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.55)));
        numericPhase.pieces[0].mathNode = phaseAdd;
        numericPhaseMedium.phaseRevision = 5835;
        renderer.setVolumeDensitySources({numericPhaseMedium}, 5836);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        const Renderer::FrameStats phaseStructureStats = renderer.frameStats();
        assert(phaseStructureStats.volumeProgramCompiles == 2 &&
               "structural Phi edit did not regenerate both camera variants");

        // TIME: Phi=t changes only the admitted medium Timeline coordinate.
        auto phaseTimeNode = std::make_shared<OntoMath::MathNode>();
        phaseTimeNode->op = OntoMath::MathNode::Op::ValueLeaf;
        phaseTimeNode->variableName = OntoMath::kTimeVar;
        OntoMath::Piecewise timedPhase =
            OntoMath::Piecewise::continuous(phaseTimeNode);
        Rendering::VolumeDensityBinding timedPhaseMedium = phaseForwardMedium;
        timedPhaseMedium.phaseExpr = &timedPhase;
        timedPhaseMedium.phaseRevision = 5841;
        timedPhaseMedium.temporalCoordinate = 0.2;
        timedPhaseMedium.temporalDelta = 0.2;
        renderer.setVolumeDensitySources({timedPhaseMedium}, 5842);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char timedPhaseDim[4];
        readCentre(timedPhaseDim);

        timedPhaseMedium.temporalCoordinate = 1.2;
        timedPhaseMedium.temporalDelta = 1.0;
        renderer.setVolumeDensitySources({timedPhaseMedium}, 5842);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char timedPhaseBright[4];
        readCentre(timedPhaseBright);
        const Renderer::FrameStats timedPhaseStats = renderer.frameStats();
        assert(timedPhaseBright[0] > timedPhaseDim[0] + 30 &&
               "advancing the medium Timeline did not visibly drive Phi(p,wi,wo,t)");
        assert(timedPhaseStats.volumeProgramCompiles == 0 &&
               timedPhaseStats.volumeProgramCacheHits >= 1 &&
               "phase Timeline advance regenerated structural WGSL");

        // REFUSAL: unsupported authored phase must suppress this medium, never
        // reuse the previously valid timed phase program.
        auto unsupportedPhaseNode = std::make_shared<OntoMath::MathNode>();
        unsupportedPhaseNode->op = OntoMath::MathNode::Op::Raycast;
        timedPhase.pieces[0].mathNode = unsupportedPhaseNode;
        timedPhaseMedium.phaseRevision = 5843;
        renderer.setVolumeDensitySources({timedPhaseMedium}, 5844);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char refusedPhasePixel[4];
        readCentre(refusedPhasePixel);
        const Renderer::FrameStats refusedPhaseStats = renderer.frameStats();
        assert(refusedPhaseStats.volumeProgramRefusals >= 1 &&
               refusedPhaseStats.volumeLastProgramRefusal.find("phase") != std::string::npos &&
               refusedPhaseStats.volumeLastProgramRefusal.find("Raycast") != std::string::npos &&
               "unsupported Phi did not produce a named phase refusal");
        assert(refusedPhasePixel[0] < 12 &&
               refusedPhasePixel[1] < 12 &&
               refusedPhasePixel[2] < 12 &&
               "refused Phi left stale volumetric output on screen");

        renderer.setRadianceSources({}, 0);

        // V4 NATIVE SELF-EMISSION: remove every admitted radiance source and
        // force sigma_s=0 so ordinary scattering cannot fake the result. The
        // control and emissive media share exact D/sigma_t/sigma_s/C_v truth;
        // only E_v differs.
        auto v4ExtinctionNode = scalarNode(0.6);
        OntoMath::Piecewise v4Extinction =
            OntoMath::Piecewise::continuous(v4ExtinctionNode);
        auto v4ZeroScatteringNode = scalarNode(0.0);
        OntoMath::Piecewise v4ZeroScattering =
            OntoMath::Piecewise::continuous(v4ZeroScatteringNode);
        auto v4ChromaNode = vectorNode(1.0, 1.0, 1.0);
        OntoMath::Piecewise v4Chroma =
            OntoMath::Piecewise::continuous(v4ChromaNode);
        auto v4EmissionNode = vectorNode(1.2, 0.15, 0.05);
        OntoMath::Piecewise v4Emission =
            OntoMath::Piecewise::continuous(v4EmissionNode);

        Rendering::VolumeDensityBinding v4Dark = medium;
        v4Dark.extinctionExpr = &v4Extinction;
        v4Dark.extinctionRevision = 5901;
        v4Dark.scatteringExpr = &v4ZeroScattering;
        v4Dark.scatteringRevision = 5902;
        v4Dark.volumeChromaExpr = &v4Chroma;
        v4Dark.volumeChromaRevision = 5903;
        v4Dark.phaseExpr = nullptr;
        v4Dark.phaseRevision = 0;
        v4Dark.emissionExpr = nullptr;
        v4Dark.emissionRevision = 0;

        Rendering::VolumeDensityBinding v4Emissive = v4Dark;
        v4Emissive.emissionExpr = &v4Emission;
        v4Emissive.emissionRevision = 5904;

        assert(v4Dark.densityExpr == v4Emissive.densityExpr &&
               v4Dark.extinctionExpr == v4Emissive.extinctionExpr &&
               v4Dark.scatteringExpr == v4Emissive.scatteringExpr &&
               v4Dark.volumeChromaExpr == v4Emissive.volumeChromaExpr &&
               v4Dark.phaseExpr == v4Emissive.phaseExpr &&
               "V4 self-emission witness stopped holding D/sigma_t/sigma_s/C_v/Phi fixed");

        renderer.setVolumeDensitySources({v4Dark}, 5910);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char v4DarkPixel[4];
        readCentre(v4DarkPixel);

        renderer.setVolumeDensitySources({v4Emissive}, 5911);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char v4EmissivePixel[4];
        readCentre(v4EmissivePixel);
        const Renderer::FrameStats v4EmissiveStats = renderer.frameStats();

        std::printf(
            "V4 no-source self-emission dark=(%d,%d,%d,%d) emissive=(%d,%d,%d,%d) compiles=%u\n",
            v4DarkPixel[0], v4DarkPixel[1], v4DarkPixel[2], v4DarkPixel[3],
            v4EmissivePixel[0], v4EmissivePixel[1], v4EmissivePixel[2], v4EmissivePixel[3],
            v4EmissiveStats.volumeProgramCompiles);

        assert(v4DarkPixel[0] < 12 && v4DarkPixel[1] < 12 && v4DarkPixel[2] < 12 &&
               "zero-source sigma_s=0 control medium produced unexpected radiance");
        assert(v4EmissivePixel[0] > v4DarkPixel[0] + 50 &&
               v4EmissivePixel[0] > v4EmissivePixel[1] + 35 &&
               v4EmissivePixel[0] > v4EmissivePixel[2] + 35 &&
               "authored E_v failed to produce visible native radiance without external illumination");
        assert(v4EmissiveStats.volumeProgramCompiles == 2 &&
               "introducing independent E_v did not establish its own volume program");

        // VALUE ONLY on the same E_v AST: dim red emission in-place. This must
        // visibly change pixels while reusing the compiled structure.
        v4EmissionNode->children[0]->scalarForm.terms[0].coefficient = 0.2;
        v4Emissive.emissionRevision = 5905;
        renderer.setVolumeDensitySources({v4Emissive}, 5912);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char v4DimmedPixel[4];
        readCentre(v4DimmedPixel);
        const Renderer::FrameStats v4DimmedStats = renderer.frameStats();
        assert(v4EmissivePixel[0] > v4DimmedPixel[0] + 35 &&
               "numeric E_v edit did not visibly refresh native self-emission");
        // A numeric authored edit changes contentRevision, so it deliberately
        // takes the parameter-refresh branch rather than the unchanged-content
        // cache-hit branch. The visible pixel delta above proves refreshed
        // parameters reached native transport; zero compiles proves structure
        // and WGSL were reused.
        assert(v4DimmedStats.volumeProgramCompiles == 0 &&
               "numeric E_v edit regenerated WGSL instead of refreshing parameters");

        // REFUSAL: an unsupported E_v must suppress the medium contribution and
        // may not replay the previously valid emissive program.
        auto unsupportedEmissionNode = std::make_shared<OntoMath::MathNode>();
        unsupportedEmissionNode->op = OntoMath::MathNode::Op::Raycast;
        v4Emission.pieces[0].mathNode = unsupportedEmissionNode;
        v4Emissive.emissionRevision = 5906;
        renderer.setVolumeDensitySources({v4Emissive}, 5913);
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char v4RefusedPixel[4];
        readCentre(v4RefusedPixel);
        const Renderer::FrameStats v4RefusedStats = renderer.frameStats();
        assert(v4RefusedStats.volumeProgramRefusals >= 1 &&
               v4RefusedStats.volumeLastProgramRefusal.find("emission") != std::string::npos &&
               v4RefusedStats.volumeLastProgramRefusal.find("Raycast") != std::string::npos &&
               "unsupported E_v did not produce a named emission refusal");
        assert(v4RefusedPixel[0] < 12 &&
               v4RefusedPixel[1] < 12 &&
               v4RefusedPixel[2] < 12 &&
               "refused E_v left stale self-emitted radiance on screen");

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
        assert(timedDensityCompileStats.volumeProgramCompiles == 2 &&
               "introducing D(p,t) did not prepare both camera variants");

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

    // --- LEGACY ENVIRONMENT / DIRECT-TRANSPORT SEPARATION ------------------
    // Compatibility environment illumination may reuse authored chi(p,t), but
    // it is not source rho and it is not visibility. A zero-rho source with a
    // blocker on its geometric path must therefore keep the same environment
    // pixel when visibility is enabled.
    {
        auto environmentMat = materials.create("webgpu_environment_separation");
        environmentMat->baseColor = glm::vec3(1.0f);
        environmentMat->ambient = 0.2f;
        environmentMat->diffuse = 1.0f;
        environmentMat->specular = 0.0f;

        Object environmentReceiver;
        auto environmentSphere =
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.55f));
        environmentReceiver.setFieldShape(environmentSphere, glm::vec3(1.5f));
        environmentReceiver.setMaterialId("material.webgpu_environment_separation");

        auto zeroRhoNode = scalarNode(0.0);
        OntoMath::Piecewise zeroRho =
            OntoMath::Piecewise::continuous(zeroRhoNode);
        auto redEnvironmentNode = vectorNode(1.0, 0.0, 0.0);
        OntoMath::Piecewise redEnvironmentChroma =
            OntoMath::Piecewise::continuous(redEnvironmentNode);

        const glm::vec3 environmentSourcePos(-0.8f, 0.0f, 1.6f);
        renderer.setRadianceSources({}, 0);
        renderer.setRadianceField(&zeroRho, 4301);
        renderer.setRadianceChroma(&redEnvironmentChroma, 4302);
        renderer.setRadianceAngular(nullptr, 0);
        renderer.setRadianceSourceCoefficients(1.0f, 0.2f, 0.8f, 0.0f);
        renderer.setLight(environmentSourcePos,
                          glm::vec3(0.2f),
                          glm::vec3(0.8f),
                          glm::vec3(0.0f));
        renderer.setLightingEnabled(true);
        renderer.setRadianceVisibilityEnabled(false);
        renderer.setModel(glm::mat4(1.0f));

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        environmentReceiver.drawObject();
        renderer.endFrame();
        unsigned char environmentBaseline[4];
        readCentre(environmentBaseline);
        assert(environmentBaseline[0] > 35 &&
               environmentBaseline[1] < 12 &&
               environmentBaseline[2] < 12 &&
               "zero-rho source lost its legacy environment illumination");

        auto environmentBlocker =
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.16f));
        environmentBlocker.offset = glm::vec3(-0.4f, 0.0f, 1.075f);
        environmentReceiver.setFieldShape(
            geom::SdfNode::binary(
                geom::SdfOp::Union, environmentSphere, environmentBlocker),
            glm::vec3(1.5f));
        renderer.setRadianceVisibilityEnabled(true);

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        environmentReceiver.drawObject();
        renderer.endFrame();
        unsigned char environmentBlocked[4];
        readCentre(environmentBlocked);
        std::printf("environment separation zero-rho baseline=(%d,%d,%d) blocked=(%d,%d,%d)\n",
                    environmentBaseline[0], environmentBaseline[1], environmentBaseline[2],
                    environmentBlocked[0], environmentBlocked[1], environmentBlocked[2]);
        assert(abs(int(environmentBaseline[0]) - int(environmentBlocked[0])) <= 2 &&
               abs(int(environmentBaseline[1]) - int(environmentBlocked[1])) <= 2 &&
               abs(int(environmentBaseline[2]) - int(environmentBlocked[2])) <= 2 &&
               "visibility or blocker geometry leaked into environment compatibility illumination");

        renderer.setRadianceVisibilityEnabled(false);
        renderer.setRadianceField(nullptr, 0);
        renderer.setRadianceChroma(nullptr, 0);
        renderer.setRadianceSourceCoefficients(1.0f, 0.2f, 0.8f, 1.0f);
    }

    // --- LEGACY DIRECT RESPONSE MIGRATION PIXEL PARITY ----------------------
    // The old source API may still carry independent diffuse/specular weights,
    // but SDF execution must receive one honest incident RGB plus compatibility
    // lobe scalars. The inherited material.default response reproduces the old
    // Blinn-Phong answer; no renderer-owned lobe formula participates.
    {
        auto legacyDirectMat = materials.create("webgpu_legacy_direct_migration");
        legacyDirectMat->baseColor = glm::vec3(0.5f, 0.25f, 1.0f);
        legacyDirectMat->ambient = 0.2f;
        legacyDirectMat->diffuse = 0.8f;
        legacyDirectMat->specular = 1.0f;
        legacyDirectMat->shininess = 32.0f;
        legacyDirectMat->responseExpr.reset();
        legacyDirectMat->environmentResponseExpr.reset();

        Object legacyDirectReceiver;
        legacyDirectReceiver.setFieldShape(
            geom::SdfNode::leaf(
                geom::SdfPrim::Box, glm::vec3(0.55f, 0.55f, 0.55f)),
            glm::vec3(1.0f));
        legacyDirectReceiver.setMaterialId(
            "material.webgpu_legacy_direct_migration");

        auto legacyRhoNode = scalarNode(1.0);
        OntoMath::Piecewise legacyRho =
            OntoMath::Piecewise::continuous(legacyRhoNode);

        renderer.setRadianceSources({}, 0);
        renderer.setRadianceField(&legacyRho, 4401);
        renderer.setRadianceChroma(nullptr, 0);
        renderer.setRadianceAngular(nullptr, 0);
        renderer.setRadianceVisibilityEnabled(false);
        // Deliberately adversarial old API: diffuse and specular use
        // DIFFERENT RGB chroma. Old no-chi SDF semantics consumed these
        // vectors independently; the authored default must preserve that fact.
        renderer.setRadianceSourceCoefficients(1.0f, 0.0f, 0.2f, 0.4f);
        renderer.setLight(glm::vec3(0.0f, 0.0f, 2.0f),
                          glm::vec3(0.0f),
                          glm::vec3(0.20f, 0.08f, 0.04f),
                          glm::vec3(0.40f, 0.20f, 0.05f));
        renderer.setLightingEnabled(true);
        renderer.setModel(glm::mat4(1.0f));

        const RenderMaterial legacyResolved =
            resolveRenderMaterial(
                "material.webgpu_legacy_direct_migration", {});
        assert(legacyResolved.responseExpr &&
               legacyResolved.responseExpr.get() ==
                   materials.defaultMaterial()->responseExpr.get() &&
               "legacy direct receiver did not inherit material.default response");

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        legacyDirectReceiver.drawObject();
        renderer.endFrame();
        unsigned char migratedLegacyPixel[4];
        readCentre(migratedLegacyPixel);
        std::printf(
            "legacy direct migration pixel=(%d,%d,%d) expected~(128,56,23)\n",
            migratedLegacyPixel[0], migratedLegacyPixel[1],
            migratedLegacyPixel[2]);

        // Exact front-center is (.50,.22,.09) -> (128,56,23). The 16x16
        // sample is slightly off-axis, mostly narrowing the shiny lobe.
        assert(migratedLegacyPixel[0] >= 108 &&
               migratedLegacyPixel[0] <= 140 &&
               migratedLegacyPixel[1] >= 42 &&
               migratedLegacyPixel[1] <= 72 &&
               migratedLegacyPixel[2] >= 14 &&
               migratedLegacyPixel[2] <= 38 &&
               migratedLegacyPixel[0] > migratedLegacyPixel[1] + 45 &&
               migratedLegacyPixel[1] > migratedLegacyPixel[2] + 15 &&
               "authored default response failed arbitrary legacy diffuse/specular RGB parity");

        renderer.setRadianceField(nullptr, 0);
        renderer.setRadianceSourceCoefficients(1.0f, 0.2f, 0.8f, 1.0f);
        renderer.setLight(glm::vec3(0.0f, 0.0f, 2.0f),
                          glm::vec3(0.2f),
                          glm::vec3(0.8f),
                          glm::vec3(1.0f));
    }

    // --- RUNG 9 MATERIAL COPY-ON-WRITE CONTINUITY ---------------------------
    // Painting an Object creates its private Material. That operation may change
    // paint ownership; it must neither erase nor alias receiver-response truth.
    {
        auto sharedResponseMat = materials.create("webgpu_rung9_cow_shared");
        assert(sharedResponseMat->setDynamicProperty(
            "roughness", PropertyValue(0.42f)));

        auto sharedResponseRoot = std::make_shared<OntoMath::MathNode>();
        sharedResponseRoot->op = OntoMath::MathNode::Op::VectorConstruct;
        auto roughnessBind = std::make_unique<OntoMath::MathNode>();
        roughnessBind->op = OntoMath::MathNode::Op::Bind;
        roughnessBind->propertyPath = "roughness";
        sharedResponseRoot->children.push_back(std::move(roughnessBind));
        sharedResponseRoot->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.2)));
        sharedResponseRoot->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.1)));
        sharedResponseMat->responseExpr = std::make_shared<OntoMath::Piecewise>(
            OntoMath::Piecewise::continuous(sharedResponseRoot));
        sharedResponseMat->bumpResponseRevision();
        const std::string sharedResponseBefore =
            sharedResponseMat->responseExpr->toJson().dump();

        Object paintedReceiver;
        paintedReceiver.setShapeKind(Object::ShapeKind::Cube);
        paintedReceiver.setMaterialId("material.webgpu_rung9_cow_shared");
        paintedReceiver.setFaceColor(0, 0.4f, 0.5f, 0.6f);

        auto privateResponseMat = materials.get(paintedReceiver.materialId());
        assert(privateResponseMat &&
               privateResponseMat.get() != sharedResponseMat.get() &&
               "painting did not create the Object's private Material");
        assert(privateResponseMat->responseExpr &&
               privateResponseMat->responseExpr->toJson().dump() ==
                   sharedResponseBefore &&
               "Material copy-on-write erased or changed authored receiver response");
        assert(privateResponseMat->responseExpr.get() !=
                   sharedResponseMat->responseExpr.get() &&
               "Material copy-on-write aliased the response Piecewise object");

        PropertyValue privateRoughness;
        PropertyValue sharedRoughness;
        assert(privateResponseMat->getDynamicProperty(
                   "roughness", privateRoughness) &&
               sharedResponseMat->getDynamicProperty(
                   "roughness", sharedRoughness) &&
               "Material copy-on-write dropped an authored response Bind input");
        double privateRoughNumber = 0.0;
        double sharedRoughNumber = 0.0;
        assert(propertyValueToNumber(privateRoughness, privateRoughNumber) &&
               propertyValueToNumber(sharedRoughness, sharedRoughNumber) &&
               std::abs(privateRoughNumber - 0.42) < 1e-6 &&
               std::abs(sharedRoughNumber - 0.42) < 1e-6);

        assert(PropertyPath::parse("roughness").setValue(
                   *privateResponseMat, PropertyValue(0.15f)) ==
               PropertyPath::PathResult::Ok);
        assert(privateResponseMat->getDynamicProperty(
                   "roughness", privateRoughness) &&
               sharedResponseMat->getDynamicProperty(
                   "roughness", sharedRoughness));
        assert(propertyValueToNumber(privateRoughness, privateRoughNumber) &&
               propertyValueToNumber(sharedRoughness, sharedRoughNumber) &&
               std::abs(privateRoughNumber - 0.15) < 1e-6 &&
               std::abs(sharedRoughNumber - 0.42) < 1e-6 &&
               "editing the private scalar Bind input mutated the shared Material");

        privateResponseMat->responseExpr->pieces[0].mathNode =
            vectorNode(0.1, 0.2, 0.9);
        privateResponseMat->bumpResponseRevision();
        assert(sharedResponseMat->responseExpr->toJson().dump() ==
                   sharedResponseBefore &&
               "editing private painted Material mutated the shared response AST");
    }

    // --- RUNG 9 AUTHORED RECEIVER RESPONSE ---------------------------------
    // Same receiver geometry, source, visibility state and white albedo. Only the
    // Material-owned response expression changes. This is the native witness that
    // f_r is receiver truth rather than another source/visibility coefficient.
    {
        auto responseMat = materials.create("webgpu_rung9_response_witness");
        responseMat->baseColor = glm::vec3(1.0f);
        // Deliberately hostile compatibility values: an authored response must
        // not secretly inherit these legacy Blinn-Phong knobs.
        responseMat->ambient = 0.73f;
        responseMat->diffuse = 0.11f;
        responseMat->specular = 0.91f;
        responseMat->shininess = 3.0f;

        auto responseRoot = vectorNode(1.0, 0.03, 0.03);
        responseMat->responseExpr = std::make_shared<OntoMath::Piecewise>(
            OntoMath::Piecewise::continuous(responseRoot));
        responseMat->bumpResponseRevision();

        Object receiver;
        receiver.setFieldShape(
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.55f)),
            glm::vec3(1.0f));
        receiver.setMaterialId("material.webgpu_rung9_response_witness");

        auto responseRhoNode = scalarNode(1.0);
        OntoMath::Piecewise responseRho =
            OntoMath::Piecewise::continuous(responseRhoNode);

        renderer.setRadianceSources({}, 0);
        renderer.setRadianceChroma(nullptr, 0);
        renderer.setRadianceAngular(nullptr, 0);
        renderer.setRadianceVisibilityEnabled(false);
        renderer.setRadianceField(&responseRho, 5001);
        renderer.setLight(glm::vec3(0.0f, 0.0f, 2.0f),
                          glm::vec3(0.0f),
                          glm::vec3(0.8f),
                          glm::vec3(0.0f));
        renderer.setLightingEnabled(true);
        renderer.setModel(glm::mat4(1.0f));

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        receiver.drawObject();
        renderer.endFrame();
        unsigned char responseRed[4];
        readCentre(responseRed);
        const Renderer::FrameStats responseFirstStats = renderer.frameStats();
        std::printf("Rung-9 response red=(%d,%d,%d) compiles=%u\n",
                    responseRed[0], responseRed[1], responseRed[2],
                    responseFirstStats.sdfProgramCompiles);
        assert(responseRed[0] > responseRed[1] + 70 &&
               responseRed[0] > responseRed[2] + 70 &&
               "authored red receiver response did not control the native surface pixel");
        assert(responseFirstStats.sdfProgramCompiles == 1 &&
               "first authored receiver-response draw must compile its structure once");

        // NUMERIC RESPONSE EDIT ONLY: identical VectorConstruct/ScalarLeaf
        // topology, same source and visibility, but response changes red -> blue.
        responseRoot->children[0]->scalarForm.terms[0].coefficient = 0.03;
        responseRoot->children[2]->scalarForm.terms[0].coefficient = 1.0;
        responseMat->bumpResponseRevision();

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        receiver.drawObject();
        renderer.endFrame();
        unsigned char responseBlue[4];
        readCentre(responseBlue);
        const Renderer::FrameStats responseValueStats = renderer.frameStats();
        std::printf("Rung-9 response blue=(%d,%d,%d) compiles=%u cacheHits=%u paramBytes=%zu\n",
                    responseBlue[0], responseBlue[1], responseBlue[2],
                    responseValueStats.sdfProgramCompiles,
                    responseValueStats.sdfProgramCacheHits,
                    responseValueStats.sdfParameterBytesUploaded);
        assert(responseBlue[2] > responseBlue[0] + 70 &&
               responseBlue[2] > responseBlue[1] + 70 &&
               "numeric receiver-response edit did not visibly change the receiver");
        assert(responseValueStats.sdfProgramCompiles == 0 &&
               responseValueStats.sdfProgramCacheHits >= 1 &&
               responseValueStats.sdfParameterBytesUploaded > 0 &&
               "numeric response edit regenerated WGSL or reused stale parameters");

        // TWO RECEIVERS, ONE WORLD: keep source, visibility, geometry, albedo,
        // camera and legacy compatibility coefficients identical. A distinct
        // Material-owned response expression alone must produce a distinct pixel.
        auto blueSurfaceMat = materials.create("webgpu_rung9_response_blue_surface");
        blueSurfaceMat->baseColor = responseMat->baseColor;
        blueSurfaceMat->ambient = responseMat->ambient;
        blueSurfaceMat->diffuse = responseMat->diffuse;
        blueSurfaceMat->specular = responseMat->specular;
        blueSurfaceMat->shininess = responseMat->shininess;
        blueSurfaceMat->responseExpr = std::make_shared<OntoMath::Piecewise>(
            OntoMath::Piecewise::continuous(vectorNode(0.03, 0.03, 1.0)));
        blueSurfaceMat->bumpResponseRevision();

        Object blueSurfaceReceiver;
        blueSurfaceReceiver.setFieldShape(
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.55f)),
            glm::vec3(1.0f));
        blueSurfaceReceiver.setMaterialId("material.webgpu_rung9_response_blue_surface");

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        blueSurfaceReceiver.drawObject();
        renderer.endFrame();
        unsigned char responseOtherSurfaceBlue[4];
        readCentre(responseOtherSurfaceBlue);
        assert(responseOtherSurfaceBlue[2] > responseOtherSurfaceBlue[0] + 70 &&
               responseOtherSurfaceBlue[2] > responseOtherSurfaceBlue[1] + 70 &&
               "second receiver's authored blue response did not control its pixel");
        assert(responseRed[0] > responseOtherSurfaceBlue[0] + 70 &&
               responseOtherSurfaceBlue[2] > responseRed[2] + 70 &&
               "two otherwise-identical receivers differing only in authored response "
               "did not yield different native pixels");

        // SOURCE VALUE EDIT ONLY: dim rho while preserving the response AST
        // byte-for-byte. Incident light may change the answer; it may not rewrite
        // the receiver's mathematics or force a response structural compile.
        const std::string responseBeforeSourceEdit =
            responseMat->responseExpr->toJson().dump();
        responseRhoNode->scalarForm.terms[0].coefficient = 0.25;
        renderer.setRadianceField(&responseRho, 5002);

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        receiver.drawObject();
        renderer.endFrame();
        unsigned char responseDimmedBySource[4];
        readCentre(responseDimmedBySource);
        const Renderer::FrameStats sourceEditStats = renderer.frameStats();
        assert(responseBlue[2] > responseDimmedBySource[2] + 70 &&
               "source rho edit did not change incident illumination on authored response");
        assert(sourceEditStats.sdfProgramCompiles == 0 &&
               sourceEditStats.sdfProgramCacheHits >= 1 &&
               "numeric source edit recompiled receiver-response structure");
        assert(responseMat->responseExpr->toJson().dump() == responseBeforeSourceEdit &&
               "source edit mutated receiver-owned response mathematics");

        // VISIBILITY STATE EDIT ONLY: with no blocker, toggling derived
        // visibility changes V from implicit 1 to computed 1. It must not rewrite
        // receiver-owned response mathematics or regenerate its structure.
        const std::string responseBeforeVisibilityEdit =
            responseMat->responseExpr->toJson().dump();
        renderer.setRadianceVisibilityEnabled(true);

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        receiver.drawObject();
        renderer.endFrame();
        unsigned char responseVisibilityOn[4];
        readCentre(responseVisibilityOn);
        const Renderer::FrameStats responseVisibilityStats = renderer.frameStats();
        assert(abs(int(responseVisibilityOn[0]) - int(responseDimmedBySource[0])) <= 2 &&
               abs(int(responseVisibilityOn[1]) - int(responseDimmedBySource[1])) <= 2 &&
               abs(int(responseVisibilityOn[2]) - int(responseDimmedBySource[2])) <= 2 &&
               "blocker-free visibility changed authored receiver response output");
        assert(responseVisibilityStats.sdfProgramCompiles == 0 &&
               responseVisibilityStats.sdfProgramCacheHits >= 1 &&
               "visibility-state edit recompiled receiver-response structure");
        assert(responseMat->responseExpr->toJson().dump() ==
                   responseBeforeVisibilityEdit &&
               "visibility-state edit mutated receiver-owned response mathematics");
        renderer.setRadianceVisibilityEnabled(false);

        // Restore source value, then alter response TOPOLOGY while retaining its
        // broad blue answer. Structure must recompile exactly because f_r changed.
        responseRhoNode->scalarForm.terms[0].coefficient = 1.0;
        renderer.setRadianceField(&responseRho, 5003);
        auto responseAdd = std::make_unique<OntoMath::MathNode>();
        responseAdd->op = OntoMath::MathNode::Op::Add;
        responseAdd->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.01)));
        responseAdd->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.02)));
        responseRoot->children[0] = std::move(responseAdd);
        responseMat->bumpResponseRevision();

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        receiver.drawObject();
        renderer.endFrame();
        const Renderer::FrameStats responseStructureStats = renderer.frameStats();
        assert(responseStructureStats.sdfProgramCompiles == 1 &&
               "response topology edit did not invalidate the relevant SDF program");

        // REFUSAL: unsupported receiver math must clear to the offscreen
        // background, not reuse the previously compiled blue response.
        auto badResponse = std::make_unique<OntoMath::MathNode>();
        badResponse->op = OntoMath::MathNode::Op::Raycast;
        responseRoot->children[0] = std::move(badResponse);
        responseMat->bumpResponseRevision();

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        receiver.drawObject();
        renderer.endFrame();
        unsigned char responseRefused[4];
        readCentre(responseRefused);
        const Renderer::FrameStats responseRefusalStats = renderer.frameStats();
        assert(responseRefusalStats.sdfProgramRefusals >= 1 &&
               responseRefusalStats.sdfLastProgramRefusal.find("material response") !=
                   std::string::npos &&
               responseRefusalStats.sdfLastProgramRefusal.find("Raycast") !=
                   std::string::npos &&
               "unsupported receiver response did not surface a named refusal");
        assert(responseRefused[0] < 12 && responseRefused[1] < 12 &&
               responseRefused[2] < 12 &&
               "refused receiver response left stale authored output on screen");

        renderer.setRadianceField(nullptr, 0);
        renderer.setRadianceVisibilityEnabled(false);
    }

    // --- DEFAULT DIRECT RESPONSE INHERITANCE / LOCAL BINDINGS ----------------
    // A legacy Material has no local response AST. Resolution must inherit the
    // default authored equation while binding THIS Material's coefficients.
    // Equation inheritance is not value inheritance.
    {
        auto legacyInherited = materials.create("webgpu_default_response_inheritance");
        legacyInherited->responseExpr.reset();
        legacyInherited->environmentResponseExpr.reset();
        legacyInherited->diffuse = 0.31f;
        legacyInherited->specular = 0.27f;
        legacyInherited->shininess = 11.0f;
        legacyInherited->ambient = 0.09f;

        auto defaultMaterial = materials.defaultMaterial();
        assert(defaultMaterial && defaultMaterial->responseExpr &&
               defaultMaterial->environmentResponseExpr);

        const RenderMaterial resolvedInherited =
            resolveRenderMaterial(
                "material.webgpu_default_response_inheritance", {});
        assert(resolvedInherited.responseExpr &&
               resolvedInherited.responseExpr.get() ==
                   defaultMaterial->responseExpr.get() &&
               "legacy Material did not inherit the authored default direct equation");
        assert(resolvedInherited.environmentResponseExpr &&
               resolvedInherited.environmentResponseExpr.get() ==
                   defaultMaterial->environmentResponseExpr.get() &&
               "legacy Material did not inherit the authored default environment equation");

        const auto readBoundNumber = [&](const char* path) {
            const auto it = resolvedInherited.responseBindings.find(path);
            assert(it != resolvedInherited.responseBindings.end());
            double value = 0.0;
            assert(propertyValueToNumber(it->second, value));
            return value;
        };
        assert(std::abs(readBoundNumber("diffuse") - 0.31) < 1e-6 &&
               std::abs(readBoundNumber("specular") - 0.27) < 1e-6 &&
               std::abs(readBoundNumber("shininess") - 11.0) < 1e-6 &&
               std::abs(readBoundNumber("ambient") - 0.09) < 1e-6 &&
               "inherited response equation bound default Material values instead of receiver values");

        auto localRoot = vectorNode(0.2, 0.3, 0.4);
        legacyInherited->responseExpr =
            std::make_shared<OntoMath::Piecewise>(
                OntoMath::Piecewise::continuous(localRoot));
        legacyInherited->bumpResponseRevision();
        const RenderMaterial resolvedLocal =
            resolveRenderMaterial(
                "material.webgpu_default_response_inheritance", {});
        assert(resolvedLocal.responseExpr.get() ==
                   legacyInherited->responseExpr.get() &&
               resolvedLocal.responseExpr.get() !=
                   defaultMaterial->responseExpr.get() &&
               "explicit local response did not remain sovereign over the default");
        assert(!resolvedLocal.environmentResponseExpr &&
               "explicit direct response secretly inherited legacy environment response");
    }

    // --- AD-21 MATERIAL RESPONSE PROPERTY BIND -------------------------------
    // The response AST stays byte-for-byte fixed while an ordinary Material
    // Property changes through PropertyPath. resolveRenderMaterial must refresh
    // the bound parameter value without a response revision bump or WGSL compile.
    {
        auto boundMat = materials.create("webgpu_response_bind_witness");
        boundMat->baseColor = glm::vec3(1.0f);
        boundMat->ambient = 0.0f;
        boundMat->diffuse = 1.0f;
        boundMat->specular = 0.0f;

        auto boundRoot = std::make_shared<OntoMath::MathNode>();
        boundRoot->op = OntoMath::MathNode::Op::VectorConstruct;
        auto boundDiffuse = std::make_unique<OntoMath::MathNode>();
        boundDiffuse->op = OntoMath::MathNode::Op::Bind;
        boundDiffuse->propertyPath = "diffuse";
        boundRoot->children.push_back(std::move(boundDiffuse));
        boundRoot->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
        boundRoot->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
        boundMat->responseExpr = std::make_shared<OntoMath::Piecewise>(
            OntoMath::Piecewise::continuous(boundRoot));
        boundMat->bumpResponseRevision();
        const uint32_t boundResponseRevision = boundMat->getResponseRevision();
        const std::string boundResponseJson = boundMat->responseExpr->toJson().dump();

        Object boundReceiver;
        boundReceiver.setFieldShape(
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.55f)),
            glm::vec3(1.0f));
        boundReceiver.setMaterialId("material.webgpu_response_bind_witness");

        auto boundRhoRoot = scalarNode(1.0);
        OntoMath::Piecewise boundRho =
            OntoMath::Piecewise::continuous(boundRhoRoot);
        renderer.setRadianceSources({}, 0);
        renderer.setRadianceChroma(nullptr, 0);
        renderer.setRadianceAngular(nullptr, 0);
        renderer.setRadianceVisibilityEnabled(false);
        renderer.setRadianceField(&boundRho, 5101);
        renderer.setLight(glm::vec3(0.0f, 0.0f, 2.0f),
                          glm::vec3(0.0f),
                          glm::vec3(0.8f),
                          glm::vec3(0.0f));
        renderer.setLightingEnabled(true);
        renderer.setModel(glm::mat4(1.0f));

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        boundReceiver.drawObject();
        renderer.endFrame();
        unsigned char boundBright[4];
        readCentre(boundBright);
        const Renderer::FrameStats boundFirstStats = renderer.frameStats();
        assert(boundBright[0] > 120 &&
               "Material Property Bind did not reach the native receiver pixel");
        assert(boundFirstStats.sdfProgramCompiles == 1 &&
               "first Property-Bound response draw did not compile exactly once");

        assert(PropertyPath::parse("diffuse").setValue(
                   *boundMat, PropertyValue(0.18f)) == PropertyPath::PathResult::Ok);
        assert(boundMat->getResponseRevision() == boundResponseRevision &&
               boundMat->responseExpr->toJson().dump() == boundResponseJson &&
               "ordinary bound-property edit mutated response structure/revision");

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        boundReceiver.drawObject();
        renderer.endFrame();
        unsigned char boundDim[4];
        readCentre(boundDim);
        const Renderer::FrameStats boundValueStats = renderer.frameStats();
        std::printf("AD-21 response Bind red %d -> %d; compiles=%u cacheHits=%u paramBytes=%zu\n",
                    boundBright[0], boundDim[0],
                    boundValueStats.sdfProgramCompiles,
                    boundValueStats.sdfProgramCacheHits,
                    boundValueStats.sdfParameterBytesUploaded);
        assert(boundBright[0] > boundDim[0] + 60 &&
               "live Material Property Bind edit did not refresh native response value");
        assert(boundValueStats.sdfProgramCompiles == 0 &&
               boundValueStats.sdfProgramCacheHits >= 1 &&
               boundValueStats.sdfParameterBytesUploaded > 0 &&
               "Property Bind value edit recompiled WGSL or reused stale parameters");

        renderer.setRadianceField(nullptr, 0);
    }

    // --- Rung-8 WebGPU begin2D native pixel witness ----------------------------
    // Production begin2D now consumes OntoMath's top-left ZO orthographic
    // projection. Draw only the top-left quarter and prove through GPU readback
    // that y=0 is still the framebuffer top rather than silently flipping.
    {
        const float halfW = static_cast<float>(W) * 0.5f;
        const float halfH = static_cast<float>(H) * 0.5f;
        const std::vector<glm::vec2> topLeftQuad = {
            {0.0f, 0.0f}, {halfW, 0.0f}, {halfW, halfH},
            {0.0f, 0.0f}, {halfW, halfH}, {0.0f, halfH},
        };

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.begin2D(W, H);
        renderer.drawTris2D(topLeftQuad, glm::vec4(1, 0, 1, 1));
        renderer.end2D();
        renderer.endFrame();

        unsigned char topLeft2D[4];
        unsigned char bottomLeft2D[4];
        readAt(2, 2, topLeft2D);
        readAt(2, H - 3, bottomLeft2D);
        assert(topLeft2D[0] > 240 && topLeft2D[2] > 240 &&
               topLeft2D[1] < 15 &&
               "OntoMath-authored begin2D projection did not cover the top-left quarter");
        assert(bottomLeft2D[0] < 15 && bottomLeft2D[1] < 15 &&
               bottomLeft2D[2] < 15 &&
               "OntoMath-authored begin2D projection flipped or stretched screen-space Y");
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

    // --- AUTHORED RECEIVER LAW ON ORDINARY TRIANGLE MESH -------------------
    // Rasterization may discover the surface, but it must execute the same
    // Material response mathematics as the SDF path. A live Bind edit changes
    // only response parameter bytes, never the mesh WGSL/pipeline.
    {
        auto meshResponseMat = materials.create("webgpu_mesh_response_witness");
        meshResponseMat->baseColor = glm::vec3(1.0f);
        meshResponseMat->ambient = 0.0f;
        meshResponseMat->diffuse = 0.82f;
        meshResponseMat->specular = 0.0f;

        auto meshResponseRoot = std::make_shared<OntoMath::MathNode>();
        meshResponseRoot->op = OntoMath::MathNode::Op::VectorConstruct;
        auto meshDiffuseBind = std::make_unique<OntoMath::MathNode>();
        meshDiffuseBind->op = OntoMath::MathNode::Op::Bind;
        meshDiffuseBind->propertyPath = "diffuse";
        meshResponseRoot->children.push_back(std::move(meshDiffuseBind));
        meshResponseRoot->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
        meshResponseRoot->children.push_back(
            std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
        meshResponseMat->responseExpr =
            std::make_shared<OntoMath::Piecewise>(
                OntoMath::Piecewise::continuous(meshResponseRoot));
        meshResponseMat->environmentResponseExpr.reset();
        meshResponseMat->bumpResponseRevision();
        const uint32_t meshResponseRevision =
            meshResponseMat->getResponseRevision();
        const std::string meshResponseJson =
            meshResponseMat->responseExpr->toJson().dump();

        Object meshReceiver;
        meshReceiver.setMaterialId("material.webgpu_mesh_response_witness");

        renderer.setLightingEnabled(true);
        renderer.setLight(glm::vec3(0.0f, 0.0f, 2.0f),
                          glm::vec3(0.2f),
                          glm::vec3(0.8f),
                          glm::vec3(1.0f));
        renderer.setModel(glm::mat4(1.0f));

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        meshReceiver.drawObject();
        renderer.endFrame();
        unsigned char meshBright[4];
        readCentre(meshBright);
        const Renderer::FrameStats meshFirstStats = renderer.frameStats();
        std::printf(
            "mesh authored response bright=(%d,%d,%d) compiles=%u paramBytes=%zu\n",
            meshBright[0], meshBright[1], meshBright[2],
            meshFirstStats.meshProgramCompiles,
            meshFirstStats.meshParameterBytesUploaded);
        assert(meshBright[0] > 175 &&
               meshBright[1] < 20 &&
               meshBright[2] < 20 &&
               "ordinary mesh did not execute its authored Material response");
        assert(meshFirstStats.meshProgramCompiles == 1 &&
               meshFirstStats.meshParameterBytesUploaded > 0 &&
               "first authored mesh response did not compile/upload its receiver program");

        assert(PropertyPath::parse("diffuse").setValue(
                   *meshResponseMat, PropertyValue(0.18f)) ==
               PropertyPath::PathResult::Ok);
        assert(meshResponseMat->getResponseRevision() == meshResponseRevision &&
               meshResponseMat->responseExpr->toJson().dump() == meshResponseJson &&
               "bound mesh Material edit mutated receiver structure/revision");

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        meshReceiver.drawObject();
        renderer.endFrame();
        unsigned char meshDim[4];
        readCentre(meshDim);
        const Renderer::FrameStats meshValueStats = renderer.frameStats();
        std::printf(
            "mesh authored response dim=(%d,%d,%d) compiles=%u paramBytes=%zu\n",
            meshDim[0], meshDim[1], meshDim[2],
            meshValueStats.meshProgramCompiles,
            meshValueStats.meshParameterBytesUploaded);
        assert(meshBright[0] > meshDim[0] + 110 &&
               meshDim[0] >= 35 && meshDim[0] <= 60 &&
               "mesh Bind value edit did not refresh the authored receiver pixel");
        assert(meshValueStats.meshProgramCompiles == 0 &&
               meshValueStats.meshParameterBytesUploaded > 0 &&
               meshValueStats.meshProgramRefusals == 0 &&
               "mesh Bind value edit recompiled or refused instead of refreshing parameters");
    }

    // --- MESH / IMPLICIT RECEIVER-LAW REPRESENTATION PARITY ----------------
    // Geometry representation may change how the surface interaction is found.
    // It must not change the Material response law that owns the pixel.
    {
        auto parityMat = materials.create("webgpu_mesh_sdf_response_parity");
        parityMat->baseColor = glm::vec3(1.0f);

        auto parityRoot = vectorNode(0.23, 0.61, 0.37);
        parityMat->responseExpr =
            std::make_shared<OntoMath::Piecewise>(
                OntoMath::Piecewise::continuous(parityRoot));
        parityMat->environmentResponseExpr.reset();
        parityMat->bumpResponseRevision();

        Object meshSphere;
        meshSphere.setShapeKind(Object::ShapeKind::Sphere);
        meshSphere.setRenderMode(Object::RenderMode::Mesh);
        meshSphere.setMaterialId(
            "material.webgpu_mesh_sdf_response_parity");

        Object implicitSphere;
        implicitSphere.setShapeKind(Object::ShapeKind::Sphere);
        implicitSphere.setRenderMode(Object::RenderMode::Auto);
        implicitSphere.setMaterialId(
            "material.webgpu_mesh_sdf_response_parity");

        auto parityRhoRoot = scalarNode(1.0);
        OntoMath::Piecewise parityRho =
            OntoMath::Piecewise::continuous(parityRhoRoot);
        renderer.setRadianceSources({}, 0);
        renderer.setRadianceField(&parityRho, 6201);
        renderer.setRadianceChroma(nullptr, 0);
        renderer.setRadianceAngular(nullptr, 0);
        renderer.setRadianceVisibilityEnabled(false);
        renderer.setLight(glm::vec3(0.0f, 0.0f, 2.0f),
                          glm::vec3(0.0f),
                          glm::vec3(0.8f),
                          glm::vec3(0.0f));
        renderer.setLightingEnabled(true);
        renderer.setModel(glm::mat4(1.0f));

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        meshSphere.drawObject();
        renderer.endFrame();
        unsigned char meshRepresentation[4];
        readCentre(meshRepresentation);

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        implicitSphere.drawObject();
        renderer.endFrame();
        unsigned char implicitRepresentation[4];
        readCentre(implicitRepresentation);

        std::printf(
            "surface-response parity mesh=(%d,%d,%d) implicit=(%d,%d,%d)\n",
            meshRepresentation[0], meshRepresentation[1],
            meshRepresentation[2], implicitRepresentation[0],
            implicitRepresentation[1], implicitRepresentation[2]);

        assert(meshRepresentation[0] > 45 &&
               meshRepresentation[1] > 135 &&
               meshRepresentation[2] > 75 &&
               "mesh representation did not produce the authored receiver RGB");
        assert(std::abs(int(meshRepresentation[0]) -
                        int(implicitRepresentation[0])) <= 3 &&
               std::abs(int(meshRepresentation[1]) -
                        int(implicitRepresentation[1])) <= 3 &&
               std::abs(int(meshRepresentation[2]) -
                        int(implicitRepresentation[2])) <= 3 &&
               "mesh and implicit representations disagreed on the same authored Material response");

        renderer.setRadianceField(nullptr, 0);
    }

    // --- REPRESENTATION-INDEPENDENT AUTHORED SOURCE TRANSPORT ---------------
    // Receiver law is deliberately just canonical incidentRadiance. Constant
    // rho/chi/alpha remove geometry-dependent lobe/normal differences, so mesh
    // and implicit representations must witness the same Source -> Receiver
    // transport while visibility is explicitly disabled (V=1).
    {
        auto transportMat =
            materials.create("webgpu_source_transport_parity");
        transportMat->baseColor = glm::vec3(1.0f);

        auto incidentRoot = std::make_shared<OntoMath::MathNode>();
        incidentRoot->op = OntoMath::MathNode::Op::ValueLeaf;
        incidentRoot->variableName = OntoMath::kIncidentRadianceVar;
        transportMat->responseExpr =
            std::make_shared<OntoMath::Piecewise>(
                OntoMath::Piecewise::continuous(incidentRoot));
        transportMat->environmentResponseExpr.reset();
        transportMat->bumpResponseRevision();

        Object meshTransport;
        meshTransport.setShapeKind(Object::ShapeKind::Sphere);
        meshTransport.setRenderMode(Object::RenderMode::Mesh);
        meshTransport.setMaterialId(
            "material.webgpu_source_transport_parity");

        Object implicitTransport;
        implicitTransport.setShapeKind(Object::ShapeKind::Sphere);
        implicitTransport.setRenderMode(Object::RenderMode::Auto);
        implicitTransport.setMaterialId(
            "material.webgpu_source_transport_parity");

        auto rhoRoot = scalarNode(0.8);
        auto chiRoot = vectorNode(0.25, 0.75, 0.50);
        auto alphaRoot = scalarNode(0.75);
        OntoMath::Piecewise rho =
            OntoMath::Piecewise::continuous(rhoRoot);
        OntoMath::Piecewise chi =
            OntoMath::Piecewise::continuous(chiRoot);
        OntoMath::Piecewise alpha =
            OntoMath::Piecewise::continuous(alphaRoot);

        Rendering::RadianceSourceBinding source;
        source.producerId = "webgpu-source-transport-parity";
        source.position = glm::vec3(0.0f, 0.0f, 2.0f);
        source.radianceExpr = &rho;
        source.radianceRevision = 1;
        source.chromaExpr = &chi;
        source.chromaRevision = 1;
        source.angularExpr = &alpha;
        source.angularRevision = 1;
        source.temporalCoordinate = 0.0;
        source.temporalDelta = 0.0;
        source.enabled = true;

        renderer.setRadianceField(nullptr, 0);
        renderer.setRadianceChroma(nullptr, 0);
        renderer.setRadianceAngular(nullptr, 0);
        renderer.setRadianceSources({source}, 7301);
        renderer.setRadianceVisibilityEnabled(false);
        renderer.setLightingEnabled(true);
        renderer.setModel(glm::mat4(1.0f));

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        meshTransport.drawObject();
        renderer.endFrame();
        unsigned char sourceMeshA[4];
        readCentre(sourceMeshA);
        const Renderer::FrameStats sourceMeshAStats =
            renderer.frameStats();

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        implicitTransport.drawObject();
        renderer.endFrame();
        unsigned char sourceImplicitA[4];
        readCentre(sourceImplicitA);

        std::printf(
            "source transport mesh=(%d,%d,%d) implicit=(%d,%d,%d) compiles=%u\n",
            sourceMeshA[0], sourceMeshA[1], sourceMeshA[2],
            sourceImplicitA[0], sourceImplicitA[1], sourceImplicitA[2],
            sourceMeshAStats.meshProgramCompiles);

        // Expected linear output is approximately (0.15, 0.45, 0.30).
        assert(sourceMeshA[0] >= 32 && sourceMeshA[0] <= 46 &&
               sourceMeshA[1] >= 108 && sourceMeshA[1] <= 122 &&
               sourceMeshA[2] >= 70 && sourceMeshA[2] <= 84 &&
               "mesh did not consume authored chi * rho * alpha source transport");
        assert(std::abs(int(sourceMeshA[0]) -
                        int(sourceImplicitA[0])) <= 3 &&
               std::abs(int(sourceMeshA[1]) -
                        int(sourceImplicitA[1])) <= 3 &&
               std::abs(int(sourceMeshA[2]) -
                        int(sourceImplicitA[2])) <= 3 &&
               "mesh and implicit surfaces disagree on authored rho/chi/alpha transport");

        // VALUE ONLY: halve rho. Source-set structure remains identical; the
        // mesh pipeline must survive while only the combined parameter stream
        // changes.
        rhoRoot->scalarForm.terms[0].coefficient = 0.4;
        source.radianceRevision = 2;
        renderer.setRadianceSources({source}, 7302);

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        meshTransport.drawObject();
        renderer.endFrame();
        unsigned char sourceMeshB[4];
        readCentre(sourceMeshB);
        const Renderer::FrameStats sourceMeshBStats =
            renderer.frameStats();

        assert(sourceMeshA[0] > sourceMeshB[0] + 15 &&
               sourceMeshA[1] > sourceMeshB[1] + 45 &&
               sourceMeshA[2] > sourceMeshB[2] + 28 &&
               "numeric rho edit did not reach ordinary mesh source transport");
        assert(sourceMeshBStats.meshProgramCompiles == 0 &&
               sourceMeshBStats.meshProgramRefusals == 0 &&
               sourceMeshBStats.meshParameterBytesUploaded > 0 &&
               "numeric rho edit recompiled/refused mesh WGSL instead of refreshing source params");

        // LOCAL RECEIVER VISIBILITY: compatibility bool-on now means the
        // explicitly scoped local-receiver query. Mesh cannot answer it, but the
        // exact implicit representation can answer from its own SDF.
        renderer.setRadianceVisibilityEnabled(true);
        assert(renderer.radianceVisibilityMode() ==
                   RadianceVisibilityMode::LocalReceiverGeometry &&
               "legacy visibility bool must map to LocalReceiverGeometry");

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        meshTransport.drawObject();
        renderer.endFrame();
        unsigned char sourceMeshLocalUnknown[4];
        readCentre(sourceMeshLocalUnknown);
        const Renderer::FrameStats sourceMeshLocalStats =
            renderer.frameStats();
        assert(sourceMeshLocalUnknown[0] < 5 &&
               sourceMeshLocalUnknown[1] < 5 &&
               sourceMeshLocalUnknown[2] < 5 &&
               sourceMeshLocalStats.meshVisibilityUnknownDirectSkips > 0 &&
               "mesh silently substituted V=1 for unknown local-receiver visibility");

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        implicitTransport.drawObject();
        renderer.endFrame();
        unsigned char sourceImplicitLocalKnown[4];
        readCentre(sourceImplicitLocalKnown);
        assert(sourceImplicitLocalKnown[0] > 8 &&
               sourceImplicitLocalKnown[1] > 35 &&
               sourceImplicitLocalKnown[2] > 20 &&
               "implicit local-receiver visibility failed to return known-unoccluded transport");

        // COMPLETE SCENE VISIBILITY: first lawful provider. Move the source
        // off the camera axis so a blocker can sit on the source ray without
        // hiding the centre receiver pixel from the eye.
        source.position = glm::vec3(1.4f, 0.0f, 1.4f);
        renderer.setRadianceSources({source}, 7303);
        renderer.setRadianceVisibilityMode(
            RadianceVisibilityMode::CompleteScene);

        // Mesh-only opaque scene is completely represented by the frame's exact
        // world-triangle materialization. No blocker => known visible.
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        meshTransport.drawObject();
        renderer.endFrame();
        unsigned char sourceMeshSceneVisible[4];
        readCentre(sourceMeshSceneVisible);
        const Renderer::FrameStats sourceMeshSceneVisibleStats =
            renderer.frameStats();

        assert(sourceMeshSceneVisible[0] > 8 &&
               sourceMeshSceneVisible[1] > 35 &&
               sourceMeshSceneVisible[2] > 20 &&
               sourceMeshSceneVisibleStats.sceneVisibilityMeshKnownFrames > 0 &&
               sourceMeshSceneVisibleStats.meshVisibilityUnknownDirectSkips == 0 &&
               sourceMeshSceneVisibleStats.sceneVisibilityTriangles > 0 &&
               "complete mesh scene did not yield a known visible path");

        // Small opaque mesh blocker lies on the receiver->source segment but is
        // displaced from the camera's x=0 centre ray.
        Object meshBlocker;
        meshBlocker.setShapeKind(Object::ShapeKind::Sphere);
        meshBlocker.setRenderMode(Object::RenderMode::Mesh);
        meshBlocker.setMaterialId("material.webgpu_source_transport_parity");
        const glm::mat4 blockerModel =
            glm::translate(glm::mat4(1.0f), glm::vec3(0.7f, 0.0f, 0.95f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.22f));

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.setModel(glm::mat4(1.0f));
        meshTransport.drawObject();
        renderer.setModel(blockerModel);
        meshBlocker.drawObject();
        renderer.endFrame();
        unsigned char sourceMeshSceneBlocked[4];
        readCentre(sourceMeshSceneBlocked);
        const Renderer::FrameStats sourceMeshSceneBlockedStats =
            renderer.frameStats();

        assert(sourceMeshSceneBlocked[0] < 5 &&
               sourceMeshSceneBlocked[1] < 5 &&
               sourceMeshSceneBlocked[2] < 5 &&
               sourceMeshSceneBlockedStats.sceneVisibilityMeshKnownFrames > 0 &&
               sourceMeshSceneBlockedStats.meshVisibilityUnknownDirectSkips == 0 &&
               "complete mesh scene failed to report a known occluded source path");

        // Mixed representation: exactly one SDF receiver may combine its exact
        // local field with the SAME complete mesh-triangle blocker set.
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.setModel(glm::mat4(1.0f));
        implicitTransport.drawObject();
        renderer.setModel(blockerModel);
        meshBlocker.drawObject();
        renderer.endFrame();
        unsigned char sourceImplicitSceneBlocked[4];
        readCentre(sourceImplicitSceneBlocked);
        const Renderer::FrameStats sourceImplicitSceneBlockedStats =
            renderer.frameStats();

        assert(sourceImplicitSceneBlocked[0] < 5 &&
               sourceImplicitSceneBlocked[1] < 5 &&
               sourceImplicitSceneBlocked[2] < 5 &&
               sourceImplicitSceneBlockedStats.sceneVisibilitySdfKnownFrames > 0 &&
               sourceImplicitSceneBlockedStats.sdfVisibilityUnknownDirectSkips == 0 &&
               "one SDF receiver plus mesh blockers failed complete-scene visibility");

        // Same-program SDF plurality is exact: the current SDF pipeline's
        // instance storage already contains every transform/parameter record.
        // First prove the one-instance cohort is known visible.
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.setModel(glm::mat4(1.0f));
        implicitTransport.drawObject();
        renderer.endFrame();
        unsigned char sourceImplicitCohortVisible[4];
        readCentre(sourceImplicitCohortVisible);
        const Renderer::FrameStats sourceImplicitCohortVisibleStats =
            renderer.frameStats();
        assert(sourceImplicitCohortVisible[0] > 8 &&
               sourceImplicitCohortVisible[1] > 35 &&
               sourceImplicitCohortVisible[2] > 20 &&
               sourceImplicitCohortVisibleStats.sceneVisibilitySdfKnownFrames > 0 &&
               sourceImplicitCohortVisibleStats.sdfVisibilityUnknownDirectSkips == 0 &&
               "single-program SDF cohort did not yield known visible transport");

        // A second instance of the SAME compiled sphere program lies exactly on
        // the receiver->source segment. CompleteScene must remain KNOWN and become
        // occluded; plurality of instances is not plurality of field laws.
        Object secondImplicit;
        secondImplicit.setShapeKind(Object::ShapeKind::Sphere);
        secondImplicit.setRenderMode(Object::RenderMode::Auto);
        secondImplicit.setMaterialId("material.webgpu_source_transport_parity");

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.setModel(glm::mat4(1.0f));
        implicitTransport.drawObject();
        renderer.setModel(
            glm::translate(glm::mat4(1.0f), glm::vec3(0.7f, 0.0f, 0.95f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.22f)));
        secondImplicit.drawObject();
        renderer.endFrame();
        unsigned char sourceSdfCohortBlocked[4];
        readCentre(sourceSdfCohortBlocked);
        const Renderer::FrameStats sourceSdfCohortBlockedStats =
            renderer.frameStats();

        assert(sourceSdfCohortBlocked[0] < 5 &&
               sourceSdfCohortBlocked[1] < 5 &&
               sourceSdfCohortBlocked[2] < 5 &&
               sourceSdfCohortBlockedStats.sceneVisibilitySdfKnownFrames > 0 &&
               sourceSdfCohortBlockedStats.sdfVisibilityUnknownDirectSkips == 0 &&
               "same-program second SDF instance was not admitted as an exact blocker");

        // DISTINCT PROGRAM, EXACT PACKET: a box is a different compiled SDF
        // law from the sphere receiver, but this bounded provider can lower the
        // primitive leaf itself into an exact analytic segment blocker.
        Object differentImplicit;
        differentImplicit.setFieldShape(
            geom::SdfNode::leaf(
                geom::SdfPrim::Box, glm::vec3(0.5f)),
            glm::vec3(0.75f));
        differentImplicit.setRenderMode(Object::RenderMode::Auto);
        differentImplicit.setMaterialId("material.webgpu_source_transport_parity");
        const glm::mat4 implicitBoxBlockerModel =
            glm::translate(glm::mat4(1.0f), glm::vec3(0.7f, 0.0f, 0.95f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.22f));

        // Mesh receivers have no local SDF evaluator, so this proves they consume
        // the global exact analytic implicit packet rather than silently ignoring
        // the box or demoting CompleteScene to Unknown.
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.setModel(glm::mat4(1.0f));
        meshTransport.drawObject();
        renderer.setModel(implicitBoxBlockerModel);
        differentImplicit.drawObject();
        renderer.endFrame();
        unsigned char sourceMeshImplicitBlocked[4];
        readCentre(sourceMeshImplicitBlocked);
        const Renderer::FrameStats sourceMeshImplicitBlockedStats =
            renderer.frameStats();

        assert(sourceMeshImplicitBlocked[0] < 5 &&
               sourceMeshImplicitBlocked[1] < 5 &&
               sourceMeshImplicitBlocked[2] < 5 &&
               sourceMeshImplicitBlockedStats.sceneVisibilityMeshKnownFrames > 0 &&
               sourceMeshImplicitBlockedStats.meshVisibilityUnknownDirectSkips == 0 &&
               sourceMeshImplicitBlockedStats.sceneVisibilityImplicitBlockers > 0 &&
               "mesh receiver did not consume exact analytic implicit blocker packet");

        // SDF receiver: its own sphere cohort stays in the exact local/cohort
        // evaluator while ONLY the foreign box travels through the packet.
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.setModel(glm::mat4(1.0f));
        implicitTransport.drawObject();
        renderer.setModel(implicitBoxBlockerModel);
        differentImplicit.drawObject();
        renderer.endFrame();
        unsigned char sourceSdfMixedProgramBlocked[4];
        readCentre(sourceSdfMixedProgramBlocked);
        const Renderer::FrameStats sourceSdfMixedProgramBlockedStats =
            renderer.frameStats();

        assert(sourceSdfMixedProgramBlocked[0] < 5 &&
               sourceSdfMixedProgramBlocked[1] < 5 &&
               sourceSdfMixedProgramBlocked[2] < 5 &&
               sourceSdfMixedProgramBlockedStats.sceneVisibilitySdfKnownFrames > 0 &&
               sourceSdfMixedProgramBlockedStats.sdfVisibilityUnknownDirectSkips == 0 &&
               sourceSdfMixedProgramBlockedStats.sceneVisibilityImplicitBlockers > 0 &&
               "foreign analytic box program did not retain complete-scene SDF authority");

        // DISTINCT PROGRAM, UNSUPPORTED PACKET: torus intersection has not been
        // admitted by this bounded provider. Its mere presence must revoke scene
        // authority instead of being triangulated, guessed, or silently omitted.
        Object unsupportedImplicit;
        unsupportedImplicit.setFieldShape(
            geom::SdfNode::leaf(
                geom::SdfPrim::Torus, glm::vec3(0.5f, 0.2f, 0.0f)),
            glm::vec3(0.8f));
        unsupportedImplicit.setRenderMode(Object::RenderMode::Auto);
        unsupportedImplicit.setMaterialId("material.webgpu_source_transport_parity");

        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.setModel(glm::mat4(1.0f));
        implicitTransport.drawObject();
        renderer.setModel(
            glm::translate(glm::mat4(1.0f), glm::vec3(0.9f, 0.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.22f)));
        unsupportedImplicit.drawObject();
        renderer.endFrame();
        unsigned char sourceSdfUnsupportedProgramUnknown[4];
        readCentre(sourceSdfUnsupportedProgramUnknown);
        const Renderer::FrameStats sourceSdfUnsupportedProgramUnknownStats =
            renderer.frameStats();

        assert(sourceSdfUnsupportedProgramUnknown[0] < 5 &&
               sourceSdfUnsupportedProgramUnknown[1] < 5 &&
               sourceSdfUnsupportedProgramUnknown[2] < 5 &&
               sourceSdfUnsupportedProgramUnknownStats.sceneVisibilitySdfKnownFrames == 0 &&
               sourceSdfUnsupportedProgramUnknownStats.sdfVisibilityUnknownDirectSkips > 0 &&
               "unsupported foreign SDF program was silently omitted from complete-scene truth");

        renderer.setRadianceVisibilityMode(RadianceVisibilityMode::Disabled);
        renderer.setRadianceSources({}, 0);
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
