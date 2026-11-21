"""On-The-Fly Chronological Consumption & 4-Mode Benchmark (§6cz)

Eliminates thread-private dynamic buffering (local_subpanels[64] and
local_overlaps[16]) and the two-stream re-merge loop from fused execution,
lowering thread-private schedule storage to 0 bytes/thread.

Evaluates Gauss-Legendre quadrature updates and overlap updates immediately
as intervals are generated along the ray in chronological ray order.

Maintains 100.0% same-device bit-exact schedule and output parity against
the guarded confirm baseline on all 32,784 distinct native rays.
"""

import argparse
import ast
import copy
import gzip
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shlex
import statistics
import subprocess
import sys


def load(root, name):
    import importlib.util
    p = root / f'scratch/probes/{name}.py'
    spec = importlib.util.spec_from_file_location(name, p)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def require_contract(shader):
    """Enforces 64/16 capacity contract on buffered fused shader."""
    sub_m = re.search(r'var local_subpanels:\s*array<Subpanel,\s*(\d+)>;', shader)
    over_m = re.search(r'var local_overlaps:\s*array<Overlap,\s*(\d+)>;', shader)
    if not sub_m or not over_m or int(sub_m.group(1)) != 64 or int(over_m.group(1)) != 16:
        raise ValueError(f"Buffered contract failed: expected 64/16, found {sub_m} / {over_m}")


def require_on_the_fly_contract(shader):
    """Enforces 0 B private buffering contract on on-the-fly shader."""
    if 'local_subpanels' in shader:
        raise ValueError("On-the-fly contract failed: local_subpanels array found in shader")
    if 'local_overlaps' in shader:
        raise ValueError("On-the-fly contract failed: local_overlaps array found in shader")
    if 'singles + n > 64u' not in shader:
        raise ValueError("On-the-fly contract failed: singles + n > 64u capacity check missing")
    if 'overs >= 16u' not in shader:
        raise ValueError("On-the-fly contract failed: overs >= 16u capacity check missing")
    if 'FALLBACK | reason' not in shader:
        raise ValueError("On-the-fly contract failed: FALLBACK | reason flag writer missing")
    if 'results[id] = vec4<f32>(L, T);' not in shader:
        raise ValueError("On-the-fly contract failed: results[id] = vec4<f32>(L, T) writer missing")


def make_fused_wgsl_64(base_fused_wgsl):
    s = base_fused_wgsl
    s = s.replace('var local_subpanels: array<Subpanel, 32>;', 'var local_subpanels: array<Subpanel, 64>;')
    s = s.replace('var local_overlaps: array<Overlap, 8>;', 'var local_overlaps: array<Overlap, 16>;')
    s = s.replace('singles + n > 32u || singles + n > cfg.maxSubPerRay', 'singles + n > 64u || singles + n > cfg.maxSubPerRay')
    s = s.replace('overs >= 8u || overs >= cfg.maxOverPerRay', 'overs >= 16u || overs >= cfg.maxOverPerRay')
    require_contract(s)
    return s


ON_THE_FLY_CONSUMPTION = '''    // On-the-fly chronological integration (zero local subpanel/overlap arrays)
    var singles = 0u;
    var overs = 0u;
    var pIdx = vec4u(0u);

    let gx0 = 0.5 * (1.0 - 0.5773502691896257);
    let gx1 = 0.5 * (1.0 + 0.5773502691896257);

    var T = 1.0;
    var L = vec3<f32>(0.0);

    // Linear mask sweep with immediate chronological consumption
    for (var i = 1u; i < ne && reason == 0u; i++) {
        let a = knots[i - 1u];
        let b = knots[i];
        if (!(b > a)) { continue; }
        let mid = a + 0.5 * (b - a);
        var mask = 0u;
        for (var m = 0u; m < 4u; m++) {
            let start = memStart[m];
            let numPieces = memCount[m];
            var cur = pIdx[m];
            loop {
                if (cur >= numPieces || pieces[start + cur].y >= mid) { break; }
                cur++;
            }
            pIdx[m] = cur;
            if (cur < numPieces) {
                let p = pieces[start + cur];
                if (mid >= p.x && mid <= p.y) {
                    mask |= (1u << m);
                }
            }
        }
        if (mask == 0u) { continue; }
        if (countOneBits(mask) == 1u) {
            let n = max(1u, u32(ceil((b - a) / 4.0)));
            if (singles + n > 64u || singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
            let member = firstTrailingBit(mask);
            let m = media[member];
            let pOffset = u32(m.freq_offset.y);
            let freq = m.freq_offset.x;
            let alpha = m.color_alpha.w;
            let color = m.color_alpha.xyz;

            for (var k = 0u; k < n; k++) {
                let s0 = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
                let s1 = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
                let sub_h = s1 - s0;

                let s_m = s0 + gx0 * sub_h;
                let s_p = s0 + gx1 * sub_h;
                let w_m = ro + s_m * d - m.origin.xyz;
                let w_p = ro + s_p * d - m.origin.xyz;

                let env_m = evalEnvelope(w_m, pOffset);
                let env_p = evalEnvelope(w_p, pOffset);

                var D_m = 0.0;
                if (env_m > 0.0) {
                    let nm = cnoise3(freq * w_m);
                    D_m = env_m * clamp(0.7 + 0.3 * nm, 0.0, 1.0);
                }
                var D_p = 0.0;
                if (env_p > 0.0) {
                    let np = cnoise3(freq * w_p);
                    D_p = env_p * clamp(0.7 + 0.3 * np, 0.0, 1.0);
                }

                // Sixth-one Sun: Strict D > 0 gate
                let sig_m = select(0.0, max(alpha * D_m, 1e-6), D_m > 0.0);
                let em_m  = select(vec3<f32>(0.0), color * (alpha * D_m), D_m > 0.0);
                let sig_p = select(0.0, max(alpha * D_p, 1e-6), D_p > 0.0);
                let em_p  = select(vec3<f32>(0.0), color * (alpha * D_p), D_p > 0.0);

                let sig_avg = 0.5 * (sig_m + sig_p);
                let em_avg  = 0.5 * (em_m + em_p);
                let tau = sub_h * sig_avg;
                let atten = exp(-tau);

                // Sixth-one Sun: Numerically safe small-depth gain in f32
                var gain: f32;
                gain = sub_h * phiSafe(tau);

                L = L + T * em_avg * gain;
                T = T * atten;

                singles++;
            }
        } else {
            if (overs >= 16u || overs >= cfg.maxOverPerRay) { reason |= 32u; break; }
            let o = Overlap(vec2f(a, b), vec2u(mask, 0u));
            let state = advanceOverlap(vec4<f32>(L, T), ro, d, o);
            L = state.xyz;
            T = state.w;
            overs++;
        }
    }

    // Branch on reason: fallback path vs normal path
    if (reason != 0u) {
        // Fallback flag writer: write FALLBACK flag; discard partial transport so fallback writes entire result
        rays[id].counts = vec4u(0u, 0u, 0u, FALLBACK | reason);
        return;
    }

    // Normal path flag writer: clear FALLBACK flag (counts.w bit 31 = 0); fallbackMain will skip
    rays[id].counts = vec4u(0u, singles, 0u, overs);
    results[id] = vec4<f32>(L, T);
'''


