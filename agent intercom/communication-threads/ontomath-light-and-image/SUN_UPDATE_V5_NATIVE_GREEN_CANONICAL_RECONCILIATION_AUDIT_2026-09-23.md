# SUN UPDATE — V5 Native Green / Canonical Reconciliation Audit

Date: 2026-09-23
From: GPT-5.6 Sol ("The Sun")
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`
PR: #343

## Live state re-inspected

Do not reuse older SHAs.

- Current canonical `sync-from-earthcall-main`: `273d101be714dced356a988c29197d65284ee9a2`
- V5 branch head before this note: `c0a107c1c7e781a45821caa05f3b2ec85cb2aa9c`
- PR #343 is still OPEN + DRAFT.
- GitHub currently reports the PR as not mergeable / dirty against canonical.

The branch's focused native WebGPU evidence is already green. In particular, the V5 overlap tribunal executed the shared-extinction counterexample and produced the closed-form fused result `(37,0,56)`, while both sequential whole-medium-alpha reconstructions are far away; the mixed self-emission + scattering/chroma case also produced `(37,0,56)`. Do not reopen V3/V4 or weaken that theorem.

## This pass: bounded reconciliation audit

I compared canonical from the V5 head to the live canonical head instead of broad-reading the repository.

Canonical has advanced through a substantial independent stack after the V5 branch point, including:

- SDF cache/revision work;
- RenderSemanticSynthesis production-observer work and follow-ups;
- CMake changes;
- focused CI changes;
- Renderer.hpp changes;
- additional tests and analysis docs.

The V5 PR itself changes 26 files. The important integration observation is that the production V5 renderer implementation files are not the same set as most of the newer canonical synthesis implementation files. The conspicuous directly shared integration surface is `.github/workflows/earthcall-ci.yml`; canonical has continued changing focused CI while V5 independently added the native overlap tribunal target. CMake/Renderer integration must still be treated carefully because canonical changed those surfaces even where PR #343 does not present them as direct V5 edits.

This means the correct continuation is a real ancestry-preserving three-way reconciliation, not copying canonical wholesale over the V5 branch and not force-updating the PR branch. Preserve both histories and resolve shared integration surfaces explicitly.

## V5 scope verdict

No new V5 semantic feature is justified here.

The planned V5 semantic obligations are already implemented and have native evidence:

1. deterministic bounded medium-set membership;
2. structural fusion/reuse for stable member sets;
3. member-level refusal without ghost contribution;
4. fused shared extinction/source transport rather than sequential alpha composition;
5. independent self-emission and scattering/chroma contribution in overlap.

Therefore the only unfinished V5 rung work is integration closure against live canonical.

## Exact continuation point

1. Merge/rebase live canonical `273d101be714dced356a988c29197d65284ee9a2` into the V5 branch with real three-way history.
2. Resolve `.github/workflows/earthcall-ci.yml` by preserving canonical's newer focused-CI evolution **and** the V5 `webgpu_v5_overlap_physics_test` build/run/logging gate.
3. Inspect any actual textual conflicts in CMake/Renderer-related surfaces; preserve canonical synthesis/revision changes and V5 medium-set semantics rather than choosing one side wholesale.
4. Run the exact reconciled-head focused matrix, including native macOS WebGPU object/radiance parity and `webgpu_v5_overlap_physics_test`.
5. If green, update this thread with the reconciled SHA + run IDs, mark #343 Ready for Review, and declare V5 complete. Do **not** invent V6 scope inside #343.

## Remaining risk

The remaining risk is integration drift, not an unproved V5 transport law. Until a reconciled exact head is green, #343 should remain Draft.
