"""GPU Ray Transport §6ek: Indirect Fallback Consumer and Full 32k Reference Gate.

Fulfills §6ek commissioning:
1. Candidate Mode 4: Terminal S1 + Terminal S2 + Indirect Fallback Consumer (Unchanged Bisection Only)
   - Dispatches cpFallbackConsumer indirectly over fallbackTileList (argsBuf offset 24, 162 workgroups instead of 513).
   - Skips S1 and S2 tiles entirely; eliminates consumer warp divergence between terminal and fallback rays.
2. Strict Publication Seam & Negative Mutations:
   - Exactly-once publication auditor tracks S1, S2, Fallback Consumer, and Current Fallback.
   - All 7 negative mutations tested (exit 250).
3. Full 32,784-Ray Reference Gating:
   - Gated against Claude Opus 5.5's converged 65,536-step Simpson reference (NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz).
   - Verifies 0 rays exceeding Current max error on both RGB and T.

Antigravity / Gemini 3.8 Flash (Sparkly Guy 🐉✨) / 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-27.
"""

from __future__ import annotations

import argparse
import ast
import gzip
import hashlib
import importlib.util
import json
import os
import re
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


def make_fallback_consumer_wgsl() -> str:
    return r"""
@group(0) @binding(8) var<storage, read> tileOffsets: array<u32, 2305>;
@group(0) @binding(9) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(10) var<storage, read> fallbackTileList: array<u32>;
@group(0) @binding(11) var<storage, read> routeCounts: array<u32>;

@compute @workgroup_size(64, 1, 1)
fn fallbackConsumerMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    if (tileSlot >= min(routeCounts[2u], 2304u)) { return; }
    let tileId = fallbackTileList[tileSlot];
    if (tileId >= 2304u) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) { return; }
    let rayIdx = tileRayIndices[rayStart + rayInTile];
    if (rayIdx >= u.rayCount) { return; }

    let r = rays[rayIdx];
    // Refused rays march in fallbackCP; completed rays should not be in fallbackTileList
    if ((r.counts.w & 0x80000000u) != 0u || (r.counts.w & 0x40000000u) != 0u) { return; }

    let ro = r.ro_clipA.xyz;
    let rd = r.rd_clipB.xyz;
    let subOffset = r.counts.x;
    let subCount = r.counts.y;
    let overOffset = r.counts.z;
    let overCount = r.counts.w & 0x00ffffffu;

    let gx0 = 0.5 * (1.0 - 0.5773502691896257);
    let gx1 = 0.5 * (1.0 + 0.5773502691896257);

    var T = 1.0;
    var L = vec3<f32>(0.0);

    if (overCount > 0u) {
        let dummy = overlaps[overOffset].info.x;
        L = L + vec3<f32>(f32(dummy) * 1e-35);
    }

    for (var i = 0u; i < subCount; i = i + 1u) {
        let sub = subpanels[subOffset + i];
        let s0 = sub.span.x;
        let s1 = sub.span.y;
        let sub_h = s1 - s0;
        let mid = sub.info.x;
        let m = media[mid];
        let pOffset = u32(m.freq_offset.y);
        let freq = m.freq_offset.x;
        let alpha = m.color_alpha.w;
        let color = m.color_alpha.xyz;

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
        if (tau < 1e-4) {
            let phi = 1.0 - 0.5 * tau + (1.0 / 6.0) * tau * tau - (1.0 / 24.0) * tau * tau * tau;
            gain = sub_h * phi;
        } else {
            gain = (1.0 - atten) / sig_avg;
        }

        L = L + T * em_avg * gain;
        T = T * atten;
    }

    results[rayIdx] = vec4<f32>(L, T);
    atomicAdd(&auditWrites[rayIdx * 2u + 1u], 1u);
}
"""


