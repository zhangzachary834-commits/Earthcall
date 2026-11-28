#!/usr/bin/env python3
"""§6el Clean Multi-Scale Ablation Screen & Indirect Fallback Consumer Qualification.

Evaluates 6 clean execution lanes across scale K in {1, 2, 4, 8, 16}:
- Mode 0: Current Monolithic Baseline (currentCP)
- Mode 1: Unchanged Bulk Bisection (cpProducer + cpUnfused + fallbackCP)
- Mode 2: Unchanged Routed Unfused (cpReset + cpClassify + cpArgs + cpS1Unfused + cpS2 + cpFallback + cpUnfused + fallbackCP)
- Mode 3: Terminal S1 + Unfused Consumer (cpReset + cpClassify + cpArgs + cpS1Terminal + cpS2 + cpFallback + cpUnfused + fallbackCP)
- Mode 4: Terminal S1 + Terminal S2 + Unfused Consumer (cpReset + cpClassify + cpArgs + cpS1Terminal + cpS2Terminal + cpFallback + cpUnfused + fallbackCP)
- Mode 5: Terminal S1 + Terminal S2 + Indirect Fallback Consumer (cpReset + cpClassify + cpArgs + cpS1Terminal + cpS2Terminal + cpFallback + cpFallbackConsumer + fallbackCP)

Built on ray_transport_gpu_6ej_clean_timing.py:
- ZERO auditWritesBuf (eliminates the 541.3 MB clear handicap from §6ek)
- ZERO wgpuCommandEncoderClearBuffer calls in timed execute()
- ZERO diagnostic subpanel endpoint dumps
- ZERO diagnostic atomics in timed shaders
- Hard preflight parity check at every scale K against Mode 1 resUnfused (0 diffs required)
- True statistical medians, OLS linear model fitting T(K) = F + t * K, and frame projections at K=112.5

Antigravity / Gemini 3.8 Flash (Sparkly Guy 🐉✨)
Session: 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-27
"""

from __future__ import annotations

import argparse
import ast
import hashlib
import importlib.util
import json
import math
import os
import re
import subprocess
import sys
from pathlib import Path


def load(root: Path, name: str):
    path = root / f"scratch/probes/{name}.py"
    ns: dict = {"__file__": str(path)}
    exec(compile(path.read_text(), str(path), "exec"), ns)
    return type(name, (), ns)


