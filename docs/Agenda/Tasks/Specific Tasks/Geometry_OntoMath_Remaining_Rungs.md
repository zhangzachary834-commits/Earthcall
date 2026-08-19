# Geometry–OntoMath Unification: Completed Status

Companion to [`GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md`](../../../architecture/mathematics/GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md).
That plan's §4 checkboxes are historical; this file is the executed record.

All six rungs are implemented, integrated, and verified by `tests/geometry_ontomath_test.cpp` and `tests/ontomath_test.cpp`. Independent primitive ground-truth (box / cylinder / torus actually differ from sphere) and the shared `kDegenerateDivisor` → `0.0` on both CPU and WGSL were confirmed 2026-08-18 (geometry-ontomath intercom).

Still open, and they are comments / leftover surface, not a seventh rung:

- Write the sqrt-versus-`Pow(_, 0.5)` canonical rule next to the `MathNode::Op` values so `toScalarForm`, the emitter, and any simplifier pick the same spelling.
- `emitNode` / `emitRpn` / `evalRpn` still exist as fallbacks. Do not delete them to "finish" Rung 4 without migrating remaining Expr/RPN saves.
- Person-facing spawn path, event name, and seed twin: [`SHAPE_GENERATOR_LAW_AUDIT_2026-08-18.md`](../../audits/SHAPE_GENERATOR_LAW_AUDIT_2026-08-18.md).

## Completed Rungs Summary

### Rung 0: OntoMath Basis Expansion & Canonical Primitives
- **Basis Polynomials**: `Operations::binom`, `ScalarForm::bernsteinBasis`, `fromBernstein`, `fromBivariateBernstein`, `toBivariateBernstein`. Exact, round-trip tested.
- **Canonical Primitives**: `MathNode::sphere`, `box`, `cylinder`, `torus`, and `smoothUnionOp` implemented using exact canonical SDF formulas over `MathNode::Op` (`Abs`, `Union`/`Intersection` min/max, `Clamp`, `Length`, `Sub`, `Add`, `Scale`).

### Rung 1: Unified Symbolic SDF Engine & RPN Retirement
- Added append-only `MathNode::Op` operations: `Div` (23), `Pow` (24), `Abs` (25), `Clamp` (26), `Sqrt` (27), and `Tan` (28).
- `rpnToMathNode` lifts 100% of standard expression tokens into `MathNode` ASTs, including division, fractional powers, transcendentals, and componentwise operations.
- `SdfNode::toMathNode` lifts `Sphere`, `Box`, `Cylinder`, `Torus`, and CSG operations directly to `MathNode`.
- `kDegenerateDivisor` (1e-6) defined as a shared CPU/GPU threshold to guard division by zero consistently across both backends.

### Rung 2: Bézier Patch Algebraic Unification
- Bézier patches carry three `ScalarForm` coordinate polynomials ($x(u,v), y(u,v), z(u,v)$).
- Normals are computed via exact symbolic cross product $T_u 	imes T_v$ via `ScalarForm::derivative`.
- Partial derivatives are hoisted via `geom::patchDerivatives()` once per patch tessellation (~7.2 ms for a 24×24 patch).

### Rung 3: Smooth Quadric Surface Matrix & Symbolic Form Unification
- `ScalarForm` is the quadrics' source of truth via `Quadric::toScalarForm(Q)` and `Quadric::fromScalarForm(form)`.
- Verified that matrix gradient $2Qp$ and raycast quadratic coefficients $(A, B, C)$ match symbolic `ScalarForm::derivative` calculations across all quadric forms.

### Rung 4: WebGPU Transpiler Consolidation
- `SdfWgsl.cpp` compiles `MathNode::Op::Div`, `Pow`, `Abs`, `Clamp`, `Sqrt`, `Tan`, and CSG min/max directly to standard WGSL builtins.
- `emitNode` compiles `SdfPrim::Expr` directly through `emitMathNode(*n.mathNode, e, lp)`.

### Rung 5: Law-Addressable Geometry & Set-to-Set Replication
- Law property paths exposed on `Object`: `field.extent`, `field.op`, `field.prim`, `field.dims`, `field.offset`, `field.p0`, `field.p1`, `field.blend`, writable `field.expr`, `patch.degreeU`, `patch.degreeV`, `patch.controlCount`, and `patch.ctrl.0..15`.
- On `geom::FieldNode`: `field.ast` and `vectorField.ast` exposed as JSON for Law inspection and modulation.
- Verified that `ObjectConcept` captures and replicates Field and Patch shapes across save/load cycles and dynamic instantiation.

## Verification
- `tests/geometry_ontomath_test.cpp`: all checks pass.
- `tests/ontomath_test.cpp`: all checks pass.
- `tests/bezier_patch_law_test.cpp`: all 30 checks pass.
- `tests/shape_generator_law_test.cpp`: all checks pass.
- `tests/test_field.cpp`: all checks pass.
