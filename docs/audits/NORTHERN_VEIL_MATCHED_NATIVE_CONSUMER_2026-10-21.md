# Northern Veil — matched native consumer comparison

**Human origin:** Zachary Zhang's continuation after the dinner-watch overlap repair: “KEEP GOING.”

**Implementation / native witness:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a`.

**Starting HEAD:** `a47e5264a`, branch `claude/ray-domain-transport-rung0`.

**Result:** complete unfused A* is **3.37× / 3.34× faster** than the production-generated marcher on the same resident compute ray workload, at the previously measured better aggregate reference accuracy. **This qualifies consumer headroom, not a full-viewport frame, producer, production or inhabited FPS win.**

## Exact comparison rather than a weaker baseline

The new scratch reproducer calls `sdfwgsl::lowerVolumeEvaluator` and `composeVolumeSet` on the four original bindings, including all channels and the actual zero authored source. It retains the generated evaluator and marching body verbatim. Only the entrypoint/ray inputs, opaque clip input and raw output are adapted from fragment to compute.

The baseline uses the original authored box events and 96-sample-per-chord quality policy, shared expression evaluation, same per-medium origin/extent/time/parameter segments, the actual `VolumeZeroProof` payloads/support bounds, and the production-generated GPU time-preparation module. The inside-camera trim-support lane is selected by the same box-membership test as the renderer. Certificates stay off; ordinary timestamps stay off. Clock is frozen at 7.25; time preparation executes before timed batches. Proofs, payloads and pipelines are resident for both candidates. Compilation, CPU boundary discovery, uploads, readback and correctness checks are excluded from this consumer experiment.

Both kernels receive the exact captured origin, direction and opaque-clipped endpoints from the same 2,304 reference rays and produce raw radiance/transmittance. The baseline emits the original accepted-lattice result; A* emits the repaired complete single-plus-overlap result. No hand-written approximate midpoint baseline is substituted. Proxy rasterization, ray unprojection, depth texture loads and final composition are omitted from both compute consumers, and remain part of the future full renderer gate.

### Baseline reproduction gate

Before timing, the generated compute marcher is compared to the stored actual native outputs in `RAYS.json`:

- **Maximum transmittance delta: exactly 0.0.**
- **Maximum RGB delta: 4.47035e-8.**

The probe refuses timing if either discrepancy exceeds 1e-5; the observed RGB discrepancy is over 200× smaller. The original marching body and generated compute source are retained with hashes. This is a finite reproduction witness, not a universal cross-stage byte-identity claim.

## Workload and measured hardware

Actual queried adapter: **Apple M5 / Metal**, timestamps disabled.

The 2,304 distinct frozen rays are repeated 128 times per dispatch: **294,912 unique output invocations**, 4,608 workgroups of 64 threads. Each invocation writes its own output; every repeated output is compared bitwise to its corresponding base ray before performance is accepted. Repetition probes occupancy/throughput on the same ray distribution; it is **not 294,912 independently sampled scene rays** and is not a full viewport.

A* reads the resident 495,872-byte precomputed base schedule, including its 253 overlap intervals. Replicas reuse that base schedule rather than allocate a distinct schedule for each repeat. Output storage is 4,718,592 bytes. Thus the experiment does not establish bandwidth/capacity for a full unique per-pixel boundary table. Any production producer must pay for its own representation and lifetime.

Each independent native process measures batch sizes 1, 2 and 4 dispatches per fence. Each scale has four warm batches per variant, then six ABBA blocks: 12 measured batches per variant, plus six empty batches. Reported normalized cost divides the four-dispatch batch by four. No empty-time subtraction is called GPU time. All durations are submission-to-blocking-poll **host elapsed time**.

| Series | Current batch, ms (4 dispatches) | A* batch, ms (4 dispatches) | Current normalized, ms | A* normalized, ms | Ratio |
|---|---:|---:|---:|---:|---:|
| 1 | 63.43734 | 18.80638 | 15.85933 | 4.70159 | **3.373×** |
| 2 | 63.49937 | 18.99355 | 15.87484 | 4.74839 | **3.343×** |

The empty four-dispatch batches are about 1.56–1.57 ms, substantially below both measured workloads. The gain therefore survives the fixed fence floor that obscured the earlier fusion trial. It also survives independent processes. No register, occupancy or hardware-counter cause is asserted.

A preliminary sparse 2,304-ray run measured approximately 3.63× with 64 dispatches/fence; the large repeated workload is the preserved qualification result. No further broad timing campaign was run once the final code passed both independent series.

## Accuracy and separate arithmetic checks

The timed **unfused** candidate is scored directly against the same independent 2^16-step reference. Both final processes produce identical candidate and baseline rows. Candidate rows are also byte-identical to the retained overlap-repair rows, avoiding duplicate dataset storage.

| Error | Stored current native | Complete unfused A* |
|---|---:|---:|
| Pooled RGB mean | 5.22350e-5 | 6.92274e-6 |
| Pooled RGB p99 | 7.49103e-4 | 5.05613e-5 |
| RGB max | 1.95826e-3 | 9.15713e-5 |
| T mean | 1.33515e-4 | 2.02057e-5 |
| T p99 | 1.43029e-3 | 1.09961e-4 |
| T max | 3.02491e-3 | 1.39098e-4 |

All six aggregate gates pass. RGB/T per-ray regressions remain 474/495 out of 2,304, as reported previously. Five independent closed-form GPU cases still pass: zero density, tiny floored emission, ordinary transfer, pure colored overlap, and single–overlap–single order. The original authored MathNode comparator remains independent of the manually copied envelope; maximum RGB/T discrepancies remain below 1e-6. Fused output is retained only as a separate arithmetic check; fusion is not the timed candidate and remains rejected from §6cb.

This establishes that a different approximation can be both more accurate on the witness and cheaper on the GPU. It does not establish an error certificate for every ray, clock or medium, or exact continuous integration.

## Reproduce

With an already configured/built ray checkout and a fresh external work directory:

```sh
python3 scratch/probes/ray_transport_native_march_comparison.py \
  --root /path/to/ray-checkout --work /private/tmp/veil-matched-new \
  --repeat 128 --label series1
