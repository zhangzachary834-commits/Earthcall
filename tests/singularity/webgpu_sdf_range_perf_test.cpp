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

struct RuntimeTaxMapResult {
    bool done = false;
    bool ok = false;
};

void onRuntimeTaxMap(WGPUMapAsyncStatus status, WGPUStringView,
                     void* userdata1, void*) {
    auto* result = static_cast<RuntimeTaxMapResult*>(userdata1);
    result->ok = status == WGPUMapAsyncStatus_Success;
    result->done = true;
}

struct RuntimeTaxRay {
    glm::vec4 ro;
    glm::vec4 rdFar;
};

struct alignas(16) RuntimeTaxOut {
    // x/y/z/w = candidate calls / clear handoffs / useful skip calls /
    // exact gradient-sample calls.
    glm::uvec4 counts0{0u};
    // x/y/z/w = fallback sdfEval calls / hit / proof-to-end exhaustions /
    // marcher iterations.
    glm::uvec4 counts1{0u};
    // x = proof-authorized distance skipped.
    glm::vec4 distances{0.0f};
};

struct RuntimeTaxInstance {
    glm::mat4 model{1.0f};
    glm::mat4 invModel{1.0f};
    glm::vec4 baseColor{1.0f};
    glm::vec4 shading{1.0f};
    glm::vec4 extents{0.0f};
    glm::vec4 misc{0.0f};
    uint32_t paramOffset = 0;
    uint32_t heightGridOffset = 0;
    uint32_t heightGridDimX = 0;
    uint32_t heightGridDimZ = 0;
    uint32_t rangeProofWordOffset = 0;
    uint32_t rangeProofWordCount = 0;
    uint32_t rangeTraversalEnabled = 0;
    uint32_t rangeProofDepth = 0;
};

static_assert(sizeof(RuntimeTaxInstance) == 224,
              "diagnostic instance ABI must match SdfInstanceData");

struct RuntimeTaxTotals {
    uint64_t rays = 0;
    uint64_t candidateCalls = 0;
    uint64_t clearHandoffs = 0;
    uint64_t skipCalls = 0;
    uint64_t onSampleSteps = 0;
    uint64_t onFallbackEvals = 0;
    uint64_t onHits = 0;
    uint64_t candidateExhaustions = 0;
    uint64_t onIterations = 0;
    uint64_t offSampleSteps = 0;
    uint64_t offFallbackEvals = 0;
    uint64_t offHits = 0;
    uint64_t offIterations = 0;
    uint64_t perRayHitMismatches = 0;
    double skippedDistance = 0.0;
    bool valid = false;
};

bool proofCellPositive(const geom::SdfPositiveProofGrid& grid,
                       uint32_t x, uint32_t y, uint32_t z);

struct alignas(16) DirectProofRun {
    // xyz are the field-local proved-positive run bounds.
    glm::vec4 bmin{0.0f};
    glm::vec4 bmax{0.0f};
    // x/y/z = axis / first source proof-cell linear index / run cell count.
    // That is sufficient to recover the exact dependency slice for a future
    // incremental repair without storing the rich theorem twice.
    glm::uvec4 provenance{0u};
};

static_assert(sizeof(DirectProofRun) == 48,
              "direct proof-run ABI must match two vec4 + one uvec4");

struct DirectRunArtifact {
    uint32_t axis = 0;
    uint32_t minRunCells = 1;
    uint32_t coveredPositiveCells = 0;
    float minAxisWorld = 0.0f;
    std::vector<DirectProofRun> runs;
};

struct DirectRuntimeTax {
    uint32_t axis = 0;
    uint32_t minRunCells = 1;
    uint32_t coveredPositiveCells = 0;
    float minAxisWorld = 0.0f;
    size_t artifactRecords = 0;
    size_t artifactBytes = 0;
    uint64_t rays = 0;
    uint64_t artifactQueries = 0;
    uint64_t recordTests = 0;
    uint64_t skipCalls = 0;
    uint64_t directSampleSteps = 0;
    uint64_t directFallbackEvals = 0;
    uint64_t directHits = 0;
    uint64_t artifactExhaustions = 0;
    uint64_t directIterations = 0;
    uint64_t offSampleSteps = 0;
    uint64_t offFallbackEvals = 0;
    uint64_t offHits = 0;
    uint64_t offIterations = 0;
    uint64_t perRayHitMismatches = 0;
    double skippedDistance = 0.0;
    bool valid = false;
};


struct DispatchAtlasArtifact {
    uint32_t minRunCells = 1;
    uint32_t entryBins = 1;
    uint32_t slopeBins = 1;
    uint32_t routeWidth = 1;
    uint32_t populatedKeys = 0;
    uint32_t coveredPositiveCells = 0;
    bool representable = true;
    std::vector<DirectProofRun> runs;
    std::vector<uint32_t> table;
};

struct DispatchAtlasRuntimeTax {
    uint32_t minRunCells = 1;
    uint32_t entryBins = 1;
    uint32_t slopeBins = 1;
    uint32_t routeWidth = 1;
    uint32_t populatedKeys = 0;
    uint32_t coveredPositiveCells = 0;
    size_t atlasEntries = 0;
    size_t atlasBytes = 0;
    size_t runRecords = 0;
    size_t runBytes = 0;
    size_t totalArtifactBytes = 0;
    uint64_t rays = 0;
    uint64_t keyComputations = 0;
    uint64_t atlasLookups = 0;
    uint64_t nonEmptyDispatches = 0;
    uint64_t selectedRouteSlots = 0;
    uint64_t selectedRunTests = 0;
    uint64_t skipCalls = 0;
    uint64_t directSampleSteps = 0;
    uint64_t directFallbackEvals = 0;
    uint64_t directHits = 0;
    uint64_t directIterations = 0;
    uint64_t offSampleSteps = 0;
    uint64_t offFallbackEvals = 0;
    uint64_t offHits = 0;
    uint64_t offIterations = 0;
    uint64_t perRayHitMismatches = 0;
    double skippedDistance = 0.0;
    bool representable = true;
    bool valid = false;
};

struct alignas(16) DispatchAtlasConfig {
    // x/y/z/w = entry bins / slope bins / route width / atlas enabled.
    glm::uvec4 dims{1u, 1u, 1u, 0u};
};

struct alignas(16) DispatchAtlasTaxOut {
    // key computations / atlas lookups / non-empty dispatches / selected slots
    glm::uvec4 counts0{0u};
    // selected run tests / useful skips / sdfSampleStep calls / fallback evals
    glm::uvec4 counts1{0u};
    // hit / iterations / reserved / reserved
    glm::uvec4 counts2{0u};
    glm::vec4 distances{0.0f};
};

static_assert(sizeof(DispatchAtlasTaxOut) == 64,
              "dispatch atlas tax ABI must match four vec4 values");

const char* directAxisName(uint32_t axis) {
    return axis == 0u ? "x" : (axis == 1u ? "y" : "z");
}

DirectRunArtifact buildDirectRunArtifact(
    const geom::SdfPositiveProofGrid& grid,
    const glm::vec3& extent,
    uint32_t axis,
    uint32_t minRunCells) {
    DirectRunArtifact out;
    out.axis = std::min(axis, 2u);
    out.minRunCells = std::max(minRunCells, 1u);
    if (grid.dim == 0u || grid.words.empty()) return out;

    const glm::vec3 absExtent = glm::abs(extent);
    const glm::vec3 cellSize =
        (2.0f * absExtent) / static_cast<float>(grid.dim);
    out.minAxisWorld =
        static_cast<float>(out.minRunCells) * cellSize[out.axis];

    auto positive = [&](uint32_t k, uint32_t u, uint32_t v) {
        if (out.axis == 0u) return proofCellPositive(grid, k, u, v);
        if (out.axis == 1u) return proofCellPositive(grid, u, k, v);
        return proofCellPositive(grid, u, v, k);
    };

    auto emitRun = [&](uint32_t start, uint32_t end,
                       uint32_t u, uint32_t v) {
        const uint32_t runCells = end - start;
        if (runCells < out.minRunCells) return;

        glm::uvec3 lo(0u);
        glm::uvec3 hi(0u);
        if (out.axis == 0u) {
            lo = glm::uvec3(start, u, v);
            hi = glm::uvec3(end, u + 1u, v + 1u);
        } else if (out.axis == 1u) {
            lo = glm::uvec3(u, start, v);
            hi = glm::uvec3(u + 1u, end, v + 1u);
        } else {
            lo = glm::uvec3(u, v, start);
            hi = glm::uvec3(u + 1u, v + 1u, end);
        }

        const glm::vec3 loF(
            static_cast<float>(lo.x),
            static_cast<float>(lo.y),
            static_cast<float>(lo.z));
        const glm::vec3 hiF(
            static_cast<float>(hi.x),
            static_cast<float>(hi.y),
            static_cast<float>(hi.z));
        const glm::vec3 bmin = -absExtent + loF * cellSize;
        const glm::vec3 bmax = -absExtent + hiF * cellSize;
        const uint32_t sourceLinear =
            lo.x + grid.dim * (lo.y + grid.dim * lo.z);
        DirectProofRun run;
        run.bmin = glm::vec4(bmin, 0.0f);
        run.bmax = glm::vec4(bmax, 0.0f);
        run.provenance =
            glm::uvec4(out.axis, sourceLinear, runCells, 0u);
        out.runs.push_back(run);
        out.coveredPositiveCells += runCells;
    };

    // One chosen axis partitions the positive cells into disjoint maximal runs.
    // No cell is duplicated inside one artifact, and thresholding only removes
    // optimization opportunities; it never creates skip authority.
    for (uint32_t v = 0; v < grid.dim; ++v) {
        for (uint32_t u = 0; u < grid.dim; ++u) {
            bool inRun = false;
            uint32_t runStart = 0u;
            for (uint32_t k = 0; k <= grid.dim; ++k) {
                const bool isPositive =
                    k < grid.dim ? positive(k, u, v) : false;
                if (isPositive && !inRun) {
                    inRun = true;
                    runStart = k;
                } else if (!isPositive && inRun) {
                    emitRun(runStart, k, u, v);
                    inRun = false;
                }
            }
        }
    }
    return out;
}


bool cpuRayAabb(
    const glm::vec3& ro,
    const glm::vec3& rd,
    const glm::vec3& bmin,
    const glm::vec3& bmax,
    float& enter,
    float& exit) {
    enter = 0.0f;
    exit = std::numeric_limits<float>::infinity();
    for (uint32_t axis = 0u; axis < 3u; ++axis) {
        if (std::abs(rd[axis]) < 1e-8f) {
            if (ro[axis] < bmin[axis] || ro[axis] > bmax[axis]) {
                return false;
            }
            continue;
        }
        float t0 = (bmin[axis] - ro[axis]) / rd[axis];
        float t1 = (bmax[axis] - ro[axis]) / rd[axis];
        if (t0 > t1) std::swap(t0, t1);
        enter = std::max(enter, t0);
        exit = std::min(exit, t1);
        if (exit <= enter) return false;
    }
    return exit > enter;
}

