# Geometry–OntoMath Unification Implementation Plan

**Document:** `docs/architecture/GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md`  
**Date:** August 17, 2026  
**Author:** Gemini Spark + Zachary Zhang  
**Target Subsystems:** `src/ConstructedBeing/Object/Geometry/`, `src/Singularity/OntoMath/`, `src/Singularity/Screen/WebGPU/`, `src/ZonesOfEarth/AuthorsOfLaw/`  
**Prerequisite Audit:** [`docs/audits/GEOMETRY_ONTOMATH_AUDIT_2026-08-17.md`](../audits/GEOMETRY_ONTOMATH_AUDIT_2026-08-17.md)  

**Status as of 2026-08-19:** the six rungs landed on 2026-08-18. This file is the
*plan that was executed*, not a backlog. Empty checkboxes in §4 are historical
— do not implement from them. The executed record, including what was *not*
done the way the plan first said, is
[`Geometry_OntoMath_Remaining_Rungs.md`](../Agenda/Tasks/Specific%20Tasks/Geometry_OntoMath_Remaining_Rungs.md).
The 2026-08-17 audit below it describes the world *before* this work.

What landed differently from the text below:

- `TransFactor::Kind` stayed frozen (`Sin`, `Cos`, `Exp`, `Ln`). Tan / Sqrt / Abs
  live on `MathNode::Op` only (23–28). There is no `Op::Min` / `Op::Max`;
  `Union` / `Intersection` were widened to componentwise min/max instead.
- `emitNode` / `emitRpn` / `evalRpn` were **not deleted**. `emitNode` prefers
  `n.mathNode` then falls back to `emitRpn`; CPU `evalSdf` prefers `MathNode`
  then `evalRpn`. The isolated engine is a fallback, not the source of truth.
- Placement / tool state is on `CreationChannel`, not `@person` (see the
  Shape Generator 3D law audit).

---

## 0. Executive Summary & Ontological Motivation

Earthcall is an **ontology with an engine prototype attached**, not a traditional game engine. In accordance with `AGENTS.md`, `ONTOMATH_FRAMEWORK.md`, `NO_BLACK_BOX.md`, and `LAW_AND_CREATION_SYSTEM.md`:
1. **Separation of Mathematics from Modality:** What something *is* mathematically must be defined in `OntoMath` and be distinct from how a specific channel (WebGPU, CPU marcher, Audio, or Physics) renders or evaluates it.
2. **No Black Box (Refusal 6):** All mathematical structures and parameters defining geometry must be legible, introspectable, and governable by Law text via `PropertyPath`s.
3. **Exact Symbolic Integration & Reversibility:** Mathematics must be represented symbolically (`ScalarForm`, `MathNode`, `Piecewise`) so that time flow ($dp/dt$) can be integrated in closed form without event logs ($p(t-\Delta) = p(t) - \int dp/dt$).

The recent audit ([`GEOMETRY_ONTOMATH_AUDIT_2026-08-17.md`](../audits/GEOMETRY_ONTOMATH_AUDIT_2026-08-17.md)) revealed that the geometry framework (`geom::`) maintains a parallel, isolated mathematical engine from scratch (custom RPN parser/evaluator, quadric matrix algebra, Bernstein polynomials, numerical finite-difference normals, and duplicate WebGPU shader generators).

**Goal of this Plan:** Systematically eliminate the isolated math in `geom::` and unify all geometry representations under `OntoMath` without breaking simulation state, tests, or render pipelines.

---

## 1. Migration Ladder Overview

To prevent breaking the 48-test suite, the migration proceeds across **6 incremental rungs**. Each rung has an explicit exit test and verification requirement.

```
[Rung 0: OntoMath Primitive & Basis Expansion]
       │  (Add primitive SDFs & Bernstein / monomial forms to OntoMath)
       ▼
[Rung 1: Unified Symbolic SDF Engine]
       │  (Deprecate ExprParser/evalRpn in favor of MathNode/ScalarForm)
       ▼
[Rung 2: Bézier Patch Algebraic Unification]
       │  (Express Bézier control nets as bivariate ScalarForm polynomials)
       ▼
[Rung 3: Quadric Manifold Unification]
       │  (Migrate Quadric matrix algebra to degree-2 ScalarForm polynomials)
       ▼
[Rung 4: WebGPU Transpiler Consolidation]
       │  (Purge emitNode/emitRpn; route all shader codegen through emitMathNode)
       ▼
[Rung 5: Law Reflection & Property Path Governance]
          (Expose OntoMath AST roots & variables on Object/FieldNode)
```

---

## 2. Detailed Rung Specifications

