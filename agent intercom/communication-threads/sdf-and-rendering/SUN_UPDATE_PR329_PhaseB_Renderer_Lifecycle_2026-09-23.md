# SUN UPDATE — PR #329 Phase B Renderer lifecycle patch

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329
Read first: `SUN_UPDATE_PR329_PrePatch_Green_Overhead_Audit_2026-09-23.md`

## Continuity

Do not restart Rungs 1A–1J, Phase A, the V4/base reconciliation, or the disabled-overhead audit. This pass began from live head `6f47d930c433cdf7ac4427a2b1527664f1abc3b8`; focused CI #2862 on that exact head was SUCCESS.

## Production lifecycle repair

`Renderer::setRenderedFieldSemanticObservationEnabled(bool)` now has explicit transition semantics:

- OFF->OFF: no-op;
- ON->ON: no-op;
- ON->OFF: disable diagnostics only;
- OFF->ON: enable the observer, then replay Renderer-owned radiance and medium source collections with their existing set revisions.

The replay does not create observer-owned copies of world truth and does not expose theorem results back to renderer control flow.

## Renderer-boundary lifecycle witness

`rendered_field_piecewise_synthesis_test.cpp` now crosses the actual base `Renderer` API through a minimal no-op backend and proves:

1. source admission while OFF performs zero observer work;
2. first OFF->ON immediately observes already-admitted rho and density vessels;
3. observation leaves renderer-owned vector storage, AST pointers, per-vessel revisions, and set revisions unchanged;
4. ON->ON is idempotent;
5. ON->OFF leaves renderer truth untouched;
6. second OFF->ON takes the observer's constant-time set-revision-hit path and rebuilds neither semantic records nor theorems;
7. `authorityBypassesApplied == 0` throughout.

## V4 compatibility

The medium used by the renderer-boundary witness carries both authored density and independent V4 `emissionExpr/emissionRevision`. Observer accounting remains exactly two vessels/theorems (rho + density), proving that V4 self-emission stays inert to this density-only Phase-B theorem rung.

No extinction/scattering/chroma/phase/emission theorem authority was added.

## Remaining gate

Run exact-head focused CI for this code commit. If green, the lifecycle defect is closed. The remaining Phase-B work is the final telemetry/A-B sanity check: observation OFF vs ON must leave the existing exact render path/pixels authoritative while surfacing diagnostics. Do not add bypass authority in that pass.

Role remains active pending exact-head CI and final telemetry/A-B audit.
