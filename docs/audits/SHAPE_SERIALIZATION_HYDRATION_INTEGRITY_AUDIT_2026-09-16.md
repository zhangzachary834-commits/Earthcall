# Shape Serialization / Hydration Integrity Audit — 2026-09-16

**Author:** GPT-5.6 Sol  
**Session:** ChatGPT account session, 2026-09-16  
**Repair branch:** `sol/shape-hydration-integrity-20260916`  
**Repair PR:** #188  
**Follow-up issue:** #189

## Question

Why can an ambitious authored Earthcall structure manifest after save/load as a much shallower collection of primitive-looking geometry, and which persistence boundaries can change what an authored shape *is*?

## Governing invariant

> **What was authored is what is saved; what was saved is what is hydrated; what was hydrated is what is manifested. Semantic authored form is authoritative. A physical sidecar or derived cache may accelerate manifestation but may never demote, replace, or reinterpret authored form.**

This is more than a mesh-cache concern. Earthcall treats mathematical form, topology and Law-addressable state as world state. Replacing a complete authored SDF with a shallow primitive shell is not merely lower visual quality; it changes what the being is.

## Findings and repair status

### Object semantic JSON — repaired in PR #188

Historically Object JSON persisted `shapeKind` plus a positional `shapeParams` array. Field payloads were semantic, but Patch and custom Polyhedron topology could depend on `.ecmatter`. The reader also let optional `patch` / `field` payload presence outrank the explicit current shape discriminant.

That was unsafe under Zone identity merge-patch: a newer overlay could change Field -> Sphere while an older `field` key survived in the merged JSON. Hydration then resurrected the obsolete Field.

PR #188 now:

- writes a self-describing `shape` arm with named parameters while retaining append-only legacy fields;
- persists complete Field, Patch and custom Polyhedron authored payloads in semantic Object JSON;
- makes the explicit current kind authoritative over stale optional payloads;
- validates persisted ShapeKind ordinals before constructing an enum value.

### `.ecmatter` FlatBuffer — semantic authority boundary repaired in PR #188

The schema can describe richer SDF information, but the current matter writer stores only one Field root and does not serialize a complete recursive boolean/morph tree, Convex planes, or the full OntoMath structure into that root. Matter is applied after semantic JSON, so the old reader could reconstruct a complete semantic Field and then overwrite it with a shallower sidecar record.

The repair now establishes one compatibility rule across topology families:

**matter may recover topology only when the current semantic kind agrees and the semantic representation is genuinely missing. It may not redefine a complete current form.**

Concretely:

- Polyhedron matter applies only to a semantic Polyhedron shell whose vertices/faces are missing.
- Patch matter applies only to a semantic Patch shell whose control net is missing.
- SmoothSurface matter applies only when the current semantic ShapeKind is in the analytic smooth family and smooth topology is missing.
- Field matter applies only to a semantic Field shell with no semantic Field data. Because the current sidecar stores only one SDF root, compatibility recovery refuses non-leaf operators and Convex roots that would require absent children/planes.
- A semantic Field's evaluation extent remains authoritative with its tree; a stale sidecar cannot clip or expand it after its topology has been refused.

`Object::setFieldShape` keeps an additional defensive boundary for callers outside `ZoneManager`: a structurally incomplete operator / Expr / Convex shell may not demote an already-complete Field, and rejection no longer mutates its extent.

### Matter semantic validation — repaired in PR #188

FlatBuffers verification proves memory structure, not valid geometry. Before this repair a structurally valid buffer could still contain dangerous or nonsensical values.

The reader now validates before mutation:

- Polyhedron vertices are finite.
- `face_offsets` must start at zero, end exactly at `face_data.size()`, remain monotone/in-range, and describe faces of at least three vertices.
- every Polyhedron face index must be within the vertex array.
- SDF primitive/operator ordinals are range-checked before enum conversion.
- SmoothSurface model/form/parametric-kind ordinals are range-checked before enum conversion.
- SmoothSurface matrix, axes, trims and parameters must be finite.
- Field dimensions/offset/scalars and extents must be finite; extents must be positive.

Malformed or lossy legacy matter is skipped and logged rather than guessed into a being.

### Historical `Sphere + expr` representation — defended in PR #188

A historical Law spawn path created a Sphere leaf and attached an expression string. `Object::setFieldShape` now treats the expression as mathematical shape truth, promotes the leaf to `SdfPrim::Expr`, and compiles executable RPN when needed.

The producer itself still deserves later cleanup to call `geom::makeImplicit` directly. The important persistence/runtime boundary is nevertheless safe: malformed historical input no longer remains a Sphere merely because its producer encoded it that way.

