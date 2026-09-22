# Earthcall Audit: Geometry Framework vs. OntoMath Purity

**Document:** `docs/audits/GEOMETRY_ONTOMATH_AUDIT_2026-08-17.md`  
**Date:** August 17, 2026  
**Auditor:** Gemini Spark  
**Target Repository:** Earthcall (`/Users/zacharyzhang/Documents/GitHub/Earthcall`)  
**Scope:** `../../src/ConstructedBeing/Singular/Object/Geometry/`, `../../src/ConstructedBeing/Singular/Object/`, `src/Singularity/OntoMath/`, `src/Singularity/Screen/WebGPU/SdfWgsl.*`  

> **Historical.** This verdict describes HEAD on 2026-08-17, *before* the six-rung
> unification. It is no longer a description of the tree. The isolated
> `geom::` math engine this document inventories was displaced: primitives and
> CSG live as `MathNode` factories, Bernstein/quadrics/Bézier as `ScalarForm`,
> new ops `Div`/`Pow`/`Abs`/`Clamp`/`Sqrt`/`Tan` (23–28), `evalSdf` / `emitNode`
> prefer the OntoMath AST and keep RPN only as fallback. Executed record:
> [`Geometry_OntoMath_Remaining_Rungs.md`](../Agenda/Tasks/Specific%20Tasks/Geometry_OntoMath_Remaining_Rungs.md).
> Plan (do not implement from its old checkboxes):
> [`GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md`](../architecture/mathematics/GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md).
> Independent primitive probe and Div-guard verification: geometry-ontomath
> intercom, 2026-08-18.

---

## 1. Executive Summary & Verdict

### Audit Question
> *Does the geometry framework in Earthcall rely on `OntoMath` for all pure math functions, or does it have a separate math that it tries to own from scratch?*

### Audit Verdict: **CRITICAL ARCHITECTURAL DIVERGENCE**
**The geometry framework in Earthcall currently does NOT rely on `OntoMath` for its pure math functions.** 

Instead, the geometry subsystem (`geom::` under `../../src/ConstructedBeing/Singular/Object/Geometry/`) has constructed a parallel, isolated, and completely separate mathematical infrastructure from scratch. Except for a single recent wrapper (`FieldNode.hpp`), virtually all algebraic, calculus, polynomial, CSG, parser, evaluator, and trigonometric operations in the geometry framework are implemented via custom, standalone C++ logic and `<cmath>` functions, bypassing `OntoMath` entirely.

---

## 2. Core Invariants & Ontological Mandates

In Earthcall, mathematical purity is not an aesthetic preference; it is load-bearing ontology:

1. **Separation of Mathematics from Mechanics (`ONTOMATH_FRAMEWORK.md` §1):**
   > *"In Earthcall, this coupling is an ontological violation. What something is mathematically must be distinct from how a specific channel renders it... By centralizing these in OntoMath, Earthcall ensures that any mathematical law authored by a Person can be reliably executed by any subsystem."*
2. **No Black Box & Universal Law Governance (`NO_BLACK_BOX.md`, `AGENTS.md` Refusal 6):**
   > *"No subsystem may define what a thing IS... Nor may a subsystem define what a thing's state means by keeping it where no law can look."*
3. **Exact Symbolic Integration & Reversibility (`ONTOMATH_FRAMEWORK.md` §6):**
   > Mathematical expressions must be held in `OntoMath` forms (`ScalarForm`, `MathNode`, `Piecewise`) so that calculus ($\partial/\partial x$, $\int dx$) is exact and laws can be reversed in closed form ($p(t-\Delta) = p(t) - \int dp/dt$).
4. **Architectural Directives on Geometry & OntoMath (`SDF_BEZIER_SHAPE_GENERATOR_LAW_REPLICATION.md`):**
   > *"NOTE from Zach: This is not sufficient. If a shape uses a SDF, we need OntoMath to actually host that SDF and expose the OntoMath variables as properties so the Person can physically modify the properties. Bezier patch likewise should use OntoMath."*

---

## 3. Comprehensive Inventory: Duplicated Math in Geometry vs. OntoMath

