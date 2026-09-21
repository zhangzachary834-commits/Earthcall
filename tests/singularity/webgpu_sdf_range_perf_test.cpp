// Native A/B measurement for the spatial-Prophetic SDF range hierarchy.
//
// This is a measurement witness, not a fixed performance gate. It renders the
// authored Perlin Noise Floor expression at the same 2880x1800 resolution and
// the same horizon / 45-degree cameras used by the maintained native camera
// corpus. OFF and ON run in the same process/device so machine drift mostly
// divides out in the ratio.
//
// Correctness remains guarded elsewhere by webgpu_perlin_exact_gradient_test:
// this file refuses only if the accelerator never activates or if supposedly
// persistent proof bytes keep uploading after warm-up.

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SdfRangeProof.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace {

std::unique_ptr<OntoMath::MathNode> number(double value) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ScalarLeaf;
    n->scalarForm.terms.push_back(OntoMath::Term(value));
    return n;
}

std::unique_ptr<OntoMath::MathNode> variable(const std::string& name) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ValueLeaf;
    n->variableName = name;
    return n;
}

std::unique_ptr<OntoMath::MathNode> vec3Node(double x, double y, double z) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::VectorConstruct;
    n->children.push_back(number(x));
    n->children.push_back(number(y));
    n->children.push_back(number(z));
    return n;
}

// Verbatim authored terrain:
//   y - 40 * noise(0.008 * (p + vec3(100, 0, 100)))
std::shared_ptr<OntoMath::MathNode> buildTerrainMath() {
    auto shifted = std::make_unique<OntoMath::MathNode>();
    shifted->op = OntoMath::MathNode::Op::Add;
    shifted->children.push_back(variable(OntoMath::kAmbientPointVar));
    shifted->children.push_back(vec3Node(100.0, 0.0, 100.0));

    auto scaledArg = std::make_unique<OntoMath::MathNode>();
    scaledArg->op = OntoMath::MathNode::Op::Scale;
    scaledArg->children.push_back(number(0.008));
    scaledArg->children.push_back(std::move(shifted));

    auto noise = std::make_unique<OntoMath::MathNode>();
    noise->op = OntoMath::MathNode::Op::Noise;
    noise->children.push_back(std::move(scaledArg));

    auto scaledNoise = std::make_unique<OntoMath::MathNode>();
    scaledNoise->op = OntoMath::MathNode::Op::Scale;
    scaledNoise->children.push_back(number(40.0));
    scaledNoise->children.push_back(std::move(noise));

    auto field = std::make_shared<OntoMath::MathNode>();
    field->op = OntoMath::MathNode::Op::Sub;
    field->children.push_back(variable("y"));
    field->children.push_back(std::move(scaledNoise));
    return field;
}

double median(std::vector<double> values) {
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    const size_t n = values.size();
    if ((n & 1u) != 0u) return values[n / 2];
    return 0.5 * (values[n / 2 - 1] + values[n / 2]);
}

struct Sample {
    double wallMs = 0.0;
    Renderer::FrameStats stats;
};

