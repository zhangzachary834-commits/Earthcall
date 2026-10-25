# Ray transport — multi-view accuracy, authored clock coefficients and native cost

> Cross-worktree relay: implementation is local on `claude/ray-domain-transport-rung0`, commit `a7d3c16da`. Run code/reproduction commands in `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop`; default receives documents and evidence only.


Zach requested the next pass after the [overnight GPU review and retained uncertainty-band checkpoint](GPU_CANONICAL_PRODUCER_OVERNIGHT_REVIEW_2026-10-22.md), commits `55f201925` / `2ea31cf38`. This continues the [existing council thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). His commission remains a useful native speed checkpoint at current-or-better accuracy, preserving the authored equations rather than retuning a pleasing image.

**Outcome:** the retained candidate now passes bounded numerical checks at four actual native camera poses and two declared clocks. A changed-clock test first exposed the prototype's clock-specific emission table; this pass binds the coefficient from the original authored expression instead. Inside/reverse/side/spawn and clock-zero cases pass all six mean/p99/max gates against **both** independent 65,536/32,768-step references. The original 2,304-ray control also passes twice. Native throughput varies substantially by view, and a tiny exact-empty control is slower; no universal enablement, full-frame/FPS or Person acceptance is established. Production/default/save/data-home/installed-app state is unchanged. No push or PR.

[Machine record and compressed raw/capture/source archives](GPU_RAY_TRANSPORT_MULTI_VIEW_QUALIFICATION_2026-10-22.json) preserve ten native numerical records (including the expected failed clock witness), seven restorable fixtures and seven exact generated C++ sources. **36 actual C++ binding/refusal checks and 13 external-case controls pass.** All final accepted native cases retain the existing five closed forms, common-schedule fused/unfused parity, original MathNode comparison and poisoned-output completeness. Hardware is Apple M5/Metal, timestamps off.

## Actual input/reference path

The existing `ray_transport_cf4_trial.build(..., capture=True)` builds a scratch renderer/entrypoint, with `EC_CF4_BASELINE=1` so no CF4 integration runs. Sparse fragment instrumentation captures actual f32 origins, directions, clip endpoints and current raw transport at native 2560×1440 pixel centres. Inside, reverse, side and inside-zero-clock produce 2,304 valid volume-ray records; spawn produces 1,666. Uniform rank selection retains 192/case before candidate or reference evaluation. Inside's selected inputs and current outputs reproduce the original frozen capture exactly. This is instrumented raw transport before final pixel composition, not an uninstrumented full-image witness; discarded/missed volume fragments are not silently described as captured rays.

The original independent `veil_projected_reference.cpp` evaluates authored density/extinction/emission MathNodes and cumulative Simpson transport on a 65,536-interval grid, retaining the nested 32,768 result. Only its include path is relocated and unused legacy comparison columns suppressed. Five newly computed cases cover 960 ray/clock queries; the full original 2,304-ray reference is reused without changing values. Maximum fine/half disagreements across the new cases are T **8.1011e-8**, RGB **5.6394e-8**. These are finite convergence witnesses, not an exact integral oracle or universal GPU execution enclosure. Both resolutions must independently favor candidate under all six unchanged aggregate gates.

Every manifest pins the authored Zone, native-ray and reference bytes. Runtime verifies file identity before/after execution, original source remains pinned, failed children propagate, and independent raw recomputation checks row/pixel alignment and every reported metric. No ray's error determines sample selection. Different views have different ray sets; clocks 0 and 7.25 share the inside geometry but use their own emission/reference results. Authored root radiance remains exactly zero, so nonzero source lighting/shadows are outside this pass.

## The clock failure and bounded repair

The inherited prototype stores per-member `alpha`/RGB tables computed at 7.25. Density itself is static here, but emission includes a time-dependent pulse. Executing the frozen table at clock 0 yields original-AST RGB disagreement **0.0990796** and reference RGB mean/max **0.0119359 / 0.0990529**, failing both the AST and reference gates while T remains accurate. That is a concrete numerical refusal; its apparent throughput is not a qualified performance result. The earlier frozen 7.25 result remains valid in its stated scope.

[Authored-coefficient binding](/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop/scratch/probes/ray_transport_gpu_pose_authored_coefficients.py) admits only the existing mathematical shape:

`D = cD · B`, `X = cX · B`, `E = B · V(t)`.