DispatchAtlasArtifact buildDispatchAtlasArtifact(
    const geom::SdfPositiveProofGrid& grid,
    const glm::vec3& extent,
    uint32_t minRunCells,
    uint32_t entryBins,
    uint32_t slopeBins,
    uint32_t routeWidth) {
    DispatchAtlasArtifact out;
    out.minRunCells = std::max(minRunCells, 1u);
    out.entryBins = std::max(entryBins, 1u);
    out.slopeBins = std::max(slopeBins, 1u);
    out.routeWidth = std::min(std::max(routeWidth, 1u), 4u);

    // The rich theorem stays authoritative. The atlas reuses only derived
    // positive runs and never upgrades unknown/clear space into skip authority.
    for (uint32_t axis = 0u; axis < 3u; ++axis) {
        DirectRunArtifact axisArtifact =
            buildDirectRunArtifact(grid, extent, axis, out.minRunCells);
        out.coveredPositiveCells += axisArtifact.coveredPositiveCells;
        out.runs.insert(
            out.runs.end(),
            axisArtifact.runs.begin(),
            axisArtifact.runs.end());
    }

    const uint64_t entries64 =
        6ull *
        static_cast<uint64_t>(out.entryBins) *
        static_cast<uint64_t>(out.entryBins) *
        static_cast<uint64_t>(out.slopeBins) *
        static_cast<uint64_t>(out.slopeBins);
    if (entries64 == 0u ||
        entries64 > static_cast<uint64_t>(
            std::numeric_limits<uint32_t>::max())) {
        out.representable = false;
        return out;
    }
    out.table.assign(static_cast<size_t>(entries64), 0u);

    // One byte uses zero as the empty sentinel. Keep one value unused so this
    // first experiment obeys the architecture document's <=254-run cap.
    if (out.runs.size() > 254u) {
        out.representable = false;
        return out;
    }

    const glm::vec3 absExtent = glm::abs(extent);
    const uint32_t uAxis[3] = {1u, 0u, 0u};
    const uint32_t vAxis[3] = {2u, 2u, 1u};

    auto tableIndex = [&](uint32_t axis, uint32_t sign,
                          uint32_t entryU, uint32_t entryV,
                          uint32_t slopeU, uint32_t slopeV) -> size_t {
        size_t idx = static_cast<size_t>(axis * 2u + sign);
        idx = idx * out.entryBins + entryU;
        idx = idx * out.entryBins + entryV;
        idx = idx * out.slopeBins + slopeU;
        idx = idx * out.slopeBins + slopeV;
        return idx;
    };

    struct Hit {
        float enter = 0.0f;
        float exit = 0.0f;
        uint32_t runIndex = 0u;
    };

    for (uint32_t axis = 0u; axis < 3u; ++axis) {
        const uint32_t u = uAxis[axis];
        const uint32_t v = vAxis[axis];
        for (uint32_t sign = 0u; sign < 2u; ++sign) {
            for (uint32_t entryU = 0u; entryU < out.entryBins; ++entryU) {
                for (uint32_t entryV = 0u; entryV < out.entryBins; ++entryV) {
                    for (uint32_t slopeU = 0u; slopeU < out.slopeBins; ++slopeU) {
                        for (uint32_t slopeV = 0u; slopeV < out.slopeBins; ++slopeV) {
                            glm::vec3 ro(0.0f);
                            ro[axis] = sign == 0u ? -absExtent[axis]
                                                  : absExtent[axis];
                            ro[u] =
                                -absExtent[u] +
                                (static_cast<float>(entryU) + 0.5f) *
                                    (2.0f * absExtent[u] /
                                     static_cast<float>(out.entryBins));
                            ro[v] =
                                -absExtent[v] +
                                (static_cast<float>(entryV) + 0.5f) *
                                    (2.0f * absExtent[v] /
                                     static_cast<float>(out.entryBins));

                            glm::vec3 rd(0.0f);
                            rd[axis] = sign == 0u ? 1.0f : -1.0f;
                            rd[u] =
                                -1.0f +
                                (static_cast<float>(slopeU) + 0.5f) *
                                    (2.0f / static_cast<float>(out.slopeBins));
                            rd[v] =
                                -1.0f +
                                (static_cast<float>(slopeV) + 0.5f) *
                                    (2.0f / static_cast<float>(out.slopeBins));
                            rd = glm::normalize(rd);

                            std::vector<Hit> hits;
                            hits.reserve(out.runs.size());
                            for (uint32_t i = 0u;
                                 i < static_cast<uint32_t>(out.runs.size());
                                 ++i) {
                                const DirectProofRun& run = out.runs[i];
                                if (run.provenance.x != axis) continue;
                                float enter = 0.0f;
                                float exit = 0.0f;
                                if (!cpuRayAabb(
                                        ro, rd,
                                        glm::vec3(run.bmin),
                                        glm::vec3(run.bmax),
                                        enter, exit)) {
                                    continue;
                                }
                                if (exit <= std::max(enter, 0.0f)) continue;
                                hits.push_back({enter, exit, i});
                            }

                            std::sort(
                                hits.begin(), hits.end(),
                                [](const Hit& a, const Hit& b) {
                                    if (a.enter != b.enter) {
                                        return a.enter < b.enter;
                                    }
                                    if (a.exit != b.exit) {
                                        return a.exit > b.exit;
                                    }
                                    return a.runIndex < b.runIndex;
                                });

                            uint32_t packed = 0u;
                            const uint32_t retained =
                                std::min(
                                    out.routeWidth,
                                    static_cast<uint32_t>(hits.size()));
                            for (uint32_t slot = 0u; slot < retained; ++slot) {
                                const uint32_t encoded =
                                    hits[slot].runIndex + 1u;
                                packed |=
                                    (encoded & 0xffu) << (slot * 8u);
                            }
                            const size_t idx =
                                tableIndex(
                                    axis, sign, entryU, entryV,
                                    slopeU, slopeV);
                            out.table[idx] = packed;
                            if (packed != 0u) ++out.populatedKeys;
                        }
                    }
                }
            }
        }
    }

    return out;
}

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
    uint64_t raysWithPositiveProof = 0;
    uint64_t raysWithoutPositiveProof = 0;
    uint64_t positiveBoundsHitRays = 0;
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
               const glm::vec3& positiveBoundsMin,
               const glm::vec3& positiveBoundsMax,
               bool hasPositiveBounds,
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

    if (hasPositiveBounds) {
        const glm::vec3 positiveCenter =
            0.5f * (positiveBoundsMin + positiveBoundsMax);
        const glm::vec3 positiveExtent =
            0.5f * (positiveBoundsMax - positiveBoundsMin);
        float positiveEnter = 0.0f;
        float positiveExit = 0.0f;
        if (rayBoxInterval(ro - positiveCenter, rd, positiveExtent,
                           positiveEnter, positiveExit) &&
            positiveExit >= t && positiveEnter <= tLimit) {
            ++out.positiveBoundsHitRays;
        }
    }

    const glm::vec3 cellSize =
        (2.0f * glm::abs(extent)) / static_cast<float>(grid.dim);
    bool inPositiveRun = false;
    bool lastCellPositive = false;
    bool sawPositiveProof = false;
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
            sawPositiveProof = true;
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

    if (sawPositiveProof) {
        ++out.raysWithPositiveProof;
    } else {
        ++out.raysWithoutPositiveProof;
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

    glm::vec3 positiveBoundsMin(std::numeric_limits<float>::infinity());
    glm::vec3 positiveBoundsMax(-std::numeric_limits<float>::infinity());
    bool hasPositiveBounds = false;
    const glm::vec3 cellSize =
        (2.0f * glm::abs(extent)) / static_cast<float>(grid.dim);
    for (uint32_t z = 0; z < grid.dim; ++z) {
        for (uint32_t y = 0; y < grid.dim; ++y) {
            for (uint32_t x = 0; x < grid.dim; ++x) {
                if (!proofCellPositive(grid, x, y, z)) continue;
                const glm::vec3 cellMin =
                    -extent + glm::vec3(static_cast<float>(x),
                                        static_cast<float>(y),
                                        static_cast<float>(z)) * cellSize;
                positiveBoundsMin = glm::min(positiveBoundsMin, cellMin);
                positiveBoundsMax = glm::max(positiveBoundsMax, cellMin + cellSize);
                hasPositiveBounds = true;
            }
        }
    }

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
            censusRay(grid, extent,
                      positiveBoundsMin, positiveBoundsMax, hasPositiveBounds,
                      eye, rd, farField, out);
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
    const double usefulRayRatio =
        s.boxHitRays > 0
            ? static_cast<double>(s.raysWithPositiveProof) /
                  static_cast<double>(s.boxHitRays)
            : 0.0;
    const double positiveBoundsHitRatio =
        s.boxHitRays > 0
            ? static_cast<double>(s.positiveBoundsHitRays) /
                  static_cast<double>(s.boxHitRays)
            : 0.0;
    const double positiveBoundsPrecision =
        s.positiveBoundsHitRays > 0
            ? static_cast<double>(s.raysWithPositiveProof) /
                  static_cast<double>(s.positiveBoundsHitRays)
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
        "clear_cells=%llu positive_runs=%llu rays_with_positive=%llu "
        "rays_without_positive=%llu positive_bounds_hit_rays=%llu "
        "upper_candidate_calls=%llu classifications_per_hit=%.4f "
        "upper_calls_per_hit=%.4f useful_classification_ratio=%.6f "
        "positive_distance_fraction=%.6f useful_ray_ratio=%.6f "
        "positive_bounds_hit_ratio=%.6f positive_bounds_precision=%.6f "
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
        static_cast<unsigned long long>(s.raysWithPositiveProof),
        static_cast<unsigned long long>(s.raysWithoutPositiveProof),
        static_cast<unsigned long long>(s.positiveBoundsHitRays),
        static_cast<unsigned long long>(s.upperCandidateCalls),
        classificationsPerHit,
        upperCallsPerHit,
        usefulClassificationRatio,
        positiveDistanceFraction,
        usefulRayRatio,
        positiveBoundsHitRatio,
        positiveBoundsPrecision,
        meanPositiveRunCells,
        meanPositiveRunDistance,
        s.maxPositiveRunDistance);
}

RuntimeTaxTotals runRuntimeTaxDiagnostic(
    wgpu::Device& gpu,
    const sdfwgsl::Program& program,
    const geom::SdfPositiveProofGrid& proofGrid,
    const glm::vec3& extent,
    const glm::vec3& eye,
    const glm::mat4& view,
    const glm::mat4& proj) {
    constexpr uint32_t sampleW = 160;
    constexpr uint32_t sampleH = 100;

    // Match WebGpuRenderer::flushSdfDraws(): derive the camera far plane from
    // the projection actually supplied instead of trusting a duplicated literal.
    float farField = 1e6f;
    {
        const glm::vec4 farPt =
            glm::inverse(proj) * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        if (std::abs(farPt.w) > 1e-9f) {
            const float d = -(farPt.z / farPt.w);
            if (std::isfinite(d) && d > 0.0f) farField = d;
        }
    }

    RuntimeTaxTotals totals;
    if (!program.ok || proofGrid.dim == 0u || proofGrid.words.empty()) {
        return totals;
    }

    std::vector<RuntimeTaxRay> rays;
    rays.reserve(static_cast<size_t>(sampleW) * sampleH);
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
            glm::vec3 rd(0.0f, 0.0f, 1.0f);
            if (std::abs(worldH.w) >= 1e-8f) {
                const glm::vec3 world = glm::vec3(worldH) / worldH.w;
                rd = glm::normalize(world - eye);
            }
            rays.push_back({glm::vec4(eye, 1.0f), glm::vec4(rd, farField)});
        }
    }

    // Append a compute-only diagnostic entry point to the exact generated
    // production WGSL. The helper calls production rangeCandidate() itself;
    // it does not carry a second C++/WGSL transcription of proof traversal.
    // ON and OFF march the same GPU ray independently, so the counters expose
    // consultation tax without perturbing the timed renderer A/B below.
    const char* diagnosticWgsl = R"WGSL(
struct RuntimeTaxRay {
    ro: vec4<f32>,
    rdFar: vec4<f32>,
};
struct RuntimeTaxOut {
    counts0: vec4<u32>,
    counts1: vec4<u32>,
    distances: vec4<f32>,
};
@group(2) @binding(0) var<storage, read> runtimeTaxRays: array<RuntimeTaxRay>;
@group(2) @binding(1) var<storage, read_write> runtimeTaxOut: array<RuntimeTaxOut>;

fn runtimeTaxMarch(ray: RuntimeTaxRay, useRange: bool) -> RuntimeTaxOut {
    var out: RuntimeTaxOut;
    g_instIdx = 0u;
    let inst = instances[0u];
    let ro = ray.ro.xyz;
    let rd = normalize(ray.rdFar.xyz);
    let box = rayAabb(ro, rd, inst.extents.xyz);
    if (box.y < box.x || box.y < 0.0) {
        return out;
    }

    var t = max(box.x, 0.0);
    let maxDist = min(min(box.y, t + inst.misc.z), ray.rdFar.w);
    var rangeCellExit = t;
    var rangeCandidateActive = false;

    var candidateCalls = 0u;
    var clearHandoffs = 0u;
    var skipCalls = 0u;
    var sampleSteps = 0u;
    var fallbackEvals = 0u;
    var hit = false;
    var candidateExhaustions = 0u;
    var iterations = 0u;
    var skippedDistance = 0.0;

    var prev_d = 1e10;
    var candidate_step = 0.0;

    for (var i = 0; i < 192; i = i + 1) {
        if (t > maxDist) { break; }
        iterations = iterations + 1u;

        if (useRange && inst.rangeTraversalEnabled != 0u &&
            (!rangeCandidateActive || t >= rangeCellExit)) {
            candidateCalls = candidateCalls + 1u;
            let oldT = t;
            let candidate = rangeCandidate(inst, ro, rd, t, maxDist);
            if (candidate.z < 0.5) {
                candidateExhaustions = candidateExhaustions + 1u;
                let exhaustedDistance = max(maxDist - oldT, 0.0);
                if (exhaustedDistance > 0.0) {
                    skipCalls = skipCalls + 1u;
                    skippedDistance = skippedDistance + exhaustedDistance;
                }
                t = maxDist + 1.0;
                break;
            }

            // A non-exhausting rangeCandidate return hands an interval back to
            // the exact marcher. If candidate.x advanced, the same consultation
            // first consumed one or more positive-proof cells.
            clearHandoffs = clearHandoffs + 1u;
            t = max(t, candidate.x);
            if (t > oldT) {
                skipCalls = skipCalls + 1u;
                skippedDistance = skippedDistance + (t - oldT);
                prev_d = 1e10;
                candidate_step = 0.0;
            }
            rangeCellExit = max(t, candidate.y);
            rangeCandidateActive = true;
            if (t > maxDist) { break; }
        }

        let p = ro + rd * t;
        let current_eps = max(inst.misc.y, t * 0.001);
        let sample = sdfSampleStep(p);
        sampleSteps = sampleSteps + 1u;
        let raw = sample.raw;
        var gl = sample.gradLen;
        if (gl <= 1e-6) {
            let ge = 1e-3;
            let g = vec3<f32>(
                sdfEval(p + vec3<f32>(ge, 0.0, 0.0)) - raw,
                sdfEval(p + vec3<f32>(0.0, ge, 0.0)) - raw,
                sdfEval(p + vec3<f32>(0.0, 0.0, ge)) - raw) / ge;
            fallbackEvals = fallbackEvals + 3u;
            gl = length(g);
        }
        let d = select(raw, raw / gl, gl > 1e-6);

        if (d <= 0.0 || abs(d) < current_eps) {
            hit = true;
            if (d < 0.0 && prev_d > 0.0 && candidate_step > 0.0) {
                let frac = clamp(prev_d / (prev_d - d), 0.0, 1.0);
                t = (t - candidate_step) + candidate_step * frac;
            }
            break;
        }

        candidate_step = max(d, current_eps);
        prev_d = d;
        t = t + candidate_step;
    }

    out.counts0 =
        vec4<u32>(candidateCalls, clearHandoffs, skipCalls, sampleSteps);
    out.counts1 =
        vec4<u32>(fallbackEvals, select(0u, 1u, hit),
                  candidateExhaustions, iterations);
    out.distances = vec4<f32>(skippedDistance, 0.0, 0.0, 0.0);
    return out;
}

