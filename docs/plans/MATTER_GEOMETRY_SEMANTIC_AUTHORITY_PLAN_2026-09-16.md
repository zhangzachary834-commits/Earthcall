# Matter Geometry Semantic Authority Plan — 2026-09-16

**Author:** GPT-5.6 Sol  
**Session:** ChatGPT account session, 2026-09-16  
**Timestamp:** 2026-09-16 ~12:05 PDT  
**Status:** Implemented for PR #188; long-term cache-fingerprint migration remains follow-up  
**Parent audit:** `docs/audits/SHAPE_SERIALIZATION_HYDRATION_INTEGRITY_AUDIT_2026-09-16.md`  
**Repair PR:** #188  
**Follow-up issue:** #189

## Purpose

Finish the architectural half of the shape-hydration repair without turning `.ecmatter` into a second ontology.

The rule is one-way:

> **Authored semantic Form determines what the Object is. Matter may carry a derived physical/render representation or recover a genuinely incomplete legacy record, but matter never changes the current semantic Form.**

## Decision: Field extent is semantic

`fieldExtent` belongs to the authored Field, not to a disposable topology cache. It is persisted beside the semantic SDF/OntoMath tree and determines the evaluation/manifestation domain. Therefore a sidecar that is not authoritative for the Field tree is also not authoritative for its extent.

Implemented result: if a semantic Field already exists, `applyMatterFlatBuffer` does not call `setFieldShape` at all. `Object::setFieldShape` also leaves the existing extent untouched when it refuses a lossy incoming shell.

## Rung 1 — representation-aware matter boundary — DONE

### Field

Implemented:

- semantic kind must still be `Field`;
- semantic `hasField()` must be false before matter is considered;
- existing semantic Field tree and extent are never replaced by matter;
- current root-only matter can recover a validated leaf only;
- non-leaf operator roots are refused because their children are not present in the current matter representation;
- Convex roots are refused because required planes are not present;
- Expr recovery requires an expression payload.

### Bezier Patch

Implemented:

- semantic kind must still be `Patch`;
- semantic `hasPatch()` must be false;
- an existing semantic control net wins;
- finite, valid matter Patch data may recover a legacy Patch shell;
- stale Patch payload cannot reclassify another current shape.

### Polyhedron

Implemented:

- semantic kind must still be `Polyhedron`;
- existing semantic vertices/faces win;
- validated matter topology may fill a legacy semantic Polyhedron shell;
- `setPolyhedronData` is unreachable from a mismatched sidecar, so stale matter cannot change another current ShapeKind into Polyhedron.

### SmoothSurface

Implemented:

- named analytic smooth kinds continue to reconstruct from semantic ShapeKind + ShapeParams;
- existing semantic smooth topology wins;
- matter is admitted only when the current semantic kind belongs to the analytic smooth family and `hasSmoothSurface()` is false;
- payload presence is never used as a semantic discriminant.

## Rung 2 — semantic validation of matter geometry — DONE

FlatBuffers verification is structural, so the reader now proves Earthcall geometry invariants before mutation.

### Polyhedron

Implemented checks:

1. vertices must be finite;
2. `face_offsets` must contain at least a start/end pair;
3. first offset must be zero;
4. final offset must equal `face_data.size()`;
5. each range must be non-negative, monotone and in bounds before `end - start` is used for allocation;
6. each face must contain at least three vertices;
7. each vertex index must be within the vertex array.

Invalid topology is logged and skipped as a whole.

### SDF

Implemented checks:

- FlatBuffer `type` is range-checked before conversion to `geom::SdfPrim`;
- `operation` is range-checked before conversion to `geom::SdfOp`;
- dimensions, offset, scalar parameters and extent must be finite;
- extent components must be positive;
- lossy root-only representations that cannot reconstruct their required structure are refused.

### SmoothSurface

Implemented checks:

- model, quadric form and parametric kind are range-checked before enum conversion;
- the quadric matrix must contain exactly 16 finite values;
- axes, z trim and parameter values must be finite.

## Rung 3 — adversarial source-precedence regression witnesses — DONE

`shape_hydration_integrity_test` now attacks the real `ZoneManager` writer/reader boundary.

Covered cases include:

- stale Patch over current Sphere;
- stale Polyhedron over current Sphere;
- stale SmoothSurface over current Torus, proving runtime topology cannot split from semantic identity;
- current semantic Field plus stale matter extent;
- direct lossy `setFieldShape` call attempting to mutate semantic extent;
- valid legacy recovery for Patch, Polyhedron, leaf Field and SmoothSurface shells;
- negative/non-monotone Polyhedron offsets;
- out-of-range Polyhedron vertex indices;
- invalid SDF enum ordinals;
- invalid SmoothSurface enum ordinals.

Assertions check public ShapeKind **and** representation flags/data, so a split-brain object cannot pass by keeping only the right label.

Focused CI also includes `object_roundtrip_test` and `matter_semantic_precedence_test` to guard older matter/address/paint behavior while this topology precedence rule changes.

## Rung 4 — stricter long-term matter architecture — FOLLOW-UP

The compatibility boundary is now in place, but the clean end-state remains one of:

### Preferred: matter is purely derived

Semantic Form stores all authored topology and mathematical representation. `.ecmatter` contains only data that can be regenerated. Deleting matter can make loading slower, never semantically different.

### Transitional alternative: key derived matter to semantic Form revision

If topology-shaped cache data remains in matter, add an append-only semantic-form fingerprint/revision to both representations. Matter topology is usable only when the fingerprint matches the already-hydrated semantic form; otherwise it is discarded/regenerated.

That fingerprint proves cache coherence. It does not become a second Object identity.

## Rung 5 — keep setters about geometry, not persistence authority — SATISFIED FOR THIS RUNG

The matter boundary now decides whether a sidecar is allowed to call a topology setter. Setters still defend their own geometry invariants—such as recognizing an authored expression as an Expr—but they are not expected to infer whether a caller is semantic JSON, matter, a Law, Creator Console, or a live Person action.

## Non-goals retained

This plan does not:

- define a new user-authored shape kind;
- reinterpret `ShapeParams.r` as RoundedBox size;
- regenerate the Cathedral save artifact;
- author the Cathedral's standing-wave OntoMath expression;
- remove append-only legacy matter fields before compatibility evidence says it is safe.

Those remain separate contracts. The result of this plan is narrower and foundational:

**a cache cannot rewrite a being.**