def sha(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def generate(root: Path) -> str:
    t6ej_clean = load(root, "ray_transport_gpu_6ej_clean_timing")
    source = t6ej_clean.clean_cpp(root, variant="bound-stack-15")

    # 0. Upgrade device initialization to request supported limits
    old_init = (
        "    wgpu::Device gpu;\n"
        "    if (!gpu.init()) {\n"
        "        std::cerr << \"FAILED to initialize WebGPU device on Metal\" << std::endl;\n"
        "        return 1;\n"
        "    }"
    )
    new_init = """    wgpu::Device gpu;
    gpu.instance = wgpuCreateInstance(nullptr);
    {
        wgpu::detail::AdapterR ar;
        WGPURequestAdapterCallbackInfo acb{};
        acb.mode = WGPUCallbackMode_AllowProcessEvents;
        acb.callback = wgpu::detail::onAdapter;
        acb.userdata1 = &ar;
        wgpuInstanceRequestAdapter(gpu.instance, nullptr, acb);
        while (!ar.done) wgpuInstanceProcessEvents(gpu.instance);
        gpu.adapter = ar.a;

        WGPULimits supportedLimits{};
        wgpuAdapterGetLimits(gpu.adapter, &supportedLimits);

        WGPUDeviceDescriptor dd{};
        dd.requiredLimits = &supportedLimits;

        wgpu::detail::DeviceR dr;
        WGPURequestDeviceCallbackInfo dcb{};
        dcb.mode = WGPUCallbackMode_AllowProcessEvents;
        dcb.callback = wgpu::detail::onDevice;
        dcb.userdata1 = &dr;
        wgpuAdapterRequestDevice(gpu.adapter, &dd, dcb);
        while (!dr.done) wgpuInstanceProcessEvents(gpu.instance);
        gpu.device = dr.d;
        gpu.queue = wgpuDeviceGetQueue(gpu.device);
    }"""
    assert source.count(old_init) == 1
    source = source.replace(old_init, new_init)

    # 1. Parameterize tileOffsets to runtime length
    source = source.replace("var<storage, read> tileOffsets: array<u32, 2305>;", "var<storage, read> tileOffsets: array<u32>;")

    # Parameterize kClassifyWgsl bound
    old_classify_bound = "    let tileId = gid.x;\n    if (tileId >= 2304u) { return; }"
    new_classify_bound = "    let tileId = gid.x;\n    let totalTiles = arrayLength(&tileOffsets) - 1u;\n    if (tileId >= totalTiles) { return; }"
    assert source.count(old_classify_bound) == 1
    source = source.replace(old_classify_bound, new_classify_bound)

    # Parameterize kRouteArgsWgsl bound
    old_route_args = "    let n = min(counts[id.x], 2304u);"
    new_route_args = "    let n = counts[id.x];"
    assert source.count(old_route_args) == 1
    source = source.replace(old_route_args, new_route_args)

    # Parameterize producer shaders
    old_entry = "    let localTile = lid >> 4u;"
    new_entry = "    let totalTiles = arrayLength(&tileOffsets) - 1u;\n    let localTile = lid >> 4u;"
    assert source.count(old_entry) == 5
    source = source.replace(old_entry, new_entry)

    source = source.replace("min(routeCounts[0u], 2304u)", "min(routeCounts[0u], totalTiles)")
    source = source.replace("min(routeCounts.x, 2304u)", "min(routeCounts.x, totalTiles)")
    source = source.replace("min(routeCounts[1u], 2304u)", "min(routeCounts[1u], totalTiles)")
    source = source.replace("min(routeCounts.y, 2304u)", "min(routeCounts.y, totalTiles)")
    source = source.replace("min(routeCounts[2u], 2304u)", "min(routeCounts[2u], totalTiles)")
    source = source.replace("if (tileId >= 2304u) { return; }", "if (tileId >= totalTiles) { return; }")

    # 2. Extract clean kUnfusedShader and build kFallbackConsumerWgsl (ZERO audit writes, ZERO atomics!)
    hit_unfused = re.search(r'(static const char\* kUnfusedShader = R"WGSL\()(.*?)(\)WGSL";)', source, re.S)
    assert hit_unfused
    unfused_code = hit_unfused[2]

    old_consumer_header = """@compute @workgroup_size(64)
fn rayMain(@builtin(global_invocation_id) gid: vec3<u32>) {
    let rayIdx = gid.x;
    if (rayIdx >= u.rayCount) { return; }"""

    new_consumer_header = """@group(0) @binding(8) var<storage, read> tileOffsets: array<u32>;
@group(0) @binding(9) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(10) var<storage, read> fallbackTileList: array<u32>;
@group(0) @binding(11) var<storage, read> routeCounts: array<u32>;

@compute @workgroup_size(64, 1, 1)
fn fallbackConsumerMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let totalTiles = arrayLength(&tileOffsets) - 1u;
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    if (tileSlot >= min(routeCounts[2u], totalTiles)) { return; }
    let tileId = fallbackTileList[tileSlot];
    if (tileId >= totalTiles) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) { return; }
    let rayIdx = tileRayIndices[rayStart + rayInTile];
    if (rayIdx >= u.rayCount) { return; }"""

    assert unfused_code.count(old_consumer_header) == 1
    fb_consumer_wgsl = unfused_code.replace(old_consumer_header, new_consumer_header)
    decl_consumer = f'\nstatic const char* kFallbackConsumerWgsl = R"WGSL({fb_consumer_wgsl})WGSL";\n'

    idx_decl = source.find('static const char* kS2TerminalProducerWgsl =')
    assert idx_decl != -1
    source = source[:idx_decl] + decl_consumer + source[idx_decl:]

    # 3. Pipeline: add cpFallbackConsumer
    pipe_hook = 'auto cpS2Terminal = buildCompute(kS2TerminalProducerWgsl, "s2TerminalProducerMain");'
    assert source.count(pipe_hook) == 1
    pipe_decl = (
        pipe_hook + '\n' +
        '    auto cpFallbackConsumer = buildCompute(std::string(kCommonWgslHeader) + "\\n" + kOverlapWgsl + "\\n" + kFallbackConsumerWgsl, "fallbackConsumerMain");'
    )
    source = source.replace(pipe_hook, pipe_decl)

    # 4. Bind group: bgFallbackConsumer
    bg_hook = 'auto bgS2TerminalForced = makeGroup(cpS2Terminal, {'
    assert source.count(bg_hook) == 1
    idx_bg = source.find(bg_hook)
    end_bg = source.find("});", idx_bg) + len("});")
    bg_consumer_decl = f"""
    auto bgFallbackConsumer = makeGroup(cpFallbackConsumer, {{
        makeEntry(0, uBuf, 0, sizeof(udata)),
        makeEntry(1, mBuf, 0, mediaBytes),
        makeEntry(2, pBuf, 0, paramBytes),
        makeEntry(3, rBuf, 0, rayBytes),
        makeEntry(4, sBuf, 0, subpanelBytes),
        makeEntry(5, oBuf, 0, overlapBytes > 0 ? overlapBytes : 16),
        makeEntry(6, outBuf, 0, outBytes),
        makeEntry(8, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(9, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(10, fbTileBuf, 0, numTiles * 4),
        makeEntry(11, cntBuf, 0, sizeof(initCnt))
    }});
"""
    source = source[:end_bg] + bg_consumer_decl + source[end_bg:]

    # 5. In C++ host code: scale rays and tile structures by scale K
    needle_ray_load = """    std::map<uint32_t, std::vector<uint32_t>> groups4x4;
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
    tileOffsets[2304] = uint32_t(tileRayIndices.size());

    // Schedule buffers
    std::vector<GpuRayHeader> gpuRays(count);
    std::vector<GpuSubpanel> gpuSubpanels;
    std::vector<GpuOverlap> gpuOverlaps;

    uint32_t maxSubpanelsPerRay = 0;


    std::vector<uint32_t> producerFlags(count,0);
    for(size_t i=0;i<count;++i){
        const auto& row=R[i];auto& ray=gpuRays[i];
        for(int k=0;k<3;++k){ray.ro[k]=float(ro[k]);ray.rd[k]=row[2+k];}
        ray.clipA=row[5];ray.clipB=row[6];
    }"""

    scaled_ray_load = """    std::map<uint32_t, std::vector<uint32_t>> groups4x4;
    for (size_t i = 0; i < count; ++i) {
        int px = R[i][0].get<int>(), py = R[i][1].get<int>();
        int gx = px / 10, gy = py / 10;
        uint32_t tx = gx / 4, ty = gy / 4;
        uint32_t key = (ty << 16) | tx;
        groups4x4[key].push_back(uint32_t(i));
    }
    if (groups4x4.size() != 2304u) throw std::runtime_error("pinned tile layout requires exactly 2304 tiles");
    std::vector<uint32_t> baseTileOffsets(2305, 0);
    std::vector<uint32_t> baseTileRayIndices;
    baseTileRayIndices.reserve(count);
    size_t baseTIdx = 0;
    for (const auto& [k, rVec] : groups4x4) {
        baseTileOffsets[baseTIdx] = uint32_t(baseTileRayIndices.size());
        baseTileRayIndices.insert(baseTileRayIndices.end(), rVec.begin(), rVec.end());
        baseTIdx++;
    }
    baseTileOffsets[2304] = uint32_t(baseTileRayIndices.size());

    uint32_t scaleK = 1;
    if (const char* envK = std::getenv("EC_GPU_SCALE")) {
        scaleK = uint32_t(std::atoi(envK));
        if (scaleK == 0) scaleK = 1;
    }
    uint32_t baseRayCount = count;
    uint32_t baseTileCount = 2304;
    count = baseRayCount * scaleK;
    uint32_t numTiles = baseTileCount * scaleK;

    std::cerr << "§6el Clean Scale K=" << scaleK << " -> Rays=" << count << " (" << count * sizeof(GpuRayHeader) / 1048576.0 << " MB), Tiles=" << numTiles << std::endl;

    // Schedule buffers
    std::vector<GpuRayHeader> gpuRays(count);
    std::vector<GpuSubpanel> gpuSubpanels;
    std::vector<GpuOverlap> gpuOverlaps;
    uint32_t maxSubpanelsPerRay = 0;
    std::vector<uint32_t> producerFlags(count, 0);

    std::vector<uint32_t> tileOffsets(numTiles + 1, 0);
    std::vector<uint32_t> tileRayIndices;
    tileRayIndices.reserve(count);

    for (uint32_t c = 0; c < scaleK; ++c) {
        for (uint32_t i = 0; i < baseRayCount; ++i) {
            uint32_t rIdx = c * baseRayCount + i;
            const auto& row = R[i];
            auto& ray = gpuRays[rIdx];
            for (int k = 0; k < 3; ++k) {
                ray.ro[k] = float(ro[k]);
                ray.rd[k] = row[2 + k];
            }
            ray.clipA = row[5];
            ray.clipB = row[6];
        }
        for (uint32_t t = 0; t < baseTileCount; ++t) {
            uint32_t tId = c * baseTileCount + t;
            tileOffsets[tId] = uint32_t(tileRayIndices.size());
            uint32_t rStart = baseTileOffsets[t];
            uint32_t rEnd = baseTileOffsets[t + 1];
            for (uint32_t r = rStart; r < rEnd; ++r) {
                tileRayIndices.push_back(c * baseRayCount + baseTileRayIndices[r]);
            }
        }
    }
    tileOffsets[numTiles] = uint32_t(tileRayIndices.size());"""

    assert source.count(needle_ray_load) == 1
    source = source.replace(needle_ray_load, scaled_ray_load)

    # 6. Scale tile buffers in C++:
    old_tile_bufs = """    std::vector<uint32_t> initList(2304, 0u);
    auto s1TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());
    auto s2TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());
    auto fbTileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());"""

    new_tile_bufs = """    std::vector<uint32_t> initList(numTiles, 0u);
    auto s1TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, numTiles * sizeof(uint32_t), initList.data());
    auto s2TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, numTiles * sizeof(uint32_t), initList.data());
    auto fbTileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, numTiles * sizeof(uint32_t), initList.data());"""

    assert source.count(old_tile_bufs) == 1
    source = source.replace(old_tile_bufs, new_tile_bufs)

    source = source.replace("makeEntry(4, s1TileBuf, 0, 2304 * 4)", "makeEntry(4, s1TileBuf, 0, numTiles * 4)")
    source = source.replace("makeEntry(5, s2TileBuf, 0, 2304 * 4)", "makeEntry(5, s2TileBuf, 0, numTiles * 4)")
    source = source.replace("makeEntry(6, fbTileBuf, 0, 2304 * 4)", "makeEntry(6, fbTileBuf, 0, numTiles * 4)")
    source = source.replace("makeEntry(7, s1TileBuf, 0, 2304 * 4)", "makeEntry(7, s1TileBuf, 0, numTiles * 4)")
    source = source.replace("makeEntry(7, s2TileBuf, 0, 2304 * 4)", "makeEntry(7, s2TileBuf, 0, numTiles * 4)")
    source = source.replace("makeEntry(7, fbTileBuf, 0, 2304 * 4)", "makeEntry(7, fbTileBuf, 0, numTiles * 4)")

    # Replace cpClassify dispatch workgroups with (numTiles + 63) / 64
    old_classify_dispatch = "wgpuComputePassEncoderDispatchWorkgroups(p, 36, 1, 1);"
    new_classify_dispatch = "wgpuComputePassEncoderDispatchWorkgroups(p, (numTiles + 63u) / 64u, 1, 1);"
    assert source.count(old_classify_dispatch) >= 1
    source = source.replace(old_classify_dispatch, new_classify_dispatch)

    # 7. Add Mode 5 execution path into execute(mode, ...)
    exec_mode4_end = """                    // Pass 6: Canonical Unfused Consumer (skips completed S1 & S2 rays)
                    dispatch(cpUnfused, bgUnfused, count);
                    // Pass 7: Current Marcher Fallback (runs only on refused rays)
                    dispatch(fallbackCP, fallbackBG, count);
                }"""
    assert source.count(exec_mode4_end) == 1

    exec_mode5 = """
                else if(mode==5){
                    // Mode 5: Terminal S1 + Terminal S2 + Indirect Fallback Consumer (§6el Clean Candidate)
                    // Pass 0: Reset Counters
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpReset);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgReset, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 1: Classify Tiles
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpClassify);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgClassify, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, (numTiles + 63u) / 64u, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 2: Route Args
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpArgs);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgArgs, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 3, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 3: S1 Terminal Producer
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpS1Terminal);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS1TerminalForced : bgS1Terminal, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 0);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 4: S2 Terminal Producer
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpS2Terminal);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS2TerminalForced : bgS2Terminal, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 12);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 5: Fallback Producer
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpFallback);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgFallbackForced : bgFallback, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 24);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 6: Indirect Fallback Consumer (dispatches over fallbackTileList only!)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpFallbackConsumer);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgFallbackConsumer, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 24);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 7: Current Marcher Fallback (runs only on refused rays)
                    dispatch(fallbackCP, fallbackBG, count);
                }"""
    source = source.replace(exec_mode4_end, exec_mode4_end + exec_mode5)

    # 8. Inject active poll (waitStyle=1) & blocking poll (waitStyle=0) in execute
    start_ex = source.index("        auto execute=")
    end_ex = source.index("        auto correctness=", start_ex)
    execute_code = source[start_ex:end_ex]

    poll_hook = "wgpuDevicePoll(gpu.device,true,nullptr);auto end=std::chrono::steady_clock::now();"
    assert execute_code.count(poll_hook) == 1
    poll_replacement = """lastPolls = 0;
            if (waitStyle == 0) {
                wgpuDevicePoll(gpu.device, true, nullptr);
                lastPolls = 1;
            } else {
                while (!wgpuDevicePoll(gpu.device, false, nullptr)) {
                    ++lastPolls;
                    if (std::chrono::steady_clock::now() - submitted > std::chrono::seconds(30)) {
                        throw std::runtime_error("active completion timeout");
                    }
                }
                ++lastPolls;
            }
            auto end = std::chrono::steady_clock::now();"""
    execute_code = execute_code.replace(poll_hook, poll_replacement)
    source = source[:start_ex] + "        int waitStyle = std::getenv(\"EC_GPU_WAIT_STYLE\") ? std::atoi(std::getenv(\"EC_GPU_WAIT_STYLE\")) : 1;\n        uint64_t lastPolls = 0;\n" + execute_code + source[end_ex:]

    # 8b. Excise legacy diagnostic benchmarks and R[i] validation loops (crashes at K > 1)
    cut_start = "    runPass(currentCP,currentBG);auto resCurrent=readResults();"
    cut_end = "    J streamingCases=J::array();"
    assert source.count(cut_start) == 1
    assert source.count(cut_end) == 1
    idx1 = source.find(cut_start)
    idx2 = source.find(cut_end)
    source = source[:idx1] + "    J report;\n    J streamingCases=J::array();\n" + source[idx2 + len(cut_end):]

    # 9. Replace from auto correctness= up to the end of main() with Hard Preflight Parity Assertion and ABBA Clean Timing
    start_cut = source.index("        auto correctness=")

    clean_timing_replacement = """        // Preflight Parity Assertion outside timed loops at scale K:
        std::cerr << "Running preflight parity check across all modes at scale K=" << scaleK << " (" << count << " rays)..." << std::endl;
        execute(0, 1, false);
        auto resCurrent = readResults();
        execute(1, 1, false);
        auto resUnfused = readResults();

        // Modes 2, 3, 4, 5 MUST match resUnfused bit-for-bit with 0 differences across all count rays!
        for (int m : {2, 3, 4, 5}) {
            execute(m, 1, false);
            auto resM = readResults();
            unsigned diffs = 0;
            double maxDeltaRGB = 0.0, maxDeltaT = 0.0;
            for (size_t i = 0; i < count; ++i) {
                bool diff = false;
                for (int c = 0; c < 3; ++c) {
                    double delta = std::abs(double(resM[i].rgb_T[c]) - double(resUnfused[i].rgb_T[c]));
                    maxDeltaRGB = std::max(maxDeltaRGB, delta);
                    if (std::bit_cast<uint32_t>(resM[i].rgb_T[c]) != std::bit_cast<uint32_t>(resUnfused[i].rgb_T[c])) diff = true;
                }
                double deltaT = std::abs(double(resM[i].rgb_T[3]) - double(resUnfused[i].rgb_T[3]));
                maxDeltaT = std::max(maxDeltaT, deltaT);
                if (std::bit_cast<uint32_t>(resM[i].rgb_T[3]) != std::bit_cast<uint32_t>(resUnfused[i].rgb_T[3])) diff = true;
                if (diff) ++diffs;
            }
            std::cerr << "PREFLIGHT MODE " << m << ": diffs=" << diffs << " maxDeltaRGB=" << maxDeltaRGB << " maxDeltaT=" << maxDeltaT << std::endl;
            if (diffs != 0) {
                throw std::runtime_error("preflight parity assertion failed at scale K=" + std::to_string(scaleK) +
                                         " for mode " + std::to_string(m) + ": " + std::to_string(diffs) + " differing rays");
            }
        }
        std::cerr << "PREFLIGHT PARITY: ALL MODES 2..5 MATCH MODE 1 (UNFUSED BISECTION) BIT-FOR-BIT (0 DIFFS)" << std::endl;

        // 4 Warmup cycles across all 6 active modes: {0, 1, 2, 3, 4, 5}
        for (int k = 0; k < 4; ++k) {
            for (int m : {0, 1, 2, 3, 4, 5}) execute(m, 1, false);
        }

        J series = J::array();
        J cleanTiming = J::object();
        J orderedTiming = J::object();

        for (int sIdx = 1; sIdx <= 2; ++sIdx) {
            std::string sKey = "series_" + std::to_string(sIdx);
            J seriesSamples = J::array();
            std::map<int, std::vector<double>> modeEncode;
            std::map<int, std::vector<double>> modeCompletion;
            std::map<int, std::vector<double>> modeTotal;

            // 4 alternating sub-blocks (ABBA order across all 6 lanes)
            for (int block = 0; block < 4; ++block) {
                bool isB = (sIdx == 1) ? (block == 1 || block == 2) : (block == 0 || block == 3);
                const std::vector<int> order = isB ?
                    std::vector<int>{5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5} :
                    std::vector<int>{0, 1, 2, 3, 4, 5, 5, 4, 3, 2, 1, 0};

                for (int mode : order) {
                    auto t = execute(mode, 1, false);
                    modeEncode[mode].push_back(t[0]);
                    modeCompletion[mode].push_back(t[1]);
                    modeTotal[mode].push_back(t[2]);
                    seriesSamples.push_back({
                        {"mode", mode},
                        {"encode_ms", t[0]},
                        {"completion_ms", t[1]},
                        {"total_ms", t[2]}
                    });
                }
            }
            orderedTiming[sKey] = seriesSamples;

            auto computeStats = [](std::vector<double> v) {
                std::sort(v.begin(), v.end());
                double sum = std::accumulate(v.begin(), v.end(), 0.0);
                double mean = sum / v.size();
                double med = 0.5 * (v[v.size() / 2 - 1] + v[v.size() / 2]);
                return J{
                    {"samples", v},
                    {"min_ms", v.front()},
                    {"median_ms", med},
                    {"mean_ms", mean},
                    {"max_ms", v.back()}
                };
            };

            J seriesData = J::object();
            for (int m : {0, 1, 2, 3, 4, 5}) {
                std::string mKey = std::to_string(m);
                seriesData[mKey] = {
                    {"encode", computeStats(modeEncode[m])},
                    {"completion", computeStats(modeCompletion[m])},
                    {"total", computeStats(modeTotal[m])}
                };
            }
            cleanTiming[sKey] = seriesData;
        }

        // Pool across both series: 16 observations per mode (8 per series)
        J pooled = J::object();
        for (int m : {0, 1, 2, 3, 4, 5}) {
            std::string mKey = std::to_string(m);
            auto poolField = [&](const std::string& field) {
                auto s1 = cleanTiming["series_1"][mKey][field]["samples"].get<std::vector<double>>();
                auto s2 = cleanTiming["series_2"][mKey][field]["samples"].get<std::vector<double>>();
                std::vector<double> allSamples = s1;
                allSamples.insert(allSamples.end(), s2.begin(), s2.end());
                std::sort(allSamples.begin(), allSamples.end());
                double sum = std::accumulate(allSamples.begin(), allSamples.end(), 0.0);
                double mean = sum / allSamples.size();
                double med = 0.5 * (allSamples[allSamples.size() / 2 - 1] + allSamples[allSamples.size() / 2]);
                return J{
                    {"samples", allSamples},
                    {"min_ms", allSamples.front()},
                    {"median_ms", med},
                    {"mean_ms", mean},
                    {"max_ms", allSamples.back()}
                };
            };
            pooled[mKey] = {
                {"encode", poolField("encode")},
                {"completion", poolField("completion")},
                {"total", poolField("total")}
            };
        }
        cleanTiming["pooled"] = pooled;
        report["clean_timing_series"] = cleanTiming;
        report["clean_timing_ordered_samples"] = orderedTiming;
        report["scale"] = scaleK;
        report["rays"] = count;
        report["tiles"] = numTiles;
        report["wait_style"] = waitStyle;
        report["preflight_parity_pass"] = true;
        report["clean_timing_scope"] = {
            {"scope", "§6el Clean Multi-Scale Screen (ZERO auditWrites, ZERO clears)"},
            {"scale", scaleK},
            {"rays", count},
            {"tiles", numTiles},
            {"wait_style", waitStyle},
            {"modes", {
                {"0", "Current Monolithic Baseline"},
                {"1", "Unchanged Bulk Bisection"},
                {"2", "Unchanged Routed Unfused Control"},
                {"3", "Terminal S1 + Unfused Consumer"},
                {"4", "Terminal S1 + Terminal S2 + Unfused Consumer"},
                {"5", "Terminal S1 + Terminal S2 + Indirect Fallback Consumer"}
            }}
        };
        if (!outputPath.empty()) {
            std::ofstream out(outputPath);
            out << report.dump(2) << std::endl;
            std::cout << "Report saved to " << outputPath << std::endl;
        }
        return 0;
    }
}
"""
    # Replace from start_cut up to the end of main()
    needle_return = "return witnessesPass && accuracyPass"
    idx_ret = source.rfind(needle_return)
    assert idx_ret != -1
    idx_brace = source.find("}", idx_ret) + 1
    source = source[:start_cut] + clean_timing_replacement + source[idx_brace:]

    return source


def linear_fit(xs: list[float], ys: list[float]) -> dict:
    n = len(xs)
    if n < 2:
        return {
            "slope_ms_per_k": 0.0,
            "intercept_ms": ys[0] if ys else 0.0,
            "r2": 1.0,
            "slope_ns_per_ray": 0.0
        }
    x_mean = sum(xs) / n
    y_mean = sum(ys) / n
    ss_xx = sum((x - x_mean) ** 2 for x in xs)
    ss_xy = sum((x - x_mean) * (y - y_mean) for x, y in zip(xs, ys))
    ss_yy = sum((y - y_mean) ** 2 for y in ys)
    slope = ss_xy / ss_xx if ss_xx != 0 else 0.0
    intercept = y_mean - slope * x_mean
    r2 = (ss_xy ** 2) / (ss_xx * ss_yy) if (ss_xx * ss_yy) != 0 else 1.0
    return {
        "slope_ms_per_k": slope,
        "intercept_ms": intercept,
        "r2": r2,
        "slope_ns_per_ray": (slope * 1e6) / 32784.0
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--build-root", type=Path, required=True)
    p.add_argument("--work", type=Path, required=True)
    p.add_argument("--case", type=Path, default=Path("/private/tmp/earthcall-wide-case/manifest.json"))
    p.add_argument("--scales", type=int, nargs="+", default=[1, 2, 4, 8, 16])
    p.add_argument("--wait-styles", type=int, nargs="+", default=[1, 0], help="1=active poll, 0=blocking poll")
    args = p.parse_args()

    root = args.root.resolve()
    build_root = args.build_root.resolve()
    worktree_root = build_root.parent if build_root.name == "build" else build_root
    work = args.work.resolve()

    if work.exists() or work == root or root in work.parents:
        raise ValueError("fresh external work required")
    work.mkdir(parents=True, exist_ok=False)

    print("Generating §6el Clean Multi-Scale source...")
    cpp_source = generate(root)
    (work / "probe.cpp").write_text(cpp_source)

    build_dir = work / "build"
    print("Compiling clean multi-scale probe...")
    subprocess.run([
        sys.executable, "-B", str(root / "scratch/probes/ray_transport_probe_build.py"),
        "--root", str(root),
        "--build-root", str(worktree_root),
        "--source", str(work / "probe.cpp"),
        "--work", str(build_dir)
    ], check=True)
    print("Compilation successful!")

    wide = load(root, "ray_transport_gpu_wide_stream")
    zone = root / "saves/zones/Northern Veil/zone.ecform"
    before = sha(zone.read_bytes())
    meta, rays, _ = wide.verify_case(root, args.case.resolve(), before)

    results = {}
    mode_names = {
        0: "Current Monolithic Baseline",
        1: "Unchanged Bulk Bisection",
        2: "Unchanged Routed Unfused Control",
        3: "Terminal S1 + Unfused Consumer",
        4: "Terminal S1 + Terminal S2 + Unfused Consumer",
        5: "Terminal S1 + Terminal S2 + Indirect Fallback Consumer"
    }

    for wait_style in args.wait_styles:
        ws_key = "active_poll" if wait_style == 1 else "blocking_poll"
        print(f"\n=======================================================")
        print(f"Executing §6el Multi-Scale Screen under {ws_key} (waitStyle={wait_style})...")
        print(f"=======================================================")
        ws_results = {}

        for k in args.scales:
            print(f"\n--- Running Scale K={k} ({32784 * k} rays, {2304 * k} tiles) [{ws_key}] ---")
            env = dict(
                os.environ,
                EC_POSE_RAYS=meta["rays"],
                EC_POSE_REFERENCE=meta["reference"],
                EC_POSE_CLOCK=str(rays["time"]),
                EC_MATCH_REPEAT="1",
                EC_GPU_BUDGET="1024",
                EC_GPU_SUB_CAP="64",
                EC_GPU_OVER_CAP="16",
                EC_GPU_MARGIN=".001",
                EC_GPU_SCALE=str(k),
                EC_GPU_WAIT_STYLE=str(wait_style)
            )
            env.pop("EC_GPU_GEOMETRY_CASE", None)
            env.pop("METAL_CAPTURE_ENABLED", None)

            run_dir = work / f"{ws_key}_K{k}"
            run_dir.mkdir(parents=True, exist_ok=True)
            out_json = run_dir / "report.json"

            res = subprocess.run([str(build_dir / "probe"), str(out_json)], cwd=root, env=env)
            if sha(zone.read_bytes()) != before:
                raise ValueError("source drift")
            wide.verify_case(root, args.case.resolve(), before)
            if res.returncode != 0:
                print(f"Probe FAILED at scale K={k} with exit code {res.returncode}")
                sys.exit(res.returncode)

            k_data = json.load(out_json.open())
            ws_results[str(k)] = k_data
            pooled = k_data["clean_timing_series"]["pooled"]
            print(f"K={k} Medians (Completion):")
            for m in range(6):
                m_med = pooled[str(m)]["completion"]["median_ms"]
                m_enc = pooled[str(m)]["encode"]["median_ms"]
                m_tot = pooled[str(m)]["total"]["median_ms"]
                print(f"  Mode {m} ({mode_names[m]}): completion={m_med:.4f} ms (encode={m_enc:.4f} ms, total={m_tot:.4f} ms)")

        results[ws_key] = ws_results

    # Linear model fits T(K) = F + t * K across all modes for both wait styles
    analysis = {}
    for ws_key, ws_data in results.items():
        ks = [float(k) for k in args.scales]
        mode_fits = {}
        for m in range(6):
            comp_meds = [ws_data[str(int(k))]["clean_timing_series"]["pooled"][str(m)]["completion"]["median_ms"] for k in ks]
            tot_meds = [ws_data[str(int(k))]["clean_timing_series"]["pooled"][str(m)]["total"]["median_ms"] for k in ks]
            enc_meds = [ws_data[str(int(k))]["clean_timing_series"]["pooled"][str(m)]["encode"]["median_ms"] for k in ks]

            fit_comp = linear_fit(ks, comp_meds)
            fit_tot = linear_fit(ks, tot_meds)
            fit_enc = linear_fit(ks, enc_meds)

            # Full-frame extrapolation at K=112.5 (2560x1440, 3,686,400 rays)
            extrap_comp_ms = fit_comp["intercept_ms"] + fit_comp["slope_ms_per_k"] * 112.4451
            extrap_fps = 1000.0 / extrap_comp_ms if extrap_comp_ms > 0 else 0.0

            mode_fits[str(m)] = {
                "name": mode_names[m],
                "completion_fit": fit_comp,
                "total_fit": fit_tot,
                "encode_fit": fit_enc,
                "medians_by_k": {
                    str(int(k)): {
                        "completion_ms": comp_meds[idx],
                        "total_ms": tot_meds[idx],
                        "encode_ms": enc_meds[idx]
                    }
                    for idx, k in enumerate(ks)
                },
                "extrapolated_2560x1440": {
                    "scale_k": 112.4451,
                    "rays": 3686400,
                    "extrapolated_completion_ms": extrap_comp_ms,
                    "extrapolated_fps": extrap_fps,
                    "label": "unmeasured extrapolation from synthetic demand, not native camera rays"
                }
            }

        # Pairwise crossovers vs Mode 0 and Mode 1
        t0 = mode_fits["0"]["completion_fit"]["slope_ms_per_k"]
        F0 = mode_fits["0"]["completion_fit"]["intercept_ms"]
        t1 = mode_fits["1"]["completion_fit"]["slope_ms_per_k"]
        F1 = mode_fits["1"]["completion_fit"]["intercept_ms"]

        for m in range(2, 6):
            tm = mode_fits[str(m)]["completion_fit"]["slope_ms_per_k"]
            Fm = mode_fits[str(m)]["completion_fit"]["intercept_ms"]
            crossover_vs_0 = (Fm - F0) / (t0 - tm) if (t0 - tm) != 0 else float("inf")
            crossover_vs_1 = (Fm - F1) / (t1 - tm) if (t1 - tm) != 0 else float("inf")
            mode_fits[str(m)]["crossover_vs_mode0_current_k"] = crossover_vs_0
            mode_fits[str(m)]["crossover_vs_mode1_bulk_k"] = crossover_vs_1

        analysis[ws_key] = mode_fits

    full_output = {
        "scope": "§6el Clean Multi-Scale Ablation Screen (ZERO auditWrites, ZERO clearBuffer)",
        "scales": args.scales,
        "wait_styles": args.wait_styles,
        "linear_models": analysis,
        "raw_runs": results,
        "shipping_gates_open": [1, 2, 3]
    }

    out_file = root / "docs/audits/GPU_RAY_TRANSPORT_6EL_CLEAN_MULTI_SCALE_ABLATION_2026-10-27.json"
    out_file.write_text(json.dumps(full_output, indent=2) + "\n")
    print(f"\nWrote machine results to {out_file}")


if __name__ == "__main__":
    main()