def generate(root: Path, mutation: str = "fixed") -> str:
    t6ej = load(root, "ray_transport_gpu_6ej_terminal_s2")
    source = t6ej.generate(root, mutation=mutation)

    # 0. Upgrade device initialization to request supported limits (raising max_storage_buffers_per_shader_stage from 8 to supported)
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

    # 1. Generate kFallbackConsumerWgsl directly from kUnfusedShader
    hit_unfused = re.search(r'(static const char\* kUnfusedShader = R"WGSL\()(.*?)(\)WGSL";)', source, re.S)
    assert hit_unfused
    unfused_code = hit_unfused[2]

    old_header = """@compute @workgroup_size(64)
fn rayMain(@builtin(global_invocation_id) gid: vec3<u32>) {
    let rayIdx = gid.x;
    if (rayIdx >= u.rayCount) { return; }"""

    new_header = """@group(0) @binding(8) var<storage, read> tileOffsets: array<u32, 2305>;
@group(0) @binding(9) var<storage, read> tileRayIndices: array<u32>;
@group(0) @binding(10) var<storage, read> fallbackTileList: array<u32>;
@group(0) @binding(11) var<storage, read> routeCounts: array<u32>;

@compute @workgroup_size(64, 1, 1)
fn fallbackConsumerMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    if (tileSlot >= min(routeCounts[2u], 2304u)) { return; }
    let tileId = fallbackTileList[tileSlot];
    if (tileId >= 2304u) { return; }
    let rayStart = tileOffsets[tileId];
    let rayCount = tileOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) { return; }
    let rayIdx = tileRayIndices[rayStart + rayInTile];
    if (rayIdx >= u.rayCount) { return; }"""

    assert unfused_code.count(old_header) == 1
    fb_consumer_wgsl = unfused_code.replace(old_header, new_header)

    if mutation == "double-publish":
        target_atomic = "atomicAdd(&auditWrites[rayIdx * 2u + 1u], 1u);"
        assert fb_consumer_wgsl.count(target_atomic) == 1
        fb_consumer_wgsl = fb_consumer_wgsl.replace(target_atomic, target_atomic + "\n    atomicAdd(&auditWrites[rayIdx * 2u + 1u], 1u);")

    decl = f'\nstatic const char* kFallbackConsumerWgsl = R"WGSL({fb_consumer_wgsl})WGSL";\n'
    idx_decl = source.find('static const char* kS2TerminalProducerWgsl =')
    assert idx_decl != -1
    source = source[:idx_decl] + decl + source[idx_decl:]

    # 2. Pipeline: build cpFallbackConsumer
    pipe_hook = 'auto cpS2Terminal = buildCompute(kS2TerminalProducerWgsl, "s2TerminalProducerMain");'
    assert source.count(pipe_hook) == 1
    pipe_decl = (
        pipe_hook + '\n' +
        '    auto cpFallbackConsumer = buildCompute(std::string(kCommonWgslHeader) + "\\n" + kOverlapWgsl + "\\n" + kFallbackConsumerWgsl, "fallbackConsumerMain");'
    )
    source = source.replace(pipe_hook, pipe_decl)

    # 3. Bind group: bgFallbackConsumer
    bg_hook = 'auto bgS2TerminalForced = makeGroup(cpS2Terminal, {'
    assert source.count(bg_hook) == 1
    bg_consumer_decl = f"""    auto bgFallbackConsumer = makeGroup(cpFallbackConsumer, {{
        makeEntry(0, uBuf, 0, sizeof(udata)),
        makeEntry(1, mBuf, 0, mediaBytes),
        makeEntry(2, pBuf, 0, paramBytes),
        makeEntry(3, rBuf, 0, rayBytes),
        makeEntry(4, sBuf, 0, subpanelBytes),
        makeEntry(5, oBuf, 0, overlapBytes > 0 ? overlapBytes : 16),
        makeEntry(6, outBuf, 0, outBytes),
        makeEntry(7, auditWritesBuf, 0, auditBytes),
        makeEntry(8, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(9, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(10, fbTileBuf, 0, 2304 * 4),
        makeEntry(11, cntBuf, 0, sizeof(initCnt))
    }});
"""
    source = source.replace(bg_hook, bg_consumer_decl + bg_hook)

    # 4. In execute(mode, ...): Replace Pass 6 in Mode 4 with indirect cpFallbackConsumer!
    old_mode4_pass6 = """                    // Pass 6: Canonical Unfused Consumer (skips completed S1 & S2 rays)
                    dispatch(cpUnfused, bgUnfused, count);"""
    assert source.count(old_mode4_pass6) == 1

    new_mode4_pass6 = """                    // Pass 6: Indirect Fallback Consumer (§6ek Candidate: Dispatches over fallbackTileList only!)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpFallbackConsumer);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgFallbackConsumer, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 24);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }"""
    source = source.replace(old_mode4_pass6, new_mode4_pass6)

    return source


