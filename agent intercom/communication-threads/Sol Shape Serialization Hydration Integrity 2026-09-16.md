# Sol Shape Serialization / Hydration Integrity — 2026-09-16

**Author:** GPT-5.6 Sol  
**Session:** ChatGPT account session, 2026-09-16  
**Branch:** `sol/shape-hydration-integrity-20260916`  
**PR:** #188

## Why this exists

Zach showed live Zones in which ambitious authored structures manifested as a sparse set of cubes, cylinders, spheres and rings. The audit found several independent persistence boundaries where authored geometric meaning could become a shallower representation.

The governing invariant for this repair is:

> **Semantic shape truth is authoritative. Physical sidecars and derived caches may accelerate it, but may never demote or overwrite it.**

## Landed in this branch

### 1. Object semantic JSON carries authored form, not only a positional recipe

`ObjectSerialization.cpp` writes a self-describing `shape` arm with named parameters while retaining the append-only legacy `shapeKind`, `geometryType`, and positional `shapeParams` fields. New reads prefer named semantic parameters.

Complete Field trees, Bezier Patch control nets, and custom Polyhedron vertices/faces now survive in semantic Object JSON, so those forms no longer depend on `.ecmatter` merely to remember what they are.

### 2. Explicit current shape beats stale optional JSON payloads

Zone identity merge-patch can retain an old optional key when a newer overlay simply omits it. The old Object reader let `patch` / `field` payload presence outrank the explicit current kind, so an obsolete Field could resurrect over a newer Sphere.

The declared current kind now wins. Payload inference remains only for legacy records that truly have no discriminant.

### 3. `.ecmatter` topology is now compatibility recovery, not semantic authority

The FlatBuffer is still append-only and still carries Polyhedron, Patch, SmoothSurface and Field payloads for old saves, but `applyMatterFlatBuffer` no longer lets those payloads redefine an already-authored representation:

- Polyhedron matter is admitted only when the semantic kind is still Polyhedron **and** semantic vertices/faces are missing.
- Patch matter is admitted only when the semantic kind is still Patch **and** the semantic control net is missing.
- SmoothSurface matter is admitted only when the current semantic ShapeKind belongs to the analytic smooth family **and** its smooth topology is missing.
- Field matter is admitted only when the semantic kind is still Field **and** no semantic Field is present. Because today's matter format stores only one root, legacy recovery refuses non-leaf boolean/morph roots and Convex roots whose required children/planes are absent.

This closes the previous split-brain paths where stale Patch/Polyhedron matter could change ShapeKind and stale SmoothSurface matter could change runtime topology without changing the reported kind.

### 4. Field extent is semantic with the Field

A stale sidecar may no longer mutate the evaluation extent of a semantic Field whose tree it was not allowed to replace. `Object::setFieldShape` likewise leaves both tree and extent untouched when refusing a lossy incoming shell.

### 5. Matter topology input is semantically validated

FlatBuffers verification proves buffer structure, not valid geometry. The reader now validates before mutation:

- Polyhedron vertices must be finite.
- face offsets must form an exact, monotone, in-range partition of `face_data` and every face must contain at least three vertices.
- every face vertex index must be within the vertex array.
- SDF primitive/operator ordinals are range-checked before conversion.
- SmoothSurface model/form/parametric-kind ordinals are range-checked, and its matrix/axes/trims/params must be finite.
- Field extents must be finite and positive.

Malformed or lossy legacy matter is logged and skipped rather than guessed into a being.

### 6. Historical malformed implicit forms are normalized at the Object boundary

A historical Law spawn path could produce a Sphere leaf carrying an expression string. `Object::setFieldShape` recognizes the expression as the actual form, promotes it to `SdfPrim::Expr`, and compiles executable RPN when necessary. An expression no longer survives merely as decorative text on a Sphere.

### 7. ObjectConcept preserves 2D ShapeParams without breaking old readers

`ObjectConcept::MemberTemplate` historically serialized exactly nine ShapeParams slots, so Shape2D/Text2D width and height vanished. Simply expanding the array to eleven would break older readers that require `size() == 9`.

The branch therefore keeps the legacy array exactly nine entries wide and adds `width2D` / `height2D` as named additive fields. New readers accept historical nine-slot records, the brief eleven-slot development representation, and the named fields. Persisted member ShapeKind ordinals are validated before admission.

### 8. BodyPart persisted ShapeKinds are checked

Primary BodyPart and nested sub-object shape ordinals no longer direct-cast arbitrary saved integers. Invalid/future values refuse deterministically to Cube.

### 9. Cathedral generator shape-contract correction

The generator encoded Sphere/Torus size in both analytic ShapeParams and transform scale, multiplying dimensions twice at manifestation time. Analytic Sphere/Torus transforms now carry pose only, and generator output paths are repository-relative.

This does **not** synthesize architecture the generator never authored: its current content remains substantially primitive-composed, and its `spatialRoot` still does not contain the manifesto's exact standing-wave OntoMath AST.

## Regression witness

`tests/constructed-being/shape_hydration_integrity_test.cpp` now exercises the real writer/reader boundary and adversarial input, including:

- semantic SmoothUnion followed by real `.ecmatter` hydration;
- authored Field tree **and extent** surviving stale matter;
- stale Patch, Polyhedron, and SmoothSurface matter refusing to reclassify or split runtime topology from semantic identity;
- positive legacy recovery for Patch, Polyhedron, leaf Field, and SmoothSurface shells when the semantic kind agrees and topology is genuinely missing;
- negative/non-monotone face offsets and out-of-range face indices;
- invalid SDF and SmoothSurface enum ordinals;
- malformed implicit-expression normalization;
- stale JSON Field payload vs newer Sphere discriminant;
- named shape-parameter precedence;
- semantic-only Patch/Polyhedron round trips;
- ObjectConcept legacy/additive/transitional compatibility;
- invalid ObjectConcept and BodyPart shape ordinals.

Focused CI also includes the pre-existing `object_roundtrip_test` and `matter_semantic_precedence_test` alongside this witness so the new topology rule is checked against the older matter/address/paint invariants.

## Still open / deliberately outside PR #188

1. `Object::setShape(RoundedBox, p)` still calls `roundedBox(0.5f, p.fillet)`. `ShapeParams` has no explicit rounded-box size/half-extent contract; do not silently repurpose `r`. Add an append-only named size contract first.
2. Other persisted integer-to-geometry boundaries outside Object/ObjectConcept/BodyPart and the matter reader should be audited by role. UI/live-selection paths that already range-check are not the same bug.
3. The tracked Cathedral Zone identity was empty at the branch point. The generator path is fixed, but this session intentionally did not overwrite a committed authored save without verification.
4. The Cathedral specification's exact standing-wave OntoMath zero-set still needs to be authored into the actual Zone form model; that is content/authoring work, not a persistence workaround.
5. Long-term `.ecmatter` should either stop carrying semantic topology entirely or key derived topology to a canonical semantic-form revision/hash and discard/regenerate it on mismatch. PR #188 establishes the runtime precedence rule while retaining legacy readability.
6. The historical Law spawn source still constructs `Sphere + expr` before the Object boundary normalizes it. The boundary is safe, but the source should eventually use `geom::makeImplicit` directly so malformed representation is not produced in the first place.

Please preserve the invariant above when touching `.ecmatter`, JSON merge, shape constructors, Zone identity stores, or generation scripts. If cache topology and authored form disagree, the cache does not get the final word.
