// V5 native overlap-physics tribunal.
//
// This is intentionally separate from webgpu_object_test: it pins the one
// remaining physical theorem of medium-set composition with a tiny constant-
// coefficient scene whose closed-form answer is known.  The test distinguishes
// one shared extinction integral from either possible sequential whole-medium
// alpha ordering, then proves that two different authored source channels
// (self-emission and scattering/chroma) both survive the same overlap.

#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>

namespace {
constexpr uint32_t W = 16, H = 16;

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

std::shared_ptr<OntoMath::MathNode> valueNode(const char* name) {
    auto node = std::make_shared<OntoMath::MathNode>();
    node->op = OntoMath::MathNode::Op::ValueLeaf;
    node->variableName = name;
    return node;
}

std::shared_ptr<OntoMath::MathNode> multiplyNode(
    const std::shared_ptr<OntoMath::MathNode>& lhs,
    const std::shared_ptr<OntoMath::MathNode>& rhs) {
    auto node = std::make_shared<OntoMath::MathNode>();
    node->op = OntoMath::MathNode::Op::Multiply;
    node->children.push_back(std::make_unique<OntoMath::MathNode>(*lhs));
    node->children.push_back(std::make_unique<OntoMath::MathNode>(*rhs));
    return node;
}

int byteOf(double linear) {
    return static_cast<int>(std::lround(255.0 * linear));
}

bool nearByte(unsigned char actual, int expected, int tolerance = 3) {
    return std::abs(static_cast<int>(actual) - expected) <= tolerance;
}
} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    wgpu::Device gpu;
    if (!gpu.init()) { std::printf("FAIL: no WebGPU device\n"); return 1; }
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

    auto readCentre = [&](unsigned char out[4]) {
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
        assert(m.ok && "V5 overlap readback map failed");
        const auto* px = static_cast<const unsigned char*>(
            wgpuBufferGetConstMappedRange(readback, 0, 256 * H));
        const size_t at = size_t(H / 2) * 256 + size_t(W / 2) * 4;
        for (int i = 0; i < 4; ++i) out[i] = px[at + i];
        wgpuBufferUnmap(readback);
    };

    const glm::vec3 eye(0.0f, 0.0f, 2.0f);
    const glm::mat4 proj = glm::perspectiveZO(
        glm::radians(45.0f), float(W) / H, 0.1f, 100.0f);
    const glm::mat4 view3d = glm::lookAt(
        eye, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    renderer.setCamera(view3d, proj, eye);

    // Both media occupy exactly z=[-1,+1] on the centre ray. D=1 means the
    // authored sigma_t values below are the actual extinction coefficients.
    auto densityNode = scalarNode(1.0);
    auto extinctionANode = scalarNode(0.35);
    auto extinctionBNode = scalarNode(0.90);
    auto zeroScatterNode = scalarNode(0.0);
    auto emissionANode = vectorNode(0.20, 0.0, 0.0);
    auto emissionBNode = vectorNode(0.0, 0.0, 0.30);
    OntoMath::Piecewise density = OntoMath::Piecewise::continuous(densityNode);
    OntoMath::Piecewise extinctionA = OntoMath::Piecewise::continuous(extinctionANode);
    OntoMath::Piecewise extinctionB = OntoMath::Piecewise::continuous(extinctionBNode);
    OntoMath::Piecewise zeroScatter = OntoMath::Piecewise::continuous(zeroScatterNode);
    OntoMath::Piecewise emissionA = OntoMath::Piecewise::continuous(emissionANode);
    OntoMath::Piecewise emissionB = OntoMath::Piecewise::continuous(emissionBNode);

    Rendering::VolumeDensityBinding a;
    a.origin = glm::vec3(0.0f);
    a.scale = glm::vec3(1.0f);
    a.densityExpr = &density;
    a.densityRevision = 5801;
    a.extinctionExpr = &extinctionA;
    a.extinctionRevision = 5802;
    a.scatteringExpr = &zeroScatter;
    a.scatteringRevision = 5803;
    a.emissionExpr = &emissionA;
    a.emissionRevision = 5804;

    Rendering::VolumeDensityBinding b = a;
    b.extinctionExpr = &extinctionB;
    b.extinctionRevision = 5811;
    b.emissionExpr = &emissionB;
    b.emissionRevision = 5812;

    renderer.setVolumeDensitySources({a, b}, 5820);
    renderer.setModel(glm::mat4(1.0f));
    renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    renderer.composeVolumes();
    renderer.endFrame();
    unsigned char fused[4];
    readCentre(fused);

    constexpr double L = 2.0;
    constexpr double sigmaA = 0.35;
    constexpr double sigmaB = 0.90;
    constexpr double sourceA = 0.20;
    constexpr double sourceB = 0.30;
    const double totalSigma = sigmaA + sigmaB;
    const double fusedGain = (1.0 - std::exp(-totalSigma * L)) / totalSigma;
    const int expectedR = byteOf(sourceA * fusedGain);
    const int expectedB = byteOf(sourceB * fusedGain);

    std::printf("V5 fused closed-form native=(%d,%d,%d) expected=(%d,0,%d)\n",
                fused[0], fused[1], fused[2], expectedR, expectedB);
    assert(nearByte(fused[0], expectedR) && fused[1] <= 2 &&
           nearByte(fused[2], expectedB) &&
           "V5 overlap does not match the shared-extinction closed form");

    const double alphaA = 1.0 - std::exp(-sigmaA * L);
    const double alphaB = 1.0 - std::exp(-sigmaB * L);
    const double radA = sourceA * alphaA / sigmaA;
    const double radB = sourceB * alphaB / sigmaB;
    const int seqABR = byteOf(radA);
    const int seqABB = byteOf((1.0 - alphaA) * radB);
    const int seqBAR = byteOf((1.0 - alphaB) * radA);
    const int seqBAB = byteOf(radB);
    const int distAB = std::abs(static_cast<int>(fused[0]) - seqABR) +
                       std::abs(static_cast<int>(fused[2]) - seqABB);
    const int distBA = std::abs(static_cast<int>(fused[0]) - seqBAR) +
                       std::abs(static_cast<int>(fused[2]) - seqBAB);
    std::printf("V5 sequential counterfactual AB=(%d,0,%d) BA=(%d,0,%d) distances=(%d,%d)\n",
                seqABR, seqABB, seqBAR, seqBAB, distAB, distBA);
    assert(distAB >= 25 && distBA >= 25 &&
           "V5 native overlap is not strongly distinguished from sequential whole-medium alpha");

    auto scatterBNode = scalarNode(0.30);
    auto chromaBNode = vectorNode(0.0, 0.0, 1.0);
    OntoMath::Piecewise scatterB = OntoMath::Piecewise::continuous(scatterBNode);
    OntoMath::Piecewise chromaB = OntoMath::Piecewise::continuous(chromaBNode);
    b.scatteringExpr = &scatterB;
    b.scatteringRevision = 5831;
    b.volumeChromaExpr = &chromaB;
    b.volumeChromaRevision = 5832;
    b.emissionExpr = nullptr;
    b.emissionRevision = 0;

    renderer.setVolumeDensitySources({a, b}, 5833);
    renderer.setModel(glm::mat4(1.0f));
    renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    renderer.composeVolumes();
    renderer.endFrame();
    unsigned char mixed[4];
    readCentre(mixed);
    std::printf("V5 mixed-source overlap=(%d,%d,%d) expected=(%d,0,%d)\n",
                mixed[0], mixed[1], mixed[2], expectedR, expectedB);
    assert(nearByte(mixed[0], expectedR) && mixed[1] <= 2 &&
           nearByte(mixed[2], expectedB) &&
           "V5 overlap dropped an independent emission or scattering/chroma contribution");

    // POST-V5 NULL-PARTICIPANT STABILITY TRIBUNAL ---------------------------
    // A's source varies sharply as z^8. B is identically null but its proxy
    // lies inside A, so the current interval partitioner is tempted to move A's
    // midpoint quadrature grid merely because B has bounds. Moving B must not
    // grant that physically absent member perceptible causal power.
    auto z = valueNode("z");
    auto z2 = multiplyNode(z, z);
    auto z4 = multiplyNode(z2, z2);
    auto z8 = multiplyNode(z4, z4);
    auto varyingEmissionNode = std::make_shared<OntoMath::MathNode>();
    varyingEmissionNode->op = OntoMath::MathNode::Op::VectorConstruct;
    varyingEmissionNode->children.push_back(std::make_unique<OntoMath::MathNode>(*z8));
    varyingEmissionNode->children.push_back(std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
    varyingEmissionNode->children.push_back(std::make_unique<OntoMath::MathNode>(*scalarNode(0.0)));
    OntoMath::Piecewise varyingEmission = OntoMath::Piecewise::continuous(varyingEmissionNode);
    auto zeroDensityNode = scalarNode(0.0);
    auto zeroExtinctionNode = scalarNode(0.0);
    auto zeroEmissionNode = vectorNode(0.0, 0.0, 0.0);
    OntoMath::Piecewise zeroDensity = OntoMath::Piecewise::continuous(zeroDensityNode);
    OntoMath::Piecewise zeroExtinction = OntoMath::Piecewise::continuous(zeroExtinctionNode);
    OntoMath::Piecewise zeroEmission = OntoMath::Piecewise::continuous(zeroEmissionNode);

    Rendering::VolumeDensityBinding varyingA = a;
    varyingA.extinctionExpr = &zeroExtinction;
    varyingA.extinctionRevision = 5901;
    varyingA.emissionExpr = &varyingEmission;
    varyingA.emissionRevision = 5902;
    varyingA.scatteringExpr = &zeroScatter;
    varyingA.scatteringRevision = 5903;
    varyingA.volumeChromaExpr = nullptr;
    varyingA.volumeChromaRevision = 0;

    renderer.setVolumeDensitySources({varyingA}, 5904);
    renderer.setModel(glm::mat4(1.0f));
    renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    renderer.composeVolumes();
    renderer.endFrame();
    unsigned char baseline[4];
    readCentre(baseline);

    Rendering::VolumeDensityBinding nullB = varyingA;
    nullB.scale = glm::vec3(0.45f);
    nullB.densityExpr = &zeroDensity;
    nullB.densityRevision = 5910;
    nullB.extinctionExpr = &zeroExtinction;
    nullB.extinctionRevision = 5911;
    nullB.emissionExpr = &zeroEmission;
    nullB.emissionRevision = 5912;

    int maxNullDrift = 0;
    const float nullPositions[] = {-0.45f, -0.20f, 0.0f, 0.25f, 0.45f};
    for (size_t i = 0; i < sizeof(nullPositions) / sizeof(nullPositions[0]); ++i) {
        nullB.origin = glm::vec3(0.0f, 0.0f, nullPositions[i]);
        renderer.setVolumeDensitySources({varyingA, nullB}, 5920u + static_cast<uint32_t>(i));
        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        renderer.composeVolumes();
        renderer.endFrame();
        unsigned char withNull[4];
        readCentre(withNull);
        const int drift = std::max({
            std::abs(int(withNull[0]) - int(baseline[0])),
            std::abs(int(withNull[1]) - int(baseline[1])),
            std::abs(int(withNull[2]) - int(baseline[2]))});
        maxNullDrift = std::max(maxNullDrift, drift);
        std::printf("post-V5 null B z=%+.2f baseline=(%d,%d,%d) withNull=(%d,%d,%d) drift=%d\n",
                    nullPositions[i], baseline[0], baseline[1], baseline[2],
                    withNull[0], withNull[1], withNull[2], drift);
    }
    std::printf("post-V5 null-participant max RGB byte drift=%d\n", maxNullDrift);
    assert(maxNullDrift <= 1 &&
           "semantically null member materially changed another medium solely by repartitioning samples");

    wgpuBufferRelease(readback);
    wgpuTextureViewRelease(view);
    wgpuTextureRelease(target);
    setCurrentRenderer(nullptr);
    std::printf("PASS: V5 fused overlap physics + post-V5 null-participant stability tribunal\n");
    return 0;
}
