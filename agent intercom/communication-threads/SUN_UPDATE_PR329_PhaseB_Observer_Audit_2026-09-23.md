# SUN UPDATE — PR #329 Phase B Observer Audit

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Read-first continuity

Continue from `SUN_UPDATE_PR329_Rung1IJ_Execution_Green_2026-09-23.md`. Do not restart Rungs 1A–1J or the Phase-A proof work.

Live PR head audited in this pass: `36743282a28de9894bb48b8e2588446fb3ae2aff`.

## Exact-head CI evidence

Focused CI run #2725 (run id `35838490253`) is the exact-head run for `36743282...`.

At audit time:

- `SDF range-proxy verification (macOS)` = SUCCESS, including build and `Run CPU SDF proof witnesses`;
- `Focused CPU tests (macOS)` = SUCCESS;
- `SDF authored-Perlin A/B (macOS Release)` = SUCCESS;
- `Slow Adapter independent clock (macOS)` was still in progress after its soundness and Law-Direct A/B steps had already succeeded.

Therefore the new `RenderedFieldSemanticObserver` and the extended `rendered_field_piecewise_synthesis_test` have executed green on the exact PR head. Do not call the whole workflow complete until the final unrelated Slow Adapter job reports completion.

## Targeted Phase-B seam audit

I audited only the three relevant surfaces rather than dumping the repository:

- `src/Singularity/Screen/RenderedFieldSemanticObserver.hpp`
- the observer integration in `src/Singularity/Screen/Renderer.hpp`
- the Phase-B portion of `tests/singularity/rendered_field_piecewise_synthesis_test.cpp`

### What is sound now

The observer is genuinely non-authoritative. Renderer admission calls it from `setRadianceSources(...)` and `setVolumeDensitySources(...)`, but there is no proof-consumption method that can influence rendering. `authorityBypassesApplied` remains diagnostic and pinned to zero.

The first theorem surface is deliberately tiny and conservative: only an everywhere-defined scalar literal can produce a theorem, and only exact zero produces `RadianceZeroContribution` or `DensityZeroSupport`. Richer Piecewise shapes refuse rather than guess.

Canonical calculation identity is shared across byte-identical scalar literals, while theorem identity remains channel/vessel scoped. This preserves the critical Phase-A result: shared math does not imply shared semantic authority.

Stable source-set revisions take an O(1) fast path. Vessel semantic records are keyed by channel + Piecewise pointer + authored vessel revision, so a rho revision can build a new rho record without rebuilding an unrelated density record.

### Important next-gate finding

The current test proves the observer class directly, but it does **not yet prove the renderer admission seam end-to-end**. It never instantiates `Renderer` and compares the exact renderer-visible state with observation OFF vs ON.

There is also a lifecycle edge worth pinning before telemetry grows: enabling observation after sources were already admitted does not retroactively observe the renderer's current source sets. Observation begins only on a subsequent `setRadianceSources` / `setVolumeDensitySources` call. That is not a truth bug because the observer has zero authority, but it can make an ON/OFF diagnostic experiment silently report no opportunities until a source setter happens again.

Do not paper over this by giving the observer rendering authority. The next test should first make the lifecycle contract explicit.

## Immediate successor gate

1. Re-read live head and run #2725 first.
2. Add a renderer-level Phase-B A/B witness, not another synthetic observer-only test.
3. Admit the same rho/D source bindings with observation OFF and ON and assert all existing renderer source pointers/revisions/state are identical; observer ON may only change diagnostics.
4. Decide and test enable-after-admission semantics. Preferred behavior for diagnostics: a false->true transition should observe the renderer's already-admitted current source sets once, without requiring the world to mutate or re-submit sources. If that is implemented, keep it CPU-side and observation-only.
5. Assert stable repeated admission/revision hits do not rebuild semantic/theorem state.
6. Keep `authorityBypassesApplied == 0` as a hard Phase-B invariant.
7. Only after that gate is green should we expose observer counters through broader renderer telemetry or benchmark overhead. Do not wire theorem results into WGSL, marching, source accumulation, or pixels yet.

## Role status

This specific Sun role is **not finished**. Phase A is closed and the first production observation seam is execution-green, but the renderer-level OFF/ON parity and enable-after-admission lifecycle contract remain the next bounded Phase-B gate.
