# OntoMath Linear Algebra & Transform Sovereignty — Implementation Plan

**Date:** 2026-09-30  
**Author:** GPT-5.6 Sol ("The Sun") + Zachary Zhang  
**Branch:** `sol/ontomath-linear-algebra-unification-20260930`  
**Base:** `sync-from-earthcall-main`  
**Human direction:** OntoMath is Earthcall's pure mathematics engine. There must not be a competing mathematical system in renderer, physics, geometry, Object motion, camera code, or other C++ substrate. GLM may remain an execution kernel, but it must not remain a second source of mathematical meaning.

**Status:** Rungs 0–3 are implemented on this branch. Rung 0 freezes the pre-unification transform constitution and inventories semantic-origin GLM calls. Rung 1 adds `OntoMath::MatrixValue`, dimension-aware `MathType`, explicit `glm::mat4` bridging, and JSON/msgpack persistence. Rung 2 appends `MathNode::Op` IDs 30–39 for matrix construction, identity, add/sub/scale/multiply, vec3 application, transpose, determinant, and inverse, with CPU evaluation and explicit dimension/non-finite/singular refusal. Rung 3 makes those forms authorable in the shared Math editor, adds a pure editor-initialization seam for witnesses, extends No Black Box coverage to `MatrixValue`, proves registered Matrix properties through the Law get/set bridge, preserves unknown future ops verbatim, and explicitly refuses matrix range proofs until a matrix interval domain exists. Matrix WGSL lowering remains deliberately absent until Rung 4. Exact-head CI for the latest Rung-3 head is pending; the prior Rung-2 head had green Focused CPU/range-proxy/authored-Perlin jobs, with only the unrelated Slow Adapter baseline arm timing out before measurement.

---

## 0. Constitutional invariant

At plan authoring time, Earthcall had the following mathematical sovereignty gap (Rung 1 has now closed the matrix-value/type portion, while transform authorship remains to migrate):

- `OntoMath::MathNode` can author scalar and 3-vector algebra.
- `PropertyValue` can carry `glm::mat4`.
- the OntoMath type system had no Matrix / LinearMap kind; **Rung 1 now provides a dimension-aware Matrix kind/value, while authored matrix operations begin in Rung 2**;
- affine transforms, inverse-transpose normals, camera matrices, quadric transforms, and other matrix mathematics are still originated directly in C++ / GLM and in hand-written WGSL.

The target invariant is:

> **All mathematical semantics belong to OntoMath. Substrates may execute, lower, cache, or serialize OntoMath mathematics, but they may not independently define mathematical truth.**

This is deliberately stronger than "put helper functions in a namespace."

### Allowed

```
authored OntoMath Inverse(A)
        ↓
OntoMath CPU evaluator
        ↓
GLM / numerical kernel computes inverse
```

```
authored OntoMath Transpose(Inverse(M))
        ↓
OntoMath WGSL compiler
        ↓
equivalent WGSL emitted or an exactly equivalent derived value supplied
```

### Forbidden

```
Physics.cpp      decides inverse(M)
Renderer.cpp     decides inverse-transpose(M)
ObjectMotion.cpp decides rotate/scale composition
Camera.cpp       decides lookAt/perspective
WGSL             independently re-states the formulas
OntoMath         cannot represent any of them
```

GLM is therefore permitted as a **backend implementation library**, not as a parallel mathematical language.

---

## 1. Non-negotiable design rules

1. **Semantic ownership before migration.** No call site is "cleaned up" by merely wrapping GLM. The operation must first exist as OntoMath-authored mathematics.
2. **No domain-specific matrix classes.** Do not add `CameraMatrix`, `PhysicsMatrix`, `NormalMatrix`, `ObjectTransformMath`, or other nouns that fracture the mathematical vocabulary.
3. **Dimension is part of mathematical type.** A matrix is not "sixteen floats." Matrix dimensions must be legible to type checking.
4. **Refuse impossible mathematics.** Singular inverse, dimension mismatch, malformed construction, or unsupported backend lowering must return an explicit refusal / `nullopt`; never identity, zero, stale output, or a guessed result.
5. **CPU and GPU are two evaluators of one authored expression.** Backend choice must not change the mathematical answer.
6. **Caches are derived state only.** `invModel`, normal matrices, view-projection matrices, and similar values may be cached for performance, but the cache cannot become the source of truth.
7. **Serialization preserves authored meaning.** Derived caches need not be serialized as authored mathematical truth.
8. **Existing `MathNode::Op` numbering is append-only.** Current values through `Noise = 29` remain untouched; `Unsupported = 255` remains reserved.
9. **Do not widen the migration by accident.** Complex-number algebra, arbitrary tensor calculus, sparse solvers, and numerical-analysis libraries are later OntoMath extensions unless a migration witness requires them.
10. **Performance is not permission for semantic duplication.** Optimized closed forms are allowed only as proven lowerings of an OntoMath expression.

