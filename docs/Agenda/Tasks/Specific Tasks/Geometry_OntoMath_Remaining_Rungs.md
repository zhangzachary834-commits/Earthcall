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
- **Rung 3.** `ScalarForm` is the quadrics' source of truth. `Quadric::gradientFromForm` and
  `raycastCoefficientsFromForm` compute the gradient and the ray quadratic straight off
  `ScalarForm::derivative`, and the test holds the fast matrix path (`2Qp`, and raycast's
  A/B/C) to agreeing with them for every quadric the factories make, at several sample
  points. The matrix stays in the hot path **deliberately** — the rung requires the
  mathematics be authored and legible in OntoMath ("a channel reads OntoMath; it never
  decides what the thing is", `ONTOMATH_FRAMEWORK.md` §1), not that every evaluation walk
  an AST. Rung 2's 920× lesson applies directly: `raycast` runs per ray per frame.
- **Rung 5.** The sculpted geometry is law-addressable at last. On `Object`:
  `field.extent/op/prim/dims/offset/p0/p1/blend` and a **writable** `field.expr` (assigning
  an implicit expression reshapes the being; an unparseable one is refused and the old shape
  stands), plus `patch.degreeU/degreeV/controlCount` and `patch.ctrl.0..15` — the control
  points *are* the Bernstein coefficients, so a law can animate the polynomial weights and
  the surface follows. On `geom::FieldNode`: `field.ast` and `vectorField.ast`, the
  `Piecewise` exposed as the JSON it already round-trips, replacing a comment that said the
  AST could only be rewritten "via specialized OntoMath endpoints or over the network" —
  i.e. not by law at all. `ObjectConcept` capture of a Field shape is verified to survive
  instantiation.

  Two things to know if you touch this. Bridges must return a **well-typed** value even when
  the payload is absent, never monostate: `knownPathOptions()` probes a bare prototype
  `Object` to decide whether a path is a vector, a string or a number, so a property reading
  as "nothing" gets mislabelled and its `.x/.y/.z` sub-paths never offered — that is what
  `channel_paths_test` catches. And the sixteen `patch.ctrl.N` slots are a fixed count for
  the same reason the six `face.N` slots are: `buildProperties` runs before the object has a
  patch. A net elevated past bicubic (`geom::elevateU`/`elevateV`) has control points beyond
  index 15 that no path yet names.

## Not done

All three below are blocked on ONE decision: extending `MathNode::Op` and
`TransFactor::Kind`. Both are append-only and serialized as ints, so appending is the
sanctioned move (Refusal 3 forbids renumbering and reuse, not growth) — but it is a real
ontological addition and wants its own pass with its own tests, not a drive-by.

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

### Rung 4 — WebGPU transpiler consolidation
Not started. `emitNode` (`SdfWgsl.cpp:391`) and `emitRpn` (`:127`) both remain. What landed
is a fallback chain `mathNode → rpn → 1e9`, not a consolidation. Blocked on Rung 1.

## Performance note

`bezierNormal` originally rebuilt the entire symbolic patch (`patchToScalarForms`) and
re-differentiated it six times **per vertex**: 1530 ms vs 1.66 ms for the finite-difference
normal it replaced, on one 4×4 patch at 24×24 — 920×, in the path that runs whenever a
control point moves. Fixed by hoisting into `geom::patchDerivatives`, taken once per
tessellation: 3474 ms → 7.2 ms for the whole patch (Debug).

Anything that adds symbolic evaluation to a per-vertex or per-sample path needs the same
check. The plan's own risk table asked for a before/after benchmark; there wasn't one.
