"""Routed producers in the canonical consumer/Current-fallback harness.
Pinned wide qualification uses actual routed packets/outputs and inherited
canonical authored-reference machinery. Legacy budget23/24/25 controls still
exercise cpFastFull/cpFastCompact; reverse/parallel and ownership mutation checks
must be witnessed separately on this composed path (see §6dr review).
Mode3 timing includes GPU classification/reset/indirect routing/consumer/fallback,
but excludes host coefficient preparation, initial upload and pipeline creation.
The saved32 ordered samples are one four-mode series, eight samples per mode;
reflection is not a register/spill/occupancy profile. No shipping speedup.
Antigravity / Gemini / 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-26.
Scope corrections: Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-26.
"""

import argparse, ast, copy, hashlib, importlib.util, json, math, os, re, shlex, statistics, subprocess, sys
from pathlib import Path


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def make_routed_residual_shaders(root: Path) -> dict:
    fr = load(root, 'ray_transport_gpu_faithful_residual')
    cpp = fr.generate_cpp_source(root, mutation='fixed')

    s1 = re.search(r'static const char\* kS1ProducerWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S).group(1)
    s2 = re.search(r'static const char\* kS2ProducerWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S).group(1)
    fb = re.search(r'static const char\* kFallbackProducerWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S).group(1)
    cl = re.search(r'static const char\* kClassifyWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S).group(1)
    ra = re.search(r'static const char\* kRouteArgsWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S).group(1)

    def clean(s):
        s = s.replace('@group(0) @binding(9) var<storage, read_write> writes: array<atomic<u32>>;\n', '')
        s = s.replace('atomicAdd(&writes[id], 1u);\n', '')
        s = s.replace('atomicAdd(&writes[id], 1u);', '')
        return s

    cl = cl.replace('@group(0) @binding(0) var<uniform> curtains: array<CurtainSpec, 4>;', '@group(0) @binding(0) var<storage, read> curtains: array<CurtainSpec, 4>;')
    s1 = clean(s1)
    s2 = clean(s2)
    fb = clean(fb)

    kResetCountersWgsl = r"""
struct DispatchCounters {
    s1Count: atomic<u32>,
    s2Count: atomic<u32>,
    fallbackCount: atomic<u32>,
    pad0: u32,
};

@group(0) @binding(0) var<storage, read_write> counters: DispatchCounters;

@compute @workgroup_size(1, 1, 1)
fn resetCountersMain(@builtin(global_invocation_id) gid: vec3u) {
    atomicStore(&counters.s1Count, 0u);
    atomicStore(&counters.s2Count, 0u);
    atomicStore(&counters.fallbackCount, 0u);
}
"""

    return {
        'kResetCountersWgsl': kResetCountersWgsl,
        'kClassifyWgsl': cl,
        'kRouteArgsWgsl': ra,
        'kS1ProducerWgsl': s1,
        'kS2ProducerWgsl': s2,
        'kFallbackProducerWgsl': fb,
    }


def make_routed_residual_cpp_source(base_cpp: str, root: Path, wide: bool = True) -> str:
    cr = load(root, 'ray_transport_gpu_continuation_review')
    s = cr.coarse_peel_source(base_cpp, root, wide=wide)
    s = s.replace('auto specBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, curtainSpecs.size() * sizeof(GpuCurtainSpec), curtainSpecs.data());',
                  'auto specBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, curtainSpecs.size() * sizeof(GpuCurtainSpec), curtainSpecs.data());')
    shaders = make_routed_residual_shaders(root)

    # 1. Inject shader strings right before int main
    shader_decl = f'''
static const char* kResetCountersWgsl = R"WGSL({shaders['kResetCountersWgsl']})WGSL";
static const char* kClassifyWgsl = R"WGSL({shaders['kClassifyWgsl']})WGSL";
static const char* kRouteArgsWgsl = R"WGSL({shaders['kRouteArgsWgsl']})WGSL";
static const char* kS1ProducerWgsl = R"WGSL({shaders['kS1ProducerWgsl']})WGSL";
static const char* kS2ProducerWgsl = R"WGSL({shaders['kS2ProducerWgsl']})WGSL";
static const char* kFallbackProducerWgsl = R"WGSL({shaders['kFallbackProducerWgsl']})WGSL";
'''
    if 'int main(int argc, char** argv) {' not in s:
        raise ValueError('int main hook mismatch')
    s = s.replace('int main(int argc, char** argv) {', shader_decl + '\nint main(int argc, char** argv) {', 1)

    # 2. Inject groups4x4, tileOffsets, tileRayIndices after reading R
    needle_after_R = '    size_t count = std::getenv("EC_GPU_GEOMETRY_CASE")?16:R.size();'
    inject_tile_indices = needle_after_R + '''
    std::map<uint32_t, std::vector<uint32_t>> groups4x4;
    for (size_t i = 0; i < count; ++i) {
        int px = R[i][0].get<int>(), py = R[i][1].get<int>();
        int gx = px / 10, gy = py / 10;
        uint32_t tx = gx / 4, ty = gy / 4;
        uint32_t key = (ty << 16) | tx;
        groups4x4[key].push_back(uint32_t(i));
    }
    if (groups4x4.size() != 2304u) throw std::runtime_error("pinned tile layout requires exactly 2304 tiles");
    for (const auto& [key, indices] : groups4x4) {
        if (indices.empty() || indices.size() > 16u) throw std::runtime_error("pinned tile capacity exceeded");
    }
    std::vector<uint32_t> tileOffsets(2305, 0);
    std::vector<uint32_t> tileRayIndices;
    tileRayIndices.reserve(count);
    size_t tIdx = 0;
    for (const auto& [k, rVec] : groups4x4) {
        tileOffsets[tIdx] = uint32_t(tileRayIndices.size());
        tileRayIndices.insert(tileRayIndices.end(), rVec.begin(), rVec.end());
        tIdx++;
    }
    tileOffsets[2304] = uint32_t(tileRayIndices.size());'''
    if s.count(needle_after_R) != 1:
        raise ValueError('needle_after_R hook mismatch')
    s = s.replace(needle_after_R, inject_tile_indices, 1)

    # 3. Inject buffers, pipelines, and bindgroups right after cpFastCompact build
    needle_pipelines = '    auto cpFastCompact = buildCompute(kFastPathCompactShader, "producerFastMain");'
    inject_routed_setup = needle_pipelines + '''
    // §6dq GPU-Resident Routed Residual Matrix Buffers & Pipelines
    auto offBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, tileOffsets.size() * sizeof(uint32_t), tileOffsets.data());
    auto idxBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, tileRayIndices.size() * sizeof(uint32_t), tileRayIndices.data());

    std::vector<uint32_t> initList(2304, 0u);
    auto s1TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());
    auto s2TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());
    auto fbTileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());

    struct alignas(16) DispatchCounters { uint32_t s1, s2, fb, pad; };
    DispatchCounters initCnt{0, 0, 0, 0};
    auto cntBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, sizeof(initCnt), &initCnt);
    auto argsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 36, nullptr);

    decltype(cfg) forcedCfg = cfg;
    forcedCfg.pad = 0;
    auto forcedCfgBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(forcedCfg), &forcedCfg);

    auto cpReset = buildCompute(kResetCountersWgsl, "resetCountersMain");
    auto cpClassify = buildCompute(kClassifyWgsl, "classifyTilesMain");
    auto cpArgs = buildCompute(kRouteArgsWgsl, "routeArgs");
    auto cpS1 = buildCompute(kS1ProducerWgsl, "s1ProducerMain");
    auto cpS2 = buildCompute(kS2ProducerWgsl, "s2ProducerMain");
    auto cpFallback = buildCompute(kFallbackProducerWgsl, "fallbackProducerMain");

    auto makeEntry = [](uint32_t binding, WGPUBuffer buffer, uint64_t offset, uint64_t size) -> WGPUBindGroupEntry {
        WGPUBindGroupEntry e{};
        e.binding = binding; e.buffer = buffer; e.offset = offset; e.size = size;
        return e;
    };
    auto makeGroup = [&](WGPUComputePipeline cp, const std::vector<WGPUBindGroupEntry>& entries) -> WGPUBindGroup {
        auto layout = wgpuComputePipelineGetBindGroupLayout(cp, 0);
        WGPUBindGroupDescriptor d{};
        d.layout = layout;
        d.entryCount = entries.size();
        d.entries = entries.data();
        auto bg = wgpuDeviceCreateBindGroup(gpu.device, &d);
        wgpuBindGroupLayoutRelease(layout);
        if (!bg) throw std::runtime_error("bind group creation failed");
        return bg;
    };

    auto bgReset = makeGroup(cpReset, { makeEntry(0, cntBuf, 0, sizeof(initCnt)) });
    auto bgClassify = makeGroup(cpClassify, {
        makeEntry(0, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(1, rBuf, 0, rayBytes),
        makeEntry(2, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(3, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(4, s1TileBuf, 0, 2304 * 4),
        makeEntry(5, s2TileBuf, 0, 2304 * 4),
        makeEntry(6, fbTileBuf, 0, 2304 * 4),
        makeEntry(7, cntBuf, 0, sizeof(initCnt))
    });
    auto bgArgs = makeGroup(cpArgs, {
        makeEntry(0, cntBuf, 0, sizeof(initCnt)),
        makeEntry(1, argsBuf, 0, 36)
    });

    auto bgS1 = makeGroup(cpS1, {
        makeEntry(0, cfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s1TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });
    auto bgS1Forced = makeGroup(cpS1, {
        makeEntry(0, forcedCfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s1TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });

    auto bgS2 = makeGroup(cpS2, {
        makeEntry(0, cfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s2TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });
    auto bgS2Forced = makeGroup(cpS2, {
        makeEntry(0, forcedCfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s2TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });

    auto bgFallback = makeGroup(cpFallback, {
        makeEntry(0, cfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(4, oBuf, 0, overlapBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, fbTileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });
    auto bgFallbackForced = makeGroup(cpFallback, {
        makeEntry(0, forcedCfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(4, oBuf, 0, overlapBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, fbTileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });
'''
    if s.count(needle_pipelines) != 1:
        raise ValueError('needle_pipelines hook mismatch')
    s = s.replace(needle_pipelines, inject_routed_setup, 1)

    # 4. Replace mode == 3 in execute lambda with GPU-Resident Routed Residual Matrix
    old_mode3_dispatch = '''                else if(mode==3)for(const auto& tile:tiles){
                    dispatch(cpFastCompact,forced?tile.forcedFastCompact:tile.fastCompact,tile.n);
                    dispatch(cpUnfused,tile.consumer,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);
                }'''
    new_mode3_dispatch = '''                else if(mode==3){
                    // Pass 0: Reset Counters
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpReset);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgReset, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 1: Classify Tiles (36 workgroups of 64 threads)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpClassify);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgClassify, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 36, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 2: Route Indirect Arguments (3 workgroups of 1 thread)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpArgs);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgArgs, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 3, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 3: S1 Producer (Indirect)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpS1);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS1Forced : bgS1, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 0);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 4: S2 Producer (Indirect)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpS2);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS2Forced : bgS2, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 12);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 5: Fallback Producer (Indirect)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpFallback);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgFallbackForced : bgFallback, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 24);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 6: Canonical Unfused A* Consumer
                    dispatch(cpUnfused, bgUnfused, count);
                    // Pass 7: Production-Generated Current Marcher Fallback (for any refused rays)
                    dispatch(fallbackCP, fallbackBG, count);
                }'''
    if s.count(old_mode3_dispatch) != 1:
        raise ValueError('old_mode3_dispatch hook mismatch')
    s = s.replace(old_mode3_dispatch, new_mode3_dispatch, 1)

    # 5. Replace candidate packet census generation block (lines 2805-2811) with routed producer passes
    old_census_block = '''        {
            auto bg=group(cpFastCompact,{entry(0,cfgBuf,0,sizeof(cfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,0,rayBytes),entry(3,sBuf,0,subpanelBytes),entry(4,oBuf,0,overlapBytes)});
            auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);
            wgpuComputePassEncoderSetPipeline(pass,cpFastCompact);wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass,(count+63u)/64u,1,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);
            auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);'''

    new_census_block = '''        {
            auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
            // Reset counters
            {
                auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(p, cpReset);
                wgpuComputePassEncoderSetBindGroup(p, 0, bgReset, 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);
                wgpuComputePassEncoderEnd(p);
                wgpuComputePassEncoderRelease(p);
            }
            // Classify
            {
                auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(p, cpClassify);
                wgpuComputePassEncoderSetBindGroup(p, 0, bgClassify, 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroups(p, 36, 1, 1);
                wgpuComputePassEncoderEnd(p);
                wgpuComputePassEncoderRelease(p);
            }
            // Route Args
            {
                auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(p, cpArgs);
                wgpuComputePassEncoderSetBindGroup(p, 0, bgArgs, 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroups(p, 3, 1, 1);
                wgpuComputePassEncoderEnd(p);
                wgpuComputePassEncoderRelease(p);
            }
            // S1 Producer (Indirect)
            {
                auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(p, cpS1);
                wgpuComputePassEncoderSetBindGroup(p, 0, bgS1, 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 0);
                wgpuComputePassEncoderEnd(p);
                wgpuComputePassEncoderRelease(p);
            }
            // S2 Producer (Indirect)
            {
                auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(p, cpS2);
                wgpuComputePassEncoderSetBindGroup(p, 0, bgS2, 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 12);
                wgpuComputePassEncoderEnd(p);
                wgpuComputePassEncoderRelease(p);
            }
            // Fallback Producer (Indirect)
            {
                auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                wgpuComputePassEncoderSetPipeline(p, cpFallback);
                wgpuComputePassEncoderSetBindGroup(p, 0, bgFallback, 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 24);
                wgpuComputePassEncoderEnd(p);
                wgpuComputePassEncoderRelease(p);
            }
            auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
            wgpuCommandEncoderRelease(enc);
            wgpuQueueSubmit(gpu.queue, 1, &cmd);
            wgpuCommandBufferRelease(cmd);
            wgpuDevicePoll(gpu.device, true, nullptr);'''

    if s.count(old_census_block) != 1:
        raise ValueError('old_census_block hook mismatch')
    s = s.replace(old_census_block, new_census_block, 1)

    # Also in census block, read back cntBuf and record in report
    old_census_end = 'report["coarse_packet_census_inputs"]=packets;wgpuBindGroupRelease(bg);'
    new_census_end = '''report["coarse_packet_census_inputs"]=packets;
            auto cbBytes = readBackBuffer(cntBuf, sizeof(initCnt));
            const auto* cbPtr = reinterpret_cast<const DispatchCounters*>(cbBytes.data());
            report["routed_residual"] = {
                {"s1_tiles", cbPtr->s1},
                {"s2_tiles", cbPtr->s2},
                {"fallback_tiles", cbPtr->fb}
            };
            std::cerr << "ROUTED RESIDUAL CENSUS: S1=" << cbPtr->s1 << " S2=" << cbPtr->s2 << " Fallback=" << cbPtr->fb << std::endl;'''
    if s.count(old_census_end) != 1:
        raise ValueError('old_census_end hook mismatch')
    s = s.replace(old_census_end, new_census_end, 1)

    # 6. In capacityControls, write to cfgBuf as well
    old_cap_loop = '        for(int kind:{0,1}){'
    new_cap_loop = '''        for(int kind:{0,1}){
            auto capCfg = cfg; if (kind == 0) capCfg.maxSubPerRay = 4; else capCfg.maxOverPerRay = 1;
            wgpuQueueWriteBuffer(gpu.queue, cfgBuf, 0, &capCfg, sizeof(capCfg));'''
    if s.count(old_cap_loop) != 1:
        raise ValueError('old_cap_loop hook mismatch')
    s = s.replace(old_cap_loop, new_cap_loop, 1)

    # And restore cfgBuf at the end of capacityControls
    old_cap_restore = 'report["coarse_capacity_controls"]=capacityControls;correctness(3,false);'
    new_cap_restore = 'wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&cfg,sizeof(cfg));\n        report["coarse_capacity_controls"]=capacityControls;correctness(3,false);'
    if s.count(old_cap_restore) != 1:
        raise ValueError('old_cap_restore hook mismatch')
    s = s.replace(old_cap_restore, new_cap_restore, 1)

    # 7. Update report scope string
    old_scope = '"Exact level-specific coarse bow reject before stack; original20/32/96/48 capacities"'
    new_scope = '"Exact level-specific 3-level bow mask + whole-member bow skip + depth-0 monotone register peel + routed-residual; 65536 capacity"'
    if s.count(old_scope) != 1:
        raise ValueError('old_scope hook mismatch')
    s = s.replace(old_scope, new_scope, 1)

    return s


def build_probe(root: Path, build_root: Path, work: Path) -> str:
    if work.exists() or root == work or root in work.parents:
        raise ValueError('fresh external work required')
    diagnostic_source = root / 'scratch/probes/ray_transport_metal_diagnostic.py'
    module = ast.parse(diagnostic_source.read_text())
    v = {x.targets[0].id: ast.literal_eval(x.value)
         for x in module.body
         if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}

    cpp = make_routed_residual_cpp_source(v['CPP_SOURCE'], root, wide=True)

    work.mkdir(parents=True, exist_ok=False)
    (work / 'probe.cpp').write_text(cpp)
    (work / 'source.json').write_text(json.dumps({
        'variant': 'routed-residual-wide',
        'cpp_sha256': sha(work / 'probe.cpp'),
        'zone_sha256': v['ZONE_HASH'],
        'build_root': str(build_root),
    }, indent=2) + '\n')

    engine_trees = []
    for checkout in [root, build_root]:
        if subprocess.check_output(['git','-c','core.fsmonitor=false','status','--porcelain','--','src'],cwd=checkout,text=True).strip():
            raise ValueError('dirty engine source')
        engine_trees.append(subprocess.check_output(['git','rev-parse','HEAD:src'],cwd=checkout,text=True).strip())
    if engine_trees[0] != engine_trees[1]: raise ValueError('engine source trees differ')
    b = build_root / 'build'
    db = json.loads((b / 'compile_commands.json').read_text())
    entry = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd = shlex.split(entry['command'])
    cmd[cmd.index('-c') + 1] = str(work / 'probe.cpp')
    cmd[cmd.index('-o') + 1] = str(work / 'probe.o')
    cmd += ['-std=c++20', '-O3']
    subprocess.run(cmd, cwd=b, check=True)

    link_cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('/src/entry.cpp.o'))] = str(work / 'probe.o')
    link_cmd[link_cmd.index('-o') + 1] = str(work / 'probe')
    subprocess.run(link_cmd, cwd=b, check=True)
    return v['ZONE_HASH']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--build-root', type=Path, default=Path('/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop'))
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--case', type=Path, default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    args = p.parse_args()

    root = args.root.resolve()
    build_root = args.build_root.resolve()
    work = args.work.resolve()
    case = args.case.resolve()

    wide = load(root, 'ray_transport_gpu_wide_stream')
    c6dd = load(root, 'ray_transport_gpu_6dd_continuation')

    zone = root / 'saves/zones/Northern Veil/zone.ecform'
    z = sha(zone)
    meta, rays, refs = wide.verify_case(root, case, z)
    case_sha = sha(case)

    print('Building routed-residual probe...')
    expected = build_probe(root, build_root, work)
    if z != expected or sha(zone) != z or sha(case) != case_sha:
        raise ValueError('source/case drift')

    print('Executing native probe on Apple M5 GPU...')
    output = work / 'result.json'
    env = dict(
        os.environ,
        EC_POSE_RAYS=meta['rays'],
        EC_POSE_REFERENCE=meta['reference'],
        EC_POSE_CLOCK=str(rays['time']),
        EC_MATCH_REPEAT='1',
        EC_GPU_BUDGET='1024',
        EC_GPU_SUB_CAP='64',
        EC_GPU_OVER_CAP='16',
        EC_GPU_MARGIN='.001',
    )
    env.pop('EC_GPU_GEOMETRY_CASE', None)

    with (work / 'native.log').open('x') as f:
        subprocess.run([str(work / 'probe'), str(output)], cwd=root, env=env, stdout=f, stderr=subprocess.STDOUT, check=True)

    report = json.loads(output.read_text())
    rows = json.loads(Path(str(output) + '-rows.json').read_text())
    baseline = json.loads(Path(str(output) + '-baseline-rows.json').read_text())
    ref = json.loads(Path(meta['reference']).read_text())

    print('Validating against 6dd accuracy gates and affine endpoints...')
    checked = c6dd.validate_6dd(root, report, rows, baseline, rays, ref)
    (work / 'independent.json').write_text(json.dumps(checked, indent=2) + '\n')
    print('PASS §6dq routed-residual qualification:', work / 'independent.json')


if __name__ == '__main__':
    main()