---

# 2. Migration ladder

```
[Rung 0 — Contract + witnesses]
        |
        v
[Rung 1 — First-class Matrix / LinearMap value and type]
        |
        v
[Rung 2 — Core authored linear algebra + CPU evaluator]
        |
        v
[Rung 3 — Serialization, editor, printing, dependency/range discipline]
        |
        v
[Rung 4 — GPU lowering / backend parity]
        |
        v
[Rung 5 — Affine transform vocabulary in OntoMath]
        |
        v
[Rung 6 — World/object/body/formation migration]
        |
        v
[Rung 7 — Physics, collision, picking, ray-space migration]
        |
        v
[Rung 8 — Camera + renderer + normal transform migration]
        |
        v
[Rung 9 — Quadric and remaining custom matrix algebra migration]
        |
        v
[Rung 10 — GLM semantic quarantine + anti-gift-shop guardrail]
        |
        v
[Later — advanced linear algebra extensions]
```

Each rung must land with witnesses before the next subsystem migration begins.

---

# 3. Rung 0 — Freeze the mathematical contract

## Objective

Define the exact current semantics before changing representation, so the unification cannot silently change Earthcall's world.

## Work

Create tests recording:

- matrix storage / indexing convention currently observable at serialization boundaries;
- multiplication composition order;
- object local -> world convention;
- point transformation convention;
- direction transformation convention;
- normal inverse-transpose convention;
- scale/rotation/translation ordering used by Object motion;
- view matrix convention;
- projection depth convention used by active render paths;
- ray unprojection convention;
- singular transform behavior currently relied upon by callers.

Add a focused audit table of every production `src/` call to:

- `glm::inverse`
- `glm::transpose`
- `glm::translate`
- `glm::rotate`
- `glm::scale`
- `glm::determinant`
- `glm::lookAt`
- `glm::perspective*`
- direct `mat4 * vec4` world/local transform sites

Classify each hit as:

1. **semantic origin** — must migrate;
2. **OntoMath backend implementation** — allowed;
3. **boundary conversion / upload** — allowed;
4. **test reference implementation** — temporarily allowed;
5. **dead/demo/smoke path** — clean separately.

## Required witnesses

- `ontomath_transform_convention_test`
- `ontomath_matrix_refusal_test`
- inventory document committed beside this plan

No production behavior changes in Rung 0.

---

# 4. Rung 1 — First-class Matrix / LinearMap semantics

## Objective

Make matrices mathematically legible to OntoMath rather than merely storable by `PropertyValue`.

## New pure value

Add:

```
src/Singularity/OntoMath/LinearAlgebra.hpp
src/Singularity/OntoMath/LinearAlgebra.cpp
```

Introduce a dimension-aware pure value, provisionally:

```cpp
namespace OntoMath {

struct MatrixValue {
    std::size_t rows;
    std::size_t cols;
    std::vector<double> elements; // canonical logical ordering, documented
};

}
```

The semantic type must not be named `Mat4`. Four-by-four is one important instance, not the ontology.

The implementation may later add compact small-matrix storage without changing authored semantics.

## Type system

Extend the OntoMath type judgement so it can distinguish:

- Scalar
- Vector
- Matrix
- ScalarField
- VectorField
- Unknown

A bare `ValueKind::Matrix` is insufficient for multiplication checks, so introduce dimension metadata in the type result rather than hiding it in runtime values.

Provisionally:

```cpp
struct MathType {
    ValueKind kind;
    std::optional<std::size_t> vectorDim;
    std::optional<std::size_t> rows;
    std::optional<std::size_t> cols;
};
```

Existing vector expressions remain 3D unless explicitly extended later. Do not break old saves merely to generalize vectors in this rung.

## PropertyValue bridge

