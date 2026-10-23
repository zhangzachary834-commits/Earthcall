# Physical Metal Sun — capability verdict and executable profiling protocol

**Checkpoint:** 2026-10-23. **Scope:** bounded feasibility and reproducibility protocol, **not** a completed GPU execution measurement or a production optimization. **Branch:** `sol/physical-metal-profiling-protocol-20261023`, forked from canonical `df0fc26cafe3af674e78bcdfc1ec9645b256b99a`. No production renderer or benchmark semantics were changed in this checkpoint.

## Evidence and current access boundary

Read live at this checkpoint:
- Canonical `sync-from-earthcall-main` = `df0fc26cafe3af674e78bcdfc1ec9645b256b99a`; exact-head GitHub check-runs show authored-Perlin macOS Release and SDF range-proxy **PASS**, Focused CPU and Slow Adapter **FAIL**. The latter two remain separate/unclassified; no causal assignment to GPU attribution.
- Final GPU Attribution branch `sol/gpu-attribution-measurement-20261021` = `41f15ce4dcc92364b1814ebc234864e917b40ece`. See `docs/analysis/gpu_attribution_native_host_separation_checkpoint_2026-10-22.md` and `agent intercom/communication-threads/sdf-and-rendering/SUN_UPDATE_GPU_Attribution_Native_Host_Separation_2026-10-22.md` on that branch.
- SourceRho predecessor `sol/sourcerho-measurement-20261016` = `405809c7a56ca79a9f729140aadf29e325669b4c`. Three native same-code replications removed **5,624,857** shadow SDF steps but did not reproduce material completed-frame gains; economic rejection stands.
- Canonical `docs/audits/NORTHERN_VEIL_METAL_DIAGNOSTIC_2026-10-21.md` and `.json`, plus `scratch/probes/ray_transport_metal_diagnostic.py`: a prior Antigravity experiment **reported Apple Silicon M5**; 2,304-ray fused/unfused consumer times ~1.2727/1.2718 ms are **completion-fenced**. They are NOT independently measured GPU execution times, and this report does not confer access to that machine.
- Exact-head CI workflow `.github/workflows/earthcall-ci.yml` uses `runs-on: macos-14` for the relevant macOS jobs; no self-hosted physical-Metal runner is declared in that workflow.
- The prior instrumented GitHub-hosted macOS Release run [39743465001](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39743465001) identified **Apple Paravirtual device**, backend 5, neither required timestamp feature advertised. Its completion-fenced ON-minus-OFF traversal penalties were +16.034/+16.774 ms; CPU record/submit differences +0.011/+0.028 ms, host poll differences +16.083/+16.848 ms. **Host poll != GPU execution time.**
- Seven native replications, **140/140 paired comparisons**, disfavor optional SDF range traversal in the tested Perlin fixture. It is already default OFF; this does not rank normal Earthcall rendering passes.
- The agent's available command execution environment at this checkpoint is **Linux x86_64**, without `xcrun` or Metal.framework. No connected tool demonstrated control of the prior physical M5 machine, an Apple Silicon shell, or a physical Metal CI runner.

**Access verdict: NOT VERIFIED / NOT AVAILABLE IN THIS EXECUTION CONTEXT.** The fact that a prior probe ran on physical M5 hardware does not imply present ability to execute there. **GPU timestamp support on physical M5: UNKNOWN.** No physical GPU execution measurements were produced by this Sun.

## Capability matrix

| Environment | Physical Apple GPU verified | Timestamp-query support verified | Valid per-pass GPU execution times | Suitable for attribution now |
| --- | --- | --- | --- | --- |
| GitHub-hosted `macos-14` exact-head SDF run | No; logged Apple Paravirtual | No; `TimestampQuery=0`, inside-encoder=0 | No | No: host-completion economics only |
| Antigravity M5 diagnostic (historical report) | Report says Apple Silicon M5; not independently re-accessed | Unknown | No; completion-fenced ~1.27 ms | No: microkernel fidelity and wall boundary only |
| This Sun's execution shell | No; Linux x86_64 | Not applicable | No | No |
| Future directly controlled physical Apple Silicon machine | Pending live adapter/device verification | Pending feature-query + calibration, or trustworthy Metal trace | Pending | **Conditional only** |

## ONE bounded next experiment: physical-Metal isolated pass attribution

### Gate 0 — acquire and identify a real device (do not proceed if unavailable)

Obtain an interactive session on a physical Apple Silicon Mac or an independently verified physical-Metal runner. **Do not substitute GitHub-hosted macOS virtualization.** On that machine, record in a durable session manifest:

```sh
date -u
uname -a
sw_vers
sysctl -n machdep.cpu.brand_string
system_profiler SPDisplaysDataType
xcodebuild -version
xcrun xctrace list templates
git rev-parse HEAD
git status --short
```

Record physical host ownership/access provenance, Apple GPU model, macOS/Xcode/Metal and wgpu-native versions, selected adapter name and `backendType`, driver details when available, display/headless mode, resolution, power source, thermal/clock context if measurable, and SHA of each shader, fixture, harness, and compiled binary. `xcrun xctrace list templates` only discovers available Instruments templates; it **does not prove** a working GPU counter trace. Require an actual trace with identifiable GPU pass intervals before claiming profiling capability.

### Gate 1 — capability probe without touching correctness device

Reuse `src/Singularity/Screen/WebGPU/WgpuDevice.hpp` as a guide: log `wgpuAdapterGetInfo`, `wgpuAdapterHasFeature(...TimestampQuery)`, and `WGPUNativeFeature_TimestampQueryInsideEncoders`. Request timestamp features **only on a separate headless profiling device**. Both advertised features and successful device creation are required for the existing `WgpuDevice::init(true)` path; query-set creation, timestamp-period validity, successful resolve/map, positive ordered deltas, and originating-frame association are further independent requirements.

