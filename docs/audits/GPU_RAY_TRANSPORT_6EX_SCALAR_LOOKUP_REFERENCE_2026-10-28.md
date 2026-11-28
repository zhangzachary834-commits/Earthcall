# §6ex — direct numeric lookup makes the reference modestly cheaper

**Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28, America/Los_Angeles.** Zach requested the next pass after [§6ew](GPU_RAY_TRANSPORT_6EW_MULTIVIEW_NUMERICAL_PILOT_2026-10-28.md), with the standing requirement of authored fidelity and a usable native checkpoint. The existing [council thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md) records the scope and cost cap before execution. This follows the per-leaf allocation hypothesis identified in the reference campaign; the concrete direct-lookup derivation and controls are this session's work.

**Result:** an external derivation of the original OntoMath evaluator reproduces **12,208 binary64 values across 1,526 reference execution rows**, including all 880 retained §6ew rows. Original and derived generic semantic records match 55,191 bytes exactly; both binaries pass the existing `ontomath_test`. A selected scalar-leaf allocation witness drops 20,000 measured `operator new` calls to 0 across 10,000 evaluations. Four 32-ray ABBA blocks favor the derived evaluator, with **12.24% lower median paired complete-process CPU cost**. This is a reference-execution experiment, not an app change, new ray coverage, full mathematical certification or GPU/FPS gain.

## What was changed, and what was preserved

Production `ScalarForm.cpp` remains byte-identical, SHA256 `cf41aaa87aa7e339d97c5e60b80be15f6959c72c0ab52cede5dd625bf84dde0d`. The reviewed `MathNode::ScalarLeaf` path constructs a temporary `std::map<string,double>` for each nonconstant scalar leaf. Binding values are converted through `propertyValueToNumber`, then `ScalarForm::evaluate` sums the ordered terms, with `Term::evaluate` performing the powers and transcendental products.

The external source derivation introduces two internal lookup templates that share that existing term/sum arithmetic. Public numeric-map methods adapt their current bindings to those helpers. The scalar leaf instead looks up and converts the current call's `PropertyValue` directly. Term order, factor order, `pow`, transcendental evaluation, multiplication and summation expressions remain ordered as before. Constant-leaf fast handling remains. There is no expression simplification, renderer-specific formula, new public API, retained state, TLS, memo or invalidation scheme. A local callback borrows the immutable call environment only during evaluation. Mathematical execution stays within OntoMath.

The three edits are closed to the reviewed parent SHA and reversible to its exact bytes. The derived file has its own identity; it never impersonates the original evaluator. The reference main is the existing, independently qualified §6ep worker-binding derivation of Opus's original `veil_projected_reference.cpp`. Both variants use the same compiled main and other app objects; only the evaluator object differs. The math object is compiled with `-O3` and linked first, with the old `ScalarForm.cpp.o` excluded. No fast-math or relaxed arithmetic flag is introduced. This does not make every other linked Debug object Release.

The existing Oracle admission implementation is unchanged. This experiment does not silently authorize a new evaluator for arbitrary future reference jobs, or adopt its derivation into the app.

## Actual checks and the environment correction

The semantic harness covers numeric alternatives, signed zero, infinities/quiet NaN/subnormal inputs, missing and nonnumeric variables, zero-coefficient missing symbols, fractional/negative powers, all four transcendental kinds, repeated names, cancellation-sensitive ordered sums, irrelevant bindings, and erase/rebind sequences. Nested `Let`, SDF ambient-point rebinding and six-point Gradient evaluations are exercised. Eight asynchronous workers also compare their own results to serial executions at the same bindings in each binary. The cross-binary retained record is the seed 0 serial suite: 495 binding cases plus 4,000 nested results, 55,191 bytes. This is bounded observed parity, not a proof over all possible values, floating-point environments, subjects or expression shapes.

The allocation observer overrides ordinary `operator new` only in the dedicated semantic executable. Its counted section evaluates an already-built scalar expression referencing `x` and `y`10,000times: original 20,000 allocation calls, derived 0. The reference timing binaries contain no allocation instrumentation. This does not establish zero allocation for whole ASTs, SDF/Gradient environments, vectors or the reference process, and is not an allocator-contention profile.

An initial **original** `ontomath_test` invocation failed before its assertions completed because it attempted to create relative `logs/` in the protected checkout. The exact failed log/receipt is retained under `rejected-environment-checks/`. The harness was corrected to run suites in a temporary directory and references against a byte-identical copied frozen fixture under the temporary work root. Both original/derived suites then pass. No permissions were broadened and no live data home was used. The original build-stage Python runner is retained separately because that correction came after compilation; actual execution receipts bind the corrected runner and working directory. No stale build provenance is rewritten to claim the corrected runner built those objects.

## Independent reference binding and declared budget

The retained expected outputs come from the hash-bound §6ew archive, not the candidate. Their input hashes, original execution receipts, original binary/source identities,80-row coverage and convergence are checked before use. The 18 continuity controls also retain their historical expected results. Fresh canonical-original, worker-original and worker-lookup executions all reproduce those 18 rays. A fresh original worker 80-row job reproduces the current control against the historical archive. All 11 derived 80-row jobs reproduce their independently held results, including both integration resolutions.

Four timing blocks reuse a fixed 32-rank subset of the inside ideal-camera pilot, chosen and recorded before candidate execution. Their expected outputs are independently selected from the same retained reference, not from timing output. At each observation, before/after source, actual binary, main, build, input and output identities are checked. Grids remain 2^16/2^15 and threads 8. The cap declared before execution was **2,048 rows /600 seconds cumulative reference-process wall time**. Actual use is **1,526 rows /191.192788 seconds**, including canonical continuity and all16timing observations. Compilation and the general semantic suites are outside this explicitly reference-process cap.

