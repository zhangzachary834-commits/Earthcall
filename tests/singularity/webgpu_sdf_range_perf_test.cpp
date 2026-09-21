// Native A/B measurement for the spatial-Prophetic SDF range hierarchy.
//
// This is a measurement witness, not a fixed performance gate. It renders the
// authored Perlin Noise Floor expression at the same 2880x1800 resolution and
// the same horizon / 45-degree cameras used by the maintained native camera
// corpus. OFF and ON run in the same process/device in separate warmed renderer
// states, then interleave in alternating AB/BA pairs. Keeping the two renderer
// states persistent avoids measuring feature-toggle cache churn while ensuring
// clock/thermal drift cannot systematically make one mode the later arm.
// Historical ratio-of-medians output is retained alongside paired ratios/deltas
// for continuity and a tighter decision signal.
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
#include <limits>
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

// CPU-side opportunity census for the exact regular proof bitmap consumed by
// rangeCandidate(). This deliberately does NOT instrument the hot shader: the
// native AB/BA GPU timing below remains unpolluted. The census walks the same
// fixed-depth cells along a deterministic NDC ray sample and reports the
// geometric tax/opportunity envelope. It is not claimed to be an exact count of
// runtime rangeCandidate() calls because the authored marcher can step across
// more than one clear cell between consultations.
struct ProofGridCensus {
    uint64_t sampledRays = 0;
    uint64_t boxHitRays = 0;
    uint64_t classifications = 0;
    uint64_t positiveCells = 0;
    uint64_t clearCells = 0;
    uint64_t positiveRuns = 0;
    uint64_t upperCandidateCalls = 0;
    double traversedDistance = 0.0;
    double positiveDistance = 0.0;
    double maxPositiveRunDistance = 0.0;
};

bool rayBoxInterval(const glm::vec3& ro, const glm::vec3& rd,
                    const glm::vec3& extent, float& tEnter, float& tExit) {
    tEnter = -std::numeric_limits<float>::infinity();
    tExit = std::numeric_limits<float>::infinity();
    for (int axis = 0; axis < 3; ++axis) {
        const float e = std::abs(extent[axis]);
        if (std::abs(rd[axis]) < 1e-8f) {
            if (ro[axis] < -e || ro[axis] > e) return false;
            continue;
        }
        const float a = (-e - ro[axis]) / rd[axis];
        const float b = ( e - ro[axis]) / rd[axis];
        const float lo = std::min(a, b);
        const float hi = std::max(a, b);
        tEnter = std::max(tEnter, lo);
        tExit = std::min(tExit, hi);
        if (tExit < tEnter) return false;
    }
    return true;
}

uint32_t proofAxisIndex(float coord, float halfExtent,
                        float dir, uint32_t dim) {
    const float e = std::abs(halfExtent);
    const float denom = std::max(2.0f * e, 1e-8f);
    const float scaled = std::clamp(
        ((coord + e) / denom) * static_cast<float>(dim),
        0.0f, static_cast<float>(dim));
    const float floored = std::floor(scaled);
    uint32_t idx = static_cast<uint32_t>(
        std::min(floored, static_cast<float>(dim - 1u)));
    if (scaled == floored && dir < 0.0f && idx > 0u) --idx;
    return idx;
}

bool proofCellPositive(const geom::SdfPositiveProofGrid& grid,
                       uint32_t x, uint32_t y, uint32_t z) {
    const uint32_t linear = x + grid.dim * (y + grid.dim * z);
    const uint32_t word = linear >> 5u;
    if (word >= grid.words.size()) return false;
    return (grid.words[word] & (1u << (linear & 31u))) != 0u;
}

