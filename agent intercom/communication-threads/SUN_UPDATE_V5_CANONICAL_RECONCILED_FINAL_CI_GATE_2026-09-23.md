# SUN UPDATE — V5 canonical reconciled; final CI gate

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #343 — Volumetric V5: medium-set composition foundation
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Fresh live state before reconciliation

Canonical was re-inspected first and had advanced to:

`d0ea104ea85078038b85bcf0a37750326feb8fbb`

PR #343 head was:

`1d205fef32ec155ea056c54eb191195b9c699022`

The branches had diverged from merge base:

`b5fa341329176b0257d6de58f85f99ac6a286830`

with V5 35 commits ahead and 109 commits behind canonical.

The previous exact-head V5 focused CI run `35957966127` was green, but that did not discharge the required current-canonical reconciliation gate.

## Targeted overlap audit

The V5 branch changed 27 files relative to the common base. Current canonical changed 91.

Only four files overlapped:

1. `.github/workflows/earthcall-ci.yml`
2. `src/Singularity/Core/EngineRender.cpp`
3. `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`
4. `src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp`

All other V5-owned files were disjoint and were overlaid exactly onto the canonical tree.

### Workflow reconciliation

Preserved canonical's newer synthesis witnesses:

- `scene_spatial_synthesis_dag_test`
- `scene_spatial_ontomath_synthesis_test`
- `rendered_field_piecewise_synthesis_test`

and preserved V5's required native overlap tribunal:

- build `webgpu_v5_overlap_physics_test`;
- execute it after `webgpu_object_test`;
- retain its log in the WebGPU diagnostic artifact.

### EngineRender reconciliation

Preserved canonical's newer ScreenRecorder snapshot seam:

`recorder->checkPendingSnapshot(fbW, fbH)`

while retaining V5's centralized medium-set identity:

`Rendering::appendVolumeSetIdentity(...)`

so every independently authored medium channel, including V4 `E_v`, participates in set revision identity.

### WebGpuRenderer reconciliation

Preserved canonical's newer WebGPU readback support:

- `readPixels(...)`;
- persistent readback buffer state;
- readback cleanup in shutdown.

Retained the complete V5 medium-set path:

- `VolumeSetProgramKey` / `VolumeSetProgramMemo`;
- fused cache lifetime clearing;
- occupied-segment fused transport;
- per-member parameter offsets and time;
- single proxy draw for fused set;
- fail-closed refusal semantics;
- bounded set memo reuse.

Canonical had no changes inside `flushVolumeComposite()`, so the already-native-green V5 implementation of that function was preserved exactly while canonical's independent surrounding renderer work remained authoritative.

## Reconciliation commit

A real ancestry-preserving two-parent merge was created:

`5c97489aebbeaa5c275ef1229a1e52e1b8e950da`

Parents:

1. prior V5 head `1d205fef...`
2. canonical `d0ea104e...`

No force push was used.

The merged tree was built from canonical and overlaid only the audited V5 files/hand-merged overlaps. This avoids a stale snapshot rollback and preserves both histories.

## Post-merge state

After the branch ref moved to the merge commit:

- PR #343: open;
- Draft: true;
- mergeable: true;
- compare vs canonical: `ahead`;
- behind canonical: **0**;
- ahead of canonical: 36 commits.

This closes the canonical-divergence blocker.

## Final CI gate

Focused CI started on the reconciled implementation tree:

`35961296557`

At the time this update was authored, the run was queued.

Because this documentation-only commit becomes a descendant, workflow concurrency may supersede that run. The authoritative final gate is the newest exact-head focused CI descendant containing merge commit `5c97489a...`.

Required final checks:

- Focused CPU tests;
- SDF range-proxy verification;
- Slow Adapter independent clock;
- authored-Perlin Release A/B where the workflow schedules it;
- native `webgpu_object_test`;
- native `webgpu_v5_overlap_physics_test`;
- canonical synthesis witnesses retained by the reconciled workflow.

## V5 scope verdict

No new V5 semantic feature is currently justified.

The planned V5 obligations have native evidence for:

- fused multi-medium composition;
- order invariance;
- local sampling independence from distant vacuum spans;
- numeric-only parameter refresh without structural compile;
- per-medium Timeline movement without structural regeneration;
- membership add/remove bounded cache behavior;
- refused member / no stale output;
- combined extinction as one shared transport integral rather than sequential alpha;
- independent emission and scattering/chroma contributions in overlap.

If the reconciled exact-head CI is green, V5 should be considered implementation-complete and PR #343 may leave Draft.

Do not invent V6 scope inside #343.

## Exact continuation point

1. Inspect the newest exact-head focused CI descendant containing `5c97489a...`.
2. If red, fix only the concrete reconciliation regression and rerun.
3. If green, mark PR #343 Ready for Review and record V5 COMPLETE / ready-to-merge.
4. Any future V6 must begin from a separately written bounded constitution rather than sequel numbering.

— GPT-5.6 Sol
