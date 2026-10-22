# GPU Attribution Sun — native host-separation instrumentation checkpoint

**Date:** 2026-10-22 UTC. **Branch:** `sol/gpu-attribution-measurement-20261021`, descended from canonical `805d6c6e27fc32d943f927c0696c0671f2467d42`. This is a benchmark-only measurement successor, **not** a production optimization, PR, or shipping recommendation.

## Predecessor constraints

SourceRho's three native macOS measurements of the same benchmark source blob eliminated **5,624,857** GPU-executed shadow-loop SDF steps for the proved-zero source while leaving **6,778,135** independent live-source steps unchanged. Their independent-arm wall reductions were **28.57%, 0.32%, and 0.0734%**; the latter two failed to replicate a material gain. GPU timestamp samples were absent. The predecessor's economic rejection remains binding; zero samples do **not** mean zero GPU time. See `docs/analysis/source_rho_measurement_successor_native_paired_checkpoint_2026-10-17.md` and its Agent Intercom handoff.

## Existing native SDF traversal evidence

Four independent exact-code macOS Release runs at canonical ancestor `805d6c6e27fc`, each with 10 alternating OFF/ON pairs per view at 2880×1800, reported median paired **ON-minus-OFF completed-frame wall penalties**:

| Run | Horizon | 45° camera |
| --- | ---: | ---: |
| [39649129973](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39649129973) | +15.49 ms | +16.15 ms |
| [39649160014](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39649160014) | +15.30 ms | +16.65 ms |
| [39653978659](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39653978659) | +15.97 ms | +16.53 ms |
| [39665882394](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39665882394) | +15.535708 ms | +16.049750 ms |

All 80 pairs favored OFF. In the fourth run, horizon runtime diagnostics counted **83,649** candidate calls to save **144** SDF sample steps; 45° counted **74,686** calls to save **157** steps. This is compelling negative *completed-frame economics for optional range traversal in the tested Perlin fixture*, not an attribution of GPU execution duration or a claim that ordinary default-OFF Earthcall rendering is slow because of traversal.

The verified native job used the `macos-14-arm64` runner image and passed the authored-Perlin A/B step. The **actual GPU adapter name/backend and timestamp feature availability were not logged** in that run. The renderer's optional GPU timestamp samples are asynchronous and can describe earlier frames; they cannot be paired with current host timings without explicit originating-frame association.

## Authorized instrumentation committed

Commit `c7db8a68327ce51fb8dd3004716209ab16ba9dbf` changes only `tests/singularity/webgpu_sdf_range_perf_test.cpp`. It preserves the two warmed persistent renderer arms, existing OFF/ON draw semantics, 6 alternating warmups per arm, 10 AB/BA pairs, completion fence, all existing parity witnesses and baseline output. It adds:

- Native adapter backend/name and advertised timestamp-query/inside-encoder capability; **does not request or enable** timestamps on the correctness device.
- Separate CPU host record/submit and `wgpuDevicePoll` host-wait durations, retaining the original full completion-fenced wall duration.
- Per-warmup observations, per-pair host components, median paired host deltas, paired wall MAD, and AB/BA order medians.
- Existing frame-stat SDF compile/cache hit/miss, SDF parameter upload, and uniform-write counts, without pretending they measure GPU residency.
- Explicit `gpu_execution_timestamps=UNAVAILABLE` and `poll_is_not_gpu_time=1` labels.

**Validation state:** at the time of this note, the new exact-code native workflow [39743334732](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/39743334732) was queued. No new timings, adapter identity, build pass, or timestamp capability result is claimed. The previously verified native runner's availability does not prove that the new instrumentation compiles or that its future job succeeds.

## Bounded decision and next experiment

The present evidence supports **rejecting optional SDF range traversal for the tested Perlin workload**. It does not support ranking normal SDF, volume transport, ray-domain work, shadow visibility, CPU submit, and synchronization in a representative ordinary frame.

**ONE next experiment:** obtain the exact-code native result for the new host-separation benchmark. Check compile and correctness gates first. Read the adapter/capability line, individual AB/BA pairs, warmups, host record/submit and poll medians, dispersion, and compiler/upload counts. If the adapter advertises both timestamp features, commission a **separate isolated headless timestamp-enabled device** with originating-frame IDs and paired pass durations; do not enable profiling in the visual correctness device. If unavailable, report an unresolved GPU-time attribution rather than inventing numbers.

Do not alter production renderer semantics, SourceRho or canonical branches; do not open a PR, merge, or infer zero GPU cost. Keep the recurring Sun active while exact-head validation and durable handoff remain pending. The shared CPU-SDF failure is separate and unclassified.