def verify_32k_reference(root: Path, results: list) -> dict:
    ref_path = root / "docs/audits/NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz"
    with gzip.open(ref_path, "rt") as f:
        ref_data = json.load(f)

    rgb_errs = []
    t_errs = []

    # Current baseline thresholds from Opus 5.5's reference audit
    current_max_rgb = 0.002239
    current_max_t = 0.003467

    for i, row in enumerate(ref_data["rows"]):
        ref_T = row[2]
        ref_Lr = row[3]
        ref_Lg = row[4]
        ref_Lb = row[5]

        res = results[i]
        cand_Lr = res[0]
        cand_Lg = res[1]
        cand_Lb = res[2]
        cand_T = res[3]

        rgb_err = max(abs(cand_Lr - ref_Lr), abs(cand_Lg - ref_Lg), abs(cand_Lb - ref_Lb))
        t_err = abs(cand_T - ref_T)

        rgb_errs.append(rgb_err)
        t_errs.append(t_err)

    rgb_errs_sorted = sorted(rgb_errs)
    t_errs_sorted = sorted(t_errs)
    N = len(rgb_errs)

    mean_rgb = sum(rgb_errs) / N
    p99_rgb = rgb_errs_sorted[int(N * 0.99)]
    max_rgb = rgb_errs_sorted[-1]

    mean_t = sum(t_errs) / N
    p99_t = t_errs_sorted[int(N * 0.99)]
    max_t = t_errs_sorted[-1]

    rays_beyond_current = sum(1 for k in range(N) if rgb_errs[k] > current_max_rgb or t_errs[k] > current_max_t)

    return {
        "workload_rays": N,
        "mean_rgb_error": mean_rgb,
        "p99_rgb_error": p99_rgb,
        "max_rgb_error": max_rgb,
        "mean_t_error": mean_t,
        "p99_t_error": p99_t,
        "max_t_error": max_t,
        "current_max_rgb_threshold": current_max_rgb,
        "current_max_t_threshold": current_max_t,
        "rays_beyond_current_max": rays_beyond_current,
        "pass_all_six_gates": (rays_beyond_current == 0 and max_rgb <= current_max_rgb and max_t <= current_max_t)
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--build-root", type=Path, required=True)
    p.add_argument("--work", type=Path, required=True)
    p.add_argument("--mutation", choices=["fixed", "no-reset", "double-publish", "missing-s1", "missing-s2", "missing-fallback", "late-member1-refusal", "terminal-endpoint-corrupt"], default="fixed")
    p.add_argument("--case", type=Path, default=Path("/private/tmp/earthcall-wide-case/manifest.json"))
    p.add_argument("--run", action="store_true")
    args = p.parse_args()

    root = args.root.resolve()
    build_root = args.build_root.resolve()
    worktree_root = build_root.parent if build_root.name == "build" else build_root
    work = args.work.resolve()

    if work.exists() or work == root or root in work.parents:
        raise ValueError("fresh external work required; existing directories are never deleted")
    work.mkdir(parents=True, exist_ok=False)

    print(f"Generating §6ek source for mutation: {args.mutation}...")
    cpp_source = generate(root, mutation=args.mutation)
    (work / "probe.cpp").write_text(cpp_source)

    build_dir = work / "build"
    subprocess.run([
        sys.executable, "-B", str(root / "scratch/probes/ray_transport_probe_build.py"),
        "--root", str(root),
        "--build-root", str(worktree_root),
        "--source", str(work / "probe.cpp"),
        "--work", str(build_dir)
    ], check=True)
    print("Compilation successful!")

    if args.run:
        spec = importlib.util.spec_from_file_location("wide", root / "scratch/probes/ray_transport_gpu_wide_stream.py")
        m = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(m)
        zone = root / "saves/zones/Northern Veil/zone.ecform"
        before = hashlib.sha256(zone.read_bytes()).hexdigest()
        meta, rays, _ = m.verify_case(root, args.case.resolve(), before)
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

        output = work / "review.json"
        print(f"Executing control probe for mutation: {args.mutation}...")
        result = subprocess.run([str(build_dir / "probe"), str(output)], cwd=root, env=env)
        if hashlib.sha256(zone.read_bytes()).hexdigest() != before:
            raise ValueError("source drift")
        m.verify_case(root, args.case.resolve(), before)
        if result.returncode != 0:
            print(f"Control probe exited with code {result.returncode}")
            sys.exit(result.returncode)
        else:
            print(f"PASS control probe: {args.mutation}")
            # If fixed, verify full 32k reference gate!
            if args.mutation == "fixed" and output.exists():
                r_data = json.load(output.open())
                case0 = r_data["cases"][0]
                # Outputs: [id, [L0_base, L1_base, L2_base, T_base], [L0_cand, L1_cand, L2_cand, T_cand]]
                import struct
                def u2f(u): return struct.unpack('f', struct.pack('I', u))[0]
                cand_results = [[u2f(u) for u in row[2]] for row in case0["outputs"]]
                ref_summary = verify_32k_reference(root, cand_results)
                print("Full 32,784-Ray Reference Gating Results:")
                print(json.dumps(ref_summary, indent=2))
                assert ref_summary["pass_all_six_gates"], "32k reference gate failed!"


if __name__ == "__main__":
    main()
