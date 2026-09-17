# Sol Shape Serialization / Hydration Integrity — 2026-09-16

**Author:** GPT-5.6 Sol  
**Session:** ChatGPT account session, 2026-09-16  
**Branch:** `sol/shape-hydration-integrity-20260916`  
**PR:** #188

## Why this exists

Zach showed live Zones in which ambitious authored structures manifested as a sparse set of cubes, cylinders, spheres and rings. The audit found several independent persistence boundaries where authored geometric meaning could become a shallower representation.

A later reread of the original split-substrate documents caught an important architectural distinction before merge: `.ecmatter` was never intended to be merely a disposable cache. It is Earthcall's binary physical substrate. The repair therefore preserves two authorities instead of collapsing one into the other:

> **Form determines what the being is. Matter gives that Form physical density. Matter may hydrate a matching Form; it may never redefine the Form.**

Dense physical arrays must not be copied back into human/agent semantic parchment just to make persistence reliable.

## Landed in this branch

### 1. Semantic Object state is self-describing without swallowing dense Matter

`ObjectSerialization.cpp` writes a named `shape` arm while retaining the append-only legacy `shapeKind`, `geometryType`, and positional `shapeParams` fields. New reads prefer named semantic parameters.

New semantic writes deliberately **do not** embed Bezier Patch control nets or custom Polyhedron vertex/face arrays. Their semantic ShapeKind survives in Form; their dense topology remains in `.ecmatter`, matching the original split-substrate design. The reader still accepts historical/transitional semantic `patch` and `polyhedron` payloads for backward compatibility.

Field is a deliberate transitional special case. Its compact SDF/OntoMath recipe and evaluation extent remain semantic because the current Matter schema stores only one SDF root and cannot losslessly encode recursive boolean/morph children, Convex planes, or the full mathematical recipe. Dense compiled/sampled field representation still belongs in Matter as that substrate matures.

### 2. Explicit current shape beats stale optional JSON payloads

Zone identity merge-patch can retain an old optional key when a newer overlay omits it. The old Object reader let `patch` / `field` payload presence outrank the explicit current kind, so an obsolete Field could resurrect over a newer Sphere.

The declared current kind now wins. Payload inference remains only for legacy records that truly have no discriminant.

### 3. `.ecmatter` remains physical substrate, constrained by semantic identity

`applyMatterFlatBuffer` now follows the intended hydration direction: semantic skeleton first, physical Matter second.

- Polyhedron Matter injects dense vertices/faces only into a semantic Polyhedron shell that lacks topology.
- Patch Matter injects dense control points only into a semantic Patch shell that lacks topology.
- SmoothSurface Matter may hydrate missing topology only when the exact analytic semantic kind agrees with the Matter model/form.
- Field Matter may hydrate only a semantic Field shell with no semantic Field. Because today's Matter format stores one root, non-leaf operators, Convex roots without planes, and invalid Expr payloads are refused rather than invented.

This prevents stale Matter from turning a Sphere into a Patch/Polyhedron or making runtime topology disagree with the semantic kind, while still allowing the normal split-substrate Patch/Polyhedron path to work.

### 4. Field extent remains coherent with the authored Field

A stale Matter record may not mutate the evaluation extent of a semantic Field whose mathematical recipe it was not allowed to replace. `Object::setFieldShape` likewise leaves tree and extent untouched when refusing a lossy incoming shell.

### 5. Matter topology input is semantically validated

FlatBuffers verification proves memory structure, not geometry validity. The reader now validates before mutation:

- Polyhedron vertices must be finite.
- face offsets must form an exact monotone in-range partition of `face_data` and each face must contain at least three vertices.
- every face vertex index must be within the vertex array.
- SDF primitive/operator ordinals are range-checked.
- SmoothSurface model/form/parametric-kind ordinals are range-checked and the payload's exact analytic kind must agree with semantic ShapeKind.
- SmoothSurface matrix/axes/trims/params must be finite.
- Field dimensions/offset/scalars/extents must be finite; extents must be positive.
- Expr Matter must contain an expression that actually compiles.

Malformed Matter is logged and skipped rather than guessed into a being.

### 6. Historical malformed implicit forms are normalized at the Object boundary

A historical Law spawn path could produce a Sphere leaf carrying an expression string. `Object::setFieldShape` recognizes the expression as the actual form, promotes it to `SdfPrim::Expr`, and compiles executable RPN when possible. The producer itself remains follow-up debt; the boundary is safe.

### 7. ObjectConcept and BodyPart persisted shape boundaries are compatibility-safe

`ObjectConcept::MemberTemplate` keeps the historical nine-slot ShapeParams array exactly nine-wide for old readers and carries `width2D` / `height2D` as additive named fields. New readers also accept the brief eleven-slot development form. Member ShapeKind ordinals are validated.

BodyPart primary and nested sub-object shape ordinals are likewise checked before enum conversion; invalid/future values refuse deterministically to Cube.

### 8. Cathedral generator shape-contract correction

Sphere/Torus size is no longer encoded both in analytic ShapeParams and transform scale, and generator output paths are repository-relative. This does not synthesize the Cathedral's specified standing-wave OntoMath AST; that remains separate authoring work.

## Regression witness

`shape_hydration_integrity_test` now explicitly locks the original split:

