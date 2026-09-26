# SUN UPDATE — PR #329 reconciled CI gate

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Continuity

Do not restart Rungs 1A–1J, Phase A, or the V4/base-drift investigation. Read `SUN_UPDATE_PR329_PhaseB_Base_Drift_V4_Audit_2026-09-23.md` first.

This pass re-read the live PR/head, the reconciled V4 handoff, the exact Renderer semantic-observation seam, and live Actions state only. No repository-wide dump was used.

## Live state

PR #329 is open, draft, and mergeable. The live head at the start of this pass was `5389c20f5d280032878d4d9f22295525b4569422`, whose parent is the two-parent current-default reconciliation merge `b34f60c98214ddc410e4e2b18b7524692488db54`.

The reconciled Renderer source still has the bounded lifecycle gap: `setRenderedFieldSemanticObservationEnabled(bool)` only calls `_renderedFieldObserver.setEnabled(on)`. Meanwhile `setRadianceSources(...)` and `setVolumeDensitySources(...)` retain renderer-owned collections/revisions and forward them to the observer. Therefore OFF admission followed by OFF->ON still cannot retro-observe the already-admitted scene without another source mutation.

The intended surgical repair is unchanged: only on a false->true edge, enable and replay `_radianceSources/_radianceSourcesRevision` and `_volumeDensitySources/_volumeDensitySourcesRevision`. ON->ON remains a no-op; ON->OFF only disables. Do not add observer-owned world state or broaden theorem authority.

## CI gate clarified

Run #2849 on reconciliation commit `b34f60c9...` did **not** fail a witness: it was cancelled after the documentation successor commit advanced the PR head. Therefore it is not a negative signal and must not be treated as a regression.

The replacement exact-head run is #2854 on `5389c20f...`.

At this pass:

- `Slow Adapter independent clock (macOS)` completed **SUCCESS**, including soundness, dramatic Law-Direct A/B, and authored-world impact.
- `Focused CPU tests (macOS)` remains queued.
- `SDF range-proxy verification (macOS)` remains queued.

So the reconciled head has one full green job but has **not yet satisfied the complete focused-CI gate**. Do not land the lifecycle mutation before the two queued jobs execute green; that preserves the handoff's requirement to prove the reconciled pre-patch witness first.

## V4 compatibility boundary remains intact

The current base carries V4 `VolumeDensityBinding::emissionExpr` / `emissionRevision`, while Phase-B semantic observation remains density-only. This pass found no reason to broaden the theorem surface. V4 emission must remain observationally inert to this rung unless a later, separately-scoped theorem expansion is authorized.

## What remains

1. Wait for #2854 `Focused CPU tests` and `SDF range-proxy verification` to complete.
2. If both are green, apply the already-specified false->true Renderer replay patch using a patch-safe mutation surface.
3. Add the renderer-boundary lifecycle witness proving OFF admission inertness, OFF->ON retro-observation, ON->ON idempotence, ON->OFF truth preservation, second-enable revision hits, and `authorityBypassesApplied == 0` throughout.
4. Add/retain an explicit V4 compatibility assertion or audit note that `emissionExpr` does not gain density theorem authority.
5. Run exact-head focused CI after the lifecycle patch.
6. Then perform the final disabled-overhead / telemetry A-B audit and decide whether this Sun role is finished.

## Role status

Still active. The base reconciliation is complete and the earlier cancelled run is explained, but the replacement exact-head pre-patch CI gate is only partially complete. No production mutation was made in this pass because doing so before the queued reconciled witnesses finish would violate the established gate.