void censusRay(const geom::SdfPositiveProofGrid& grid,
               const glm::vec3& extent,
               const glm::vec3& ro,
               const glm::vec3& rd,
               float farField,
               ProofGridCensus& out) {
    ++out.sampledRays;
    if (grid.dim == 0u || grid.words.empty()) return;

    float boxEnter = 0.0f;
    float boxExit = 0.0f;
    if (!rayBoxInterval(ro, rd, extent, boxEnter, boxExit) ||
        boxExit < 0.0f) {
        return;
    }

    float t = std::max(boxEnter, 0.0f);
    const float maxDim = std::max({extent.x, extent.y, extent.z});
    const float tLimit =
        std::min({boxExit, t + maxDim * 8.0f, farField});
    if (!(tLimit > t)) return;
    ++out.boxHitRays;

    const glm::vec3 cellSize =
        (2.0f * glm::abs(extent)) / static_cast<float>(grid.dim);
    bool inPositiveRun = false;
    bool lastCellPositive = false;
    double positiveRunDistance = 0.0;
    uint64_t clearCellsThisRay = 0;

    // Depth 6 has at most 190 crossed regular cells for a straight ray through
    // a cube. Keep the shader's same 192-cell diagnostic horizon.
    for (int guard = 0; guard < 192 && t < tLimit; ++guard) {
        const glm::vec3 p = ro + rd * t;
        if (glm::any(glm::lessThan(p, -extent)) ||
            glm::any(glm::greaterThan(p, extent))) {
            break;
        }

        const uint32_t ix = proofAxisIndex(p.x, extent.x, rd.x, grid.dim);
        const uint32_t iy = proofAxisIndex(p.y, extent.y, rd.y, grid.dim);
        const uint32_t iz = proofAxisIndex(p.z, extent.z, rd.z, grid.dim);
        const bool positive = proofCellPositive(grid, ix, iy, iz);

        const glm::vec3 cellMin =
            -extent + glm::vec3(static_cast<float>(ix),
                                static_cast<float>(iy),
                                static_cast<float>(iz)) * cellSize;
        const glm::vec3 cellMax = cellMin + cellSize;
        glm::vec3 safeRd = rd;
        for (int axis = 0; axis < 3; ++axis) {
            if (std::abs(safeRd[axis]) < 1e-8f) safeRd[axis] = 1e-8f;
        }
        glm::vec3 exitFace;
        for (int axis = 0; axis < 3; ++axis) {
            exitFace[axis] =
                safeRd[axis] >= 0.0f ? cellMax[axis] : cellMin[axis];
        }
        const glm::vec3 axisExit = (exitFace - ro) / safeRd;
        const float cellExit =
            std::min({axisExit.x, axisExit.y, axisExit.z, tLimit});
        if (!(cellExit > t)) break;

        const double segment = static_cast<double>(cellExit - t);
        ++out.classifications;
        out.traversedDistance += segment;
        lastCellPositive = positive;

        if (positive) {
            ++out.positiveCells;
            out.positiveDistance += segment;
            if (!inPositiveRun) {
                inPositiveRun = true;
                positiveRunDistance = 0.0;
                ++out.positiveRuns;
            }
            positiveRunDistance += segment;
        } else {
            ++out.clearCells;
            ++clearCellsThisRay;
            if (inPositiveRun) {
                out.maxPositiveRunDistance =
                    std::max(out.maxPositiveRunDistance, positiveRunDistance);
                inPositiveRun = false;
                positiveRunDistance = 0.0;
            }
        }

        t = cellExit;
    }

    if (inPositiveRun) {
        out.maxPositiveRunDistance =
            std::max(out.maxPositiveRunDistance, positiveRunDistance);
    }

    // If the exact marcher reached every regular-cell boundary, each clear cell
    // would force a handoff; a terminal positive run needs one final candidate
    // call of its own. Real runtime calls can be lower because exact authored
    // steps may leap across clear-cell boundaries. This is therefore an explicit
    // upper envelope, not a mislabeled shader counter.
    out.upperCandidateCalls += clearCellsThisRay + (lastCellPositive ? 1u : 0u);
}

