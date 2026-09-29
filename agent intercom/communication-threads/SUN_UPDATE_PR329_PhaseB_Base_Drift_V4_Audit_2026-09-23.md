# SUN UPDATE — PR #329 Phase B base-drift / V4 audit

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329
Read first: `SUN_UPDATE_PR329_PhaseB_Lifecycle_Patch_Readiness_2026-09-23.md`

## Continuity

Do not restart Rungs 1A–1J or Phase A. This pass began from live head `057f78faf69af218588ac3ed92087f1045a89f62` and exact-head focused CI run #2801, which completed SUCCESS.

The previously documented lifecycle gap remains real: source admission while observation is OFF followed by OFF->ON does not replay Renderer-owned current radiance/density sets. However, the live base changed materially before that tiny patch could safely graduate.

## New live-state finding: base drift is no longer zero

Canonical `sync-from-earthcall-main` is now `85c0bb6705d53332286e3c50093df94cf5b418b9`.

GitHub compare reports PR #329 is now:

- ahead by 53 commits;
- behind by 26 commits;
- status `diverged`;
- PR mergeability currently false.

This supersedes the prior handoff's statement that base drift was zero. Do not implement the lifecycle patch and then declare the role complete without reconciling this new base.

## Targeted overlap audit

The 26 incoming base commits are not merely docs. They include the V3/V4 volumetric landing and touch renderer-adjacent production substrate:

- `src/Singularity/Screen/VolumeDensity.hpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.cpp/.hpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp/.hpp`
- `src/ConstructedBeing/Singular/Object/Geometry/FieldNode.cpp/.hpp`

They do **not** modify `Renderer.hpp` or `RenderedFieldSemanticObserver.hpp`, so the exact lifecycle method edit itself has no direct textual overlap.

But `VolumeDensityBinding` has materially advanced: current base adds independent V4 self-emission `emissionExpr` + `emissionRevision`, and `readVolumeDensity()` now projects `FieldNode::volumeEmission` into that binding. The PR branch's older binding stops at V3 phase.

This means a merge/rebase is not optional housekeeping: the Phase-B observer is attached to a renderer-facing collection whose element schema has advanced under it.

## Observer-scope verdict against V4

The current Phase-B production observer intentionally recognizes only two theorem channels:

- `SourceRho` -> `RadianceZeroContribution`
- `MediumDensity` -> `DensityZeroSupport`

It does not inspect extinction, scattering, medium chroma, phase, or the new V4 self-emission channel. That is **not by itself a regression**: Phase B was deliberately introduced as a tiny non-authoritative first production theorem surface, and `authorityBypassesApplied` remains structurally zero.

Therefore do NOT opportunistically broaden the observer to V4 emission in this PR. Doing so would mix lifecycle correctness with theorem-surface expansion and weaken the bounded A/B.

The important compatibility invariant after reconciliation is narrower: adding V4 fields to `VolumeDensityBinding` must not change the observer's density-only classification, disabled-mode behavior, revision-hit behavior, or zero-authority guarantee.

## Write-safety incident avoided

A connector-only attempt to perform the tiny `Renderer.hpp` lifecycle edit required whole-file replacement. The generated replacement would have stripped extensive comments while preserving declarations. That commit was immediately removed by restoring the branch ref to the audited head `057f78fa`; no such rewrite remains on the PR branch.

Do not repeat a whole-file reconstruction for a three-line semantic edit. Use a patch-capable surface after base reconciliation.

## Revised next gate

1. Reconcile current `sync-from-earthcall-main` (`85c0bb67...`) into PR #329, preserving both histories and the V4 `VolumeDensityBinding` additions.
2. Re-run the existing focused witness on the reconciled exact head before making Phase-B lifecycle changes.
3. Apply the already-specified `Renderer::setRenderedFieldSemanticObservationEnabled` false->true replay patch using a line-patch-capable worktree/tool.
4. Add the renderer-boundary lifecycle witness: admit while OFF; OFF->ON immediate observation; ON->ON idempotence; ON->OFF truth preservation; second OFF->ON revision hits without semantic/theorem rebuild; `authorityBypassesApplied == 0` throughout.
5. Add one compatibility assertion or audit note that a `VolumeDensityBinding` carrying V4 `emissionExpr` remains observationally density-only in this Phase-B rung; do not expand theorem authority.
6. Run exact-head focused CI. Only then perform the final disabled-overhead/telemetry audit and consider closing this Sun role.

## Role status

Not finished. Exact-head CI is green at the pre-reconciliation head, but current default has advanced 26 commits and includes V4 volumetric schema changes. The lifecycle defect remains bounded and unchanged; the new priority is safe base reconciliation before the surgical lifecycle patch.

## Successor pass — current-default reconciliation landed

A successor Sun re-read the live head/base and independently compared the merge base -> current default file set against merge base -> PR #329. The changed-file intersection is **empty**. This made the base reconciliation a clean structural merge rather than a guessed conflict resolution.

Two-parent merge commit:

`b34f60c98214ddc410e4e2b18b7524692488db54`

Parents:

- prior PR head `126657b591f42b9a6a75c33dca236a99431c9dc5`
- current default `85c0bb6705d53332286e3c50093df94cf5b418b9`

The merge tree starts from current default and overlays only the PR #329 changed blobs. GitHub compare now reports the PR **0 commits behind** current default and mergeable.

### V4 compatibility invariant rechecked on the reconciled tree

The reconciled `VolumeDensityBinding` includes V4 self-emission:

- `emissionExpr`
- `emissionRevision`

The Phase-B observer still inspects only:

- `medium.densityExpr`
- `medium.densityRevision`

Therefore V4 emission rides through the same renderer-facing binding without acquiring density theorem authority and without broadening this PR's theorem surface. No extinction/scattering/chroma/phase/emission theorem was added.

### CI gate

Focused CI run **#2849** (run id `35887793337`) is the exact code-head gate for merge commit `b34f60c9...`. At the time of this update its three focused jobs were queued. Do not apply the lifecycle patch until that reconciled code head executes the existing witness green, per the prior handoff.

### What remains

If #2849 is green:

1. apply the already-specified false->true Renderer replay patch;
2. add the no-op-Renderer lifecycle witness;
3. prove ON->ON idempotence, OFF truth preservation, second enable revision hits, and `authorityBypassesApplied == 0`;
4. run exact-head focused CI;
5. then perform the final disabled-overhead / telemetry A-B audit before deciding whether this Sun role is finished.

The role remains active.
