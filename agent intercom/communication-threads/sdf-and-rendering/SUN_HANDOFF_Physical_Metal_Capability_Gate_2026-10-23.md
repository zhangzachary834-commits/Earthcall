# Physical Metal Sun — bounded capability gate and profiling handoff (2026-10-23)

**Scope:** read-only physical-GPU feasibility investigation plus **documentation-only** profiling protocol on isolated branch `sol/physical-metal-profiling-protocol-20261023`. Canonical fork point `df0fc26cafe3af674e78bcdfc1ec9645b256b99a`. No production renderer, shader, tests or predecessor branch changed; no PR/merge.

## What the successor must know

1. **Physical Apple Silicon access is not currently verified.** The Sun's command environment is Linux x86_64 without Metal/Xcode. Canonical `.github/workflows/earthcall-ci.yml` specifies hosted `macos-14` runners, not a verified physical GPU. The prior exact-head macOS run [39743465001](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39743465001) identified **Apple Paravirtual device/backend 5** and advertised **neither** required timestamp feature.
2. A separate historical [Northern Veil M5 diagnostic](https://github.com/zhangzachary834-commits/Earthcall/blob/sync-from-earthcall-main/docs/audits/NORTHERN_VEIL_METAL_DIAGNOSTIC_2026-10-21.md) reports a physical Apple Silicon M5 microkernel test, but provides **no present access** to that host and **no independently valid GPU execution timestamps**. Its ~1.27 ms fused/unfused measurements are completion-fenced; `wgpuDevicePoll` is not GPU time.
3. The finalized GPU Attribution predecessor `sol/gpu-attribution-measurement-20261021` (head `41f15ce4dcc92364b1814ebc234864e917b40ece`) proved optional SDF range traversal **economically negative for the Perlin fixture** in **140/140 paired native comparisons**. It is already default OFF. CPU record/submit differential was tiny, host poll differential large; this **does not rank ordinary rendering GPU passes**.
4. SourceRho measurement predecessor `sol/sourcerho-measurement-20261016` (head `405809c7a56ca79a9f729140aadf29e325669b4c`) proved **5,624,857** eliminated shadow-loop steps in a fixture but failed to reproduce a material completed-frame gain across three independent native runs. Economic rejection stands. Missing timestamps != zero GPU cost.
5. Canonical exact-head `df0fc26cafe3` check-runs: SDF authored-Perlin and range-proxy **PASS**; Focused CPU and Slow Adapter **FAIL**, separate/unclassified.

## Durable protocol

Read **`docs/analysis/physical_metal_pass_attribution_protocol_2026-10-23.md`** on this branch (committed at `98e59465c427f3f12aa59ebe8be8cdc231580567`). It includes a capability matrix, physical-Mac environment manifest commands, native WebGPU feature and optional external Metal trace gates, timestamp-free correctness witnesses, three matched SDF-only/volume-only/combined benchmark variants, asynchronous frame-ID ownership, paired warmups/repeats/MAD, and hard STOP conditions.

**One next justified experiment:** after independently obtaining a physical Apple GPU session, run the protocol's Gate 0/1 capability test; only then consider isolated pass-level profiling. If neither native timestamps nor a trustworthy GPU trace is available, report **INSUFFICIENT GPU EXECUTION EVIDENCE** and stop. Do not launch additional paravirtual benchmarks, guess a bottleneck ranking, or promote production optimizations.

Keep `rho_source != V_transport != D_medium`, byte-exact reference and diagnostic/ordinary parity, visibility ON/OFF, independent live source, hostile lifetime/repair/fail-open, ambient/receiver/V1–V4 and identity/revision/generation/channel gates intact. No PR, merge, production changes, or force push are authorized.

**Disposition:** this Sun's bounded investigation is complete when this handoff and the protocol are committed and ancestry verified. A future physical-Metal measurement requires verified hardware access; it is a separate mission, not a reason to spin this recurring Sun.
