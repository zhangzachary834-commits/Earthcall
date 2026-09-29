# SUN UPDATE — V5 reconciled to current canonical; exact-head CI gate remains

Date: 2026-09-24
Repository: `zhangzachary834-commits/Earthcall`
PR: #343 — Volumetric V5: medium-set composition foundation
Owner branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Live state re-inspected first

Canonical `sync-from-earthcall-main` had advanced to:

`789f4cb07853cfe6ae1d2ae803c082b0913b617a`

PR #343 was still open + Draft, mergeable, at head:

`d6d27efddaeab2100409b6f84f1ffb2d94052552`

The compare against live canonical showed the V5 branch was 37 commits ahead and 3 commits behind, with merge base `d0ea104ea85078038b85bcf0a37750326feb8fbb`.

The canonical-only delta after that merge base was targeted and documentation/spec-only: two added files, `SUN_AUDIT_VOLUMETRIC_TORCH_SCOPE_STILL_COMPLETE_2026-09-23.md` and `FORMATION_PROPHETIC_SPATIAL_WITNESS_COLLISION_ABSORPTION_SPEC_2026-09-23.md`. No V5 production/test seam overlapped.

## Exact-head CI audit before reconciliation

The newest PR-head focused CI associated with `d6d27efd...` was run `35961356251`. It was cancelled, not green. Its four jobs (SDF range-proxy verification, Focused CPU tests, Slow Adapter independent clock, and authored-Perlin A/B) were all cancelled before useful execution. Therefore that run is not evidence of a V5 regression, but it also cannot satisfy the final exact-head gate.

Do not reuse an older green run as proof for the reconciled head.

## Canonical reconciliation performed

To avoid a snapshot overwrite or force push, I opened integration-only PR #360 with head `sync-from-earthcall-main` and base the V5 owner branch. GitHub computed it mergeable with exactly 3 commits / 2 changed files / 883 additions / 0 deletions.

PR #360 was merged with merge method `merge`, producing the ancestry-preserving V5 branch merge commit:

`066235ee20e467eaa6426da804f82aa5b49bb921`

This preserves both the V5 lineage and current canonical lineage. No production renderer code was edited in this pass.

## Post-merge CI state

Immediately after the reconciliation merge, no PR-triggered workflow run was yet associated with `066235ee...` through the connector. That means V5 is **not yet complete** under the declared definition of done. The final focused CI must execute on an exact-head descendant containing this reconciliation and include the native WebGPU V5 overlap gate.

This documentation commit is a descendant of `066235ee...`, so if workflow concurrency supersedes an earlier run, use the newest exact-head descendant rather than insisting on the merge SHA itself.

## Semantic scope remains closed

No new V5 semantic feature is justified here. Preserve the already-established witnesses and implementation for fused medium-set transport, occupied/local sampling, numeric-only refresh, per-medium Timeline updates, membership invalidation, refusal/no-stale-output, shared combined extinction, independent scattering/chroma/emission overlap contribution, and `webgpu_v5_overlap_physics_test`.

Do not reopen V3/V4 and do not invent V6 inside #343.

## Exact continuation point

1. Re-inspect live canonical first; if it advanced again, compare only the new delta and reconcile only if needed.
2. Inspect the newest focused CI on the current V5 exact-head descendant containing `066235ee...` and this documentation commit.
3. If CI is red, repair only the concrete V5/reconciliation regression and rerun.
4. If CI is green including the native WebGPU/V5 overlap gate and canonical synthesis witnesses, V5 satisfies the remaining integration gate: mark #343 Ready for Review, write the final V5 completion/handoff document, and retire the V5 torch.
5. Do not merge #343 into canonical unless separately authorized; completion here means implementation-complete and genuinely ready for review/landing.

— GPT-5.6 Sol
