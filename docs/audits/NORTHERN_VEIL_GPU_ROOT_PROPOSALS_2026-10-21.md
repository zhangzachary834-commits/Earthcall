# Northern Veil — GPU split-point proposal experiment

**Human origin:** Zachary Zhang commissioned the next pass after [§6cd's target-only producer](NORTHERN_VEIL_TARGET_ONLY_PRODUCER_2026-10-21.md): keep useful mathematical split points and reduce their production cost. This continues the [existing Intercom thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#sixth-one-sun-next-pass--device-split-point-proposals-in-progress).

**Starting HEAD:** `906a075f3`, branch `claude/ray-domain-transport-rung0`.

**Result:** the device now executes the bounded switch-root proposal prototype through existing production math lowering. **This fixed-bin scan is rejected:** the independent mathematical coverage check refuses 1,392/2,304 frozen rays, with 2,192 switch-gap sign changes outside all member proposals. All 20 preserved examples also change sign through the retained original operand DAG. Proposal generation alone costs **2.2481 / 2.2236 ms** at the same 18,432-ray distribution for which the verified current consumer costs **1.3396 / 1.3248 ms**. No new transport output, production producer, native omission certificate or FPS gain follows. The earlier branch-aware A* remains the stronger consumer candidate.

## Implemented bridge

The scratch driver reads the frozen fixture and its existing geometric envelope, then reuses OntoMath's `LineAlgebraicForm` switch identities and original `sourceA`/`sourceB` operands. For each switch it calls the existing backend `lowerVolumeSupportKillers` with a singleton selection, supplies the candidate threshold, and renames the generated function. Thus the expression arithmetic comes from the production emitter, not a new hand-written Northern Veil formula or a new renderer mathematics office.

The helper returns `max(-1,g)` for non-positive values under this payload. That preserves **sign**, not magnitude. The root search uses only signs and bisection; these values must not be used as Newton derivatives, distances, error bounds or unscaled function magnitudes. The header's unit scale and zero error allowance here are diagnostic inputs, not certified support data or a native execution enclosure. Unsupported operands or unresolved binding contexts refuse the experiment.

A GPU work item handles one concrete `(ray, switch)` pair. It clips that ray against the member box, scans eight equal bins and bisects detected sign changes, returning outward-padded proposal windows. A capacity overflow or nonfinite result sets an explicit flag. Eight windows are reserved per switch; no-hit has its own flag. The kernel writes the complete fixed record for every repeated invocation. Every replica's root count, flags, evaluation count and all windows are checked against its base invocation. These are each ray's own computed endpoints, not a neighboring ray's numeric answer.

Forty switch cases, 2,304 distinct frozen rays and repeat factor 8 give **737,280 work items** for 18,432 ray invocations. Ordinary timestamp queries stay disabled; the actual adapter query observes Apple M5 / Metal.

**Scope limits:** this is a scratch proposal producer. It does not sort/compact the complete executable transport schedule, consume these proposals in A*, execute a runtime fallback or implement production revision/device lifetime. The inherited SHA-locked fixture envelope extraction is still diagnostic-specific; no new general envelope/native skip admission was added. No production C++ or CMake/test target changed.

## Independent coverage rejects the proposal set

For every member/ray, the checker unions **all** its proposed switch windows, then asks the existing CPU interval office to classify **every switch** on every remaining gap. Unknown gaps are subdivided to the same relative resolution; unresolved or undefined gaps refuse. This is mathematical coverage analysis, not a proof of Metal f32 execution error.

The initial pilot checked each switch only against its own proposals and therefore refused all 2,304 rays. That was stricter than the actual shared partition: a boundary supplied by another switch also splits this switch. The final checker uses the member union, reducing refusals to 1,392 rays. The earlier all-ray count is not the final coverage result.

Final repeated results:

| Coverage quantity | Result |
|---|---:|
| Switch/ray jobs with a box hit | 87,200 |
| Raw proposed windows, before member union | 35,044 |
| Base-ray switch-function evaluations | 1,204,772 |
| GPU capacity/nonfinite refusals | 0 |
| CPU interval gap-query attempts | 1,298,276 |
| Unresolved switch-gap leaves | 11,522 |
| Switch/ray jobs refusing coverage | 2,051 |
| Rays with any refused member/switch | **1,392 / 2,304** |
| Strict endpoint sign changes outside the union | **2,192 switch-gap occurrences** |
| Preserved independent authored-DAG examples passing | **20 / 20** |

Occurrences can name the same geometric location through different switches; 2,192 is not a unique-root census. An interval refusal alone does not prove a root is missing. The strict point pairs supply separate concrete evidence: the original operand evaluated at both endpoints changes sign, and those endpoints lie in a gap disjoint from every member proposal. The reports preserve both line-evaluator values and independent `MathNode::evaluate` values at f32 property coordinates. This is stronger than comparing the root generator to a copy of its own arithmetic, but it is still not a device error enclosure.

Example: frozen ray 15, member 0, switch 7, interval approximately `[30.4084008,30.4124174]` has opposite operand signs. Another uncovered crossing occurs near `[33.7581934,33.7622099]` on the same switch/ray. A bin can contain more than one crossing while its endpoints have equal signs. Searching only detected endpoint sign changes cannot establish root completeness.

The final checker costs about six seconds on the CPU, outside native timing. It is a falsification tool, not a proposed per-frame host producer. Every unresolved case remains unadmitted; no renderer omission or runtime fallback execution is claimed.

## Native bill and matched ray-count controls

Two independent proposal processes and two independent consumer-control processes use the same captured directions/clips and repeat factor 8. Both measure batches of 1, 4 and 16 dispatches after warmup, with six ABBA blocks at each scale. Root ABBA compares root versus empty dispatch; consumer ABBA compares the verified current marcher versus precomputed branch-aware A*. **Root and consumer controls are separate processes**, not a jointly interleaved producer/consumer comparison. Treat the numbers as a diagnostic budget rejection, not a precise new end-to-end slowdown factor.

Each cell below is the 16-dispatch host batch mean divided by 16. These are completion-fenced host durations, not GPU timestamps; no empty-time subtraction is called GPU execution. Compilation, cold setup, CPU coverage verification, readback and uploads stay outside timed batches.

| Independent process | Root proposal only | Root empty control | Current marching consumer | Resident branch-aware A* control |
|---|---:|---:|---:|---:|
| 1 | 2.24806 ms | 0.11234 ms | 1.33965 ms | 0.40837 ms |
| 2 | 2.22361 ms | 0.10071 ms | 1.32484 ms | 0.39129 ms |

The current consumer controls reproduce stored native T exactly and RGB within 4.47e-8. Their original-AST, five closed-form GPU and six mean/p99/max accuracy gates pass. A* controls produce the same retained rows as the prior checkpoint. This pass adds no new transport-image result because the proposal coverage gate fails.

The fixed packet layout writes **3,200 bytes/ray**: 40 records ×80 bytes. Base packets occupy **7,372,800 bytes**; the full repeated diagnostic output occupies **58,982,400 bytes**. Parameters occupy 1,360 bytes. This is an intentionally simple complete-record representation whose padded output cost is included, not a lower bound on an optimized producer's representation. It repeats duplicate/tangent switch work and reserves many unused windows. Do not infer that all GPU root-finding is intrinsically this expensive or that register pressure is a measured cause.

Cold fixture decode, line restriction and operand lowering measure ~121 ms; pipeline compilation is separate. No lookup/amortization/cache invalidation, repair, dynamic full-frame upload, compact allocation/prefix scan, peak-memory or device-loss lifecycle is implemented. The consumer still receives its old precomputed schedule; it does **not** consume the GPU root output. Adding these two timings is not an observed end-to-end pipeline.

## Decision and concrete next boundary

**Reject the fixed-bin/per-switch scan. Do not increase bin count to hide the missing-root problem, turn proposal windows into certified omissions, or ship the CPU checker.** No current producer gate is closed.

The next bounded derivation belongs in OntoMath:

1. Canonicalize the zero sets of admitted switch expressions before device search. Solve affine/quadratic cases directly. For admitted `abs(g)=a`, preserve its domain conditions and derive the signed equations `g=a` and `g=-a`; deduplicate coincident roots/equations, including `a=0`. These are mathematical identities, not authored category labels.
2. For the remaining smooth functions, derive bounded monotone pieces/root coverage over a valid ray family, then instantiate each ray's own endpoints with a bounded numerical solve. Preserve uncertainty bands and refuse unsupported/stale/out-of-domain cases. Endpoint sampling alone is insufficient.
3. Screen should lower the admitted recipe and use a compact or fused representation. Count actual preparation, lookup, numeric solve, repair, upload, memory and complete producer-plus-consumer cost. Keep branch-aware quadrature split points and complete overlap transport.

Canonicalization must retain original authored-operand provenance and its execution-error enclosure. An exact-real zero-set identity alone does not certify a Metal f32 branch decision; possible execution shifts stay inside retained uncertainty bands or the existing fallback, as the earlier cancellation repairs require.

This recipe is **not implemented by this experiment**. It is the smaller, evidence-directed next target. No new broad framework or renderer-owned branch semantics are needed to explain the failure. The existing best consumer has measured headroom; proof search and padded generic switch work must be replaced, not made free by leaving them outside the frame clock.

Remaining shipping gates: affordable complete producer, full unique viewport with depth/raster/composition and total frame costs, broader pose/clock/source/overlap and production revision/device/fallback lifecycle, inhabited movement and Zach's acceptance. Current authored controls, clocks, save data, lighting, 96-sample policy and production renderer remain unchanged. No app launch, push or PR.

## Reproduce and preserved evidence

```sh
python3 scratch/probes/ray_transport_gpu_root_proposals.py \
  --root /path/to/ray-checkout --work /private/tmp/veil-roots-new \
  --repeat 8 --bins 8 --label series1
python3 scratch/probes/ray_transport_gpu_root_proposals.py \
  --root /path/to/ray-checkout --work /private/tmp/veil-roots-new \
  --run-only --repeat 8 --bins 8 --label series2
python3 scratch/probes/ray_transport_gpu_root_baseline.py \
  --root /path/to/ray-checkout --work /private/tmp/veil-root-control-new \
  --repeat 8 --label control1
```

The control wrapper changes only the prior matching consumer driver's batch scales from 1/2/4 to 1/4/16; marching, proof payloads, time preparation and arithmetic remain unchanged. Use `--run-only --label control2` for its second process. Root experiments return a diagnostic report even when coverage refuses; `production_admitted` remains false. Device or compiler acquisition failure is an execution failure, not a pass.

[Two root series, controls, bounded readable counterexamples and provenance](NORTHERN_VEIL_GPU_ROOT_PROPOSALS_2026-10-21.json), [complete generated proposal WGSL](NORTHERN_VEIL_GPU_ROOT_PROPOSALS_2026-10-21_KERNEL.wgsl), [complete raw base proposals compressed as JSON](NORTHERN_VEIL_GPU_ROOT_PROPOSALS_2026-10-21_ROOTS.json.gz), [proposal driver](../../scratch/probes/ray_transport_gpu_root_proposals.py), [ray-count control wrapper](../../scratch/probes/ray_transport_gpu_root_baseline.py). The compressed proposals have schema `[rayIndex,globalSwitch,flags,[[lo,hi],...]]`; the report maps each switch to `(member,localSwitch)`. Uncompressed and compressed SHA-256 values are recorded; both independent native processes emit identical base proposals.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T21:33:37.349632-07:00`.
