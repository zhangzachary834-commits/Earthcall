"""Ray Transport GPU 6ej Clean Complete-Cost Screen.

Evaluates 5 resident execution lanes on Apple M5:
- Lane 0: Current Baseline (currentCP)
- Lane 1: Unchanged Bulk Bisection (cpProducer + cpUnfused + fallbackCP)
- Lane 2: Unchanged Routed Bisection (cpReset + cpClassify + cpArgs + cpS1Unfused + cpS2 + cpFallback + cpUnfused + fallbackCP)
- Lane 3: Unchanged Terminal S1 (cpReset + cpClassify + cpArgs + cpS1Terminal + cpS2 + cpFallback + cpUnfused + fallbackCP)
- Lane 4: §6ej Candidate Terminal S1 + Terminal S2 (cpReset + cpClassify + cpArgs + cpS1Terminal + cpS2Terminal + cpFallback + cpUnfused + fallbackCP)

Runs ordered raw repeats 1, 2, 4 across 2 independent series (blocks) of 4 alternating sub-blocks
(ABBA order: {0,1,2,3,4,4,3,2,1,0} and {4,3,2,1,0,0,1,2,3,4}).
Captures host encoding, submit-to-completion wait, and total time, reporting means and medians.
Includes candidate parity verification on Mode 4 outside timed batches.

Antigravity / Gemini 3.8 Flash (Sparkly Guy 🐉✨) / 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-27.
"""

from __future__ import annotations

import argparse
import ast
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path