Allow `PropertyValue` to carry the OntoMath matrix value.

During migration, legacy `glm::mat4` properties may coexist at storage boundaries, but OntoMath must have an explicit lossless adapter:

```
glm::mat4 <-> OntoMath::MatrixValue(4,4)
```

That adapter is a representation bridge, not a second mathematical definition.

## Required witnesses

- construction and indexing round trip;
- exact 4x4 GLM bridge round trip;
- dimension metadata survives JSON/msgpack property round trip;
- malformed matrix shape refuses;
- old saves containing `glm::mat4` still load.

---

# 5. Rung 2 — Core authored linear algebra

## Objective

Make the mathematical operations themselves authorable and evaluable inside OntoMath.

Append new `MathNode::Op` values after 29. Exact numeric assignments are fixed when implementation begins and then never renumbered.

Required semantic operations:

- matrix construction;
- identity matrix;
- matrix addition / subtraction;
- scalar × matrix;
- matrix × matrix;
- matrix × vector where dimensions permit;
- transpose;
- determinant;
- inverse.

Strongly preferred in this rung if they simplify callers:

- solve `Ax=b` as an authored operation;
- trace;
- matrix component access.

Do **not** add `Translate`, `CameraPerspective`, or `NormalMatrix` as primitive matrix ops yet. Those are derived constructions in later rungs.

## Refusal semantics

- dimension mismatch -> refusal;
- determinant on non-square matrix -> refusal;
- inverse on non-square matrix -> refusal;
- inverse on singular / numerically non-invertible matrix -> refusal with one shared threshold policy;
- NaN / non-finite inputs -> refusal unless OntoMath already defines a different explicit policy.

Never return identity for failed inverse.

## CPU evaluator

The CPU evaluator may use GLM for supported fixed-size matrices or an internal numerical kernel. The important boundary is:

> only OntoMath implementation code chooses what `Inverse`, `Transpose`, `MatMul`, etc. mean.

Domain callers never call the numerical kernel directly to originate those meanings.

## Printing / legibility

Every matrix expression must have a human-legible `MathNode::print()` form.

Examples:

```
transpose(M)
inverse(M)
A * B
A * v
identity(4)
matrix4(...)
```

No opaque "mat op 33".

## Required witnesses

- hand-computable 2x2 and 3x3 examples;
- 4x4 affine examples;
- associativity witness within floating tolerance;
- determinant/inverse identities where defined;
- singular inverse refusal;
- dimension mismatch refusal;
- authored JSON -> AST -> print -> evaluate round trip.

---

# 6. Rung 3 — Serialization, editor, authoring and inspection

## Objective

A mathematical operation that only C++ can construct is not yet fully Earthcall-native.

## Serialization

Extend `MathNode::toJson/fromJson` and unknown-op preservation for the new operations.

Old builds encountering future ops must continue preserving unsupported payloads rather than destroying authored law text.

## Math editor

Extend `src/Singularity/Screen/MathEditors.cpp` with matrix operations.

The editor must expose:

- matrix dimensions;
- matrix element expressions;
- multiplication;
- transpose;
- inverse;
- determinant;
- identity.

No renderer-specific names.

## Law / property reachability

Where a MatrixValue is registered as a property, make it legible to Law read/write paths at the same level of governance as existing scalar/vector values, subject to the normal property-authority rules.

## Required witnesses

- editor-created matrix AST round trip;
- save/reload identity;
- unsupported future op survives round trip;
- property read/write round trip;
- No Black Box test extended to MatrixValue.

---

# 7. Rung 4 — WGSL / GPU lowering

## Objective

The GPU becomes another evaluator of OntoMath matrix expressions, not a second author of matrix mathematics.

## Rules

- `SdfWgsl.cpp` or its successor lowers new matrix ops from the AST.
- WGSL native matrix operators may be emitted where their semantics match exactly.
- Where a GPU language lacks a required primitive, generate equivalent code or provide an exact derived value produced from the same OntoMath expression.
- unsupported dimensions/operations refuse compilation explicitly.
- numeric parameter edits must not force structural recompilation when existing OntoMath parameterization can preserve structure.

## Important distinction

Hand-written WGSL may still *execute* a matrix expression that has already been derived from OntoMath.

It may not independently invent:

```
normal = transpose(inverse(model))
```

unless that emitted expression is the lowering of the corresponding OntoMath tree / standard function.