- semantic Patch/Polyhedron records retain identity but contain no dense control-net / vertex-face payload;
- the real Matter writer carries that topology;
- semantic hydration creates Patch/Polyhedron shells first;
- real `applyMatterFlatBuffer` then fleshes those matching shells out;
- stale Patch/Polyhedron/SmoothSurface/Field Matter cannot redefine current semantic identity;
- Field tree and extent survive stale Matter;
- hostile Polyhedron offsets/indices, invalid enums, invalid Expr Matter and non-finite data are refused;
- ObjectConcept and BodyPart compatibility/ordinal cases remain covered.

Focused CI also runs `object_roundtrip_test` and `matter_semantic_precedence_test` alongside this witness.

## Still open / deliberately outside PR #188

1. **Pre-existing split debt: dense paint pixels.** `Material::toJson()` still Base64-embeds `FaceTexture` pixels in semantic JSON even though the original architecture assigns dense pixel arrays to `.ecmatter`. Move them without regressing Material identity, copy-on-write paint, or the Basic Pixel Changer precedence fix.
2. **Per-being Matter coherence.** Generation-level `matterGeneration` already couples a root to its Matter file atomically. Add a per-being Matter handle and/or semantic-form revision/fingerprint so a physical payload can prove which Form it belongs to.
3. **Placement duplication needs a deliberate end-state.** Transform/center/axis/target rotation currently exist in semantic Object state as an independently-loadable authoring fallback and in Matter as physical state. Resolve this under the newer Property-Variant graph architecture rather than deleting one side ad hoc.
4. **Field Matter is root-only.** Extend Matter to encode recursive SDF/OntoMath/Convex physical representation losslessly, or store compiled field bytecode/buffers referenced from semantic Form.
5. **RoundedBox** still lacks an explicit append-only size/half-extent contract.
6. Remaining persisted integer-to-geometry boundaries outside Object/ObjectConcept/Body/Matter should be classified by role.
7. The historical Law producer should eventually call `geom::makeImplicit` directly instead of producing `Sphere + expr`.
8. Cathedral artifact regeneration and exact standing-wave authoring remain separate, deliberate work.

Please preserve the domain boundary when touching save code:

**semantic Form / Relations / Laws -> matching physical Matter -> manifestation.**

Matter is not a second ontology. Semantic parchment is not a dumping ground for dense physical arrays.

## Takeover resolution — 2026-09-16

**Author:** Codex (GPT-5)
**Session:** current Codex desktop session, takeover of Sol handoff
**Branch:** `sol/shape-hydration-integrity-20260916`
**Timestamp:** 2026-09-16 22:54:11 PDT

### Exact CI failure

GitHub Actions run `35152461029`, job `104985122991`, checked out merge commit
`69c1d4f39e7c34507a98e28668ad2c1177317048` (PR head `ce6d25e1` merged into the
base). The configure and focused build steps passed. The focused test step ran
18 tests; 17 passed and `object_roundtrip_test` failed before its assertions with:

`object_roundtrip_test: no GL context`

The test's only direct graphics requirement was an unnecessary hidden GLFW window.
This was a CPU workflow running a semantic Object/material round trip, not a
rendering witness.

### Resolution

- Removed the GLFW init/window/teardown from `tests/constructed-being/object_roundtrip_test.cpp`.
  The test now exercises the CPU-authoritative FaceTexture pixels; the existing
  OpenGL renderer guard safely returns no GPU handle when no context exists.
- Replaced the historical law producer in
  `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp` that made a Sphere leaf and
  attached `expr`. It now calls `geom::makeImplicit(expr)` and refuses an
  unparseable expression using the same empty-RPN validation as the
  `field.expr` property bridge.
- Re-read the current `ZoneManager::applyMatterFlatBuffer`, Object setters, and
  ObjectConcept ShapeParams path. The semantic-dominance, Matter validation,
  and 9-slot-plus-named compatibility work already landed; no duplicate changes
  were made.

### Verification

- `cmake --build build --target object_roundtrip_test shape_hydration_integrity_test matter_semantic_precedence_test --parallel 8` — passed.
- `ctest --test-dir build --output-on-failure -R '^(object_roundtrip_test|matter_semantic_precedence_test|shape_hydration_integrity_test)$'` — 3/3 passed.
- `cmake --build build --target shape_generator_law_test --parallel 8` — passed.
- `ctest --test-dir build --output-on-failure -R '^shape_generator_law_test$'` — 1/1 passed.
- The exact 18-test regex was started locally; the repaired object witness and
  the first five following witnesses passed. Local `channel_paths_test` then
  blocked inside GLFW initialization because this desktop session has no GUI
  service. This is separate from the captured CI result, where
  `channel_paths_test` passed; no local headless hang is being claimed as a
  product regression.

### Deliberately deferred

RoundedBox authored-size semantics remain unmodified because no current caller,
documentation, or witness establishes the intended contract. Cathedral saves
were not regenerated. The already-recorded split-substrate follow-ups remain
open: dense paint pixels in `.ecmatter`, per-being Matter/form coherence,
placement duplication, recursive Field Matter, and remaining integer geometry
boundaries.

## Fresh merge-CI retrigger — 2026-09-16

Zach authorized a pragmatic merge once the current merged tree compiles and the substantive shape witnesses remain green so he can test the 3D world-authoring path locally. Base branch commit `ccd55edf21d8e4dd84fc090bfe7073ecb8100aa7` fixes the unrelated new `DomMirrorBridge.cpp` macOS build break by compiling the native WebKit bridge as Objective-C++. This bookkeeping-only edit intentionally retriggers PR CI against that repaired base; no shape semantics or authored save artifacts are changed here.
