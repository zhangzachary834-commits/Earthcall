# §6eo — bounded original-AST reference pilot and corrected local cost

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28, America/Los_Angeles. The machine record carries the exact signed timestamp.

Zach requested the next pass after [§6en](GPU_RAY_TRANSPORT_6EN_MODE2_OWNERSHIP_2026-10-28.md). His criterion remains current-or-better authored accuracy with useful native performance, without waiting for every part of the certification campaign. This pass executes the previously promised **cost-capped CPU reference pilot**. It does not execute the whole distinct population or the renderer candidate.

**Result:** seven bounded CPU executions pass, covering 18 old continuity rays and **1,094 distinct new-capture rays**. The unchanged generator reproduces all 144 retained continuity values exactly. Two isolated compilation/link controls preserve all eight stored reference values on every continuity and uniform/stress pilot ray. The larger 1,024-ray cohort converges within the existing diagnostic guard, but its measured local cost projects **~963 minutes / 16.05 hours**, rather than the inherited ~183-minute estimate. No full-population job was launched.

## Frozen inputs and execution

The source is Opus 5.5's unchanged `veil_projected_reference.cpp`, SHA256 `bdcdd79eb7408b45cc48346010d6584e2d4cdedbf974ae25b5302148fa443b1f`. It evaluates the original authored AST through OntoMath, not candidate WGSL or a candidate-derived numerical oracle. Its hybrid double/f32 projected-model semantics are unchanged: f32 local positions/emission values, shared clock7.25, original recorded directions/clips, additive extinction/emission, and the existing extinction floor. Its authored zero-source precondition passes. This is a converged numerical reference for that model, not a proof of exact continuum transport or general scattering correctness.

The source-matching engine tree is `3603add4018961529a91bc9592d76f9a553500b1`; the reused build is the independent `ourverse_navigator_prototype_loop/build`. The standalone probe was freshly compiled/linked through `ray_transport_probe_build.py`. Neither shared build objects nor engine sources were edited.

Before any evaluation, the selection file froze:

- **Continuity:** 16 uniform ranks plus old indices26392/28559, 18 total, from the original32,784-ray capture bound to Opus's retained full reference.
- **Uniform/stress:** 64 uniform ranks of the new230,400-ray capture, nine stride4 neighbours around the known grazing failure, and the nearest new pixel to the previous healthy-bisection worst ray, 74 total.
- **Cost cohort:** 1,024 uniform ranks of the new capture. Its four overlaps with the74-ray pilot leave **1,094 unique new rays**, not1,098.

All jobs use `2^16` finest / `2^15` half grids, four threads, and a **300-second process cap**. The original direction, clip, pixel, origin and clock are retained without normalization, jitter or replication. The Oracle/Supplier adapter verifies the parent manifest and fixture; actual four member identifiers carry explicit7.25 clock bindings. Differing medium clocks still refuse.

Receipts bind the selected input bytes, original parent indices/hash, fixture, clock map, generator, binary, build receipt, imported test, compile database and engine tree. Source/input/binary identities are checked before and after execution. A timed-out or failed run cannot obtain a passing receipt. The raw reference's eight columns acquire pixel identity only through this receipt and the existing Oracle adapter. No old reference rows were relabelled as new rays.

## Local build-cost controls

The reusable engine objects were compiled with Debug flags. The standalone main already uses `-O3`; optimizing that main does not optimize the linked evaluator. I rebuilt the **unchanged `ScalarForm.cpp` only**, in fresh external storage with `-O3` and no fast-math. A second link control places this object first so its optimized weak/template definitions can take preference over copies from earlier Debug objects. The shared build tree is read-only throughout. Full compiler/link arguments and object/source fingerprints are retained.

| Build control | 18-ray continuity wall seconds | 74-ray uniform/stress wall seconds | Changed stored reference values |
|---|---:|---:|---:|
| Original reused Debug engine |15.009 |57.469 |0 versus retained continuity |
| Isolated optimized evaluator, original link position |9.259 |30.647 |0 versus original pilot |
| Same evaluator, optimized object first |4.852 |16.796 |0 versus original pilot |

On this one ordered pilot series, the last variant takes3.42× less wall time than the original on the same74 rays. This is **reference-generation tooling cost**, with process startup/fixture load included and compilation excluded. It is not an Earthcall GPU/frame improvement, a repeated statistical benchmark, or proof of a unique physical bottleneck. Link order is controlled; register pressure, allocator contention and GPU occupancy are not measured here. Exact comparisons cover all eight parsed binary64 values, including the nested resolutions and signed-zero representation.

Only after continuity and full74-ray parity passed, the preselected1,024-ray cohort ran on the optimized-first variant:

| Observation | Result |
|---|---:|
| Process wall time |256.798 seconds |
| Integrator-reported time |256.6 seconds |
| Reported field evaluations, rounded |148 million |
| Max finest/half transmittance difference |6.52939064e−8 |
| Max finest/half RGB difference |3.90828087e−8 |
| Linear230,400-ray wall estimate |962.994 minutes /16.05 hours |

The74-ray stress cohort's maximum differences are6.81554679e−8 T /4.01704683e−8 RGB. Every selected ray clears the unchanged1e−6 convergence refusal. That threshold is a diagnostic self-consistency guard, **not an authored error budget or formal error enclosure**. Agreement of two grids does not rule out a feature both grids miss. The pilot does not establish whole-population p99/max accuracy, candidate accuracy on the new capture, or convergence of the other229,306 new rays.

The inherited183-minute model was not portable to this local partially Debug-backed evaluator. Even after the isolated optimization, the measured1,024-ray estimate is5.26× longer. No expensive background job was started on the old estimate. The1,024-ray cohort was held until the74-ray optimized-first measurement projected it inside the cap, then completed within that cap.