### Rung 0: `OntoMath` Primitives & Polynomial Basis Expansion

**Objective:** Equip `OntoMath` with the canonical symbolic constructs needed by 3D geometry before touching `geom::`.

#### 1. Add Geometric Primitive Leaf Operations to `OntoMath::MathNode`
- Add canonical primitive factory helpers to `OntoMath::MathNode` or symbolic expansions into `ScalarForm` / `MathNode`:
  - `MathNode::sphere(const std::string& pointVar, float radius)` $\rightarrow \sqrt{x^2+y^2+z^2} - r$ (or analytic SDF op).
  - `MathNode::box(const std::string& pointVar, glm::vec3 halfExtents)`.
  - `MathNode::cylinder`, `cone`, `torus`, `ellipsoid`.
- Ensure CSG operations (`Union` (Op 20), `Intersection` (Op 21), `Difference` (Op 22), and `SmoothUnion` / `smin`) have identical semantics on CPU and WebGPU.

#### 2. Implement Bernstein $\leftrightarrow$ Monomial Polynomial Conversion in `OntoMath::ScalarForm`
- Move `binom(n, k)` and Bernstein basis expansion from `Patch.cpp` into `Singularity/OntoMath/ScalarForm.hpp` / `.cpp`.
- Add `ScalarForm::fromBernstein(int degree, const std::vector<double>& controlPoints, const std::string& var)`.
- Add `ScalarForm::fromBivariateBernstein(int du, int dv, const std::vector<double>& grid, const std::string& uVar, const std::string& vVar)`.

**Files to Modify/Create:**
- `src/Singularity/OntoMath/ScalarForm.hpp` / `ScalarForm.cpp`
- `src/Singularity/OntoMath/Operations.hpp` / `Operations.cpp`
- `tests/ontomath_test.cpp`

**Exit Test:**
- Unit tests verify exact polynomial equivalence between Bernstein evaluation and expanded `ScalarForm` evaluation for degrees 1 through 5.
- CSG and primitive evaluations in `MathNode` pass exact parity tests.

---

### Rung 1: Unified Symbolic SDF Engine

**Objective:** Replace `geom::SdfNode`'s custom parser and stack VM with `OntoMath::MathNode` and `OntoMath::Piecewise`.

#### 1. Deprecate `geom::ExprParser`, `geom::SdfToken`, and `geom::evalRpn`
- Replace string-based implicit expression compiling (`compileExpr`, `makeImplicit`) with `OntoMath::MathNode::fromJson` or `OntoMath::ScalarForm` construction.
- Replace `geom::evalSdf(const SdfNode&, const glm::vec3&)` with `MathNode::evaluate` or `Piecewise::evaluate` binding ambient point $(p, x, y, z)$.

#### 2. Refactor `geom::SdfNode` as an Alias / Container over `OntoMath::MathNode`
- Embed `std::shared_ptr<OntoMath::MathNode>` as the primary truth inside `geom::SdfNode` (or alias directly).
- Implement backwards compatibility for existing JSON save files by migrating legacy `op`/`prim` JSON fields into `OntoMath::MathNode` representation during deserialization.

#### 3. Exact Surface Normals via `OntoMath` Symbolic / Unified Gradient
- Replace `geom::sdfNormal`'s ad-hoc central differences with `OntoMath::MathNode::Op::Gradient` (sharing `OntoMath::kGradientEpsilon = 1e-3`).

**Files to Modify:**
- `src/ConstructedBeing/Object/Geometry/Sdf.hpp`
- `src/ConstructedBeing/Object/Geometry/Sdf.cpp`
- `src/ConstructedBeing/Object/Geometry/SdfJson.cpp`
- `tests/webgpu_sdf_parity_test.cpp`

**Exit Test:**
- `tests/webgpu_sdf_parity_test.cpp` and `tests/shape_generator_law_test.cpp` build and pass with 100% parity against old evaluations.

---

### Rung 2: Bézier Patch Algebraic Unification

**Objective:** Express freeform Bézier surfaces directly as trivariate polynomial functions $(X(u,v), Y(u,v), Z(u,v))$ using `OntoMath::ScalarForm`.

#### 1. Algebraic Representation of Patches
- Update `geom::BezierPatch` to carry three `OntoMath::ScalarForm` coordinate functions:
  $$X(u, v) = \sum_{k=0}^{du} \sum_{l=0}^{dv} a_{kl}^x u^k v^l, \quad Y(u, v) = \dots, \quad Z(u, v) = \dots$$
- Compute control points $\leftrightarrow$ monomial polynomial coefficients via `OntoMath::ScalarForm::fromBivariateBernstein`.

