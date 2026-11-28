# §6cu — review of merge, culling and secant experiments

Zach commissioned review of Sparkly Guy’s work and Firmament’s analysis, corrective repairs, then Sparkly’s next assignment. His criterion remains current-or-better faithfulness to authored equations with a substantial native performance checkpoint. This pass repairs an actual fallback regression and completes a structurally acceptable numerical trial; it promotes no renderer optimization.

Reviewed ray HEAD `e1a682d4e`, including `794e5ea03`. Parents: [§6cr](GPU_RAY_TRANSPORT_MERGE_SWEEP_2026-10-22.md), [§6cs/ct](GPU_RAY_TRANSPORT_ENVELOPE_SUBCACHE_AND_SECANT_2026-10-22.md), [Firmament counsel](../../agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CR_LINEAR_Z_CULL_SUBDIVISION_CACHE_AND_SECANT_2026-10-22.md), [§6cq safeguards](GPU_RAY_TRANSPORT_SLIVER_REVIEW_2026-10-22.md), and [existing Intercom](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md).

## Findings and implemented corrections

**Exact-budget regression, reproduced on Metal.** The original producer charges work before each of24 rejected coarse cells. Exactly24 available units suffice. Firmament’s proposed shortcut and Sparkly’s implementation use `work + 24 >= budget`, unnecessarily refusing equality. Nine native geometry controls (16 rays each, budgets23/24/25, original/faulty/fixed) show original and corrected refuse16/0/0 rays while the faulty version refuses16/16/0. The repair uses `work > budget || budget - work < 24`, preserving equality and avoiding addition overflow. It is applied at both shortcuts in the original §6cs script. These are producer geometry controls, not RGB accuracy or timing witnesses; the fault adds fallback work rather than demonstrating wrong pixels.

**The all-level secant failed a required structural gate.** Independent replay reproduces6 missing matched affine endpoints and47,084 changed ray-member support unions. Its all-query AST and sparse reference checks pass, but those checks do not discharge this separate admission requirement. The old executable could return success while its metadata claimed affine preservation. Its runner now invokes a mandatory shared structural gate; the actual admission function refuses the old native result. Metadata/prose no longer claims affine preservation. No assertion is made that every shifted bracket is physically wrong; it fails this experiment’s agreed preservation contract.

**The zero-level-only alternative was executed.** Firmament proposed this as Variant2A; it had not been included in the previous pass. The new `ray_transport_gpu_producer_budget_review.py --variant secant-zero` confines secant to `c==0`, retains original outer-level bisection and the corrected budget rule, and runs independent structural admission against the immutable guarded baseline. All12,102 matched affine endpoints remain, with0 changed outer support unions. Its28,208 changed schedule rays and247,255 subpanels make it a separately labeled numerical candidate, not exact old execution. Overlaps remain1799. The corrected cs path retains exactly247,326 subpanels/1799 overlaps and0 schedule/output differences on this capture.

**Mathematical direction accepted; universal parity and hardware diagnosis qualified.** Shared subdivision values and slope-bracketed secant are reasonable proposals under the existing declared error model. The fixed0.05/0.001 culling allowances do not establish a generic f32 theorem. A stored CPU-f32 scalar counterexample at `s≈1e6` has `Phi=2`, `zCull≈0.251`, but the original arithmetic slack/bow exceeds2: the physical cull rejects while the old slack test retains uncertainty. This refutes universal schedule-parity inference, not authored-image correctness; physical density in that control is zero. Native parity remains established only for the frozen capture. No measured SIMD counters, actual execution-width query, occupancy/spill trace or root-iteration census establishes the claimed SIMD root cause or4–6× call reduction. Different-process phase clocks neither establish the claimed0.14ms causal gain nor add into an inherent producer+consumer lower bound. Those are hypotheses, not findings.

## Executed accuracy and economics

Both new variants pass32,784 native original-AST assertions at1e-5, nine actual merge-policy GPU controls, normal streaming/bulk bit equality and exact forced native fallback at8192/16384/32768 capacities. Max AST deviation: corrected cs RGB3.5763e-7/T8.9407e-7; zero-only RGB3.9488e-7/T8.9407e-7. The same192 predetermined holdout rays pass mean/p99/max gates at both65536/32768 reference resolutions against captured and contemporaneous native baselines. Dense references cover192 rays, not all32,784, and these are aggregate gates rather than a claim every ray improves.