ProofGridCensus censusProofGrid(const geom::SdfPositiveProofGrid& grid,
                                const glm::vec3& extent,
                                const glm::vec3& eye,
                                const glm::mat4& view,
                                const glm::mat4& proj) {
    constexpr uint32_t sampleW = 160;
    constexpr uint32_t sampleH = 100;
    constexpr float farField = 3000.0f;
    ProofGridCensus out;
    const glm::mat4 invViewProj = glm::inverse(proj * view);

    for (uint32_t y = 0; y < sampleH; ++y) {
        for (uint32_t x = 0; x < sampleW; ++x) {
            const float sx =
                (static_cast<float>(x) + 0.5f) / static_cast<float>(sampleW);
            const float sy =
                (static_cast<float>(y) + 0.5f) / static_cast<float>(sampleH);
            const glm::vec4 ndc(
                sx * 2.0f - 1.0f,
                (1.0f - sy) * 2.0f - 1.0f,
                1.0f,
                1.0f);
            const glm::vec4 worldH = invViewProj * ndc;
            if (std::abs(worldH.w) < 1e-8f) {
                ++out.sampledRays;
                continue;
            }
            const glm::vec3 world =
                glm::vec3(worldH) / worldH.w;
            const glm::vec3 rd = glm::normalize(world - eye);
            censusRay(grid, extent, eye, rd, farField, out);
        }
    }
    return out;
}

