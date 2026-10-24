# OntoMath Rung 0 — Matrix / Transform Semantic-Origin Inventory

**Date:** 2026-09-30  
**Branch:** `sol/ontomath-linear-algebra-unification-20260930`  
**Purpose:** freeze the pre-unification transform conventions and enumerate the production C++ / GLM sites that currently originate matrix mathematics outside OntoMath.

This document is an **inventory of debt, not an allowlist**. Rung 10 will turn the final, intentionally narrow set of backend/boundary calls into an enforceable allowlist after migration.

## Frozen conventions

The new `ontomath_transform_convention_test` records the currently observable, well-defined conventions:

- GLM matrices are used column-major and indexed `m[column][row]`.
- Object translation is stored in column 3.
- Vectors are transformed as column vectors: `M * v`.
- Object Euler recomposition is `T * Rx * Ry * Rz * S`.
- points use homogeneous `w=1`; directions use `w=0`.
- world/local conversion uses `inverse(M)`.
- normals use `transpose(inverse(L))` for the 3x3 linear component.
- view/projection composition is `P * V`.
- active rendering explicitly distinguishes OpenGL NDC depth `[-1,1]` from WebGPU NDC depth `[0,1]` through `Renderer::zeroToOneDepth()` and `frustumNO/frustumZO`.
- `PersonPerspective` itself currently delegates to `glm::lookAt` / `glm::perspective`.

Existing `object_pose_serialization_guard_test` remains the save/load witness for Object transform/pose state; Rung 0 does not rewrite serialization.

## Singular transforms: deliberately NOT canonized

There is no coherent current Earthcall-level singular-matrix contract to freeze.

Examples:

- raycast and collision paths call `glm::inverse(transform)` directly;
- normal transforms call `glm::inverse(linear)` and then normalize;
- Object/Automation scale extraction separately replaces near-zero extracted scale with `1.0`;
- no shared semantic layer says whether a singular inverse is undefined, zero, identity, stale, or exceptional.

Therefore **Rung 0 does not turn GLM's platform-dependent NaN/Inf behavior into a constitutional rule**. The target already specified by the unification plan stands: a singular authored inverse becomes an explicit OntoMath refusal.

`ontomath_matrix_refusal_test` freezes the related current fact that `PropertyValue` can carry `glm::mat4` while `MathNode` cannot mathematically operate on it.

## Production semantic-origin inventory

Classification:

- **S — semantic origin:** mathematical meaning is currently authored/decided here; must migrate under OntoMath.
- **B — backend/boundary candidate:** may remain after migration only if it is lowering, storage, API layout, or upload of OntoMath-owned meaning.
- **D — demo/smoke:** not production ontology; clean independently and never use as authority.

### Inverse

Current `src/` search: **13 files**.

| Class | File | Current meaning |
|---|---|---|
| S | `Singularity/Screen/GL/GluCompat.cpp` | inverse projection/model for unproject |
| S | `Person/Person.cpp` | parent-world/local body transform conversion |
| S | `Singularity/Core/EngineUpdate.cpp` | object/world to local field operand placement |
| S | `ConstructedBeing/Singular/Object/Object/ObjectEvents.cpp` | inverse view-projection hover ray |
| S | `ZonesOfEarth/Physics/CollisionDispatcher.cpp` | world/local transform + normal inverse-transpose |
| S/B | `Singularity/Screen/WebGPU/WebGpuRenderer.cpp` | cached inverse model + normal matrix; may remain only as derived backend cache |
| S | `ConstructedBeing/Singular/Object/ObjectRaycast.cpp` | world ray -> local ray |
| S | `ConstructedBeing/Singular/Object/ObjectCollision.cpp` | world/local collision + normal transform |
| S | `ConstructedBeing/Singular/Object/ObjectRender.cpp` | baked mesh normal inverse-transpose |
| S | `Singularity/FirstMoverOntology/FirstMoverWindowTools/CursorTools.cpp` | inverse VP cursor ray |
| S | `Singularity/Input/Interaction/InteractionChannel.cpp` | inverse VP interaction ray |
| S | `Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.cpp` | world -> local authored body/tool transforms |
| S | `Singularity/FirstMoverOntology/FirstMoverWindowTools/CreationTools.cpp` | world -> local creation coordinates |

### Transpose

Current `src/` search: **5 files**.

| Class | File | Current meaning |
|---|---|---|
| S | `ZonesOfEarth/Physics/CollisionDispatcher.cpp` | normal inverse-transpose |
| S/B | `Singularity/Screen/WebGPU/WebGpuRenderer.cpp` | normal matrix cache |
| S | `ConstructedBeing/Singular/Object/ObjectCollision.cpp` | normal transform and support-direction dual transform |
| S | `ConstructedBeing/Singular/Object/ObjectRender.cpp` | baked mesh normal transform |
| S | `ConstructedBeing/Singular/Object/Geometry/SmoothSurface.cpp` | quadric transform `M^T Q M` |

