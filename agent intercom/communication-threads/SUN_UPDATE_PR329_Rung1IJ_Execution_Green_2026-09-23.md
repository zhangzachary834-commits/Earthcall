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


## Successor pass — Phase A closure + Phase B observation-only seam audit

This pass re-read live PR #329 before changing anything. Live head at the start was `4378fd6f9cdfb3884d77ff8b8e1c70ebdd52d3f4`; PR remained open, draft, mergeable, and based on `sync-from-earthcall-main`.

### Exact economics gate is now execution-green

Focused workflow **#2710**, run id `35833902456`, is the exact-head run for `4378fd6f...` (the head containing `fa95215f` plus the Intercom record).

The relevant job, **SDF range-proxy verification (macOS)**, completed successfully. Crucially, both `Build SDF proof and GPU parity witnesses` and `Run CPU SDF proof witnesses` completed successfully. That CPU step executes `rendered_field_piecewise_synthesis_test`, so the newly pinned eight-counter economics contract has now executed green on the exact head.

Other unrelated jobs in the same workflow were still running when this audit was recorded. They are not required to establish that the Rung 1J economics assertions themselves executed successfully; nevertheless, do not call the whole workflow complete until GitHub reports it complete.

### Phase A verdict

For the bounded density/radiance theorem family, **Phase A is materially closed**. We now have executable evidence for canonical calculation sharing, channel/vessel theorem sovereignty, local invalidation and re-proof, exact fall-open behavior, runtime x/t reuse without theorem rebuild, hostile cross-channel proof rejection, and an exact deterministic economics regression contract.

No renderer/WGSL optimization authority should be added merely because Phase A closed.

### Phase B seam decision

The next production boundary should be **observation-only**, not bypass-authoritative.

The first production seam should have this shape:

```
existing authored Piecewise / field truth
        |
        +--> existing exact renderer evaluation --------> PIXELS (sole authority)
        |
        +--> diagnostic semantic observer
                 |
                 +--> canonical compilation/cache
                 +--> channel + ValueKind vessel identity
                 +--> structure/value revision provenance
                 +--> theorem build/invalid/reuse accounting
                 +--> hypothetical exact bypass opportunities
```

Constitutional requirements for that seam:

1. The observer's result is **never** substituted for the exact renderer result in Phase B.
2. No theorem state may alter pixels, ray termination, density integration, source accumulation, visibility, or shader control flow.
3. The observer must be opt-in/diagnostic and cheap to disable completely.
4. Compilation/proof rebuilds are driven by authored structure/value revision changes, not ordinary frame/sample/time movement when the theorem quantifies over those runtime inputs.
5. Observation records must preserve `Channel` + `ValueKind` + source/vessel identity even when canonical math nodes are shared.
6. Unknown/unsupported Piecewise operations are recorded as refusal/fallback opportunities, never guessed through.
7. Counters must distinguish compilation/build cost from hypothetical work avoided; no `proof exists == profit` accounting.
8. The first production A/B must compare observer OFF vs observer ON while requiring identical existing exact rendered truth.

### What NOT to do in the next pass

Do not begin with production branch bypasses. Do not wire `DensityZeroSupport` directly into volumetric marching yet. Do not let a radiance theorem skip source evaluation yet. Do not create a giant renderer-global theorem table or perform an O(world) scan per frame.

### Immediate successor gate

The next targeted pass should locate the smallest existing production ownership boundary where authored `OntoMath::Piecewise` rendered fields are already admitted/cached, and introduce or design the observer beside that boundary rather than inside an individual shader effect. Prefer one narrow CPU-side observation seam with explicit revision keys and counters. Preserve the exact renderer path as sole truth authority.

This Sun role remains active: Phase A is closed, but Phase B observation-only integration has not yet been implemented.