def make_on_the_fly_wgsl(fused_64):
    start_anchor = '    // Thread-local interval storage'
    end_anchor = '    results[id] = vec4<f32>(L, T);'
    idx1 = fused_64.find(start_anchor)
    idx2 = fused_64.find(end_anchor)
    if idx1 == -1 or idx2 == -1:
        raise ValueError("Anchors for on-the-fly replacement not found in fused_64")
    s = fused_64[:idx1] + ON_THE_FLY_CONSUMPTION + fused_64[idx2 + len(end_anchor):]
    s = s.replace('fn fusedMain(@builtin(global_invocation_id) gid: vec3u) {',
                  'fn onTheFlyMain(@builtin(global_invocation_id) gid: vec3u) {')
    require_on_the_fly_contract(s)
    return s


def make_on_the_fly_trace_wgsl(on_the_fly_wgsl):
    s = on_the_fly_wgsl
    extra_bindings = '''
@group(0) @binding(6) var<storage, read_write> diag_subpanels: array<Subpanel>;
@group(0) @binding(7) var<storage, read_write> diag_overlaps: array<Overlap>;
'''
    s = s.replace('@group(0) @binding(5) var<storage, read_write> results: array<vec4<f32>>;',
                  '@group(0) @binding(5) var<storage, read_write> results: array<vec4<f32>>;' + extra_bindings)
    s = s.replace('fn onTheFlyMain(@builtin(global_invocation_id) gid: vec3u) {',
                  'fn onTheFlyTraceMain(@builtin(global_invocation_id) gid: vec3u) {')

    sub_hook = 'let sub_h = s1 - s0;'
    sub_trace = 'let sub_h = s1 - s0;\n                let subBase = id * 64u;\n                diag_subpanels[subBase + singles] = Subpanel(vec2f(s0, s1), vec2u(member, 0u));'
    if sub_hook not in s:
        raise ValueError("Subpanel trace anchor not found")
    s = s.replace(sub_hook, sub_trace)

    over_hook = 'let o = Overlap(vec2f(a, b), vec2u(mask, 0u));'
    over_trace = 'let o = Overlap(vec2f(a, b), vec2u(mask, 0u));\n            let overBase = id * 16u;\n            diag_overlaps[overBase + overs] = o;'
    if over_hook not in s:
        raise ValueError("Overlap trace anchor not found")
    s = s.replace(over_hook, over_trace)
    return s