### Translation / rotation / scale composition

`glm::translate`: **14 files**.  
`glm::rotate`: **5 files**.  
`glm::scale`: **7 files**.

Primary semantic origins:

| Class | File | Current meaning |
|---|---|---|
| S | `Person/Body/Body.cpp` | authored/default body-part local placement |
| S | `Person/Person.cpp` | Person body-root/world pose |
| S | `Singularity/Core/CreationChannel.cpp` | cursor spawn `T*Rx*Ry*Rz*S` and oriented extent |
| S | `Relation/Formation/Formation.cpp` | inherited/member transform composition |
| S | `Singularity/Screen/CreationWindow.cpp` | creation placement transform |
| S | `ConstructedBeing/Singular/Object/ObjectMotion.cpp` | Object `T*Rx*Ry*Rz*S` recomposition |
| S | `ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp` | Law spawn/placement translation |
| S | `ConstructedBeing/Singular/Object/Automation/Automation.cpp` | automation decomposition/recomposition |
| S | `ConstructedBeing/Singular/Object/Creation/ObjectConcept.cpp` | captured concept centroid transform |
| S | First Mover Tool / Create3D / Assets consoles | tool-side placement and scale |
| B/S | `ConstructedBeing/Singular/Object/ObjectRender.cpp` | static mesh baking; math meaning must become OntoMath-derived |
| S | `ConstructedBeing/Singular/Object/Geometry/SmoothSurface.cpp` | quadric translation |
| D | `Singularity/Screen/WebGPU/smoke_window.mm` | smoke/demo transform only |

`BodyPart.cpp` also composes its local transform with dimensions via `T * scale(dimensions)`; this is a world/body semantic origin even though it does not call `glm::translate`.

### Determinant

Current `src/` search: **2 files**.

- **S** `ObjectMotion.cpp`: reflection detection while extracting Euler rotation.
- **S** `Automation.cpp`: same decomposition/reflection convention.

These become OntoMath decomposition semantics or proven lowerings; they must not remain independent definitions.

### Camera / projection

`glm::lookAt`: **5 files**.  
`glm::perspective`: **3 files**.  
`glm::frustumZO/frustumNO`: active production choice in `EngineRender.cpp`.

| Class | File | Current meaning |
|---|---|---|
| S | `Person/Perspective/PersonPerspective.cpp` | standalone view/projection functions |
| S | `Singularity/Core/EngineRender.cpp` | active view and backend-depth projection |
| B/S | `Singularity/Screen/GL/GluCompat.cpp` | GL compatibility view helper; currently still originates `lookAt` |
| D | WebGPU smoke files | demo camera math |

The renderer is allowed to choose the **channel convention** (e.g. required NDC depth range). OntoMath must own the mathematical projection selected by that convention.

### Direct homogeneous matrix application

Representative production semantic-origin sites using direct `mat4 * vec4` include:

- `EngineUpdate.cpp`
- `CreationChannel.cpp`
- `CollisionDispatcher.cpp`
- `ObjectRaycast.cpp`
- `Formation.cpp`
- `ObjectEvents.cpp`
- `SmoothSurface.cpp`
- `CreationTools.cpp`
- `Object.hpp::getWorldCenter`
- `ObjectCollision.cpp`
- `InteractionChannel.cpp`
- `ObjectRender.cpp`
- `Tool.cpp`
- `WebGpuRenderer.cpp`

The important convention frozen by the witness is not every call site's existence but the semantic split:

- **point:** `M * vec4(p, 1)`
- **direction:** `M * vec4(d, 0)`
- **normal:** `transpose(inverse(linear(M))) * n`

Rungs 6–8 migrate these call sites to OntoMath-owned operations / derived caches.

## Rung 0 exit check

Rung 0 is complete when:

- [x] transform composition convention has an executable witness;
- [x] point vs direction convention has an executable witness;
- [x] inverse world/local convention has an executable witness;
- [x] inverse-transpose normal convention has an executable witness;
- [x] camera/view/projection and NDC depth conventions have an executable witness;
- [x] current matrix invisibility/refusal in OntoMath has an executable witness;
- [x] production GLM semantic-origin sites are inventoried and classified;
- [x] singular legacy behavior is explicitly documented without canonizing numerical garbage;
- [x] production runtime semantics are unchanged by the rung.

Next: **Rung 1 — first-class Matrix / LinearMap semantics.**
