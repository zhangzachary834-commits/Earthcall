# SUN UPDATE — PR #329 Rung 1I/J Execution-Green Gate

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Read-first continuity

Continue from `SUN_UPDATE_PR329_Rung1J_Radiance_Zero_Contribution_Proof_2026-09-23.md`. Do not restart Rungs 1A–1J.

This pass began from head `5403d7a663de877d85cf8c27a35bc63ef7914f0f`, which already contained the Rung 1J code and the prior economics/CI audit.

## Concrete result: exact-head CI is green

The previously pending exact execution gate has now completed successfully.

GitHub Actions workflow **#2694**, run id `35824835814`, tested exact head:

`5403d7a663de877d85cf8c27a35bc63ef7914f0f`

The workflow completed with conclusion **success**.

This is the first exact-head green evidence after the Rung 1J radiance theorem algebra and the Rung 1I density theorem authority were both present in the tested tree.

Therefore Rungs **1I and 1J are now execution-green**, not merely implemented/static-audited.

## What this closes

The bounded rendered-field witness now has CI-backed execution evidence for:

- vessel-scoped density theorem authority;
- separate `DensityZeroSupport` and `RadianceZeroContribution` theorem kinds;
- shared canonical zero mathematics without shared semantic authority;
- hostile density -> radiance proof-copy rejection;
- hostile radiance -> density proof-copy rejection;
- exact fallback when theorem authority is absent/stale;
- topology-driven local invalidation;
- authored-child-driven local invalidation;
- partial local re-proof;
- runtime `x/t` movement without semantic/proof rebuild;
- deterministic proof-economics counters emitted by the witness.

No production renderer/WGSL optimization authority was granted by this pass.

## Economics audit

The prior targeted audit derived the expected deterministic tuple from the exact witness control flow:

- `proof_builds=6`
- `proof_invalidations=4`
- `proof_consultations=39`
- `proof_bypasses=29`
- `proof_fallbacks=10`
- `proof_refusals=1`
- `proof_premise_inspections=12`
- `exact_evaluations_avoided=29`

The witness currently prints these counters and asserts the weaker amortization relation `exactEvaluationsAvoided > proofBuilds`.

Because the exact-head workflow is now green, the next targeted code change may safely pin the deterministic tuple as regression assertions without confusing a first-execution failure with a counter-contract failure.

## Current architectural verdict

The proof architecture has crossed the intended Phase-A semantic threshold for the two first theorem families:

```
shared canonical calculation identity
        !=
shared theorem authority
```

and:

```
authored premise mutation
        -> local theorem invalidation/repair
runtime coordinate/time movement
        -> consume preserved theorem
```

This is now executable CI evidence, not only documentation.

## What remains

The specific PR #329 Sun role is **not yet finished**.

Next pass should be narrow:

1. re-read current head/base/CI first;
2. pin the eight exact economics counters in `rendered_field_piecewise_synthesis_test.cpp` as explicit regression assertions;
3. run exact-head CI and investigate any mismatch rather than changing expected values casually;
4. after that green gate, decide whether Phase A is complete enough to open the first **production observation-only** seam (Phase B), where production code records hypothetical theorem bypasses but cannot alter rendered truth;
5. keep future `sigma_t`, `sigma_s`, `C_v`, `Phi`, visibility/material/GI theorem algebras semantically separate even where underlying math canonicalizes.

Do not jump directly from this green bounded witness to production optimization authority.

This Sun role is not finished.


## Successor pass — exact economics contract pinned

This successor pass re-read live PR #329 and confirmed the prior execution-green evidence before changing code. The bounded witness now pins the eight deterministic Rung 1J theorem-economics counters as explicit assertions in `tests/singularity/rendered_field_piecewise_synthesis_test.cpp`.

Code commit: `fa95215ff3d355383f4f57251c20b67c1fcd455c`.

Pinned contract:

- `proof_builds == 6`
- `proof_invalidations == 4`
- `proof_consultations == 39`
- `proof_bypasses == 29`
- `proof_fallbacks == 10`
- `proof_refusals == 1`
- `proof_premise_inspections == 12`
- `exact_evaluations_avoided == 29`

This intentionally converts the earlier derived economics tuple into executable regression authority. Do not relax these values merely to make CI green: a mismatch means the proof lifecycle changed and must be explained.

No production renderer/WGSL behavior changed. The next gate is exact-head focused CI for this assertion commit. If green, Phase A's density/radiance bounded theorem witness is materially closed and the next architectural pass should audit/design the first production **observation-only** seam: production may measure where a theorem *would* bypass work, but rendered truth must still come from the existing exact path until separately authorized.

This Sun role remains active pending that exact-head gate and Phase-B seam decision.
