# Unify Geometry Framework with OntoMath (6-Rung Plan)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Singular · Relation · Formation  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Unify Geometry Framework with OntoMath (6-Rung Plan)** — done and verified (2026-08-18): Unified pure math across the geometry subsystem with OntoMath. Implemented Bernstein basis polynomials and combinatorics in `Operations.hpp` and `ScalarForm.hpp`; added canonical primitive & CSG MathNode factories (`sphere`, `box`, `cylinder`, `torus`, `smoothUnionOp`); widened `Op::Union`/`Intersection` to componentwise min/max; added `Op::Div`, `Pow`, `Abs`, `Clamp`, `Sqrt`, `Tan` to `MathNode::Op`; backed `geom::SdfNode` with `OntoMath::MathNode` and `Piecewise`; upgraded Bézier patches to `OntoMath::ScalarForm` coordinate functions with exact symbolic normals via `ScalarForm::derivative`; added Quadric Matrix <-> ScalarForm conversions in `SmoothSurface.cpp`; unified WebGPU shader generation in `SdfWgsl.cpp` through `emitMathNode`; registered Law property paths for `field.*`, `patch.*`, `field.ast`, and `vectorField.ast`. Verified in `tests/geometry_ontomath_test.cpp`, `tests/ontomath_test.cpp`, `tests/bezier_patch_law_test.cpp`, `tests/shape_generator_law_test.cpp`, and `tests/test_field.cpp`. (Ref: `docs/architecture/mathematics/GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md`.)
