# Ray-domain transport: source review of Opus's implementation

**Scope:** `claude/ray-domain-transport-rung0`, HEAD `ee905784589ed42626c61c0c2e6fa67c39657ec3`, rendering changes from `d9cd72aa`. Zach explicitly requested this review, then instructed **no tests**, because another Sun is doing live validation. **No tests, builds, probes, benchmark or app launch were performed. No implementation was changed.** Findings below are source/arithmetic deductions, not freshly observed pixels.

This reviews the current later rungs as well as Rungs 0/1, continuing [Opus's handoff](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md) and [Rung 2's current native scope](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6e-rung-2f-native--the-gpu-certificate-tracer-opt-in-2026-10-17-sixth-pass). The architectural direction follows Zach's ray-transport canon: lower mathematics once per authored revision and retain numeric ray/clock bindings at the shorter lifetime.

## P1 — Simplified real identities are being used as exact execution-zero proofs

**Primary location:** [LineRestriction.cpp:174](../../src/Singularity/OntoMath/LineRestriction.cpp#L174), particularly `a->scalar.plus(b->scalar.scaled(sign))`. `ScalarForm::normalized` merges like coefficients and removes canceled terms. `exactDegreeOn` and the later range machinery only see that simplified result. They have no record of the original expression's rounding/cancellation. `planVolumeRaySegment` then selects ExactEmpty at `VolumeRayTransport.cpp:350`; algebraic support certificates use the same transformed polynomial leaves at `VolumeRayTransport.cpp:832`.

A concrete MathNode Add/Sub tree, **not a precombined ScalarLeaf**:

```text
A = 2^54
F(x) = ((A + x) - A) - x
D(x) = -F(x)
```

Restriction cancels F identically to zero. At x=1, the original scalar double evaluation rounds A+1 to A, so F=-1 and D=1. The Rung 1 planner instead sees a constant-zero density and authorizes ExactEmpty. The corresponding separated f32 operations also lose the +1, although a native compiler's reassociation must be checked by the other Sun. This disproves the exact-empty claim against the existing scalar evaluator by arithmetic inspection; it is more than a small accepted radiance rounding difference.

The GPU certificate route has the same missing premise. Use `D=-min(F,0)` (`Scale(-1, Union(F,0))`). The algebraic F leaf is zero; forcing the min switch's negative side proves zero density with zero mathematical Lipschitz rate and essentially zero argument magnitude. The emitted candidate evaluates the original F tree. Wherever that evaluates to -1, the negative-side margin is positive while the actual density is 1. The small declared tolerance cannot account for operations that disappeared before its magnitude calculation. This is a proposed native manifestation, **not an executed native result**.

**Correction:** preserve and enclose the original evaluation DAG's error/domain through lowering, or restrict omission eligibility to transformations with a demonstrated execution-zero equivalence. A real-algebra identity may remain useful as a semantic form while omission falls back. Estimating errors only when evaluating already-simplified coefficients cannot restore canceled intermediate information. Increasing the global tolerance from the reduced magnitude is insufficient.

## P1 — Clamp certificates need the shader's bound, not a separately folded CPU bound

**Primary location:** [WebGpuRenderer.cpp:2607](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp#L2607), `h[3] = static_cast<float>(c.offset)`. `AlgebraicBuilder::constantOf` folds the authored bound in host double arithmetic (`LineRestriction.cpp:607`); the certificate candidate compares its operand with that stored result (`SdfWgsl.cpp:4274–4276`). The actual density's `emitMathNode(Clamp)` separately emits the original lower/upper operand trees (`SdfWgsl.cpp:783`). Casting the folded answer to f32 does not make it equal to evaluating the tree in f32.

Concrete authored bound trees:

```text
B = 2^25
lo = B - (B - 1)
D(x) = 0.5 - clamp(x, lo, 1)
```

Host double computes lo=1. The producer can prove D=-0.5 on the lower saturated branch, with ample outward-rounding slack. Under separated f32 arithmetic, B-1 rounds to B, so the shader computes lo=0. At x=0, the actual density is then +0.5, while the certificate compares x to its stored offset 1 and can skip it as empty. In a small box such as [-2,2]^3, the declared magnitude-scaled tolerance is only a few thousandths, far below the one-unit threshold discrepancy. No large spatial coordinates are needed. This f32 instruction-sequence deduction is **not a measured Metal/lavapipe result**; backend optimization and the live manifestation belong to the other Sun's validation.

**Correction:** require shader-equivalent bounds (initially, eligible literal bounds are a conservative subset), or lower and certify the original bound expression under the actual backend arithmetic with its error. Keep the candidate disabled when equivalence is unknown. This is independent of an inaccurate derivative: even a valid derivative for x does not repair the wrong threshold.

## What is useful, and what remains conditional

The revision/ray lifetime separation is visible in the source, caches are explicitly Kernel-derived, and unsupported forms have named fallbacks. Source/source-clock/occluder domains remain distinguished. The multi-term ScalarLeaf parentheses repair in `emitScalarForm` is locally sound by inspection: a leaf's sum must remain one operand under Sub/Scale. Its intentionally changed authored appearances and Opus's recorded tests remain dated evidence, not rerun evidence from this review.

The GPU certificate lane defaults off. Keep it off while the omission premises above and the declared-but-unproved device tolerance remain unresolved. The latter is openly documented by Opus and is a remaining qualification risk, not a freshly reproduced third bug. Existing CPU count reductions and lavapipe timings do not establish native Metal correctness or Northern Veil FPS. No recommendation to undo the entire ray-domain architecture follows from these two findings.

## Handoff to the Sun doing live validation

Use the exact authored MathNode trees above; a host-side pre-simplification would erase the counterexamples. For the first case inspect restricted coefficients/strategy as well as the retained CPU evaluator. For the native min wrapper and computed-bound clamp, compare against the same backend's certificate-off accepted lattice and record generated WGSL/header offsets; isolate single/fused and grid off/on. Preserve default-off and the independent reference. These are proposed checks only: **none were run here**. AD-30/36 and existing Person appearance/movement acceptance remain open. No saves, installed app, implementation, commits or pushes changed.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-16T22:35:31.712804-07:00 (PDT).
