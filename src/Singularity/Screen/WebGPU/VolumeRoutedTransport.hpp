#pragma once
#include <webgpu/webgpu.h>
#include <glm/glm.hpp>
#include <array>
#include <map>
#include <functional>
#include <string>
#include <vector>

// Kernel-only GPU execution state beneath Screen, not an authored being or a
// second mathematical office. Handles/layouts/programs are device-bound; buffers
// and tile permutation rebuild on viewport changes, counts reset per strip.
// Equations/parameters remain the production compiler's P/instances snapshots.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
struct KernelRoutedSpec {glm::vec4 origin,half,limits,curv,amps,freqs,phases;};
static_assert(sizeof(KernelRoutedSpec)==112);
struct KernelRoutedRange {WGPUBuffer buffer=nullptr;uint64_t offset=0,size=0;};
struct KernelRoutedTransport {
    WGPUDevice device=nullptr;WGPUQueue queue=nullptr;
    WGPUBuffer publication=nullptr,rays=nullptr,singles=nullptr,overlaps=nullptr,offsets=nullptr,indices=nullptr,counts=nullptr,args=nullptr;
    std::array<WGPUBuffer,3> lists{};
    std::array<WGPUComputePipeline,6> common{};
    uint32_t width=0,height=0,stripRows=0,capacity=0,tiles=0;
    bool coalescePasses=true; // §6fb: one compute pass for the identical ordered dispatch stream (default ON after native parity); false = original 8 passes/strip, kept for A/B.
    // Kernel diagnostics (beneath Screen, like the GPU handles above; not being state): last encode()'s compute-pass count, dispatch count and FNV-1a order hash of (pipeline,workgroups/indirect offset,kind).
    // collectDispatchTrace is opt-in: the renderer diagnostic setter writes it; encode/note reads it. Ordinary rendering never pays the per-dispatch FNV loop.
    bool collectDispatchTrace=false;
    uint32_t encodedPasses=0,encodedDispatches=0;uint64_t dispatchTrace=0;
    // §6fd route census (Kernel diagnostic, OFF by default, never touched by ordinary rendering): when observeRoutes is set, encode() adds two
    // dispatches per strip after routeArgs that histogram (activeMemberMask, AABB-hit mask, overlap, ordered-S2) per tile and total the production list counters.
    // The observer WGSL is derived from kClassifyWgsl by substitution of its routing tail only; routeHistogram holds 1024 key bins then 3 production totals + 1 strip count.
    bool observeRoutes=false;WGPUComputePipeline observerPipe=nullptr,accumPipe=nullptr;WGPUBuffer routeHistogram=nullptr;
    bool enableRouteObserver(bool on);
    void resetRouteHistogram();
    // §6fd step 2a (Kernel ablation toggle, default OFF): wave-proved-empty tiles (activeMemberMask==0) leave the fallback list and ecSetup pre-writes the exact
    // packet their producer would (base offsets id*maxSub/id*maxOver, zero counts, FALLBACK|1 at zero budget). Positive budgets below 96 are no longer charged for skipped
    // empty-member work (the budget bounds work performed); budget 0 still forces refusal. Same classifier proof S1/S2 already trust for their other members.
    bool skipEmptyTiles=false;WGPUComputePipeline classifySkip=nullptr;
    // §6fe Pass A (Rung 0 telemetry, Kernel diagnostic, OFF by default, never touched by ordinary rendering): when observeWork is set, encode() runs producer variants derived by
    // substitution that also record each lane's final (work, reason) into workTrace, indexed [strip][producer S1/S2/Fallback][tileSlot*16+rayInTile] (0 = lane wrote nothing).
    // workTrace is cleared at the start of every encode(), so it holds the last frame's complete per-lane trace. It never changes what the producers publish.
    bool observeWork=false;std::array<WGPUComputePipeline,3> workPipes{};WGPUBuffer workTrace=nullptr;uint64_t workTraceBytes=0;
    bool enableWorkTrace(bool on);
    // §6fe refusal isolation (same Kernel diagnostic, default OFF; rebuild by enableWorkTrace(false) then (true)): when traceFallbackByRay is set, the Fallback producer's trace is indexed by ray id
    // instead of tileSlot*16+rayInTile (so the same ray can be compared across routing variants) and bits 23..24 hold the member index being evaluated at the lane's exit (the refusing member on a refusal).
    bool traceFallbackByRay=false;
    // §6fe step 2b (Kernel ablation toggle, default OFF; implies the step-2a empty-tile skip): tiles with exactly one wave-active member and no overlap route to the S1 producer
    // with the member id packed in bits 28..31 of the list entry (producer variant derived by substitution: member from the entry, stack 20 = the fallback producer's, Subpanel info.x = member).
    // ecConsume reads only Subpanel.info.x, so the consumer is unchanged. Same classifier proof S1/S2 already trust for their other members.
    bool singleMemberS1=false;WGPUComputePipeline classifyS1=nullptr,s1Member=nullptr;
    uint32_t workLimit=1024; // Kernel search ceiling; lowering it can only force Current refusal.
    struct Program {WGPUComputePipeline setup=nullptr,consumer=nullptr;WGPUBindGroupLayout globals=nullptr,instances=nullptr,local=nullptr;};
    std::map<std::string,Program> programs;
    ~KernelRoutedTransport();
    void releaseBuffers();
    bool ensure(WGPUDevice d,WGPUQueue q,uint32_t w,uint32_t h);
    Program* program(const std::string& source);
    bool encode(Program&,WGPUCommandEncoder,const KernelRoutedRange& globals,const KernelRoutedRange& params,const KernelRoutedRange& instances,WGPUTextureView depth,const KernelRoutedRange& specs,const KernelRoutedRange& uniformSpecs,
                const std::function<KernelRoutedRange(const void*,size_t)>& uniform);
};
