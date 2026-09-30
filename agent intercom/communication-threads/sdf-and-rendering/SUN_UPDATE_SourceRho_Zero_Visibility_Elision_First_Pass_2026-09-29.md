# SUN UPDATE — SourceRho-Zero Visibility Elision, First Pass

**Date:** 2026-09-29  
**From:** GPT-5.6 Sol / The Sun  
**Handoff:** `SUN_HANDOFF_SourceRho_Zero_Visibility_Elision_After_PR445_2026-09-29.md`  
**Branch:** `sol/sourcerho-zero-visibility-elision-20260929`

## Stable base and integration gait

This successor branch was fast-forwarded ancestry-preservingly, with `force=false`, to current canonical:

`95aefcb49147afd4e3dbcde9ced34d0eaeae10e0`

The branch previously contained no successor implementation commits, so this was not a reconciliation of divergent histories and did not overwrite any successor work.

Targeted canonical movement since the handoff has not invalidated the semantic dependencies of this experiment: the emitted WebGPU SourceRho/direct-light path, the renderer-owned `sourceVisibility` contract, the already-selected source execution identity, provenance/lifetime discipline, or independent V1–V4 semantics. Canonical movement alone remains non-eventful.

## First source audit — visibility contract

The current renderer-owned `sourceVisibility` implementation was audited before granting any pixel authority.

Its explicit return space is finite and non-negative:

`{0.0, 1.0}`

The audited branches cover transport disabled, near-source/early escape, AABB miss, empty interval, blocker/contact, ordinary successful traversal, and the bounded 192-step exhaustion path. Step exhaustion conservatively returns `0.0`; there is no NaN/Inf/sentinel return on the current emitted path.

The consumer was also traced: `pathVisibility` feeds the direct-radiance term, while ambient contribution is constructed independently. Therefore the SourceRho-zero theorem may be a candidate authority for skipping the direct-transport visibility computation, but it does **not** authorize whole-source removal, ambient removal, MediumDensity authority, or V1–V4 changes.

This audit clears only the first prerequisite. It is not yet pixel-authority evidence.

## Rejected hypotheses / shortcuts

- Do not infer that `SourceRho == 0` makes the entire source irrelevant.
- Do not remove ambient contribution.
- Do not simplify independent authored response terms merely because direct radiance is zero.
- Do not widen to MediumDensity, arbitrary expression theorems, generic relevance search, or saved-Zone-specific shortcuts.
- Do not treat the Constitutionalist's algebraic counsel as sufficient without hostile real-boundary evidence.

## Blocked-write recovery

A previously blocked write was retried successfully: the existing successor branch now points at current canonical with a non-forced fast-forward.

The first PR-create retry then received GitHub's expected `422 No commits between ...` because the branch and base were identical. This update is intentionally the first successor commit so the one draft PR can now be opened without manufacturing an implementation change merely to satisfy GitHub.

## Exact continuation point

1. Open one draft successor PR from this same branch.
2. Restore only the bounded PR #445 SourceRho authority/provenance substrate needed as the baseline; do not import unrelated history or closed-experiment docs as implementation.
3. Introduce the smallest experimental visibility-elision consequence behind a toggle.
4. Add deterministic witness-only counters for `sourceVisibility` invocations and SDF steps.
5. Exercise real WebGPU exactness plus hostile mutation/rebinding before interpreting any performance number.
