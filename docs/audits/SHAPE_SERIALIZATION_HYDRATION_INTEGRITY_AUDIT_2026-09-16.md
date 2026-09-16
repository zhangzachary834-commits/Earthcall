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

This matters more than an ordinary mesh-cache bug. Earthcall treats mathematical form, topology, Law-addressable properties, provenance, and authored ontology as first-class world state. If persistence replaces a complete SDF/OntoMath being with a shallow primitive shell, the program has not merely chosen a lower-quality rendering; it has forgotten what the being was.

## Persistence surfaces audited

### Object semantic JSON

`ObjectSerialization.cpp` historically persisted `shapeKind` plus a positional `shapeParams` array. Field payloads were semantic, but Bezier Patch and custom Polyhedron topology could depend on `.ecmatter`. The reader also tested optional `patch` / `field` payload presence before the explicit current shape discriminant.

That became dangerous under Zone identity merge-patch: a newer overlay that changed a Field into a Sphere but merely omitted the obsolete `field` key could inherit the older payload. On hydration, payload presence won and resurrected the obsolete Field.

PR #188 changes the contract:

- writes a self-describing `shape` arm with named parameters while preserving append-only legacy fields;
- persists complete Field, Patch, and custom Polyhedron authored payloads in semantic JSON;
- makes the explicit current shape discriminant authoritative over stale optional payloads;
- validates persisted ShapeKind ordinals before constructing an enum value.

### `.ecmatter` FlatBuffer

The FlatBuffer schema has fields for richer SDF data, but the current writer serializes only one Field root and leaves recursive children, convex planes, and OntoMath structures absent. Load applies matter *after* semantic JSON. Before this repair, a semantic SmoothUnion/OntoMath tree could therefore be reconstructed correctly and then overwritten by a shallower matter root.

PR #188 adds a defensive Field boundary in `Object::setFieldShape`: an incomplete operator / Expr / Convex shell may not demote an already-complete semantic Field. The historical `Sphere + expr string` form is also normalized into an actual Expr and its RPN is compiled.

This closes the demonstrated Field corruption path, but it does **not** make `.ecmatter` a fully safe semantic authority. `applyMatterFlatBuffer` still directly applies Polyhedron, Bezier Patch, and SmoothSurface payloads. A stale Patch/Polyhedron sidecar can change ShapeKind; stale SmoothSurface data can change runtime topology while leaving ShapeKind unchanged. That is tracked explicitly in #189.

### ObjectConcept member templates

`ObjectConcept::MemberTemplate` had a separate shape codec. It wrote only the historical first nine `ShapeParams` slots even after Shape2D/Text2D added `width2D` and `height2D`. Therefore a concept could correctly capture a 2D member in memory and lose its authored dimensions merely by crossing concept save/load.

PR #188 now writes all eleven current slots, continues reading historical nine-slot members, and validates member ShapeKind ordinals.

### BodyPart / composite sub-object hydration

`BodySerialization.cpp` independently direct-cast persisted integers for both the primary BodyPart shape and nested sub-object shape. A corrupt or future ordinal could therefore become an invalid enum value even after the main Object reader was repaired.

PR #188 now validates both boundaries and deterministically falls back to Cube on an invalid persisted ordinal. The regression witness exercises both paths.

### Cathedral generator contract

The Cathedral generator encoded Sphere/Torus size twice: once in analytic `ShapeParams` and again in transform scale. That squared/multiplied dimensions at manifestation time. The generator also wrote through one developer machine's absolute path.

PR #188 makes analytic Sphere/Torus transforms carry pose only and makes output paths repository-relative.

This correction must not be mistaken for authoring the Cathedral manifesto's full form. The current generator still composes many ordinary primitives and its `spatialRoot` does not yet contain the exact standing-wave OntoMath AST described by the specification. Perfect persistence can preserve only what was actually authored.

## Regression witness

`tests/constructed-being/shape_hydration_integrity_test.cpp` is intentionally adversarial rather than a happy-path round trip. It covers:

1. real `ZoneManager::buildMatterFlatBuffer` -> semantic hydration -> real `applyMatterFlatBuffer`, proving a shallow matter root cannot erase SmoothUnion children;
2. malformed implicit-expression normalization and executable inside/outside evaluation;
3. stale Field payload vs newer Sphere discriminant;
4. invalid Object ShapeKind ordinal refusal;
5. named parameter precedence over stale positional compatibility data;
6. semantic-only Bezier Patch round trip;
7. semantic-only custom Polyhedron round trip;
8. ObjectConcept eleven-slot round trip and historical nine-slot readability;
9. invalid ObjectConcept ShapeKind refusal;
10. invalid BodyPart primary and nested sub-object ShapeKind refusal.

Focused CI explicitly builds and runs this witness.

## Remaining correctness edges

### A. Non-Field matter topology must become non-authoritative

`applyMatterFlatBuffer` should not be allowed to redefine semantic topology. The end-state should be one of these equivalent invariants:

- matter carries only derived/physical cache state and never authored topology; or
- every derived topology payload is keyed to a canonical semantic-form revision/hash, and a mismatch is discarded/regenerated before it can touch the live Object.

A compatibility bridge may still hydrate legacy objects that genuinely lack semantic topology, but it must never let stale matter overwrite a complete newer semantic form.

### B. Polyhedron matter input needs semantic validation

FlatBuffers structural verification does not imply valid face topology. The current reader should reject negative, non-monotone, or out-of-range `face_offsets` before using `end - start` as a reserve size or traversing `face_data`. Face vertex indices should likewise be checked against the vertex count before constructing the custom polyhedron.

### C. RoundedBox needs an explicit size contract

`Object::setShape(RoundedBox, p)` currently passes hardcoded `0.5f` as the box size and only consumes `p.fillet`. `ShapeParams` currently defines no explicit rounded-box size/half-extent field. Reusing `r` because it happens to exist would silently change the meaning documented for that field. The correct repair is append-only: introduce and expose an explicit rounded-box size/half-extent parameter, persist it, register it to the Law/property surface, then consume it in the constructor.

### D. Remaining enum boundaries should be classified, not blindly replaced

The main Object, ObjectConcept, and BodyPart persisted ShapeKind readers are checked in PR #188. Other integer-to-shape casts should be classified by role. Persisted geometry filters/law targets need the same checked boundary; UI/live-selection paths that already prove `0..Text2D` before casting are not the same persistence bug.

### E. Cathedral generated artifacts and exact authored math remain separate work

The tracked Cathedral Zone identity was empty at this branch point even though the World package held Cathedral data. The generator path is corrected, but this session intentionally did not overwrite/regenerate the committed Zone artifact without verification. Separately, the standing-wave OntoMath zero-set must actually be authored if the desired manifestation is to be that mathematical cathedral rather than a composition of boxes, spheres and tori.

## Architectural conclusion

Earthcall currently has two representations with overlapping geometric authority: semantic Object state and `.ecmatter`. That overlap is the root architectural hazard. The repair in #188 makes semantic truth much harder to lose and closes the demonstrated Field demotion path, but the durable end-state is stricter:

**Person-authored semantic Form -> deterministic lowering/compilation -> disposable physical/render caches.**

The arrow must not run backward. A cache may say how to accelerate a being's manifestation. It may never tell Earthcall what the being was.