void printProofGridCensus(const char* viewName,
                          const geom::SdfPositiveProofGrid& grid,
                          const ProofGridCensus& s) {
    const double classificationsPerHit =
        s.boxHitRays > 0
            ? static_cast<double>(s.classifications) /
                  static_cast<double>(s.boxHitRays)
            : 0.0;
    const double usefulClassificationRatio =
        s.classifications > 0
            ? static_cast<double>(s.positiveCells) /
                  static_cast<double>(s.classifications)
            : 0.0;
    const double positiveDistanceFraction =
        s.traversedDistance > 0.0
            ? s.positiveDistance / s.traversedDistance
            : 0.0;
    const double meanPositiveRunCells =
        s.positiveRuns > 0
            ? static_cast<double>(s.positiveCells) /
                  static_cast<double>(s.positiveRuns)
            : 0.0;
    const double meanPositiveRunDistance =
        s.positiveRuns > 0
            ? s.positiveDistance / static_cast<double>(s.positiveRuns)
            : 0.0;
    const double upperCallsPerHit =
        s.boxHitRays > 0
            ? static_cast<double>(s.upperCandidateCalls) /
                  static_cast<double>(s.boxHitRays)
            : 0.0;

    std::printf(
        "SDF_RANGE_TAX_GEOMETRY view=%s depth=%u sampled_rays=%llu "
        "box_hit_rays=%llu classifications=%llu positive_cells=%llu "
        "clear_cells=%llu positive_runs=%llu upper_candidate_calls=%llu "
        "classifications_per_hit=%.4f upper_calls_per_hit=%.4f "
        "useful_classification_ratio=%.6f positive_distance_fraction=%.6f "
        "mean_positive_run_cells=%.4f mean_positive_run_distance=%.6f "
        "max_positive_run_distance=%.6f\n",
        viewName,
        static_cast<unsigned>(grid.depth),
        static_cast<unsigned long long>(s.sampledRays),
        static_cast<unsigned long long>(s.boxHitRays),
        static_cast<unsigned long long>(s.classifications),
        static_cast<unsigned long long>(s.positiveCells),
        static_cast<unsigned long long>(s.clearCells),
        static_cast<unsigned long long>(s.positiveRuns),
        static_cast<unsigned long long>(s.upperCandidateCalls),
        classificationsPerHit,
        upperCallsPerHit,
        usefulClassificationRatio,
        positiveDistanceFraction,
        meanPositiveRunCells,
        meanPositiveRunDistance,
        s.maxPositiveRunDistance);
}

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    constexpr uint32_t W = 2880;
    constexpr uint32_t H = 1800;
    constexpr int kWarmupFrames = 6;
    constexpr int kSamplePairs = 10;
    constexpr uint64_t kMemoId = 0x5045524c494e5046ULL; // "PERLINPF"

    wgpu::Device gpu;
    if (!gpu.init()) {
        std::printf("SDF_RANGE_PERF FAIL no WebGPU device\n");
        return 1;
    }

    WebGpuRenderer offRenderer;
    WebGpuRenderer onRenderer;
    if (!offRenderer.init(gpu) || !onRenderer.init(gpu)) {
        std::printf("SDF_RANGE_PERF FAIL renderer init\n");
        return 1;
    }
    offRenderer.setSdfRangeProxyEnabled(false);
    onRenderer.setSdfRangeProxyEnabled(true);
    // Some shared helpers still expect a current renderer, but this benchmark
    // invokes both renderers directly. Point the global compatibility handle at
    // the accelerated renderer and never switch it as part of measurement.
    setCurrentRenderer(&onRenderer);

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
    // Renderer proof depth remains a production profitability policy; this witness
    // deliberately owns no duplicate selected-depth constant. Reporting the whole
    // neighboring ladder lets one run reveal the coalescing cliff without changing
    // shader behavior between measurements.
    std::array<geom::SdfPositiveProofGrid, 4> proofGrids;
    for (uint8_t proofDepth = 3u; proofDepth <= 6u; ++proofDepth) {
        auto& proofGrid = proofGrids[proofDepth - 3u];
        proofGrid =
            geom::derivePositiveRangeProofGrid(proofHierarchy, proofDepth);
        const size_t proofDim = size_t{1} << proofDepth;
        const size_t proofCells = proofDim * proofDim * proofDim;
        const size_t proofBytes = (proofCells + 7u) / 8u;
        std::printf(
            "SDF_RANGE_PERF_GPU_PROOF depth=%u positive_cells=%u total_cells=%zu "
            "proof_bytes=%zu\n",
            static_cast<unsigned>(proofDepth),
            proofGrid.positiveCells,
            proofCells,
            proofBytes);
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

    auto renderOne = [&](WebGpuRenderer& renderer) -> Sample {
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

    auto observeSample = [&](const Sample& s) {
        if (s.stats.sdfRangeTraversalDraws > 0) sawTraversal = true;
        if (s.stats.sdfRangeHierarchyBuilds > 0) sawHierarchyBuild = true;
    };

    auto warmupOne = [&](WebGpuRenderer& renderer) {
        const Sample s = renderOne(renderer);
        observeSample(s);
    };

    auto recordSample = [&](WebGpuRenderer& renderer, Arm& arm) {
        const Sample s = renderOne(renderer);
        arm.wallMs.push_back(s.wallMs);
        if (s.stats.gpuMainPassTimingValid) {
            arm.gpuMs.push_back(static_cast<double>(s.stats.gpuMainPassMs));
        }
        arm.recurringRangeUploadBytes += s.stats.sdfRangeNodeBytesUploaded;
        arm.traversalDraws += s.stats.sdfRangeTraversalDraws;
        observeSample(s);
        return s;
    };

    for (const CameraCase& c : cameras) {
        const float aspect = static_cast<float>(W) / static_cast<float>(H);
        const glm::mat4 proj =
            glm::perspectiveZO(glm::radians(c.fovDeg), aspect, 0.1f, 3000.0f);
        const glm::mat4 view = glm::lookAt(c.eye, c.target, c.up);
        offRenderer.setCamera(view, proj, c.eye);
        onRenderer.setCamera(view, proj, c.eye);

        // The geometric census is intentionally outside the timed GPU samples.
        // It measures the proof grid's opportunity/tax structure without
        // perturbing either renderer arm.
        for (const auto& proofGrid : proofGrids) {
            const ProofGridCensus census =
                censusProofGrid(proofGrid, proofExtent, c.eye, view, proj);
            printProofGridCensus(c.name, proofGrid, census);
        }

        Arm off;
        Arm on;

        // Preserve the historical warm-up cost (six frames per arm), but
        // alternate order so neither mode is always warmed later.
        for (int i = 0; i < kWarmupFrames; ++i) {
            const bool abOrder = (i % 2) == 0;
            if (abOrder) {
                warmupOne(offRenderer);
                warmupOne(onRenderer);
            } else {
                warmupOne(onRenderer);
                warmupOne(offRenderer);
            }
        }

        std::vector<double> pairedWallRatios;
        std::vector<double> pairedWallDeltas;
        std::vector<double> pairedGpuRatios;
        std::vector<double> pairedGpuDeltas;

        // Each pair contains exactly one OFF and one ON sample. Alternate AB
        // and BA order so a monotonic runner drift cannot systematically favor
        // either mode. Total sample count remains close to historical cost at ten frames per arm,
        // with exactly five AB and five BA pairs.
        for (int i = 0; i < kSamplePairs; ++i) {
            const bool abOrder = (i % 2) == 0;
            Sample offSample;
            Sample onSample;
            if (abOrder) {
                offSample = recordSample(offRenderer, off);
                onSample = recordSample(onRenderer, on);
            } else {
                onSample = recordSample(onRenderer, on);
                offSample = recordSample(offRenderer, off);
            }

            const double wallPairRatio =
                offSample.wallMs > 0.0 ? onSample.wallMs / offSample.wallMs : 0.0;
            const double wallPairDelta = onSample.wallMs - offSample.wallMs;
            pairedWallRatios.push_back(wallPairRatio);
            pairedWallDeltas.push_back(wallPairDelta);

            const bool gpuPairValid =
                offSample.stats.gpuMainPassTimingValid &&
                onSample.stats.gpuMainPassTimingValid;
            const double offGpuSample = gpuPairValid
                ? static_cast<double>(offSample.stats.gpuMainPassMs) : 0.0;
            const double onGpuSample = gpuPairValid
                ? static_cast<double>(onSample.stats.gpuMainPassMs) : 0.0;
            const double gpuPairRatio =
                gpuPairValid && offGpuSample > 0.0
                    ? onGpuSample / offGpuSample : 0.0;
            const double gpuPairDelta =
                gpuPairValid ? onGpuSample - offGpuSample : 0.0;
            if (gpuPairValid) {
                pairedGpuRatios.push_back(gpuPairRatio);
                pairedGpuDeltas.push_back(gpuPairDelta);
            }

            std::printf(
                "SDF_RANGE_PERF_PAIR view=%s pair=%d order=%s "
                "off_wall_ms=%.6f on_wall_ms=%.6f wall_ratio=%.4f "
                "wall_delta_ms=%.6f gpu_valid=%d off_gpu_ms=%.6f "
                "on_gpu_ms=%.6f gpu_ratio=%.4f gpu_delta_ms=%.6f\n",
                c.name, i,
                abOrder ? "AB" : "BA",
                offSample.wallMs, onSample.wallMs,
                wallPairRatio, wallPairDelta,
                gpuPairValid ? 1 : 0,
                offGpuSample, onGpuSample,
                gpuPairRatio, gpuPairDelta);
        }

        const double offWall = median(off.wallMs);
        const double onWall = median(on.wallMs);
        const double wallRatio = offWall > 0.0 ? onWall / offWall : 0.0;
        const double offGpu = median(off.gpuMs);
        const double onGpu = median(on.gpuMs);
        const double gpuRatio = offGpu > 0.0 ? onGpu / offGpu : 0.0;
        const double pairedWallRatio = median(pairedWallRatios);
        const double pairedWallDelta = median(pairedWallDeltas);
        const double pairedGpuRatio = median(pairedGpuRatios);
        const double pairedGpuDelta = median(pairedGpuDeltas);

        std::printf(
            "SDF_RANGE_PERF view=%s resolution=%ux%u "
            "off_wall_median_ms=%.6f on_wall_median_ms=%.6f wall_ratio=%.4f "
            "off_gpu_median_ms=%.6f on_gpu_median_ms=%.6f gpu_ratio=%.4f "
            "paired_wall_ratio_median=%.4f paired_wall_delta_median_ms=%.6f "
            "paired_gpu_ratio_median=%.4f paired_gpu_delta_median_ms=%.6f "
            "paired_gpu_samples=%zu "
            "timestamp_samples_off=%zu timestamp_samples_on=%zu "
            "traversal_draws=%u recurring_range_upload_bytes=%zu\n",
            c.name, W, H,
            offWall, onWall, wallRatio,
            offGpu, onGpu, gpuRatio,
            pairedWallRatio, pairedWallDelta,
            pairedGpuRatio, pairedGpuDelta,
            pairedGpuRatios.size(),
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
    onRenderer.shutdown();
    offRenderer.shutdown();
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