def source(base, root):
    q = load(root, 'ray_transport_gpu_pose_qualification')
    fn = load(root, 'ray_transport_gpu_producer_fused_normal')
    s = fn.source(base, root)

    # 1. Update fused shader to 64/16 contract
    fused_64 = make_fused_wgsl_64(fn.FUSED_WGSL)
    fused_decl = 'static const char* kFusedNormalShader = R"WGSL(\n' + fn.FUSED_WGSL.strip() + '\n)WGSL";'
    fused_64_decl = 'static const char* kFusedNormalShader = R"WGSL(\n' + fused_64.strip() + '\n)WGSL";'
    s = q.once(s, fused_decl, fused_64_decl)

    # 2. Add On-The-Fly and Trace shaders
    fly_wgsl = make_on_the_fly_wgsl(fused_64)
    trace_wgsl = make_on_the_fly_trace_wgsl(fly_wgsl)

    shaders_block = '\nstatic const char* kOnTheFlyShader = R"WGSL(\n' + fly_wgsl.strip() + '\n)WGSL";\n\n' + \
                    'static const char* kOnTheFlyTraceShader = R"WGSL(\n' + trace_wgsl.strip() + '\n)WGSL";\n\n'
    s = q.once(s, '    auto cpFusedNormal = buildCompute(kFusedNormalShader, "fusedMain");',
                  shaders_block + '    auto cpFusedNormal = buildCompute(kFusedNormalShader, "fusedMain");\n' +
                                  '    auto cpOnTheFly = buildCompute(kOnTheFlyShader, "onTheFlyMain");\n' +
                                  '    auto cpOnTheFlyPacketTrace = buildCompute(kOnTheFlyTraceShader, "onTheFlyTraceMain");')

    # 3. Add readBackBuffer helper
    readback_helper = r'''
    auto readBackBuffer = [&](WGPUBuffer src, uint64_t size) -> std::vector<uint8_t> {
        auto rb = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, size, nullptr);
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(enc, src, 0, rb, 0, size);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        MapContext mc;
        WGPUBufferMapCallbackInfo cbInfo = {};
        cbInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        cbInfo.callback = onMapped;
        cbInfo.userdata1 = &mc;
        wgpuBufferMapAsync(rb, WGPUMapMode_Read, 0, size, cbInfo);
        while (!mc.done) wgpuDevicePoll(gpu.device, true, nullptr);
        if (!mc.ok) throw std::runtime_error("buffer mapping failed");

        const uint8_t* ptr = static_cast<const uint8_t*>(wgpuBufferGetConstMappedRange(rb, 0, size));
        std::vector<uint8_t> data(ptr, ptr + size);
        wgpuBufferUnmap(rb);
        wgpuBufferRelease(rb);
        return data;
    };
'''
    s = q.once(s, '    auto readResults = [&]() -> std::vector<GpuResult> {',
                  readback_helper + '\n    auto readResults = [&]() -> std::vector<GpuResult> {')

    # 4. Update Tile struct to hold onTheFly bind groups
    s = q.once(s, 'struct Tile {uint32_t start,n;WGPUBindGroup producer,forced,consumer,fallback,fused,forcedFused;};',
                  'struct Tile {uint32_t start,n;WGPUBindGroup producer,forced,consumer,fallback,fused,forcedFused,onTheFly,forcedOnTheFly;};')

    # 5. In tile creation loop, create onTheFly bind groups and push them
    old_tile_push = r'''auto fused = [&](WGPUBuffer configuration){return group(cpFusedNormal,{
                entry(0,configuration,0,sizeof(pcfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(3,mBuf,0,mediaBytes),
                entry(4,pBuf,0,paramBytes),entry(5,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult))});};
            tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP),fused(cb),fused(fb)});'''

    new_tile_push = r'''auto fused = [&](WGPUBuffer configuration){return group(cpFusedNormal,{
                entry(0,configuration,0,sizeof(pcfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(3,mBuf,0,mediaBytes),
                entry(4,pBuf,0,paramBytes),entry(5,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult))});};
            auto onTheFly = [&](WGPUBuffer configuration){return group(cpOnTheFly,{
                entry(0,configuration,0,sizeof(pcfg)),entry(1,specBuf,0,curtainSpecs.size()*sizeof(GpuCurtainSpec)),
                entry(2,rBuf,rayOffset,uint64_t(n)*sizeof(GpuRayHeader)),entry(3,mBuf,0,mediaBytes),
                entry(4,pBuf,0,paramBytes),entry(5,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult))});};
            tiles.push_back({start,n,producer(cb),producer(fb),consumer,current(fallbackCP),fused(cb),fused(fb),onTheFly(cb),onTheFly(fb)});'''
    s = q.once(s, old_tile_push, new_tile_push)

    # 6. Update tile bind group release
    s = q.once(s, 'for(auto bg:{tile.producer,tile.forced,tile.consumer,tile.fallback,tile.fused,tile.forcedFused})wgpuBindGroupRelease(bg);',
                  'for(auto bg:{tile.producer,tile.forced,tile.consumer,tile.fallback,tile.fused,tile.forcedFused,tile.onTheFly,tile.forcedOnTheFly})wgpuBindGroupRelease(bg);')

    # 7. Update execute lambda to support 4 modes
    old_execute_loop = r'''            for(int k=0;k<repeats;++k){
                if(mode==0)dispatch(currentCP,currentBG,count);
                else if(mode==1){dispatch(cpProducer,bgProducer,count);dispatch(cpUnfused,bgUnfused,count);dispatch(fallbackCP,fallbackBG,count);}
                else for(const auto& tile:tiles){
                    dispatch(cpFusedNormal,forced?tile.forcedFused:tile.fused,tile.n);
                    dispatch(fallbackCP,tile.fallback,tile.n);
                }
            }'''

    new_execute_loop = r'''            for(int k=0;k<repeats;++k){
                if(mode==0)dispatch(currentCP,currentBG,count);
                else if(mode==1){dispatch(cpProducer,bgProducer,count);dispatch(cpUnfused,bgUnfused,count);dispatch(fallbackCP,fallbackBG,count);}
                else if(mode==2)for(const auto& tile:tiles){
                    dispatch(cpFusedNormal,forced?tile.forcedFused:tile.fused,tile.n);
                    dispatch(fallbackCP,tile.fallback,tile.n);
                }
                else if(mode==3)for(const auto& tile:tiles){
                    dispatch(cpOnTheFly,forced?tile.forcedOnTheFly:tile.onTheFly,tile.n);
                    dispatch(fallbackCP,tile.fallback,tile.n);
                }
            }'''
    s = q.once(s, old_execute_loop, new_execute_loop)

    # 8. Limit capacity to primary 32,768 (2 tiles: 32,768 + 16 tail)
    s = q.once(s, 'for(uint32_t capacity:{8192u,16384u,32768u})', 'for(uint32_t capacity:{32768u})')

    # 9. Declare variables outside capacity loop
    decl_outside = r'''
    J normalRows;
    J forcedRows;
    J mode2NormalRows;
    J mode2ForcedRows;
    std::vector<GpuRayHeader> finalHeaders;
    size_t normalFallbacks = 0;
    size_t totalSingles = 0;
    size_t totalOvers = 0;
'''
    s = q.once(s, 'J streamingCases=J::array();', 'J streamingCases=J::array();\n' + decl_outside)

    # 10. Update correctness checks to verify Mode 3 (On-The-Fly) and Mode 2 (Buffered Fused 64/16)
    idx_corr = s.find('auto correctness=[&](bool forced){')
    idx_norm = s.find('auto normalRows=correctness(false);')
    if idx_corr == -1 or idx_norm == -1:
        raise ValueError("correctness block not found")
    
    new_correctness_block = r'''auto correctness=[&](int testMode, bool forced){
            std::vector<GpuResult> poison(executionRays);for(auto& p:poison)for(float& v:p.rgb_T)v=NAN;
            wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poison.data(),outBytes);execute(testMode,1,forced);auto result=readResults();
            const auto& expected=forced?resCurrent:resUnfused;
            for(size_t i=0;i<count;++i) {
                bool mismatch = false;
                for(int k=0;k<4;++k) {
                    if(!std::isfinite(result[i].rgb_T[k])||std::bit_cast<uint32_t>(result[i].rgb_T[k])!=std::bit_cast<uint32_t>(expected[i].rgb_T[k])) mismatch = true;
                }
                if (mismatch) {
                    std::cerr << "MISMATCH mode=" << testMode << " forced=" << forced << " ray=" << i << " (" << R[i][0] << "," << R[i][1] << ")"
                              << " res=[" << result[i].rgb_T[0] << "," << result[i].rgb_T[1] << "," << result[i].rgb_T[2] << "," << result[i].rgb_T[3] << "]"
                              << " exp=[" << expected[i].rgb_T[0] << "," << expected[i].rgb_T[1] << "," << expected[i].rgb_T[2] << "," << expected[i].rgb_T[3] << "]"
                              << " resBits=[" << std::hex << std::bit_cast<uint32_t>(result[i].rgb_T[0]) << "," << std::bit_cast<uint32_t>(result[i].rgb_T[1]) << "," << std::bit_cast<uint32_t>(result[i].rgb_T[2]) << "," << std::bit_cast<uint32_t>(result[i].rgb_T[3]) << "]"
                              << " expBits=[" << std::hex << std::bit_cast<uint32_t>(expected[i].rgb_T[0]) << "," << std::bit_cast<uint32_t>(expected[i].rgb_T[1]) << "," << std::bit_cast<uint32_t>(expected[i].rgb_T[2]) << "," << std::bit_cast<uint32_t>(expected[i].rgb_T[3]) << "]" << std::dec << "\n";
                    throw std::runtime_error("stream mode " + std::to_string(testMode) + " changed or missed a result");
                }
            }
            J rows=J::array();for(size_t i=0;i<count;++i)rows.push_back({R[i][0],R[i][1],result[i].rgb_T[0],result[i].rgb_T[1],result[i].rgb_T[2],result[i].rgb_T[3]});
            return rows;
        };
        '''
    s = s[:idx_corr] + new_correctness_block + s[idx_norm:]

    # 11. Replace correctness execution block
    old_correctness_call = r'''        auto normalRows=correctness(false);auto forcedRows=correctness(true);
        auto rb=makeBuffer(WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead,rayBytes,nullptr);
        auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(enc,rBuf,0,rb,0,rayBytes);
        auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
        MapContext map;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onMapped;cb.userdata1=&map;
        wgpuBufferMapAsync(rb,WGPUMapMode_Read,0,rayBytes,cb);while(!map.done)wgpuDevicePoll(gpu.device,true,nullptr);
        if(!map.ok)throw std::runtime_error("stream header readback refused");
        const auto* headers=static_cast<const GpuRayHeader*>(wgpuBufferGetConstMappedRange(rb,0,rayBytes));
        unsigned forcedCount=0;for(size_t i=0;i<count;++i)if(headers[i].overlapCount&0x80000000u)++forcedCount;
        wgpuBufferUnmap(rb);wgpuBufferRelease(rb);
        if(forcedCount!=count)throw std::runtime_error("stream did not force every fallback");'''

    new_correctness_call = r'''
        // 1. Execute Mode 3 (On-The-Fly) normal and read headers immediately before forced fallback overwrites rBuf!
        normalRows = correctness(3, false);
        auto normalHeaderBytes = readBackBuffer(rBuf, rayBytes);
        const auto* normalHeaders = reinterpret_cast<const GpuRayHeader*>(normalHeaderBytes.data());
        finalHeaders.assign(normalHeaders, normalHeaders + count);

        normalFallbacks = 0;
        totalSingles = 0;
        totalOvers = 0;
        for (size_t i = 0; i < count; ++i) {
            if (normalHeaders[i].overlapCount & 0x80000000u) normalFallbacks++;
            totalSingles += normalHeaders[i].subpanelCount;
            totalOvers += normalHeaders[i].overlapCount;
        }
        if (normalFallbacks != 0) throw std::runtime_error("normal_fallbacks != 0 in on-the-fly path");

        // 2. Execute Mode 3 (On-The-Fly) forced fallback mode
        forcedRows = correctness(3, true);
        auto forcedHeaderBytes = readBackBuffer(rBuf, rayBytes);
        const auto* forcedHeaders = reinterpret_cast<const GpuRayHeader*>(forcedHeaderBytes.data());
        unsigned forcedCount = 0;
        for (size_t i = 0; i < count; ++i) {
            if (forcedHeaders[i].overlapCount & 0x80000000u) forcedCount++;
        }
        if (forcedCount != count) throw std::runtime_error("stream did not force every fallback");

        // 3. Execute Mode 2 (Buffered Fused 64/16) normal and forced runs
        mode2NormalRows = correctness(2, false);
        mode2ForcedRows = correctness(2, true);
'''
    s = q.once(s, old_correctness_call, new_correctness_call)

    # 12. Update 4-mode timing loop (modes 0, 1, 2, 3 in balanced reverse-symmetric order)
    old_timing_loop = r'''        // Separate warm correctness from measured ABBA/BAAB complete costs.
        for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);}
        J series=J::array();
        for(int repeats:{1,2,4}){
            J samples=J::array();
            for(int block=0;block<4;++block){
                const std::vector<int> order=block%2?std::vector<int>{2,1,0,0,1,2}:std::vector<int>{0,1,2,2,1,0};
                for(int mode:order){std::cout<<"STREAM capacity="<<capacity<<" repeats="<<repeats<<" mode="<<mode<<std::endl;auto t=execute(mode,repeats,false);samples.push_back({{"mode",mode},{"encode_ms",t[0]},{"completion_ms",t[1]},{"total_ms",t[2]}});}
            }
            series.push_back({{"repeats",repeats},{"samples",samples}});
        }'''

    new_timing_loop = r'''        // Separate warm correctness from measured balanced complete costs across 4 modes
        for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);execute(3,1,false);}
        J series=J::array();
        for(int repeats:{1,2,4}){
            J samples=J::array();
            for(int block=0;block<4;++block){
                const std::vector<int> order = (block % 2) ?
                    std::vector<int>{3, 2, 1, 0, 0, 1, 2, 3} :
                    std::vector<int>{0, 1, 2, 3, 3, 2, 1, 0};
                for(int mode:order){
                    std::cout<<"STREAM capacity="<<capacity<<" repeats="<<repeats<<" mode="<<mode<<std::endl;
                    auto t=execute(mode,repeats,false);
                    samples.push_back({{"mode",mode},{"encode_ms",t[0]},{"completion_ms",t[1]},{"total_ms",t[2]}});
                }
            }
            series.push_back({{"repeats",repeats},{"samples",samples}});
        }'''
    s = q.once(s, old_timing_loop, new_timing_loop)

    # 13. Wire up Diagnostic Packet Trace on cpOnTheFlyPacketTrace and Synthetic Controls on cpOnTheFly
    diagnostic_block = r'''
    // --- Diagnostic Packet Trace and Synthetic Controls on cpOnTheFly ---
    auto resultRows = [&](const std::vector<GpuResult>& data) {
        J a = J::array();
        for (size_t i = 0; i < count; ++i)
            a.push_back({R[i][0], R[i][1], data[i].rgb_T[0], data[i].rgb_T[1], data[i].rgb_T[2], data[i].rgb_T[3]});
        return a;
    };

    const uint64_t diagSubSingleBytes = uint64_t(count) * 64u * sizeof(GpuSubpanel);
    const uint64_t diagOverBytes = uint64_t(count) * 16u * sizeof(GpuOverlap);
    auto diagSubpanelsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, diagSubSingleBytes, nullptr);
    auto diagOverlapsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, diagOverBytes, nullptr);

    WGPUBindGroupEntry bgptEntries[8] = {};
    bgptEntries[0].binding = 0; bgptEntries[0].buffer = cfgBuf; bgptEntries[0].size = sizeof(cfg);
    bgptEntries[1].binding = 1; bgptEntries[1].buffer = specBuf; bgptEntries[1].size = curtainSpecs.size() * sizeof(GpuCurtainSpec);
    bgptEntries[2].binding = 2; bgptEntries[2].buffer = rBuf; bgptEntries[2].size = rayBytes;
    bgptEntries[3].binding = 3; bgptEntries[3].buffer = mBuf; bgptEntries[3].size = mediaBytes;
    bgptEntries[4].binding = 4; bgptEntries[4].buffer = pBuf; bgptEntries[4].size = paramBytes;
    bgptEntries[5].binding = 5; bgptEntries[5].buffer = outBuf; bgptEntries[5].size = outBytes;
    bgptEntries[6].binding = 6; bgptEntries[6].buffer = diagSubpanelsBuf; bgptEntries[6].size = diagSubSingleBytes;
    bgptEntries[7].binding = 7; bgptEntries[7].buffer = diagOverlapsBuf; bgptEntries[7].size = diagOverBytes;

    auto bglTrace = wgpuComputePipelineGetBindGroupLayout(cpOnTheFlyPacketTrace, 0);
    WGPUBindGroupDescriptor bgptd = {};
    bgptd.layout = bglTrace;
    bgptd.entryCount = 8;
    bgptd.entries = bgptEntries;
    WGPUBindGroup bgOnTheFlyPacketTrace = wgpuDeviceCreateBindGroup(gpu.device, &bgptd);
    wgpuBindGroupLayoutRelease(bglTrace);

    // Run Packet Trace
    {
        std::vector<GpuResult> poison(executionRays); for(auto& p:poison) for(float& v:p.rgb_T) v=NAN;
        wgpuQueueWriteBuffer(gpu.queue, outBuf, 0, poison.data(), outBytes);
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpOnTheFlyPacketTrace);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bgOnTheFlyPacketTrace, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        auto traceResults = readResults();
        report["trace_output_rows"] = resultRows(traceResults);
        for (size_t i = 0; i < count; ++i) {
            for (int k = 0; k < 4; ++k) {
                if (std::bit_cast<uint32_t>(traceResults[i].rgb_T[k]) != std::bit_cast<uint32_t>(resUnfused[i].rgb_T[k])) {
                    std::cerr << "TRACE MISMATCH ray=" << i << " (" << R[i][0] << "," << R[i][1] << ")"
                              << " trace=[" << traceResults[i].rgb_T[0] << "," << traceResults[i].rgb_T[1] << "," << traceResults[i].rgb_T[2] << "," << traceResults[i].rgb_T[3] << "]"
                              << " unfused=[" << resUnfused[i].rgb_T[0] << "," << resUnfused[i].rgb_T[1] << "," << resUnfused[i].rgb_T[2] << "," << resUnfused[i].rgb_T[3] << "]"
                              << " traceBits=[" << std::hex << std::bit_cast<uint32_t>(traceResults[i].rgb_T[0]) << "," << std::bit_cast<uint32_t>(traceResults[i].rgb_T[1]) << "," << std::bit_cast<uint32_t>(traceResults[i].rgb_T[2]) << "," << std::bit_cast<uint32_t>(traceResults[i].rgb_T[3]) << "]"
                              << " unfusedBits=[" << std::bit_cast<uint32_t>(resUnfused[i].rgb_T[0]) << "," << std::bit_cast<uint32_t>(resUnfused[i].rgb_T[1]) << "," << std::bit_cast<uint32_t>(resUnfused[i].rgb_T[2]) << "," << std::bit_cast<uint32_t>(resUnfused[i].rgb_T[3]) << "]" << std::dec << std::endl;
                    throw std::runtime_error("on-the-fly packet trace output differs from normal rows");
                }
            }
        }
    }

    auto subBytes = readBackBuffer(diagSubpanelsBuf, diagSubSingleBytes);
    auto overBytes = readBackBuffer(diagOverlapsBuf, diagOverBytes);
    const auto* diagSubs = reinterpret_cast<const GpuSubpanel*>(subBytes.data());
    const auto* diagOvers = reinterpret_cast<const GpuOverlap*>(overBytes.data());

    // Construct fused_packet_census_inputs
    J fusedPacketCensus = J::array();
    for (size_t i = 0; i < count; ++i) {
        uint32_t px = (uint32_t)R[i][0];
        uint32_t py = (uint32_t)R[i][1];
        const auto& hdr = finalHeaders[i];
        uint32_t fallbackFlag = hdr.overlapCount & 0x80000000u;
        J rawSingles = J::array();
        J rawOverlaps = J::array();
        if (fallbackFlag != 0u) {
            fusedPacketCensus.push_back({px, py, hdr.overlapCount, rawSingles, rawOverlaps});
        } else {
            uint32_t singles = hdr.subpanelCount;
            uint32_t overs = hdr.overlapCount;
            for (uint32_t s = 0; s < singles; s++) {
                const auto& sub = diagSubs[i * 64 + s];
                rawSingles.push_back({sub.s0, sub.s1, sub.mediumIndex});
            }
            for (uint32_t o = 0; o < overs; o++) {
                const auto& over = diagOvers[i * 16 + o];
                rawOverlaps.push_back({over.s0, over.s1, over.activeMask});
            }
            fusedPacketCensus.push_back({px, py, overs, rawSingles, rawOverlaps});
        }
    }
    report["fused_packet_census_inputs"] = fusedPacketCensus;
    report["normal_headers_readback"] = {
        {"normal_fallbacks", normalFallbacks},
        {"total_singles", totalSingles},
        {"total_overs", totalOvers},
        {"contract_pass", true}
    };

    // --- Synthetic Budget Controls on cpOnTheFly ---
    J budgetControlResults = J::array();
    std::vector<GpuCurtainSpec> budgetSpecs = curtainSpecs;
    for (auto& s : budgetSpecs) {
        s.origin = glm::vec4(1000.0f, 1000.0f, 1000.0f, 0.0f);
        s.half = glm::vec4(1.0f);
    }
    budgetSpecs[0].origin = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    budgetSpecs[0].half = glm::vec4(10.0f, 10.0f, 10.0f, 0.0f);
    budgetSpecs[0].limits = glm::vec4(2.0f, 2.0f, 0.0f, 0.2f);
    budgetSpecs[0].curv = glm::vec4(0.0f, 0.001f, 0.0f, 0.0f);

    std::vector<GpuRayHeader> budgetRays(16);
    for (auto& ray : budgetRays) {
        ray.ro[0] = -0.5f; ray.ro[1] = 0.0f; ray.ro[2] = 2.0f; ray.clipA = 1.0f;
        ray.rd[0] = 1.0f;  ray.rd[1] = 0.0f; ray.rd[2] = 0.0f; ray.clipB = 2.0f;
        ray.subpanelOffset = 0; ray.subpanelCount = 0;
        ray.overlapOffset = 0;  ray.overlapCount = 0;
    }
    auto budgetSpecBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, budgetSpecs.size() * sizeof(GpuCurtainSpec), budgetSpecs.data());
    auto budgetRayBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, budgetRays.size() * sizeof(GpuRayHeader), budgetRays.data());
    auto budgetOutBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, budgetRays.size() * sizeof(GpuResult), nullptr);

    for (uint32_t testBudget : {23u, 24u, 25u}) {
        auto bcfg = cfg;
        bcfg.pad = testBudget;
        bcfg.rayCount = 16u;
        auto bcfgBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(bcfg), &bcfg);

        wgpuQueueWriteBuffer(gpu.queue, budgetRayBuf, 0, budgetRays.data(), budgetRays.size() * sizeof(GpuRayHeader));
        std::vector<GpuResult> poison(16); for(auto& p:poison) for(float& v:p.rgb_T) v=NAN;
        wgpuQueueWriteBuffer(gpu.queue, budgetOutBuf, 0, poison.data(), 16 * sizeof(GpuResult));

        WGPUBindGroupEntry bgentries[6] = {};
        bgentries[0].binding = 0; bgentries[0].buffer = bcfgBuf; bgentries[0].size = sizeof(bcfg);
        bgentries[1].binding = 1; bgentries[1].buffer = budgetSpecBuf; bgentries[1].size = budgetSpecs.size() * sizeof(GpuCurtainSpec);
        bgentries[2].binding = 2; bgentries[2].buffer = budgetRayBuf; bgentries[2].size = budgetRays.size() * sizeof(GpuRayHeader);
        bgentries[3].binding = 3; bgentries[3].buffer = mBuf; bgentries[3].size = mediaBytes;
        bgentries[4].binding = 4; bgentries[4].buffer = pBuf; bgentries[4].size = paramBytes;
        bgentries[5].binding = 5; bgentries[5].buffer = budgetOutBuf; bgentries[5].size = 16 * sizeof(GpuResult);

        auto bgl = wgpuComputePipelineGetBindGroupLayout(cpOnTheFly, 0);
        WGPUBindGroupDescriptor bgd = {}; bgd.layout = bgl; bgd.entryCount = 6; bgd.entries = bgentries;
        auto bg = wgpuDeviceCreateBindGroup(gpu.device, &bgd);
        wgpuBindGroupLayoutRelease(bgl);

        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpOnTheFly);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bg, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, 1, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        auto hb = readBackBuffer(budgetRayBuf, budgetRays.size() * sizeof(GpuRayHeader));
        const auto* hdrs = reinterpret_cast<const GpuRayHeader*>(hb.data());
        uint32_t fallbacks = 0;
        J flags = J::array();
        for (size_t i = 0; i < 16; ++i) {
            if (hdrs[i].overlapCount & 0x80000000u) fallbacks++;
            flags.push_back(hdrs[i].overlapCount);
        }
        uint32_t expected = (testBudget == 23u) ? 16u : 0u;
        budgetControlResults.push_back({
            {"budget", testBudget},
            {"fallbacks", fallbacks},
            {"expected", expected},
            {"flags", flags},
            {"pass", fallbacks == expected}
        });
        wgpuBindGroupRelease(bg);
        wgpuBufferRelease(bcfgBuf);
    }
    wgpuBufferRelease(budgetSpecBuf);
    wgpuBufferRelease(budgetRayBuf);
    wgpuBufferRelease(budgetOutBuf);
    report["synthetic_budget_controls"] = budgetControlResults;

    // --- Synthetic Capacity Controls on cpOnTheFly ---
    J capacityControlResults = J::array();
    // 1. maxSubPerRay = 4u
    {
        auto ccfg = cfg;
        ccfg.maxSubPerRay = 4u;
        auto ccfgBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(ccfg), &ccfg);
        WGPUBindGroupEntry entries[6] = {};
        entries[0].binding = 0; entries[0].buffer = ccfgBuf; entries[0].size = sizeof(ccfg);
        entries[1].binding = 1; entries[1].buffer = specBuf; entries[1].size = curtainSpecs.size() * sizeof(GpuCurtainSpec);
        entries[2].binding = 2; entries[2].buffer = rBuf; entries[2].size = rayBytes;
        entries[3].binding = 3; entries[3].buffer = mBuf; entries[3].size = mediaBytes;
        entries[4].binding = 4; entries[4].buffer = pBuf; entries[4].size = paramBytes;
        entries[5].binding = 5; entries[5].buffer = outBuf; entries[5].size = outBytes;
        auto bgl = wgpuComputePipelineGetBindGroupLayout(cpOnTheFly, 0);
        WGPUBindGroupDescriptor bgd = {}; bgd.layout = bgl; bgd.entryCount = 6; bgd.entries = entries;
        auto bg = wgpuDeviceCreateBindGroup(gpu.device, &bgd);
        wgpuBindGroupLayoutRelease(bgl);

        std::vector<GpuResult> poison(executionRays); for(auto& p:poison) for(float& v:p.rgb_T) v=NAN;
        wgpuQueueWriteBuffer(gpu.queue, outBuf, 0, poison.data(), outBytes);

        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpOnTheFly);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bg, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderSetPipeline(pass, fallbackCP);
        wgpuComputePassEncoderSetBindGroup(pass, 0, fallbackBG, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        auto hb = readBackBuffer(rBuf, rayBytes);
        const auto* hdrs = reinterpret_cast<const GpuRayHeader*>(hb.data());
        auto res = readResults();
        uint32_t subFallbacks = 0;
        bool fallbackParityPass = true;
        J flags = J::array();
        for (size_t i = 0; i < count; ++i) {
            const bool refused = (hdrs[i].overlapCount & 0x80000000u) != 0;
            if (refused) {
                subFallbacks++;
                if ((hdrs[i].overlapCount & 0x7fffffffu) != 16u) throw std::runtime_error("wrong subpanel capacity fallback reason");
            }
            const auto& expected = refused ? resCurrent[i] : resUnfused[i];
            for (int k = 0; k < 4; ++k) {
                if (!std::isfinite(res[i].rgb_T[k]) || std::bit_cast<uint32_t>(res[i].rgb_T[k]) != std::bit_cast<uint32_t>(expected.rgb_T[k]))
                    fallbackParityPass = false;
            }
            flags.push_back(hdrs[i].overlapCount);
        }
        if (!fallbackParityPass || subFallbacks == 0) throw std::runtime_error("subpanel capacity witness failed or vacuous");
        capacityControlResults.push_back({
            {"control", "maxSubPerRay=4"},
            {"fallbacks", subFallbacks},
            {"flags", flags},
            {"output_rows", resultRows(res)},
            {"fallback_bit_exact_pass", fallbackParityPass}
        });
        wgpuBindGroupRelease(bg);
        wgpuBufferRelease(ccfgBuf);
    }

    // 2. maxOverPerRay = 1u
    {
        auto ccfg = cfg;
        ccfg.maxOverPerRay = 1u;
        auto ccfgBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(ccfg), &ccfg);
        WGPUBindGroupEntry entries[6] = {};
        entries[0].binding = 0; entries[0].buffer = ccfgBuf; entries[0].size = sizeof(ccfg);
        entries[1].binding = 1; entries[1].buffer = specBuf; entries[1].size = curtainSpecs.size() * sizeof(GpuCurtainSpec);
        entries[2].binding = 2; entries[2].buffer = rBuf; entries[2].size = rayBytes;
        entries[3].binding = 3; entries[3].buffer = mBuf; entries[3].size = mediaBytes;
        entries[4].binding = 4; entries[4].buffer = pBuf; entries[4].size = paramBytes;
        entries[5].binding = 5; entries[5].buffer = outBuf; entries[5].size = outBytes;
        auto bgl = wgpuComputePipelineGetBindGroupLayout(cpOnTheFly, 0);
        WGPUBindGroupDescriptor bgd = {}; bgd.layout = bgl; bgd.entryCount = 6; bgd.entries = entries;
        auto bg = wgpuDeviceCreateBindGroup(gpu.device, &bgd);
        wgpuBindGroupLayoutRelease(bgl);

        std::vector<GpuResult> poison(executionRays); for(auto& p:poison) for(float& v:p.rgb_T) v=NAN;
        wgpuQueueWriteBuffer(gpu.queue, outBuf, 0, poison.data(), outBytes);

        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cpOnTheFly);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bg, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderSetPipeline(pass, fallbackCP);
        wgpuComputePassEncoderSetBindGroup(pass, 0, fallbackBG, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63u) / 64u, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuCommandEncoderRelease(enc);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);

        auto hb = readBackBuffer(rBuf, rayBytes);
        const auto* hdrs = reinterpret_cast<const GpuRayHeader*>(hb.data());
        auto res = readResults();
        uint32_t overFallbacks = 0;
        bool fallbackParityPass = true;
        J flags = J::array();
        for (size_t i = 0; i < count; ++i) {
            const bool refused = (hdrs[i].overlapCount & 0x80000000u) != 0;
            if (refused) {
                overFallbacks++;
                if ((hdrs[i].overlapCount & 0x7fffffffu) != 32u) throw std::runtime_error("wrong overlap capacity fallback reason");
            }
            const auto& expected = refused ? resCurrent[i] : resUnfused[i];
            for (int k = 0; k < 4; ++k) {
                if (!std::isfinite(res[i].rgb_T[k]) || std::bit_cast<uint32_t>(res[i].rgb_T[k]) != std::bit_cast<uint32_t>(expected.rgb_T[k]))
                    fallbackParityPass = false;
            }
            flags.push_back(hdrs[i].overlapCount);
        }
        if (!fallbackParityPass || overFallbacks == 0) throw std::runtime_error("overlap capacity witness failed or vacuous");
        capacityControlResults.push_back({
            {"control", "maxOverPerRay=1"},
            {"fallbacks", overFallbacks},
            {"flags", flags},
            {"output_rows", resultRows(res)},
            {"fallback_bit_exact_pass", fallbackParityPass}
        });
        wgpuBindGroupRelease(bg);
        wgpuBufferRelease(ccfgBuf);
    }
    report["synthetic_capacity_controls"] = capacityControlResults;
'''
    anchor_streamed = 'report["streamed_schedule"]='
    pos_streamed = s.find(anchor_streamed)
    if pos_streamed == -1:
        raise ValueError("streamed_schedule anchor not found")
    pos_semi = s.find('};', pos_streamed)
    if pos_semi == -1:
        raise ValueError("end of streamed_schedule not found")
    s = s[:pos_semi + 2] + '\n' + diagnostic_block + s[pos_semi + 2:]

    # 14. Add on-the-fly report metadata and avoid dumping huge JSON to stdout
    meta = '''
    report["on_the_fly_path"] = {
        {"thread_private_subpanels_storage_bytes", 0},
        {"thread_private_overlaps_storage_bytes", 0},
        {"two_stream_remerge_loop_eliminated", true},
        {"chronological_quadrature_evaluated_on_the_fly", true},
        {"subdivision_subpanel_rule_preserved", "n = max(1u, u32(ceil((b - a) / 4.0))) with exact s0/s1 selects"},
        {"overlap_rule_preserved", "advanceOverlap with exact mask and attenuation order"},
        {"late_failure_publication_boundary_preserved", "partial (L, T) discarded on reason != 0; whole ray routed through fallbackCP"},
        {"modes_tested", {"Mode 0 (Current)", "Mode 1 (Guarded Bulk)", "Mode 2 (Buffered Fused 64/16)", "Mode 3 (On-The-Fly Fused 64/16)"}}
    };
'''
    anchor = '    std::cout << report.dump(2) << std::endl;'
    s = q.once(s, anchor, meta + '\n    std::cout << "Report generated successfully." << std::endl;')
    return s