| Mathematical Domain | Current Implementation in `geom::` / `Object` | `OntoMath` Native Capability | Status |
| :--- | :--- | :--- | :--- |
| **Implicit Expressions & Parsing** | `geom::ExprParser`, `geom::SdfToken` (RPN tokenizer & recursive-descent parser in `Sdf.cpp`) | `OntoMath::ScalarForm`, `OntoMath::MathNode::fromJson`, continuous multivariate algebra | **Duplicated / Redundant Engine** |
| **Expression Evaluation & VM** | `geom::evalRpn` (128-slot float stack virtual machine in `Sdf.cpp`) | `OntoMath::ScalarForm::evaluate`, `OntoMath::MathNode::evaluate` | **Duplicated / Redundant Engine** |
| **CSG Boolean Operations** | `geom::SdfOp` (`Union`, `Intersect`, `Subtract`, `Morph`, `SmoothUnion`, `smin`) in `Sdf.cpp` | `OntoMath::MathNode::Op::Union` (20), `Intersection` (21), `Difference` (22), `Piecewise` | **Duplicated / Fragmented** |
| **Bézier & Bernstein Polynomials** | `geom::binom()`, `geom::bernstein()`, `evalBezier()` in `Patch.cpp` | `OntoMath::ScalarForm` multivariate signomials / polynomials | **Bypasses OntoMath** |
| **Polynomial Basis Conversion** | `geom::patchToMonomial`, `monomialToPatch` using custom `std::vector<glm::vec3>` in `Patch.cpp` | `OntoMath::ScalarForm` power basis & canonical normalization | **Bypasses OntoMath** |
| **Quadric Algebra** | `namespace geom::Quadric` (4x4 matrix quadrics $p^T Q p = 0$, quadric transform $M^T Q M$, quadratic formula solver in `SmoothSurface.cpp`) | `OntoMath::ScalarForm` degree-2 multivariate polynomials | **Isolated Custom Algebra** |
| **Calculus & Surface Normals** | `geom::sdfNormal`, `geom::bezierNormal` (numerical central differences with $\epsilon=10^{-3}$) | `OntoMath::ScalarForm::derivative`, `OntoMath::MathNode::Op::Gradient` (`kGradientEpsilon`) | **Disconnected from Symbolic Core** |
| **Parametric Surfaces & Trigonometry** | Hardcoded trigonometric evaluations in `SmoothSurface.cpp` (Torus, Ovoid), `ComplexShape.cpp` (disks, fillet strips, corner octants), and `Contour.cpp` | `OntoMath::TransFactor` (`Sin`, `Cos`, `Exp`, `Ln`), `OntoMath::Piecewise` | **Hardcoded C++ / Black Box** |
| **Differential Geometry & Curvatures** | `AngleTools.cpp` (dihedral angles, solid angles, Descartes deficit), `Contour.cpp` (Gaussian/Mean/Principal curvatures) | Pure mathematical definitions not registered in `OntoMath` | **Isolated in Geometry Domain** |
| **Shader Compilation (WebGPU)** | `sdfwgsl::compile` has dual code paths: `emitNode` (for `geom::SdfNode`) vs `emitMathNode` (for `OntoMath::MathNode`) | `sdfwgsl::emitMathNode`, `sdfwgsl::emitPiecewise` | **Dual Compiler Divergence** |

---

## 4. Detailed Subsystem Audit

### 4.1. `Sdf.hpp` & `Sdf.cpp`: Standalone Implicit Expression & CSG Engine
* **Standalone Tokenizer and Parser:**
  `Sdf.cpp` implements `ExprParser` and `SdfToken` with its own enum of operations (`Num`, `X`, `Y`, `Z`, `Add`, `Sub`, `Mul`, `Div`, `Pow`, `Neg`, `Sin`, `Cos`, `Tan`, `Sqrt`, `Abs`, `Exp`, `Log`).
* **Standalone Stack Machine:**
  `evalRpn()` allocates a fixed 128-float stack to execute RPN bytecode on the CPU.
* **CSG Operations:**
  `evalSdf()` evaluates trees of `SdfOp::Union`, `Intersect`, `Subtract`, `Morph`, and `SmoothUnion` using hardcoded `min`, `max`, `mix`, and `smin` functions.
