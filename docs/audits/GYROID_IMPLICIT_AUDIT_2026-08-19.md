# Audit — Creator Console Gyroid (implicit field) — 2026-08-19

**Document:** `docs/audits/GYROID_IMPLICIT_AUDIT_2026-08-19.md`
**Reporter:** Zach (in-app): the Gyroid “looks like a cube” and “flashes crazy
complex hole patterns” when looking at it; it did not match a gyroid from
search.
**Scope:** Creator Console Implicit `f(x,y,z)=0` Gyroid preset
(`Create3DConsole.cpp`), `geom::makeImplicit`, WebGPU SDF marcher
(`SdfWgsl.cpp` `kMarcher`), CPU `evalSdf` / `tessellateSdf`.
**Fix landed in the same session** (AABB-clipped march + thickened one-cell
preset). `webgpu_sdf_parity_test` re-run: 20/20 silhouettes agree with CPU.
**Not re-clicked in-app after the fix.**

---

## Verdict

The Gyroid button was not drawing a gyroid lattice. It was rasterising a
**bounding cube** and raymarching an **unbounded, paper-thin, high-frequency
implicit** from the eye. The flash is the marcher hitting a different
periodic sheet every time the camera moved.

Two independent defects, stacked:

1. **The preset was the zero set of a triply-periodic function**, not the
   thickened lattice people mean by “gyroid.”
2. **The marcher started at the eye** in field space. The gyroid is defined
   everywhere, so the first hit was between the Person and the object.

The cube silhouette is the raster bounds (`pos * extent`). It is not the
shape. When most rays miss or hit the wrong sheet, the cube is what remains
visible, with holes strobing through it.

---

## What the button actually authored

`Create3DConsole.cpp` (before):

```
sin(8*x)*cos(8*y) + sin(8*y)*cos(8*z) + sin(8*z)*cos(8*x)
```

Then `geom::makeImplicit` + `Object::setFieldShape(node, 1.1f)`.

That expression is Schoen's gyroid *isosurface* \(f=0\). Google images are
almost always a **thickened** lattice \(|f|-t\), sometimes intersected with a
ball or box. \(f=0\) has no thickness. Sphere tracing of a non-distance field
through a sheet that thin tunnels; the surface pops in and out with view
angle.

Frequency 8 in a box of half-extent 1.1 is \(\approx 2.8\) periods — busy, not
one cell. Combined with a 24-sample marching-tet mesh on the OpenGL path, it
aliases into noise.

`rpnToMathNode` **refuses** `sin(8*x)` (compound argument to a transcendental;
`TransFactor` only lifts `sin` of a bare variable). `mathNode` is null; CPU
and GPU both fall back to RPN. That fallback evaluates the formula
*correctly*. The glitch is not a wrong AST. It is the domain and the
thickness.

---

## Why it looked like a flashing cube

WebGPU `drawImplicit` draws a unit cube scaled by `extent`, culling off, and
the fragment shader sphere-traces `sdfEval` **from the eye** in field space
(`kMarcher`, before this audit). Comment on file: the rasterised face is not
a usable ray origin, so the whole ray was traced.

For a compact SDF (sphere, box) the first hit from the eye *is* the object.
For a **periodic implicit defined on all of \(\mathbb{R}^3\)**:

- Field space is the object's local frame, unbounded.
- The eye, transformed by `invModel`, sits somewhere in that frame.
- \(f\) has zeros every \(\pi/4\) units at frequency 8.
- The first hit is a sheet **near the camera**, not the object.
- Depth is written from that hit. Look around: different fragments hit
  different nearby sheets. Flash.

The CPU tessellator (`tessellateSdf`) already samples only \([-e,e]^3\). The
OpenGL mesh path would have shown a noisy chunk *inside* the box. The
WebGPU path is the one that fills the world. Zach's report (cube +
view-dependent holes) is the WebGPU failure mode.

`webgpu_sdf_parity_test` could not catch this: its cases are compact SDFs,
and its CPU reference already **discards hits outside the box**
(`kExtent * 1.05`). The GPU had no such clip.

---

## What was changed

1. **Marcher (`SdfWgsl.cpp`).** Analytic AABB slab (`rayAabb`). `t` starts at
   `max(tEnter, 0)` and stops at `tExit`. A miss of the cube discards. Periodic
   copies outside the object's box are no longer marched. Comment on file
   records why the old “from the eye” choice was wrong for implicits.

2. **Preset.** One cell, thickened:

```
abs(sin(pi*x)*cos(pi*y) + sin(pi*y)*cos(pi*z) + sin(pi*z)*cos(pi*x)) - 0.2
```

Period 2 in a box of width 2.2 \(\approx\) one Schoen cell. `|f|-0.2` is the
lattice wall. Console disabled-text says so.

**Verification:** `webgpu_sdf_parity_test` rebuilt and run after the marcher
change: 20 shapes, `diff=0` against CPU. Compact SDFs are unaffected (first
hit from the eye already lay inside the box). The gyroid case is not in that
suite.

An existing implicit object still holds the old formula until Gyroid is
clicked again and Create Implicit is pressed.

---

## What is still open

- **CPU `raycastSdf` is still unbounded.** Picking / hover of a leftover
  raw-gyroid object can still hit a sheet between the Person and the box.
  Match the AABB clip, or pass `extent` into the raycaster.
- **`rpnToMathNode` still refuses `sin(k*x)`.** Faithful lift exists:
  `TransFactor(Sin, "x", k, 0)`. Not the visual bug; it is why the gyroid
  never becomes a `MathNode`.
- **OpenGL tessellation stays at res 24.** Fine for one thickened cell;
  a Person who pastes frequency-8 raw \(f=0\) still gets a holey mesh.
- **No in-app re-click after the fix.** Feature-sized item 2 still owns
  that gate.

---

## Related

- Shape Generator 3D law audit: [`SHAPE_GENERATOR_LAW_AUDIT_2026-08-18.md`](SHAPE_GENERATOR_LAW_AUDIT_2026-08-18.md)
  (placement / Create bit). This report is the implicit *field* path from
  the same console, not the cube-spawn law.
- Math architecture:
  [`docs/architecture/mathematics/ONTOMATH_FRAMEWORK.md`](../architecture/mathematics/ONTOMATH_FRAMEWORK.md).
- Marcher / CPU formula contract: `tests/singularity/webgpu_sdf_parity_test.cpp`.
