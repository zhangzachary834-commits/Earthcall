# Sol Shape Serialization / Hydration Integrity — 2026-09-16

**Author:** GPT-5.6 Sol  
**Session:** ChatGPT account session, 2026-09-16  
**Branch:** `sol/shape-hydration-integrity-20260916`  
**PR:** #188

## Why this exists

Zach showed two live screenshots in which ambitious authored Zones manifested as a sparse set of cubes, cylinders, spheres and rings. The immediate audit found that this was not one bug. Earthcall had several independent places where authored geometric meaning could cross a persistence boundary and become a shallower representation.

The governing invariant for this repair is:

> **Semantic shape truth is authoritative. Physical sidecars and derived caches may accelerate it, but may never demote or overwrite it.**

## Landed in this branch

### 1. Object semantic JSON now carries the whole authored form

`ObjectSerialization.cpp` now writes a self-describing `shape` arm with named parameters while keeping the append-only legacy `shapeKind`, `geometryType`, and positional `shapeParams` fields for old readers. New reads prefer named semantic parameters.

Field trees were already semantic; Bezier Patch control nets and custom Polyhedron vertices/faces are now semantic too. A Zone identity can therefore remain independently complete instead of depending on an `.ecmatter` sidecar merely to remember its sculpted topology.

### 2. Explicit current shape beats stale optional payloads

The old reader tested for `patch` / `field` payload presence before consulting the current shape discriminant. JSON merge-patch can retain an older optional payload when a newer overlay simply omits the key. Therefore a being changed from Field -> Sphere could reload as the obsolete Field.

The reader now treats the declared current kind as authoritative. Legacy records with no kind can still infer from payload presence.

### 3. Lossy `.ecmatter` Field shells cannot erase complete semantic SDFs

The current FlatBuffer writer stores only one SDF root and leaves the recursive children / planes / OntoMath structures out. During load, matter is applied after semantic JSON. Previously the shallow root could therefore overwrite a complete already-hydrated tree.

`Object::setFieldShape` now detects structurally incomplete operator / Expr / Convex shells and refuses to demote an already-complete field. Matter may update physical pose/extent; it may not erase the mathematical being.

### 4. Historical `Sphere + expr string` malformed implicit forms are normalized

A law-spawn path historically created a Sphere leaf and merely attached an expression string. The setter now recognizes an expression as mathematical shape truth, promotes the leaf to `SdfPrim::Expr`, and recompiles RPN when necessary. An expression can no longer silently remain decorative text on a sphere.

### 5. ShapeKind ordinals are validated

Persisted shape kinds are append-only integer ordinals. The main Object reader now bounds-checks them and refuses invalid/future garbage deterministically to Cube rather than storing an invalid enum value.

### 6. Adversarial regression witness

New `shape_hydration_integrity_test` covers:

- semantic SDF hydration followed by the **real ZoneManager `.ecmatter` hydration path**;
- SmoothUnion child preservation against a shallow matter root;
- malformed implicit expression normalization and executable evaluation;
- stale Field payload vs newer Sphere discriminant;
- invalid ShapeKind ordinal;
- named semantic parameter precedence over stale positional compatibility data;
- semantic-only Bezier Patch round trip;
- semantic-only custom Polyhedron round trip.

Focused CI now explicitly builds and runs this witness.

### 7. Cathedral generator shape-contract correction

The generator encoded Sphere/Torus size once in analytic `shapeParams` and again in the object transform, so dimensions were multiplied twice at manifestation time. Analytic sphere/torus transforms now carry pose only; their geometric size remains in their geometry recipe. Generator output paths are repository-relative rather than one developer machine's absolute home path.

## Still open / do not silently rediscover

1. `ObjectConcept::MemberTemplate` still persists the historical 9-slot ShapeParams payload rather than all 11 current slots; Shape2D width/height can therefore be lost across a concept save. This branch has **not** repaired that file yet.
2. `Object::setShape(RoundedBox, p)` still calls `roundedBox(0.5f, p.fillet)`, ignoring the authored size parameter. That is a constructor contract defect distinct from serialization.
3. Other secondary readers (for example body-part serialization) still direct-cast persisted shape ordinals and should eventually use the same validated conversion boundary.
4. The tracked Cathedral Zone identity was empty at the branch point even though its World package contains the Zone data. `scripts/generate_cathedral.py` now writes the proper repository-relative Zone path, but the committed generated artifact has not been regenerated by this session.
5. The Cathedral manifesto says its architecture is the exact OntoMath standing-wave zero-set. The current generator's `spatialRoot` still carries only procedural scalar/vector-field parameters, not that actual authored OntoMath AST. That is content authoring debt, not a persistence excuse.

Please preserve the invariant above when touching `.ecmatter`, JSON merge, shape constructors, Zone identity stores, or generation scripts. If a cache and an authored form disagree, discard/regenerate the cache; do not rewrite the being to match the cache.