@compute @workgroup_size(64)
fn cs_runtime_tax(@builtin(global_invocation_id) gid: vec3<u32>) {
    let idx = gid.x;
    if (idx >= arrayLength(&runtimeTaxRays)) { return; }
    let ray = runtimeTaxRays[idx];
    runtimeTaxOut[idx * 2u] = runtimeTaxMarch(ray, true);
    runtimeTaxOut[idx * 2u + 1u] = runtimeTaxMarch(ray, false);
}
)WGSL";

    const std::string shaderCode = program.wgsl + diagnosticWgsl;

    WGPUShaderSourceWGSL wgslSrc = {};
    wgslSrc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslSrc.code = wgpu::Device::str(shaderCode.c_str());
    WGPUShaderModuleDescriptor smd = {};
    smd.nextInChain = &wgslSrc.chain;
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(gpu.device, &smd);
    if (!shader) {
        std::printf("SDF_RANGE_RUNTIME_TAX FAIL shader module\n");
        return totals;
    }

    WGPUComputePipelineDescriptor cpd = {};
    cpd.compute.module = shader;
    cpd.compute.entryPoint = wgpu::Device::str("cs_runtime_tax");
    WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(gpu.device, &cpd);
    if (!pipeline) {
        std::printf("SDF_RANGE_RUNTIME_TAX FAIL compute pipeline\n");
        wgpuShaderModuleRelease(shader);
        return totals;
    }

    RuntimeTaxInstance inst;
    inst.extents = glm::vec4(extent, 0.0f);
    inst.misc = glm::vec4(0.0f, 1e-4f, 8000.0f, 0.25f);
    inst.rangeProofWordOffset = 0u;
    inst.rangeProofWordCount = static_cast<uint32_t>(proofGrid.words.size());
    inst.rangeTraversalEnabled = 1u;
    inst.rangeProofDepth = proofGrid.depth;

    const size_t rayBytes = rays.size() * sizeof(RuntimeTaxRay);
    const size_t outCount = rays.size() * 2u;
    const size_t outBytes = outCount * sizeof(RuntimeTaxOut);
    const size_t paramBytes =
        std::max(program.params.size() * sizeof(float), sizeof(float));
    const size_t proofBytes =
        std::max(proofGrid.words.size() * sizeof(uint32_t), sizeof(uint32_t));

    auto makeBuffer = [&](uint64_t size, WGPUBufferUsage usage) {
        WGPUBufferDescriptor desc = {};
        desc.size = size;
        desc.usage = usage;
        return wgpuDeviceCreateBuffer(gpu.device, &desc);
    };

    WGPUBuffer rayBuffer = makeBuffer(
        rayBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    WGPUBuffer outBuffer = makeBuffer(
        outBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
    WGPUBuffer readback = makeBuffer(
        outBytes, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);
    WGPUBuffer paramBuffer = makeBuffer(
        paramBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    WGPUBuffer instBuffer = makeBuffer(
        sizeof(RuntimeTaxInstance),
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    WGPUBuffer proofBuffer = makeBuffer(
        proofBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);

    if (!rayBuffer || !outBuffer || !readback ||
        !paramBuffer || !instBuffer || !proofBuffer) {
        std::printf("SDF_RANGE_RUNTIME_TAX FAIL buffer allocation\n");
        if (proofBuffer) wgpuBufferRelease(proofBuffer);
        if (instBuffer) wgpuBufferRelease(instBuffer);
        if (paramBuffer) wgpuBufferRelease(paramBuffer);
        if (readback) wgpuBufferRelease(readback);
        if (outBuffer) wgpuBufferRelease(outBuffer);
        if (rayBuffer) wgpuBufferRelease(rayBuffer);
        wgpuComputePipelineRelease(pipeline);
        wgpuShaderModuleRelease(shader);
        return totals;
    }

    wgpuQueueWriteBuffer(gpu.queue, rayBuffer, 0, rays.data(), rayBytes);
    if (!program.params.empty()) {
        wgpuQueueWriteBuffer(
            gpu.queue, paramBuffer, 0, program.params.data(),
            program.params.size() * sizeof(float));
    } else {
        const float zero = 0.0f;
        wgpuQueueWriteBuffer(gpu.queue, paramBuffer, 0, &zero, sizeof(zero));
    }
    wgpuQueueWriteBuffer(
        gpu.queue, instBuffer, 0, &inst, sizeof(RuntimeTaxInstance));
    wgpuQueueWriteBuffer(
        gpu.queue, proofBuffer, 0, proofGrid.words.data(),
        proofGrid.words.size() * sizeof(uint32_t));

    WGPUBindGroupLayout bgl0 =
        wgpuComputePipelineGetBindGroupLayout(pipeline, 0);
    WGPUBindGroupLayout bgl1 =
        wgpuComputePipelineGetBindGroupLayout(pipeline, 1);
    WGPUBindGroupLayout bgl2 =
        wgpuComputePipelineGetBindGroupLayout(pipeline, 2);

    WGPUBindGroupEntry g0e = {};
    g0e.binding = 1;
    g0e.buffer = paramBuffer;
    g0e.offset = 0;
    g0e.size = paramBytes;
    WGPUBindGroupDescriptor g0d = {};
    g0d.layout = bgl0;
    g0d.entryCount = 1;
    g0d.entries = &g0e;
    WGPUBindGroup g0 = wgpuDeviceCreateBindGroup(gpu.device, &g0d);

    WGPUBindGroupEntry g1e[2] = {};
    g1e[0].binding = 0;
    g1e[0].buffer = instBuffer;
    g1e[0].offset = 0;
    g1e[0].size = sizeof(RuntimeTaxInstance);
    g1e[1].binding = 2;
    g1e[1].buffer = proofBuffer;
    g1e[1].offset = 0;
    g1e[1].size = proofBytes;
    WGPUBindGroupDescriptor g1d = {};
    g1d.layout = bgl1;
    g1d.entryCount = 2;
    g1d.entries = g1e;
    WGPUBindGroup g1 = wgpuDeviceCreateBindGroup(gpu.device, &g1d);

    WGPUBindGroupEntry g2e[2] = {};
    g2e[0].binding = 0;
    g2e[0].buffer = rayBuffer;
    g2e[0].offset = 0;
    g2e[0].size = rayBytes;
    g2e[1].binding = 1;
    g2e[1].buffer = outBuffer;
    g2e[1].offset = 0;
    g2e[1].size = outBytes;
    WGPUBindGroupDescriptor g2d = {};
    g2d.layout = bgl2;
    g2d.entryCount = 2;
    g2d.entries = g2e;
    WGPUBindGroup g2 = wgpuDeviceCreateBindGroup(gpu.device, &g2d);

    if (!g0 || !g1 || !g2) {
        std::printf("SDF_RANGE_RUNTIME_TAX FAIL bind group\n");
    } else {
        WGPUCommandEncoder encoder =
            wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUComputePassEncoder pass =
            wgpuCommandEncoderBeginComputePass(encoder, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, g0, 0, nullptr);
        wgpuComputePassEncoderSetBindGroup(pass, 1, g1, 0, nullptr);
        wgpuComputePassEncoderSetBindGroup(pass, 2, g2, 0, nullptr);
        const uint32_t workgroups =
            static_cast<uint32_t>((rays.size() + 63u) / 64u);
        wgpuComputePassEncoderDispatchWorkgroups(pass, workgroups, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);

        wgpuCommandEncoderCopyBufferToBuffer(
            encoder, outBuffer, 0, readback, 0, outBytes);
        WGPUCommandBuffer command =
            wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &command);
        wgpuCommandBufferRelease(command);
        wgpuCommandEncoderRelease(encoder);

        RuntimeTaxMapResult mapResult;
        WGPUBufferMapCallbackInfo mapInfo = {};
        mapInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        mapInfo.callback = onRuntimeTaxMap;
        mapInfo.userdata1 = &mapResult;
        wgpuBufferMapAsync(
            readback, WGPUMapMode_Read, 0, outBytes, mapInfo);
        while (!mapResult.done) {
            wgpuDevicePoll(gpu.device, true, nullptr);
        }

        if (mapResult.ok) {
            const auto* out = static_cast<const RuntimeTaxOut*>(
                wgpuBufferGetConstMappedRange(readback, 0, outBytes));
            if (out) {
                totals.rays = rays.size();
                for (size_t i = 0; i < rays.size(); ++i) {
                    const RuntimeTaxOut& on = out[i * 2u];
                    const RuntimeTaxOut& off = out[i * 2u + 1u];
                    totals.candidateCalls += on.counts0.x;
                    totals.clearHandoffs += on.counts0.y;
                    totals.skipCalls += on.counts0.z;
                    totals.onSampleSteps += on.counts0.w;
                    totals.onFallbackEvals += on.counts1.x;
                    totals.onHits += on.counts1.y;
                    totals.candidateExhaustions += on.counts1.z;
                    totals.onIterations += on.counts1.w;
                    totals.offSampleSteps += off.counts0.w;
                    totals.offFallbackEvals += off.counts1.x;
                    totals.offHits += off.counts1.y;
                    totals.offIterations += off.counts1.w;
                    if (on.counts1.y != off.counts1.y) {
                        ++totals.perRayHitMismatches;
                    }
                    totals.skippedDistance +=
                        static_cast<double>(on.distances.x);
                }
                totals.valid = true;
            }
            wgpuBufferUnmap(readback);
        } else {
            std::printf("SDF_RANGE_RUNTIME_TAX FAIL readback map\n");
        }
    }

    if (g2) wgpuBindGroupRelease(g2);
    if (g1) wgpuBindGroupRelease(g1);
    if (g0) wgpuBindGroupRelease(g0);
    wgpuBindGroupLayoutRelease(bgl2);
    wgpuBindGroupLayoutRelease(bgl1);
    wgpuBindGroupLayoutRelease(bgl0);
    wgpuBufferRelease(proofBuffer);
    wgpuBufferRelease(instBuffer);
    wgpuBufferRelease(paramBuffer);
    wgpuBufferRelease(readback);
    wgpuBufferRelease(outBuffer);
    wgpuBufferRelease(rayBuffer);
    wgpuComputePipelineRelease(pipeline);
    wgpuShaderModuleRelease(shader);
    return totals;
}

void printRuntimeTax(const char* viewName, const RuntimeTaxTotals& t) {
    if (!t.valid) {
        std::printf("SDF_RANGE_RUNTIME_TAX view=%s valid=0\n", viewName);
        return;
    }
    const double callsPerRay =
        t.rays > 0
            ? static_cast<double>(t.candidateCalls) /
                  static_cast<double>(t.rays)
            : 0.0;
    const double usefulCallRatio =
        t.candidateCalls > 0
            ? static_cast<double>(t.skipCalls) /
                  static_cast<double>(t.candidateCalls)
            : 0.0;
    const int64_t savedSampleSteps =
        static_cast<int64_t>(t.offSampleSteps) -
        static_cast<int64_t>(t.onSampleSteps);
    const double samplesSavedPerCall =
        t.candidateCalls > 0
            ? static_cast<double>(savedSampleSteps) /
                  static_cast<double>(t.candidateCalls)
            : 0.0;

    std::printf(
        "SDF_RANGE_RUNTIME_TAX view=%s valid=1 rays=%llu "
        "candidate_calls=%llu clear_handoffs=%llu useful_skip_calls=%llu "
        "candidate_exhaustions=%llu on_sample_steps=%llu off_sample_steps=%llu "
        "saved_sample_steps=%lld on_fallback_evals=%llu off_fallback_evals=%llu "
        "on_iterations=%llu off_iterations=%llu on_hits=%llu off_hits=%llu "
        "per_ray_hit_mismatches=%llu skipped_distance=%.6f "
        "calls_per_ray=%.6f useful_call_ratio=%.6f "
        "samples_saved_per_call=%.6f\n",
        viewName,
        static_cast<unsigned long long>(t.rays),
        static_cast<unsigned long long>(t.candidateCalls),
        static_cast<unsigned long long>(t.clearHandoffs),
        static_cast<unsigned long long>(t.skipCalls),
        static_cast<unsigned long long>(t.candidateExhaustions),
        static_cast<unsigned long long>(t.onSampleSteps),
        static_cast<unsigned long long>(t.offSampleSteps),
        static_cast<long long>(savedSampleSteps),
        static_cast<unsigned long long>(t.onFallbackEvals),
        static_cast<unsigned long long>(t.offFallbackEvals),
        static_cast<unsigned long long>(t.onIterations),
        static_cast<unsigned long long>(t.offIterations),
        static_cast<unsigned long long>(t.onHits),
        static_cast<unsigned long long>(t.offHits),
        static_cast<unsigned long long>(t.perRayHitMismatches),
        t.skippedDistance,
        callsPerRay,
        usefulCallRatio,
        samplesSavedPerCall);
}


std::vector<DirectRuntimeTax> runDirectArtifactDiagnostic(
    wgpu::Device& gpu,
    const sdfwgsl::Program& program,
    const std::vector<DirectRunArtifact>& artifacts,
    const RuntimeTaxTotals& genericBaseline,
    const glm::vec3& extent,
    const glm::vec3& eye,
    const glm::mat4& view,
    const glm::mat4& proj) {
    constexpr uint32_t sampleW = 160;
    constexpr uint32_t sampleH = 100;

    std::vector<DirectRuntimeTax> results;
    results.reserve(artifacts.size());
    if (!program.ok) return results;

    float farField = 1e6f;
    {
        const glm::vec4 farPt =
            glm::inverse(proj) * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        if (std::abs(farPt.w) > 1e-9f) {
            const float d = -(farPt.z / farPt.w);
            if (std::isfinite(d) && d > 0.0f) farField = d;
        }
    }

    std::vector<RuntimeTaxRay> rays;
    rays.reserve(static_cast<size_t>(sampleW) * sampleH);
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
            glm::vec3 rd(0.0f, 0.0f, 1.0f);
            if (std::abs(worldH.w) >= 1e-8f) {
                const glm::vec3 world = glm::vec3(worldH) / worldH.w;
                rd = glm::normalize(world - eye);
            }
            rays.push_back({glm::vec4(eye, 1.0f), glm::vec4(rd, farField)});
        }
    }

    const char* diagnosticWgsl = R"WGSL(
struct DirectTaxRay {
    ro: vec4<f32>,
    rdFar: vec4<f32>,
};
struct DirectProofRun {
    bmin: vec4<f32>,
    bmax: vec4<f32>,
    provenance: vec4<u32>,
};
struct DirectTaxOut {
    counts0: vec4<u32>,
    counts1: vec4<u32>,
    distances: vec4<f32>,
};
@group(2) @binding(0) var<storage, read> directTaxRays: array<DirectTaxRay>;
@group(2) @binding(1) var<storage, read_write> directTaxOut: array<DirectTaxOut>;
@group(2) @binding(2) var<storage, read> directRuns: array<DirectProofRun>;

fn findDirectRun(ro: vec3<f32>, rd: vec3<f32>,
                 t: f32, maxDist: f32) -> vec4<f32> {
    var found = false;
    var bestEnter = maxDist + 1.0;
    var bestExit = 0.0;
    var tests = 0u;
    let count = arrayLength(&directRuns);

    for (var i = 0u; i < count; i = i + 1u) {
        tests = tests + 1u;
        let run = directRuns[i];
        let center = 0.5 * (run.bmin.xyz + run.bmax.xyz);
        let halfExtent =
            max(0.5 * (run.bmax.xyz - run.bmin.xyz), vec3<f32>(1e-8));
        let interval = rayAabb(ro - center, rd, halfExtent);
        let enter = max(interval.x, t);
        let exit = min(interval.y, maxDist);
        if (exit <= enter) { continue; }

        if (!found || enter < bestEnter - 1e-6 ||
            (abs(enter - bestEnter) <= 1e-6 && exit > bestExit)) {
            found = true;
            bestEnter = enter;
            bestExit = exit;
        }
    }

    return vec4<f32>(
        bestEnter, bestExit, select(0.0, 1.0, found), f32(tests));
}

fn directTaxMarch(ray: DirectTaxRay, useDirect: bool) -> DirectTaxOut {
    var out: DirectTaxOut;
    g_instIdx = 0u;
    let inst = instances[0u];
    let ro = ray.ro.xyz;
    let rd = normalize(ray.rdFar.xyz);
    let box = rayAabb(ro, rd, inst.extents.xyz);
    if (box.y < box.x || box.y < 0.0) {
        return out;
    }

    var t = max(box.x, 0.0);
    let maxDist = min(min(box.y, t + inst.misc.z), ray.rdFar.w);

    var artifactQueries = 0u;
    var recordTests = 0u;
    var skipCalls = 0u;
    var sampleSteps = 0u;
    var fallbackEvals = 0u;
    var hit = false;
    var artifactExhaustions = 0u;
    var iterations = 0u;
    var skippedDistance = 0.0;

    var directValid = false;
    var directExhausted = !useDirect;
    var directEnter = 0.0;
    var directExit = 0.0;

    var prev_d = 1e10;
    var candidate_step = 0.0;

    for (var i = 0; i < 192; i = i + 1) {
        if (t > maxDist) { break; }
        iterations = iterations + 1u;

        if (useDirect && directValid && t >= directExit) {
            directValid = false;
        }

        // The direct artifact is discovered at most once per retained candidate
        // interval, not once per exact marcher step. A no-future-run result is
        // final for this monotonic ray and suppresses all later artifact work.
        if (useDirect && !directExhausted && !directValid) {
            artifactQueries = artifactQueries + 1u;
            let candidate = findDirectRun(ro, rd, t, maxDist);
            recordTests = recordTests + u32(candidate.w);
            if (candidate.z > 0.5) {
                directEnter = candidate.x;
                directExit = candidate.y;
                directValid = true;
            } else {
                artifactExhaustions = artifactExhaustions + 1u;
                directExhausted = true;
            }
        }

        if (useDirect && directValid &&
            t >= directEnter && t < directExit) {
            let oldT = t;
            t = directExit;
            directValid = false;
            if (t > oldT) {
                skipCalls = skipCalls + 1u;
                skippedDistance = skippedDistance + (t - oldT);
                prev_d = 1e10;
                candidate_step = 0.0;
            }
            if (t > maxDist) { break; }
        }

        let p = ro + rd * t;
        let current_eps = max(inst.misc.y, t * 0.001);
        let sample = sdfSampleStep(p);
        sampleSteps = sampleSteps + 1u;
        let raw = sample.raw;
        var gl = sample.gradLen;
        if (gl <= 1e-6) {
            let ge = 1e-3;
            let g = vec3<f32>(
                sdfEval(p + vec3<f32>(ge, 0.0, 0.0)) - raw,
                sdfEval(p + vec3<f32>(0.0, ge, 0.0)) - raw,
                sdfEval(p + vec3<f32>(0.0, 0.0, ge)) - raw) / ge;
            fallbackEvals = fallbackEvals + 3u;
            gl = length(g);
        }
        let d = select(raw, raw / gl, gl > 1e-6);

        if (d <= 0.0 || abs(d) < current_eps) {
            hit = true;
            if (d < 0.0 && prev_d > 0.0 && candidate_step > 0.0) {
                let frac = clamp(prev_d / (prev_d - d), 0.0, 1.0);
                t = (t - candidate_step) + candidate_step * frac;
            }
            break;
        }

        candidate_step = max(d, current_eps);
        prev_d = d;
        t = t + candidate_step;
    }

    out.counts0 =
        vec4<u32>(artifactQueries, recordTests, skipCalls, sampleSteps);
    out.counts1 =
        vec4<u32>(fallbackEvals, select(0u, 1u, hit),
                  artifactExhaustions, iterations);
    out.distances = vec4<f32>(skippedDistance, 0.0, 0.0, 0.0);
    return out;
}

@compute @workgroup_size(64)
fn cs_direct_tax(@builtin(global_invocation_id) gid: vec3<u32>) {
    let idx = gid.x;
    if (idx >= arrayLength(&directTaxRays)) { return; }
    let ray = directTaxRays[idx];
    directTaxOut[idx * 2u] = directTaxMarch(ray, true);
    directTaxOut[idx * 2u + 1u] = directTaxMarch(ray, false);
}
)WGSL";

    const std::string shaderCode = program.wgsl + diagnosticWgsl;
    WGPUShaderSourceWGSL wgslSrc = {};
    wgslSrc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslSrc.code = wgpu::Device::str(shaderCode.c_str());
    WGPUShaderModuleDescriptor smd = {};
    smd.nextInChain = &wgslSrc.chain;
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(gpu.device, &smd);
    if (!shader) return results;

    WGPUComputePipelineDescriptor cpd = {};
    cpd.compute.module = shader;
    cpd.compute.entryPoint = wgpu::Device::str("cs_direct_tax");
    WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(gpu.device, &cpd);
    if (!pipeline) {
        wgpuShaderModuleRelease(shader);
        return results;
    }

    RuntimeTaxInstance inst;
    inst.extents = glm::vec4(extent, 0.0f);
    inst.misc = glm::vec4(0.0f, 1e-4f, 8000.0f, 0.25f);

    const size_t rayBytes = rays.size() * sizeof(RuntimeTaxRay);
    const size_t outCount = rays.size() * 2u;
    const size_t outBytes = outCount * sizeof(RuntimeTaxOut);
    const size_t paramBytes =
        std::max(program.params.size() * sizeof(float), sizeof(float));

    size_t maxDirectBytes = sizeof(DirectProofRun);
    for (const auto& artifact : artifacts) {
        maxDirectBytes = std::max(
            maxDirectBytes,
            artifact.runs.size() * sizeof(DirectProofRun));
    }

    auto makeBuffer = [&](uint64_t size, WGPUBufferUsage usage) {
        WGPUBufferDescriptor desc = {};
        desc.size = size;
        desc.usage = usage;
        return wgpuDeviceCreateBuffer(gpu.device, &desc);
    };

    WGPUBuffer rayBuffer = makeBuffer(
        rayBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    WGPUBuffer outBuffer = makeBuffer(
        outBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
    WGPUBuffer readback = makeBuffer(
        outBytes, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);
    WGPUBuffer paramBuffer = makeBuffer(
        paramBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    WGPUBuffer instBuffer = makeBuffer(
        sizeof(RuntimeTaxInstance),
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    WGPUBuffer directBuffer = makeBuffer(
        maxDirectBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);

    if (!rayBuffer || !outBuffer || !readback ||
        !paramBuffer || !instBuffer || !directBuffer) {
        if (directBuffer) wgpuBufferRelease(directBuffer);
        if (instBuffer) wgpuBufferRelease(instBuffer);
        if (paramBuffer) wgpuBufferRelease(paramBuffer);
        if (readback) wgpuBufferRelease(readback);
        if (outBuffer) wgpuBufferRelease(outBuffer);
        if (rayBuffer) wgpuBufferRelease(rayBuffer);
        wgpuComputePipelineRelease(pipeline);
        wgpuShaderModuleRelease(shader);
        return results;
    }

    wgpuQueueWriteBuffer(gpu.queue, rayBuffer, 0, rays.data(), rayBytes);
    if (!program.params.empty()) {
        wgpuQueueWriteBuffer(
            gpu.queue, paramBuffer, 0, program.params.data(),
            program.params.size() * sizeof(float));
    } else {
        const float zero = 0.0f;
        wgpuQueueWriteBuffer(gpu.queue, paramBuffer, 0, &zero, sizeof(zero));
    }
    wgpuQueueWriteBuffer(
        gpu.queue, instBuffer, 0, &inst, sizeof(RuntimeTaxInstance));

    WGPUBindGroupLayout bgl0 =
        wgpuComputePipelineGetBindGroupLayout(pipeline, 0);
    WGPUBindGroupLayout bgl1 =
        wgpuComputePipelineGetBindGroupLayout(pipeline, 1);
    WGPUBindGroupLayout bgl2 =
        wgpuComputePipelineGetBindGroupLayout(pipeline, 2);

    WGPUBindGroupEntry g0e = {};
    g0e.binding = 1;
    g0e.buffer = paramBuffer;
    g0e.offset = 0;
    g0e.size = paramBytes;
    WGPUBindGroupDescriptor g0d = {};
    g0d.layout = bgl0;
    g0d.entryCount = 1;
    g0d.entries = &g0e;
    WGPUBindGroup g0 = wgpuDeviceCreateBindGroup(gpu.device, &g0d);

    WGPUBindGroupEntry g1e = {};
    g1e.binding = 0;
    g1e.buffer = instBuffer;
    g1e.offset = 0;
    g1e.size = sizeof(RuntimeTaxInstance);
    WGPUBindGroupDescriptor g1d = {};
    g1d.layout = bgl1;
    g1d.entryCount = 1;
    g1d.entries = &g1e;
    WGPUBindGroup g1 = wgpuDeviceCreateBindGroup(gpu.device, &g1d);

    if (!g0 || !g1) {
        if (g1) wgpuBindGroupRelease(g1);
        if (g0) wgpuBindGroupRelease(g0);
        wgpuBindGroupLayoutRelease(bgl2);
        wgpuBindGroupLayoutRelease(bgl1);
        wgpuBindGroupLayoutRelease(bgl0);
        wgpuBufferRelease(directBuffer);
        wgpuBufferRelease(instBuffer);
        wgpuBufferRelease(paramBuffer);
        wgpuBufferRelease(readback);
        wgpuBufferRelease(outBuffer);
        wgpuBufferRelease(rayBuffer);
        wgpuComputePipelineRelease(pipeline);
        wgpuShaderModuleRelease(shader);
        return results;
    }

    for (const auto& artifact : artifacts) {
        DirectRuntimeTax totals;
        totals.axis = artifact.axis;
        totals.minRunCells = artifact.minRunCells;
        totals.coveredPositiveCells = artifact.coveredPositiveCells;
        totals.minAxisWorld = artifact.minAxisWorld;
        totals.artifactRecords = artifact.runs.size();
        totals.artifactBytes = artifact.runs.size() * sizeof(DirectProofRun);
        totals.rays = rays.size();

        if (artifact.runs.empty()) {
            totals.directSampleSteps = genericBaseline.offSampleSteps;
            totals.offSampleSteps = genericBaseline.offSampleSteps;
            totals.directFallbackEvals = genericBaseline.offFallbackEvals;
            totals.offFallbackEvals = genericBaseline.offFallbackEvals;
            totals.directHits = genericBaseline.offHits;
            totals.offHits = genericBaseline.offHits;
            totals.directIterations = genericBaseline.offIterations;
            totals.offIterations = genericBaseline.offIterations;
            totals.valid = genericBaseline.valid;
            results.push_back(totals);
            continue;
        }

        const size_t directBytes =
            artifact.runs.size() * sizeof(DirectProofRun);
        wgpuQueueWriteBuffer(
            gpu.queue, directBuffer, 0, artifact.runs.data(), directBytes);

        WGPUBindGroupEntry g2e[3] = {};
        g2e[0].binding = 0;
        g2e[0].buffer = rayBuffer;
        g2e[0].offset = 0;
        g2e[0].size = rayBytes;
        g2e[1].binding = 1;
        g2e[1].buffer = outBuffer;
        g2e[1].offset = 0;
        g2e[1].size = outBytes;
        g2e[2].binding = 2;
        g2e[2].buffer = directBuffer;
        g2e[2].offset = 0;
        g2e[2].size = directBytes;
        WGPUBindGroupDescriptor g2d = {};
        g2d.layout = bgl2;
        g2d.entryCount = 3;
        g2d.entries = g2e;
        WGPUBindGroup g2 =
            wgpuDeviceCreateBindGroup(gpu.device, &g2d);
        if (!g2) {
            results.push_back(totals);
            continue;
        }

        WGPUCommandEncoder encoder =
            wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUComputePassEncoder pass =
            wgpuCommandEncoderBeginComputePass(encoder, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, g0, 0, nullptr);
        wgpuComputePassEncoderSetBindGroup(pass, 1, g1, 0, nullptr);
        wgpuComputePassEncoderSetBindGroup(pass, 2, g2, 0, nullptr);
        const uint32_t workgroups =
            static_cast<uint32_t>((rays.size() + 63u) / 64u);
        wgpuComputePassEncoderDispatchWorkgroups(pass, workgroups, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        wgpuCommandEncoderCopyBufferToBuffer(
            encoder, outBuffer, 0, readback, 0, outBytes);
        WGPUCommandBuffer command =
            wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &command);
        wgpuCommandBufferRelease(command);
        wgpuCommandEncoderRelease(encoder);

        RuntimeTaxMapResult mapResult;
        WGPUBufferMapCallbackInfo mapInfo = {};
        mapInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        mapInfo.callback = onRuntimeTaxMap;
        mapInfo.userdata1 = &mapResult;
        wgpuBufferMapAsync(
            readback, WGPUMapMode_Read, 0, outBytes, mapInfo);
        while (!mapResult.done) {
            wgpuDevicePoll(gpu.device, true, nullptr);
        }

        if (mapResult.ok) {
            const auto* out = static_cast<const RuntimeTaxOut*>(
                wgpuBufferGetConstMappedRange(readback, 0, outBytes));
            if (out) {
                for (size_t i = 0; i < rays.size(); ++i) {
                    const RuntimeTaxOut& direct = out[i * 2u];
                    const RuntimeTaxOut& off = out[i * 2u + 1u];
                    totals.artifactQueries += direct.counts0.x;
                    totals.recordTests += direct.counts0.y;
                    totals.skipCalls += direct.counts0.z;
                    totals.directSampleSteps += direct.counts0.w;
                    totals.directFallbackEvals += direct.counts1.x;
                    totals.directHits += direct.counts1.y;
                    totals.artifactExhaustions += direct.counts1.z;
                    totals.directIterations += direct.counts1.w;
                    totals.offSampleSteps += off.counts0.w;
                    totals.offFallbackEvals += off.counts1.x;
                    totals.offHits += off.counts1.y;
                    totals.offIterations += off.counts1.w;
                    if (direct.counts1.y != off.counts1.y) {
                        ++totals.perRayHitMismatches;
                    }
                    totals.skippedDistance +=
                        static_cast<double>(direct.distances.x);
                }
                totals.valid = true;
            }
            wgpuBufferUnmap(readback);
        }

        wgpuBindGroupRelease(g2);
        results.push_back(totals);
    }

    wgpuBindGroupRelease(g1);
    wgpuBindGroupRelease(g0);
    wgpuBindGroupLayoutRelease(bgl2);
    wgpuBindGroupLayoutRelease(bgl1);
    wgpuBindGroupLayoutRelease(bgl0);
    wgpuBufferRelease(directBuffer);
    wgpuBufferRelease(instBuffer);
    wgpuBufferRelease(paramBuffer);
    wgpuBufferRelease(readback);
    wgpuBufferRelease(outBuffer);
    wgpuBufferRelease(rayBuffer);
    wgpuComputePipelineRelease(pipeline);
    wgpuShaderModuleRelease(shader);
    return results;
}


std::vector<DispatchAtlasRuntimeTax> runDispatchAtlasDiagnostic(
    wgpu::Device& gpu,
    const sdfwgsl::Program& program,
    const std::vector<DispatchAtlasArtifact>& artifacts,
    const RuntimeTaxTotals& genericBaseline,
    const glm::vec3& extent,
    const glm::vec3& eye,
    const glm::mat4& view,
    const glm::mat4& proj) {
    constexpr uint32_t sampleW = 160;
    constexpr uint32_t sampleH = 100;

    std::vector<DispatchAtlasRuntimeTax> results;
    results.reserve(artifacts.size());
    if (!program.ok) return results;

    float farField = 1e6f;
    {
        const glm::vec4 farPt =
            glm::inverse(proj) * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        if (std::abs(farPt.w) > 1e-9f) {
            const float d = -(farPt.z / farPt.w);
            if (std::isfinite(d) && d > 0.0f) farField = d;
        }
    }

    std::vector<RuntimeTaxRay> rays;
    rays.reserve(static_cast<size_t>(sampleW) * sampleH);
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
            glm::vec3 rd(0.0f, 0.0f, 1.0f);
            if (std::abs(worldH.w) >= 1e-8f) {
                const glm::vec3 world = glm::vec3(worldH) / worldH.w;
                rd = glm::normalize(world - eye);
            }
            rays.push_back({glm::vec4(eye, 1.0f), glm::vec4(rd, farField)});
        }
    }

    const char* diagnosticWgsl = R"WGSL(
struct AtlasTaxRay {
    ro: vec4<f32>,
    rdFar: vec4<f32>,
};
struct DirectProofRun {
    bmin: vec4<f32>,
    bmax: vec4<f32>,
    provenance: vec4<u32>,
};
struct AtlasConfig {
    dims: vec4<u32>,
};
struct AtlasTaxOut {
    counts0: vec4<u32>,
    counts1: vec4<u32>,
    counts2: vec4<u32>,
    distances: vec4<f32>,
};
@group(2) @binding(0) var<storage, read> atlasTaxRays: array<AtlasTaxRay>;
@group(2) @binding(1) var<storage, read_write> atlasTaxOut: array<AtlasTaxOut>;
@group(2) @binding(2) var<storage, read> atlasRuns: array<DirectProofRun>;
@group(2) @binding(3) var<storage, read> dispatchAtlas: array<u32>;
@group(2) @binding(4) var<storage, read> atlasConfig: AtlasConfig;

fn transverse(v: vec3<f32>, axis: u32) -> vec2<f32> {
    if (axis == 0u) { return v.yz; }
    if (axis == 1u) { return vec2<f32>(v.x, v.z); }
    return v.xy;
}

fn dominantAxis(rd: vec3<f32>) -> u32 {
    let a = abs(rd);
    if (a.x >= a.y && a.x >= a.z) { return 0u; }
    if (a.y >= a.z) { return 1u; }
    return 2u;
}

fn quantize01(x: f32, bins: u32) -> u32 {
    if (bins <= 1u) { return 0u; }
    let q = clamp(x, 0.0, 0.99999994);
    return min(u32(q * f32(bins)), bins - 1u);
}

fn dispatchKey(
    pEntry: vec3<f32>,
    rd: vec3<f32>,
    extent: vec3<f32>,
    entryBins: u32,
    slopeBins: u32) -> u32 {
    let axis = dominantAxis(rd);
    let sign = select(0u, 1u, rd[axis] < 0.0);
    let entryUV = transverse(pEntry, axis);
    let extentUV = max(transverse(extent, axis), vec2<f32>(1e-8));
    let rdUV = transverse(rd, axis);
    let dominant = max(abs(rd[axis]), 1e-8);
    let slopes = clamp(rdUV / dominant, vec2<f32>(-1.0), vec2<f32>(1.0));

    let entryU = quantize01(
        0.5 * (entryUV.x / extentUV.x + 1.0), entryBins);
    let entryV = quantize01(
        0.5 * (entryUV.y / extentUV.y + 1.0), entryBins);
    let slopeU = quantize01(0.5 * (slopes.x + 1.0), slopeBins);
    let slopeV = quantize01(0.5 * (slopes.y + 1.0), slopeBins);

    var idx = axis * 2u + sign;
    idx = idx * entryBins + entryU;
    idx = idx * entryBins + entryV;
    idx = idx * slopeBins + slopeU;
    idx = idx * slopeBins + slopeV;
    return idx;
}

fn atlasTaxMarch(ray: AtlasTaxRay) -> AtlasTaxOut {
    var out: AtlasTaxOut;
    g_instIdx = 0u;
    let inst = instances[0u];
    let ro = ray.ro.xyz;
    let rd = normalize(ray.rdFar.xyz);
    let box = rayAabb(ro, rd, inst.extents.xyz);
    if (box.y < box.x || box.y < 0.0) {
        return out;
    }

    var t = max(box.x, 0.0);
    let maxDist = min(min(box.y, t + inst.misc.z), ray.rdFar.w);
    let useAtlas = atlasConfig.dims.w != 0u;

    var keyComputations = 0u;
    var atlasLookups = 0u;
    var nonEmptyDispatches = 0u;
    var selectedSlots = 0u;
    var runTests = 0u;
    var skipCalls = 0u;
    var sampleSteps = 0u;
    var fallbackEvals = 0u;
    var iterations = 0u;
    var hit = false;
    var skippedDistance = 0.0;

    var intervals: array<vec2<f32>, 4>;
    var intervalCount = 0u;

    if (useAtlas) {
        keyComputations = 1u;
        let pEntry = ro + rd * t;
        let key = dispatchKey(
            pEntry, rd, inst.extents.xyz,
            atlasConfig.dims.x, atlasConfig.dims.y);
        atlasLookups = 1u;
        if (key < arrayLength(&dispatchAtlas)) {
            let packed = dispatchAtlas[key];
            if (packed != 0u) {
                nonEmptyDispatches = 1u;
                for (var slot = 0u; slot < 4u; slot = slot + 1u) {
                    if (slot >= atlasConfig.dims.z) { break; }
                    let encoded = (packed >> (slot * 8u)) & 0xffu;
                    if (encoded == 0u) { continue; }
                    selectedSlots = selectedSlots + 1u;
                    let runIndex = encoded - 1u;
                    if (runIndex >= arrayLength(&atlasRuns)) { continue; }
                    runTests = runTests + 1u;
                    let run = atlasRuns[runIndex];
                    let center = 0.5 * (run.bmin.xyz + run.bmax.xyz);
                    let halfExtent =
                        max(0.5 * (run.bmax.xyz - run.bmin.xyz),
                            vec3<f32>(1e-8));
                    let interval = rayAabb(ro - center, rd, halfExtent);
                    let enter = max(interval.x, t);
                    let exit = min(interval.y, maxDist);
                    if (exit > enter && intervalCount < 4u) {
                        intervals[intervalCount] = vec2<f32>(enter, exit);
                        intervalCount = intervalCount + 1u;
                    }
                }

                // Route width is <=4. Sort the exact actual-ray intervals once;
                // no global search or neighboring-key walk occurs at runtime.
                for (var i = 0u; i < 4u; i = i + 1u) {
                    for (var j = i + 1u; j < 4u; j = j + 1u) {
                        if (i < intervalCount && j < intervalCount &&
                            intervals[j].x < intervals[i].x) {
                            let tmp = intervals[i];
                            intervals[i] = intervals[j];
                            intervals[j] = tmp;
                        }
                    }
                }
            }
        }
    }

    var intervalIndex = 0u;
    var prev_d = 1e10;
    var candidate_step = 0.0;

    for (var i = 0; i < 192; i = i + 1) {
        if (t > maxDist) { break; }
        iterations = iterations + 1u;

        loop {
            if (intervalIndex >= intervalCount) { break; }
            if (t < intervals[intervalIndex].y) { break; }
            intervalIndex = intervalIndex + 1u;
        }

        if (intervalIndex < intervalCount &&
            t >= intervals[intervalIndex].x &&
            t < intervals[intervalIndex].y) {
            let oldT = t;
            t = intervals[intervalIndex].y;
            intervalIndex = intervalIndex + 1u;
            if (t > oldT) {
                skipCalls = skipCalls + 1u;
                skippedDistance = skippedDistance + (t - oldT);
                prev_d = 1e10;
                candidate_step = 0.0;
            }
            if (t > maxDist) { break; }
        }

        let p = ro + rd * t;
        let current_eps = max(inst.misc.y, t * 0.001);
        let sample = sdfSampleStep(p);
        sampleSteps = sampleSteps + 1u;
        let raw = sample.raw;
        var gl = sample.gradLen;
        if (gl <= 1e-6) {
            let ge = 1e-3;
            let g = vec3<f32>(
                sdfEval(p + vec3<f32>(ge, 0.0, 0.0)) - raw,
                sdfEval(p + vec3<f32>(0.0, ge, 0.0)) - raw,
                sdfEval(p + vec3<f32>(0.0, 0.0, ge)) - raw) / ge;
            fallbackEvals = fallbackEvals + 3u;
            gl = length(g);
        }
        let d = select(raw, raw / gl, gl > 1e-6);

        if (d <= 0.0 || abs(d) < current_eps) {
            hit = true;
            if (d < 0.0 && prev_d > 0.0 && candidate_step > 0.0) {
                let frac = clamp(prev_d / (prev_d - d), 0.0, 1.0);
                t = (t - candidate_step) + candidate_step * frac;
            }
            break;
        }

        candidate_step = max(d, current_eps);
        prev_d = d;
        t = t + candidate_step;
    }

    out.counts0 =
        vec4<u32>(
            keyComputations, atlasLookups,
            nonEmptyDispatches, selectedSlots);
    out.counts1 =
        vec4<u32>(
            runTests, skipCalls, sampleSteps, fallbackEvals);
    out.counts2 =
        vec4<u32>(select(0u, 1u, hit), iterations, 0u, 0u);
    out.distances = vec4<f32>(skippedDistance, 0.0, 0.0, 0.0);
    return out;
}

@compute @workgroup_size(64)
fn cs_atlas_tax(@builtin(global_invocation_id) gid: vec3<u32>) {
    let idx = gid.x;
    if (idx >= arrayLength(&atlasTaxRays)) { return; }
    atlasTaxOut[idx] = atlasTaxMarch(atlasTaxRays[idx]);
}
)WGSL";

    const std::string shaderCode = program.wgsl + diagnosticWgsl;
    WGPUShaderSourceWGSL wgslSrc = {};
    wgslSrc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslSrc.code = wgpu::Device::str(shaderCode.c_str());
    WGPUShaderModuleDescriptor smd = {};
    smd.nextInChain = &wgslSrc.chain;
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(gpu.device, &smd);
    if (!shader) return results;

    WGPUComputePipelineDescriptor cpd = {};
    cpd.compute.module = shader;
    cpd.compute.entryPoint = wgpu::Device::str("cs_atlas_tax");
    WGPUComputePipeline pipeline =
        wgpuDeviceCreateComputePipeline(gpu.device, &cpd);
    if (!pipeline) {
        wgpuShaderModuleRelease(shader);
        return results;
    }

    RuntimeTaxInstance inst;
    inst.extents = glm::vec4(extent, 0.0f);
    inst.misc = glm::vec4(0.0f, 1e-4f, 8000.0f, 0.25f);

    const size_t rayBytes = rays.size() * sizeof(RuntimeTaxRay);
    const size_t outBytes = rays.size() * sizeof(DispatchAtlasTaxOut);
    const size_t paramBytes =
        std::max(program.params.size() * sizeof(float), sizeof(float));

    size_t maxRunBytes = sizeof(DirectProofRun);
    size_t maxAtlasBytes = sizeof(uint32_t);
    for (const auto& artifact : artifacts) {
        maxRunBytes = std::max(
            maxRunBytes,
            artifact.runs.size() * sizeof(DirectProofRun));
        maxAtlasBytes = std::max(
            maxAtlasBytes,
            artifact.table.size() * sizeof(uint32_t));
    }

    auto makeBuffer = [&](uint64_t size, WGPUBufferUsage usage) {
        WGPUBufferDescriptor desc = {};
        desc.size = size;
        desc.usage = usage;
        return wgpuDeviceCreateBuffer(gpu.device, &desc);
    };

    WGPUBuffer rayBuffer = makeBuffer(
        rayBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    WGPUBuffer outBuffer = makeBuffer(
        outBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
    WGPUBuffer readback = makeBuffer(
        outBytes, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);
    WGPUBuffer paramBuffer = makeBuffer(
        paramBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    WGPUBuffer instBuffer = makeBuffer(
        sizeof(RuntimeTaxInstance),
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    WGPUBuffer runBuffer = makeBuffer(
        maxRunBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    WGPUBuffer atlasBuffer = makeBuffer(
        maxAtlasBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    WGPUBuffer configBuffer = makeBuffer(
        sizeof(DispatchAtlasConfig),
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);

    if (!rayBuffer || !outBuffer || !readback || !paramBuffer ||
        !instBuffer || !runBuffer || !atlasBuffer || !configBuffer) {
        if (configBuffer) wgpuBufferRelease(configBuffer);
        if (atlasBuffer) wgpuBufferRelease(atlasBuffer);
        if (runBuffer) wgpuBufferRelease(runBuffer);
        if (instBuffer) wgpuBufferRelease(instBuffer);
        if (paramBuffer) wgpuBufferRelease(paramBuffer);
        if (readback) wgpuBufferRelease(readback);
        if (outBuffer) wgpuBufferRelease(outBuffer);
        if (rayBuffer) wgpuBufferRelease(rayBuffer);
        wgpuComputePipelineRelease(pipeline);
        wgpuShaderModuleRelease(shader);
        return results;
    }

    wgpuQueueWriteBuffer(gpu.queue, rayBuffer, 0, rays.data(), rayBytes);
    if (!program.params.empty()) {
        wgpuQueueWriteBuffer(
            gpu.queue, paramBuffer, 0, program.params.data(),
            program.params.size() * sizeof(float));
    } else {
        const float zero = 0.0f;
        wgpuQueueWriteBuffer(gpu.queue, paramBuffer, 0, &zero, sizeof(zero));
    }
    wgpuQueueWriteBuffer(
        gpu.queue, instBuffer, 0, &inst, sizeof(RuntimeTaxInstance));

    WGPUBindGroupLayout bgl0 =
        wgpuComputePipelineGetBindGroupLayout(pipeline, 0);
    WGPUBindGroupLayout bgl1 =
        wgpuComputePipelineGetBindGroupLayout(pipeline, 1);
    WGPUBindGroupLayout bgl2 =
        wgpuComputePipelineGetBindGroupLayout(pipeline, 2);

    WGPUBindGroupEntry g0e = {};
    g0e.binding = 1;
    g0e.buffer = paramBuffer;
    g0e.size = paramBytes;
    WGPUBindGroupDescriptor g0d = {};
    g0d.layout = bgl0;
    g0d.entryCount = 1;
    g0d.entries = &g0e;
    WGPUBindGroup g0 = wgpuDeviceCreateBindGroup(gpu.device, &g0d);

    WGPUBindGroupEntry g1e = {};
    g1e.binding = 0;
    g1e.buffer = instBuffer;
    g1e.size = sizeof(RuntimeTaxInstance);
    WGPUBindGroupDescriptor g1d = {};
    g1d.layout = bgl1;
    g1d.entryCount = 1;
    g1d.entries = &g1e;
    WGPUBindGroup g1 = wgpuDeviceCreateBindGroup(gpu.device, &g1d);

    if (!g0 || !g1) {
        if (g1) wgpuBindGroupRelease(g1);
        if (g0) wgpuBindGroupRelease(g0);
        wgpuBindGroupLayoutRelease(bgl2);
        wgpuBindGroupLayoutRelease(bgl1);
        wgpuBindGroupLayoutRelease(bgl0);
        wgpuBufferRelease(configBuffer);
        wgpuBufferRelease(atlasBuffer);
        wgpuBufferRelease(runBuffer);
        wgpuBufferRelease(instBuffer);
        wgpuBufferRelease(paramBuffer);
        wgpuBufferRelease(readback);
        wgpuBufferRelease(outBuffer);
        wgpuBufferRelease(rayBuffer);
        wgpuComputePipelineRelease(pipeline);
        wgpuShaderModuleRelease(shader);
        return results;
    }

    const DirectProofRun dummyRun{};
    const uint32_t dummyAtlas = 0u;
    std::vector<DispatchAtlasTaxOut> baselineOut;

    auto runPass = [&](const DispatchAtlasArtifact* artifact,
                       bool enabled,
                       std::vector<DispatchAtlasTaxOut>& hostOut) -> bool {
        size_t runBytes = sizeof(DirectProofRun);
        size_t atlasBytes = sizeof(uint32_t);
        DispatchAtlasConfig config;
        if (enabled && artifact) {
            if (!artifact->representable ||
                artifact->runs.empty() || artifact->table.empty()) {
                return false;
            }
            runBytes = artifact->runs.size() * sizeof(DirectProofRun);
            atlasBytes = artifact->table.size() * sizeof(uint32_t);
            wgpuQueueWriteBuffer(
                gpu.queue, runBuffer, 0, artifact->runs.data(), runBytes);
            wgpuQueueWriteBuffer(
                gpu.queue, atlasBuffer, 0, artifact->table.data(), atlasBytes);
            config.dims = glm::uvec4(
                artifact->entryBins,
                artifact->slopeBins,
                artifact->routeWidth,
                1u);
        } else {
            wgpuQueueWriteBuffer(
                gpu.queue, runBuffer, 0, &dummyRun, sizeof(dummyRun));
            wgpuQueueWriteBuffer(
                gpu.queue, atlasBuffer, 0, &dummyAtlas, sizeof(dummyAtlas));
            config.dims = glm::uvec4(1u, 1u, 1u, 0u);
        }
        wgpuQueueWriteBuffer(
            gpu.queue, configBuffer, 0, &config, sizeof(config));

        WGPUBindGroupEntry g2e[5] = {};
        g2e[0].binding = 0;
        g2e[0].buffer = rayBuffer;
        g2e[0].size = rayBytes;
        g2e[1].binding = 1;
        g2e[1].buffer = outBuffer;
        g2e[1].size = outBytes;
        g2e[2].binding = 2;
        g2e[2].buffer = runBuffer;
        g2e[2].size = runBytes;
        g2e[3].binding = 3;
        g2e[3].buffer = atlasBuffer;
        g2e[3].size = atlasBytes;
        g2e[4].binding = 4;
        g2e[4].buffer = configBuffer;
        g2e[4].size = sizeof(DispatchAtlasConfig);
        WGPUBindGroupDescriptor g2d = {};
        g2d.layout = bgl2;
        g2d.entryCount = 5;
        g2d.entries = g2e;
        WGPUBindGroup g2 =
            wgpuDeviceCreateBindGroup(gpu.device, &g2d);
        if (!g2) return false;

        WGPUCommandEncoder encoder =
            wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUComputePassEncoder pass =
            wgpuCommandEncoderBeginComputePass(encoder, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, g0, 0, nullptr);
        wgpuComputePassEncoderSetBindGroup(pass, 1, g1, 0, nullptr);
        wgpuComputePassEncoderSetBindGroup(pass, 2, g2, 0, nullptr);
        const uint32_t workgroups =
            static_cast<uint32_t>((rays.size() + 63u) / 64u);
        wgpuComputePassEncoderDispatchWorkgroups(pass, workgroups, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        wgpuCommandEncoderCopyBufferToBuffer(
            encoder, outBuffer, 0, readback, 0, outBytes);
        WGPUCommandBuffer command =
            wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &command);
        wgpuCommandBufferRelease(command);
        wgpuCommandEncoderRelease(encoder);

        RuntimeTaxMapResult mapResult;
        WGPUBufferMapCallbackInfo mapInfo = {};
        mapInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        mapInfo.callback = onRuntimeTaxMap;
        mapInfo.userdata1 = &mapResult;
        wgpuBufferMapAsync(
            readback, WGPUMapMode_Read, 0, outBytes, mapInfo);
        while (!mapResult.done) {
            wgpuDevicePoll(gpu.device, true, nullptr);
        }

        bool ok = false;
        if (mapResult.ok) {
            const auto* out =
                static_cast<const DispatchAtlasTaxOut*>(
                    wgpuBufferGetConstMappedRange(
                        readback, 0, outBytes));
            if (out) {
                hostOut.assign(out, out + rays.size());
                ok = true;
            }
            wgpuBufferUnmap(readback);
        }
        wgpuBindGroupRelease(g2);
        return ok;
    };

    const bool baselineOk = runPass(nullptr, false, baselineOut);
    if (!baselineOk || baselineOut.size() != rays.size()) {
        wgpuBindGroupRelease(g1);
        wgpuBindGroupRelease(g0);
        wgpuBindGroupLayoutRelease(bgl2);
        wgpuBindGroupLayoutRelease(bgl1);
        wgpuBindGroupLayoutRelease(bgl0);
        wgpuBufferRelease(configBuffer);
        wgpuBufferRelease(atlasBuffer);
        wgpuBufferRelease(runBuffer);
        wgpuBufferRelease(instBuffer);
        wgpuBufferRelease(paramBuffer);
        wgpuBufferRelease(readback);
        wgpuBufferRelease(outBuffer);
        wgpuBufferRelease(rayBuffer);
        wgpuComputePipelineRelease(pipeline);
        wgpuShaderModuleRelease(shader);
        return results;
    }

    uint64_t baselineSamples = 0u;
    uint64_t baselineFallbacks = 0u;
    uint64_t baselineHits = 0u;
    uint64_t baselineIterations = 0u;
    for (const auto& off : baselineOut) {
        baselineSamples += off.counts1.z;
        baselineFallbacks += off.counts1.w;
        baselineHits += off.counts2.x;
        baselineIterations += off.counts2.y;
    }

    for (const auto& artifact : artifacts) {
        DispatchAtlasRuntimeTax totals;
        totals.minRunCells = artifact.minRunCells;
        totals.entryBins = artifact.entryBins;
        totals.slopeBins = artifact.slopeBins;
        totals.routeWidth = artifact.routeWidth;
        totals.populatedKeys = artifact.populatedKeys;
        totals.coveredPositiveCells = artifact.coveredPositiveCells;
        totals.atlasEntries = artifact.table.size();
        totals.atlasBytes = artifact.table.size() * sizeof(uint32_t);
        totals.runRecords = artifact.runs.size();
        totals.runBytes = artifact.runs.size() * sizeof(DirectProofRun);
        totals.totalArtifactBytes = totals.atlasBytes + totals.runBytes;
        totals.rays = rays.size();
        totals.representable = artifact.representable;
        totals.offSampleSteps = baselineSamples;
        totals.offFallbackEvals = baselineFallbacks;
        totals.offHits = baselineHits;
        totals.offIterations = baselineIterations;

        if (!artifact.representable ||
            artifact.runs.empty() || artifact.table.empty()) {
            totals.directSampleSteps = baselineSamples;
            totals.directFallbackEvals = baselineFallbacks;
            totals.directHits = baselineHits;
            totals.directIterations = baselineIterations;
            totals.valid = true;
            results.push_back(totals);
            continue;
        }

        std::vector<DispatchAtlasTaxOut> directOut;
        if (!runPass(&artifact, true, directOut) ||
            directOut.size() != rays.size()) {
            results.push_back(totals);
            continue;
        }

        for (size_t i = 0; i < rays.size(); ++i) {
            const auto& direct = directOut[i];
            const auto& off = baselineOut[i];
            totals.keyComputations += direct.counts0.x;
            totals.atlasLookups += direct.counts0.y;
            totals.nonEmptyDispatches += direct.counts0.z;
            totals.selectedRouteSlots += direct.counts0.w;
            totals.selectedRunTests += direct.counts1.x;
            totals.skipCalls += direct.counts1.y;
            totals.directSampleSteps += direct.counts1.z;
            totals.directFallbackEvals += direct.counts1.w;
            totals.directHits += direct.counts2.x;
            totals.directIterations += direct.counts2.y;
            totals.skippedDistance +=
                static_cast<double>(direct.distances.x);
            if (direct.counts2.x != off.counts2.x) {
                ++totals.perRayHitMismatches;
            }
        }
        totals.valid = true;
        results.push_back(totals);
    }

    wgpuBindGroupRelease(g1);
    wgpuBindGroupRelease(g0);
    wgpuBindGroupLayoutRelease(bgl2);
    wgpuBindGroupLayoutRelease(bgl1);
    wgpuBindGroupLayoutRelease(bgl0);
    wgpuBufferRelease(configBuffer);
    wgpuBufferRelease(atlasBuffer);
    wgpuBufferRelease(runBuffer);
    wgpuBufferRelease(instBuffer);
    wgpuBufferRelease(paramBuffer);
    wgpuBufferRelease(readback);
    wgpuBufferRelease(outBuffer);
    wgpuBufferRelease(rayBuffer);
    wgpuComputePipelineRelease(pipeline);
    wgpuShaderModuleRelease(shader);
    return results;
}

void printDirectRuntimeTax(const char* viewName, const DirectRuntimeTax& t) {
    const int64_t savedSampleSteps =
        static_cast<int64_t>(t.offSampleSteps) -
        static_cast<int64_t>(t.directSampleSteps);
    const double queriesPerRay =
        t.rays > 0
            ? static_cast<double>(t.artifactQueries) /
                  static_cast<double>(t.rays)
            : 0.0;
    const double recordTestsPerRay =
        t.rays > 0
            ? static_cast<double>(t.recordTests) /
                  static_cast<double>(t.rays)
            : 0.0;
    const double samplesSavedPerQuery =
        t.artifactQueries > 0
            ? static_cast<double>(savedSampleSteps) /
                  static_cast<double>(t.artifactQueries)
            : 0.0;
    const double samplesSavedPerRecordTest =
        t.recordTests > 0
            ? static_cast<double>(savedSampleSteps) /
                  static_cast<double>(t.recordTests)
            : 0.0;

    std::printf(
        "SDF_DIRECT_RUNTIME_TAX view=%s valid=%d axis=%s "
        "min_run_cells=%u min_axis_world=%.6f artifact_records=%zu "
        "artifact_bytes=%zu covered_positive_cells=%u rays=%llu "
        "artifact_queries=%llu record_tests=%llu useful_skip_calls=%llu "
        "artifact_exhaustions=%llu direct_sample_steps=%llu "
        "off_sample_steps=%llu saved_sample_steps=%lld "
        "direct_fallback_evals=%llu off_fallback_evals=%llu "
        "direct_iterations=%llu off_iterations=%llu "
        "direct_hits=%llu off_hits=%llu per_ray_hit_mismatches=%llu "
        "skipped_distance=%.6f queries_per_ray=%.6f "
        "record_tests_per_ray=%.6f samples_saved_per_query=%.6f "
        "samples_saved_per_record_test=%.6f\n",
        viewName,
        t.valid ? 1 : 0,
        directAxisName(t.axis),
        t.minRunCells,
        t.minAxisWorld,
        t.artifactRecords,
        t.artifactBytes,
        t.coveredPositiveCells,
        static_cast<unsigned long long>(t.rays),
        static_cast<unsigned long long>(t.artifactQueries),
        static_cast<unsigned long long>(t.recordTests),
        static_cast<unsigned long long>(t.skipCalls),
        static_cast<unsigned long long>(t.artifactExhaustions),
        static_cast<unsigned long long>(t.directSampleSteps),
        static_cast<unsigned long long>(t.offSampleSteps),
        static_cast<long long>(savedSampleSteps),
        static_cast<unsigned long long>(t.directFallbackEvals),
        static_cast<unsigned long long>(t.offFallbackEvals),
        static_cast<unsigned long long>(t.directIterations),
        static_cast<unsigned long long>(t.offIterations),
        static_cast<unsigned long long>(t.directHits),
        static_cast<unsigned long long>(t.offHits),
        static_cast<unsigned long long>(t.perRayHitMismatches),
        t.skippedDistance,
        queriesPerRay,
        recordTestsPerRay,
        samplesSavedPerQuery,
        samplesSavedPerRecordTest);
}


void printDispatchAtlasRuntimeTax(
    const char* viewName,
    const DispatchAtlasRuntimeTax& t) {
    const int64_t saved =
        static_cast<int64_t>(t.offSampleSteps) -
        static_cast<int64_t>(t.directSampleSteps);
    const uint64_t chargedOps =
        t.keyComputations + t.atlasLookups +
        t.selectedRouteSlots + t.selectedRunTests;
    const double savedPerLookup =
        t.atlasLookups > 0u
            ? static_cast<double>(saved) /
                  static_cast<double>(t.atlasLookups)
            : 0.0;
    const double savedPerRunTest =
        t.selectedRunTests > 0u
            ? static_cast<double>(saved) /
                  static_cast<double>(t.selectedRunTests)
            : 0.0;
    const double savedPerChargedOp =
        chargedOps > 0u
            ? static_cast<double>(saved) /
                  static_cast<double>(chargedOps)
            : 0.0;
    const double lookupPerRay =
        t.rays > 0u
            ? static_cast<double>(t.atlasLookups) /
                  static_cast<double>(t.rays)
            : 0.0;

    std::printf(
        "SDF_DISPATCH_ATLAS_TAX view=%s valid=%d representable=%d "
        "min_run_cells=%u entry_bins=%u slope_bins=%u route_width=%u "
        "atlas_entries=%zu populated_keys=%u atlas_bytes=%zu "
        "run_records=%zu run_bytes=%zu total_artifact_bytes=%zu "
        "covered_positive_cells=%u rays=%llu key_computations=%llu "
        "atlas_lookups=%llu non_empty_dispatches=%llu "
        "selected_route_slots=%llu selected_run_tests=%llu "
        "useful_skip_calls=%llu direct_sample_steps=%llu "
        "off_sample_steps=%llu saved_sample_steps=%lld "
        "direct_fallback_evals=%llu off_fallback_evals=%llu "
        "direct_iterations=%llu off_iterations=%llu "
        "direct_hits=%llu off_hits=%llu per_ray_hit_mismatches=%llu "
        "skipped_distance=%.6f lookups_per_ray=%.6f "
        "saved_per_lookup=%.6f saved_per_run_test=%.6f "
        "saved_per_charged_dispatch_op=%.6f\n",
        viewName,
        t.valid ? 1 : 0,
        t.representable ? 1 : 0,
        t.minRunCells,
        t.entryBins,
        t.slopeBins,
        t.routeWidth,
        t.atlasEntries,
        t.populatedKeys,
        t.atlasBytes,
        t.runRecords,
        t.runBytes,
        t.totalArtifactBytes,
        t.coveredPositiveCells,
        static_cast<unsigned long long>(t.rays),
        static_cast<unsigned long long>(t.keyComputations),
        static_cast<unsigned long long>(t.atlasLookups),
        static_cast<unsigned long long>(t.nonEmptyDispatches),
        static_cast<unsigned long long>(t.selectedRouteSlots),
        static_cast<unsigned long long>(t.selectedRunTests),
        static_cast<unsigned long long>(t.skipCalls),
        static_cast<unsigned long long>(t.directSampleSteps),
        static_cast<unsigned long long>(t.offSampleSteps),
        static_cast<long long>(saved),
        static_cast<unsigned long long>(t.directFallbackEvals),
        static_cast<unsigned long long>(t.offFallbackEvals),
        static_cast<unsigned long long>(t.directIterations),
        static_cast<unsigned long long>(t.offIterations),
        static_cast<unsigned long long>(t.directHits),
        static_cast<unsigned long long>(t.offHits),
        static_cast<unsigned long long>(t.perRayHitMismatches),
        t.skippedDistance,
        lookupPerRay,
        savedPerLookup,
        savedPerRunTest,
        savedPerChargedOp);
}

void printDispatchAtlasVerdict(
    const char* viewName,
    const RuntimeTaxTotals& genericBaseline,
    const std::vector<DispatchAtlasRuntimeTax>& candidates) {
    const int64_t genericSaved =
        static_cast<int64_t>(genericBaseline.offSampleSteps) -
        static_cast<int64_t>(genericBaseline.onSampleSteps);
    const double baselineEconomics =
        genericBaseline.candidateCalls > 0u && genericSaved > 0
            ? static_cast<double>(genericSaved) /
                  static_cast<double>(genericBaseline.candidateCalls)
            : 0.0;

    const DispatchAtlasRuntimeTax* best = nullptr;
    double bestEconomics = 0.0;
    for (const auto& candidate : candidates) {
        const int64_t saved =
            static_cast<int64_t>(candidate.offSampleSteps) -
            static_cast<int64_t>(candidate.directSampleSteps);
        const uint64_t chargedOps =
            candidate.keyComputations + candidate.atlasLookups +
            candidate.selectedRouteSlots + candidate.selectedRunTests;
        if (!candidate.valid || !candidate.representable ||
            candidate.perRayHitMismatches != 0u ||
            saved <= 0 || chargedOps == 0u) {
            continue;
        }
        const double economics =
            static_cast<double>(saved) /
            static_cast<double>(chargedOps);
        if (!best || economics > bestEconomics) {
            best = &candidate;
            bestEconomics = economics;
        }
    }

    const double gain =
        baselineEconomics > 0.0
            ? bestEconomics / baselineEconomics
            : 0.0;
    const bool passes10x =
        best != nullptr &&
        baselineEconomics > 0.0 &&
        bestEconomics >= baselineEconomics * 10.0;

    std::printf(
        "SDF_DISPATCH_ATLAS_VERDICT view=%s baseline_samples_per_call=%.6f "
        "candidate_found=%d best_min_run_cells=%u best_entry_bins=%u "
        "best_slope_bins=%u best_route_width=%u "
        "best_samples_per_charged_dispatch_op=%.6f complete_gain=%.2f "
        "passes_10x=%d production_eligible=0 camera_independent=1\n",
        viewName,
        baselineEconomics,
        best ? 1 : 0,
        best ? best->minRunCells : 0u,
        best ? best->entryBins : 0u,
        best ? best->slopeBins : 0u,
        best ? best->routeWidth : 0u,
        bestEconomics,
        gain,
        passes10x ? 1 : 0);
}


void printDirectDispatchOracleCeiling(
    const char* viewName,
    const RuntimeTaxTotals& genericBaseline,
    const std::vector<DirectRuntimeTax>& candidates) {
    const int64_t genericSaved =
        static_cast<int64_t>(genericBaseline.offSampleSteps) -
        static_cast<int64_t>(genericBaseline.onSampleSteps);
    const double baselineEconomics =
        genericBaseline.candidateCalls > 0u && genericSaved > 0
            ? static_cast<double>(genericSaved) /
                  static_cast<double>(genericBaseline.candidateCalls)
            : 0.0;

    for (const auto& candidate : candidates) {
        const int64_t saved =
            static_cast<int64_t>(candidate.offSampleSteps) -
            static_cast<int64_t>(candidate.directSampleSteps);
        if (!candidate.valid || candidate.perRayHitMismatches != 0u ||
            candidate.artifactRecords == 0u || candidate.skipCalls == 0u ||
            saved <= 0) {
            continue;
        }

        // This is deliberately an oracle CEILING, not a production claim.
        // It charges exactly one hypothetical direct dispatch for each query
        // that actually produced a proof-authorized skip and charges no
        // irrelevant queries. A real stable-key atlas must approach this
        // ceiling without using camera/frame-derived state.
        const double savedPerUsefulDispatch =
            static_cast<double>(saved) /
            static_cast<double>(candidate.skipCalls);
        const double usefulDispatchesPerRay =
            candidate.rays > 0u
                ? static_cast<double>(candidate.skipCalls) /
                      static_cast<double>(candidate.rays)
                : 0.0;
        const double irrelevantQueryFraction =
            candidate.artifactQueries > 0u
                ? 1.0 -
                      static_cast<double>(candidate.skipCalls) /
                          static_cast<double>(candidate.artifactQueries)
                : 0.0;
        const double oracleGain =
            baselineEconomics > 0.0
                ? savedPerUsefulDispatch / baselineEconomics
                : 0.0;

        std::printf(
            "SDF_DIRECT_DISPATCH_ORACLE view=%s axis=%s min_run_cells=%u "
            "useful_dispatches=%llu rays=%llu saved_sample_steps=%lld "
            "saved_per_useful_dispatch=%.6f useful_dispatches_per_ray=%.8f "
            "irrelevant_query_fraction=%.8f baseline_samples_per_call=%.6f "
            "oracle_gain=%.2f camera_independent=0 production_eligible=0\n",
            viewName,
            directAxisName(candidate.axis),
            candidate.minRunCells,
            static_cast<unsigned long long>(candidate.skipCalls),
            static_cast<unsigned long long>(candidate.rays),
            static_cast<long long>(saved),
            savedPerUsefulDispatch,
            usefulDispatchesPerRay,
            irrelevantQueryFraction,
            baselineEconomics,
            oracleGain);
    }
}


void printDirectProfitabilityVerdict(
    const char* viewName,
    const RuntimeTaxTotals& genericBaseline,
    const std::vector<DirectRuntimeTax>& candidates) {
    const int64_t genericSaved =
        static_cast<int64_t>(genericBaseline.offSampleSteps) -
        static_cast<int64_t>(genericBaseline.onSampleSteps);
    const double baselineEconomics =
        genericBaseline.candidateCalls > 0u && genericSaved > 0
            ? static_cast<double>(genericSaved) /
                  static_cast<double>(genericBaseline.candidateCalls)
            : 0.0;

    bool found = false;
    uint32_t bestAxis = 0u;
    uint32_t bestMinRunCells = 0u;
    double bestQueryGain = 0.0;
    double bestRecordGain = 0.0;
    double bestCombinedGain = 0.0;

    if (genericBaseline.valid && baselineEconomics > 0.0) {
        for (const auto& candidate : candidates) {
            const int64_t saved =
                static_cast<int64_t>(candidate.offSampleSteps) -
                static_cast<int64_t>(candidate.directSampleSteps);
            if (!candidate.valid ||
                candidate.perRayHitMismatches != 0u ||
                candidate.artifactRecords == 0u ||
                candidate.artifactQueries == 0u ||
                candidate.recordTests == 0u ||
                saved <= 0) {
                continue;
            }

            const double savedPerQuery =
                static_cast<double>(saved) /
                static_cast<double>(candidate.artifactQueries);
            const double savedPerRecord =
                static_cast<double>(saved) /
                static_cast<double>(candidate.recordTests);
            const double queryGain = savedPerQuery / baselineEconomics;
            const double recordGain = savedPerRecord / baselineEconomics;
            const double combinedGain = std::min(queryGain, recordGain);
            if (!found || combinedGain > bestCombinedGain) {
                found = true;
                bestAxis = candidate.axis;
                bestMinRunCells = candidate.minRunCells;
                bestQueryGain = queryGain;
                bestRecordGain = recordGain;
                bestCombinedGain = combinedGain;
            }
        }
    }

    constexpr double kRequiredEconomicsGain = 10.0;
    const bool graduates =
        found &&
        bestQueryGain >= kRequiredEconomicsGain &&
        bestRecordGain >= kRequiredEconomicsGain;

    std::printf(
        "SDF_DIRECT_PROFITABILITY_VERDICT view=%s baseline=%.6f "
        "candidate_found=%d best_axis=%s best_min_run_cells=%u "
        "best_query_gain=%.4f best_record_gain=%.4f "
        "best_combined_gain=%.4f required_gain=%.1f graduation=%s\n",
        viewName,
        baselineEconomics,
        found ? 1 : 0,
        directAxisName(bestAxis),
        bestMinRunCells,
        bestQueryGain,
        bestRecordGain,
        bestCombinedGain,
        kRequiredEconomicsGain,
        graduates ? "PASS" : "REJECT");
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

    // Test-only Spatial-Prophetic Direct candidates. Each artifact chooses one
    // partition axis and a minimum maximal-run length. This deliberately sweeps
    // representation economics before any production shader mutation.
    std::vector<DirectRunArtifact> directArtifacts;
    for (uint32_t axis = 0u; axis < 3u; ++axis) {
        for (uint32_t minRunCells : {1u, 2u, 4u}) {
            directArtifacts.push_back(
                buildDirectRunArtifact(
                    proofGrids[1], proofExtent, axis, minRunCells));
        }
    }


    // Camera-independent AOT ray-entry dispatch candidates. The builder sees
    // only field-local proof geometry and fixed quantization; no camera rays.
    std::vector<DispatchAtlasArtifact> dispatchAtlases;
    for (uint32_t minRunCells : {1u, 2u}) {
        for (uint32_t entryBins : {4u, 8u}) {
            for (uint32_t slopeBins : {2u, 4u}) {
                for (uint32_t routeWidth : {1u, 2u}) {
                    dispatchAtlases.push_back(
                        buildDispatchAtlasArtifact(
                            proofGrids[1], proofExtent,
                            minRunCells, entryBins,
                            slopeBins, routeWidth));
                }
            }
        }
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

        // Exact runtime-consumption diagnostic for the production-selected
        // depth-4 proof grid. It runs outside the timed renderer samples and
        // calls the emitted production rangeCandidate() directly on GPU.
        const RuntimeTaxTotals runtimeTax =
            runRuntimeTaxDiagnostic(
                gpu, probeProgram, proofGrids[1], proofExtent,
                c.eye, view, proj);
        printRuntimeTax(c.name, runtimeTax);
        if (!runtimeTax.valid || runtimeTax.candidateCalls == 0u) {
            std::printf(
                "SDF_RANGE_PERF FAIL runtime tax diagnostic inactive for %s\n",
                c.name);
            measurementWarnings = true;
        }
        if (runtimeTax.valid && runtimeTax.perRayHitMismatches != 0u) {
            std::printf(
                "SDF_RANGE_PERF FAIL runtime tax ON/OFF per-ray hit mismatch "
                "for %s: mismatches=%llu on=%llu off=%llu\n",
                c.name,
                static_cast<unsigned long long>(
                    runtimeTax.perRayHitMismatches),
                static_cast<unsigned long long>(runtimeTax.onHits),
                static_cast<unsigned long long>(runtimeTax.offHits));
            measurementWarnings = true;
        }

        const auto directTaxes =
            runDirectArtifactDiagnostic(
                gpu, probeProgram, directArtifacts, runtimeTax,
                proofExtent, c.eye, view, proj);
        if (directTaxes.size() != directArtifacts.size()) {
            std::printf(
                "SDF_RANGE_PERF FAIL direct artifact diagnostic result count "
                "for %s: got=%zu expected=%zu\n",
                c.name, directTaxes.size(), directArtifacts.size());
            measurementWarnings = true;
        }
        for (const auto& directTax : directTaxes) {
            printDirectRuntimeTax(c.name, directTax);
            if (!directTax.valid) {
                std::printf(
                    "SDF_RANGE_PERF FAIL direct artifact diagnostic invalid "
                    "for %s axis=%s min_run_cells=%u\n",
                    c.name, directAxisName(directTax.axis),
                    directTax.minRunCells);
                measurementWarnings = true;
                continue;
            }
            if (directTax.perRayHitMismatches != 0u) {
                std::printf(
                    "SDF_RANGE_PERF FAIL direct artifact per-ray hit mismatch "
                    "for %s axis=%s min_run_cells=%u mismatches=%llu\n",
                    c.name, directAxisName(directTax.axis),
                    directTax.minRunCells,
                    static_cast<unsigned long long>(
                        directTax.perRayHitMismatches));
                measurementWarnings = true;
            }
            if (runtimeTax.valid &&
                directTax.offSampleSteps != runtimeTax.offSampleSteps) {
                std::printf(
                    "SDF_RANGE_PERF FAIL direct diagnostic OFF baseline drift "
                    "for %s axis=%s min_run_cells=%u direct_off=%llu "
                    "generic_off=%llu\n",
                    c.name, directAxisName(directTax.axis),
                    directTax.minRunCells,
                    static_cast<unsigned long long>(
                        directTax.offSampleSteps),
                    static_cast<unsigned long long>(
                        runtimeTax.offSampleSteps));
                measurementWarnings = true;
            }
        }

        printDirectDispatchOracleCeiling(c.name, runtimeTax, directTaxes);
        printDirectProfitabilityVerdict(c.name, runtimeTax, directTaxes);

        const auto atlasTaxes =
            runDispatchAtlasDiagnostic(
                gpu, probeProgram, dispatchAtlases, runtimeTax,
                proofExtent, c.eye, view, proj);
        if (atlasTaxes.size() != dispatchAtlases.size()) {
            std::printf(
                "SDF_RANGE_PERF FAIL dispatch atlas diagnostic result count "
                "for %s: got=%zu expected=%zu\n",
                c.name, atlasTaxes.size(), dispatchAtlases.size());
            measurementWarnings = true;
        }
        for (const auto& atlasTax : atlasTaxes) {
            printDispatchAtlasRuntimeTax(c.name, atlasTax);
            if (!atlasTax.valid) {
                std::printf(
                    "SDF_RANGE_PERF FAIL dispatch atlas diagnostic invalid "
                    "for %s min_run=%u entry_bins=%u slope_bins=%u "
                    "route_width=%u\n",
                    c.name,
                    atlasTax.minRunCells,
                    atlasTax.entryBins,
                    atlasTax.slopeBins,
                    atlasTax.routeWidth);
                measurementWarnings = true;
                continue;
            }
            if (atlasTax.perRayHitMismatches != 0u) {
                std::printf(
                    "SDF_RANGE_PERF FAIL dispatch atlas per-ray hit mismatch "
                    "for %s min_run=%u entry_bins=%u slope_bins=%u "
                    "route_width=%u mismatches=%llu\n",
                    c.name,
                    atlasTax.minRunCells,
                    atlasTax.entryBins,
                    atlasTax.slopeBins,
                    atlasTax.routeWidth,
                    static_cast<unsigned long long>(
                        atlasTax.perRayHitMismatches));
                measurementWarnings = true;
            }
            if (runtimeTax.valid &&
                atlasTax.offSampleSteps != runtimeTax.offSampleSteps) {
                std::printf(
                    "SDF_RANGE_PERF FAIL dispatch atlas OFF baseline drift "
                    "for %s min_run=%u entry_bins=%u slope_bins=%u "
                    "route_width=%u atlas_off=%llu generic_off=%llu\n",
                    c.name,
                    atlasTax.minRunCells,
                    atlasTax.entryBins,
                    atlasTax.slopeBins,
                    atlasTax.routeWidth,
                    static_cast<unsigned long long>(
                        atlasTax.offSampleSteps),
                    static_cast<unsigned long long>(
                        runtimeTax.offSampleSteps));
                measurementWarnings = true;
            }
        }
        printDispatchAtlasVerdict(c.name, runtimeTax, atlasTaxes);

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