* **Primitive Functions:**
  Hardcoded analytic distance functions (`sdSphere`, `sdBox`, `sdRoundBox`, `sdEllipsoid`, `sdCylinder`, `sdCone`, `sdTorus`) that operate strictly on `glm::vec3` and float primitives.
* **Architectural Flaw:**
  None of these expressions or CSG operations are exposed to the Law system as `OntoMath::MathNode` trees. A Person cannot query, differentiate, modulate via `PropertyPath`, or integrate backwards any shape defined through `SdfNode`.

### 4.2. `Patch.hpp` & `Patch.cpp`: Standalone Polynomial & Combinatoric Math
* **Binomial & Bernstein Math:**
  `Patch.cpp` defines static functions `binom(n, k)` and `bernstein(n, i, t)`:
  $$	ext{bernstein}(n, i, t) = \binom{n}{i} t^i (1-t)^{n-i}$$
* **Tensor-Product Evaluation & Numerical Normal:**
  Evaluates 2D tensor product sums using raw loops and computes surface normals using finite difference offsets $\epsilon = 10^{-3}$.
* **Disconnected Monomial Basis Transformation:**
  `patchToMonomial()` converts control nets into power-basis polynomial coefficients $\sum\sum a_{kl} u^k v^l$, but stores them in a raw `std::vector<glm::vec3>` rather than instantiating `OntoMath::ScalarForm` objects.
* **Architectural Flaw:**
  Bézier surfaces are purely algebraic bivariate polynomials. By isolating them in `geom::BezierPatch`, the engine loses the ability to execute exact symbolic calculus (`ScalarForm::derivative`), compile them through the unified shader compiler, or author law-driven surface deformations.

### 4.3. `SmoothSurface.hpp` & `SmoothSurface.cpp`: Standalone Quadric & Parametric Algebra
* **Matrix Quadrics ($p^T Q p = 0$):**
  Defines `namespace Quadric` which manually constructs $4 	imes 4$ symmetric matrices for Sphere, Ellipsoid, Cylinder, Cone, and Paraboloid.
* **Quadratic Formula Raycast Solver:**
  Evaluates $(o + td)^T Q (o + td) = A t^2 + B t + C = 0$, explicitly computing $B^2 - 4AC$ and $(-B \pm \sqrt{	ext{disc}})/(2A)$.
* **Parametric Torus / Ovoid SDFs:**
  Hardcodes custom distance approximations (`torusSDF`, `ovoidSDF`) and central-difference gradient calculations.
* **Architectural Flaw:**
  A quadric is simply a degree-2 multivariate polynomial in $(x, y, z)$. In `OntoMath`, this is a canonical `ScalarForm`. Creating a specialized matrix algebra creates a second mathematical system in the engine that cannot compose with other `OntoMath` operations (like CSG booleans, field evaluations, or law actions).

### 4.4. `ComplexShape.hpp` & `ComplexShape.cpp`: Hardcoded Procedural Generators
* **Parametric Sweeps:**
  Contains procedural generators for circular disks (`diskPolygon`), quarter-cylinder fillet strips (`filletStrip`), and corner spherical octants (`cornerOctant`) using direct calls to `std::cos` and `std::sin`.
* **Intersection Algorithms:**
  Implements Möller–Trumbore ray/triangle intersection (`rayTri`) and planar polygon raycasting from scratch.

### 4.5. `FieldNode.hpp` & `FieldNode.cpp`: The Lone Integration Seam
* `FieldNode` is the **only** class in `../../src/ConstructedBeing/Singular/Object/Geometry/` that references `Singularity/OntoMath/Field.hpp`.
* It wraps `std::shared_ptr<OntoMath::ScalarField>` and `std::shared_ptr<OntoMath::VectorField>`, registering properties such as `field.baseDensity`, `field.frequency`, and `field.amplitude`.
* However, as noted in `docs/ontomath_fields.md`, Path B (AST-driven compilation) is not yet wired to active geometry generation, and `FieldNode` is an exception rather than the standard pattern in `Geometry/`.

