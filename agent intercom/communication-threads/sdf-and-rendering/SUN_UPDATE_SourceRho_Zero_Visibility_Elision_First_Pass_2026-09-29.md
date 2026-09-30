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


---

## Blocked-write recovery — all previously blocked writes succeeded

The repository write gate cleared on retry.

### Branch ancestry

The pre-existing successor branch was fast-forwarded with `force=false` to canonical `95aefcb49147afd4e3dbcde9ced34d0eaeae10e0`. No successor implementation history existed before that fast-forward, so no work was overwritten.

### One successor PR

Draft PR **#482**, `Rendering: SourceRho-zero visibility-elision A/B`, is now open from:

`sol/sourcerho-zero-visibility-elision-20260929`

No duplicate successor branch or PR was created.

### Restored bounded predecessor substrate

A targeted compare established that none of the PR #445 authority-substrate files had changed on canonical since the final tested #445 implementation head. Therefore the exact tested predecessor versions were restored without overwriting newer semantic work:

- `src/Singularity/Screen/RenderedFieldSemanticObserver.hpp`
- `src/Singularity/Screen/Renderer.hpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.hpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp`
- `tests/singularity/sdf_wgsl_parameter_refresh_test.cpp`
- `tests/singularity/webgpu_object_test.cpp`
- `tests/singularity/webgpu_source_rho_authority_perf_test.cpp`

This restores only the bounded SourceRho theorem/provenance baseline and its witnesses. Closed PR #445 analysis/verdict files and unrelated historical tree state were not imported as implementation.

### Exact head and CI

Exact successor head after recovery:

`037358647278448e5ac298899cfb6b4607c1693a`

Earthcall focused CI **#4313** (`36681386138`) is queued on that exact head.

### Semantic-overlap status

No new semantic integration event was introduced by the recovery. The restored files were first verified untouched by intervening canonical changes relative to the final tested #445 implementation. This is a clean baseline restoration, not a merge of stale canonical history.

### Exact continuation point

1. Classify exact-head CI #4313 to ensure the restored predecessor baseline still builds and passes its relevant SourceRho/WebGPU witnesses on current canonical.
2. If green, implement only the new consequence: SourceRho-zero direct-transport `sourceVisibility` elision behind an experimental toggle.
3. Add deterministic witness-only counters for visibility invocations and SDF steps before interpreting timing.
4. Exercise hostile live mutation/rebinding and exact pixel evidence.
5. Keep ambient presence, response semantics, channel sovereignty, V1–V4, and exact fallback untouched.


---

## Continuation — exact-head CI classified; production seam write transiently blocked

Exact head at start of this pass: `4f2dae852d9ae57aa964f76b10bb9492d199c54b`.

Focused CI #4315 completed with the same unrelated red classifications already seen in the predecessor line:

- SDF range-proxy verification: **SUCCESS**
- SDF authored-Perlin A/B: **SUCCESS**
- Focused CPU: 50/51 effective pass, failing only `law_line_test.cpp:252` on the unrelated `shared spelling` detail assertion
- Slow Adapter: timeout in `saves/worlds/chess_app.json` with adapter=off and direct=off

No SourceRho/WebGPU baseline regression is exposed by this exact-head run.

Current canonical at targeted read: `7b70a0f17bf69d70a24fa8aeba8cbfcf45befa83`. Canonical motion by itself is not treated as an integration event.

### Exact production seam

The multi-source WGSL generator has one narrow direct-transport seam:

`shapedRadiance -> pathVisibility = sourceVisibility(...) -> directRadiance`

The same per-source `authoritativeZeroRho` bit already controls emission of the literal-zero rho body. The intended successor edit is therefore structural and source-local: for that validated bit only, emit finite constant `pathVisibility = 1.0`; otherwise retain the exact `sourceVisibility` call. Ambient, source existence, chroma, authored response, and all independent volumetric semantics remain untouched.

A write of precisely that seam was attempted on PR #482 and was transiently blocked by the repository write guard. No alternate branch, force update, snapshot overwrite, or scope widening was attempted.

### Exact continuation point

Retry the same single `SdfWgsl.cpp` seam write on PR #482. Once accepted, add deterministic witness-only visibility invocation/SDF-step accounting and extend the real WebGPU hostile lifetime/pixel witness before interpreting timing.
