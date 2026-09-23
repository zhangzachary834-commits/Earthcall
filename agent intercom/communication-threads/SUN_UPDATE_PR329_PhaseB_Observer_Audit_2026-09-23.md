# SUN UPDATE — PR #329 Phase B Observer Audit

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Read-first continuity

Continue from `SUN_UPDATE_PR329_Rung1IJ_Execution_Green_2026-09-23.md`. Do not restart Rungs 1A–1J or the Phase-A proof work.

## Exact-head CI evidence

Current PR head audited in this successor pass: `98f947eaa7de4f7126001d81b4bc87f158665539`.

Focused CI run #2785 (run id `35846621375`) completed SUCCESS on that exact SHA. The earlier uncertainty around the Slow Adapter tail is therefore closed as well as the focused CPU/authored-Perlin evidence from the preceding pass. The current observer head is workflow-green.

## Targeted Phase-B seam audit

I re-read only the relevant surfaces rather than dumping the repository:

- `src/Singularity/Screen/Renderer.hpp` observer admission/setter seam;
- `src/Singularity/Screen/RenderedFieldSemanticObserver.hpp`;
- the Phase-B observer assertions in `tests/singularity/rendered_field_piecewise_synthesis_test.cpp`;
- exact-head PR/workflow state.

### What remains sound

The observer remains genuinely non-authoritative. `setRadianceSources(...)` and `setVolumeDensitySources(...)` submit admitted bindings only to diagnostic observation. No theorem result is consumed by renderer control flow. `authorityBypassesApplied == 0` remains the constitutional Phase-B invariant.

Stable source-set revisions have an O(1) observer fast path. Authored vessel revisions remain distinct from source-set revisions. Canonical math sharing does not merge channel/vessel theorem authority.

### Lifecycle gap confirmed at renderer boundary

The renderer setter still does only:

`_renderedFieldObserver.setEnabled(on);`

Thus this sequence remains diagnostic-incomplete:

1. admit radiance/density source sets while observation is OFF;
2. flip observation OFF -> ON;
3. do not mutate or re-submit the world;
4. query observer telemetry.

The renderer already owns `_radianceSources`, `_radianceSourcesRevision`, `_volumeDensitySources`, and `_volumeDensitySourcesRevision`, but the false->true transition does not submit them. Telemetry stays empty until a later source setter call.

This is not a rendered-truth failure because the observer has no authority. It is an observability/lifecycle defect: an A/B experiment can enable diagnostics against an already-live world and silently miss that scene.

## Successor audit: exact minimal implementation contract

The correct repair belongs in `Renderer`, not in `RenderedFieldSemanticObserver`.

Do **not** make the observer cache copies of source vectors while disabled merely so it can replay them later. That would impose disabled-mode copying/state cost and duplicate renderer-owned admission truth. The renderer already owns exactly the state needed for retro-observation.

The setter should have this semantic shape:

```cpp
void setRenderedFieldSemanticObservationEnabled(bool on) {
    const bool wasEnabled = _renderedFieldObserver.enabled();
    _renderedFieldObserver.setEnabled(on);
    if (!wasEnabled && on) {
        _renderedFieldObserver.observeRadianceSources(
            _radianceSources, _radianceSourcesRevision);
        _renderedFieldObserver.observeVolumeDensitySources(
            _volumeDensitySources, _volumeDensitySourcesRevision);
    }
}
```

This gives the desired lifecycle properties without touching world/render truth:

- OFF -> ON observes the current admitted scene exactly once;
- ON -> ON is idempotent and performs no observer work;
- ON -> OFF only changes diagnostic enablement;
- OFF admission remains observer-inert and does not duplicate/copy source collections;
- the observer's existing revision fast path still governs later stable source submissions.

### Test boundary finding

The existing Phase-B test currently includes `RenderedFieldSemanticObserver.hpp` directly and tests the observer class in isolation. It does **not** include or instantiate `Renderer`, so it cannot witness the lifecycle defect above.

The next witness must therefore cross the actual renderer boundary. Use a tiny test-only `Renderer` subclass implementing the required pure virtual draw/texture methods as no-ops; do not instantiate a real OpenGL/WebGPU backend and do not pull GPU initialization into this CPU witness.

Pin these exact assertions:

1. Admit one zero-rho source and one zero-density medium while observation is OFF.
2. Snapshot source vector sizes/pointers (or expression identities) and both source-set revisions.
3. Assert observer counters remain zero while OFF.
4. Flip OFF -> ON without re-admitting either source set.
5. Assert two vessel observations/two theorem builds and one hypothetical bypass per channel, with `authorityBypassesApplied == 0`.
6. Assert renderer source vectors/expression identities and revisions are byte/identity-equivalent to the pre-enable snapshot.
7. Call `setRenderedFieldSemanticObservationEnabled(true)` again and assert all observer counters are unchanged: ON -> ON must be a genuine no-op.
8. Disable and assert renderer truth is unchanged.
9. Re-enable without world mutation. Because the observer already knows the same source-set revisions, this should produce revision hits rather than semantic/theorem rebuilds; pin `semanticBuilds` and `theoremBuilds` unchanged.

That final disable/re-enable assertion is useful because it distinguishes correct retained diagnostic cache semantics from accidentally resetting/rebuilding the proof observer on every toggle.

## Scope guard

Do not broaden theorem algebra, touch WGSL, marching, accumulation, visibility, or make hypothetical bypasses authoritative in this gate. Do not move source ownership into the observer. The entire production mutation should remain the tiny renderer false->true edge above plus the renderer-level CPU witness.

## Role status

This specific Sun role is **not finished**. Phase A remains closed and exact-head CI is green. Phase B is now reduced to one surgical production lifecycle repair plus its renderer-boundary witness. After that exact gate executes green, audit telemetry/overhead only if the PR still needs it; theorem consumption remains explicitly out of scope.
