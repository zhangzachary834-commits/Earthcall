# SUN HANDOFF — Volumetric V5 COMPLETE / ready to land

Date: 2026-09-24
Owner lane: GPT-5.6 Sol
PR: #343 — Volumetric V5: medium-set composition foundation
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Live completion evidence

Canonical `sync-from-earthcall-main` was re-inspected at `789f4cb07853cfe6ae1d2ae803c082b0913b617a`.

The V5 branch exact head before this completion note was `6e2d82aa80d04d97cc9714dcf0e4ab5baf778b34`. PR #343 was open, mergeable, and based on the same canonical head; the earlier ancestry-preserving reconciliation is therefore still current (behind 0 at the code/tested head).

Earthcall focused CI run `35967528777` on exact head `6e2d82aa...` completed SUCCESS. All four jobs passed:

- Focused CPU tests (macOS)
- SDF range-proxy verification (macOS)
- Slow Adapter independent clock (macOS)
- SDF authored-Perlin A/B (macOS Release)

Inside the SDF range-proxy job, `Verify V5 fused overlap physics` passed natively. Generic WebGPU SDF parity, distance parity, authored-color parity, object/radiance parity, authored-Perlin core gates, and authored-Perlin camera gate also passed.

PR #343 has been moved from Draft to Ready for Review.

This documentation commit is intentionally code-neutral. The authoritative tested implementation ancestor remains `6e2d82aa...`; no production/test code changed after that green run.

## What V5 established

V5 replaces sequential whole-medium framebuffer composition for 2+ projected participating media with one fused medium-set transport answer while preserving the exact V0–V4 single-medium path.

The completed rung has evidence for:

- canonical medium-set identity across independently authored `(D, sigma_t, sigma_s, C_v, Phi, E_v)` channels;
- fused multi-medium composition;
- order invariance: medium ordering is not physics;
- occupied-segment/local sampling so distant vacuum span does not coarsen local medium quality;
- numeric-only parameter refresh without structural recompilation;
- per-medium Timeline runtime movement without structural regeneration;
- membership add/remove bounded cache behavior;
- refused-member / no-stale-output behavior;
- shared combined-extinction transport rather than sequential alpha composition;
- independent scattering/chroma/emission contribution inside overlap;
- native `webgpu_v5_overlap_physics_test` in focused CI.

Canonical synthesis CI, ScreenRecorder/readback work, and newer canonical intent were preserved during reconciliation. No force push or stale snapshot overwrite was used.

## Landing action

PR #343 is implementation-complete and Ready for Review. The remaining action is ordinary review/merge of #343, not more V5 semantic work. Before merging, re-inspect canonical and exact PR mergeability; if canonical advances, reconcile only if required and rerun the affected exact-head gates. Do not reopen completed V5 semantics without new failing evidence.

## Next architecture frontier

Do not call the successor “V6” merely because V5 is complete. The next bounded task must be selected from live repository evidence after #343 lands.

Successor mission:

1. Re-inspect canonical after #343 landing and read this handoff plus the V5 plan.
2. Audit the now-landed volumetric architecture for the first concrete remaining transport/authoring limitation that is not already solved by V0–V5.
3. Write a bounded constitution/plan before production implementation. It must name the old constraint, proposed invariant, explicit non-goals, compatibility boundary, invalidation/cache semantics, and native witnesses that would falsify the design.
4. Prefer a test/spec/probe rung first when economics or semantics are uncertain. Do not silently jump into GI, multiple scattering, spectral transport, new renderer-owned domain nouns, or unrelated renderer refactors.
5. Preserve the authored-channel sovereignty established through V5 and preserve exact single-medium and fused-set semantics unless a separately proven successor constitution supersedes them.

## Successor handoff prompt

Continue Earthcall volumetric architecture only after PR #343 is landed. Read `agent intercom/communication-threads/SUN_HANDOFF_V5_COMPLETE_READY_TO_LAND_2026-09-24.md` and `docs/plans/VOLUMETRIC_V5_MEDIUM_SET_COMPOSITION_PLAN_2026-09-23.md`, then inspect live canonical, open renderer/volumetric PRs, CI, and current architecture notes using targeted reads only. Do not restart V0–V5. First identify the smallest concrete post-V5 transport or authoring limitation supported by live evidence. Write a bounded successor constitution/plan before production implementation, with explicit invariants, non-goals, compatibility boundaries, invalidation/cache behavior, and falsifying native witnesses. If uncertainty is architectural or economic, build a test/spec/probe rung before changing production rendering. Do not assume the successor is GI, multiple scattering, spectral transport, or any pre-guessed “V6”; earn the next scope from evidence. Update Agent Intercom with the chosen frontier, evidence, plan, and exact continuation point. Definition of done for the successor task: one bounded post-V5 frontier has a reviewed evidence-backed constitution and either a completed proof/test rung or a clearly justified implementation landing; then write a new handoff and retire that successor task rather than expanding scope indefinitely.

— GPT-5.6 Sol