## Required witnesses

- CPU/WGSL matrix multiply parity;
- CPU/WGSL transpose parity;
- CPU/WGSL inverse parity for supported matrices;
- refusal parity;
- no stale GPU result after authored matrix parameter change.

---

# 8. Rung 5 — Canonical affine mathematics

## Objective

Define translation, scaling, rotation, affine composition, point/direction application, and normal transformation **inside OntoMath**.

These should be built from the general linear algebra, not become a second special-purpose math engine.

Canonical functions should include equivalents of:

- identity affine transform;
- translation;
- non-uniform scale;
- axis-angle rotation;
- existing Euler composition convention as a named derived function;
- compose;
- transform point;
- transform direction;
- transform normal;
- inverse affine transform.

Point and direction must not be conflated:

```
point      -> homogeneous w = 1
direction  -> homogeneous w = 0
```

Normal transformation is mathematically derived from the linear component:

```
n_world = normalize(transpose(inverse(L)) * n_local)
```

The inverse-transpose formula must exist once as OntoMath meaning.

### Singular normal transform

If the linear component is singular, the normal transform is undefined unless a separately authored fallback exists. It must not silently invent a normal.

## Required witnesses

- translation parity;
- rotation parity;
- non-uniform scaling parity;
- composition order parity with current Object behavior;
- point vs direction translation witness;
- inverse round trip;
- normal under non-uniform scale;
- singular normal refusal.

---

# 9. Rung 6 — Object, Body, Formation and creation migration

**Status (2026-10-03): IN PROGRESS.** Rungs 0–5 remain complete. Rung 6 has migrated Object Euler recomposition, Body default placement, BodyPart dimension/nested affine composition, Formation full/selective inheritance, CreationChannel spawn and CursorSnap rotated-axis transform meaning, ObjectConcept centroid-relative capture/newborn placement composition, and First Mover tool/creation parent-world → local derivation to OntoMath. `creation_tools_test` now pins First Mover rotated + non-uniform-scale world/local parity against a frozen GLM oracle and explicitly witnesses singular-parent refusal. Exact-head CI #4994 on `340c539a81366b7f1cf6ba4dbe5efc81ecb23b9f` passed the OntoMath-owned Focused CPU, SDF range-proxy/WGSL, and authored-Perlin A/B lanes; its overall red was the separate Slow Adapter lane. Law Spawn/Create placement now delegates translation authorship to OntoMath and `action_spawn_test` pins nonzero authored-subject placement through that path. Object and Automation decomposition now delegate scale/Euler extraction to OntoMath; `affineExtractEulerXYZDegrees` preserves the frozen reflected-basis/quaternion convention, and `ontomath_affine_sovereignty_test` includes a reflected non-uniform-scale GLM oracle. Exact-head CI #5086 on `4919367c5ad3fe5ae4f58685a04d98f2eb794623` passed the campaign-owned Focused CPU, SDF range-proxy/WGSL, and authored-Perlin A/B lanes; the overall workflow red is the separately owned Slow Adapter performance lane. A targeted production audit of the Rung 6 surfaces found no remaining direct semantic-origin `glm::translate`, `glm::rotate`, `glm::scale`, `glm::inverse`, `glm::transpose`, or `glm::determinant` calls. **Rung 6 is COMPLETE.**


## Objective

Remove direct transform mathematics from world-being code.

Primary targets include current direct GLM semantics in:

- `ObjectMotion.cpp`
- `Body.cpp` / `BodyPart.cpp`
- `Formation.cpp`
- `CreationChannel.cpp`
- `ObjectConcept.cpp`
- `Automation.cpp`
- First Mover creation/tool code
- Law spawn/placement paths

## Migration rule

Callers ask OntoMath for the transform result or invoke an OntoMath-authored standard function.

They may store the resulting 4x4 representation for hot runtime use.

They may not re-derive the formula locally.

Example target:

```
Object rotation state / authored parameters
            ↓
OntoMath affine expression
            ↓
OntoMath evaluate/lower
            ↓
cached 4x4 runtime representation
```

## Required witnesses

- Object rotation behavior unchanged;
- shape generator placement unchanged;
- Body local/world transform parity;
- Formation nested transform parity;
- save/load round trip;
- Law-authored transform modifications remain reachable.