#### 2. Exact Symbolic Normals and Tangents
- Compute exact surface tangents via symbolic derivatives:
  $$\mathbf{T}_u = \left(\frac{\partial X}{\partial u}, \frac{\partial Y}{\partial u}, \frac{\partial Z}{\partial u}\right), \quad \mathbf{T}_v = \left(\frac{\partial X}{\partial v}, \frac{\partial Y}{\partial v}, \frac{\partial Z}{\partial v}\right)$$
  using `ScalarForm::derivative("u")` and `ScalarForm::derivative("v")`.
- Normal is computed as exact cross product $\mathbf{N}(u,v) = \mathbf{T}_u 	imes \mathbf{T}_v$, eliminating numerical central difference artifacts.

#### 3. Law-Governed Deformation
- Expose polynomial coefficients $a_{kl}$ to the Law system as `PropertyPath`s, allowing Laws to animate or morph Bézier surfaces continuously.

**Files to Modify:**
- `src/ConstructedBeing/Object/Geometry/Patch.hpp`
- `src/ConstructedBeing/Object/Geometry/Patch.cpp`
- `src/ConstructedBeing/Object/Object.hpp`
- `tests/bezier_patch_law_test.cpp`

**Exit Test:**
- `tests/bezier_patch_law_test.cpp` passes all 30 checks, with patch surfaces evaluating identically via both control grid and `ScalarForm` symbolic expansion.

---

### Rung 3: Quadric Manifold Unification

**Objective:** Convert `SmoothSurface.hpp`'s standalone `namespace Quadric` ($4 	imes 4$ matrix algebra) into degree-2 `OntoMath::ScalarForm` signomials.

#### 1. Map Quadric Forms to `OntoMath::ScalarForm`
- Express each quadric canonical equation in `OntoMath::ScalarForm`:
  - **Sphere:** $x^2 + y^2 + z^2 - r^2 = 0$
  - **Ellipsoid:** $\frac{x^2}{a^2} + \frac{y^2}{b^2} + \frac{z^2}{c^2} - 1 = 0$
  - **Cylinder:** $\frac{x^2}{r^2} + \frac{y^2}{r^2} - 1 = 0 \quad (	ext{bounded by } z \in [z_{min}, z_{max}])$
  - **Cone:** $x^2 + y^2 - k^2 z^2 = 0$
  - **Paraboloid:** $a(x^2 + y^2) - z = 0$
- Quadrics with spatial boundaries are represented as `OntoMath::Piecewise` functions.

#### 2. Exact Symbolic Gradient
- Surface gradient $
abla f(x,y,z) = \left(\frac{\partial f}{\partial x}, \frac{\partial f}{\partial y}, \frac{\partial f}{\partial z}\right)$ is evaluated directly via `ScalarForm::derivative()`, replacing manual matrix multiplication $
abla(p^T Q p) = 2 Q p$.

#### 3. Analytic Raycast Solvers
- Extract degree-2 coefficients $(A, B, C)$ by evaluating the symbolic composition $f(o + td)$ along the ray, solving $At^2 + Bt + C = 0$ algebraically.

**Files to Modify:**
- `src/ConstructedBeing/Object/Geometry/SmoothSurface.hpp`
- `src/ConstructedBeing/Object/Geometry/SmoothSurface.cpp`
- `tests/primitive_render_test.cpp`

**Exit Test:**
- All smooth surface tessellations and raycasts maintain floating-point equivalence with legacy implementations.

---

### Rung 4: WebGPU Transpiler Consolidation

**Objective:** Purge duplicate code generation backends in `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`.

#### 1. Delete Legacy Emitter Paths
- Remove `emitNode()` and `emitRpn()` from `SdfWgsl.cpp`.
- Remove standalone `kPrimitives` WGSL string where equivalent `MathNode` operations exist.

#### 2. Route All Shader Codegen through `emitMathNode()` and `emitPiecewise()`
- All geometry trees, SDF fields, and Bézier surfaces compile into WGSL via the unified `OntoMath` AST traversal.
- Retain the strict refusal doctrine: uncompilable or unbound operations in the AST call `e.refuse()` with a clear diagnostic line instead of emitting `0.0`.

**Files to Modify:**
- `src/Singularity/Screen/WebGPU/SdfWgsl.hpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`
- `src/Singularity/Screen/WebGpuRenderer.cpp`
- `tests/webgpu_sdf_parity_test.cpp`
- `tests/webgpu_object_test.cpp`

**Exit Test:**
- WebGPU shaders compile cleanly and pass rendering regression tests without any reference to `geom::SdfToken` or legacy RPN tokens.

---

### Rung 5: Law Reflection & Property Path Governance