If either feature is missing, attempt a **trustworthy external Metal Instruments GPU trace** with actual per-encoder/per-pass GPU execution intervals. A CPU completion fence or GPU utilization percentage alone is not a replacement. If neither works, **STOP**, mark GPU execution durations **UNAVAILABLE**, and do not rank GPU passes.

### Gate 2 — preserve independent exactness reference

Run the existing timestamp-**free** offscreen correctness witnesses at the exact tested code and fixture, and store output hashes/byte comparisons. `WgpuDevice::init(false)` is the reference path; do not enable timestamp instrumentation in the windowed renderer (historical visual-coherence regression). Existing witnesses must remain passing: framebuffer byte-exactness; diagnostic vs ordinary; visibility ON/OFF boundary; independent live-source shadow work; hostile lifetime/fail-open/repair; ambient/receiver/V1–V4; stable identity/revision/generation/channel gates; and `rho_source != V_transport != D_medium`. Distinguish preexisting canonical failures from experiment regressions. **Any new exactness failure stops the experiment before economic analysis.**

### Gate 3 — isolated, matched benchmark-only pass variants

If Gates 0–2 pass, implement **only in a dedicated headless benchmark/test** (separate authorization for production changes) three immutable variants at identical resolution, camera, scene, shader parameters and source/medium state:

1. **SDF-only:** opaque/SDF work, with its own measured GPU interval; separate shadow visibility only if the same shader/inputs can be instrumented without changing semantics.
2. **Volume-only:** the exact production volumetric dispatch over a precomputed **frozen** depth/scene input, preserving density, extinction, scattering, chroma and overlap; isolate ray-domain transport only if the active branch's executable path is verified.
3. **Combined:** the same SDF + volume sequence and composition as the real frame, with GPU timestamp pairs bracketing each pass and the total. Do not assume total equals sum: barriers, overlap, transitions, residency and synchronization can differ.

Do not present artificial pass subtraction as an exact independent cost when pipelines interact. Log pipeline compilation/cache misses, uploads, resource allocations and residency/evictions if actually observable; otherwise mark **UNKNOWN**. Record CPU scene/AST producer work separately from GPU consumer work. No new domain classes, enums, or authored ontology semantics.

### Gate 4 — valid timing ownership and repeatability

For each sample log `{session_id, device, branch_sha, fixture_hash, frame_id, variant, pass, query_slot, gpu_start_tick, gpu_end_tick, timestamp_period_ns, gpu_duration_ms, cpu_record_submit_ms, host_poll_ms, upload_bytes, compile_count, cache_misses, output_hash}`. Associate asynchronous readback with **originating frame ID**, not the frame that receives the callback. Inspect `WebGpuRenderer.cpp` `beginGpuTimestampFrame`, `endGpuTimestampFrame`, `collectGpuTimestampResults` for the existing four-slot ring and late-sample semantics, but do not silently reuse it as a validated pass profiler.

Warm pipelines and buffers before sampling (at least six warmup draws per variant, extend until stable). Alternate AB/BA order across **at least 12 matched pairs per camera per session**, repeat **three sessions** under documented power/thermal conditions, and report paired median, MAD, p10/p90, favorable pair count, order-stratified medians, and session-to-session dispersion. Ensure every GPU query has a successful map and positive finite period-corrected delta. Treat missing/overwritten/stale samples as **missing**, not zero. Record CPU record/submit and host completion wait independently; neither is GPU execution.

### Gate 5 — stop and rank only what was directly measured

Pass-level GPU ranking is allowed **only** after Gates 0–4 succeed and byte-exact reference parity holds. Show absolute per-pass GPU durations and percentages only for measured intervals of the same representative workload; give confidence/dispersion and identify uncovered GPU work. Recommend **ONE** bounded optimization experiment targeting the largest reproducible pass opportunity, with predicted ceiling clearly labeled a hypothesis. If timestamps/traces or exactness fail, report **INSUFFICIENT GPU EXECUTION EVIDENCE** and no shipping recommendation.

## Existing artifacts and reproduction pointers

- Benchmark/host separation: `tests/singularity/webgpu_sdf_range_perf_test.cpp` on the finalized GPU Attribution branch (not canonical). It is a completed-frame economics benchmark, **not** the proposed per-pass profiler.
- Physical-M5 consumer microkernel probe: `scratch/probes/ray_transport_metal_diagnostic.py`; its generated harness currently fences via `wgpuDevicePoll`, so do not relabel its numbers GPU kernel time.
- Existing optional timestamp infrastructure: `src/Singularity/Screen/WebGPU/WgpuDevice.hpp` and `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`. `WgpuDevice::init(true)` requests both features if advertised; renderer timestamp readback is asynchronous, and visual correctness must remain timestamp-free.
- SourceRho and GPU Attribution predecessor reports/handoffs linked above. Shared CPU-SDF CI failure is separate/unclassified.

## Disposition

**Scientific result of this Sun:** actual physical GPU access and trustworthy GPU timestamp/trace capability are **not demonstrated**; ordinary SDF, shadow, ray-domain and volumetric GPU execution costs remain **unranked**. This runbook is a reproducible gated experiment proposal, not a completed profiling run. The one next justified experiment is **a single physical-Metal pass-attribution session after Gate 0 access is established**. Do not run indefinite virtual CI replications, modify canonical/production, merge, create PR, or force-push.
