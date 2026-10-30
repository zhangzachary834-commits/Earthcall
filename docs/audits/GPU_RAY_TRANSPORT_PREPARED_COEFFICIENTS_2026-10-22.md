# Ray transport — owned mathematical factors and changing-clock GPU preparation

**Default-checkout evidence relay:** implementation is on local `claude/ray-domain-transport-rung0`, commit `f590ae1e2`; new source links below point to that ray checkout. This relay does not install the implementation or publish it remotely.

Zach commissioned another pass after [§6cj's multi-view checkpoint](GPU_RAY_TRANSPORT_MULTI_VIEW_QUALIFICATION_2026-10-22.md). His direction remains current-or-better authored accuracy with useful native performance, and mathematical meaning under OntoMath. This continues the [existing council thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md), from local ray checkpoint `b7c35e4b9`.

**Outcome:** the shared-factor derivation now has a reusable OntoMath office and owns its original source snapshots. The scratch GPU consumer separates source admission from changing-clock preparation. The registered mathematical test passes 33 checks; actual four-member source/refusal/revision checks pass 52, including 257 changing bindings per member that produce exactly the previous binder's float coefficients. A same-device five-frame clock/fallback witness and the original 2,304-ray control pass unchanged numerical gates. This advances a compiler/preparation component; it changes no production rendering, default, save, data home or installed app. No FPS claim, push or PR.

## Mathematical ownership and refusal

[`CommonScalarFactor.hpp`](/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop/src/Singularity/OntoMath/CommonScalarFactor.hpp) admits original binary Scale expressions sharing a structurally identical scalar child: `f_i = B · c_i`. It accepts scalar or vec3 coefficients and an explicit type environment plus admitted coefficient symbols. The test uses `theta`, rather than a built-in clock: OntoMath owns neither a Timeline nor a camera. Negative and computed coefficients can be valid mathematics; the diagnostic transport consumer separately retains its positive-literal policy. Unknown, malformed, undeclared/mistyped, nonfinite, impure/random, scoped/unsupported and over-budget forms refuse. No sampling or algebraic cancellation infers source identity.

The result contains owned factor/coefficient/original-expression trees, original operand positions and coefficient dependencies/types. It remains an explicitly historical value after source mutation/destruction; changed source/binding revisions require fresh admission. This is an ordinary mathematical IR value, not a new kind of being or a persisted opcode. There is no global cache, source listener, source revision key or GPU handle inside it. `evaluateFactorCoefficients` invokes the original coefficient evaluator with explicit finite bindings, without evaluating the shared factor or repeating source JSON/type matching. It does not certify GPU reassociation, divisions, rounded constants or omissions.

[`ray_transport_gpu_prepared_coefficients.py`](/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop/scratch/probes/ray_transport_gpu_prepared_coefficients.py) is a real consumer of that office. Screen's diagnostic still refuses computed/nonpositive scalar coefficients, non-vector/spatial emission coefficients and unusable backend conversions. Fixed-offset density decoding and per-member noise frequencies remain fixture-specific. **Only common-factor derivation has become reusable; generic density/root/source admission has not landed.** The compiler header hash is part of generated source replay identity, so an edited mathematical implementation cannot silently reuse an older binary.

## Cost: admission once, preparation per binding

Four alternating-order rounds of 64 changing bindings per member measure **2.31086 ms** median across all four members when repeating the old structural binder, versus **0.01685 ms** with owned recipes and coefficient-only preparation. Four-round prepared totals span 0.01663–0.01697 ms. Checksums consume results; 1,028 additional clock/member comparisons independently reproduce the old float coefficients. This is optimized host CPU preparation cost, excluding uploads and GPU work; it is not a renderer speedup.

The new cold derivation is **15.99–16.15 ms** across four source trees, including bounded validation, type judgement, exact identity comparisons and owned provenance copies. That is more expensive than the previous ~2.4 ms setup. Repeated full-tree type judgements initially cost ~51.5 ms; the final derivation judges the shared subtree once and separately judges its coefficients. It still must be kept off the per-frame/per-pixel path. Source/revision caching, edit-stutter policy and production ownership/invalidation are not implemented. First-touch preparation observations (0.041–0.119 ms) also differ from warmed loop medians; the machine record preserves both.

## Same-device clock update, return and native fallback

[`ray_transport_gpu_clock_lifecycle.py`](/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop/scratch/probes/ray_transport_gpu_clock_lifecycle.py) reuses the same owned recipes, shader pipelines, device and existing buffers across:

1. Clock 7.25, normal GPU producer/transport.
2. Clock 0, normal GPU producer/transport.
3. Clock 7.25 again, reproducing frame 1 exactly.
4. Clock 0, forcing every ray through the actual native fallback.
5. Clock 7.25, forcing every ray through the actual native fallback.

Each frame prepares coefficients, updates both candidate and native instance/source clock payloads, runs the native clock-preparation shader and executes connected producer/transport/fallback. Outputs are poisoned first. All 192 results are written in every frame. Normal frames have zero fallback; each forced frame has 192 flagged native fallbacks and output exactly equal to the current native marcher at that clock. Independent native capture agreement is <=2.99e-8; clock-zero current reproduces its capture exactly. All six mean/p99/max numerical gates pass against both 65,536/32,768 references in every frame, including equality for the forced baseline path. The references and thresholds are unchanged. Return-clock output is exactly repeatable, while changed-clock RGB changes.

The first version reused the ray helper to dispatch native clock preparation. That shader has workgroup size 1 and needs **one group per medium**; the ray helper issued three groups for 192 rays, refreshing only three curtains. The stale crimson coefficient caused max native-capture disagreement **0.0854224** at clock zero. The wrapper refused it. That native report and exact source are preserved as expected failures. The corrected dispatch submits four groups and passes. An initial C++ header-field spelling error was fixed before this native witness. The raw verifier was also split from saved-summary comparison so each replay frame is compared to its own measured current baseline and clock-specific reference, rather than the primary frame's saved statistics; existing saved-summary checks remain mandatory for primary records.

Observed prepare/upload/native-preparation/connected-completion host costs are 3.113–3.123 ms on normal sparse frames and 1.576–1.585 ms on forced fallback frames. Correctness readbacks and poison writes are outside those intervals; the last buffer upload is still followed by the normal completion fence. These five diagnostic observations are **not** stable frame economics or a native GPU timestamp. They include 864 bytes/frame of candidate/native clock, member, instance and diagnostic configuration writes, and a native preparation dispatch. No raster, opaque depth, composition or unique full viewport is included. In particular, sparse single-query fence/dispatch cost remains real.

## Preserved accuracy and throughput boundary

The final-source full 2,304-ray control passes original AST agreement, five closed forms, common-schedule device parity, complete output and all six gates at both independent reference resolutions. The same 512-dispatch completion-fenced protocol measures **1.574×** connected throughput, consistent with the preceding 1.57–1.58× checkpoint. The 192-ray primary clock run measures 1.818×. This pass changes coefficient ownership/preparation, not the transport kernel; neither number is a new incremental kernel gain or shipped FPS. The four-view evidence remains §6cj's bounded evidence; no additional camera validity is inferred here.

[Machine record](GPU_RAY_TRANSPORT_PREPARED_COEFFICIENTS_2026-10-22.json) pins 16 compressed source/raw/control archives. The offline verifier rechecks two primary records and five per-clock records, exact source/header provenance, and eight lifecycle refusals: missing frame, wrong sequence, stale current, nonfinite output, inexact fallback, missing forced fallback, inexact return and the preserved native partial-preparation failure. The preceding 13 external-case guards also pass after raw-metric extraction. The actual mathematical CTest and original-source helper have separate CPU paths; native references remain independent of their factor derivation.

## Reproduce and continue

From the ray branch, first restore the preceding audit's fixtures using its verifier, then:

```sh
ctest --test-dir build -R '^ontomath_common_scalar_factor_test$' --output-on-failure
python3 scratch/probes/ray_transport_prepared_evidence.py --root . --record docs/audits/GPU_RAY_TRANSPORT_PREPARED_COEFFICIENTS_2026-10-22.json --fixtures /private/tmp/your-restored-pose-fixtures
python3 scratch/probes/ray_transport_prepared_coefficients_test.py --root . --work /private/tmp/new-prepared-cpu
python3 scratch/probes/ray_transport_gpu_clock_lifecycle.py --root . --work /private/tmp/new-clock-replay --build-only
python3 scratch/probes/ray_transport_gpu_clock_lifecycle.py --root . --work /private/tmp/new-clock-replay --run-only --case /private/tmp/your-restored-pose-fixtures/inside/manifest.json --paired-case /private/tmp/your-restored-pose-fixtures/inside-zero-clock/manifest.json --label clocks
```

For a new build directory, configure/build the named CTest target using the [existing build recipe](../BUILD_AND_ENVIRONMENT.md); no dependency policy is introduced here. Use fresh scratch paths. No broad-suite result is claimed.

**Standing shipping distance:** reusable shared-factor ownership and a bounded same-device clock/upload/fallback witness now exist. Remaining: generic density/root/factor execution recipes with original provenance/refusal; production source/revision/device admission and lifetime, including edit costs; bounded-memory **unique** native viewport with opaque depth/composition and complete costs (including empty regions); then moving inhabited p95 and Zach's appearance acceptance. The diagnostic's padded 1,328 bytes/ray still implies 4.90 GB if naively replicated at 2560×1440; this pass does not solve it. A finite numerical checkpoint remains distinct from universal omission/quadrature authority. Unknowns keep the current marcher. Existing Person checks remain open; no app-visible change is introduced.

All probes finished; native lane released at closeout. Implementation stays on the local ray branch; default receives evidence and Intercom relays only.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T10:25:19.648159-07:00`.
