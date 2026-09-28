// Native WebGPU A/B for the first production SourceRho authority experiment.
//
// PR #369 established that an already-known source binding slot can consult a
// provenance-checked semantic artifact without relevance search. This witness
// measures the next boundary: two otherwise-identical production WebGPU
// renderers, one exact and one allowed to compile a proven literal-zero SourceRho
// slot to `return 0.0`.
//
// The test reports speed; it does NOT assert a speedup. CI timing is evidence,
// not correctness. Correctness gates are: authority actually crossed into the
// authoritative renderer, both arms are warmed, and no measured frame recompiles
// shader structure.

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include "Singularity/Screen/RadianceSource.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>

namespace {

std::shared_ptr<OntoMath::MathNode> scalarNode(double value) {
    auto node = std::make_shared<OntoMath::MathNode>();
    node->op = OntoMath::MathNode::Op::ScalarLeaf;
    node->scalarForm.terms.push_back(OntoMath::Term(value));
    return node;
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
    uint64_t recurringCompiles = 0;
    uint64_t cacheHits = 0;
    size_t recurringParamUploadBytes = 0;
};

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    constexpr uint32_t W = 1280;
    constexpr uint32_t H = 720;
    constexpr int kWarmupFrames = 6;
    constexpr int kSamplePairs = 12;
    constexpr uint64_t kMemoId = 0x52584f4155544831ULL; // "RXOAUTH1"

    wgpu::Device gpu;
    if (!gpu.init()) {
        std::printf("SOURCE_RHO_AUTH_PERF FAIL no WebGPU device\n");
        return 1;
    }

    WebGpuRenderer exactRenderer;
    WebGpuRenderer authorityRenderer;
    if (!exactRenderer.init(gpu) || !authorityRenderer.init(gpu)) {
        std::printf("SOURCE_RHO_AUTH_PERF FAIL renderer init\n");
        return 1;
    }

    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment;
    td.dimension = WGPUTextureDimension_2D;
    td.size = {W, H, 1};
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1;
    td.sampleCount = 1;
    WGPUTexture texture = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView target = wgpuTextureCreateView(texture, nullptr);
    if (!texture || !target) {
        std::printf("SOURCE_RHO_AUTH_PERF FAIL offscreen target\n");
        return 1;
    }

    auto field = geom::SdfNode::leaf(
        geom::SdfPrim::Sphere, glm::vec3(1.0f));
    const glm::vec3 extent(1.25f);

    RenderMaterial mat;
    mat.baseColor = glm::vec3(0.75f);
    mat.ambient = 0.2f;
    mat.diffuse = 0.8f;
    mat.specular = 0.25f;
    mat.shininess = 24.0f;

    auto zeroNode = scalarNode(0.0);
    auto liveNode = scalarNode(0.55);
    OntoMath::Piecewise zeroRho =
        OntoMath::Piecewise::continuous(zeroNode);
    OntoMath::Piecewise liveRho =
        OntoMath::Piecewise::continuous(liveNode);

    Rendering::RadianceSourceBinding zeroSource;
    zeroSource.producerId = "perf/source-zero";
    zeroSource.position = glm::vec3(-1.0f, 0.8f, 2.2f);
    zeroSource.radianceExpr = &zeroRho;
    zeroSource.radianceRevision = 51001;
    zeroSource.coefficients = glm::vec4(1.0f, 0.2f, 0.8f, 0.25f);

    Rendering::RadianceSourceBinding liveSource;
    liveSource.producerId = "perf/source-live";
    liveSource.position = glm::vec3(1.0f, 0.8f, 2.2f);
    liveSource.radianceExpr = &liveRho;
    liveSource.radianceRevision = 51002;
    liveSource.coefficients = glm::vec4(1.0f, 0.2f, 0.8f, 0.25f);

    const std::vector<Rendering::RadianceSourceBinding> sources{
        zeroSource, liveSource};
    constexpr uint64_t kSourceSetRevision = 52001;

