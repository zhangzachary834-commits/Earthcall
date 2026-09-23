# SUN UPDATE — PR #329 Phase B Observer Audit

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Read-first continuity

Continue from `SUN_UPDATE_PR329_Rung1IJ_Execution_Green_2026-09-23.md`. Do not restart Rungs 1A–1J or the Phase-A proof work.

## Exact-head CI evidence

Current PR head audited in this successor pass: `49ee9d90eb743ea5b829e4644ecf65e8ffbb83f7`.

GitHub check-runs for this exact SHA now show the previously outstanding jobs completed successfully, including:

- `Focused CPU tests (macOS)` = SUCCESS;
- `SDF authored-Perlin A/B (macOS Release)` = SUCCESS;
- `Slow Adapter independent clock (macOS)` = SUCCESS.

Therefore the prior uncertainty about the unrelated Slow Adapter tail is closed. The Phase-B observer head is execution-green at the workflow level.

## Targeted Phase-B seam audit

I re-read only the relevant surfaces rather than dumping the repository:

- `src/Singularity/Screen/Renderer.hpp` observer admission/setter seam;
- `src/Singularity/Screen/RenderedFieldSemanticObserver.hpp` via the existing Phase-B diff/context;
- the Phase-B observer assertions in `tests/singularity/rendered_field_piecewise_synthesis_test.cpp`;
- exact-head PR/check state.

### What remains sound

The observer remains genuinely non-authoritative. `setRadianceSources(...)` and `setVolumeDensitySources(...)` merely submit admitted bindings to diagnostic observation. No observer theorem result is consumed by rendering control flow. `authorityBypassesApplied == 0` remains the constitutional Phase-B invariant.

Stable source-set revisions still have a bounded observer fast path; authored vessel revisions are distinct from source-set revisions; canonical math sharing does not merge channel/vessel theorem authority.

### Lifecycle gap confirmed at renderer boundary

The exact renderer setter currently does only:

`_renderedFieldObserver.setEnabled(on);`

Thus this concrete sequence is still diagnostic-incomplete:

1. admit radiance/density source sets while observation is OFF;
2. flip observation OFF -> ON;
3. do not mutate or re-submit the world;
4. query observer telemetry.

The renderer already owns the admitted `_radianceSources`, `_radianceSourcesRevision`, `_volumeDensitySources`, and `_volumeDensitySourcesRevision`, but the false->true transition does not submit them. Telemetry therefore remains empty until a later source setter call.

This is not a rendered-truth failure because the observer has no authority, but it is a real observability/lifecycle defect: an A/B experiment can enable diagnostics against an already-live world and silently miss the current scene.

## Concrete next implementation gate

Do not broaden theorem algebra or touch WGSL. The next code change should be exactly at the renderer seam:

- make `setRenderedFieldSemanticObservationEnabled(true)` detect a false->true transition;
- after enabling, immediately observe the renderer's already-admitted radiance and density source sets using their current revisions;
- repeated `true -> true` must not resubmit/rebuild;
- `true -> false` must only disable observation, never mutate renderer source state;
- preserve source vectors, source revisions, rendering state, and `authorityBypassesApplied == 0` exactly;
- add a renderer-level A/B/lifecycle witness proving OFF and ON expose identical renderer truth while only diagnostic counters differ;
- pin stable repeated admission/revision hits as no semantic/theorem rebuilds.

Important subtlety: retro-observation should happen only on the false->true edge. Calling the observation methods on every `set...Enabled(true)` would turn an idempotent control setter into extra observer work and muddy the economics.

## Why this pass did not widen production code

The live branch has accumulated 47 commits and 19 changed files, but the current bounded defect is only the enable lifecycle. Exact-head CI is green. There is no CI failure justifying opportunistic churn, and no evidence that renderer truth itself is wrong. The safest next mutation is therefore the tiny lifecycle edge plus an end-to-end renderer witness, not another proof/compiler rung.

## Role status

This specific Sun role is **not finished**. Phase A remains closed and the first production observer seam is execution-green. The remaining bounded Phase-B gate is now sharply isolated: implement and test false->true retro-observation plus renderer OFF/ON truth parity. After that gate is green, broader telemetry/overhead measurement can be considered; theorem consumption must still remain out of scope.
