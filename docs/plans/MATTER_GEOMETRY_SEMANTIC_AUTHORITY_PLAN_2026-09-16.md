# Matter Geometry Semantic Authority Plan — 2026-09-16

**Author:** GPT-5.6 Sol  
**Session:** ChatGPT account session, 2026-09-16  
**Timestamp:** 2026-09-16 ~12:05 PDT  
**Parent audit:** `docs/audits/SHAPE_SERIALIZATION_HYDRATION_INTEGRITY_AUDIT_2026-09-16.md`  
**Repair PR:** #188  
**Follow-up issue:** #189

## Purpose

Finish the architectural half of the shape-hydration repair without turning `.ecmatter` into a second ontology.

The rule is one-way:

> **Authored semantic Form determines what the Object is. Matter may carry a derived physical/render representation or recover a genuinely incomplete legacy record, but matter never changes the current semantic Form.**

The demonstrated Field failure is only one instance of the larger substrate split. `applyMatterFlatBuffer()` currently runs after semantic Zone hydration and can still call topology setters for Polyhedron, Patch, SmoothSurface and Field. Those setters are not equivalent: Patch and Polyhedron can change `ShapeKind`; SmoothSurface can alter runtime topology while leaving `ShapeKind` unchanged; Field now has a defensive incomplete-shell guard. The boundary therefore needs one explicit policy rather than four accidental setter behaviors.

## Decision: Field extent is semantic

`fieldExtent` belongs to the authored Field, not to a disposable physical cache.

Reasons:

1. It is already persisted in semantic Object JSON next to the SDF/OntoMath tree.
2. It determines the evaluation/manifestation domain. A stale extent can clip an otherwise correct implicit surface or expand work far beyond what was authored.
3. A matter sidecar whose SDF root is too stale/lossy to replace the semantic tree is not credible authority for the domain of that same tree.

Therefore when an Object already has a complete semantic Field, matter must not replace either the Field tree **or its extent**. Matter may supply both only as a compatibility fallback for a semantic record that genuinely lacks a Field payload.

## Rung 1 — make `applyMatterFlatBuffer` representation-aware

Before any matter topology setter runs, inspect the already-hydrated semantic Object.

### Field

- If current semantic `ShapeKind != Field`: ignore matter Field topology entirely.
- If `ShapeKind == Field` and `hasField() == true`: keep semantic tree, extent and cell-size authority; do not call `setFieldShape` from matter.
- If `ShapeKind == Field` and `hasField() == false`: legacy-recovery mode may hydrate a validated matter Field payload.
- An incomplete operator, empty Expr or empty Convex root is never enough to replace a complete semantic Field.

The existing setter guard remains useful as defense in depth, but the matter boundary should stop asking the setter to adjudicate source authority.

### Bezier Patch

- If current semantic `ShapeKind != Patch`: ignore matter Patch payload. A stale sidecar may not turn a Sphere/Cube/etc. back into a Patch.
- If `ShapeKind == Patch` and `hasPatch() == true`: semantic control net wins; skip matter topology.
- If `ShapeKind == Patch` and `hasPatch() == false`: allow validated matter Patch as legacy recovery.
- Validate degrees and control-net cardinality before admission; malformed cache data is discarded, not normalized into a different patch.

### Polyhedron

- If current semantic `ShapeKind != Polyhedron`: ignore matter Polyhedron payload. `setPolyhedronData` must never be reached from a mismatched sidecar because it changes the semantic kind.
- If `ShapeKind == Polyhedron` and semantic vertices/faces are already present: keep them and skip matter topology.
- If `ShapeKind == Polyhedron` but semantic topology is absent: allow a validated matter Polyhedron as legacy recovery.
- Validate offsets and face indices before allocating/constructing faces (Rung 2).

### SmoothSurface

Analytic named forms already reconstruct deterministically from semantic `ShapeKind + ShapeParams` during semantic hydration.

- If semantic hydration produced `hasSmoothSurface() == true`, matter `smooth_data` is derived/cache data and must not replace the semantic surface.
- If the semantic kind is not one of the named smooth forms, ignore stale `smooth_data` entirely.
- Only a legacy record whose semantic form explicitly identifies a smooth representation but lacks reconstructible topology may use matter as recovery.

Do **not** infer the semantic kind from the presence of `smooth_data`; payload presence is not a discriminant.

## Rung 2 — validate matter geometry before construction