**Objective:** Ensure all geometry mathematical parameters are addressable by Law text ("No Black Box").

#### 1. Register Mathematical Subtrees on `Singular`
- In `Object::buildProperties()`:
  - Expose `@object.geometry.math` (`PropertyRef` to `OntoMath::Piecewise` or `MathNode`).
  - Expose parameters (radii, blend factors, polynomial weights) as reachable `PropertyPath`s.
- In `FieldNode::buildProperties()`:
  - Wire full AST reflection for `OntoMath::ScalarField` and `OntoMath::VectorField` properties.

#### 2. Verify Set-to-Set Creation with `ObjectConcept`
- Verify that `ObjectConcept::MemberTemplate` captures and deep-copies `OntoMath` mathematical structures.
- Verify that Laws can spawn, mutate, and reverse geometric forms in-world.

**Files to Modify:**
- `src/ConstructedBeing/Object/Object/ObjectProperties.cpp`
- `src/ConstructedBeing/Object/Geometry/FieldNode.hpp`
- `src/ConstructedBeing/Object/Creation/ObjectConcept.cpp`
- `tests/no_black_box_test.cpp`
- `tests/singular_set_to_set_test.cpp`

**Exit Test:**
- `tests/no_black_box_test.cpp` validates that 100% of geometric shape properties are readable and governable by Law text.
- Full test suite passes: `ctest --test-dir build --output-on-failure -j4` (48 registered, 47 pass, 1 deliberate pending).

---

## 3. Risk Analysis & Mitigation Strategies

| Risk / Failure Mode | Impact | Mitigation Strategy |
| :--- | :--- | :--- |
| **Performance regression in CPU raymarching/tessellation** | Slower interactive editing or marching tetrahedra | Benchmark CPU evaluation before and after; keep fast polynomial evaluation paths in `ScalarForm::evaluate` using pre-indexed variable arrays. |
| **Save file backward incompatibility** | Old saves fail to load | Implement legacy migration deserializers in `SdfJson.cpp` that automatically translate `SdfNode` JSON into `MathNode` ASTs on load. |
| **Shader compilation stalls in WebGPU** | Increased frame hitching when recompiling AST shaders | Preserve the parameter buffer split (`Params.v[i]`) so changing numeric coefficients updates uniform/storage buffers without triggering WGSL recompilation. |

---

## 4. Prioritized Execution Checklist

Historical. All six rungs are done as of 2026-08-18. Deviations are in the
status banner at the top; do not treat an unchecked box here as remaining work.

- [x] **Rung 0: OntoMath Primitives & Basis Expansion**
  - [x] Add geometric primitives (`sphere`, `box`, `cylinder`, `torus`) to `OntoMath::MathNode`.
  - [x] Implement Bernstein $\leftrightarrow$ Monomial basis conversions in `OntoMath::ScalarForm`.
  - [x] Add unit tests in `tests/ontomath_test.cpp` / `tests/geometry_ontomath_test.cpp`.
- [x] **Rung 1: Unified Symbolic SDF Engine**
  - [x] Lift expressions through `rpnToMathNode`; `evalRpn` remains fallback only.
  - [x] `geom::evalSdf` prefers `MathNode::evaluate`.
  - [x] Ground-truth primitive tests (not self-agreeing parity) in `geometry_ontomath_test`.
- [x] **Rung 2: Bézier Patch Algebraic Unification**
  - [x] `geom::BezierPatch` carries 3 coordinate `OntoMath::ScalarForm` polynomials.
  - [x] Replace finite-difference normals with symbolic derivatives ($\mathbf{T}_u 	imes \mathbf{T}_v$).
  - [x] `tests/bezier_patch_law_test.cpp` (30 checks). Symbolic normals via `patchDerivatives()`.
- [x] **Rung 3: Quadric Manifold Unification**
  - [x] `Quadric::toScalarForm` / `fromScalarForm`; matrix path held to the form.
  - [x] Gradient / raycast coefficients checked against `ScalarForm::derivative`.
- [x] **Rung 4: WebGPU Transpiler Consolidation**
  - [x] New ops compile through `emitMathNode` to WGSL builtins.
  - [ ] `emitNode` / `emitRpn` **not deleted** — wrappers that prefer the MathNode. Do not "finish" this by deleting them without a migration of remaining Expr/RPN saves.
- [x] **Rung 5: Law Reflection & Property Path Governance**
  - [x] `field.*`, `patch.*`, `field.ast`, `vectorField.ast` registered.
  - [x] `tests/geometry_ontomath_test.cpp`, `tests/test_field.cpp`.

---

*Plan compiled and saved to `docs/architecture/GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md`.*