The original tables’ 2/4-repeat times are total batch milliseconds. Ratios are valid, but comparison with per-query numbers requires division by repetitions. The table below normalizes every observation to one complete32,784-query workload, at32768 tile capacity and4 repetitions:

|Stage/series|Current mode0 ms/query|Global bulk mode1|Bounded stream mode2|
|---|---:|---:|---:|
|§6cr first|1.810835|1.915138|2.067770|
|§6cr confirm|2.180570|2.474656|2.666271|
|§6cs original|1.687874|1.909379|1.930853|
|§6ct all-level|1.546336|1.587073|1.614091|
|§6cu corrected cs|1.574526|1.717723|1.830505|
|§6cu zero-only|1.901240|2.184417|2.245142|

Both corrected candidates lose both connected modes here. The all-level secant remains rejected on structure and cost; the zero-only candidate passes scoped structure/accuracy but loses cost. No second economic promotion series is justified after these losses. Includes host encoding, submission and blocking completion; excludes cold preparation, CPU reference and readbacks. GPU timestamps remain off. Bulk is not the bounded ring path. Component clocks remain non-additive. No full-frame/depth/composition or FPS claim.

## Evidence and reproduction

[Machine record](GPU_RAY_TRANSPORT_ROOT_REVIEW_2026-10-22.json), [executed replay](GPU_RAY_TRANSPORT_ROOT_REVIEW_2026-10-22_REPLAY.json), and [value-preserving bundle](GPU_RAY_TRANSPORT_ROOT_REVIEW_2026-10-22.zip).48 source/data documents share11 deduplicated large arrays (24.8MB compressed). JSON values reconstruct losslessly and are hash-checked; original JSON whitespace is not archived. Compiled source bytes are exact, with original raw-report hashes retained as provenance. The replay checks six native stages, both reference baselines, packet/output parity, captured affine endpoints/support unions, complete clock normalization, exact fallback, nine budget controls and nine corruption refusals. All four current witness generators match their executed C++ snapshots. Native AST assertions are recorded; raw AST node values are not independently reevaluated offline.

Portable replay from either checkout (standard library, no GPU or temporary fixture):

```sh
python3 -B scratch/probes/ray_transport_root_review.py --root . --record docs/audits/GPU_RAY_TRANSPORT_ROOT_REVIEW_2026-10-22.json --output /private/tmp/root-review-fresh.json
```

Native reproduction **from the ray checkout**, with the configured desktop build, frozen fixture, exclusive timing ownership and fresh external paths:

```sh
python3 -B scratch/probes/ray_transport_gpu_producer_budget_review.py --root . --variant cs-fixed --work /private/tmp/root-fixed-fresh --build-only
python3 -B scratch/probes/ray_transport_gpu_producer_budget_review.py --root . --variant cs-fixed --work /private/tmp/root-fixed-fresh --run-only --case /private/tmp/earthcall-wide-case/manifest.json --label fresh
```

Use `--variant secant-zero` in a separate work directory for the qualified numerical variant. `baseline` and `cs-before` preserve the executable budget regression witnesses; `cs-before` deliberately recreates the old fault. Corrected standalone cs/secant runners enforce structural admission. Their older compiled snapshots remain historical artifacts; rebuild rather than bypass source guards after a generator change.

## Shipping standing and next experiment

All three gates remain OPEN: profitable repeatable connected distinct-ray execution; generic OntoMath density/root/source/clock/revision/device lifetimes plus full viewport/depth/composition/frame economics; moving inhabited p95 and Zach acceptance. No production/default/sample/lighting/budget/save/live-data/app change or push. Existing Person verification remains pending; this creates no application-visible change. New caches remain ephemeral per-ray/member GPU scratch, rebuilt from current specs/ray/config each dispatch.

Sparkly’s next isolated experiment is to fuse **normal producer+consumer execution**, retain the existing native fallback dispatch, and avoid the normal-path global schedule packet round trip. This tests lowering/dispatch/storage organization without claiming SIMD is the cause. Use the least complex validated guarded mathematics and original consumer; do not bundle more root/culling/quad changes. The fallback dispatch still counts. Private-state growth can make this worse, so there is no predicted win. Firmament reviews the contract; Sparkly implements and measures. Full instructions follow in the existing Intercom.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T18:21:23.901547-07:00`.
