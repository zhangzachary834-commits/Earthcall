# GPU Attribution Sun — native host-separation continuation (2026-10-22)

**Scope and custody:** measurement-only branch `sol/gpu-attribution-measurement-20261021` descended from canonical `805d6c6e27fc`; canonical, historical draft PR #482, and predecessor SourceRho measurement branch are untouched. Do not PR/merge, broaden production semantics, or disable this Sun merely because of a blocked write.

## What is established

- SourceRho's fixture-bounded **5,624,857** eliminated GPU shadow steps did not produce a reliably replicable material completed-frame gain across three native same-code runs (28.57%, 0.32%, 0.0734%). The economic rejection stands. Missing GPU timestamps are **unknown**, not zero.
- Four native macOS Release runs at exact benchmark code `805d6c6e27fc` gave **80/80** pairs favoring optional SDF range traversal OFF, with **+15–17 ms** median paired ON penalties across two camera views. The accelerator is already default OFF. It made 83,649 candidate calls to save 144 SDF samples (horizon) and 74,686 calls to save 157 (45°). This is a negative economics finding for the fixture, not a cross-pass bottleneck ranking.
- The existing benchmark had paired completed-frame wall timings but conflated host recording/submission and completion polling. The renderer's GPU timestamp readback is asynchronous and can refer to an earlier frame.

## New measurement work

- Commit `c7db8a68327ce51fb8dd3004716209ab16ba9dbf` updated **only** `tests/singularity/webgpu_sdf_range_perf_test.cpp`: log adapter/backend and advertised timestamp features; retain timestamp-free correctness device; split host record/submit from host device-poll; print warmup, AB/BA, paired MAD and host deltas; log existing SDF compile/cache/upload counters.
- Analysis document: `docs/analysis/gpu_attribution_native_host_separation_checkpoint_2026-10-22.md` (commit `8d8a9a8e6b1a50fad1fc93ca0db53c9d4f795135`).
- The initial exact-code workflow [39743334732](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39743334732) was **cancelled** following the subsequent documentation commit, not a test pass or failure. **Native compilation, actual adapter identity, and new timings remain UNVERIFIED.** Check the latest exact-head CI after this handoff commit, and inspect the authored-Perlin job and any bounded failure steps. Do not claim success from queued/cancelled jobs.

## One next bounded experiment

Validate the latest exact-head native macOS authored-Perlin benchmark and its existing parity/CPU-SDF gates. If it passes, use the new per-pair host attribution to decide whether the observed range penalty is dominated by host record/submit or host completion waiting; **host polling still cannot be labeled GPU execution time**. Confirm adapter timestamp feature advertisement. If both timestamp features are present, design a separate headless timestamp-enabled device experiment with frame-origin IDs and matched SDF/volume/combined passes, preserving correctness on the timestamp-free device. If unavailable, record the attribution limit rather than a fictional GPU duration.

No production changes, no new PR, no merge, no shipping claim. Keep the Sun's recurring schedule active until the native evidence and a final evidence-backed disposition are durably recorded. The SourceRho shared CPU-SDF failure remains separate/unclassified.

---

## Final native-evidence handoff (2026-10-23)

The final evidence and bounded disposition have been committed to `docs/analysis/gpu_attribution_native_host_separation_checkpoint_2026-10-22.md` at commit `5a043bef894b2d30c3ba7962570c2b761ac7ebab`. The instrumented native macOS run [#39743465001](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39743465001) passed the authored-Perlin benchmark: Apple Paravirtual device/backend 5, no advertised GPU timestamp features, no valid GPU execution samples. Paired ON-minus-OFF wall penalties were +16.034/+16.774 ms (horizon/45°), with CPU record/submit differences only +0.011/+0.028 ms and host polling differences +16.083/+16.848 ms. Polling is not GPU duration.

Two later canonical replications, including [#39835987441](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39835987441) and [#39885392474](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39885392474), extended the tested optional SDF range-traversal result to **140/140 pairs favoring OFF** across seven native runs. The accelerator is already default OFF. The negative economics are fixture-specific and do not rank ordinary renderer passes. The SourceRho economic rejection remains unchanged.

**Final recommendation:** do not promote optional traversal or SourceRho on current evidence. Commission **one isolated physical-Metal pass-attribution experiment** only when a verified adapter and valid timestamps or trustworthy external profiling are available, with originating-frame IDs, SDF-only/volume-only/combined passes, warmups, AB/BA pairs, dispersion, and byte-exact correctness gates. Physical M5 microkernel evidence in `docs/audits/NORTHERN_VEIL_METAL_DIAGNOSTIC_2026-10-21.md` is completion-fenced, not GPU timestamp evidence. If hardware is unavailable, stop at insufficient evidence; do not spin on paravirtual repeats. The shared CPU-SDF failure remains separate.

This closes the bounded GPU Attribution measurement and handoff scope. No production changes, PR or merge were authorized.
