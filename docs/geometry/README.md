# `docs/geometry/` — historical tool notes

These files describe the **pre-law, pre-OntoMath** 3D Shape Generator surface
(dropdowns named "Shape Generator", a "3D button", polyhedron variant panels).
They are not the current math architecture and they are not the current
Person-facing creation path.

**Math (as of 2026-08-18).** Pure geometry math lives in `OntoMath`
(`MathNode`, `ScalarForm`, `Piecewise`). `geom::` still *names* shapes and
tessellates them; it does not own a second algebra. Start at
[`ONTOMATH_FRAMEWORK.md`](../architecture/ONTOMATH_FRAMEWORK.md) and
[`Geometry_OntoMath_Remaining_Rungs.md`](../Agenda/Tasks/Specific%20Tasks/Geometry_OntoMath_Remaining_Rungs.md).
The 2026-08-17 audit that said otherwise is historical.

**Making a shape.** The Person-facing path is the `shape-generator-3d-law`
(L arms `creation-channel.active3DMode == "Create"`). The Creator Console
Create button is the developer bypass. Polyhedron spawn still refuses
(`buildCurrentPolyhedron` is a stub). See
[`SHAPE_GENERATOR_LAW_AUDIT_2026-08-18.md`](../audits/SHAPE_GENERATOR_LAW_AUDIT_2026-08-18.md).

`SHAPE_FORMATION_DAG_PLAN.md` is a separate ontological project (tree → DAG →
Formation). It is not undone by the math unification; `SdfNode` now *carries*
a `MathNode`, and the containment-tree question is still open.

Do not add new Person-facing instructions here until the Create3D console and
the law agree on one gesture.