FlatBuffers verification proves buffer structure, not Earthcall geometry semantics.

### Polyhedron validation

Before creating any face vector:

1. `face_offsets` must begin at or above zero.
2. Every offset must be monotone non-decreasing.
3. Every offset must be `<= face_data.size()`.
4. Each face range must satisfy `end >= start` before `reserve(end - start)`.
5. Every vertex index in every accepted face must be `0 <= index < vertices.size()`.
6. Faces with fewer than three vertices should be rejected (prefer rejecting the whole matter topology rather than silently changing authored connectivity).

No subtraction used for allocation may occur before those checks.

### SDF enum validation

Validate FlatBuffer integers before casting to:

- `geom::SdfPrim`
- `geom::SdfOp`

Unknown/future values make that matter Field ineligible for legacy recovery. They do not become a different primitive by guess.

### SmoothSurface enum validation

Validate integers before casting to:

- model kind
- form kind
- parametric kind
- any other persisted discriminant that controls evaluator dispatch

Unknown values discard the derived payload. Semantic form remains alive.

## Rung 3 — regression witnesses that attack source precedence

Extend `shape_hydration_integrity_test` (or split a dedicated matter-authority witness if link time becomes excessive) with adversarial cases produced through the real matter path:

1. **Stale Patch over current Sphere:** semantic Sphere remains Sphere and does not acquire a Patch.
2. **Stale Polyhedron over current Sphere:** semantic Sphere remains Sphere and its topology does not become the matter polyhedron.
3. **Stale SmoothSurface over current Cube:** Cube remains flat/polyhedral and does not acquire a hidden smooth runtime surface.
4. **Current semantic Patch plus different matter Patch:** semantic control points survive exactly.
5. **Current semantic custom Polyhedron plus different matter Polyhedron:** semantic vertices/faces survive exactly.
6. **Current semantic Field plus stale matter extent:** both SDF tree and semantic extent survive exactly.
7. **Legacy Field shell with no semantic payload:** valid matter can still recover it.
8. **Legacy Patch/Polyhedron missing semantic payload:** valid matter can still recover them when the semantic discriminant explicitly names that representation.
9. **Negative/decreasing/out-of-range poly face offsets:** matter topology is rejected without allocation blow-up or crash.
10. **Out-of-range poly vertex index:** matter topology rejected.
11. **Invalid SDF/Smooth enum ordinal:** matter payload rejected; semantic Object remains unchanged.

Every test should assert both the public `ShapeKind` and the representation flags (`hasField`, `hasPatch`, `hasSmoothSurface`, topology contents) so split-brain states cannot pass by checking only the label.

## Rung 4 — stop relying on parallel semantic truth long-term

The compatibility boundary above is necessary while historical `.ecmatter` files exist, but it should not become the permanent architecture.

Choose one end-state:

### Preferred: matter is purely derived

Semantic Form stores every authored topology and mathematical representation. `.ecmatter` stores only data that can be regenerated from that Form (compiled/tessellated/physical acceleration data, pose caches where appropriate). Deleting `.ecmatter` may make a load slower, never semantically different.

### Transitional alternative: key matter to semantic Form revision

If matter must continue carrying topology-shaped data, add an append-only semantic-form fingerprint/revision to both sides:

- canonicalize the semantic Form representation;
- hash/revision it at save time;
- write the fingerprint into the matter entity;
- on load, apply derived matter only when its fingerprint equals the already-hydrated semantic Form;
- on mismatch, discard/regenerate matter.

The fingerprint is a cache-coherence proof, not a new identity for the Object.

## Rung 5 — remove duplicated defensive authority once the boundary is proven

After matter source precedence is enforced and the regression suite proves it:

- retain input-normalization in shape setters (e.g. a valid expression is an Expr), because setters should defend their own invariants;
- do not make every topology setter independently guess whether its caller is semantic JSON, matter, a Law, the Creator Console, or a live Person action;
- source authority belongs at the persistence boundary, before the setter call.

This prevents a future caller from being blocked merely because a setter accumulated save-system policy it cannot actually know.

## Non-goals

This plan does not:

- define a new user-authored shape kind;
- reinterpret `ShapeParams.r` as RoundedBox size;
- regenerate the Cathedral save artifact;
- author the Cathedral's standing-wave OntoMath expression;
- remove legacy fields before compatibility evidence says it is safe.

Those are separate contracts. The purpose here is narrower and foundational: **a cache cannot rewrite a being.**