struct Arm {
    std::vector<double> wallMs;
    std::vector<double> gpuMs;
    size_t recurringRangeUploadBytes = 0;
    uint32_t traversalDraws = 0;
};

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    constexpr uint32_t W = 2880;
    constexpr uint32_t H = 1800;
    constexpr int kWarmupFrames = 6;
    constexpr int kSampleFrames = 9;
    constexpr uint64_t kMemoId = 0x5045524c494e5046ULL; // "PERLINPF"

    wgpu::Device gpu;
    if (!gpu.init()) {
        std::printf("SDF_RANGE_PERF FAIL no WebGPU device\n");
        return 1;
    }

    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) {
        std::printf("SDF_RANGE_PERF FAIL renderer init\n");
        return 1;
    }
    setCurrentRenderer(&renderer);

    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment;
    td.dimension = WGPUTextureDimension_2D;
    td.size = {W, H, 1};
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1;
    td.sampleCount = 1;
    WGPUTexture tex = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView target = wgpuTextureCreateView(tex, nullptr);
    if (!tex || !target) {
        std::printf("SDF_RANGE_PERF FAIL offscreen target\n");
        return 1;
    }

    geom::SdfNode field;
    field.op = geom::SdfOp::Leaf;
    field.prim = geom::SdfPrim::Expr;
    field.mathNode = buildTerrainMath();

    const glm::vec3 extent(1000.0f, 30.0f, 1000.0f);

    // Release-mode latch diagnostics. These are the exact non-camera-dependent
    // conditions required before drawImplicit can advertise traversal.
    const sdfwgsl::Program probeProgram = sdfwgsl::compile(field);
    if (!probeProgram.ok) {
        std::printf("SDF_RANGE_PERF FAIL Release Perlin program refused: %s\n",
                    probeProgram.error.c_str());
        return 1;
    }
    const glm::vec3 proofExtent = glm::abs(extent * 1.05f);
    const auto proofHierarchy = geom::buildRangeHierarchy(
        field, proofExtent, /*maxDepth=*/6, /*maxNodes=*/327680);
    size_t positiveSkipNodes = 0;
    size_t negativeZeroFreeNodes = 0;
    std::array<size_t, 7> positiveByDepth{};
    std::array<size_t, 7> negativeByDepth{};
    std::array<size_t, 7> ambiguousLeafByDepth{};
    for (const auto& node : proofHierarchy.nodes) {
        const size_t depth =
            std::min<size_t>(node.depth, positiveByDepth.size() - 1u);
        if (geom::rangeNodeProvesPositiveOutside(node)) {
            ++positiveSkipNodes;
            ++positiveByDepth[depth];
        }
        if (node.boundFinite && node.rangeHi < 0.0f) {
            ++negativeZeroFreeNodes;
            ++negativeByDepth[depth];
        }
        if (node.childCount == 0u && node.boundFinite &&
            !node.provedNoZero) {
            ++ambiguousLeafByDepth[depth];
        }
    }
    std::printf(
        "SDF_RANGE_PERF_LATCH needsGradientStep=%d hierarchy_nodes=%zu "
        "proved_empty=%zu positive_skip_nodes=%zu negative_zero_free_nodes=%zu "
        "ambiguous_leaves=%zu unknown_leaves=%zu\n",
        probeProgram.needsGradientStep ? 1 : 0,
        proofHierarchy.nodes.size(),
        proofHierarchy.provedEmptyNodes,
        positiveSkipNodes,
        negativeZeroFreeNodes,
        proofHierarchy.ambiguousLeaves,
        proofHierarchy.unknownLeaves);
    std::printf(
        "SDF_RANGE_PERF_DEPTH positive=%zu,%zu,%zu,%zu,%zu,%zu,%zu "
        "negative=%zu,%zu,%zu,%zu,%zu,%zu,%zu "
        "ambiguous=%zu,%zu,%zu,%zu,%zu,%zu,%zu\n",
        positiveByDepth[0], positiveByDepth[1], positiveByDepth[2],
        positiveByDepth[3], positiveByDepth[4], positiveByDepth[5],
        positiveByDepth[6],
        negativeByDepth[0], negativeByDepth[1], negativeByDepth[2],
        negativeByDepth[3], negativeByDepth[4], negativeByDepth[5],
        negativeByDepth[6],
        ambiguousLeafByDepth[0], ambiguousLeafByDepth[1],
        ambiguousLeafByDepth[2], ambiguousLeafByDepth[3],
        ambiguousLeafByDepth[4], ambiguousLeafByDepth[5],
        ambiguousLeafByDepth[6]);

    // Measure the exact same derived proof semantics consumed by the renderer.
    // The selected renderer depth remains a profitability policy; the coalescing
    // theorem itself has one implementation shared by production and this witness.
    // Report neighboring depths too so one run reveals the coalescing cliff
    // without changing shader behavior between measurements.
    constexpr uint8_t gpuProofDepth = 4u;
    for (uint8_t proofDepth = 3u; proofDepth <= 6u; ++proofDepth) {
        const auto proofGrid =
            geom::derivePositiveRangeProofGrid(proofHierarchy, proofDepth);
        const size_t proofDim = size_t{1} << proofDepth;
        const size_t proofCells = proofDim * proofDim * proofDim;
        const size_t proofBytes = (proofCells + 7u) / 8u;
        std::printf(
            "SDF_RANGE_PERF_GPU_PROOF depth=%u positive_cells=%u total_cells=%zu "
            "proof_bytes=%zu selected=%d\n",
            static_cast<unsigned>(proofDepth),
            proofGrid.positiveCells,
            proofCells,
            proofBytes,
            proofDepth == gpuProofDepth ? 1 : 0);
    }

    if (!probeProgram.needsGradientStep || positiveSkipNodes == 0) {
        std::printf("SDF_RANGE_PERF FAIL Release traversal prerequisites are absent\n");
        return 1;
    }
    RenderMaterial mat;
    mat.baseColor = glm::vec3(0.2f, 0.8f, 0.2f);
    mat.ambient = 0.2f;
    mat.diffuse = 0.8f;

    struct CameraCase {
        const char* name;
        glm::vec3 eye;
        glm::vec3 target;
        glm::vec3 up;
        float fovDeg;
    };
    const CameraCase cameras[] = {
        {"horizon", {0.0f, 25.0f, -900.0f}, {0.0f, 20.0f, 900.0f},
                    {0.0f, 1.0f, 0.0f}, 60.0f},
        {"45deg",   {0.0f, 60.0f, -100.0f}, {0.0f, 0.0f, 0.0f},
                    {0.0f, 1.0f, 0.0f}, 60.0f},
    };

    bool sawTraversal = false;
    bool sawHierarchyBuild = false;
    bool measurementWarnings = false;

    auto renderOne = [&](bool enabled) -> Sample {
        renderer.setSdfRangeProxyEnabled(enabled);
        renderer.setModel(glm::mat4(1.0f));

        const auto t0 = std::chrono::steady_clock::now();
        renderer.beginFrameOffscreen(target, W, H, glm::vec4(0.1f, 0.1f, 0.15f, 1.0f));
        renderer.drawImplicit(field, extent, mat, nullptr,
                              kMemoId,
                              /*memoRevision=*/1,
                              nullptr,
                              /*memoParameterRevision=*/1);
        renderer.endFrame();
        // The benchmark deliberately waits here. That makes wallMs an honest
        // submitted-frame cost instead of merely measuring command recording.
        wgpuDevicePoll(gpu.device, true, nullptr);
        const auto t1 = std::chrono::steady_clock::now();

        Sample s;
        s.wallMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        s.stats = renderer.frameStats();
        return s;
    };

    auto runArm = [&](bool enabled) {
        Arm arm;

        for (int i = 0; i < kWarmupFrames; ++i) {
            const Sample s = renderOne(enabled);
            if (s.stats.sdfRangeTraversalDraws > 0) sawTraversal = true;
            if (s.stats.sdfRangeHierarchyBuilds > 0) sawHierarchyBuild = true;
        }

        for (int i = 0; i < kSampleFrames; ++i) {
            const Sample s = renderOne(enabled);
            arm.wallMs.push_back(s.wallMs);
            if (s.stats.gpuMainPassTimingValid) {
                arm.gpuMs.push_back(static_cast<double>(s.stats.gpuMainPassMs));
            }
            arm.recurringRangeUploadBytes += s.stats.sdfRangeNodeBytesUploaded;
            arm.traversalDraws += s.stats.sdfRangeTraversalDraws;
            if (s.stats.sdfRangeTraversalDraws > 0) sawTraversal = true;
            if (s.stats.sdfRangeHierarchyBuilds > 0) sawHierarchyBuild = true;
        }
        return arm;
    };

    for (const CameraCase& c : cameras) {
        const float aspect = static_cast<float>(W) / static_cast<float>(H);
        const glm::mat4 proj =
            glm::perspectiveZO(glm::radians(c.fovDeg), aspect, 0.1f, 3000.0f);
        const glm::mat4 view = glm::lookAt(c.eye, c.target, c.up);
        renderer.setCamera(view, proj, c.eye);

        const Arm off = runArm(false);
        const Arm on = runArm(true);

        const double offWall = median(off.wallMs);
        const double onWall = median(on.wallMs);
        const double wallRatio = offWall > 0.0 ? onWall / offWall : 0.0;
        const double offGpu = median(off.gpuMs);
        const double onGpu = median(on.gpuMs);
        const double gpuRatio = offGpu > 0.0 ? onGpu / offGpu : 0.0;

        std::printf(
            "SDF_RANGE_PERF view=%s resolution=%ux%u "
            "off_wall_median_ms=%.6f on_wall_median_ms=%.6f wall_ratio=%.4f "
            "off_gpu_median_ms=%.6f on_gpu_median_ms=%.6f gpu_ratio=%.4f "
            "timestamp_samples_off=%zu timestamp_samples_on=%zu "
            "traversal_draws=%u recurring_range_upload_bytes=%zu\n",
            c.name, W, H,
            offWall, onWall, wallRatio,
            offGpu, onGpu, gpuRatio,
            off.gpuMs.size(), on.gpuMs.size(),
            on.traversalDraws,
            on.recurringRangeUploadBytes);

        if (on.traversalDraws == 0) {
            std::printf("SDF_RANGE_PERF FAIL traversal did not activate for %s\n", c.name);
            measurementWarnings = true;
        }
        if (on.recurringRangeUploadBytes != 0) {
            std::printf(
                "SDF_RANGE_PERF FAIL persistent range nodes re-uploaded %zu bytes for %s\n",
                on.recurringRangeUploadBytes, c.name);
            measurementWarnings = true;
        }
    }

    if (!sawTraversal) {
        std::printf("SDF_RANGE_PERF FAIL no range traversal activation\n");
        measurementWarnings = true;
    }
    if (!sawHierarchyBuild) {
        std::printf("SDF_RANGE_PERF FAIL hierarchy never built\n");
        measurementWarnings = true;
    }

    setCurrentRenderer(nullptr);
    renderer.shutdown();
    wgpuTextureViewRelease(target);
    wgpuTextureRelease(tex);

    if (measurementWarnings) {
        std::printf("SDF_RANGE_PERF FAIL measurement instrumentation did not prove active traversal\n");
        std::fflush(stdout);
        std::_Exit(2);
    }
    std::printf("SDF_RANGE_PERF PASS active traversal measurement witness\n");
    std::fflush(stdout);
    std::_Exit(0);
}