> **Inert future-rung scaffold (2026-10-01):** `docs/plans/ONTOMATH_RUNGS_7_10_SCAFFOLD_2026-10-01.md` records the already-authorized Rung 7–10 boundaries and future witness surfaces. It is intentionally not wired to production, CMake, or CI and does **not** advance Rung 6 or mark any later rung started.

---

# 10. Rung 7 — Physics, collision, raycast and picking migration

## Objective

Remove the physics-side matrix gift shop.

Primary targets:

- `CollisionDispatcher.cpp`
- `ObjectCollision.cpp`
- `ObjectRaycast.cpp`
- `ObjectEvents.cpp`
- interaction/cursor unprojection paths

Migrate:

- world -> local point transformation;
- local -> world support/normal transformation;
- inverse transforms;
- transpose operations;
- ray unprojection math where it is general matrix mathematics.

Collision algorithms themselves (GJK, support mapping, collision policy) are not "linear algebra" and remain in physics. Only their mathematical transforms move under OntoMath authority.

## Required witnesses

- collision normal parity;
- raycast hit parity;
- transformed SDF signed-value parity;
- picking parity;
- non-uniform-scale collision witness;
- singular-transform refusal behavior is explicit.

---

# 11. Rung 8 — Camera, renderer and shader migration

## Objective

Remove renderer-owned matrix semantics while preserving modality-specific responsibility.

A Screen channel may decide **which** mathematical camera projection to request and which values bind its parameters. It may not privately define the projection formula.

Migrate current direct meanings such as:

- `lookAt`;
- perspective projection;
- view-projection composition;
- inverse view-projection;
- model transforms;
- normal matrices.

OntoMath should define the corresponding mathematical functions. Screen supplies authored/bound camera parameters and consumes their results.

Renderer responsibilities that remain renderer-owned:

- GPU buffer layout;
- transpose/layout conversion required by an API;
- upload;
- batching;
- draw ordering;
- pipeline selection.

Those are representation/modality concerns, not authored mathematics.

## Required witnesses

- camera view parity;
- projection parity;
- world -> clip parity;
- clip -> world unprojection parity;
- native-resolution render image parity;
- SDF normal parity;
- no camera motion regression.

---

# 12. Rung 9 — Quadric and remaining custom matrix algebra

## Objective

Finish the older Geometry-OntoMath promise with the new general linear-algebra substrate.

The current `SmoothSurface.cpp::Quadric` path contains:

```
Q' = transpose(M) * Q * M
```

That expression must become OntoMath-authored mathematics rather than custom GLM algebra followed by conversion into `ScalarForm`.

Two valid representations may coexist if they are mathematically connected:

1. a quadric as a degree-2 `ScalarForm`;
2. a quadric as a symmetric matrix form.

Neither may become an isolated engine.

If both exist, conversion/equivalence must be defined and tested in OntoMath.

## Required witnesses

- sphere/ellipsoid/cylinder/cone/paraboloid parity;
- translated quadric parity;
- matrix-form <-> ScalarForm equivalence;
- gradient/normal parity;
- ray intersection parity.

---

# 13. Rung 10 — Quarantine GLM as backend machinery

## Objective

Prevent the Mathematics Gift Shop from regrowing.

Add a source-level architecture guard over production code.

The guard should flag semantic-origin calls to:

- `glm::inverse`
- `glm::transpose`
- `glm::determinant`
- `glm::translate`
- `glm::rotate`
- `glm::scale`
- `glm::lookAt`
- `glm::perspective*`

outside an explicit allowlist.

Allowed regions should be narrow and named, for example:

- OntoMath numerical backend;
- OntoMath <-> GLM representation adapter;
- renderer API-layout conversion where no mathematical meaning is originated;
- tests intentionally serving as an independent reference oracle.

The guard should fail CI when a new unauthorized semantic call appears.

Do not simply ban `glm::mat4`. Runtime storage, graphics API interfaces, and caches may legitimately use it.

The thing being banned is **unauthorized mathematical authorship**.

## Exit criterion

A repository search for the targeted operations in production code should have every remaining hit explained by one of the allowed categories.

---

# 14. Later OntoMath linear-algebra extensions

These are intentionally **not required to complete transform sovereignty**, but the new type system should leave room for them:

