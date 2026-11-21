"""§6ej Terminal S2 with Unchanged Bisection Only.

Fulfills Sixth-One Sun's §6ej commissioning:
1. Candidate Mode 4: Terminal S2 with UNCHANGED BISECTION ONLY.
   - Evaluates both member 0 and member 1 in-producer.
   - Strictly NO outward-radius / SBI / cache / streamer combination.
   - Preserves existing two-member order (member 0 before member 1) and shared capacities:
     pieces[48], maxSubPerRay=64.
   - Resolves both members and all capacity/refusal checks BEFORE publishing final (L, T).
   - Late member 1 refusal discards partial work and falls back to whole-ray Current marcher fallback.
   - Quadrature uses exact canonical member order, original arithmetic, and authored media into shared (L, T).
2. Diagnostic Audit & Capture:
   - Captures actual terminal endpoints (s0, s1) AND mediumIndex/member for both member 0 and member 1.
   - Poisoning check: verifies global sBuf remains 0xcdcdcdcd for all completed S1 and S2 rays.
3. Strict Parity Gate:
   - 30 matrix qualification cases (6 poses x 4 reps, 4 budgets, 2 capacities).
   - Bitwise output equality (wrong == 0) and packet equality (packets == 0) against unchanged bisection controls.
4. Negative Mutations:
   - 'no-reset', 'double-publish', 'missing-s1', 'missing-s2', 'missing-fallback',
     'late-member1-refusal', 'terminal-endpoint-corrupt'
   - All exiting 250 at intended assertions.

Antigravity / Gemini 3.8 Flash (Sparkly Guy 🐉✨) / 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-27.
"""

from __future__ import annotations

import argparse
import ast
import copy
import hashlib
import importlib.util
import json
import math
import os
import re
import shlex
import statistics
import subprocess
import sys
from pathlib import Path


