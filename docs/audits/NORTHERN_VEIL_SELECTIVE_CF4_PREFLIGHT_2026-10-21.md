# Northern Veil selective CF4: branch fallback improves accuracy, fails native economics

Zach commissioned the next pass after [uniform CF4's failed accuracy screen](NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21.md). This continues the [existing Intercom thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#codex-next-pass--selective-cf4-branchfallback-screen-in-progress). Zach's current-or-better accuracy and substantial native gain are the acceptance goal. Codex built and measured the selective trial; Opus independently completed the [projected reference](NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21.md) during this pass.

**Decision: rejected.** Two group-4 native ABBA series cost **+98.87% and +97.65%** versus current. Mean RGB error against Opus's converged reference remains **58% above current**. Selective fallback improves the old uniform CF4 error but does not pass accuracy and cost together. No production source, save, installed app, lighting, quality, output budget or ordinary timestamp setting changed. No shipped gain, PR or push.

## The bounded experiment

The new `scratch/probes/ray_transport_selective_cf4_trial.py` reuses the previous builder, which has backwards-compatible transform/diagnostic-buffer hooks. All code is experimental scratch tooling. Coarse panels group contiguous original sample intervals. On a differing observed `abs`/`clamp` branch signature between the two Gauss nodes, the panel evaluates the **original midpoint positions and update order**, including shared attenuation and the projected channel/source/gate/floor rules. There are ten traced scalar sites per medium in the current emitted shared evaluator, each with a unique two-bit slot in that medium's u32 word. No hashes collide; unvisited short-circuited branches stay zero. Private masks are fragment-lifetime Kernel scratch, reset per node and never persisted or retained across frames.

Branch equality is **not** proof of smoothness: crossings outside the two node positions can remain unseen. Two zero nodes cannot justify emptiness. A separate query checks the existing support/cell proof from the panel start across its full width; unknown zero panels use the original lattice. Proven empty runs may jump only to the earliest evidence exit or future member support entry. The detector and fallback, including discarded coarse samples, are included in cost. No general proof-grid execution or offline support oracle is used. Mathematical promotion would still belong in OntoMath; Screen would consume/lower a mathematical recipe. This trial creates no renderer-private production mathematics.

## A scratch traversal bug found and fixed before timing

The first selective version produced a new outlier at pixel `(1260,1140)`, slot 1823. A single-ray native trace exposed a skipped interval `[47.979,71.450]` containing roughly **0.042418** optical depth. The empty-panel check only needed to prove its current panel empty, but its jump reach failed to stop at a later medium's support entry when that entry lay beyond the current panel. That let the experimental jump pass real future contribution. This bug was introduced by this scratch experiment, never by the production renderer.

The fix removes the conditional that excluded those future entries from the exit calculation. On the exact regression ray, T changes **0.906554 -> 0.869107**, versus independent reference **0.869113**. A fresh native panel trace confirms that the corrected jumps skip zero-depth gaps on this ray; the old positive-depth gap is gone. Fine-reference depth allocation per gap is diagnostic, not a formal residual bound.

A forced-original-lattice native control on all 2,304 rays differs from the previous current outputs by at most **2.384e-7** over T/RGB. It validates the reconstruction/control path on the frozen fixture; it is not general branch or traversal admission. The largest-discrepancy regression and corrected trace are in the [evidence JSON](NORTHERN_VEIL_SELECTIVE_CF4_PREFLIGHT_2026-10-21.json). Hook/validation failures and the buggy pre-fix consumer were never used for timing.

## Independent accuracy is now available and consumed

Opus's `770575e6c` supplies authored-AST CPU results and finer/half-resolution columns for the same actual f32 rays, clock 7.25, projected four-medium problem and authored zero source. I inspected the reference source/contract, matched all native inputs/row coordinates, recomputed every candidate comparison and checked the published finest-half spreads: maximum **7.310e-8 T / 4.768e-8 RGB**. I did not rerun Opus's 109-second CPU integration; the producer/reference convergence run is his evidence. Numerical convergence on one frozen problem is not a universal theorem.

This supersedes the previous same-native refinement comparator as the accuracy judge. In particular, native 8,192 midpoint samples are less accurate than 4,096 against Opus's rows, consistent with accumulated f32 rounding. The previous native-refinement rejection of uniform CF4 remains correct; its comparator must not be promoted to truth.

All values below are absolute discrepancies of instrumented raw pre-framebuffer outputs against the independent reference, on 2,304 actual 2560x1440 inside rays. T is dimensionless and RGB is linear premultiplied radiance. Sparse diagnostic instrumentation can perturb arithmetic; no bit-identical uninstrumented raw-output or Person acceptance claim is made.

| Method | RGB mean | RGB p99 | RGB max | T max |
|---|---:|---:|---:|---:|
| current | 0.00005223 | 0.00074910 | 0.00195826 | 0.00302491 |
| old uniform CF4 group 4 | 0.00028785 | 0.00378632 | 0.00921488 | 0.01445591 |
| corrected selective group 4 | 0.00008248 | 0.00095293 | 0.00250968 | 0.00390996 |
| corrected selective group 3 | 0.00006423 | 0.00083928 | 0.00265423 | 0.00410193 |

Both selective variants still exceed current mean/p99/max; group 3 has no native timing claim. All captured outputs are finite, no negative RGB, and selected T stays above 0.49, excluding the legacy 0.01 cutoff as this discrepancy's cause. The method is neither declared equivalent nor activated based on sub-byte-looking errors.

## Complete native cost, not an optimistic work count

Native desktop WebGPU/Metal, configured Debug host objects, unchanged production renderer source from `a739fc755`; Opus's reference-only commits arrived during the pass. Corrected authored-source volume-only offscreen fixture, 2560x1440, inside camera `(0,50,75)/(40,62,110)`, fixed time 7.25. The baseline is the previously frozen current authored-source executable with matching ABI. This includes host work; it is not isolated GPU execution, an inhabited whole frame or live FPS.

Two sequential ABBA series at group 4, four warm frames then completion fence and 24 readback-free timed frames/run: **all 8 runs / 192 frames** are retained. No GPU correctness run from this session overlapped timing; no ordinary timestamps. Opus's reference was CPU-only and its commit preceded the first cost log. All final warm counters report zero compiles/lowerings/parameter and proof upload bytes. Absolute baseline drift is retained; no best-run selection or retesting manufactured the result.

| Series | Baseline -> selective ms | Cost change |
|---|---:|---:|
| 0 | 94.227 -> 187.384 | +98.87% |
| 1 | 111.093 -> 219.577 | +97.65% |
| pooled | 102.660 -> 203.481 | +98.21% |

The diagnostic work census visits 78,939 panels: 12,556 coarse, 44,174 fallback and 22,209 whole-zero. Density calls sum to **167,248**, versus the previous current census's 147,796 (+13.16%). Counts are instrumented diagnostics and D==0 may exit before expensive noise; they are not GPU instruction costs. Extra checking, state and duplicated evaluations plausibly explain cost, but no register/occupancy counter measurement isolates a cause. The measured complete implementation is the rejection evidence.

## Reproduce and inherit

- Build the new selective driver with `--root <ray-worktree> --output /private/tmp/<new-directory>`, optionally `--capture`. Cost must use the uninstrumented build. `EC_CF4_GROUP=4` chooses this finite screen. The existing probe CLI supplies zone, membership, viewport, time and `cost 24`.
- `EC_SELECTIVE_FORCE_FINE=1` forces original-lattice reconstruction; it is a control, never an optimization. `--trace-slot 1823 --capture` logs at most 16,384 panels in an enlarged scratch buffer for that one selected ray. Captures write the existing `EC_CENSUS` path.
- `--analyse <capture.bin> --output <report.json>` compares every raw output with the independent reference and checks pixel/ray/clip identity. `--original-control` additionally checks the frozen forced-original control within 1e-6. These controls are scratch only.
- Successful accuracy, cost, pre-fix trace and corrected regression artifacts are under the four roots in the evidence JSON. The pre-fix traced control remains valid as a forced-original control because it bypasses coarse zero jumps. No pre-fix timing is used. The durable JSON preserves all run means, independent-reference comparisons, work counts and the corrected trace/regression.

**Next preflight:** Opus's tentative support-aware placement direction remains worth testing at low cost. Use actual rays/reference to count executable panels over conservative occupied intervals and include support discovery, then build a streaming native candidate only if the complete work estimate is promising. A previous truth-assisted 8.5-sample oracle is not free support discovery; the support bounds are conservative, and unresolved kinks still need an accuracy/fallback strategy. This result rejects repeated per-node branch tracking in this form; it does not disprove every selective or higher-order method.

**Shipping account:** the independent accuracy reference is delivered and consumed; a method passing accuracy and native cost together is still missing. A profitable complete consumer, revision/device/fallback lifecycle, broader views/clocks/clipping and inhabited-frame/Person acceptance remain open. No new Person-visible behavior or Person check.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T11:04:34.754344-07:00`.
