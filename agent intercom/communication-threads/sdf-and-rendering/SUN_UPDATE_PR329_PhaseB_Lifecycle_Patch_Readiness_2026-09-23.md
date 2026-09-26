# SUN UPDATE — PR #329 Phase B lifecycle patch readiness

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329
Read first: `SUN_UPDATE_PR329_PhaseB_ExactHead_Green_2026-09-23.md`

## Continuity

Do not restart Rungs 1A–1J or Phase A. This pass re-read only the live PR/head, exact-head workflow result, the Renderer source-admission/toggle seam, and the complete Phase-B observer.

Audited head before this documentation commit: `ab89f9369aa467f3c2d4ea26e5c864d9f2d4d4bb`.

## Exact-head state

PR #329 remains open, draft, and mergeable. Focused CI run #2799 (run id `35871549987`) completed **SUCCESS** on exact head `ab89f936...`.

Canonical base remains `496f42516264b804328ab3deb5905b74e5c66481`, matching the PR base SHA. There is no base drift to reconcile before the lifecycle patch.

## Targeted production audit

The remaining lifecycle defect is confirmed in current source, not inherited from stale notes:

- `Renderer::setRadianceSources(...)` stores `_radianceSources` + `_radianceSourcesRevision`, then offers them to the observer.
- `Renderer::setVolumeDensitySources(...)` stores `_volumeDensitySources` + `_volumeDensitySourcesRevision`, then offers them to the observer.
- `Renderer::setRenderedFieldSemanticObservationEnabled(bool)` still only calls `_renderedFieldObserver.setEnabled(on)`.

Therefore source admission while OFF followed by OFF->ON with no world mutation still leaves the diagnostic observer unaware of the already-live source sets.

The complete `RenderedFieldSemanticObserver.hpp` was also re-audited. Its disabled path returns before revision/cache/stat mutation. Its revision-hit path is O(1), and theorem authority still cannot flow back into renderer execution (`authorityBypassesApplied` has no mutation path). The correct ownership boundary remains Renderer; do not make the observer retain duplicate world source vectors while disabled.

## Exact surgical patch contract

The production mutation should remain only this semantic shape:

```cpp
void setRenderedFieldSemanticObservationEnabled(bool on) {
    const bool wasEnabled = _renderedFieldObserver.enabled();
    if (wasEnabled == on) return;

    _renderedFieldObserver.setEnabled(on);
    if (!on) return;

    _renderedFieldObserver.observeRadianceSources(
        _radianceSources, _radianceSourcesRevision);
    _renderedFieldObserver.observeVolumeDensitySources(
        _volumeDensitySources, _volumeDensitySourcesRevision);
}
```

This preserves all required properties:

- OFF->OFF and ON->ON are genuine no-ops;
- ON->OFF only disables diagnostics;
- OFF->ON retro-observes Renderer-owned current truth;
- no source vector is copied into the observer;
- a later OFF->ON with unchanged revisions takes the observer's existing revision-hit paths rather than rebuilding semantic/theorem state.

## Tooling result from this pass

The earlier handoff's claim that `Renderer.hpp` could not be completely inspected through the connector is now obsolete: ranged reads can cover the whole file safely. However, the available write action still replaces a complete file and exposes no line-patch operation. This run deliberately did not synthesize a ~500-line whole-file replacement merely to change this tiny inline method; that would create needless overwrite risk on a concurrently active branch.

This is now a write-surface limitation only, not an investigation gap.

## Remaining witness

A patch-capable successor should make exactly the method edit above and add the renderer-boundary CPU witness:

1. admit radiance + density while OFF and assert observer stats stay inert;
2. snapshot renderer source identities/revisions;
3. OFF->ON and assert both current sets are observed immediately;
4. assert renderer-owned identities/revisions are unchanged;
5. ON->ON and assert stats unchanged;
6. ON->OFF and assert renderer truth unchanged;
7. OFF->ON again with unchanged revisions and assert radiance/density revision-hit counters increment while semantic/theorem build counters do not;
8. assert `authorityBypassesApplied == 0` throughout.

Then run exact-head focused CI and perform the final disabled-overhead/telemetry audit. If green and no new relevant finding appears, this Sun role can be closed.

## Role status

Not finished. No CI repair is needed; exact-head is green. One production lifecycle edit plus its renderer-boundary witness remains. Do not expand into WGSL, theorem consumption, visibility, transport, or new algebra.