## Reproduction and evidence integrity

[Machine record](GPU_RAY_TRANSPORT_6EO_REFERENCE_PILOT_2026-10-28.json), [compact evidence ZIP](GPU_RAY_TRANSPORT_6EO_REFERENCE_PILOT_2026-10-28.zip), [pilot runner](../../scratch/probes/ray_transport_6eo_reference_pilot.py), [isolated build control](../../scratch/probes/ray_transport_6eo_reference_build.py), [CPU replay](../../scratch/probes/ray_transport_6eo_replay.py).

The archive retains all selected inputs, selection files, raw eight-column outputs, execution receipts, logs, build metadata, source tools and the counsel received during the pass. Parent source-of-truth ray bytes remain in the separately hash-bound §6en archive. Replay reconstructs selected new rows directly from its packed native census and checks old rows against its archived original ray file. It also checks old-reference continuity and exact optimization parity; it does not trust today’s selected file to identify itself.

Seven targeted replay mutations refuse: changed input during execution, timeout marked as a result, stale generator, re-signed altered native direction, re-signed truncated reference, re-signed nonconverged reference, and re-signed nonfinite reference. All17 existing Sonnet integration controls pass separately. These are CPU provenance/validation controls, not another native GPU run.

```sh
python3 -B scratch/probes/ray_transport_6eo_replay.py \
 --record docs/audits/GPU_RAY_TRANSPORT_6EO_REFERENCE_PILOT_2026-10-28.json \
 --archive docs/audits/GPU_RAY_TRANSPORT_6EO_REFERENCE_PILOT_2026-10-28.zip \
 --output /tmp/6eo-replay.json
```

Rebuilding uses a source-matching build with `ray_transport_probe_build.py`, then `ray_transport_6eo_reference_build.py --base-build <fresh-original-build> --work <fresh-external-directory> --link-position first`. `original` reproduces the intermediate link-position control. Do not reuse a differently sourced installed/release app to save compile time. Run the18-ray continuity and74-ray fixed pilot before allowing a larger cohort; retain the before/after receipt. The generator and shared numerical gate were not edited by this pass.

## Reply to Firmament's newly arrived counsel

During this run Aetherion / The Firmament supplied [post-§6en counsel](FIRMAMENT_COUNSEL_POST_6EN_AND_SONNET_MERGE_WHY_MODE2_WON_AND_6EO_BRIDGE_PLAN_2026-10-28.md). His original file was concurrent and untracked when received; I did not edit or stage it. The archive preserves the exact received version and hash. This continues that counsel and the existing [Opus handoff thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md).

**Adopted direction, with boundaries:** separating support scheduling from optical-channel evaluation is the useful Mode2 seam. The consumer should reuse production channel lowering/bindings rather than maintain Northern-Veil-specific optical meaning. Producer density-plan parameters still need their own correct translation; binding production `P` in the consumer does not by itself solve the producer's density-only offset contract. Correct per-medium clocks, frames/directional arguments, floors/clamps, shared attenuation and final composition must be witnessed through the consumer, not assumed from function names. No new renderer-specific OntoMath API is adopted here.

The array/liveness explanation for Mode2's timing advantage remains a **physical hypothesis**. Source array sizes do not establish live registers, spills or occupancy. Existing matched clean timings favor the candidate; they do not establish that causal account.

His proposal to construct each worker's binding map once and update its seven mutable values is the right **next bounded validation experiment**, attributed to Firmament. Keep the canonical reference unchanged as a control; generate a separately named derived source, freeze its source/derivation identity, and require exact eight-value equality on at least256 fixed rays before expanding. The current Oracle deliberately accepts only the original generator hash: a changed generator must have an explicit verified derivation contract, never a receipt falsely naming the original source. No hoisted implementation or measured5–10× gain is claimed in this pass.

Three corrections accompany that proposal: field-evaluation counts include separate density/extinction/emission calls and are not the number of outer binding-map constructions; AST evaluation itself still creates local maps/variants, so hoisting the outer map does **not** establish zero inner-loop allocation; and linear4→8-thread scaling must be measured. The existing local build-profile cost also needs to stay in the model. These qualifications preserve the proposal while removing unmeasured promises.

His preflight tightening is sound: reset header counts and poison subpanel/overlap storage as well as outputs before each untimed mode. Keep those diagnostic writes outside timed loops and retain §6en producer/publication controls. The obsolete stream allocations are80MiB (83.89MB) of planned footprint; deleting them is a memory-footprint change until matched cost is measured. Removing an unused reference parse must not remove the mandatory full-row Oracle gate or let an unreferenced diagnostic run acquire an accuracy label. This pass does not modify or execute that GPU harness.

## Remaining shipping account and next work

1. **Distinct-population numerical and economic qualification remains OPEN.** Reduce reference construction overhead under exact continuity, establish a measured job budget, generate all remaining rows with receipts, then gate contemporary Current and actual candidate rows over the complete population at both resolutions. Keep full-schedule preflight poisoning and storage/dispatch limits. No192-holdout shortcut.
2. **Production Screen connection remains OPEN.** Begin with all-refused publication matching Current, then authoritative device ray/opaque-depth equality, composition and validity/clock/edit witnesses. Production-evaluator reuse is direction, not an implemented bridge.
3. **Inhabited frame performance and Person acceptance remain OPEN.** Offscreen pilot convergence and reference build speed establish neither native frame profit nor movement FPS.

No engine source, original reference generator, shared numerical gate, save, installed app, rendering default, lighting or resolution changed. No new Person-visible feature/check was introduced; existing Person checks remain open. No GPU lane was claimed. Evidence and coordination are local; this pass does not push to GitHub.
