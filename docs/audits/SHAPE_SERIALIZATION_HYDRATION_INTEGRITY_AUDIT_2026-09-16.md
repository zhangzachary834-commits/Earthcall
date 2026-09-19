# Shape Serialization / Hydration Integrity Audit — 2026-09-16

**Author:** GPT-5.6 Sol  
**Session:** ChatGPT account session, 2026-09-16  
**Repair branch:** `sol/shape-hydration-integrity-20260916`  
**Repair PR:** #188  
**Follow-up issue:** #189

## Question

Why can an ambitious authored Earthcall structure manifest after save/load as a much shallower collection of primitive-looking geometry, and which persistence boundaries can change what an authored shape *is*?

## Governing architecture

The original split-substrate design is not “semantic text plus an optional geometry cache.” It assigns different kinds of truth to different substrates:

- semantic Form / Relations / Laws say what the being is and what it means;
- `.ecmatter` is the binary physical substrate for dense geometry and other physical density;
- loading proceeds in ontological order: semantic skeleton first, matching Matter second.

The invariant repaired by this PR is therefore:

> **Form determines what the being is. Matter gives that Form physical density. Matter may hydrate a matching Form, but may never redefine the Form.**

Dense Matter should not be duplicated into human/agent semantic parchment merely to make persistence reliable.

## Findings and repair status

### Object semantic JSON — self-describing without swallowing dense Matter

Historically Object JSON persisted `shapeKind` plus positional `shapeParams`, and the reader let optional `patch` / `field` payload presence outrank the explicit current shape discriminant. JSON merge-patch could therefore leave an obsolete payload behind after the current shape changed.

PR #188 now:

- writes a self-describing named `shape` arm while retaining append-only legacy fields;
- makes the explicit current kind authoritative over stale optional payloads;
- validates persisted ShapeKind ordinals before enum conversion;
- keeps new Bezier Patch control nets and custom Polyhedron vertex/face arrays **out of semantic JSON**. Their semantic kind is written in Form; their dense topology remains in `.ecmatter` as the original split intended;
- continues to read historical/transitional semantic `patch` / `polyhedron` payloads for backward compatibility.

Field is a transitional exception for a concrete reason: the current Matter schema writes only one SDF root. It cannot losslessly represent recursive boolean/morph children, Convex planes, or the complete SDF/OntoMath recipe. The compact mathematical recipe and its evaluation extent therefore remain semantic until Matter can carry the physical/compiled representation without truncation. This is not a license to move arbitrary dense geometry back into semantic text.

### `.ecmatter` FlatBuffer — physical substrate with a semantic admission boundary

Matter is applied after semantic hydration. Before this repair, presence of a Matter topology payload could call a topology setter even when the current semantic representation disagreed, allowing stale Matter to reclassify or split the runtime representation from semantic identity.

PR #188 establishes one rule across topology families:

**Matter may flesh out only a matching semantic representation; it may not choose the representation.**

Concretely:

- Polyhedron Matter injects vertices/faces only into a semantic Polyhedron shell whose topology is absent.
- Patch Matter injects control points only into a semantic Patch shell whose topology is absent.
- SmoothSurface Matter may hydrate missing topology only when the exact semantic analytic kind agrees with the Matter model/form/parametric kind.
- Field Matter applies only to a semantic Field shell with no semantic Field recipe. Because current Matter stores only one root, non-leaf operators, Convex roots without planes, invalid enums, invalid/non-compiling Expr roots and invalid extents are refused.
- A complete semantic Field's evaluation extent remains with its recipe; stale Matter cannot clip or expand it.

This simultaneously preserves the original two-stage load and prevents a stale physical record from becoming a second ontology.

### Matter semantic validation — repaired in PR #188

FlatBuffers verification proves memory structure, not valid Earthcall geometry. The reader now validates before mutation:

- Polyhedron vertices are finite.
- `face_offsets` start at zero, end exactly at `face_data.size()`, remain monotone/in-range, and describe faces with at least three vertices.
- every Polyhedron face index is within the vertex array.
- SDF primitive/operator ordinals are range-checked before enum conversion.
- SmoothSurface model/form/parametric-kind ordinals are range-checked and exact analytic identity is checked against semantic ShapeKind.
- SmoothSurface matrix, axes, trims and parameters are finite.
- Field dimensions/offset/scalars/extents are finite; extents are positive.
- Expr Matter must actually compile before it can hydrate a Field shell.

Malformed Matter is skipped and logged rather than guessed into a being.

### Historical `Sphere + expr` representation — defended in PR #188