- arbitrary-dimensional vectors;
- general rectangular matrices;
- rank;
- null space / column space;
- linear-system solving;
- LU decomposition;
- QR decomposition;
- eigenvalues / eigenvectors;
- singular-value decomposition;
- orthogonalization;
- least squares;
- sparse matrices;
- complex scalars and complex matrices.

When added, they belong to OntoMath first and receive backend lowerings second.

Do not create a separate "ScientificMath", "PhysicsMath", "RendererMath", or "MatrixUtils" semantic system to add them.

---

# 15. Expected file surface

Likely additions:

```
src/Singularity/OntoMath/LinearAlgebra.hpp
src/Singularity/OntoMath/LinearAlgebra.cpp
tests/singularity/ontomath_linear_algebra_test.cpp
tests/singularity/ontomath_transform_convention_test.cpp
tests/singularity/ontomath_matrix_cpu_gpu_parity_test.cpp
tests/singularity/ontomath_math_sovereignty_test.cpp
```

Likely modifications:

```
src/Singularity/OntoMath/ScalarForm.hpp
src/Singularity/OntoMath/ScalarForm.cpp
src/ConstructedBeing/Singular/Property/PropertyValue.hpp
src/ConstructedBeing/Singular/Property/PropertyValueJson.cpp
src/Singularity/Screen/MathEditors.cpp
src/Singularity/Screen/WebGPU/SdfWgsl.cpp

src/ConstructedBeing/Singular/Object/ObjectMotion.cpp
src/ConstructedBeing/Singular/Object/ObjectCollision.cpp
src/ConstructedBeing/Singular/Object/ObjectRaycast.cpp
src/ConstructedBeing/Singular/Object/Object/ObjectEvents.cpp
src/ConstructedBeing/Singular/Object/ObjectRender.cpp
src/ConstructedBeing/Singular/Object/Geometry/SmoothSurface.cpp

src/Person/Body/Body.cpp
src/Person/Body/BodyPart/BodyPart.cpp
src/Person/Perspective/PersonPerspective.cpp
src/Relation/Formation/Formation.cpp

src/ZonesOfEarth/Physics/CollisionDispatcher.cpp
src/Singularity/Core/CreationChannel.cpp
src/Singularity/Core/EngineRender.cpp
src/Singularity/Input/Interaction/InteractionChannel.cpp
src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp
src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp
```

This is an inventory, not permission for a giant one-commit rewrite. Each rung should remain reviewable.

---

# 16. Required compatibility constraints

The unification must preserve:

- existing saves;
- append-only serialized MathNode op IDs;
- exact authored ScalarForm / field semantics;
- current Object transform convention unless a separately approved migration changes it;
- CPU/GPU parity;
- native-resolution rendering parity;
- collision/raycast behavior;
- law reachability;
- No Black Box requirements;
- unknown/future-op preservation;
- performance-sensitive cacheability.

No save migration may silently reinterpret a matrix's ordering or transform composition.

---

# 17. Performance policy

OntoMath sovereignty does **not** require rebuilding ASTs or dynamically inverting matrices every draw call.

The intended performance architecture is:

```
authored parameters / law state
        ↓ dirty only when premises change
OntoMath expression / compiled proof
        ↓
derived transform + inverse + normal matrix cache
        ↓
renderer / physics hot loops consume cache
```

This matches Earthcall's prophetic/incremental direction:

> derive ahead of time; recompute only when mathematical premises change.

An optimized cache is acceptable precisely because its truth is traceable back to OntoMath.

---

# 18. Completion definition

This campaign is complete when all of the following are true:

1. OntoMath can represent, type-check, serialize, print, and evaluate matrices.
2. Core matrix algebra has CPU witnesses.
3. GPU-supported matrix algebra has CPU/WGSL parity witnesses.
4. Affine transforms are authored/derived in OntoMath.
5. Object/Body/Formation transform composition no longer originates in scattered GLM calls.
6. Physics/collision/raycast space transforms no longer originate in scattered GLM calls.
7. camera/view/projection mathematics is OntoMath-owned.
8. normal inverse-transpose has one mathematical source of truth.
9. Quadric matrix algebra is no longer isolated custom algebra.
10. all remaining production GLM matrix-operation hits are either OntoMath backend execution or representation/channel boundaries.
11. CI prevents a new unauthorized math subsystem from appearing.

At that point, Earthcall does not have "OntoMath plus renderer math plus physics math plus geometry math."

It has **one mathematics, with many faithful execution channels.**