def sha(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def load(root: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, root / "scratch/probes" / f"{name}.py")
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def make_terminal_s2_shaders(root: Path, mutation: str = "fixed") -> dict:
    t6dt = load(root, "ray_transport_gpu_6dt_true_terminal_s1")
    shaders_6dt = t6dt.make_terminal_s1_shaders(root)

    fr = load(root, "ray_transport_gpu_faithful_residual")
    cpp = fr.generate_cpp_source(root, mutation="fixed")
    s1_old = re.search(r'static const char\* kS1ProducerWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S).group(1)

    def clean(s):
        s = s.replace('@group(0) @binding(9) var<storage, read_write> writes: array<atomic<u32>>;\n', '')
        s = s.replace('atomicAdd(&writes[id], 1u);\n', '')
        s = s.replace('atomicAdd(&writes[id], 1u);', '')
        return s

    # 1. kS1UnfusedProducerWgsl
    s1_unfused = clean(s1_old)
    idx_emission = s1_unfused.find('let ne = count * 2u;')
    assert idx_emission != -1
    idx_end = s1_unfused.find('rays[id].counts = vec4u(subBase, singles, overBase, overs);') + len('rays[id].counts = vec4u(subBase, singles, overBase, overs);')
    assert idx_end != -1

    direct_s1_emission = r"""var singles = 0u;
 for (var p = 0u; p < count; p++) {
  let a = pieces[p].x; let b = pieces[p].y;
  let n = max(1u, u32(ceil((b - a) / 4.0)));
  if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
  for (var k = 0u; k < n; k++) {
   let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
   let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
   subpanels[subBase + singles] = Subpanel(vec2f(l, r), vec2u(0u, 0u)); singles++;
  }
 }
 if (reason != 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | reason); }
 else { rays[id].counts = vec4u(subBase, singles, overBase, 0u);"""

    kS1UnfusedProducerWgsl = s1_unfused[:idx_emission] + direct_s1_emission + s1_unfused[idx_end:]

    # 2. kS2TerminalProducerWgsl (Terminal S2 with UNCHANGED BISECTION)
    # Uses kS1ProducerWgsl as the exact template for headers, cnoise3, evalEnvelope, phiSafe, roots
    s1_term = shaders_6dt["kS1ProducerWgsl"]

    # In s2 terminal:
    # Tile list is s2TileList at binding 7
    # Route counts index is y (routeCounts.y)
    # Loop over 2 members, shared pieces[48]
    s2_term_header = s1_term.replace(
        "@group(0) @binding(7) var<storage, read> s1TileList: array<u32>;",
        "@group(0) @binding(7) var<storage, read> s2TileList: array<u32>;"
    )

    # Replace entrypoint with S2 entrypoint
    idx_entry = s2_term_header.find("@compute @workgroup_size(64, 1, 1)")
    assert idx_entry != -1

    late_refusal_hook = ""
    if mutation == "late-member1-refusal":
        late_refusal_hook = "\n        if (member == 1u) { reason |= 1u; break; }\n"

    captured_s0_s2 = "bitcast<u32>(s0) ^ 1u" if mutation == "terminal-endpoint-corrupt" else "bitcast<u32>(s0)"
    publish_s2 = "if (id > 1000000u) { results[id] = vec4<f32>(L, T); }" if mutation == "missing-s2" else "results[id] = vec4<f32>(L, T);\n    atomicAdd(&auditWrites[id * 2u + 1u], 1u);"

    s2_compute_body = f"""@compute @workgroup_size(64, 1, 1)
fn s2TerminalProducerMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {{
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    if (tileSlot >= min(routeCounts.y, 2304u)) {{ return; }}
    let tileId = s2TileList[tileSlot];
    if (tileId >= 2304u) {{ return; }}
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) {{ return; }}
    let id = tileRayIndices[rayStart + rayInTile];
    if (id >= cfg.rayCount) {{ return; }}

    atomicAdd(&auditWrites[id * 2u], 1u);

    let input = rays[id]; let ro = input.ro_clipA.xyz; let d = input.rd_clipB.xyz;
    let subBase = id * cfg.maxSubPerRay; let overBase = id * cfg.maxOverPerRay;
    var reason = 0u; var work = 0u; var count = 0u;
    var pieces: array<vec2f, 48>;
    var knots: array<f32, 96>;
    var memStart = vec2u(0u, 0u);
    var memCount = vec2u(0u, 0u);
    if (cfg.budget == 0u) {{ rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | 1u); return; }}

    for (var member = 0u; member < 2u; member++) {{
        memStart[member] = count;
        {late_refusal_hook}
        let m = curtains[member]; let o = ro - m.origin.xyz;
        var a = input.ro_clipA.w; var b = input.rd_clipB.w; var hit = true;
        for (var axis = 0u; axis < 3u; axis++) {{
            var half = m.half[axis];
            if (axis == 0u) {{ half = min(half, m.limits.x); }} if (axis == 1u) {{ half = min(half, m.limits.y); }}
            if (d[axis] == 0.0) {{ if (abs(o[axis]) > half + m.curv.y) {{ hit = false; break; }} }}
            else {{
                let q0 = (-half - m.curv.y - o[axis]) / d[axis]; let q1 = (half + m.curv.y - o[axis]) / d[axis];
                a = max(a, min(q0, q1)); b = min(b, max(q0, q1));
            }}
        }}
        if (!hit || !(b > a)) {{ continue; }}
        var phi_knots: array<f32, 9>;
        for (var k = 0u; k < 9u; k++) {{
            var s = a + (b - a) * f32(k) / 8.0;
            if (k == 0u) {{ s = a; }} if (k == 8u) {{ s = b; }}
            phi_knots[k] = evalPhi(m, o, d, s);
        }}
        var allPositiveH = true;
        let mask0 = exactLevelActiveMask(m, d, a, b, -m.limits.w, &phi_knots, &allPositiveH);
        let mask1 = exactLevelActiveMask(m, d, a, b, 0.0, &phi_knots, &allPositiveH);
        let mask2 = exactLevelActiveMask(m, d, a, b, m.limits.w, &phi_knots, &allPositiveH);
        if (allPositiveH && (mask0 | mask1 | mask2) == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) {{
            if (work + 24u > cfg.budget) {{ reason |= 1u; break; }}
            work += 24u;
            memCount[member] = 0u;
            continue;
        }}
        var nb = 1u; knots[0] = a;
        var bands: array<vec2f, 32>; var nr = 0u; var zeroBands = 0u;
        roots(m, o, d, a, b, -m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask0);
        if (reason != 0u) {{ break; }}
        roots(m, o, d, a, b, 0.0, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, true, mask1);
        if (reason != 0u) {{ break; }}
        roots(m, o, d, a, b, m.limits.w, &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, false, mask2);
        if (reason != 0u) {{ break; }}
        if (d.x != 0.0) {{ let x = -o.x / d.x; if (x > a && x < b) {{ knots[nb] = x; nb++; }} }}
        if (d.y != 0.0) {{
            let y = -o.y / d.y; if (y > a && y < b) {{ knots[nb] = y; nb++; }}
            let k = (m.limits.z - o.y) / d.y; if (k > a && k < b) {{ knots[nb] = k; nb++; }}
        }}
        knots[nb] = b; nb++;
        for (var i = 1u; i < nb; i++) {{
            let v = knots[i]; var j = i; loop {{ if (j == 0u || knots[j - 1u] <= v) {{ break; }} knots[j] = knots[j - 1u]; j--; }}
            knots[j] = v;
        }}
        let memberStart = count; var previousShortBand = false; var previousPositive = false;
        for (var i = 1u; i < nb; i++) {{
            let l = knots[i - 1u]; let r = knots[i]; if (!(r > l)) {{ continue; }}
            let mid = l + 0.5 * (r - l); var uncertain = false; var protectedBoundary = false;
            if (d.x != 0.0) {{ protectedBoundary = l == -o.x / d.x; }}
            if (d.y != 0.0) {{ protectedBoundary = protectedBoundary || l == -o.y / d.y || l == (m.limits.z - o.y) / d.y; }}
            for (var k = 0u; k < nr; k++) {{
                uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
                if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) {{ protectedBoundary = true; }}
            }}
            if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {{
                if (count > memberStart && mayAbsorb(pieces[count - 1u], vec2f(l, r),
                     previousShortBand, previousPositive, uncertain, protectedBoundary)) {{
                    pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;
                }} else {{
                    if (count >= 48u) {{ reason |= 8u; break; }}
                    pieces[count] = vec2f(l, r);
                    count++; previousShortBand = uncertain && r - l <= 0.02; previousPositive = !uncertain;
                }}
            }}
        }}
        memCount[member] = count - memStart[member];
        if (reason != 0u) {{ break; }}
    }}
    if (reason != 0u) {{
        rays[id].counts = vec4u(subBase, 0u, overBase, FALLBACK | reason);
        return;
    }}

    // Pre-check total subpanel count across finalized pieces[0..count-1]
    var singles = 0u;
    for (var p = 0u; p < count; p++) {{
        let pa = pieces[p].x;
        let pb = pieces[p].y;
        let n = max(1u, u32(ceil((pb - pa) / 4.0)));
        if (singles + n > cfg.maxSubPerRay) {{
            reason |= 16u;
            break;
        }}
        singles += n;
    }}
    if (reason != 0u) {{
        rays[id].counts = vec4u(subBase, 0u, overBase, FALLBACK | reason);
        return;
    }}

    // Subdivide pieces directly in scalar registers and integrate into shared (L, T):
    let rd = d;
    let gx0 = 0.5 * (1.0 - 0.5773502691896257);
    let gx1 = 0.5 * (1.0 + 0.5773502691896257);
    var T = 1.0;
    var L = vec3<f32>(0.0);
    var capturedSubpanels = 0u;

    for (var member = 0u; member < 2u; member++) {{
        let m = media[member];
        let pOffset = u32(m.freq_offset.y);
        let freq = m.freq_offset.x;
        let alpha = m.color_alpha.w;
        let color = m.color_alpha.xyz;
        let start = memStart[member];
        let end = start + memCount[member];

        for (var p = start; p < end; p++) {{
            let pa = pieces[p].x;
            let pb = pieces[p].y;
            let n = max(1u, u32(ceil((pb - pa) / 4.0)));
            for (var k = 0u; k < n; k++) {{
                let s0 = select(pa + f32(k) * (pb - pa) / f32(n), pa, k == 0u);
                let s1 = select(pa + f32(k + 1u) * (pb - pa) / f32(n), pb, k + 1u == n);

                let captureBase = 2u * arrayLength(&rays) + id * 256u + capturedSubpanels * 4u;
                atomicStore(&auditWrites[captureBase], {captured_s0_s2});
                atomicStore(&auditWrites[captureBase + 1u], bitcast<u32>(s1));
                atomicStore(&auditWrites[captureBase + 2u], member);
                capturedSubpanels++;

                let sub_h = s1 - s0;

                let s_m = s0 + gx0 * sub_h;
                let s_p = s0 + gx1 * sub_h;
                let w_m = ro + s_m * rd - m.origin.xyz;
                let w_p = ro + s_p * rd - m.origin.xyz;

                let env_m = evalEnvelope(w_m, pOffset);
                let env_p = evalEnvelope(w_p, pOffset);

                var D_m = 0.0;
                if (env_m > 0.0) {{
                    let nm = cnoise3(freq * w_m);
                    D_m = env_m * clamp(0.7 + 0.3 * nm, 0.0, 1.0);
                }}
                var D_p = 0.0;
                if (env_p > 0.0) {{
                    let np = cnoise3(freq * w_p);
                    D_p = env_p * clamp(0.7 + 0.3 * np, 0.0, 1.0);
                }}

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
            }}
        }}
    }}

    {publish_s2}
    rays[id].counts = vec4u(subBase, singles, overBase, 0x40000000u);
}}
"""

    kS2TerminalProducerWgsl = s2_term_header[:idx_entry] + s2_compute_body

    # In s1 terminal shader: update capture layout to 4 words (s0, s1, member=0u, pad)
    s1_shader = s1_term
    captured_s0_s1 = "bitcast<u32>(s0) ^ 1u" if mutation == "terminal-endpoint-corrupt" else "bitcast<u32>(s0)"
    publish_s1 = "if (id > 1000000u) { results[id] = vec4<f32>(L, T); }" if mutation == "missing-s1" else "results[id] = vec4<f32>(L, T);\n    atomicAdd(&auditWrites[id * 2u + 1u], 1u);"

    needle_s1_res = "results[id] = vec4<f32>(L, T);"
    assert s1_shader.count(needle_s1_res) == 1
    s1_shader = s1_shader.replace(needle_s1_res, publish_s1)

    needle_s1_sub_h = "            let sub_h = s1 - s0;"
    assert s1_shader.count(needle_s1_sub_h) == 1
    s1_capture = f"""            let captureBase = 2u * arrayLength(&rays) + id * 256u + capturedSubpanels * 4u;
            atomicStore(&auditWrites[captureBase], {captured_s0_s1});
            atomicStore(&auditWrites[captureBase + 1u], bitcast<u32>(s1));
            atomicStore(&auditWrites[captureBase + 2u], 0u);
            capturedSubpanels++;
            let sub_h = s1 - s0;"""
    s1_shader = s1_shader.replace("    let rd = d;", "    var capturedSubpanels = 0u;\n    let rd = d;")
    s1_shader = s1_shader.replace(needle_s1_sub_h, s1_capture)

    # In s1 and s2 terminal shaders, declare auditWrites at binding 3
    decl_audit = "@group(0) @binding(3) var<storage, read_write> auditWrites: array<atomic<u32>>;\n"
    needle_rays = "@group(0) @binding(2) var<storage,read_write> rays:array<Ray>;\n"
    assert s1_shader.count(needle_rays) == 1
    s1_shader = s1_shader.replace(needle_rays, needle_rays + decl_audit)
    s1_shader = s1_shader.replace("    let input = rays[id];", "    atomicAdd(&auditWrites[id * 2u], 1u);\n    let input = rays[id];")

    assert kS2TerminalProducerWgsl.count(needle_rays) == 1
    kS2TerminalProducerWgsl = kS2TerminalProducerWgsl.replace(needle_rays, needle_rays + decl_audit)

    return {
        "kResetCountersWgsl": shaders_6dt["kResetCountersWgsl"],
        "kClassifyWgsl": shaders_6dt["kClassifyWgsl"],
        "kRouteArgsWgsl": shaders_6dt["kRouteArgsWgsl"],
        "kS1UnfusedProducerWgsl": kS1UnfusedProducerWgsl,
        "kS1TerminalProducerWgsl": s1_shader,
        "kS2ProducerWgsl": shaders_6dt["kS2ProducerWgsl"],
        "kS2TerminalProducerWgsl": kS2TerminalProducerWgsl,
        "kFallbackProducerWgsl": shaders_6dt["kFallbackProducerWgsl"],
    }


BRANCH = r"""        J routeChecks=J::array(); auto originalInputs=gpuRays;
        auto bitsEqual=[](const GpuResult& a,const GpuResult& b){for(int k=0;k<4;++k)if(!std::isfinite(a.rgb_T[k])||std::bit_cast<uint32_t>(a.rgb_T[k])!=std::bit_cast<uint32_t>(b.rgb_T[k]))return false;return true;};
        auto poisonOutput=[&](){std::vector<GpuResult> x(executionRays);for(auto& a:x)for(float& v:a.rgb_T)v=NAN;wgpuQueueWriteBuffer(gpu.queue,outBuf,0,x.data(),outBytes);};
        if(cfg.maxSubPerRay>64)throw std::runtime_error("diagnostic endpoint capacity requires at most 64 panels per ray");
        auto poisonSchedule=[&](){
            std::vector<uint32_t> pSub(subpanelBytes / 4, 0xcdcdcdcd);
            wgpuQueueWriteBuffer(gpu.queue, sBuf, 0, pSub.data(), subpanelBytes);
            std::vector<uint32_t> pOver(overlapBytes / 4, 0xcdcdcdcd);
            wgpuQueueWriteBuffer(gpu.queue, oBuf, 0, pOver.data(), overlapBytes);
        };
        std::vector<GpuResult> currentReview,bulkReview; std::vector<uint8_t> refH,refS,refO;
        auto baselineReview=[&](){execute(0,1,false);currentReview=readResults();execute(1,1,false);bulkReview=readResults();refH=readBackBuffer(rBuf,rayBytes);refS=readBackBuffer(sBuf,subpanelBytes);refO=readBackBuffer(oBuf,overlapBytes);};
        auto checkReview=[&](const std::string& label,int rep,bool normal){
            poisonOutput();
            poisonSchedule();
            execute(4,rep,false); // Mode 4: Terminal S1 + Terminal S2 candidate
            auto output=readResults();
            auto hb=readBackBuffer(rBuf,rayBytes);
            auto sb=readBackBuffer(sBuf,subpanelBytes);
            auto ob=readBackBuffer(oBuf,overlapBytes);
            auto cb=readBackBuffer(cntBuf,sizeof(initCnt));
            auto ab=readBackBuffer(argsBuf,36);
            auto wb=readBackBuffer(auditWritesBuf,auditBytes);
            DispatchCounters c{};std::memcpy(&c,cb.data(),sizeof(c));std::array<uint32_t,9> args{};std::memcpy(args.data(),ab.data(),36);
            if(c.s1+c.s2+c.fb!=2304u)throw std::runtime_error("routed counter accumulation mismatch");
            const uint32_t ns[3]={c.s1,c.s2,c.fb};for(int k=0;k<3;++k)if(args[3*k]!=(ns[k]+3)/4||args[3*k+1]!=1||args[3*k+2]!=1)throw std::runtime_error("routed indirect args mismatch");
            auto writes=reinterpret_cast<const uint32_t*>(wb.data());
            auto hs=reinterpret_cast<const GpuRayHeader*>(hb.data());auto rh=reinterpret_cast<const GpuRayHeader*>(refH.data());
            auto subs=reinterpret_cast<const GpuSubpanel*>(sb.data());auto rs=reinterpret_cast<const GpuSubpanel*>(refS.data());auto overs=reinterpret_cast<const GpuOverlap*>(ob.data());auto rr=reinterpret_cast<const GpuOverlap*>(refO.data());
            const auto* sWords=reinterpret_cast<const uint32_t*>(sb.data());
            uint32_t refused=0,packets=0,wrong=0,owners=0,finalOwners=0,completed_s1=0,completed_s2=0,terminalEndpointChecks=0;J flags=J::array(),rows=J::array(),terminalEndpoints=J::array();
            for(size_t i=0;i<count;++i){
                uint32_t producerEntries = writes[i * 2];
                uint32_t finalPublishes = writes[i * 2 + 1];
                owners += (producerEntries != 1u);
                finalOwners += (finalPublishes != 1u);
                uint32_t flagBits = hs[i].overlapCount & 0xC0000000u;
                if(flagBits == 0xC0000000u) throw std::runtime_error("corrupt mutually exclusive ownership flags: ray=" + std::to_string(i));
                bool fallback=(flagBits & 0x80000000u)!=0;
                bool completed=(flagBits & 0x40000000u)!=0;
                refused+=fallback;
                const auto& expected=normal?bulkReview[i]:(fallback?currentReview[i]:bulkReview[i]);
                bool eq = bitsEqual(output[i],expected);
                if (!eq) {
                    if (wrong < 5) {
                        std::cerr << "MISMATCH [" << label << "] ray=" << i << " out=(" << output[i].rgb_T[0] << "," << output[i].rgb_T[1] << "," << output[i].rgb_T[2] << "," << output[i].rgb_T[3]
                                  << ") exp=(" << expected.rgb_T[0] << "," << expected.rgb_T[1] << "," << expected.rgb_T[2] << "," << expected.rgb_T[3]
                                  << ") fallback=" << fallback << " completed=" << completed << " flagBits=0x" << std::hex << flagBits << std::dec
                                  << " finalWrites=" << finalPublishes << " revWrites=" << producerEntries
                                  << " hexOut=[" << std::hex << std::bit_cast<uint32_t>(output[i].rgb_T[0]) << "," << std::bit_cast<uint32_t>(output[i].rgb_T[1]) << "," << std::bit_cast<uint32_t>(output[i].rgb_T[2]) << "," << std::bit_cast<uint32_t>(output[i].rgb_T[3]) << "]"
                                  << " hexExp=[" << std::bit_cast<uint32_t>(expected.rgb_T[0]) << "," << std::bit_cast<uint32_t>(expected.rgb_T[1]) << "," << std::bit_cast<uint32_t>(expected.rgb_T[2]) << "," << std::bit_cast<uint32_t>(expected.rgb_T[3]) << "]" << std::dec << std::endl;
                    }
                    wrong++;
                }

                if(completed){
                    // Identify S1 vs S2: check if any reference subpanel has mediumIndex == 1u
                    bool hasMember1 = false;
                    for(uint32_t k = 0; k < rh[i].subpanelCount; ++k){
                        if(rs[rh[i].subpanelOffset + k].mediumIndex == 1u) hasMember1 = true;
                    }
                    if(hasMember1) completed_s2++; else completed_s1++;

                    if(hs[i].subpanelCount != rh[i].subpanelCount || (hs[i].overlapCount & 0x00ffffffu) != 0u) ++packets;
                    J captured=J::array();
                    for(uint32_t k=0;k<hs[i].subpanelCount;++k){
                        uint32_t captureBase=2u*count+i*256u+4u*k;
                        uint32_t s0= writes[captureBase],s1=writes[captureBase+1],mem=writes[captureBase+2];
                        auto expectedSub=rs[rh[i].subpanelOffset+k];
                        if(s0!=std::bit_cast<uint32_t>(expectedSub.s0)||s1!=std::bit_cast<uint32_t>(expectedSub.s1)||mem!=expectedSub.mediumIndex)++packets;
                        ++terminalEndpointChecks;
                        if(rep==1)captured.push_back({s0,s1,mem});
                    }
                    if(rep==1&&hs[i].subpanelCount)terminalEndpoints.push_back({i,captured});
                    uint32_t offsetWords = hs[i].subpanelOffset * (sizeof(GpuSubpanel) / 4);
                    uint32_t countWords = hs[i].subpanelCount * (sizeof(GpuSubpanel) / 4);
                    if(uint64_t(offsetWords) + countWords > sb.size() / 4) throw std::runtime_error("terminal packet range outside schedule storage");
                    for(uint32_t w = 0; w < countWords; ++w){
                        if(sWords[offsetWords + w] != 0xcdcdcdcd){
                            throw std::runtime_error("violation: global sBuf written for terminal ray " + std::to_string(i));
                        }
                    }
                } else if(!fallback){
                    bool same=hs[i].subpanelCount==rh[i].subpanelCount&&(hs[i].overlapCount&0x00ffffffu)==rh[i].overlapCount;
                    if(same){
                        for(uint32_t k=0;k<hs[i].subpanelCount;++k){auto a=subs[hs[i].subpanelOffset+k],b=rs[rh[i].subpanelOffset+k];same=same&&std::bit_cast<uint32_t>(a.s0)==std::bit_cast<uint32_t>(b.s0)&&std::bit_cast<uint32_t>(a.s1)==std::bit_cast<uint32_t>(b.s1)&&a.mediumIndex==b.mediumIndex;}
                        for(uint32_t k=0;k<(hs[i].overlapCount&0x00ffffffu);++k){auto a=overs[hs[i].overlapOffset+k],b=rr[rh[i].overlapOffset+k];same=same&&std::bit_cast<uint32_t>(a.s0)==std::bit_cast<uint32_t>(b.s0)&&std::bit_cast<uint32_t>(a.s1)==std::bit_cast<uint32_t>(b.s1)&&a.activeMask==b.activeMask;}
                    }
                    if(!same)packets++;
                }
                flags.push_back(hs[i].overlapCount);
                if(rep==1){J a=J::array(),e=J::array();for(int k=0;k<4;++k){a.push_back(std::bit_cast<uint32_t>(output[i].rgb_T[k]));e.push_back(std::bit_cast<uint32_t>(expected.rgb_T[k]));}rows.push_back({i,a,e});}
            }
            if(owners)throw std::runtime_error("routed exactly-once producer publication failure: " + std::to_string(owners) + " errors");
            if(finalOwners)throw std::runtime_error("routed exactly-once final output publication failure: " + std::to_string(finalOwners) + " errors");
            if(wrong||packets)throw std::runtime_error("routed composed path differs: "+std::to_string(wrong)+" outputs, "+std::to_string(packets)+" packets");
            routeChecks.push_back({{"case",label},{"repeats",rep},{"rays",count},{"family_counts",{c.s1,c.s2,c.fb}},{"indirect_args",args},{"producer_entry_errors",owners},{"final_output_errors",finalOwners},{"output_differences",wrong},{"packet_differences",normal?J(packets):J(nullptr)},{"refused",refused},{"completed_s1",completed_s1},{"completed_s2",completed_s2},{"terminal_endpoint_checks",terminalEndpointChecks},{"terminal_endpoints",terminalEndpoints},{"flags",flags},{"outputs",rows}});
            std::cerr<<"ROUTED REVIEW "<<label<<" repeats="<<rep<<" PASS counts="<<c.s1<<"/"<<c.s2<<"/"<<c.fb<<" completed_s1="<<completed_s1<<" completed_s2="<<completed_s2<<" refused="<<refused<<" finalOwners="<<finalOwners<<" (0 sBuf writes verified)\n";
        };
        for(int pose:{0,1,0,2,3,0}){
            gpuRays=originalInputs;
            for(auto& a:gpuRays){if(pose==1)a.ro[0]+=10000.f;if(pose==2||pose==3){a.ro[0]=0;a.ro[1]=50;a.ro[2]=240;a.rd[2]=-std::abs(a.rd[2]);a.clipA=0;a.clipB=1000;}if(pose==3){a.rd[0]=0;a.rd[1]=0;a.rd[2]=-1;}}
            wgpuQueueWriteBuffer(gpu.queue,rBuf,0,gpuRays.data(),rayBytes);baselineReview();
            for(int rep:{1,2,4,9})checkReview("pose-"+std::to_string(pose),rep,true);
        }
        for(uint32_t budget:{0u,23u,24u,25u}){auto limited=cfg;limited.pad=budget;wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&limited,sizeof(limited));checkReview("budget-"+std::to_string(budget),1,false);}
        wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&cfg,sizeof(cfg));
        for(int kind:{0,1}){auto limited=cfg;if(kind==0)limited.maxSubPerRay=4;else limited.maxOverPerRay=1;wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&limited,sizeof(limited));checkReview("capacity-"+std::to_string(kind),1,false);}
        wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&cfg,sizeof(cfg));
        std::ofstream reviewOut(outputPath);reviewOut<<J({{"scope","§6ej Terminal S2 routed correctness and ownership controls"},{"cases",routeChecks},{"clean_timing",false},{"shipping_qualified",false}}).dump(2)<<std::endl;return 0;
"""


def generate(root: Path, mutation: str = "fixed") -> str:
    m = load(root, "ray_transport_gpu_6dt_true_terminal_s1")
    v = {
        x.targets[0].id: ast.literal_eval(x.value)
        for x in ast.parse((root / "scratch/probes/ray_transport_metal_diagnostic.py").read_text()).body
        if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)
    }
    source = m.make_terminal_s1_cpp_source(v["CPP_SOURCE"], root, wide=True)

    # Flush stderr on WebGPU error
    source = source.replace(
        'std::fprintf(stderr, "[WebGPU Error %d]: %.*s\\n", (int)type, (int)msg.length, msg.data);',
        'std::fprintf(stderr, "[WebGPU Error %d]: %.*s\\n", (int)type, (int)msg.length, msg.data); std::fflush(stderr);'
    )

    # Replace shaders with §6ej shaders
    shaders = make_terminal_s2_shaders(root, mutation=mutation)

    # Replace kS1ProducerWgsl, kS2ProducerWgsl, kFallbackProducerWgsl definitions
    def replace_shader_def(src, name, val):
        hit = re.search(r'(static const char\* ' + name + r' = R"WGSL\()(.*?)(\)WGSL";)', src, re.S)
        assert hit, f"missing shader def {name}"
        return src[:hit.start(2)] + val + src[hit.end(2):]

    source = replace_shader_def(source, "kS1ProducerWgsl", shaders["kS1TerminalProducerWgsl"])
    source = replace_shader_def(source, "kS2ProducerWgsl", shaders["kS2ProducerWgsl"])
    source = replace_shader_def(source, "kFallbackProducerWgsl", shaders["kFallbackProducerWgsl"])

    # Inject kS2TerminalProducerWgsl definition right after kS2ProducerWgsl
    hit_s2 = re.search(r'static const char\* kS2ProducerWgsl = R"WGSL\((.*?)\)WGSL";', source, re.S)
    assert hit_s2
    s2_term_decl = f'\nstatic const char* kS2TerminalProducerWgsl = R"WGSL({shaders["kS2TerminalProducerWgsl"]})WGSL";\n'
    source = source[:hit_s2.end()] + s2_term_decl + source[hit_s2.end():]

    # Buffer: auditWritesBuf (258 words per ray: 2 counters + 64 * 4 words)
    needle_out_buf = '    WGPUBuffer readBuf = makeBuffer(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, outBytes, nullptr);'
    assert source.count(needle_out_buf) == 1
    new_buffers = (
        needle_out_buf + '\n' +
        '    size_t auditBytes = executionRays * (2 + 4 * 64) * sizeof(uint32_t);\n' +
        '    WGPUBuffer auditWritesBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, auditBytes, nullptr);'
    )
    source = source.replace(needle_out_buf, new_buffers)

    # Inject auditWrites at binding 9 for kS2ProducerWgsl and kFallbackProducerWgsl
    for name in ['kS2ProducerWgsl', 'kFallbackProducerWgsl']:
        hit = re.search(r'(static const char\* ' + name + r' = R"WGSL\()(.*?)(\)WGSL";)', source, re.S)
        assert hit
        shader = hit[2]
        needle = '@group(0) @binding(8) var<storage, read> routeCounts: array<u32>;'
        assert shader.count(needle) == 1
        shader = shader.replace(needle, needle + '\n@group(0) @binding(9) var<storage, read_write> auditWrites: array<atomic<u32>>;')
        needle_input = '    let input = rays[id];' if name == 'kS2ProducerWgsl' else ' let input = rays[id];'
        assert shader.count(needle_input) == 1
        shader = shader.replace(needle_input, ' atomicAdd(&auditWrites[id * 2u], 1u);\n' + needle_input)
        source = source[:hit.start(2)] + shader + source[hit.end(2):]

    # Common WGSL header (consumer shaders): binding 7 for auditWrites
    old_common_results = '@group(0) @binding(6) var<storage, read_write> results: array<vec4<f32>>;'
    new_common_results = (
        old_common_results + '\n' +
        '@group(0) @binding(7) var<storage, read_write> auditWrites: array<atomic<u32>>;'
    )
    assert source.count(old_common_results) == 1
    source = source.replace(old_common_results, new_common_results)

    # Decode counts.w low 24 bits for overlap count in consumers
    assert source.count('let overCount = r.counts.w;') == 2
    source = source.replace('let overCount = r.counts.w;', 'let overCount = r.counts.w & 0x00ffffffu;')

    # Instrument cpUnfused results write
    hit_unfused = re.search(r'(static const char\* kUnfusedShader = R"WGSL\()(.*?)(\)WGSL";)', source, re.S)
    assert hit_unfused
    unfused_shader = hit_unfused[2]
    needle_unfused_ray = '    results[rayIdx] = vec4<f32>(L, T);'
    assert unfused_shader.count(needle_unfused_ray) == 1
    unfused_shader = unfused_shader.replace(needle_unfused_ray, needle_unfused_ray + '\n    atomicAdd(&auditWrites[rayIdx * 2u + 1u], 1u);')

    if mutation == 'double-publish':
        needle_completed_check = '    if((r.counts.w & 0x40000000u) != 0u){ return; }'
        assert unfused_shader.count(needle_completed_check) == 1
        unfused_shader = unfused_shader.replace(needle_completed_check, '    // S1/S2 completed guard omitted for double-publish mutation')

    source = source[:hit_unfused.start(2)] + unfused_shader + source[hit_unfused.end(2):]

    # Also instrument results write in kFusedShader
    hit_fused = re.search(r'(static const char\* kFusedShader = R"WGSL\()(.*?)(\)WGSL";)', source, re.S)
    assert hit_fused
    fused_shader = hit_fused[2]
    assert fused_shader.count(needle_unfused_ray) == 1
    fused_shader = fused_shader.replace(needle_unfused_ray, needle_unfused_ray + '\n    atomicAdd(&auditWrites[rayIdx * 2u + 1u], 1u);')
    source = source[:hit_fused.start(2)] + fused_shader + source[hit_fused.end(2):]

    # Bind groups for cpUnfused and cpFused: add entry 7 for auditWritesBuf
    old_make_bg = '''        WGPUBindGroupEntry entries[7] = {};
        entries[0].binding = 0; entries[0].buffer = uBuf; entries[0].size = sizeof(udata);
        entries[1].binding = 1; entries[1].buffer = mBuf; entries[1].size = mediaBytes;
        entries[2].binding = 2; entries[2].buffer = pBuf; entries[2].size = paramBytes;
        entries[3].binding = 3; entries[3].buffer = rBuf; entries[3].size = rayBytes;
        entries[4].binding = 4; entries[4].buffer = sBuf; entries[4].size = subpanelBytes;
        entries[5].binding = 5; entries[5].buffer = oBuf; entries[5].size = overlapBytes > 0 ? overlapBytes : 16;
        entries[6].binding = 6; entries[6].buffer = outBuf; entries[6].size = outBytes;

        WGPUBindGroupDescriptor bgd = {};
        bgd.layout = bgl;
        bgd.entryCount = 7;
        bgd.entries = entries;'''

    new_make_bg = '''        WGPUBindGroupEntry entries[8] = {};
        entries[0].binding = 0; entries[0].buffer = uBuf; entries[0].size = sizeof(udata);
        entries[1].binding = 1; entries[1].buffer = mBuf; entries[1].size = mediaBytes;
        entries[2].binding = 2; entries[2].buffer = pBuf; entries[2].size = paramBytes;
        entries[3].binding = 3; entries[3].buffer = rBuf; entries[3].size = rayBytes;
        entries[4].binding = 4; entries[4].buffer = sBuf; entries[4].size = subpanelBytes;
        entries[5].binding = 5; entries[5].buffer = oBuf; entries[5].size = overlapBytes > 0 ? overlapBytes : 16;
        entries[6].binding = 6; entries[6].buffer = outBuf; entries[6].size = outBytes;
        entries[7].binding = 7; entries[7].buffer = auditWritesBuf; entries[7].size = auditBytes;

        WGPUBindGroupDescriptor bgd = {};
        bgd.layout = bgl;
        bgd.entryCount = 8;
        bgd.entries = entries;'''
    assert source.count(old_make_bg) == 1
    source = source.replace(old_make_bg, new_make_bg)

    # In tile streaming loop: update consumer bind group
    old_tile_consumer = 'entry(6,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult))});'
    new_tile_consumer = 'entry(6,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult)), entry(7,auditWritesBuf,outputOffset/2,uint64_t(n)*2*sizeof(uint32_t))});'
    assert source.count(old_tile_consumer) == 1
    source = source.replace(old_tile_consumer, new_tile_consumer)

    # Fallback/Current shader bindings and instrumentation
    old_extra = (
        '    const std::string extra=R"WGSL(\n'
        'struct CapturedRay { ro_clipA:vec4<f32>,rd_clipB:vec4<f32>,counts:vec4<u32> };\n'
        '@group(0) @binding(3) var<storage,read> capturedRays:array<CapturedRay>;\n'
        '@group(0) @binding(4) var<storage,read_write> capturedOutput:array<vec4<f32>>;\n'
        ')WGSL";'
    )
    new_extra = (
        '    const std::string extra=R"WGSL(\n'
        'struct CapturedRay { ro_clipA:vec4<f32>,rd_clipB:vec4<f32>,counts:vec4<u32> };\n'
        '@group(0) @binding(3) var<storage,read> capturedRays:array<CapturedRay>;\n'
        '@group(0) @binding(4) var<storage,read_write> capturedOutput:array<vec4<f32>>;\n'
        '@group(0) @binding(5) var<storage,read_write> auditWrites:array<atomic<u32>>;\n'
        ')WGSL";'
    )
    assert source.count(old_extra) == 1
    source = source.replace(old_extra, new_extra)

    needle_curr_out = ' capturedOutput[idx]=vec4<f32>(integratedRadiance,transmittance);'
    assert source.count(needle_curr_out) == 1
    source = source.replace(needle_curr_out, needle_curr_out + '\n atomicAdd(&auditWrites[idx * 2u + 1u], 1u);')

    needle_curr_discard = 'capturedOutput[idx]=vec4<f32>(0.0,0.0,0.0,1.0);return;'
    assert source.count(needle_curr_discard) == 1
    source = source.replace(needle_curr_discard, 'capturedOutput[idx]=vec4<f32>(0.0,0.0,0.0,1.0); atomicAdd(&auditWrites[idx * 2u + 1u], 1u); return;')

    old_bind_current = '''    auto bindCurrent=[&](WGPUComputePipeline cp) {
        WGPUBindGroupEntry entries[5]={};
        WGPUBuffer buffers[5]={cuBuf,cparamsBuf,ciBuf,rBuf,outBuf};
        uint64_t sizes[5]={sizeof(currentU),currentParams.size()*4,currentInstances.size()*sizeof(CurrentInstance),rayBytes,outBytes};
        for(uint32_t k=0;k<5;++k){entries[k].binding=k;entries[k].buffer=buffers[k];entries[k].size=sizes[k];}
        auto layout=wgpuComputePipelineGetBindGroupLayout(cp,0);WGPUBindGroupDescriptor d={};d.layout=layout;d.entryCount=5;d.entries=entries;
        auto bg=wgpuDeviceCreateBindGroup(gpu.device,&d);wgpuBindGroupLayoutRelease(layout);return bg;
    };
    auto currentBG=bindCurrent(currentCP),prepBG=bindCurrent(prepCP);'''
    new_bind_current = '''    auto bindCurrent5=[&](WGPUComputePipeline cp) {
        WGPUBindGroupEntry entries[5]={};
        WGPUBuffer buffers[5]={cuBuf,cparamsBuf,ciBuf,rBuf,outBuf};
        uint64_t sizes[5]={sizeof(currentU),currentParams.size()*4,currentInstances.size()*sizeof(CurrentInstance),rayBytes,outBytes};
        for(uint32_t k=0;k<5;++k){entries[k].binding=k;entries[k].buffer=buffers[k];entries[k].size=sizes[k];}
        auto layout=wgpuComputePipelineGetBindGroupLayout(cp,0);WGPUBindGroupDescriptor d={};d.layout=layout;d.entryCount=5;d.entries=entries;
        auto bg=wgpuDeviceCreateBindGroup(gpu.device,&d);wgpuBindGroupLayoutRelease(layout);return bg;
    };
    auto bindCurrent=[&](WGPUComputePipeline cp) {
        WGPUBindGroupEntry entries[6]={};
        WGPUBuffer buffers[6]={cuBuf,cparamsBuf,ciBuf,rBuf,outBuf,auditWritesBuf};
        uint64_t sizes[6]={sizeof(currentU),currentParams.size()*4,currentInstances.size()*sizeof(CurrentInstance),rayBytes,outBytes,auditBytes};
        for(uint32_t k=0;k<6;++k){entries[k].binding=k;entries[k].buffer=buffers[k];entries[k].size=sizes[k];}
        auto layout=wgpuComputePipelineGetBindGroupLayout(cp,0);WGPUBindGroupDescriptor d={};d.layout=layout;d.entryCount=6;d.entries=entries;
        auto bg=wgpuDeviceCreateBindGroup(gpu.device,&d);wgpuBindGroupLayoutRelease(layout);return bg;
    };
    auto prepBG=bindCurrent5(prepCP),currentBG=bindCurrent(currentCP);'''
    assert source.count(old_bind_current) == 1
    source = source.replace(old_bind_current, new_bind_current)

    old_fallback_hook = '    fallbackShader.insert(where+rayHook.size(),"\\n if((ray.counts.w & 0x80000000u)==0u){return;}\\n");'
    assert source.count(old_fallback_hook) == 1
    if mutation == 'missing-fallback':
        mutation_patch = (
            old_fallback_hook + '\n' +
            '    {\n' +
            '        const std::string target = "capturedOutput[idx]=vec4<f32>(integratedRadiance,transmittance);\\n atomicAdd(&auditWrites[idx * 2u + 1u], 1u);";\n' +
            '        auto p = fallbackShader.find(target);\n' +
            '        if(p == std::string::npos) throw std::runtime_error("fallback missing-publish mutation hook absent");\n' +
            '        fallbackShader.replace(p, target.size(), "if(idx > 1000000u) { capturedOutput[idx]=vec4<f32>(integratedRadiance,transmittance); atomicAdd(&auditWrites[idx * 2u + 1u], 1u); }");\n' +
            '    }'
        )
        source = source.replace(old_fallback_hook, mutation_patch)

    old_tile_current = 'entry(4,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult))});'
    assert source.count(old_tile_current) == 1
    new_tile_current = 'entry(4,outBuf,outputOffset,uint64_t(n)*sizeof(GpuResult)), entry(5,auditWritesBuf,outputOffset/2,uint64_t(n)*2*sizeof(uint32_t))});'
    source = source.replace(old_tile_current, new_tile_current)

    # Bind groups for S1: add auditWrites at binding 3 (in both normal and forced bgS1!)
    needle_bg_s1 = ('        makeEntry(2, rBuf, 0, rayBytes),\n' +
                    '        makeEntry(4, outBuf, 0, outBytes),')
    assert source.count(needle_bg_s1) == 2
    source = source.replace(needle_bg_s1,
                            '        makeEntry(2, rBuf, 0, rayBytes),\n' +
                            '        makeEntry(3, auditWritesBuf, 0, auditBytes),\n' +
                            '        makeEntry(4, outBuf, 0, outBytes),')

    # Bind groups for S2 and fallback: add auditWrites at binding 9
    needle_bg_s2_fb = 'makeEntry(8, cntBuf, 0, sizeof(initCnt))\n    });'
    assert source.count(needle_bg_s2_fb) == 4
    source = source.replace(needle_bg_s2_fb, 'makeEntry(8, cntBuf, 0, sizeof(initCnt)),\n        makeEntry(9, auditWritesBuf, 0, count * 8)\n    });')

    # Pipelines: add cpS2Terminal
    needle_pipe_s2 = 'auto cpS2 = buildCompute(kS2ProducerWgsl, "s2ProducerMain");'
    assert source.count(needle_pipe_s2) == 1
    new_pipe_s2 = (
        needle_pipe_s2 + '\n' +
        '    auto cpS2Terminal = buildCompute(kS2TerminalProducerWgsl, "s2TerminalProducerMain");'
    )
    source = source.replace(needle_pipe_s2, new_pipe_s2)

    # Bind groups for cpS2Terminal: bgS2Terminal and bgS2TerminalForced
    needle_bg_s2 = '    auto bgS2 = makeGroup(cpS2, {'
    assert source.count(needle_bg_s2) == 1
    new_bg_s2_term = f'''    auto bgS2Terminal = makeGroup(cpS2Terminal, {{
        makeEntry(0, cfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, auditWritesBuf, 0, auditBytes),
        makeEntry(4, outBuf, 0, outBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s2TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt)),
        makeEntry(9, mBuf, 0, mediaBytes),
        makeEntry(10, pBuf, 0, paramBytes)
    }});
    auto bgS2TerminalForced = makeGroup(cpS2Terminal, {{
        makeEntry(0, forcedCfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, auditWritesBuf, 0, auditBytes),
        makeEntry(4, outBuf, 0, outBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s2TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt)),
        makeEntry(9, mBuf, 0, mediaBytes),
        makeEntry(10, pBuf, 0, paramBytes)
    }});
'''
    source = source.replace(needle_bg_s2, new_bg_s2_term + needle_bg_s2)

    # In execute(mode, ...): add Mode 4 dispatch (Terminal S1 + Terminal S2 Candidate)
    # Mode 4 uses cpS1Terminal (argsBuf, 0) AND cpS2Terminal (argsBuf, 12)
    needle_mode3_tail = '''                    // Pass 6: Canonical Unfused A* Consumer (skips completed S1 rays and refused rays)
                    dispatch(cpUnfused, bgUnfused, count);
                    // Pass 7: Production-Generated Current Marcher Fallback (only for refused rays)
                    dispatch(fallbackCP, fallbackBG, count);
                }'''
    assert source.count(needle_mode3_tail) == 1

    mode4_dispatch = '''
                else if(mode==4){
                    // Candidate Mode 4: Terminal S1 + Terminal S2 (Unchanged Bisection Only)
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
                        wgpuComputePassEncoderSetPipeline(p, cpS1);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS1Forced : bgS1, 0, nullptr);
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
                }'''

    source = source.replace(needle_mode3_tail, needle_mode3_tail + mode4_dispatch)

    # Clear auditWritesBuf in mode 3 and mode 4
    a = source.index('        auto execute=')
    b = source.index('        auto correctness=', a)
    part = source[a:b]
    needle_repeats = '            for(int k=0;k<repeats;++k){'
    assert part.count(needle_repeats) == 1
    part = part.replace(needle_repeats, needle_repeats + '\n                if(mode==3||mode==4) wgpuCommandEncoderClearBuffer(enc,auditWritesBuf,0,auditBytes);')
    source = source[:a] + part + source[b:]

    # Inject BRANCH harness
    source = source[:source.index('        auto correctness=', a)] + BRANCH + source[source.index('        auto correctness=', a):]

    s = source
    if mutation == 'no-reset':
        start = s.index('        auto execute=')
        end = s.index('        J routeChecks=', start)
        old_reset = 'auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);\n                        wgpuComputePassEncoderSetPipeline(p, cpReset);\n                        wgpuComputePassEncoderSetBindGroup(p, 0, bgReset, 0, nullptr);\n                        wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);\n                        wgpuComputePassEncoderEnd(p);\n                        wgpuComputePassEncoderRelease(p);'
        assert s.count(old_reset, start, end) >= 1
        s = s[:start] + s[start:end].replace(old_reset, '// reset omitted for no-reset mutation') + s[end:]

    return s


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--build-root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--mutation', choices=['fixed', 'no-reset', 'double-publish', 'missing-s1', 'missing-s2', 'missing-fallback', 'late-member1-refusal', 'terminal-endpoint-corrupt'], default='fixed')
    p.add_argument('--case', type=Path, default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    p.add_argument('--run', action='store_true')
    args = p.parse_args()

    root = args.root.resolve()
    build_root = args.build_root.resolve()
    worktree_root = build_root.parent if build_root.name == 'build' else build_root
    work = args.work.resolve()

    if work.exists() or work == root or root in work.parents:
        raise ValueError('fresh external work required; existing directories are never deleted')
    work.mkdir(parents=True, exist_ok=False)

    print(f'Generating source for mutation: {args.mutation}...')
    cpp_source = generate(root, mutation=args.mutation)
    (work / 'probe.cpp').write_text(cpp_source)

    build_dir = work / 'build'
    subprocess.run([
        sys.executable, '-B', str(root / 'scratch/probes/ray_transport_probe_build.py'),
        '--root', str(root),
        '--build-root', str(worktree_root),
        '--source', str(work / 'probe.cpp'),
        '--work', str(build_dir)
    ], check=True)
    print('Compilation successful!')

    if args.run:
        spec = importlib.util.spec_from_file_location('wide', root / 'scratch/probes/ray_transport_gpu_wide_stream.py')
        m = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(m)
        zone = root / 'saves/zones/Northern Veil/zone.ecform'
        before = hashlib.sha256(zone.read_bytes()).hexdigest()
        meta, rays, _ = m.verify_case(root, args.case.resolve(), before)
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
        env.pop('METAL_CAPTURE_ENABLED', None)

        output = work / 'review.json'
        print(f'Executing control probe for mutation: {args.mutation}...')
        result = subprocess.run([str(build_dir / 'probe'), str(output)], cwd=root, env=env)
        if hashlib.sha256(zone.read_bytes()).hexdigest() != before:
            raise ValueError('source drift')
        m.verify_case(root, args.case.resolve(), before)
        if result.returncode != 0:
            print(f'Control probe exited with code {result.returncode}')
            sys.exit(result.returncode)
        else:
            print(f'PASS control probe: {args.mutation}')


if __name__ == '__main__':
    main()