def sha(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def load(root: Path, name: str):
    path = root / f"scratch/probes/{name}.py"
    ns: dict = {"__file__": str(path)}
    exec(compile(path.read_text(), str(path), "exec"), ns)
    return type(name, (), ns)


def make_clean_s2_terminal_shader(root: Path) -> str:
    t6dt = load(root, "ray_transport_gpu_6dt_true_terminal_s1")
    s = t6dt.make_terminal_s1_shaders(root)
    s1_term = s["kS1ProducerWgsl"]

    s2_term_header = s1_term.replace(
        "@group(0) @binding(7) var<storage, read> s1TileList: array<u32>;",
        "@group(0) @binding(7) var<storage, read> s2TileList: array<u32>;"
    )

    idx_entry = s2_term_header.find("@compute @workgroup_size(64, 1, 1)")
    assert idx_entry != -1

    s2_compute_body = """@compute @workgroup_size(64, 1, 1)
fn s2TerminalProducerMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    if (tileSlot >= min(routeCounts.y, 2304u)) { return; }
    let tileId = s2TileList[tileSlot];
    if (tileId >= 2304u) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) { return; }
    let id = tileRayIndices[rayStart + rayInTile];
    if (id >= cfg.rayCount) { return; }

    let input = rays[id]; let ro = input.ro_clipA.xyz; let d = input.rd_clipB.xyz;
    let subBase = id * cfg.maxSubPerRay; let overBase = id * cfg.maxOverPerRay;
    var reason = 0u; var work = 0u; var count = 0u;
    var pieces: array<vec2f, 48>;
    var knots: array<f32, 96>;
    var memStart = vec2u(0u, 0u);
    var memCount = vec2u(0u, 0u);
    if (cfg.budget == 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | 1u); return; }

    for (var member = 0u; member < 2u; member++) {
        memStart[member] = count;
        let m = curtains[member]; let o = ro - m.origin.xyz;
        var a = input.ro_clipA.w; var b = input.rd_clipB.w; var hit = true;
        for (var axis = 0u; axis < 3u; axis++) {
            var half = m.half[axis];
            if (axis == 0u) { half = min(half, m.limits.x); } if (axis == 1u) { half = min(half, m.limits.y); }
            if (d[axis] == 0.0) { if (abs(o[axis]) > half + m.curv.y) { hit = false; break; } }
            else {
                let q0 = (-half - m.curv.y - o[axis]) / d[axis]; let q1 = (half + m.curv.y - o[axis]) / d[axis];
                a = max(a, min(q0, q1)); b = min(b, max(q0, q1));
            }
        }
        if (!hit || !(b > a)) { continue; }
        var phi_knots: array<f32, 9>;
        for (var k = 0u; k < 9u; k++) {
            var s = a + (b - a) * f32(k) / 8.0;
            if (k == 0u) { s = a; } if (k == 8u) { s = b; }
            phi_knots[k] = evalPhi(m, o, d, s);
        }
        var allPositiveH = true;
        let mask0 = exactLevelActiveMask(m, d, a, b, -m.limits.w, &phi_knots, &allPositiveH);
        let mask1 = exactLevelActiveMask(m, d, a, b, 0.0, &phi_knots, &allPositiveH);
        let mask2 = exactLevelActiveMask(m, d, a, b, m.limits.w, &phi_knots, &allPositiveH);
        if (allPositiveH && (mask0 | mask1 | mask2) == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) {
            if (work + 24u > cfg.budget) { reason |= 1u; break; }
            work += 24u;
            memCount[member] = 0u;
            continue;
        }
        var nb = 1u; knots[0] = a;
        var bands: array<vec2f, 32>; var nr = 0u; var zeroBands = 0u;
        roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask0);
        if (reason != 0u) { break; }
        roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, mask1);
        if (reason != 0u) { break; }
        roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask2);
        if (reason != 0u) { break; }
        if (d.x != 0.0) { let x = -o.x / d.x; if (x > a && x < b) { knots[nb] = x; nb++; } }
        if (d.y != 0.0) {
            let y = -o.y / d.y; if (y > a && y < b) { knots[nb] = y; nb++; }
            let k = (m.limits.z - o.y) / d.y; if (k > a && k < b) { knots[nb] = k; nb++; }
        }
        knots[nb] = b; nb++;
        for (var i = 1u; i < nb; i++) {
            let v = knots[i]; var j = i; loop { if (j == 0u || knots[j - 1u] <= v) { break; } knots[j] = knots[j - 1u]; j--; }
            knots[j] = v;
        }
        let memberStart = count; var previousShortBand = false; var previousPositive = false;
        for (var i = 1u; i < nb; i++) {
            let l = knots[i - 1u]; let r = knots[i]; if (!(r > l)) { continue; }
            let mid = l + 0.5 * (r - l); var uncertain = false; var protectedBoundary = false;
            if (d.x != 0.0) { protectedBoundary = l == -o.x / d.x; }
            if (d.y != 0.0) { protectedBoundary = protectedBoundary || l == -o.y / d.y || l == (m.limits.z - o.y) / d.y; }
            for (var k = 0u; k < nr; k++) {
                uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
                if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) { protectedBoundary = true; }
            }
            if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
                if (count > memberStart && mayAbsorb(pieces[count - 1u], vec2f(l, r),
                     previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                    pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;
                } else {
                    if (count >= 48u) { reason |= 8u; break; }
                    pieces[count] = vec2f(l, r);
                    count++; previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
                }
            }
        }
        memCount[member] = count - memStart[member];
        if (reason != 0u) { break; }
    }
    if (reason != 0u) {
        rays[id].counts = vec4u(subBase, 0u, overBase, FALLBACK | reason);
        return;
    }

    // Pre-check total subpanel count across finalized pieces[0..count-1]
    var singles = 0u;
    for (var p = 0u; p < count; p++) {
        let pa = pieces[p].x;
        let pb = pieces[p].y;
        let n = max(1u, u32(ceil((pb - pa) / 4.0)));
        if (singles + n > cfg.maxSubPerRay) {
            reason |= 16u;
            break;
        }
        singles += n;
    }
    if (reason != 0u) {
        rays[id].counts = vec4u(subBase, 0u, overBase, FALLBACK | reason);
        return;
    }

    // Subdivide pieces directly in scalar registers and integrate into shared (L, T):
    let rd = d;
    let gx0 = 0.5 * (1.0 - 0.5773502691896257);
    let gx1 = 0.5 * (1.0 + 0.5773502691896257);
    var T = 1.0;
    var L = vec3<f32>(0.0);

    for (var member = 0u; member < 2u; member++) {
        let m = media[member];
        let pOffset = u32(m.freq_offset.y);
        let freq = m.freq_offset.x;
        let alpha = m.color_alpha.w;
        let color = m.color_alpha.xyz;
        let start = memStart[member];
        let end = start + memCount[member];

        for (var p = start; p < end; p++) {
            let pa = pieces[p].x;
            let pb = pieces[p].y;
            let n = max(1u, u32(ceil((pb - pa) / 4.0)));
            for (var k = 0u; k < n; k++) {
                let s0 = select(pa + f32(k) * (pb - pa) / f32(n), pa, k == 0u);
                let s1 = select(pa + f32(k + 1u) * (pb - pa) / f32(n), pb, k + 1u == n);
                let sub_h = s1 - s0;

                let s_m = s0 + gx0 * sub_h;
                let s_p = s0 + gx1 * sub_h;
                let w_m = ro + s_m * rd - m.origin.xyz;
                let w_p = ro + s_p * rd - m.origin.xyz;

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

                let sig_m = select(0.0, max(alpha * D_m, 1e-6), D_m > 0.0);
                let em_m  = select(vec3<f32>(0.0), color * (alpha * D_m), D_m > 0.0);
                let sig_p = select(0.0, max(alpha * D_p, 1e-6), D_p > 0.0);
                let em_p  = select(vec3<f32>(0.0), color * (alpha * D_p), D_p > 0.0);

                let sig_avg = 0.5 * (sig_m + sig_p);
                let em_avg  = 0.5 * (em_m + em_p);
                let tau = sub_h * sig_avg;
                let atten = exp(-tau);

                var gain: f32;
                gain = sub_h * phiSafe(tau);

                L = L + T * em_avg * gain;
                T = T * atten;
            }
        }
    }

    results[id] = vec4<f32>(L, T);
    rays[id].counts = vec4u(subBase, singles, overBase, 0x40000000u);
}
"""
    return s2_term_header[:idx_entry] + s2_compute_body


def clean_cpp(root: Path, variant: str = "bound-stack-15") -> str:
    base = load(root, "ray_transport_gpu_6du_true_terminal_s1")
    dx = load(root, "ray_transport_gpu_6dx_stack_bound")

    constants = {
        x.targets[0].id: ast.literal_eval(x.value)
        for x in ast.parse((root / "scratch/probes/ray_transport_metal_diagnostic.py").read_text()).body
        if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)
    }
    cpp = base.make_terminal_s1_cpp_source(constants["CPP_SOURCE"], root)
    cpp = dx.apply_variant(cpp, variant)

    # 1. Shaders: inject clean kS2TerminalProducerWgsl
    s2_term = make_clean_s2_terminal_shader(root)
    decl = f'\nstatic const char* kS2TerminalProducerWgsl = R"WGSL({s2_term})WGSL";\n'

    hook_main = "int main(int argc, char** argv) {"
    if cpp.count(hook_main) != 1:
        raise ValueError("main hook mismatch")
    cpp = cpp.replace(hook_main, decl + "\n" + hook_main, 1)

    # 2. Pipelines: add cpS2Terminal
    pipe_hook = 'auto cpFallback = buildCompute(kFallbackProducerWgsl, "fallbackProducerMain");'
    if cpp.count(pipe_hook) != 1:
        raise ValueError("pipeline hook mismatch")
    pipe_decl = """auto cpFallback = buildCompute(kFallbackProducerWgsl, "fallbackProducerMain");
    auto cpS2Terminal = buildCompute(kS2TerminalProducerWgsl, "s2TerminalProducerMain");"""
    cpp = cpp.replace(pipe_hook, pipe_decl, 1)

    # 3. Bind Groups: add bgS2Terminal and bgS2TerminalForced
    bg_hook = "auto bgFallbackForced = makeGroup(cpFallback, {"
    idx = cpp.find(bg_hook)
    if idx == -1:
        raise ValueError("bgFallbackForced hook mismatch")
    end_bg = cpp.find("});", idx) + len("});")
    bg_decl = """
    auto bgS2Terminal = makeGroup(cpS2Terminal, {
        makeEntry(0, cfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(4, outBuf, 0, outBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s2TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt)),
        makeEntry(9, mBuf, 0, mediaBytes),
        makeEntry(10, pBuf, 0, paramBytes)
    });
    auto bgS2TerminalForced = makeGroup(cpS2Terminal, {
        makeEntry(0, forcedCfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(4, outBuf, 0, outBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s2TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt)),
        makeEntry(9, mBuf, 0, mediaBytes),
        makeEntry(10, pBuf, 0, paramBytes)
    });
"""
    cpp = cpp[:end_bg] + bg_decl + cpp[end_bg:]

    # 4. In execute(mode, ...): add Mode 4 dispatch
    hook_exec_mode3 = """                    // Pass 6: Canonical Unfused A* Consumer (skips completed S1 rays and refused rays)
                    dispatch(cpUnfused, bgUnfused, count);
                    // Pass 7: Production-Generated Current Marcher Fallback (only for refused rays)
                    dispatch(fallbackCP, fallbackBG, count);
                }"""
    if cpp.count(hook_exec_mode3) != 1:
        raise ValueError("hook_exec_mode3 mismatch")

    exec_mode4 = """
                else if(mode==4){
                    // Mode 4: §6ej Candidate Terminal S1 + Terminal S2 (Unchanged Bisection Only)
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
                        wgpuComputePassEncoderDispatchWorkgroups(p, 36, 1, 1);
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
                    // Pass 4: S2 Terminal Producer (Candidate!)
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
                    // Pass 6: Canonical Unfused Consumer (skips completed S1 & S2 rays)
                    dispatch(cpUnfused, bgUnfused, count);
                    // Pass 7: Current Marcher Fallback (runs only on refused rays)
                    dispatch(fallbackCP, fallbackBG, count);
                }"""
    cpp = cpp.replace(hook_exec_mode3, hook_exec_mode3 + exec_mode4, 1)

    # Flush WebGPU error stderr
    cpp = cpp.replace(
        'std::fprintf(stderr, "[WebGPU Error %d]: %.*s\\n", (int)type, (int)msg.length, msg.data);',
        'std::fprintf(stderr, "[WebGPU Error %d]: %.*s\\n", (int)type, (int)msg.length, msg.data); std::fflush(stderr);'
    )

    # 5. Candidate verification check before timing loop
    corr_hook = "correctness(3,false);"
    idx_c3 = cpp.find(corr_hook)
    if idx_c3 == -1:
        raise ValueError("correctness(3,false) hook mismatch")
    end_c3 = idx_c3 + len(corr_hook)

    cand_check = """
        // Mode 4 Candidate Verification (outside timed batches)
        execute(4, 1, false);
        auto candResults = readResults();
        auto candHdrsBytes = readBackBuffer(rBuf, rayBytes);
        const auto* candHdrs = reinterpret_cast<const GpuRayHeader*>(candHdrsBytes.data());
        unsigned candTerminalCount = 0;
        unsigned candFallbackCount = 0;
        double candMaxDeltaRGB = 0.0;
        double candMaxDeltaT = 0.0;
        unsigned candOutputDiffs = 0;
        for (size_t i = 0; i < count; ++i) {
            if ((candHdrs[i].overlapCount & 0x40000000u) != 0) candTerminalCount++;
            if ((candHdrs[i].overlapCount & 0x80000000u) != 0) candFallbackCount++;
            bool diff = false;
            for (int k = 0; k < 3; ++k) {
                double delta = std::abs(double(candResults[i].rgb_T[k]) - double(resCurrent[i].rgb_T[k]));
                candMaxDeltaRGB = std::max(candMaxDeltaRGB, delta);
                if (std::bit_cast<uint32_t>(candResults[i].rgb_T[k]) != std::bit_cast<uint32_t>(resCurrent[i].rgb_T[k])) diff = true;
            }
            double deltaT = std::abs(double(candResults[i].rgb_T[3]) - double(resCurrent[i].rgb_T[3]));
            candMaxDeltaT = std::max(candMaxDeltaT, deltaT);
            if (std::bit_cast<uint32_t>(candResults[i].rgb_T[3]) != std::bit_cast<uint32_t>(resCurrent[i].rgb_T[3])) diff = true;
            if (diff) candOutputDiffs++;
        }
        report["candidate_verification"] = {
            {"mode", 4},
            {"terminal_rays", candTerminalCount},
            {"fallback_rays", candFallbackCount},
            {"output_differences_vs_current", candOutputDiffs},
            {"current_max_rgb_delta", candMaxDeltaRGB},
            {"current_max_t_delta", candMaxDeltaT}
        };
        std::cerr << "CANDIDATE VERIFICATION: terminal=" << candTerminalCount << " fallback=" << candFallbackCount << " outputDiffs=" << candOutputDiffs << std::endl;
"""
    cpp = cpp[:end_c3] + cand_check + cpp[end_c3:]

    # 6. Timing loop replacement
    old_timing_block = """        // 4 Warmup cycles
        for(int k=0;k<4;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);execute(3,1,false);}

        J series = J::array();
        J cleanTiming = J::object();
        J orderedTiming = J::object();
        for (int sIdx = 1; sIdx <= 2; ++sIdx) {
            std::string sKey = "series_" + std::to_string(sIdx);
            J seriesData = J::object();
            for (int rep : {1, 2, 4}) {
                std::string repKey = std::to_string(rep);
                J repData = J::object();
                J series1Samples = J::array();
                std::vector<std::vector<double>> modeSamples(4);
                for (int block = 0; block < 4; ++block) {
                    const std::vector<int> order = block % 2 ? std::vector<int>{3, 2, 1, 0, 0, 1, 2, 3} : std::vector<int>{0, 1, 2, 3, 3, 2, 1, 0};
                    for (int mode : order) {
                        auto t = execute(mode, rep, false);
                        modeSamples[mode].push_back(t[1]); // completion_ms
                        series1Samples.push_back({{"mode", mode}, {"encode_ms", t[0]}, {"completion_ms", t[1]}, {"total_ms", t[2]}});
                    }
                }
                orderedTiming[sKey][repKey] = series1Samples;
                if (sIdx == 1) {
                    series.push_back({{"repeats", rep}, {"samples", series1Samples}});
                }
                for (int m = 0; m < 4; ++m) {
                    auto& sVec = modeSamples[m];
                    std::sort(sVec.begin(), sVec.end());
                    double sum = std::accumulate(sVec.begin(), sVec.end(), 0.0);
                    double mean = sum / sVec.size();
                    double med = 0.5 * (sVec[sVec.size() / 2 - 1] + sVec[sVec.size() / 2]);
                    repData[std::to_string(m)] = {
                        {"samples", sVec},
                        {"min_ms", sVec.front()},
                        {"median_ms", med},
                        {"mean_ms", mean},
                        {"max_ms", sVec.back()}
                    };
                }
                seriesData[repKey] = repData;
            }
            cleanTiming[sKey] = seriesData;
        }

        // Pool both blocks: 64 total observations, 16 per mode per repeat.
        J pooled = J::object();
        for (int rep : {1, 2, 4}) {
            std::string repKey = std::to_string(rep);
            J repPooled = J::object();
            for (int m = 0; m < 4; ++m) {
                std::string mKey = std::to_string(m);
                auto s1 = cleanTiming["series_1"][repKey][mKey]["samples"].get<std::vector<double>>();
                auto s2 = cleanTiming["series_2"][repKey][mKey]["samples"].get<std::vector<double>>();
                std::vector<double> allSamples = s1;
                allSamples.insert(allSamples.end(), s2.begin(), s2.end());
                std::sort(allSamples.begin(), allSamples.end());
                double sum = std::accumulate(allSamples.begin(), allSamples.end(), 0.0);
                double mean = sum / allSamples.size();
                double med = 0.5 * (allSamples[allSamples.size() / 2 - 1] + allSamples[allSamples.size() / 2]);
                repPooled[mKey] = {
                    {"samples", allSamples},
                    {"min_ms", allSamples.front()},
                    {"median_ms", med},
                    {"mean_ms", mean},
                    {"max_ms", allSamples.back()}
                };
            }
            pooled[repKey] = repPooled;
        }
        cleanTiming["pooled"] = pooled;
        report["clean_timing_series"] = cleanTiming;

        report["clean_timing_ordered_samples"] = orderedTiming;
        report["clean_timing_scope"] = {
            {"independent_native_invocations", 1},
            {"blocks_per_invocation", 2},
            {"observations_per_mode_per_block_per_repeat", 8},
            {"clock", "host submit-to-completion; includes GPU work and host completion wait"},
            {"encode_and_total_samples_retained", true},
            {"gpu_frequency_observed", false}
        };"""

    new_timing_block = """        // 4 Warmup cycles across all 5 active modes: {0, 1, 2, 3, 4}
        for(int k=0;k<4;++k){for(int m:{0,1,2,3,4})execute(m,1,false);}

        J series = J::array();
        J cleanTiming = J::object();
        J orderedTiming = J::object();
        for (int sIdx = 1; sIdx <= 2; ++sIdx) {
            std::string sKey = "series_" + std::to_string(sIdx);
            J seriesData = J::object();
            for (int rep : {1, 2, 4}) {
                std::string repKey = std::to_string(rep);
                J repData = J::object();
                J seriesSamples = J::array();
                std::map<int, std::vector<double>> modeEncode;
                std::map<int, std::vector<double>> modeCompletion;
                std::map<int, std::vector<double>> modeTotal;
                for (int block = 0; block < 4; ++block) {
                    const std::vector<int> order = block % 2 ? std::vector<int>{4, 3, 2, 1, 0, 0, 1, 2, 3, 4} : std::vector<int>{0, 1, 2, 3, 4, 4, 3, 2, 1, 0};
                    for (int mode : order) {
                        auto t = execute(mode, rep, false);
                        modeEncode[mode].push_back(t[0]);
                        modeCompletion[mode].push_back(t[1]);
                        modeTotal[mode].push_back(t[2]);
                        seriesSamples.push_back({{"mode", mode}, {"encode_ms", t[0]}, {"completion_ms", t[1]}, {"total_ms", t[2]}});
                    }
                }
                orderedTiming[sKey][repKey] = seriesSamples;
                if (sIdx == 1) {
                    series.push_back({{"repeats", rep}, {"samples", seriesSamples}});
                }
                auto computeStats = [](std::vector<double> v) {
                    std::sort(v.begin(), v.end());
                    double sum = std::accumulate(v.begin(), v.end(), 0.0);
                    double mean = sum / v.size();
                    double med = 0.5 * (v[v.size() / 2 - 1] + v[v.size() / 2]);
                    return J{{"samples", v}, {"min_ms", v.front()}, {"median_ms", med}, {"mean_ms", mean}, {"max_ms", v.back()}};
                };
                for (int m : {0, 1, 2, 3, 4}) {
                    std::string mKey = std::to_string(m);
                    auto encStats = computeStats(modeEncode[m]);
                    auto compStats = computeStats(modeCompletion[m]);
                    auto totStats = computeStats(modeTotal[m]);
                    repData[mKey] = {
                        {"encode_ms", encStats},
                        {"completion_ms", compStats},
                        {"total_ms", totStats},
                        {"samples", compStats["samples"]},
                        {"min_ms", compStats["min_ms"]},
                        {"median_ms", compStats["median_ms"]},
                        {"mean_ms", compStats["mean_ms"]},
                        {"max_ms", compStats["max_ms"]}
                    };
                }
                seriesData[repKey] = repData;
            }
            cleanTiming[sKey] = seriesData;
        }

        // Pool both blocks: 16 observations per mode per repeat
        J pooled = J::object();
        for (int rep : {1, 2, 4}) {
            std::string repKey = std::to_string(rep);
            J repPooled = J::object();
            auto computePooledStats = [&](int m, const std::string& field) {
                std::string mKey = std::to_string(m);
                auto s1 = cleanTiming["series_1"][repKey][mKey][field]["samples"].get<std::vector<double>>();
                auto s2 = cleanTiming["series_2"][repKey][mKey][field]["samples"].get<std::vector<double>>();
                std::vector<double> allSamples = s1;
                allSamples.insert(allSamples.end(), s2.begin(), s2.end());
                std::sort(allSamples.begin(), allSamples.end());
                double sum = std::accumulate(allSamples.begin(), allSamples.end(), 0.0);
                double mean = sum / allSamples.size();
                double med = 0.5 * (allSamples[allSamples.size() / 2 - 1] + allSamples[allSamples.size() / 2]);
                return J{{"samples", allSamples}, {"min_ms", allSamples.front()}, {"median_ms", med}, {"mean_ms", mean}, {"max_ms", allSamples.back()}};
            };
            for (int m : {0, 1, 2, 3, 4}) {
                std::string mKey = std::to_string(m);
                auto encStats = computePooledStats(m, "encode_ms");
                auto compStats = computePooledStats(m, "completion_ms");
                auto totStats = computePooledStats(m, "total_ms");
                repPooled[mKey] = {
                    {"encode_ms", encStats},
                    {"completion_ms", compStats},
                    {"total_ms", totStats},
                    {"samples", compStats["samples"]},
                    {"min_ms", compStats["min_ms"]},
                    {"median_ms", compStats["median_ms"]},
                    {"mean_ms", compStats["mean_ms"]},
                    {"max_ms", compStats["max_ms"]}
                };
            }
            pooled[repKey] = repPooled;
        }
        cleanTiming["pooled"] = pooled;
        report["clean_timing_series"] = cleanTiming;

        report["clean_timing_ordered_samples"] = orderedTiming;
        report["clean_timing_scope"] = {
            {"independent_native_invocations", 1},
            {"blocks_per_invocation", 2},
            {"observations_per_mode_per_block_per_repeat", 8},
            {"clock", "host submit-to-completion; includes GPU work and host completion wait"},
            {"encode_and_total_samples_retained", true},
            {"gpu_frequency_observed", false}
        };"""

    idx_tb = cpp.find(old_timing_block)
    if idx_tb == -1:
        raise ValueError("old_timing_block hook mismatch")
    cpp = cpp[:idx_tb] + new_timing_block + cpp[idx_tb + len(old_timing_block):]
    return cpp


