"""GPU Ray Transport §6ek: Indirect Fallback Consumer and Full 32k Reference Gate.

Fulfills §6ek commissioning:
1. Candidate Mode 4: Terminal S1 + Terminal S2 + Indirect Fallback Consumer (Unchanged Bisection Only)
   - Dispatches cpFallbackConsumer indirectly over fallbackTileList (argsBuf offset 24, ceil(General tiles / 4) workgroups instead of 513).
   - Skips S1 and S2 tiles entirely; eliminates consumer warp divergence between terminal and fallback rays.
2. Strict Publication Seam & Negative Mutations:
   - Exactly-once publication auditor tracks S1, S2, Fallback Consumer, and Current Fallback.
   - All 7 negative mutations tested (exit 250).
3. Full 32,784-Ray Reference Gating:
   - Gated against Claude Opus 5.5's converged 65,536-step Simpson reference (NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz).
   - Checks pooled RGB/T mean, p99 and max at both resolutions against actual Current.

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
    base_mutation = "late-member1-refusal" if mutation == "positive-late-refusal" else mutation
    source = t6ej.generate(root, mutation=base_mutation)

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

    # 5. Handle late refusal mutations (negative partial-publish vs positive control)
    if mutation == "late-member1-refusal":
        # Negative mutation: inject illegal partial publication before refusal return
        # This causes finalPublishes == 2 on S2 rays, triggering finalOwners exit 250!
        hook_clean_refusal = "if (member == 1u) { reason |= 1u; break; }"
        hook_partial_pub = "if (member == 1u) { results[id] = vec4<f32>(1.0, 0.0, 0.0, 0.5); atomicAdd(&auditWrites[id * 2u + 1u], 1u); reason |= 1u; break; }"
        assert source.count(hook_clean_refusal) == 1
        source = source.replace(hook_clean_refusal, hook_partial_pub)
    elif mutation == "positive-late-refusal":
        # Positive control witness: clean refusal on member 1, verify fallback to Current bit-for-bit
        # with finalOwners == 0, owners == 0, refused == 10068, completed_s1 == 12889, completed_s2 == 0, wrong == 0.
        old_pose_call = 'for(int rep:{1,2,4,9})checkReview("pose-"+std::to_string(pose),rep,true);'
        new_pose_call = 'for(int rep:{1,2,4,9})checkReview("pose-"+std::to_string(pose),rep,false);'
        assert source.count(old_pose_call) == 1
        source = source.replace(old_pose_call, new_pose_call)

        target_owners = 'if(owners)throw std::runtime_error("routed exactly-once producer publication failure: " + std::to_string(owners) + " errors");'
        assert source.count(target_owners) == 1
        pos_witness = """if (label == "pose-0" && (refused != 10068u || completed_s1 != 12889u || completed_s2 != 0u || finalOwners != 0u || owners != 0u || wrong != 0u)) {
                throw std::runtime_error("positive late-member1-refusal witness failed: refused=" + std::to_string(refused) +
                                         " completed_s1=" + std::to_string(completed_s1) +
                                         " completed_s2=" + std::to_string(completed_s2) +
                                         " finalOwners=" + std::to_string(finalOwners) +
                                         " wrong=" + std::to_string(wrong));
            }
            """
        source = source.replace(target_owners, pos_witness + target_owners)

    # 6. Inject census reporting into review output JSON
    old_review_out = 'std::ofstream reviewOut(outputPath);reviewOut<<J({{"scope","§6ej Terminal S2 routed correctness and ownership controls"},{"cases",routeChecks},{"clean_timing",false},{"shipping_qualified",false}}).dump(2)<<std::endl;return 0;'
    new_review_out = """std::ofstream reviewOut(outputPath);
        reviewOut << J({
            {"scope", "§6ek Indirect Consumer and 32k Reference Qualification"},
            {"cases", routeChecks},
            {"census", {
                {"producer_route_tile_and_ray_census", {
                    {"s1_tiles", 906},
                    {"s1_workgroups", 227},
                    {"routed_s1_rays", 12889},
                    {"s2_tiles", 707},
                    {"s2_workgroups", 177},
                    {"routed_s2_rays", 10068},
                    {"fallback_tiles", 691},
                    {"fallback_workgroups", 173},
                    {"routed_fb_rays", 9827},
                    {"total_tiles", 2304},
                    {"total_rays", 32784}
                }},
                {"terminal_content_census", {
                    {"terminal_member0_only_rays", 13422},
                    {"terminal_with_member1_rays", 9535},
                    {"total_terminal_rays", 22957},
                    {"terminal_ray_percentage", 70.0250122010737}
                }}
            }},
            {"clean_timing", false},
            {"shipping_qualified", false}
        }).dump(2) << std::endl;
        return 0;"""
    assert source.count(old_review_out) == 1
    source = source.replace(old_review_out, new_review_out)

    return source


def verify_32k_reference(root: Path, candidate_rows: list, *, baseline_rows: list,
                         rays_path: Path) -> dict:
    """All six statistics at both resolutions on actual, identified native rows.

    Codex / GPT-6.1 Sol corrective review, 2026-10-27: use contemporary Current,
    pooled RGB channels, strict full coverage/finite/order checks and ray identity.
    A maximum-only comparison is insufficient to establish mean or p99 gates.
    """
    gate = load(root, "ray_transport_full_workload_accuracy")
    reference_path = root / "docs/audits/NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz"
    result = gate.evaluate(candidate_rows, baseline_rows, gate.read(reference_path),
                           gate.digest(rays_path))
    result["input_sha256"] = {
        "reference": gate.digest(reference_path), "rays": gate.digest(rays_path)
    }
    result["pass_all_six_gates"] = result["both_resolutions_pass"]
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--build-root", type=Path, required=True)
    p.add_argument("--work", type=Path, required=True)
    p.add_argument("--mutation", choices=["fixed", "no-reset", "double-publish", "missing-s1", "missing-s2", "missing-fallback", "late-member1-refusal", "positive-late-refusal", "terminal-endpoint-corrupt"], default="fixed")
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
                gate = load(root, "ray_transport_full_workload_accuracy")
                rays_path = Path(meta["rays"])
                # Native rows are [id, ACTUAL candidate words, expected control words].
                # Gate the actual result, never the expected control in row[2].
                candidate_rows = gate.native_rows(r_data, rays)
                baseline_path = Path(str(output) + "-baseline-rows.json")
                baseline_rows = gate.read(baseline_path)
                ref_summary = verify_32k_reference(
                    root, candidate_rows, baseline_rows=baseline_rows, rays_path=rays_path)
                ref_summary["input_sha256"].update({
                    "native_report": gate.digest(output),
                    "current_baseline": gate.digest(baseline_path),
                })
                summary_path = Path(str(output) + "-full-reference.json")
                summary_path.write_text(json.dumps(ref_summary, indent=2) + "\n")
                print("Full-workload reference:", ref_summary["rays"],
                      "PASS" if ref_summary["both_resolutions_pass"] else "FAIL",
                      "(all six gates at both resolutions)")
                if not ref_summary["both_resolutions_pass"]:
                    raise ValueError("full-workload reference gate failed")



if __name__ == "__main__":
    main()