def clocks_4mode(report):
    summaries = []
    for c in report['streamed_schedule']['cases']:
        series = []
        for batch in c['series']:
            samples = batch['samples']
            n = batch['repeats']
            if not isinstance(n, int) or n <= 0:
                raise ValueError('invalid repeats')
            order = ([0, 1, 2, 3, 3, 2, 1, 0, 3, 2, 1, 0, 0, 1, 2, 3]) * 2
            if [s['mode'] for s in samples] != order:
                raise ValueError(f'paired 4-mode order mismatch')
            for s in samples:
                if any(not math.isfinite(s[k]) or s[k] < 0 for k in ['total_ms', 'completion_ms', 'encode_ms']):
                    raise ValueError('invalid clock')
                if not math.isclose(s['total_ms'], s['completion_ms'] + s['encode_ms'], rel_tol=1e-12, abs_tol=1e-10):
                    raise ValueError('clock accounting')
            means = {str(m): statistics.mean(s['total_ms'] / n for s in samples if s['mode'] == m) for m in range(4)}
            paired = []
            for i in range(0, len(samples), 8):
                block = samples[i:i + 8]
                avg = {m: statistics.mean(s['total_ms'] / n for s in block if s['mode'] == m) for m in range(4)}
                paired.append({
                    'bulk_minus_current_ms': avg[1] - avg[0],
                    'fused_buffered_minus_current_ms': avg[2] - avg[0],
                    'on_the_fly_minus_current_ms': avg[3] - avg[0],
                    'on_the_fly_minus_fused_buffered_ms': avg[3] - avg[2]
                })
            series.append({'repeats': n, 'means_ms': means, 'paired': paired})
        summaries.append({'capacity': c['capacity'], 'series': series})
    return summaries