### ObjectConcept member templates — repaired in PR #188

`ObjectConcept::MemberTemplate` had a separate shape codec whose historical reader required exactly nine ShapeParams slots. Shape2D/Text2D width and height therefore vanished across concept save/load.

Expanding that legacy array to eleven would break older readers. The compatibility-safe repair keeps `params` exactly nine entries wide and adds `width2D` / `height2D` as named additive fields. New readers accept historical nine-slot records, the brief eleven-slot development form, and the named fields, with named fields taking precedence. Member ShapeKind ordinals are validated.

### BodyPart / composite sub-object hydration — repaired in PR #188

Body serialization independently direct-cast persisted integers for the primary shape and nested sub-object shapes. Both boundaries now validate the append-only ShapeKind range and refuse invalid/future values deterministically to Cube.

### Cathedral generator contract — repaired narrowly in PR #188

The generator encoded Sphere/Torus size both in analytic ShapeParams and in transform scale, multiplying dimensions at manifestation time. Analytic Sphere/Torus transforms now carry pose only. Generator output paths are repository-relative rather than tied to one developer machine.

This does not manufacture architecture the generator never authored. The generator still composes substantial parts of the Cathedral from ordinary primitives, and its current `spatialRoot` does not contain the manifesto's exact standing-wave OntoMath AST. Persistence can preserve only actual authored form.

## Regression evidence

`tests/constructed-being/shape_hydration_integrity_test.cpp` now covers both positive compatibility and adversarial refusal through the real `ZoneManager` matter path:

1. complete semantic SmoothUnion surviving real matter hydration;
2. semantic Field tree and extent surviving stale matter;
3. stale Patch matter refusing to turn a current Sphere into a Patch;
4. stale Polyhedron matter refusing to turn a current Sphere into a Polyhedron;
5. stale SmoothSurface matter refusing to split Torus identity from runtime smooth topology;
6. valid legacy recovery for Patch, Polyhedron, leaf Field and SmoothSurface shells when semantic kind agrees and topology is absent;
7. negative/non-monotone Polyhedron offsets and out-of-range face indices being rejected;
8. invalid SDF and SmoothSurface enum ordinals being rejected;
9. malformed implicit-expression normalization and executable evaluation;
10. stale JSON Field payload vs newer Sphere discriminant;
11. invalid Object/ObjectConcept/BodyPart ShapeKind ordinals;
12. named parameter precedence and semantic-only Patch/Polyhedron round trips;
13. ObjectConcept nine-slot, additive named-field and brief eleven-slot compatibility.

Focused CI also builds and runs `object_roundtrip_test` and `matter_semantic_precedence_test`, preserving earlier matter/address/paint invariants while this topology rule changes.

## Remaining correctness edges — intentionally outside PR #188

### A. RoundedBox needs an explicit size contract

`Object::setShape(RoundedBox, p)` still calls `roundedBox(0.5f, p.fillet)`. Current ShapeParams defines fillet but no explicit rounded-box size/half-extent. Reinterpreting `r` silently would create another ambiguous persistence contract. Add an append-only named size/half-extent parameter, expose it to the Property/Law surface, then consume it in the constructor.

### B. Remaining integer-to-geometry boundaries should be classified by role

Object, ObjectConcept, BodyPart and matter topology readers are checked in PR #188. Other persisted law/physics geometry filters should receive the same treatment when audited. UI/live-selection casts that already prove the range are a different category and should not be mechanically rewritten.

### C. Cathedral artifact and exact authored mathematics remain separate work

The tracked Cathedral Zone identity was empty at this branch point even though the World package contained Cathedral data. The generator path is corrected, but this session intentionally did not overwrite/regenerate a committed authored Zone artifact without verification.

Separately, the standing-wave OntoMath zero-set must actually be authored into the Zone if that is the intended architecture. This is authoring/content debt, not a serialization excuse.

### D. End-state matter architecture can be stricter still

PR #188 turns topology payloads into runtime compatibility recovery instead of competing authority. The cleaner long-term design is one of:

- semantic topology disappears from `.ecmatter` entirely and matter contains only disposable derived/physical data; or
- derived topology carries a canonical semantic-form revision/hash and is discarded/regenerated on mismatch.

That migration can happen append-only without forcing old saves to stop loading.

## Architectural conclusion

The repaired direction is:

**Person-authored semantic Form -> deterministic lowering / compatibility cache -> manifestation.**

The arrow does not run backward. A cache may help Earthcall realize a being efficiently. It does not get to tell Earthcall what that being was.
