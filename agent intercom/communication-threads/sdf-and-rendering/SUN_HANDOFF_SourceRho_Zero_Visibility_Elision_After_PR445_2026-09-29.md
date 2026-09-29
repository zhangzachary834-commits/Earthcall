# SUN HANDOFF — SourceRho-Zero Direct-Transport Visibility Elision After PR #445

**Date:** 2026-09-29  
**Predecessor:** PR #445 — Rendering: first SourceRho pixel-authority A/B  
**Final analysis:** `docs/analysis/production_sourcerho_authority_ab_after_pr369_2026-09-29.md`  
**Final verdict:** `agent intercom/communication-threads/sdf-and-rendering/SUN_VERDICT_PR445_SourceRho_Production_Authority_AB_2026-09-29.md`  
**Counsel:** `agent intercom/communication-threads/sdf-and-rendering/COUNSEL_To_The_Suns_On_SourceRho_And_The_Shadow_Beneath_It_2026-09-29.md`  
**Sun reply:** `agent intercom/communication-threads/sdf-and-rendering/SUN_REPLY_To_The_Constitutionalist_On_SourceRho_And_The_Shadow_2026-09-29.md`

## Why this successor exists

PR #445 answered its bounded question asymmetrically:

- the already-known execution-key authority mechanism survived hostile real WebGPU pixel-boundary mutation/provenance/lifetime tests;
- bypassing only the literal-zero SourceRho body did **not** earn economic promotion: ~1% directional warmed CPU-wall difference over 12 pairs, with no usable GPU timestamps.

Claude Opus 5.5 / the Constitutionalist then identified the immediate downstream cost that #445 was not allowed to skip: the renderer can still execute `sourceVisibility(...)`, potentially marching up to 192 SDF steps, before multiplying that result by radiance already proven to be zero.

This successor asks whether the **same theorem** may lawfully purchase that larger direct-transport consequence.

This is a new commission. Do not reopen or widen PR #445.

## Bounded successor question

For one already-selected `SourceRho` binding whose authored scalar expression is conservatively proven everywhere-defined literal zero:

> Can the renderer replace only that source's direct-light `sourceVisibility` evaluation with a finite constant, eliminating the shadow march, while remaining exact with respect to the current renderer contract and preserving ambient presence, authored-response behavior, provenance/lifetime fail-open semantics, and complete work/economic accounting?

## Scope

Exactly one authority shape:

- channel: `SourceRho`;
- theorem: everywhere-defined scalar literal zero;
- execution identity: the already-selected ordered radiance-source binding plus stable producer identity;
- authority action: elide **only** that source's direct-transport `sourceVisibility` work, not the source as a whole;
- proposed replacement: a finite visibility constant only after the renderer-owned visibility contract is proven sufficient;
- comparison: exact current direct-transport path vs theorem-authoritative visibility-elision path behind an experimental toggle.

Do not widen to:

- MediumDensity authority;
- arbitrary expressions or sample-position theorem search;
- generic Scene DAG relevance;
- whole-source elision;
- ambient-term elision;
- authored-response algebraic simplification;
- V1–V4 semantic changes;
- Northern-Veil-specific or saved-Zone-specific hacks.

## Required proof before pixel authority

Do not assume the Constitutionalist's algebra is sufficient. Prove the actual implementation contract.

Targeted source audit and native witness must establish:

1. `sourceVisibility` returns only finite, non-negative values under every current path relevant to this consumer:
   - near-source / early escape;
   - AABB miss;
   - ordinary successful march;
   - step exhaustion;
   - any refusal/fallback path actually reachable in emitted WGSL.
2. No other downstream observable in this source loop depends on the actual `pathVisibility` value when SourceRho is proven zero.
3. Replacing visibility with the chosen finite constant preserves the exact path's observable floating-point behavior for the supported domain, including any intended NaN/Inf propagation in independent authored expressions.
4. Ambient presence remains untouched. Zero SourceRho does not authorize erasing the source's ambient contribution.
5. Receiver-response and other independent authored multiplications remain present unless separately proven irrelevant; this commission does not grant that proof.

If any current visibility path can produce a non-finite or otherwise semantically observable value that breaks exact replacement, record that and stop rather than weakening the contract.

## Provenance and lifetime invariants

Reuse the execution-key discipline established by PRs #369 and #445:

