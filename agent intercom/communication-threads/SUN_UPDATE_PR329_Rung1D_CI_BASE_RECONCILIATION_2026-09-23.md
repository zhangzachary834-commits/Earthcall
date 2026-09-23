# SUN UPDATE — PR #329 Rung 1D CI + Base Reconciliation

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Read-first continuity

This is a continuation of `SUN_UPDATE_PR329_Scene_Spatial_Synthesis_DAG_Rung1_2026-09-22.md`. Do not restart the investigation.

The immediately prior technical rung is Rung 1D: a test-only conservative support proof on the synthetic scene-spatial execution DAG. It recognizes `min(shared-biasA, shared-biasB)`, bypasses the proved losing branch, survives ambient/runtime sample changes, invalidates on authored bias changes, falls open to exact evaluation when invalid, and can be locally re-proved with the winner reversed.

## CI verdict changed: Rung 1D is green

The prior handoff recorded Rung 1D CI as queued. That is now stale.

Exact-head PR workflow run **#2572** for commit `d7726ae1789888b1d25525cd841ead21b2bbaf9e` completed **successfully**. This head contains the Rung 1D code commit `4393ee853681a16491674bf8a62fe978c7504f18` plus the Intercom documentation commit. Therefore the synthetic proof-on-road bypass witness has now passed the focused CI gate.

Run #2570 was cancelled without a runner, but it is superseded by successful exact-head run #2572. Do not treat #2570 cancellation as a code failure.

## Concurrent-Sun collision detected and handled safely

During this pass another chat/Sun appended the exact user-facing Rung 1D report to the canonical Intercom thread as commit `7107426ffd690ac6e5c146f7254cd4a46fc55d1d` (`Mirror Rung 1D report to Zach in Agent Intercom`).

An attempted ref advance based on the older `d7726ae1` head correctly failed non-fast-forward. No force push was used. The live head was re-read and the other Sun's commit was preserved.

This is important coordination evidence: always re-read PR head immediately before writes because the hourly/chat successors can overlap.

## Current-default reconciliation

After Rung 1D went green, default advanced by one commit to `458949c178b853534862a9cb5d9b897cc4c13665` (`THE SPECIFIC TASKS AREA CTUALLY ORGANIZED NOW`). The PR became 1 commit behind and GitHub temporarily reported it non-mergeable.

That base commit is organizational/docs work and does not overlap the four PR #329 files.

The branch was reconciled with a real two-parent merge commit:

`d9dcb082623649cf79a4319dc2ab5df7739ad441`

Parents:
- live PR head `7107426ffd690ac6e5c146f7254cd4a46fc55d1d`
- current default `458949c178b853534862a9cb5d9b897cc4c13665`

The merge tree starts from the current-default tree and overlays the four exact PR blobs, including the concurrently-added Intercom report. No force update was used.

## What remains

Rung 1D synthetic proof-on-road is now green and base-reconciled. The next technical gate remains the one already named in the canonical handoff: mirror the same conservative support-proof semantics onto the **real OntoMath-compiled DAG** in `scene_spatial_ontomath_synthesis_test.cpp`.

That next rung must preserve all six invariants:

1. derive proof from canonical compiled semantic child identity, never pretty-printed text;
2. declare/track the authored premise dependencies;
3. runtime sample movement must not rebuild proof;
4. authored premise changes invalidate only the dependent proof/repair frontier;
5. missing/invalid proof falls open to exact compiled evaluation with no optimization authority;
6. count proof consultation, bypass, fallback, and avoided compiled-node work separately.

Do not move this into production renderer/WGSL yet. The next experiment belongs in the real OntoMath test adapter first.

## Immediate successor instruction

First check CI on merge head `d9dcb082...` (and any newer documentation-only head). If green, implement the real-OntoMath support-proof rung. If red, fix only the concrete regression. Re-read live PR head before every write to avoid another Sun collision. No Big Chungus repository dumps.