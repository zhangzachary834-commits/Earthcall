// Volumetric mist and light shaft physical tribunal.
//
// Verifies the physical formation:
//   LIGHT + MEDIUM + SPACE + VIEW -> visible volumetric radiance
//
// In an atmospheric participating medium (mist/fog) under direct illumination:
// 1. Without occluder geometry, the medium scatters incident light uniformly.
// 2. With authored occluder geometry (an aperture / blocker), volumetric visibility
//    traces path clearance to the radiant source via bounded sphere-tracing.
// 3. Illuminated beam paths (vis = 1) exhibit high scattered radiance.
// 4. Shadowed regions (vis = 0) exhibit minimal radiance.
// 5. Spatial contrast between illuminated beam and shadow is strongly distinguished:
//    I_beam / (I_shadow + eps) >> 1.

#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

namespace {
constexpr uint32_t W = 32, H = 16;

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

void readImage(const wgpu::Device& gpu, WGPUTexture target, WGPUBuffer readback,
               std::vector<unsigned char>& outPixels) {
    outPixels.resize(W * H * 4);
    WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    WGPUTexelCopyTextureInfo src = {};
    src.texture = target;
    src.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyBufferInfo dst = {};
    dst.buffer = readback;
    dst.layout.bytesPerRow = 256;
    dst.layout.rowsPerImage = H;
    WGPUExtent3D ext = {W, H, 1};
    wgpuCommandEncoderCopyTextureToBuffer(enc, &src, &dst, &ext);
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
    wgpuQueueSubmit(gpu.queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(enc);

    MapR m;
    WGPUBufferMapCallbackInfo ci = {};
    ci.mode = WGPUCallbackMode_AllowProcessEvents;
    ci.callback = onMap;
    ci.userdata1 = &m;
    wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, 256 * H, ci);
    while (!m.done) wgpuInstanceProcessEvents(gpu.instance);
    assert(m.ok && "WebGPU readback map failed");
    const auto* px = static_cast<const unsigned char*>(
        wgpuBufferGetConstMappedRange(readback, 0, 256 * H));
    for (uint32_t y = 0; y < H; ++y) {
        for (uint32_t x = 0; x < W; ++x) {
            size_t srcIdx = y * 256 + x * 4;
            size_t dstIdx = (y * W + x) * 4;
            outPixels[dstIdx + 0] = px[srcIdx + 0];
            outPixels[dstIdx + 1] = px[srcIdx + 1];
            outPixels[dstIdx + 2] = px[srcIdx + 2];
            outPixels[dstIdx + 3] = px[srcIdx + 3];
        }
    }
    wgpuBufferUnmap(readback);
}
} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    wgpu::Device gpu;
    if (!gpu.init()) { std::printf("SKIP: no WebGPU device\n"); return 0; }
    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) { std::printf("FAIL: renderer init\n"); return 1; }
    setCurrentRenderer(&renderer);

    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = {W, H, 1};
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1;
    td.sampleCount = 1;
    WGPUTexture target = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(target, nullptr);

    WGPUBufferDescriptor rbd = {};
    rbd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    rbd.size = 256 * H;
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &rbd);

    const glm::vec3 eye(0.0f, 0.0f, 2.5f);
    const glm::mat4 proj = glm::perspectiveZO(
        glm::radians(45.0f), float(W) / H, 0.1f, 50.0f);
    const glm::mat4 view3d = glm::lookAt(
        eye, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    renderer.setCamera(view3d, proj, eye);

    // Warm golden incident sunlight positioned behind the medium at z = -3.5.
    const glm::vec3 lightPos(0.0f, 0.0f, -3.5f);
    const glm::vec3 lightAmbient(0.0f);
    const glm::vec3 lightDiffuse(3.0f, 2.4f, 1.2f);
    const glm::vec3 lightSpecular(0.0f);
    renderer.setLight(lightPos, lightAmbient, lightDiffuse, lightSpecular);
    renderer.setLightingEnabled(true);
    Rendering::RadianceSourceBinding sunSource;
    sunSource.position = lightPos;
    sunSource.ambientRadiance = lightAmbient;
    sunSource.diffuseRadiance = lightDiffuse;
    sunSource.specularRadiance = lightSpecular;
    sunSource.coefficients = glm::vec4(1.0f);
    sunSource.enabled = true;

    // Cross-rung authority witness: volumetric transport consumes source rho,
    // but source value/structure revisions remain source-owned.
    auto sourceRhoNode = scalarNode(1.0);
    OntoMath::Piecewise sourceRho = OntoMath::Piecewise::continuous(sourceRhoNode);
    sunSource.radianceExpr = &sourceRho;
    sunSource.radianceRevision = 9000;
    renderer.setRadianceSources({sunSource}, 9001);

    // Physical mist parameters
    auto densityNode = scalarNode(1.0);
    auto extinctionNode = scalarNode(0.5);
    auto scatterNode = scalarNode(0.45);
    auto mediumChromaNode = vectorNode(0.9, 0.95, 1.0); // cool atmospheric tint

    OntoMath::Piecewise density = OntoMath::Piecewise::continuous(densityNode);
    OntoMath::Piecewise extinction = OntoMath::Piecewise::continuous(extinctionNode);
    OntoMath::Piecewise scatter = OntoMath::Piecewise::continuous(scatterNode);
    OntoMath::Piecewise mediumChroma = OntoMath::Piecewise::continuous(mediumChromaNode);

    Rendering::VolumeDensityBinding mist;
    mist.origin = glm::vec3(0.0f);
    mist.scale = glm::vec3(1.5f);
    mist.densityExpr = &density;
    mist.densityRevision = 9101;
    mist.extinctionExpr = &extinction;
    mist.extinctionRevision = 9102;
    mist.scatteringExpr = &scatter;
    mist.scatteringRevision = 9103;
    mist.volumeChromaExpr = &mediumChroma;
    mist.volumeChromaRevision = 9104;

    // Phase 1: Unoccluded uniform mist.
    renderer.setVolumeDensitySources({mist}, 9110);
    renderer.setModel(glm::mat4(1.0f));
    renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    renderer.composeVolumes();
    renderer.endFrame();

    std::vector<unsigned char> unoccludedPx;
    readImage(gpu, target, readback, unoccludedPx);

    // Sample left (x=6, y=8) and right (x=26, y=8) in unoccluded mist
    size_t leftIdx = (8 * W + 6) * 4;
    size_t rightIdx = (8 * W + 26) * 4;
    int unoccLeftLum = unoccludedPx[leftIdx] + unoccludedPx[leftIdx+1] + unoccludedPx[leftIdx+2];
    int unoccRightLum = unoccludedPx[rightIdx] + unoccludedPx[rightIdx+1] + unoccludedPx[rightIdx+2];

    std::printf("Phase 1 unoccluded mist: left=(%d,%d,%d, lum=%d) right=(%d,%d,%d, lum=%d)\n",
                unoccludedPx[leftIdx], unoccludedPx[leftIdx+1], unoccludedPx[leftIdx+2], unoccLeftLum,
                unoccludedPx[rightIdx], unoccludedPx[rightIdx+1], unoccludedPx[rightIdx+2], unoccRightLum);
    assert(unoccLeftLum > 30 && unoccRightLum > 30 &&
           "Unoccluded mist must be visibly illuminated across both sides");

    // Source numeric refresh must reach the already-compiled volume transport.
    // Keep the outer source-set revision deliberately unchanged; the projected
    // source channel's own revision is the authority under test.
    sourceRhoNode->scalarForm.terms[0].coefficient = 0.1;
    sunSource.radianceRevision = 9002;
    renderer.setRadianceSources({sunSource}, 9001);
    renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    renderer.composeVolumes();
    renderer.endFrame();
    std::vector<unsigned char> dimmedPx;
    readImage(gpu, target, readback, dimmedPx);
    const int dimmedLeftLum =
        dimmedPx[leftIdx] + dimmedPx[leftIdx+1] + dimmedPx[leftIdx+2];
    const Renderer::FrameStats sourceRefreshStats = renderer.frameStats();
    assert(dimmedLeftLum < unoccLeftLum / 2 &&
           "numeric source rho edit did not refresh volumetric incident transport");
    assert(sourceRefreshStats.volumeProgramCompiles == 0 &&
           "numeric source rho edit regenerated volume WGSL instead of refreshing params");

    // Restore full source strength for the visibility tribunal.
    sourceRhoNode->scalarForm.terms[0].coefficient = 1.0;
    sunSource.radianceRevision = 9003;
    renderer.setRadianceSources({sunSource}, 9001);

    // Phase 2: Authored occluder geometry (a solid blocker covering the left half x < 0).
    // The blocker sits between the medium center and the light at z = -1.5, spanning x in [-2.0, -0.05].
    auto occluder = std::make_shared<geom::SdfNode>();
    occluder->op = geom::SdfOp::Leaf;
    occluder->prim = geom::SdfPrim::Box;
    occluder->dims = glm::vec3(1.0f, 2.0f, 0.5f);
    occluder->offset = glm::vec3(-1.05f, 0.0f, -1.2f);

    mist.occluderSdf = occluder.get();
    mist.occluderRevision = 9201;

    renderer.setVolumeDensitySources({mist}, 9210);
    renderer.setModel(glm::mat4(1.0f));
    renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    renderer.composeVolumes();
    renderer.endFrame();

    std::vector<unsigned char> occludedPx;
    readImage(gpu, target, readback, occludedPx);

    int occLeftLum = occludedPx[leftIdx] + occludedPx[leftIdx+1] + occludedPx[leftIdx+2];
    int occRightLum = occludedPx[rightIdx] + occludedPx[rightIdx+1] + occludedPx[rightIdx+2];

    std::printf("Phase 2 beam & shadow: left-shadow=(%d,%d,%d, lum=%d) right-beam=(%d,%d,%d, lum=%d)\n",
                occludedPx[leftIdx], occludedPx[leftIdx+1], occludedPx[leftIdx+2], occLeftLum,
                occludedPx[rightIdx], occludedPx[rightIdx+1], occludedPx[rightIdx+2], occRightLum);

    // Assert that the shadowed region is dark and the beam region is bright
    assert(occRightLum > 30 && "Beam core must remain brightly illuminated");
    assert(occLeftLum < occRightLum / 4 && "Shadowed region must be significantly darker than the beam");

    double contrast = double(occRightLum) / (double(occLeftLum) + 1.0);
    std::printf("Phase 2 spatial contrast ratio = %.2fx\n", contrast);
    assert(contrast >= 4.0 && "Volumetric beam must exhibit strong spatial contrast against shadow");

    // Phase 3: Spatial gradient / penumbra along horizontal line across boundary
    std::printf("Horizontal luminance profile at y=8 across aperture boundary:\n");
    int prevLum = -1;
    for (uint32_t x = 4; x < 28; x += 3) {
        size_t idx = (8 * W + x) * 4;
        int lum = occludedPx[idx] + occludedPx[idx+1] + occludedPx[idx+2];
        std::printf("  x=%2d: lum=%3d (RGB=%d,%d,%d)\n",
                    x, lum, occludedPx[idx], occludedPx[idx+1], occludedPx[idx+2]);
    }

    wgpuBufferRelease(readback);
    wgpuTextureViewRelease(view);
    wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);

    std::printf("PASS: Volumetric mist and light shaft physical tribunal\n");
    return 0;
}