- numeric slot is not lifetime identity;
- stable producer identity, authored rho revision, artifact generation, and channel are gates;
- stale/missing/mismatched/unknown authority fails open to exact visibility execution;
- removal/re-addition and slot reorder/reuse do not inherit authority;
- repair remains dirty-slot scoped;
- no spatial/hierarchy/candidate/hash/theorem search enters the hot path.

The theorem may be the same as #445. The authority **consequence** is new and must receive its own hostile real-boundary witness.

## Hostile live-mutation gate

At the real WebGPU authority boundary, exercise at minimum:

- zero -> nonzero authored rho revision;
- producer replacement in same numeric slot;
- stale artifact generation;
- producer removal/re-add;
- slot reorder/reuse;
- experiment OFF/ON structural transition;
- positive recovery after local repair.

Every invalid/stale case must restore the exact `sourceVisibility` path before accepting pixels as evidence.

## Exactness witness

Use a fixture where visibility work actually occurs and one source is proven dark.

Compare exact versus authoritative output at the same authored scene state:

- full pixel/frame hash where practical;
- targeted pixels around occlusion boundaries;
- ambient contribution preserved;
- independent response terms preserved;
- no source disappearance caused merely by zero direct radiance.

If exact bit parity is impossible for a documented reason, do not substitute "looks identical." Explain the numerical contract and stop unless the handoff is explicitly amended by Zach.

## Work accounting — mandatory

PR #445 lacked GPU timestamps. This successor therefore requires deterministic work evidence in addition to wall timing.

In a witness-only instrumentation path, count at minimum:

- `sourceVisibility` invocations per frame;
- visibility SDF steps per frame;
- rho-body evaluations per frame;
- authority decisions;
- proof reads;
- metadata/provenance tests;
- fallbacks;
- authority applications;
- build/repair count and time;
- resident authority bytes.

The instrumentation must not become production overhead when disabled.

The central work statement should be machine-independent, e.g.:

> exact executes N visibility steps for the proven-dark source; authoritative executes 0 for that source while preserving the same pixel result.

## Renderer economics

Also measure, where available:

- CPU submitted-frame wall;
- GPU timestamp/query time if supported;
- shader WGSL bytes;
- compile/pipeline cost;
- parameter/upload bytes;
- cold and warmed behavior;
- mutation/repair frame cost;
- long-run cache/memo behavior.

Use a deliberately GPU-bound fixture if needed, but do not change authored semantics merely to manufacture a win.

Report GPU timing unavailability explicitly.

## Integration gait

Use the same discipline as the prior Suns:

- first run records current `sync-from-earthcall-main` SHA and exact semantic dependencies;
- create one successor branch/PR;
- every later run continues that same branch/PR;
- canonical movement alone is not an integration event;
- reconcile only for real semantic dependency invalidation/overlap or one deliberate final landing reconciliation;
- preserve ancestry;
- never force-push;
- never snapshot-overwrite newer canonical history.

No Big Chungus repository dumps. Targeted reads only.

## Agent Intercom gait

Every run leaves a continuation update containing:

- current exact head and CI;
- stable semantic base;
- semantic-overlap status;
- evidence gathered;
- rejected hypotheses;
- changes made;
- exact next continuation point.

## Interpretation discipline

A large reduction in counted shadow steps is not automatically a material frame win.

A faster frame is not automatically proof of exactness.

A sound SourceRho theorem is not authority over ambient, density, response, or the whole source.

Keep these as separate claims.

## Definition of done

This successor is complete when:

- the actual `sourceVisibility` contract has been proven or has refused the experiment;
- exact-vs-authoritative real WebGPU pixel evidence exists;
- hostile live lifetime/provenance cases fail open;
- direct-key/no-hidden-search accounting remains explicit;
- deterministic visibility work-unit accounting exists;
- complete relevant CPU/GPU/compile/repair/residency economics are recorded;
- one deliberate final canonical reconciliation is performed if landing remains warranted;
- exact-head relevant evidence is rerun;
- final analysis is written under `docs/analysis`;
- final Agent Intercom verdict is written.

If exactness fails: record why and stop.

If exactness passes but economics are not material: record that the expensive consequence still did not pay and stop.

If it wins: state only the narrow promotion consideration earned — zero-SourceRho direct-transport visibility elision under the proven renderer contract. Do not generalize to arbitrary Prophetic Rendering.

At definition of done, disable this successor automation. Do not invent the next scope.