### 4.6. `SdfWgsl.cpp`: Dual Compiler Divergence in WebGPU
* `SdfWgsl.cpp` contains two complete, distinct transpilation backends:
  1. `emitNode()`: Traverses `geom::SdfNode`, invoking `kPrimitives` and `emitRpn()`.
  2. `emitMathNode()` / `emitPiecewise()`: Traverses `OntoMath::MathNode` and `OntoMath::Piecewise`, translating `OntoMath::ScalarForm` terms into WGSL.
* This is conclusive proof of architectural duplication: the rendering subsystem has to maintain two distinct transpilers because the geometry framework refused to use `OntoMath`.

---

## 5. Architectural Violations & Operational Consequences

1. **Law Inaccessibility ("Black Box Math"):**
   Because `geom::SdfNode`, `geom::BezierPatch`, and `geom::SmoothSurfaceData` are stored as raw C++ structs, their mathematical parameters are largely invisible to `PropertyPath`. Laws cannot dynamically modulate the underlying functions or replace subtrees.
2. **Breakdown of Symbolic Inversion & Reversibility:**
   `OntoMath` enables exact past-state reconstruction through symbolic closed-form antiderivatives. Because the geometry subsystem uses hardcoded floating-point routines, its mathematical states cannot participate in closed-form time reversal.
3. **Redundant Code & Maintenance Burden:**
   Multiple parsers, ASTs, CSG evaluators, and shader transpilers exist simultaneously across `Singularity/OntoMath` and `ConstructedBeing/Singular/Object/Geometry`.
4. **Disconnection between Tools and Ontology:**
   Tools and shape generators authored in C++ or via `ObjectConcept` manipulate geometry data structures that are alien to the rest of the simulation's mathematical ontology.

---

## 6. Target Architecture & Remediation Roadmap

To bring the geometry framework into strict alignment with `OntoMath` and the Earthcall manifesto:

```
[Legacy Geometry Framework]             [Target Pure Architecture]
  geom::SdfNode (RPN parser/eval)  ───►   OntoMath::MathNode / Piecewise AST
  geom::BezierPatch (bernstein)    ───►   OntoMath::ScalarForm (Multivariate Polynomials)
  geom::SmoothSurface (Quadric Q)  ───►   OntoMath::ScalarForm (Degree-2 Signomials)
  geom::ComplexShape (hardcoded)   ───►   OntoMath Composition & CSG Operators
  SdfWgsl.cpp (emitNode)           ───►   SdfWgsl.cpp (emitMathNode / emitPiecewise exclusively)
```

### Remediation Steps:
1. **Phase 1: Unify Implicit Geometry under `OntoMath::MathNode`**
   - Replace `geom::SdfNode` with `OntoMath::MathNode` and `OntoMath::Piecewise`.
   - Deprecate `geom::ExprParser` and `geom::evalRpn` in favor of `OntoMath::ScalarForm` and `OntoMath::MathNode::evaluate`.
   - Map all primitive shapes (`sdSphere`, `sdBox`, etc.) to standard `OntoMath::MathNode` subtrees or AST expressions.
2. **Phase 2: Unify Bézier & Quadric Geometry under `OntoMath::ScalarForm`**
   - Refactor `geom::patchToMonomial` to produce 3 continuous `OntoMath::ScalarForm` instances $(X(u,v), Y(u,v), Z(u,v))$.
   - Refactor quadrics ($p^T Q p = 0$) into canonical degree-2 `OntoMath::ScalarForm` expressions.
3. **Phase 3: Unify WebGPU Codegen (`SdfWgsl.cpp`)**
   - Eliminate `emitNode()` and `emitRpn()` from `SdfWgsl.cpp`.
   - Route all shader compilation through `emitMathNode()` and `emitPiecewise()`.
4. **Phase 4: Expose Geometry Equations as Governable `PropertyPath`s**
   - Ensure all geometric functions are held in `Singular` containers whose mathematical parameters and AST roots are fully addressable by Law text.

---

*Report compiled and archived to `docs/audits/GEOMETRY_ONTOMATH_AUDIT_2026-08-17.md`.*
