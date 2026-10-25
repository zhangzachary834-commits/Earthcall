# Northern Veil — target-only producer experiment

**Human origin:** Zachary Zhang asked to see progress bars and then perform the next implementation pass, following the [matched native consumer checkpoint](NORTHERN_VEIL_MATCHED_NATIVE_CONSUMER_2026-10-21.md) and the [existing council thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#sixth-one-sun-producer-pass--target-only-support-preparation-in-progress).

**Starting HEAD:** `175988faf`, `claude/ray-domain-transport-rung0`.

**Result:** implemented and ran a general target-range support producer using existing OntoMath operations. Host concrete preparation falls **49.73%** in alternating processes, but remains **3.12 seconds for 2,304 rays**. Its cheaper boundary partition loses useful kink split points. Four- and two-metre panels fail accuracy; one-metre panels pass but reduce the native consumer advantage to **1.603× / 1.600×**, versus the earlier branch-aware candidate's 3.37× / 3.34×. **This is a diagnostic, not the shipping producer or a new FPS gain.** The earlier candidate remains the stronger native consumer.

## What was implemented

The scratch producer calls `LineAlgebraicEvaluator::range` and `slack` on the existing noise-free geometric envelope's density root. It cuts at existing analytic proposals and recursively classifies the whole target: proved non-positive spans may be omitted in this mathematical diagnostic; proved-positive spans stop subdividing; uncertain, unbounded, undefined and budget-limited spans remain represented. Proposals never decide emptiness. Coverage is checked for a continuous ordered partition with no gaps. Invalid envelope restriction/binding refuses the diagnostic rather than silently dropping a member.

The existing CPU `segmentLine` resolves internal switches to produce branch labels. A* never reads those labels, so this experiment removes that work and retains only target support states. However, the old partition also supplied useful locations at which the mathematical expression changes branch. Omitting labels and omitting those locations have different accuracy consequences: the latter explains why the consumer now needs more panels. This is an inference supported by the changed partition and panel-refinement results, not a general proof that every kink needs a mandatory split.

The ordered union of all four members' support pieces still generates single and coupled-overlap packets. Single packets have the configured maximum width; overlap intervals are also subdivided to that cap before their existing two-panel CF4 evaluation. Original density gating, pointwise extinction floor, emission, shared attenuation, clock and source remain as in the prior repaired diagnostic. No neighboring ray inherits another ray's answer. No new Zone-specific formula was added to the producer: it traverses the existing restricted form using the existing office's range/proposal machinery.

**Important boundary:** the original SHA-locked diagnostic still supplies its fixture-specific envelope/noise extraction and consumer constants. This pass does not generalize that admission or establish a native f32 empty-space certificate. It is neither a replacement for the production marcher nor a new permission to omit authored density.

## Host preparation cost

Eight independent CPU-only processes alternate `legacy, support, support, legacy, support, legacy, legacy, support`. Both use a one-metre single-panel cap here to compare preparation at the same panel policy. This differs from the original branch-aware GPU candidate, which uses four-metre single panels. The first exploratory legacy/four-metre preparation was 6,112.81 ms; the final paired result below includes the one-metre packet construction for both producers.

| Host concrete preparation | Legacy branch segmentation | Target-only support |
|---|---:|---:|
| Mean total, 2,304 rays | 6208.88783 ms | 3121.44030 ms |
| Mean per ray | 2694.830 µs | 1354.792 µs |
| Retained member pieces | 17,657 | 10,663 |
| Driver-level range/classification query bill | 2,097,684 including final pruning | 322,008 root queries |

The query columns count different offices: legacy switch/target classifications plus final root pruning versus target-only root queries. Neither counts every recursive interval operation. Do not turn their ratio into an instruction-cost claim. The measured host reduction is 49.73%; total includes per-ray bindings, evaluator construction, segmentation, event ordering and packet construction. Frozen fixture decode, reusable line restriction, peak host allocation and dynamic per-frame uploads are excluded. These missing costs can only worsen the already insufficient production economics; no end-to-end producer benefit is claimed.

For 8,720 box hits, target-only classification produces 98,379 empty leaves, 62,546 positive leaves and **13,529 uncertain leaves retained**. Adjacent equal support states merge, yielding 10,663 retained member pieces. No fixture budget or undefined refusal occurs. Root classification never reuses another ray's numeric endpoints.

## Accuracy decides which panels are admissible on this witness

Same frozen 2,304 rays, origin `(0,50,75)`, clock 7.25 and independent 65,536-step reference as prior passes. All means/p99/max must be no worse than stored current native output. The finite reference is a convergence witness, not exact real arithmetic.

| Candidate | RGB mean | RGB p99 | RGB max | T mean | T p99 | T max | Gate |
|---|---:|---:|---:|---:|---:|---:|---|
| Current native | 5.22350e-5 | 7.49103e-4 | 1.95826e-3 | 1.33515e-4 | 1.43029e-3 | 3.02491e-3 | Reference comparison baseline |
| Target support, width 4 | 1.10854e-4 | 8.10041e-4 | 1.84152e-3 | 3.14449e-4 | 1.53115e-3 | 2.66074e-3 | **Rejected** |
| Target support, width 2 | 7.96173e-5 | 7.49213e-4 | 1.07565e-3 | 2.02455e-4 | 1.36839e-3 | 1.67559e-3 | **Rejected** |
| Target support, width 1 | 2.39087e-5 | 3.00338e-4 | 4.71175e-4 | 6.56893e-5 | 6.49456e-4 | 8.02738e-4 | **All six pass** |

The rejected runs' timing remains in raw reports because the inherited driver times before assembling final accuracy qualifications; their attractive cost figures are not accepted performance results. The final configuration passes original authored MathNode agreement (RGB max 3.13e-7, T max 1.85e-6), five independent closed-form GPU cases and all replicated outputs. Both final processes produce byte-identical candidate and baseline rows.

Seven new meaningful support witnesses also pass: negative constant, positive constant, zero density, interior positive island despite negative endpoints, zero query budget retaining the whole interval, partial budget exhaustion retaining coverage, and undefined expression retaining coverage. These corroborate refusal/coverage behavior; finite sampled island checks are not a universal certificate for all expressions.

## Native consumer cost and memory

Observed Apple M5 / Metal, timestamps disabled. Same production-generated matching marcher, resident payloads, 294,912 invocations (2,304 distinct rays repeated 128×), four-dispatch batches, four warm batches per variant and six ABBA blocks per batch scale. Every repeated output is checked. These are completion-fenced host times, not GPU timestamps. CPU preparation, upload, readback, raster/depth lookup/composition and full unique viewport costs remain outside the consumer clock.

| Independent process | Current normalized host ms | Target-support A* normalized host ms | Ratio |
|---|---:|---:|---:|
| 1 | 15.90219 | 9.92005 | **1.603×** |
| 2 | 15.83440 | 9.89943 | **1.600×** |

Width 1 emits 49,302 single packets and 350 overlap intervals, versus the prior branch-aware candidate's 23,776 and 253. Complete resident packet/parameter/media bytes rise from **495,872 to 905,840**, on these same base rays. This is a concrete representation bill, not peak device/host memory or full-viewport capacity. Schedules are still reused by replicas. No shared-cell fusion is reintroduced.

## Decision and next implementation boundary

**Do not replace the existing candidate or integrate this host loop per pixel/frame.** Its reduced host work remains milliseconds per ray and its coarser partition consumes much of the native headroom. A simple “labels are unused, delete all branch partitions” shortcut fails the combined cost/accuracy objective.

The next bounded producer should preserve useful switch/kink/root split locations without paying per-pixel general branch proof search: reuse an authored-revision mathematical derivation/root-order recipe with explicit validity, instantiate each ray's own endpoints numerically on the GPU, and retain unknown/stale cases on the existing path. Mathematical ownership remains OntoMath; Screen lowers and executes. No claim that such a recipe is already implemented or cheap enough follows from this experiment. Require an independent coverage/fallback witness and the total producer-plus-consumer native cost before further framework work.

Still open: affordable boundary generation/lookup/amortization, full unique viewport/pass/frame costs, broader camera/clock/source/overlap witnesses, revision/device/fallback lifecycle, inhabited movement and Zach's acceptance. The dashboard retains the earlier **3.34–3.37× consumer** evidence and marks the new producer attempt as insufficient. No production source/tests/default/budget/save/app change; no visible app launch, push or PR.

## Reproduce and artifacts

```sh
python3 scratch/probes/ray_transport_support_producer.py \
  --root /path/to/ray-checkout --work /private/tmp/veil-support-new \
  --width 1 --repeat 128 --label series1
python3 scratch/probes/ray_transport_support_producer.py \
  --root /path/to/ray-checkout --work /private/tmp/veil-support-new \
  --run-only --width 1 --repeat 128 --label series2
```

Use `--cpu-only --mode legacy --width 1` and `--cpu-only --mode support --width 1` with unique labels to reproduce alternating preparation measurements. `--width 4` and `--width 2` reproduce the rejected accuracy configurations; the program exits nonzero when their reference gate fails. The wrapper checks frozen input hashes and replay source identity, links configured app objects into an external executable, and never opens the live data home. The tracked source includes the seven support witnesses; exploratory rejected reports predate those assertions but run the same support algorithm.

[Reproducer](../../scratch/probes/ray_transport_support_producer.py), [paired host/native reports including failures](NORTHERN_VEIL_TARGET_ONLY_PRODUCER_2026-10-21.json), [candidate raw rows](NORTHERN_VEIL_TARGET_ONLY_PRODUCER_2026-10-21_ROWS.json). The baseline rows are byte-identical between final processes and to the [previous baseline artifact](NORTHERN_VEIL_MATCHED_NATIVE_CONSUMER_2026-10-21_BASELINE_ROWS.json); generated current WGSL/march body are unchanged from that checkpoint. They are not duplicated here.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T20:29:03.842557-07:00`.
