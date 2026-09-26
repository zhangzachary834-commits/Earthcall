# SUN POST-V5 CONSTITUTION — Null-participant sampling stability

Date: 2026-09-24
Owner lane: GPT-5.6 Sol
Base inspected: `sync-from-earthcall-main` at `a46ae6c9eeb7ccc134c57f535d02cea17c02044e`
V5 landing: merge commit `a46ae6c9eeb7ccc134c57f535d02cea17c02044e`, PR #343 merged.
Branch: `sol/post-v5-null-participant-stability-20260924`

## Why this is the first post-V5 frontier

V5 is landed and its fused transport semantics are closed. A fresh targeted audit of the landed architecture found one concrete numerical seam that is narrower than GI, multiple scattering, spectral transport, scene-wide visibility, or multi-source in-scattering:

`compileVolumeSet(...)` partitions a ray at every admitted medium AABB event and assigns a fresh midpoint sample grid to each occupied interval. A semantically null medium B (zero density/extinction/source) whose bounds lie *inside* a spatially varying medium A can therefore split A's integration interval even though B contributes no physics. The physical integral is unchanged, but the numerical quadrature grid changes.

The independent 2026-09-24 radiance audit demonstrated this with a CPU midpoint surrogate (about two linear 8-bit levels for one narrow-emission placement), explicitly not yet as a native pixel failure. That is sufficient evidence for a bounded proof rung, not sufficient evidence for a production rewrite.

## Invariant to test

Adding, removing, or moving a semantically null participating-medium member must not create a perceptible change in an already-authored non-null medium solely by repartitioning its numerical sample grid.

This is deliberately *not* a bit-identity requirement. Numerical integration may differ within a measured tolerance. The witness must measure native pixel error and fragment-work consequences rather than assuming either exact equality or failure.

V5 laws remain sovereign:

- one fused transport state for 2+ participating media;
- combined extinction/source law remains unchanged;
- medium order is not physics;
- distant vacuum must not dilute local sample density;
- V0–V4 single-medium compatibility remains unchanged;
- no stale output after refusal;
- numeric/time/membership invalidation semantics remain unchanged.

## First falsifying witness

Construct a native WebGPU witness with:

1. medium A alone, with a narrow spatially varying `E_v` (or another lawful sharply varying authored channel) so quadrature movement is observable;
2. the same A plus medium B whose AABB lies partially inside A but whose authored participation is identically null;
3. move B through at least several positions within A;
4. compare the same camera pixel(s) against A-alone;
5. report maximum RGB byte error and relevant renderer/fragment-work telemetry if available;
6. retain the existing V5 A/B permutation, closed-form overlap, and far-zero-medium witnesses.

The proof rung passes if error is bounded below a justified perceptual/numerical tolerance without pathological work growth. It falsifies the present sampling policy if the null member creates visible/material pixel drift or unacceptable work amplification.

## Production change gate

Do **not** alter `compileVolumeSet(...)` merely because the surrogate predicts drift. First obtain the native witness. If native evidence falsifies the policy, choose the smallest stable/error-controlled local sampling rule that:

- retains one shared fused transport state;
- does not return to a fixed grid over the whole union vacuum span;
- does not make null membership into physical law;
- does not silently truncate media;
- preserves V5's existing witnesses.

## Explicit non-goals

This bounded frontier does not implement:

- GI/path tracing;
- multiple scattering;
- spectral transport;
- scene-wide Rung-8 visibility;
- plural incident-source scattering;
- new renderer-owned Medium/Light nouns;
- the separate per-frame AST serialization economics task;
- generalized shader-membership scaling limits except insofar as the native null-member witness reports work.

## Exact continuation point

Add the native A-alone versus A+partially-overlapping-null-B witness first. Run it against the landed V5 implementation without changing production transport. Record pixel delta and work. Only if that witness fails the invariant should the next pass modify sampling policy.

This constitution is intentionally a proof-first rung: the repository contains a concrete suspected numerical defect, but not yet a native failure. The next Sun must earn any production rewrite from that evidence.

— GPT-5.6 Sol