def validate_on_the_fly_report(root, report, rows, baseline, rays, reference):
    """Rigorous admission validation of on-the-fly execution."""
    p = load(root, 'ray_transport_sliver_review')
    count = len(rows)
    reference_rows, indices = reference

    # 1. Packet evidence and comparison to guarded baseline
    if 'fused_packet_census_inputs' not in report:
        raise ValueError("fused_packet_census_inputs absent from report")
    historical = json.loads(gzip.decompress((root / 'docs/audits/GPU_RAY_TRANSPORT_SLIVER_REVIEW_2026-10-22.guarded_confirm_report.gz').read_bytes()))
    trace = report['fused_packet_census_inputs']
    original = historical['schedule_census_inputs']
    packets_equal = load(root, 'ray_transport_root_review').packets_equal
    if len(trace) != len(original) or any(not packets_equal(a, b, p) for a, b in zip(trace, original)):
        raise ValueError("On-the-fly packet endpoints/counts/masks differ from guarded confirm baseline")

    # 2. Output checks against holdouts with 4-mode clock verifier
    p.clocks = clocks_4mode
    portable = p.check(root, report, rows, baseline, rays, reference_rows, indices)
    if not portable['accuracy']['both_resolutions_pass']:
        raise ValueError("Reference holdout accuracy failed on captured baseline")
    if not portable['contemporary_accuracy']['both_resolutions_pass']:
        raise ValueError("Reference holdout accuracy failed on contemporary baseline")

    # 3. Synthetic budget controls
    for c in report['synthetic_budget_controls']:
        expected = 16 if c['budget'] == 23 else 0
        if c['budget'] not in [23, 24, 25] or c['fallbacks'] != expected or c['expected'] != expected or not c['pass']:
            raise ValueError(f"Budget control failure for budget {c['budget']}")
        if 'flags' in c and (len(c['flags']) != 16 or any(x != (0x80000001 if expected else 0) for x in c['flags'])):
            raise ValueError("Budget flags corrupted")

    # 4. Synthetic capacity controls
    caps = report['synthetic_capacity_controls']
    if [x['control'] for x in caps] != ['maxSubPerRay=4', 'maxOverPerRay=1']:
        raise ValueError("Capacity coverage failure")
    for c, reason in zip(caps, [16, 32]):
        if c['fallbacks'] <= 0 or not c['fallback_bit_exact_pass']:
            raise ValueError(f"Capacity failure/vacuity for control {c['control']}")
        if 'flags' in c:
            if len(c['flags']) != count or len(c['output_rows']) != count:
                raise ValueError("Capacity raw coverage mismatch")
            if sum(bool(x & 0x80000000) for x in c['flags']) != c['fallbacks']:
                raise ValueError("Capacity fallback count mismatch")
            for flag, a, b, cur in zip(c['flags'], c['output_rows'], rows, baseline):
                if (flag & 0x80000000) and (flag & 0x7fffffff) != reason:
                    raise ValueError(f"Capacity reason code incorrect: expected {reason}")
                if a[:2] != b[:2] or p.bits(a[2:6]) != p.bits((cur if (flag & 0x80000000) else b)[2:6]):
                    raise ValueError("Capacity output bits mismatch")

    # 5. Diagnostic trace output parity
    if 'trace_output_rows' in report:
        actual = report['trace_output_rows']
        if len(actual) != count or any(a[:2] != b[:2] or p.bits(a[2:6]) != p.bits(b[2:6]) for a, b in zip(actual, rows)):
            raise ValueError("Diagnostic trace output differs from normal rows")

    # 6. Stream timing clock verification across 4 modes
    for case in report['streamed_schedule']['cases']:
        for s in case['series']:
            samples = s['samples']
            mode_counts = {m: sum(x['mode'] == m for x in samples) for m in range(4)}
            if len(samples) != 32 or any(v != 8 for v in mode_counts.values()):
                raise ValueError(f"4-mode sample balance failure: {mode_counts}")
            if not all(x['encode_ms'] >= 0 and x['completion_ms'] >= 0 and
                       math.isclose(x['encode_ms'] + x['completion_ms'], x['total_ms'], rel_tol=1e-12, abs_tol=1e-10)
                       for x in samples):
                raise ValueError("Clock accounting failure in 4-mode benchmark")

    return {
        'queries': count,
        'portable_output_reference_check': portable,
        'fused_packets_equal': True,
        'contract_pass': True
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--case', type=Path, required=True)
    p.add_argument('--label', default='on_the_fly')
    p.add_argument('--build-only', action='store_true')
    args = p.parse_args()

    root = args.root.resolve()
    work = args.work.resolve()
    case_path = args.case.resolve()
    if work == root or root in work.parents or Path(args.label).name != args.label:
        raise SystemExit('unsafe work/label')

    # Load modules
    wide = load(root, 'ray_transport_gpu_wide_stream')

    # Verify case and authored zone
    v = {x.targets[0].id: ast.literal_eval(x.value)
         for x in ast.parse((root / 'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body
         if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}
    zone = root / 'saves/zones/Northern Veil/zone.ecform'
    if sha(zone) != v['ZONE_HASH']:
        raise SystemExit('pinned authored source changed')
    case_meta, rays, (ref_rows, ref_indices) = wide.verify_case(root, case_path, v['ZONE_HASH'])
    mhash = sha(case_path)

    # Generate source
    print('Generating C++ source with 4-mode benchmark, on-the-fly consumption, and packet trace...')
    cpp = source(v['CPP_SOURCE'], root)
    digest = hashlib.sha256(cpp.encode()).hexdigest()

    # Build probe
    work.mkdir(parents=True, exist_ok=False)
    probe_cpp = work / 'probe.cpp'
    probe_o = work / 'probe.o'
    probe_bin = work / 'probe'

    probe_cpp.write_text(cpp)
    (work / 'source.json').write_text(json.dumps({'cpp_sha256': digest, 'zone_sha256': v['ZONE_HASH']}, indent=2) + '\n')

    b = root / 'build'
    db = json.loads((b / 'compile_commands.json').read_text())
    c = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])

    print('Compiling probe.cpp with -std=c++20 -O3...')
    cmd = shlex.split(c['command'])
    cmd[cmd.index('-c') + 1] = str(probe_cpp)
    cmd[cmd.index('-o') + 1] = str(probe_o)
    cmd += ['-std=c++20', '-O3']
    subprocess.run(cmd, cwd=b, check=True)

    print('Linking probe executable...')
    cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    cmd[next(i for i, x in enumerate(cmd) if x.endswith('entry.cpp.o'))] = str(probe_o)
    cmd[cmd.index('-o') + 1] = str(probe_bin)
    subprocess.run(cmd, cwd=b, check=True)

    if args.build_only:
        print('Build-only completed successfully.')
        return

    # Run probe
    output_json = work / (args.label + '.json')
    if output_json.exists():
        raise SystemExit('refusing output overwrite')

    env = dict(os.environ,
               EC_POSE_RAYS=case_meta['rays'],
               EC_POSE_REFERENCE=case_meta['reference'],
               EC_POSE_CLOCK=str(rays['time']),
               EC_MATCH_REPEAT='1',
               EC_GPU_TIMING_BURST='0',
               EC_GPU_BURST_WARMUP='0',
               EC_GPU_BURST_MEASURE='0',
               EC_GPU_MAX_SUBPANELS='64',
               EC_GPU_MAX_OVERLAPS='16',
               EC_GPU_BUDGET='1024',
               EC_GPU_MARGIN='.001')
    env.pop('EC_GPU_GEOMETRY_CASE', None)

    wide.verify_case(root, case_path, v['ZONE_HASH'])
    if sha(case_path) != mhash or sha(zone) != v['ZONE_HASH']:
        raise SystemExit('fixture changed before execution')

    print('Executing 4-mode benchmark on GPU...')
    res = subprocess.run([str(probe_bin), str(output_json)], cwd=root, env=env)
    if res.returncode != 0:
        raise SystemExit(f'Probe execution failed with code {res.returncode}')

    wide.verify_case(root, case_path, v['ZONE_HASH'])
    if sha(case_path) != mhash or sha(zone) != v['ZONE_HASH']:
        raise SystemExit('fixture changed during execution')

    # Validate output
    report = json.loads(output_json.read_text())
    rows_path = Path(str(output_json) + '-rows.json')
    rows = json.loads(rows_path.read_text())
    baseline = json.loads(Path(str(output_json) + '-baseline-rows.json').read_text())

    print('Validating on-the-fly admission and 4-mode contracts...')
    checked = validate_on_the_fly_report(root, report, rows, baseline, rays, (ref_rows, ref_indices))
    Path(str(output_json) + '-independent.json').write_text(json.dumps(checked, indent=2) + '\n')

    # Print 4-mode benchmark timing summary
    print('\n================ 4-MODE CONNECTED BENCHMARK SUMMARY ================')
    case = report['streamed_schedule']['cases'][0]
    print(f"Capacity: {case['capacity']} (tiles: {case['tiles']}, tail: {case['tail_rays']})")
    mode_names = ['Mode 0 (Current Marcher)', 'Mode 1 (Guarded Bulk)', 'Mode 2 (Buffered Fused 64/16)', 'Mode 3 (On-The-Fly Fused 64/16)']

    for s in case['series']:
        rep = s['repeats']
        samples = s['samples']
        print(f"\n--- Repeats = {rep} (8 samples/mode, balanced order) ---")
        means = {}
        for m in range(4):
            m_samples = [x for x in samples if x['mode'] == m]
            mean_total = statistics.mean(x['total_ms'] for x in m_samples) / rep
            mean_encode = statistics.mean(x['encode_ms'] for x in m_samples) / rep
            mean_complete = statistics.mean(x['completion_ms'] for x in m_samples) / rep
            means[m] = (mean_total, mean_encode, mean_complete)
            print(f"  {mode_names[m]:32s}: {mean_total:8.4f} ms/query (encode: {mean_encode:6.4f} ms, completion: {mean_complete:6.4f} ms)")

        speedup_vs_mode0 = means[0][0] / means[3][0]
        speedup_vs_mode2 = means[2][0] / means[3][0]
        print(f"  >> Mode 3 vs Mode 0 (Current):  {speedup_vs_mode0:6.3f}x speedup ({'FASTER' if speedup_vs_mode0 > 1.0 else 'SLOWER'})")
        print(f"  >> Mode 3 vs Mode 2 (Buffered): {speedup_vs_mode2:6.3f}x speedup ({'FASTER' if speedup_vs_mode2 > 1.0 else 'SLOWER'})")

    print('\nAll validations passed! Deliverable ready.')


if __name__ == '__main__':
    main()