`cD` and `cX` must be positive finite literal scalar leaves; original `B` subtrees must match structurally in all three channels; the existing `MathNode::collectDependencies` must show that `V` depends only on the explicit clock. The original MathNode evaluator evaluates the vector at that clock. The numeric backend receives `alpha=cX/cD` and `color=V(t)/cX`. No spatial samples infer proportionality, no lighting or authored expression is edited, and unknown shapes refuse. This removes the used clock-specific color table; fixed density-layout decoding/noise frequencies still prevent any claim of generic authored-source extraction.

Structural identity is an exact source condition; the numeric ratios, reassociated products and CPU time evaluation do **not** become a general native execution certificate. The retained numerical gates remain necessary. The five camera/clock cases show AST RGB disagreement <=3.43e-7 and T <=7.75e-7; the full original control shows RGB 3.13e-7 / T 8.94e-7. No future author/source/device/domain is admitted by these observations.

The actual helper is executed on CPU through 36 checks: all four original members bind and change with the clock; unshared factor subtrees, zero/negative/computed/nonfinite coefficients, scalar emission and spatial emission dependencies refuse. Native clock-zero/reference execution supplies the independent second path, rather than a CPU test merely agreeing with its own algebra.

**Lifetime/cost limit:** structural JSON comparison plus coefficient binding takes about 2.36–2.42 ms once at process setup for four members. It is outside stable GPU batches. This pass has no production cache or dynamic frame upload path; it does not prove that redoing this work each frame is affordable. Next separate source/revision admission from clock-only preparation and bind/upload through the existing backend machinery, then charge the real frame bill. Pointers are process-local and kept alive by their original field owners; no source/clock/device lifetime beyond this witness is promised.

## Outside-view compute adapter

Spawn initially refuses shader creation because the generated production marching body contains the fragment-only empty-support `discard`. Inside cameras did not emit this branch. The scratch adapter now converts **only** `if (supportEnd <= supportStart) { discard; }` to a raw identity write `(L=0,T=1)` followed by return. Unknown/duplicate fragment-only operations refuse. Every other marching operation remains generated production code. Metadata marks the body as adapted for spawn and retains `integration_body_verbatim=true` only when no conversion occurs.

The corrected spawn baseline reproduces captured native outputs and passes the independent reference gates. A separate five-ray manufactured downward query lies below every authored AABB (verified y minima 12,18,42,8 while query y runs 2→1). It executes the empty-support identity adapter and produces exact zero radiance/unit transmittance. Its reference is analytic emptiness, with resolution metadata 0/0; it is not counted as a captured camera/dense-reference case. Its tiny repeated workload is much slower (0.117×), so it is a correctness control **without** performance qualification. Empty-region/dispatch admission remains part of the production cost gate.

## Matched native costs — limited compute scope

Stable scene/clock/buffers, actual current generated marcher versus GPU production + full ordered A* + native fallback dispatch. CPU reference work finishes before qualified timing. All values below are completion-fenced **host** elapsed means, not GPU timestamps or fence-floor subtraction. Each batch has 512 successive dispatches on the same input/output addresses. The 192-ray cases therefore perform 98,304 invocations/batch; the full control performs 1,179,648. There is no free producer reuse across different concrete ray invocations. Cold decode/structural binding, compilation, uploads, opaque depth/raster/composition and frame lifecycle are outside the measured steady-state scope.

| Query | Distinct rays | Current batch ms | Connected batch ms | Ratio |
|---|---:|---:|---:|---:|
| Inside, 7.25 | 192 | 226.2721 | 126.9904 | **1.7818×** |
| Reverse, 7.25 | 192 | 164.5582 | 155.9198 | **1.0554×** |
| Reverse confirmation | 192 | 164.7001 | 156.5196 | **1.0523×** |
| Side, 7.25 | 192 | 217.3542 | 166.4919 | **1.3055×** |
| Spawn, 7.25 | 192 | 224.9771 | 147.1341 | **1.5291×** |
| Inside, 0 | 192 | 226.4228 | 124.5362 | **1.8181×** |
| Full original control, 7.25 | 2,304 | 251.4390 | 159.0327 | **1.5811×** |
| Full control confirmation | 2,304 | 251.5101 | 160.0778 | **1.5712×** |

All camera/control runs above have zero whole-ray fallbacks in this fixture. The fallback mechanism/forced controls remain from the preceding pass. The reverse view has only a small positive margin; the lower count cases and full control are different workloads. Earlier 1.48–1.50× evidence used 64-dispatch batches, so the 1.57–1.58× full-control result is **not** an isolated incremental speedup over that implementation. No FPS is extrapolated. Individual rays can still have worse error despite all aggregate gates passing; their counts/outliers remain in each machine report.

