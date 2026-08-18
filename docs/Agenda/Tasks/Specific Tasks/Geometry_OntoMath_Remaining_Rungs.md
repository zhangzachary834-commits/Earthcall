# Geometry–OntoMath unification: what is actually left

Companion to [`GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md`](../../../architecture/GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md).
Commit `d4514815` landed the six-rung plan as one commit and it was recorded as six rungs
done. Two were. This file is the honest remainder.

Guarded by `tests/geometry_ontomath_test.cpp` (37 checks) — every refusal below is asserted,
so "implementing" one means turning a passing refusal check into a passing behaviour check.

## Done and verified (2026-08-17)

- **Rung 0, basis half.** `Operations::binom`, `ScalarForm::bernsteinBasis`,
  `fromBernstein`, `fromBivariateBernstein`, `toBivariateBernstein`. Exact, round-trip
  tested to 1e-6.
- **Rung 2.** Bézier patches carry three `ScalarForm` coordinate polynomials; normals are
  exact symbolic `T_u × T_v` via `ScalarForm::derivative`.

## Not done — recorded as done in error

### Rung 0, primitives half
`MathNode::box`, `cylinder`, `torus`, and `smoothUnionOp` shipped as stubs returning
`sphere(...)` with their distinguishing parameters discarded. They now **refuse** (return
`nullptr`) rather than lie.

Implementing them needs vocabulary `MathNode::Op` does not have: componentwise absolute
value, `clamp`, and scalar `min`/`max` (`Union`/`Intersection` are the CSG booleans over
whole distance fields, not general min/max). `Op` is **append-only and serialized as ints** —
widening it is a deliberate ontological act with its own tests, per Refusal 3.

Canonical forms to implement against:
- box: `length(max(|p| - b, 0)) + min(max(q.x, q.y, q.z), 0)`
- cylinder: `max(length(p.xz) - r, |p.y| - h)`
- torus: `length(vec2(length(p.xz) - R, p.y)) - r`
- smin: the polynomial blend, which needs `clamp`

### Rung 1 — unified symbolic SDF engine
Not started. `geom::ExprParser`, `compileExpr`, `evalRpn`, and `SdfToken` all remain, and
`SdfNode` now carries **three** representations of one expression (`expr`, `rpn`, `mathNode`)
that can disagree — the opposite of the rung's goal.

`rpnToMathNode` currently lifts only what it can say faithfully (numbers, `x`/`y`/`z`,
`+ - *`, unary minus, `var ^ constant`, and sin/cos/exp/log **of a bare variable**) and
refuses everything else, so `evalSdf` falls back to the correct `evalRpn`. Retiring the RPN
path means first giving OntoMath: **division**, general **power**, and `tan`/`sqrt`/`abs`
(`TransFactor::Kind` holds only Sin/Cos/Exp/Ln and is likewise append-only).

Note for whoever does this: `tests/webgpu_sdf_parity_test.cpp` **cannot** catch a bad lift.
Both `evalSdf` and `emitNode` prefer `mathNode`, so a wrong AST is read identically on both
sides and the test agrees with itself. Parity is not correctness here.

### Rung 3 — quadric unification
Not started. `Quadric::toScalarForm` / `fromScalarForm` were added and round-trip tested,
but have **no production callers**; the 4×4 matrix algebra in `SmoothSurface.cpp` is
untouched. The rung is the migration, not the converter.

### Rung 4 — WebGPU transpiler consolidation
Not started. `emitNode` (`SdfWgsl.cpp:391`) and `emitRpn` (`:127`) both remain. What landed
is a fallback chain `mathNode → rpn → 1e9`, not a consolidation. Blocked on Rung 1.

### Rung 5 — law reflection & property-path governance
Not attempted. `ObjectProperties.cpp`, `no_black_box_test.cpp`, and
`singular_set_to_set_test.cpp` were not touched. This is the rung that actually delivers
Zach's note in `SDF_BEZIER_SHAPE_GENERATOR_LAW_REPLICATION.md` — that a Person must be able
to reach an SDF's variables as properties — so it is the one with ontological stakes, and it
is the one nothing was done on.

## Performance note

`bezierNormal` originally rebuilt the entire symbolic patch (`patchToScalarForms`) and
re-differentiated it six times **per vertex**: 1530 ms vs 1.66 ms for the finite-difference
normal it replaced, on one 4×4 patch at 24×24 — 920×, in the path that runs whenever a
control point moves. Fixed by hoisting into `geom::patchDerivatives`, taken once per
tessellation: 3474 ms → 7.2 ms for the whole patch (Debug).

Anything that adds symbolic evaluation to a per-vertex or per-sample path needs the same
check. The plan's own risk table asked for a before/after benchmark; there wasn't one.