    exactRenderer.setRadianceSources(sources, kSourceSetRevision);
    authorityRenderer.setRadianceSources(sources, kSourceSetRevision);

    // Charge semantic artifact construction explicitly. This setup happens once
    // for the admitted source-set identity and must not disappear from the
    // economics merely because steady-state frames are timed later.
    const auto authoritySetupT0 = std::chrono::steady_clock::now();
    authorityRenderer.setRadianceZeroAuthorityExperimentEnabled(true);
    const auto authoritySetupT1 = std::chrono::steady_clock::now();
    const uint64_t authoritySetupNs =
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                authoritySetupT1 - authoritySetupT0).count());

    const glm::vec3 eye(0.0f, 0.0f, 3.2f);
    const glm::mat4 view =
        glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    const glm::mat4 proj = glm::perspectiveZO(
        glm::radians(45.0f), static_cast<float>(W) / H, 0.1f, 100.0f);
    exactRenderer.setCamera(view, proj, eye);
    authorityRenderer.setCamera(view, proj, eye);

    auto renderOne = [&](WebGpuRenderer& renderer) -> Sample {
        renderer.setModel(glm::mat4(1.0f));
        const auto t0 = std::chrono::steady_clock::now();
        renderer.beginFrameOffscreen(
            target, W, H, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        renderer.drawImplicit(
            field, extent, mat, nullptr, kMemoId,
            /*memoRevision=*/1, nullptr,
            /*memoParameterRevision=*/1);
        renderer.endFrame();
        // Include submission completion in wall time, matching the maintained
        // SDF performance harness rather than timing CPU command recording only.
        wgpuDevicePoll(gpu.device, true, nullptr);
        const auto t1 = std::chrono::steady_clock::now();

        Sample s;
        s.wallMs =
            std::chrono::duration<double, std::milli>(t1 - t0).count();
        s.stats = renderer.frameStats();
        return s;
    };

    // Capture one-time compilation economics separately from steady execution.
    // These cold samples are descriptive rather than a speed gate.
    const Sample exactCold = renderOne(exactRenderer);
    const Sample authorityCold = renderOne(authorityRenderer);
    if (exactCold.stats.sdfProgramCompiles != 1 ||
        authorityCold.stats.sdfProgramCompiles != 1) {
        std::printf(
            "SOURCE_RHO_AUTH_PERF FAIL cold compile count "
            "exact=%u authority=%u\n",
            exactCold.stats.sdfProgramCompiles,
            authorityCold.stats.sdfProgramCompiles);
        return 1;
    }

    // Warm independently so timestamp pipeline latency and any first-frame
    // residency effects are outside the steady paired samples.
    for (int i = 0; i < kWarmupFrames; ++i) {
        (void)renderOne(exactRenderer);
        (void)renderOne(authorityRenderer);
    }

    const auto authorityStatsAfterWarmup =
        authorityRenderer.renderedFieldSemanticObservationStats();
    if (authorityStatsAfterWarmup.authorityBypassesApplied == 0) {
        std::printf(
            "SOURCE_RHO_AUTH_PERF FAIL authority never reached WebGPU codegen\n");
        return 1;
    }

    Arm exact;
    Arm authority;

    auto record = [&](WebGpuRenderer& renderer, Arm& arm) {
        const Sample s = renderOne(renderer);
        arm.wallMs.push_back(s.wallMs);
        if (s.stats.gpuMainPassTimingValid)
            arm.gpuMs.push_back(static_cast<double>(s.stats.gpuMainPassMs));
        arm.recurringCompiles += s.stats.sdfProgramCompiles;
        arm.cacheHits += s.stats.sdfProgramCacheHits;
        arm.recurringParamUploadBytes += s.stats.sdfParameterBytesUploaded;
    };

    // Alternate which arm runs first to reduce monotonic thermal/runner drift.
    for (int i = 0; i < kSamplePairs; ++i) {
        if ((i & 1) == 0) {
            record(exactRenderer, exact);
            record(authorityRenderer, authority);
        } else {
            record(authorityRenderer, authority);
            record(exactRenderer, exact);
        }
    }

    const double exactWall = median(exact.wallMs);
    const double authorityWall = median(authority.wallMs);
    const double wallRatio =
        authorityWall > 0.0 ? exactWall / authorityWall : 0.0;
    const double exactGpu = median(exact.gpuMs);
    const double authorityGpu = median(authority.gpuMs);
    const double gpuRatio =
        authorityGpu > 0.0 ? exactGpu / authorityGpu : 0.0;

    // Incremental repair economics: mutate only source slot 0 from proven zero
    // to authored nonzero. Time semantic admission/repair separately from the
    // next draw's necessary shader-structure fallback.
    auto repairedNode = scalarNode(0.55);
    OntoMath::Piecewise repairedRho =
        OntoMath::Piecewise::continuous(repairedNode);
    std::vector<Rendering::RadianceSourceBinding> repairedSources = sources;
    repairedSources[0].radianceExpr = &repairedRho;
    repairedSources[0].radianceRevision = 51003;
    const auto repairStatsBefore =
        authorityRenderer.renderedFieldSemanticObservationStats();
    const auto repairT0 = std::chrono::steady_clock::now();
    authorityRenderer.setRadianceSources(repairedSources, 52002);
    const auto repairT1 = std::chrono::steady_clock::now();
    const uint64_t authorityRepairNs =
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                repairT1 - repairT0).count());
    const auto repairStatsAfter =
        authorityRenderer.renderedFieldSemanticObservationStats();
    if (repairStatsAfter.alignedSlotRepairs !=
            repairStatsBefore.alignedSlotRepairs + 1) {
        std::printf(
            "SOURCE_RHO_AUTH_PERF FAIL incremental repair count before=%llu "
            "after=%llu\n",
            static_cast<unsigned long long>(
                repairStatsBefore.alignedSlotRepairs),
            static_cast<unsigned long long>(
                repairStatsAfter.alignedSlotRepairs));
        return 1;
    }
    const uint64_t authorityApplicationsBeforeRepairDraw =
        repairStatsAfter.authorityBypassesApplied;
    const Sample repairDraw = renderOne(authorityRenderer);
    const auto repairStatsAfterDraw =
        authorityRenderer.renderedFieldSemanticObservationStats();
    if (repairDraw.stats.sdfProgramCompiles != 1) {
        std::printf(
            "SOURCE_RHO_AUTH_PERF FAIL repaired zero->nonzero slot did not "
            "recompile exact shader structure: compiles=%u\n",
            repairDraw.stats.sdfProgramCompiles);
        return 1;
    }
    if (repairStatsAfterDraw.authorityBypassesApplied !=
            authorityApplicationsBeforeRepairDraw) {
        std::printf(
            "SOURCE_RHO_AUTH_PERF FAIL repaired nonzero slot retained "
            "authority\n");
        return 1;
    }

    if (exact.recurringCompiles != 0 || authority.recurringCompiles != 0) {
        std::printf(
            "SOURCE_RHO_AUTH_PERF FAIL measured frame recompiled "
            "exact=%llu authority=%llu\n",
            static_cast<unsigned long long>(exact.recurringCompiles),
            static_cast<unsigned long long>(authority.recurringCompiles));
        return 1;
    }
    if (exact.cacheHits < kSamplePairs ||
        authority.cacheHits < kSamplePairs) {
        std::printf(
            "SOURCE_RHO_AUTH_PERF FAIL measured frame missed memo "
            "exact_hits=%llu authority_hits=%llu\n",
            static_cast<unsigned long long>(exact.cacheHits),
            static_cast<unsigned long long>(authority.cacheHits));
        return 1;
    }

    std::printf(
        "SOURCE_RHO_AUTH_PERF samples=%d "
        "exact_wall_median_ms=%.6f authority_wall_median_ms=%.6f "
        "ratio_exact_over_authority=%.6f "
        "exact_gpu_median_ms=%.6f authority_gpu_median_ms=%.6f "
        "gpu_ratio_exact_over_authority=%.6f "
        "exact_gpu_samples=%zu authority_gpu_samples=%zu "
        "exact_cold_wall_ms=%.6f authority_cold_wall_ms=%.6f "
        "exact_cold_wgsl_bytes=%zu authority_cold_wgsl_bytes=%zu "
        "exact_cold_param_upload_bytes=%zu authority_cold_param_upload_bytes=%zu "
        "exact_recurring_compiles=%llu authority_recurring_compiles=%llu "
        "exact_cache_hits=%llu authority_cache_hits=%llu "
        "exact_param_upload_bytes=%zu authority_param_upload_bytes=%zu "
        "authority_artifact_bytes=%zu authority_mask_bytes=%zu "
        "authority_setup_ns=%llu authority_repair_ns=%llu "
        "authority_repair_draw_ms=%.6f authority_repair_wgsl_bytes=%zu "
        "vessel_observations=%llu semantic_builds=%llu semantic_cache_hits=%llu "
        "slot_builds=%llu slot_repairs=%llu "
        "handle_publications=%llu handle_validations=%llu "
        "proof_reads=%llu metadata_tests=%llu proof_fallbacks=%llu "
        "authority_applications=%llu\n",
        kSamplePairs,
        exactWall, authorityWall, wallRatio,
        exactGpu, authorityGpu, gpuRatio,
        exact.gpuMs.size(), authority.gpuMs.size(),
        exactCold.wallMs, authorityCold.wallMs,
        exactCold.stats.sdfWgslBytesGenerated,
        authorityCold.stats.sdfWgslBytesGenerated,
        exactCold.stats.sdfParameterBytesUploaded,
        authorityCold.stats.sdfParameterBytesUploaded,
        static_cast<unsigned long long>(exact.recurringCompiles),
        static_cast<unsigned long long>(authority.recurringCompiles),
        static_cast<unsigned long long>(exact.cacheHits),
        static_cast<unsigned long long>(authority.cacheHits),
        exact.recurringParamUploadBytes,
        authority.recurringParamUploadBytes,
        authorityRenderer.renderedFieldSemanticObservationStats()
            .alignedSlotLogicalBytes,
        sources.size(),
        static_cast<unsigned long long>(authoritySetupNs),
        static_cast<unsigned long long>(authorityRepairNs),
        repairDraw.wallMs,
        repairDraw.stats.sdfWgslBytesGenerated,
        static_cast<unsigned long long>(
            authorityRenderer.renderedFieldSemanticObservationStats()
                .vesselObservations),
        static_cast<unsigned long long>(
            authorityRenderer.renderedFieldSemanticObservationStats()
                .semanticBuilds),
        static_cast<unsigned long long>(
            authorityRenderer.renderedFieldSemanticObservationStats()
                .semanticCacheHits),
        static_cast<unsigned long long>(
            authorityRenderer.renderedFieldSemanticObservationStats()
                .alignedSlotBuilds),
        static_cast<unsigned long long>(
            authorityRenderer.renderedFieldSemanticObservationStats()
                .alignedSlotRepairs),
        static_cast<unsigned long long>(
            authorityRenderer.renderedFieldSemanticObservationStats()
                .alignedHandlePublications),
        static_cast<unsigned long long>(
            authorityRenderer.renderedFieldSemanticObservationStats()
                .alignedHandleValidations),
        static_cast<unsigned long long>(
            authorityRenderer.renderedFieldSemanticObservationStats()
                .alignedProofReads),
        static_cast<unsigned long long>(
            authorityRenderer.renderedFieldSemanticObservationStats()
                .alignedHandleMetadataTests),
        static_cast<unsigned long long>(
            authorityRenderer.renderedFieldSemanticObservationStats()
                .alignedProofReadFallbacks),
        static_cast<unsigned long long>(
            authorityRenderer.renderedFieldSemanticObservationStats()
                .authorityBypassesApplied));

    wgpuTextureViewRelease(target);
    wgpuTextureRelease(texture);
    return 0;
}