A historical Law spawn path created a Sphere leaf and attached an expression string. `Object::setFieldShape` now treats the expression as mathematical shape truth, promotes the leaf to `SdfPrim::Expr`, and compiles executable RPN when needed. The producer itself remains follow-up cleanup.

### ObjectConcept member templates — repaired compatibly

`ObjectConcept::MemberTemplate` had a separate shape codec whose historical reader required exactly nine ShapeParams slots. Shape2D/Text2D width and height therefore vanished across concept save/load.

The compatibility-safe repair keeps `params` exactly nine entries wide and adds `width2D` / `height2D` as named additive fields. New readers accept historical nine-slot records, the brief eleven-slot development form, and the named fields, with named fields taking precedence. Member ShapeKind ordinals are validated.

### BodyPart / composite sub-object hydration — repaired

Body serialization independently direct-cast persisted integers for primary and nested sub-object shapes. Both boundaries now validate the append-only ShapeKind range and refuse invalid/future values deterministically to Cube.

### Cathedral generator contract — repaired narrowly

The generator encoded Sphere/Torus size both in analytic ShapeParams and transform scale, multiplying dimensions at manifestation time. Analytic Sphere/Torus transforms now carry pose only, and generator output paths are repository-relative.

This does not manufacture architecture never authored. The current generator still substantially composes ordinary primitives and its `spatialRoot` does not yet contain the specification's exact standing-wave OntoMath AST.

## Regression evidence

`tests/constructed-being/shape_hydration_integrity_test.cpp` now locks the substrate boundary itself, not merely happy-path round trips:

1. new semantic Patch records retain Patch identity but do not contain the Bezier control net;
2. new semantic Polyhedron records retain Polyhedron identity but do not contain vertices/faces;
3. real `buildMatterFlatBuffer` carries that dense topology and real `applyMatterFlatBuffer` fleshes matching semantic shells out;
4. a complete semantic SmoothUnion and its extent survive stale Matter;
5. stale Patch/Polyhedron/SmoothSurface Matter cannot redefine a newer semantic representation;
6. valid matching Matter hydrates Patch/Polyhedron/leaf-Field/SmoothSurface shells when physical topology is absent;
7. negative/non-monotone offsets and out-of-range vertex indices are rejected;
8. invalid SDF/SmoothSurface ordinals, non-compiling Expr Matter and mismatched analytic kinds are rejected;
9. stale JSON payload precedence, named shape-parameter precedence, ObjectConcept compatibility and BodyPart ordinal refusal remain covered.

Focused CI also builds/runs `object_roundtrip_test` and `matter_semantic_precedence_test` alongside this witness.

## Important pre-existing split-substrate debt discovered while reconciling this PR

These are not introduced by #188, but they matter to the original architecture and are now tracked in #189.

### A. FaceTexture pixels are still Base64 in semantic Material JSON

`Material::toJson()` currently serializes `FaceTexture` pixel buffers as `pixelsB64`. The original split explicitly places dense pixel arrays in Matter. This should move back to `.ecmatter` (or a referenced Matter blob) without regressing Material identity, copy-on-write paint, shared-material semantics, or the Basic Pixel Changer precedence fix.

### B. Placement exists in both semantic state and Matter

Transform/center/axis/target-rotation are currently persisted in semantic Object state as a safe independently-loadable authoring fallback and in Matter as physical state. The newer Property-Variant graph design permits Law-addressable primitive/mat4 properties in semantic state, so this is not automatically wrong; it needs an explicit end-state rather than another blind deletion.

### C. Matter needs per-being coherence beyond generation-level atomicity

The existing `matterGeneration` mechanism couples the semantic root and Matter file with snapshot/hash/length/schema metadata. A future per-being Matter handle and/or semantic-form revision/fingerprint would additionally prove that a dense topology payload belongs to a particular current Form before injection.

### D. Field Matter is still root-only

Current Matter is not yet capable of carrying a complete recursive SDF/OntoMath/Convex physical representation. Extend it losslessly or store compiled field bytecode/buffers referenced from semantic Form.

### E. Other independent follow-ups

RoundedBox still needs an explicit append-only size/half-extent contract; remaining persisted geometry integer boundaries should be classified by role; the historical Law implicit producer should call `geom::makeImplicit` directly; Cathedral artifact regeneration and exact standing-wave authoring remain separate deliberate work.

## Architectural conclusion

The corrected direction is:

**Person-authored semantic Form / Relations / Laws -> matching binary Matter -> manifestation.**

For analytic or compiled accelerations there may also be disposable caches, but `.ecmatter` itself must not be reduced conceptually to “just a cache.” It is the physical half of the split. The crucial safety rule is that physical density may realize a semantic being; it may not decide which being exists.