python3 scratch/probes/ray_transport_native_march_comparison.py \
  --root /path/to/ray-checkout --work /private/tmp/veil-matched-new \
  --run-only --repeat 128 --label series2
```

The wrapper composes the checked prior overlap repair and preserved original scratch source. It verifies frozen fixture hashes, captures generated source provenance, checks replay source identity, baseline output reproduction and replicated-output equality, and fails on independent-AST, closed-form or aggregate-accuracy failure. Native readback is outside all timed batches. Generated WGSL and raw output are emitted beside each report. The installed app and data home are not opened.

Artifacts: [two-series JSON](NORTHERN_VEIL_MATCHED_NATIVE_CONSUMER_2026-10-21.json), [baseline raw rows](NORTHERN_VEIL_MATCHED_NATIVE_CONSUMER_2026-10-21_BASELINE_ROWS.json), [generated current-marcher compute shader](NORTHERN_VEIL_MATCHED_NATIVE_CONSUMER_2026-10-21_CURRENT.wgsl), [candidate raw rows](NORTHERN_VEIL_METAL_OVERLAP_REPAIR_2026-10-21_ROWS.json), [reproducer](../../scratch/probes/ray_transport_native_march_comparison.py).

## What now stands between this and shipping

**Passed:** a matching-workload native **consumer** cost/accuracy gate on the frozen ray distribution. This is the first qualified positive consumer-cost checkpoint in this sequence; it supersedes “consumer benefit unmeasured” for this precise compute scope.

**Still open:**

1. Affordable concrete boundary production/lookup with all cold preparation, reuse lifetime, repair, invalidation, upload, memory and fallback costs included. The current ~903-range-evaluation/ray CPU producer is not viable per pixel/frame. This experiment supplies its boundaries for free to establish headroom, not to hide that bill.
2. Full unique viewport, opaque clipping/composition, broader poses/clocks/source/overlap and a full pass/frame cost win. The repeated 2,304-ray distribution does not establish those outcomes.
3. Production admission/key/revision/device/fallback lifetime and an inhabited movement result with Zach's acceptance.

The next implementation target is the **smallest general boundary recipe that can repay itself**. Reuse a valid derivation/topology/root-order representation while instantiating each ray's own endpoints and depths. Unknown/stale cases retain the existing marcher. Mathematical validity remains OntoMath's office; backend consumption remains Screen's. Do not introduce a Zone-specific root formula, loosen an author budget, lower resolution or retune the 96-sample control to claim a win.

No production source/tests, save, installed app, defaults or budget changed. No visible app was launched, no new Person interaction is claimed, and the existing inhabited verification remains open. Local evidence/implementation only; no push or PR.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T20:08:48.886942-07:00`.