| Case | Candidate RGB max | Current RGB max | Candidate T max | Current T max |
|---|---:|---:|---:|---:|
| Inside, 7.25 | 6.112e-5 | 1.075e-3 | 8.885e-5 | 1.679e-3 |
| Reverse | 3.976e-5 | 6.829e-4 | 6.196e-5 | 1.124e-3 |
| Side | 1.460e-4 | 1.086e-3 | 2.275e-4 | 1.693e-3 |
| Spawn | 3.766e-5 | 8.903e-4 | 6.287e-5 | 1.514e-3 |
| Inside, 0 | 8.172e-5 | 1.526e-3 | 8.885e-5 | 1.679e-3 |
| Full control | 1.127e-4 | 1.958e-3 | 1.639e-4 | 3.025e-3 |

All mean and p99 gates pass too, at each reference resolution. Mathematical equations, four-unit single-panel cap, root work/capacity bounds, uncertainty model/margin and reference thresholds are unchanged. This pass broadens the evidence and removes a frozen-input assumption; it does not close generic source admission or a complete frame contract.

## Reproduction without repeating dense CPU work

On `claude/ray-domain-transport-rung0`, in the ray worktree:

```sh
python3 scratch/probes/ray_transport_gpu_pose_evidence.py --root . --record docs/audits/GPU_RAY_TRANSPORT_MULTI_VIEW_QUALIFICATION_2026-10-22.json --restore /private/tmp/earthcall-pose-restored-new
python3 scratch/probes/ray_transport_gpu_pose_authored_coefficients.py --root . --work /private/tmp/earthcall-pose-run-new --build-only
python3 scratch/probes/ray_transport_gpu_pose_authored_coefficients.py --root . --work /private/tmp/earthcall-pose-run-new --run-only --case /private/tmp/earthcall-pose-restored-new/inside-zero-clock/manifest.json --label clock-zero
python3 scratch/probes/ray_transport_gpu_pose_binding_test.py --root . --work /private/tmp/earthcall-pose-bind-test-new
python3 scratch/probes/ray_transport_gpu_pose_harness_test.py --root . --output /private/tmp/earthcall-pose-negative-new.json
```

Use fresh directories/output labels. The verifier independently recomputes all ten native raw records, including preserving the frozen-color refusal, checks captures/source/archive hashes, and restores seven valid external case manifests. The thirteen failure controls cover changed files/source, relative paths, invalid/nonfinite/misordered/duplicate inputs, missing pixel coverage and a manufactured case where the fine reference favors candidate but half reference favors current; fine-only success never becomes broader qualification.

The capture/reference helper can rebuild the data with explicit external native capture/reference executables; their exact generated sources and logs are archived. No broad suite/app rebuild is claimed. Initial namespace compile failures were corrected before qualification; the spawn stage restriction is preserved and corrected as described. No production code or authored save was changed.

## Standing distance account and next smallest step

The bounded multi-pose/two-clock numerical gate has advanced. The original transport expressions now supply admitted time coefficients; the prototype's hardcoded 7.25 pulse no longer governs the new case runner. This is still an isolated pinned-source experiment, with finite current-or-better accuracy and view-dependent native throughput.

Before a shipped checkpoint: derive/refuse the remaining density/root recipe semantically under OntoMath, preserve original execution provenance, and give reusable structural admission plus clock preparation/upload explicit source/revision/device lifetimes and costs. Then qualify bounded-memory **unique** native viewport/depth/composition and complete frame cost (including empty regions), followed by inhabited movement/p95 and Zach's appearance acceptance. The padded diagnostic remains 1,328 bytes/ray; naive full-resolution replication is still 4.90 GB. No new native omission certificate is claimed; unknowns retain baseline fallback. The [existing Person movement/appearance checks](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md) remain open; this pass creates no new app-visible behavior or control to judge.

Ownership stays mathematical: OntoMath owns factor/dependency/derivation meaning; Screen instantiates backend execution. The scratch structural matcher is not a new renderer definition of aurora behavior. Suggested next pass: promote the smallest reusable mathematical recipe and prepared coefficients with refusal/lifetime witnesses, then measure the actual bounded-memory viewport bridge. No need to solve the entire certification campaign before Zach's explicitly accepted numerical checkpoint, but do not substitute repeated sparse throughput for the missing frame result.

All probes are finished and the native lane is released at signed closeout. No push/PR or app activation. Default receives signed evidence/Intercom relays only; source implementation remains on the ray branch.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T08:14:35.094887-07:00`.
