# SUN UPDATE — PR #329 Phase B CI Revalidation

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329
Read first: `SUN_UPDATE_PR329_PhaseB_Observer_Audit_2026-09-23.md`

## Targeted successor pass

Do not restart Rungs 1A–1J or Phase A. This pass re-read only the live PR head, the Phase-B handoff, `Renderer.hpp`, `RenderedFieldSemanticObserver.hpp`, the existing Phase-B witness tail, and exact-head Actions state.

Audited head before this documentation commit: `41c2185cb7a0b4af4811c407effa7305e7bab10f`.

## CI result that changed the live picture

Focused CI run #2789 on exact head `41c2185cb...` completed FAILURE, but the failure is isolated to the unrelated `Slow Adapter independent clock (macOS)` job, step `Measure adapter impact across authored worlds`.

The PR329-relevant evidence on the same SHA is green:

- `SDF range-proxy verification (macOS)`: SUCCESS, including `Run CPU SDF proof witnesses`.
- `Focused CPU tests (macOS)`: SUCCESS, including focused regression witnesses.
- `SDF authored-Perlin A/B (macOS Release)`: SUCCESS.
- Slow Adapter soundness and the dramatic Law-Direct A/B steps themselves passed before the authored-world impact step failed.

A new focused CI run #2795 was queued on the same SHA during this audit. Treat #2789's aggregate red as an unrelated tail failure unless #2795 or later evidence shows a PR329-relevant regression.

## Lifecycle gate remains exact and bounded

`Renderer::setRenderedFieldSemanticObservationEnabled(bool)` still only delegates to `_renderedFieldObserver.setEnabled(on)`. The renderer already owns `_radianceSources`, `_radianceSourcesRevision`, `_volumeDensitySources`, and `_volumeDensitySourcesRevision`.

Therefore the still-blocking Phase-B lifecycle defect remains: source admission while OFF followed by OFF->ON does not observe the already-admitted scene until another source setter call happens. This is diagnostic incompleteness, not rendered-truth corruption, because `authorityBypassesApplied` remains zero and the observer exposes no theorem-consumption authority.

The minimal repair remains unchanged: on the false->true edge only, enable and replay the renderer-owned radiance and density collections with their current set revisions. ON->ON must do no work; ON->OFF must only disable; OFF->ON after prior observation should hit the observer's set-revision fast paths rather than rebuild semantic/theorem artifacts.

## Important execution note

No production mutation is claimed in this pass. The branch was left with the production source intact after the audit. The next implementation pass must make the tiny `Renderer.hpp` lifecycle change and add a renderer-boundary no-op-backend witness before this Sun role can be considered finished.

## What remains

1. Implement false->true replay at the Renderer ownership boundary.
2. Add the renderer-level lifecycle witness: OFF admission inert; OFF->ON retro-observes; ON->ON idempotent; disable leaves renderer truth unchanged; re-enable takes revision-hit paths; `authorityBypassesApplied == 0` throughout.
3. Run exact-head focused CI and distinguish PR329-relevant failures from unrelated Slow Adapter authored-world variance.
4. If green, perform the final observer overhead/telemetry audit and decide whether this role is finished.

Scope remains strict: no WGSL, transport, visibility, theorem consumption, new algebra, or observer-owned duplicate world state.