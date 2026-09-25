# SUN UPDATE — execution-key successor Rung 6 production provenance projection

Date: 2026-09-24
PR: #369 — Rendering: already-known execution-key direct-dispatch witness
Branch: `sol/already-known-execution-key-consumer-20260924`
Canonical observed/reconciled: `2c581e751c8d53a273257bd088efadb9f358d4ea`
Reconciliation merge: `d89beb9f3bfa30cf4068ff9fc283106d393dc079`
Authoritative code head for this pass: `b36b3d6ec54297614f09321bde754eaef24a0932`
Exact-head focused CI: run `36083812887` / #3214 — queued at handoff-writing time

## Continuity

Same successor, same branch, same draft PR #369. No replacement branch/PR.

This pass re-read current canonical, the original handoff, Rung 5, PR #369, and exact-head CI before changing code.

## Execution-backed witness evidence inherited and verified

The prior docs head `328bedd75d6b67ed51d8cb509e00224dd53512e3` received focused-CI rerun `36074005406` / #3140 after the earlier failed run.

That rerun completed SUCCESS across all four jobs. In particular, the SDF range-proxy verification job completed its `Run CPU SDF proof witnesses` step successfully. The workflow's CPU witness list executes `rendered_field_direct_dispatch_test`, so the direct-dispatch witness is now execution-backed rather than source-audited only.

Its asserted hot-path ledger remains:

- 11 direct slot lookups;
- 44 fixed provenance comparisons;
- 4 exact evaluations avoided;
- 7 exact fail-open fallbacks;
- 0 record scans;
- 0 hierarchy walks;
- 0 hash probes;
- 0 spatial searches;
- 1 full artifact build;
- 1 local repair.

The connector cannot recover the printed build/repair nanoseconds from the huge aggregate job log, so no numeric timing is invented here.

## Production provenance projection implemented

The first narrow production prerequisite is now implemented without theorem consumption.

### RadianceSourceBinding

`RadianceSourceBinding` now retains:

`std::string producerId`

EngineRender assigns it from the already-known admitting `FieldNode::getIdentifier()` before pushing the ordered renderer-facing source binding.

The existing source-set revision string now reuses `source.producerId` rather than independently asking the FieldNode for the same identifier again.

### VolumeDensityBinding

`VolumeDensityBinding` now retains the same provenance field.

`readVolumeDensity(...)` sets it directly from the admitted FieldNode. EngineRender's existing medium-set identity now consumes `medium.producerId`.

This preserves rather than invents identity: both source and medium admission already knew the FieldNode identifier because that identifier was already part of set identity before this rung.

## Why this remains zero-authority

No theorem table, observer theorem, or direct-dispatch action is consulted by production renderer control flow.

The new field is passive provenance only.

No pixel, WGSL, marcher, visibility, source contribution, density, extinction, scattering, chroma, phase, emission, or occluder decision reads `producerId`.

Therefore PR329's zero-authority boundary remains intact.

## Witness tightened to real binding provenance

`rendered_field_direct_dispatch_test` no longer invents producer identity solely inside its test Admission objects.

It now constructs real:

- `Rendering::RadianceSourceBinding`;
- `Rendering::VolumeDensityBinding`;

and derives test Admission provenance from the new production `producerId` plus the real channel revision/expression fields.

The hostile matrix remains unchanged:

- same slot / different producer => exact fallback;
- authored revision change => exact fallback until repair;
- stale artifact generation => exact fallback;
- channel mismatch => exact fallback;
- remove/re-add identity => exact fallback;
- local repair preserves unaffected neighbor generation.

Byte-identical rho/density math remains channel-separated, and density-zero still leaves V4 emission plus the independent occluder lane untouched.

## Real projection test

The existing focused `zone_spatial_field_roundtrip_test` now asserts that `readVolumeDensity` preserves the admitting FieldNode's identifier in `VolumeDensityBinding::producerId`.

Its V5 set-identity checks then use the retained projected identity itself, proving the medium's already-known producer identity survives the real FieldNode -> Screen projection boundary.

The source-side EngineRender assignment is compiled through the normal Earthcall build; no alternate source-discovery path was added.

## Economic note

This rung does add host-side provenance residency to each admitted source/medium binding: one `std::string` object plus its identifier storage.

It does **not** add another `getIdentifier()` derivation in EngineRender's source/medium set construction; the retained value replaces the temporary identifier previously used to build set identity.

That storage cost must be included in the later native A/B accounting. It is not declared free.

## Rejected hypotheses

1. **Create a new authoritative ID subsystem for rendering.** Rejected. The producer identity was already known at admission.
2. **Use slot index as lifetime identity.** Rejected by the existing hostile witness.
3. **Hash producer identity down to a collision-prone value before authority.** Rejected for this rung; exact stable identity is retained.
4. **Let the diagnostic observer's raw Piecewise pointer become identity.** Rejected; lifetime/reuse hazards remain.
5. **Grant authority now because provenance exists.** Rejected. Provenance projection is necessary but not sufficient; exact-head CI and a separately measured native A/B still gate authority.

## Exact next continuation point

Stay on PR #369 / this branch.

1. Re-read exact-head focused CI for code head `b36b3d6e...` / run #3214.
2. If CI is not green, repair this same branch/PR.
3. If green, measure the production provenance overhead explicitly enough to account for host binding residency/build cost.
4. Then build the smallest production **diagnostic-only** aligned artifact whose slot is the already-selected source/medium slot and whose provenance gate consumes `producerId + channel + authored revision + generation`.
5. Keep authority zero while validating lifecycle/incremental repair against real admitted bindings.
6. Only after that real diagnostic substrate is green should a native exact-vs-authoritative A/B be introduced behind an explicit experimental toggle.

Do not skip directly from producer identity to unconditional pixel authority.
