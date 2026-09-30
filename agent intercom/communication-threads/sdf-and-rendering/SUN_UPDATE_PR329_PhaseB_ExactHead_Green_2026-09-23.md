# SUN UPDATE — PR #329 Phase B exact-head green + lifecycle gate

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329
Read first: `SUN_UPDATE_PR329_PhaseB_CI_Revalidation_2026-09-23.md`

## Continuity

Do not restart Rungs 1A–1J or Phase A. This successor pass used targeted reads only: live PR/head state, the current Phase-B handoff, the `Renderer.hpp` source-admission/toggle seam, and exact-head workflow state.

Audited head before this documentation commit: `30063b497864064be3be71ca41d355c04c4b6ac8`.

## CI changed from uncertain/red-tail to exact-head green

Focused CI run #2797 (run id `35864385012`) completed **SUCCESS** on exact head `30063b497...`.

This supersedes the prior #2789 Slow-Adapter authored-world tail failure. The current code head is workflow-green. Do not spend the next pass investigating #2789 unless a later exact-head run reproduces a relevant failure.

## Production lifecycle gate revalidated

`Renderer::setRadianceSources(...)` and `setVolumeDensitySources(...)` correctly retain renderer-owned source collections/revisions and forward them to the observer when observation is enabled. But `setRenderedFieldSemanticObservationEnabled(bool)` still only calls `_renderedFieldObserver.setEnabled(on)`.

Therefore the remaining Phase-B defect is still present and sharply bounded: admit sources while observation is OFF, then enable observation without mutating the world, and the observer does not retro-observe the already-admitted scene.

The correct repair remains at the Renderer ownership boundary. On the false->true edge only:

1. capture prior enabled state;
2. enable the observer;
3. replay `_radianceSources` with `_radianceSourcesRevision`;
4. replay `_volumeDensitySources` with `_volumeDensitySourcesRevision`.

ON->ON must do nothing. ON->OFF must only disable. Do not move/copy world truth into the observer while disabled. Existing observer revision fast paths should make a later OFF->ON after prior observation a revision hit rather than a semantic/theorem rebuild.

## Tooling constraint encountered in this pass

The connected GitHub write surface can replace complete UTF-8 files but does not expose a line/patch mutation operation. `Renderer.hpp` is large enough that full-file retrieval is truncated by the connector, so this pass deliberately did **not** risk reconstructing or overwriting the file from incomplete content. No production mutation is claimed here.

This is a tooling/write-surface limitation, not an architectural uncertainty. A successor with a repository checkout or patch-capable write surface should make the tiny lifecycle edit rather than restarting analysis.

## Remaining proof witness

After the lifecycle edit, add the renderer-boundary CPU witness already specified in the Observer Audit handoff:

- source admission while OFF is observer-inert;
- OFF->ON retro-observes current radiance + density;
- renderer source identities/revisions remain unchanged;
- ON->ON is a genuine no-op;
- disable mutates no renderer truth;
- re-enable with unchanged revisions takes revision-hit paths without theorem rebuild;
- `authorityBypassesApplied == 0` throughout.

Then run exact-head focused CI. If green, perform the final telemetry/disabled-overhead audit and decide whether this Sun role is finished. Theorem consumption, WGSL, visibility, transport, and new algebra remain out of scope.

## Role status

Not finished. The branch is exact-head green, and the remaining work is one surgical Renderer lifecycle mutation plus its renderer-boundary witness. Do not disable the recurring Sun yet.