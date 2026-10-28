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
    struct Program {WGPUComputePipeline setup=nullptr,consumer=nullptr;WGPUBindGroupLayout globals=nullptr,instances=nullptr,local=nullptr;};
    std::map<std::string,Program> programs;
    ~KernelRoutedTransport();
    void releaseBuffers();
    bool ensure(WGPUDevice d,WGPUQueue q,uint32_t w,uint32_t h);
    Program* program(const std::string& source);
    bool encode(Program&,WGPUCommandEncoder,const KernelRoutedRange& globals,const KernelRoutedRange& params,const KernelRoutedRange& instances,WGPUTextureView depth,const KernelRoutedRange& specs,
                const std::function<KernelRoutedRange(const void*,size_t)>& uniform);
};
