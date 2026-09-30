# SUN UPDATE — V5 exact-head green + canonical divergence audit

<!-- NAV_BLOCK_START -->
> [!NOTE]
> **Thread Navigation: Volumetric V5 Updates (PR #343)**
> [View Full Thread Index](../00_THREAD_INDEX.md)
>
> **Related in this thread:**
> - [SUN_UPDATE_V5_CANONICAL_RECONCILED_FINAL_CI_GATE_2026-09-23.md](SUN_UPDATE_V5_CANONICAL_RECONCILED_FINAL_CI_GATE_2026-09-23.md)
> - [SUN_UPDATE_V5_EXACT_HEAD_GREEN_SEGMENTATION_GATE_2026-09-23.md](SUN_UPDATE_V5_EXACT_HEAD_GREEN_SEGMENTATION_GATE_2026-09-23.md)
> - [SUN_UPDATE_V5_FUSED_INCREMENTAL_RUNTIME_WITNESSES_2026-09-23.md](SUN_UPDATE_V5_FUSED_INCREMENTAL_RUNTIME_WITNESSES_2026-09-23.md)
> - [SUN_UPDATE_V5_INCREMENTAL_GREEN_MEMBERSHIP_REFUSAL_GATE_2026-09-23.md](SUN_UPDATE_V5_INCREMENTAL_GREEN_MEMBERSHIP_REFUSAL_GATE_2026-09-23.md)
> - [SUN_UPDATE_V5_LOCAL_SAMPLING_AND_CI_RECHECK_2026-09-23.md](SUN_UPDATE_V5_LOCAL_SAMPLING_AND_CI_RECHECK_2026-09-23.md)
> - [SUN_UPDATE_V5_MEDIUM_SET_COMPOSITION_CI_REVIEW_2026-09-23.md](SUN_UPDATE_V5_MEDIUM_SET_COMPOSITION_CI_REVIEW_2026-09-23.md)
> - [SUN_UPDATE_V5_MEDIUM_SET_IDENTITY_2026-09-23.md](SUN_UPDATE_V5_MEDIUM_SET_IDENTITY_2026-09-23.md)
> - [SUN_UPDATE_V5_MEMBERSHIP_GREEN_OVERLAP_NUMERICAL_GATE_2026-09-23.md](SUN_UPDATE_V5_MEMBERSHIP_GREEN_OVERLAP_NUMERICAL_GATE_2026-09-23.md)
> - [SUN_UPDATE_V5_MEMBERSHIP_REFUSAL_NATIVE_WITNESS_2026-09-23.md](SUN_UPDATE_V5_MEMBERSHIP_REFUSAL_NATIVE_WITNESS_2026-09-23.md)
> - [SUN_UPDATE_V5_NATIVE_GREEN_CANONICAL_RECONCILIATION_AUDIT_2026-09-23.md](SUN_UPDATE_V5_NATIVE_GREEN_CANONICAL_RECONCILIATION_AUDIT_2026-09-23.md)
> - [SUN_UPDATE_V5_OCCUPIED_SEGMENT_SAMPLING_2026-09-23.md](SUN_UPDATE_V5_OCCUPIED_SEGMENT_SAMPLING_2026-09-23.md)
> - [SUN_UPDATE_V5_OVERLAP_NATIVE_GREEN_RECONCILIATION_GATE_2026-09-23.md](SUN_UPDATE_V5_OVERLAP_NATIVE_GREEN_RECONCILIATION_GATE_2026-09-23.md)
> - [SUN_UPDATE_V5_OVERLAP_PHYSICS_CI_WIRED_2026-09-23.md](SUN_UPDATE_V5_OVERLAP_PHYSICS_CI_WIRED_2026-09-23.md)
> - [SUN_UPDATE_V5_OVERLAP_PHYSICS_TRIBUNAL_2026-09-23.md](SUN_UPDATE_V5_OVERLAP_PHYSICS_TRIBUNAL_2026-09-23.md)
> - [SUN_UPDATE_V5_RECONCILED_TO_789F4CB_CI_GATE_2026-09-24.md](SUN_UPDATE_V5_RECONCILED_TO_789F4CB_CI_GATE_2026-09-24.md)
> - [SUN_UPDATE_V5_SEGMENTATION_GREEN_NEXT_WITNESS_GATE_2026-09-23.md](SUN_UPDATE_V5_SEGMENTATION_GREEN_NEXT_WITNESS_GATE_2026-09-23.md)
<!-- NAV_BLOCK_END -->



Date: 2026-09-23
Owner lane: GPT-5.6 Sol
PR: #343 — Volumetric V5: medium-set composition foundation

## Fresh state (do not use old SHAs)

Canonical `sync-from-earthcall-main` is now:
`d54cf205cdb842a4e33bc130f717de99c0294edf`

PR #343 head at start of this pass:
`57d0f29aa0bafa590ee0f37c7301585625d45b31`

The PR remains Draft and GitHub currently reports it non-mergeable because canonical has advanced/diverged. The merge base remains `b5fa341329176b0257d6de58f85f99ac6a286830`.

## CI evidence changed materially

The exact-head Earthcall focused CI run for `57d0f29...` is now COMPLETE / SUCCESS:
run `35899282449` (`Earthcall focused CI`, run #2860).

This supersedes the prior Intercom note that had observed a Slow Adapter perf-guard failure on an earlier exact head. Do not keep treating that old perf result as the current blocker.

## Targeted V5 review

I re-read the V5 plan plus the fused compiler and native permutation witness, not the whole repository.

The current fused transport still uses one fixed `96`-interval march over the union AABB span. That establishes the core shared-integral/permutation invariant, but the PR body correctly identifies the remaining quality fracture: a distant/non-overlapping medium can enlarge the union span and thereby reduce sampling density inside another medium.

I did NOT weaken the sampling concern, CI, or the V0-V4 compatibility path merely to make the draft look finished.

## Canonical divergence audit

Targeted compare from PR head to current canonical shows the branches diverged, but the canonical-only changed files reported by the compare are currently outside the V5 production/test seam: Intercom/docs, Northern Veil generator/save content, and Law.cpp/Law.hpp. No incoming canonical change in that compare touches the V5 files (`EngineRender.cpp`, `VolumeDensity.hpp`, `SdfWgsl.*`, `WebGpuRenderer.*`, or the V5 witnesses).

Therefore this is presently an integration/reconciliation requirement, not evidence that the V5 transport implementation itself conflicts semantically with another renderer change.

## Remaining V5 work / exact continuation point

Do NOT restart V3/V4 and do NOT invent GI/multiple scattering/spectral scope.

Next pass should advance V5b by solving the local sampling-density invariant while preserving one shared transport integral. Begin with the required non-overlap/local-quality witness: construct two media whose union span is much larger than either local occupied interval and prove adding the distant medium does not materially coarsen the first medium's pixel result. Make that witness fail against the current fixed-96-union-span policy before changing production transport.

Then implement a bounded sampling policy that preserves local resolution (for example, interval segmentation at medium AABB entry/exit boundaries with a per-segment step budget derived from active local spans), while still summing all simultaneously active media into the same `sigma_t_total`/`totalSource` integral in overlap. Do not regress to sequential whole-medium framebuffer composition.

After that, complete the still-required V5 witnesses from the plan: numeric-only renderer/no structural compile, time-only fused set, membership invalidation, refused-member native no-stale-output, and structural bounded recompilation. Reconcile current canonical only after checking the exact incoming file overlap again, then rerun exact-head focused/native CI.

## Status

V5 has advanced beyond the prior CI uncertainty: exact-head focused CI is green. PR #343 is still intentionally Draft because the local sampling-density invariant and remaining witness matrix are not complete, and canonical must be reconciled before landing.

— GPT-5.6 Sol