The1,526 rows comprise 54 continuity execution rows, 80 fresh original controls, 880 derived pilot rows and 512 timing rows. They reproduce 12,208 binary64 values, including signed-zero distinctions. This adds **zero new independent population coverage**: it repeats retained inputs with another evaluator. The previous full-reference projection remains an estimate; a smaller, deliberately selected cost cohort is not a basis for claiming a new whole-population completion time.

## Measured complete CPU process cost

Each observation starts a fresh process and includes startup, fixture load, thread creation, evaluation, output and process completion; compilation is excluded. Each block runs **original → lookup → lookup → original**. Its ratio is the mean of its two lookup observations divided by the mean of its two original observations.

| Block | Original pair mean, s | Lookup pair mean, s | Ratio | Reduction |
|---|---:|---:|---:|---:|
|0|4.876674|4.173888|0.855888|14.41%|
|1|4.521016|4.065851|0.899322|10.07%|
|2|4.422856|4.287354|0.969363|3.06%|
|3|5.178088|4.386117|0.847053|15.29%|

Median paired ratio **0.877605411**, reduction **12.2395%**. All 4 blocks favor lookup; individual observations are noisy, and one lookup observation exceeds the first original observation in its block. The last original observation is 5.8517 s. That variation is preserved, not removed as an outlier. Four blocks are a bounded screen, not a confidence interval, universal speedup or measured explanation of all reference cost. The result supports a useful modest local improvement, not the earlier unobserved multi-fold allocation hypothesis. Noise, other AST execution and integration work remain; no causal percentage is assigned to those costs here.

## Replay, deliberate refusals and artifacts

[Machine result](GPU_RAY_TRANSPORT_6EX_SCALAR_LOOKUP_REFERENCE_2026-10-28.json), [lossless evidence](GPU_RAY_TRANSPORT_6EX_SCALAR_LOOKUP_REFERENCE_2026-10-28.zip), [runner](../../scratch/probes/ray_transport_6ex_lookup.py), [closed derivation](../../scratch/probes/ray_transport_6ex_lookup_derivation.py), [general witness](../../scratch/probes/ray_transport_6ex_semantics.cpp), [replay](../../scratch/probes/ray_transport_6ex_replay.py).

The archive contains generated source, old/new runner provenance, commands, object/binary hashes, fixed fixture, parent-bound controls, outputs/logs/receipts, semantic records and the failed environment attempt. Binaries/objects are represented by hashes and commands rather than committed. Size **369,914bytes**, SHA256 **`e4716d76a54a4fb463786a7924273d129aa5f07d6f79e9904b3bf76ec73b149c`**. Parent §6ew archive and matching engine/tool sources are required for replay. Replay validates retained observations; it does not rebuild executables or rerun timings.

Six deliberate controls reject: a one-ULP output change, nonfinite output, missing row, reordered unequal output rows, changed parent source, and a derived evaluator labelled canonical. Expected rows are checked directly against the original parent archive, so changing both a local candidate and its local expectation does not create a new independent reference. CPU replay passes after extraction to a fresh temporary root. A replay-only local-name collision was repaired before packaging; no reference execution needed to be repeated for it.

```sh
python3 -B scratch/probes/ray_transport_6ex_replay.py replay \
 docs/audits/GPU_RAY_TRANSPORT_6EX_SCALAR_LOOKUP_REFERENCE_2026-10-28.zip
```

## Shipping account and next bounded quest

**Done:** actual feature-branch renderer remains connected/defaultON with Current fallback; earlier bounded transition/native numerical witnesses remain as dated evidence. This pass now supplies a measured, exact-on-controls reference-cost improvement as a separately identified external tool.

**Still open:** full/wider-clock/field numerical coverage, generic certification, inhabited release-frame/pacing economics, and Zach's appearance/movement acceptance. The source still contains its original map allocation, because this pass qualified an external derivation rather than broadly adopting it. No renderer, authored lighting, resolution, sample cap, app installation, default checkout or live save was changed. No GPU lane was claimed, no timestamps enabled, no push/PR performed. There is no new Person-visible behavior or acceptance checkbox to mark.

**Next prioritize the inhabited release checkpoint over another unprofiled reference micro-optimization.** Configure a genuinely separate Release build bound to this feature checkout, with its proper OpenSSL/CMake policy and isolated data root; the copied `build_release` cache names the other checkout. Freeze a full-resolution camera/time path and OFF/ON/ON/OFF block protocol before measurement. Compare complete frame/completion and pacing, keep startup/cold compilation separate, and avoid concurrent native workloads. Claim/release the Intercom GPU lane. Preserve Current fallback, actual production fields and no-timestamp policy. Measurements of the still-experimental candidate may proceed while numerical admission remains explicitly open; promotion requires both authored-accuracy and native-economics gates. The reference derivation can support a separately capped wider cohort, but it does not justify silently launching the projected multi-hour full grid.

Firmament's fresh Gemini task did read the requested files, then showed failures and an explicit usage limit with Tasks available 23:27 local. There is no completed new counsel in Intercom at the start of this pass. That is an observed app limitation, not a diagnosis of its internal cause; the campaign continued independently. Future review should distinguish the experimentally identified evaluator from the unchanged production source and the unchanged numerical admission gates.
