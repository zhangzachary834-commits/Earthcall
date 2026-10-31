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
    WGPUBuffer stripUniform=nullptr;
    std::array<WGPUBuffer,3> lists{};
    std::array<WGPUComputePipeline,6> common{};
    uint32_t width=0,height=0,stripRows=0,capacity=0,tiles=0,maxStrips=0;
    static constexpr uint32_t kUniformStride=256;
    std::array<std::array<WGPUBindGroup,64>,3> prodGroups{};
    WGPUBuffer cachedSpecsBuf=nullptr;uint64_t cachedSpecsOff=0,cachedSpecsSize=0;
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
    uint32_t workLimit=1024; // Kernel search ceiling; lowering it can only force Current refusal.
    struct Program {WGPUComputePipeline setup=nullptr,consumer=nullptr;WGPUBindGroupLayout globals=nullptr,instances=nullptr,local=nullptr;std::array<WGPUBindGroup,64> localGroups{};bool localGroupsBuilt=false;};
    std::map<std::string,Program> programs;
    ~KernelRoutedTransport();
    void releaseBuffers();
    bool ensure(WGPUDevice d,WGPUQueue q,uint32_t w,uint32_t h);
    Program* program(const std::string& source);
    bool encode(Program&,WGPUCommandEncoder,const KernelRoutedRange& globals,const KernelRoutedRange& params,const KernelRoutedRange& instances,WGPUTextureView depth,const KernelRoutedRange& specs,const KernelRoutedRange& uniformSpecs,
                const std::function<KernelRoutedRange(const void*,size_t)>& uniform);
};