def build_probe(root: Path, build_root: Path, work: Path, variant: str = "bound-stack-15") -> Path:
    worktree_root = build_root.parent if build_root.name == "build" else build_root
    if work.exists() or work == root or root in work.parents:
        raise ValueError("fresh external work required")
    work.mkdir(parents=True, exist_ok=False)

    cpp = clean_cpp(root, variant)
    source = work / "probe.cpp"
    source.write_text(cpp)

    build_dir = work / "build"
    subprocess.run([
        sys.executable, "-B", str(root / "scratch/probes/ray_transport_probe_build.py"),
        "--root", str(root),
        "--build-root", str(worktree_root),
        "--source", str(source),
        "--work", str(build_dir)
    ], check=True)
    return build_dir / "probe"


def run_probe(root: Path, binary: Path, work: Path) -> dict:
    wide = load(root, "ray_transport_gpu_wide_stream")
    zone = root / "saves/zones/Northern Veil/zone.ecform"
    before = sha(zone.read_bytes())
    manifest = Path("/private/tmp/earthcall-wide-case/manifest.json")
    meta, rays, _ = wide.verify_case(root, manifest, before)

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
    )
    env.pop("EC_GPU_GEOMETRY_CASE", None)
    env.pop("METAL_CAPTURE_ENABLED", None)

    out_json = work / "clean_timing_result.json"
    log_path = work / "native.log"
    with log_path.open("xb") as f:
        res = subprocess.run([str(binary), str(out_json)], cwd=root, env=env, stdout=f, stderr=subprocess.STDOUT)

    if res.returncode != 0:
        log_content = log_path.read_text(errors="replace")
        raise RuntimeError(f"probe execution failed with code {res.returncode}:\n{log_content}")

    return json.loads(out_json.read_text())


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--build-root", type=Path, required=True)
    p.add_argument("--work", type=Path, required=True)
    p.add_argument("--variant", default="bound-stack-15")
    p.add_argument("--run", action="store_true")
    args = p.parse_args()

    root = args.root.resolve()
    build_root = args.build_root.resolve()
    work = args.work.resolve()

    binary = build_probe(root, build_root, work, args.variant)
    print(f"Probe binary built successfully at {binary}")

    if args.run:
        print("Executing clean complete-cost screen on native Apple M5...")
        report = run_probe(root, binary, work)
        print("Native execution complete! Verification summary:")
        print(json.dumps(report.get("candidate_verification", {}), indent=2))
        print("Clean timing pooled summary:")
        print(json.dumps(report.get("clean_timing_series", {}).get("pooled", {}), indent=2))


if __name__ == "__main__":
    main()
