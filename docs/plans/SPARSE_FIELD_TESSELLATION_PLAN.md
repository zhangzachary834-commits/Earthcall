# Sparse Field Tessellation & Authored Resolution — Implementation Plan

**Date**: 2026-08-28
**Author**: Claude Opus 5 (Claude Code), session `c2015fc1-5fbe-4690-92af-db5f289f17b0`, 13:38 PDT
**Status**: Plan only. Nothing here is implemented. No save file and no source was modified for it.
**Continues**: [FIELD_TESSELLATION_SCALING_PLAN.md](FIELD_TESSELLATION_SCALING_PLAN.md) — that plan's
Phases 0, 1a, 2, 3 and 4 have landed; this is its Phases **1b** and **1c** plus the corrections its §9 left open.
**Reads**: `Sdf.cpp`, `ObjectCollision.cpp`, `ObjectProperties.cpp`, `FieldNode.hpp`, `ScalarForm.hpp`

---

## 0. Where this picks up

The startup hang is gone: **106.3 s → 2.6 s**, and `frame_lag_test` went from 105.7 s failing to
3.7 s passing. That came from bounding the cost (a global cell budget), deferring the build off
the hydration path, taking normals from the presampled grid, and killing the per-sample
allocation in the OntoMath interpreter.

**But the sampling is bounded, not right.** `rebuildFieldMesh` still derives resolution from box
size and then divides by whatever it takes to fit under `MAX_CELLS = 125000`:

```cpp
glm::vec3 fRes = _fieldExtent * (1.0f / 5.0f);        // 1000,30,1000 -> 200, 6, 200
fRes = max(fRes, 24);                                 //             -> 200,24,200
if (total > MAX_CELLS) fRes *= cbrt(MAX_CELLS/total); //             -> 101,12,101
```

For the noise floor that is a cell **19.8 world units wide in x and z** on a surface whose Perlin
wavelength is ~50 units. Under three cells per wave. The ground is not smooth because it is not
sampled; it is sampled just finely enough to stay under a budget picked to stop a freeze.

Two things are still wrong, and they are the two this plan addresses:

1. **Nothing anywhere says how finely this field should be sampled.** The engine infers it from
   the bounding box, which carries no such information (§2).
2. **Cost is `O(N³)` for a surface that is `O(N²)`.** Raising resolution is unaffordable not
   because the surface is large but because the empty volume around it is (§3).

---

## 1. The constraint that shapes the whole design

Before proposing to skip regions of space, note what this tree already knows. `Sdf.cpp:528`:

> Sphere tracing assumes the field is a **DISTANCE** (1-Lipschitz), so a full step can never
> overshoot. An implicit `f(x,y,z)=0` leaf breaks that assumption: it is an iso-surface value that
> can be arbitrarily larger than the true distance, and a full step then tunnels straight through
> the surface.

`containsExpr` exists so the raymarcher damps its steps only where it must. **The identical hazard
governs sparse tessellation**, and it is the reason the obvious implementation is wrong:

> **Do not prune a cell because `|f(center)|` is large.** For an `SdfPrim::Expr` leaf that value
> says nothing about how far the surface is. You would skip cells the surface runs straight
> through, and the ground would develop holes that appear and disappear as the camera moves —
> the kind of silent wrong answer this codebase writes tests against.

The correct instrument is a **conservative range** over a box: an interval `[lo, hi]` guaranteed to
enclose every value `f` takes inside it. If `lo > 0` or `hi < 0`, the box provably contains no
iso-surface and can be skipped with no risk. If the interval straddles zero, we recurse. A range
that is merely *loose* costs speed; it never costs correctness. That asymmetry is what makes this
safe to build.

---

## 2. Phase 1b — ⚑ AUTHOR: say how finely a field is sampled

**The knob.** Add a **cell size in world units** to the field shape: "sample this field every
`h` units", from which `N = 2·extent/h` per axis. Cell size is the right unit because it is the
one that composes — it means the same thing on a 1-unit blob and a 1000-unit ground plane, which
is exactly what `extent` fails to do.

**Where it goes.** `ObjectProperties.cpp:527` already registers the field's mathematics through
`FieldShapeBridge`:

```cpp
addField("extent", ...); addField("op",   ...); addField("prim", ...);
addField("dims",   ...); addField("offset",...); addField("p0", ...);
addField("p1",     ...); addField("blend", ...); addField("expr", ...);
```

`field.cellSize` is a sibling of `field.extent` and belongs in that block. Two notes for whoever
implements it:

- This satisfies **Refusal 6** rather than straining it. Resolution currently lives in a
  hard-coded constant inside `rebuildFieldMesh` where no law can reach it — "nobody registered it
  yet", which the refusal names as the one access level no law can change.
- `FieldShapeBridge::Field` is a file-local enum in `ObjectProperties.cpp`, used only for internal
  dispatch and **never serialized**. Adding a value to it is not a Refusal 3 event. (`ShapeKind`,
  `BeingKind`, `ConditionNode::Kind` and `ActionNode::Kind` are the serialized ones.)

**⚑ AUTHOR decisions — Zach's, not mine:**

- **The name.** `field.cellSize` is the obvious one, but "cell" is engine vocabulary. If a Person
  is meant to author this, `field.detail` or `field.featureSize` may say more.
- **Scalar or per-axis?** A single `float` is simpler and right for nearly everything. A `vec3`
  would let a ground plane be sampled finely in x/z and coarsely in y — genuinely useful for
  heightfields, and it is what the anisotropy in this very object wants.
- **Unset default.** Keep today's budget-derived number, so every existing save behaves exactly as
  it does now and nothing needs migrating.

**Guard, and say so when it bites.** The cell budget must remain as a **Kernel guard** on the main
thread — an authored `cellSize` of 0.001 on a 1000-unit box must not be able to freeze the window.
But when the guard overrides authored intent it has to **say which number it used and why**, one
line, naming the authored cell size, the budget, and the size actually applied. This is the audio
channel's infrasound floor exactly (`ONTOMATH_FRAMEWORK.md` §7a): refuse or clamp out loud, never
silently filter a Person's mathematics. A ground plane that quietly ignores the resolution someone
asked for is the same bug in a different channel.

**Serialization.** One optional key next to `fieldExtent`; absent means "unset, use the default".
Old saves therefore load unchanged, and a field that never had one never grows one.

---

## 3. Phase 1c — sparse, surface-following tessellation

### 3.1 What to build

Replace the dense `N³` sweep with an octree refinement driven by the conservative range of §1.

```
subdivide(box, depth):
    [lo, hi] = evalRange(node, box)
    if lo > 0 or hi < 0:  return          # provably no surface here
    if box cell size <= target h:
        marchTet over this cell (existing code, unchanged)
        return
    for each of 8 children: subdivide(child, depth+1)
```

The emission step does not change — `marchTet` and the corner-interpolated normals from Phase 3
stay exactly as they are. Only *which* cells get visited changes.

### 3.2 `evalRange` — the one genuinely new piece

`geom::evalRange(const SdfNode&, glm::vec3 boxMin, glm::vec3 boxMax) -> std::pair<float,float>`,
a conservative enclosure. Rules, in rough order of how much they buy:

| node | rule |
|---|---|
| any **true-distance** leaf (Sphere, Box, RoundBox, Ellipsoid, Cylinder, Cone, Torus, Convex) | 1-Lipschitz, so `[f(c) − R, f(c) + R]` where `c` is the box centre and `R` its half-diagonal. One rule, all seven, always valid. |
| `Union` = `min(a,b)` | `[min(lo_a,lo_b), min(hi_a,hi_b)]` |
| `Intersect` = `max(a,b)` | `[max(lo_a,lo_b), max(hi_a,hi_b)]` |
| `Subtract` = `max(a,−b)` | `[max(lo_a,−hi_b), max(hi_a,−lo_b)]` |
| `Morph` (lerp at fixed `t`) | componentwise lerp of the two intervals |
| `SmoothUnion` | `smin ≤ min`, and is below it by at most a `k`-dependent constant: `[min(lo_a,lo_b) − k, min(hi_a,hi_b)]`. Loose on purpose. |
| `Expr` leaf | range-evaluate the MathNode tree (below) |
| **anything unrecognised** | `[−∞, +∞]` — never prunes, always correct |

That last row is the safety property, and it should be the *default* arm of the switch rather than
an afterthought: an op nobody has written a rule for degrades to today's dense behaviour instead of
punching holes in someone's world. `MathNode::Op::Unsupported` already establishes this instinct —
an op this build cannot read evaluates to `nullopt` rather than to a guess.

**MathNode interval rules** (`Op::` values from `ScalarForm.hpp:252`): `ScalarLeaf` with no factors
is `[c,c]`; `ValueLeaf` `"x"/"y"/"z"` is that axis's box range and `"p"` the box itself;
`Add`/`Sub`/`Scale`/`Div`/`Pow`/`Abs`/`Sqrt`/`Clamp` are textbook interval arithmetic;
`Noise` is `[−1, 1]`, since `glm::perlin` is bounded. Everything else returns `[−∞,+∞]`.

Work the noise floor through it — `Sub(y, Scale(15, Noise(…)))`:

```
range(f) over a box with y in [y0, y1]
  = [y0, y1] − 15·[−1, 1]
  = [y0 − 15, y1 + 15]
```

so every box with `y0 > 15` or `y1 < −15` prunes immediately, from the trivial noise bound alone,
with no knowledge of the expression's interior. The ±30 box collapses to the ±15 band that actually
contains ground, and it does so at the coarsest level, before a single child is visited.

### 3.3 What it is worth — honestly

At **today's** resolution the win is modest. The surface spans the full 2000×2000 footprint, so
pruning only removes the empty y above and below: roughly 2× on this object.

The real argument is asymptotic, and it should be made in those terms rather than as a speed
number. Dense cost grows as `N³`; sparse grows with the surface, `N²`. So:

| target cell size | dense cells | sparse (order of) |
|---|---|---|
| 19.8 (today) | 122 K | ~60 K |
| 10 | 1.0 M | ~240 K |
| 5 | 7.7 M | ~960 K |
| 2.5 | 61 M | ~3.8 M |

**Sparse is not primarily how the current mesh gets faster — it is how a better-looking ground
plane becomes affordable at all.** Halving the cell size costs 8× dense and 4× sparse, and the gap
compounds every time you halve again. Phase 1b is what lets someone ask for the finer number;
Phase 1c is what makes the request answerable.

A second benefit worth having: the dense path allocates the whole `(N+1)³` corner grid up front
(`std::vector<float> g`, plus `gn` for gradients since Phase 3). At a 5-unit cell that is 7.7 M
floats of values and 23 M more of normals before a single triangle exists. The octree never
materialises the empty volume.

### 3.4 Risks, and the two ways this goes wrong

- **A range rule that is not conservative.** A single arithmetically wrong interval rule punches
  holes in a surface, and it will look like a rendering bug, not a math bug. Every rule needs a
  randomised property test: for many random boxes and many random points inside them, assert
  `lo ≤ f(p) ≤ hi`. That test is cheap, it is the only real guard, and it should be written
  **before** the pruning is switched on.
- **Cracks between refinement levels.** Neighbouring cells at different octree depths do not share
  edge samples, so marching tets independently in each leaves T-junction cracks. This is the
  classic adaptive-isosurface problem. Either keep the leaf level uniform (restricted octree —
  neighbours differ by at most one level, and stitch that one case), or move to dual contouring,
  which places one vertex per cell and is crack-free across levels by construction. **Restricted
  octree first**; dual contouring is a larger change and buys sharp features this surface has none
  of.

### 3.5 Staging

1. `evalRange` for the SdfNode ops, `[−∞,+∞]` for `Expr`. Property test. **No behaviour change yet.**
2. Octree traversal at a uniform leaf level, pruning by range. Same output as dense; assert equal
   triangle counts on the existing field fixtures.
3. MathNode interval rules, so `Expr` leaves start pruning. Re-run the same equality assertions.
4. Only then, non-uniform leaf depth and the restricted-octree stitch.

Steps 1–3 are behaviour-preserving and independently verifiable, which is what makes this
tractable; step 4 is the one that changes output and needs the crack test.

---

## 4. The corrections §9 left open

**4.1 The cell-budget floor is applied before the scale.** `rebuildFieldMesh` raises each axis to
24 and *then* multiplies by `cbrt(MAX_CELLS/total)`, so an axis can finish below its own floor —
the noise floor's y lands at 12. Only `max(4, …)` is a real floor. Apply the budget scale first and
the floor after, then re-check the budget. Small, and it makes the code mean what it says.

**4.2 A field's collision AABB is never tightened.** `rebuildGeometryCaches` sets
`_localMin/_localMax = ∓_fieldExtent` and `rebuildFieldMesh` never narrows them to the mesh it just
built, so the noise floor's zone spans ±30 in y where the surface occupies about ±15. Conservative,
therefore safe, but it hands the broadphase twice the box it needs. Tighten at the end of
`rebuildFieldMesh`, and leave the extent-sized box as the pre-build value.

**4.3 Polyhedron bounds are patched at the wrong end.** `updateCollisionZone` now special-cases
polyhedra and reads `polyhedronData.vertices` directly, because **no polyhedron mutation point
calls `rebuildGeometryCaches`** — `setPolyhedronData`, the four `create*` solids, and
`setPolyhedronVertexLocal` all set `_polyhedronDirty` and stop. That fix is correct and should
stay, but the root cause is still there, and `setPolyhedronData` carries the fingerprint:

```cpp
    if (_shapeKind == ShapeKind::Polyhedron) {
    }
```

an empty block where the rebuild call plainly used to be, or was meant to go. Give polyhedra the
same bounds-dirty treatment as everything else and the special case in `updateCollisionZone` can
be deleted rather than maintained. Until then, **anything else that starts trusting
`_localMin/_localMax` will hit this same hole** — which is exactly how it was hit the first time.

**4.4 Re-record `frame_lag_baseline.txt` downward**, on a quiet machine
(`./build/frame_lag_test --rebaseline`), so the tripwire tightens behind the fix instead of leaving
~100 s of slack. Per the Performance section's own rule, and not done yet because the machine that
measured all this had just finished a full build.

**4.5 Separate thread, filed not fixed.** `saves/zones/Perlin Noise Floor Zone/zone.json` holds a
field with `prim: 7` (`Expr`) and **no `mathNode`, `expr`, or `piecewise` payload at all**. It
evaluates to `1e9` everywhere — empty space — silently. `SdfJson.cpp:20` documents this exact
failure shape for the piecewise arm. Something authored that zone and lost its expression on the
way to disk. Worth chasing before anyone concludes that Perlin floor "just doesn't render", and
worth a load-time complaint: an `Expr` leaf with no expression is never what anyone meant.

---

## 5. Sequencing

1. **4.1** and **4.2** — minutes each, no design questions.
2. **4.4** — re-baseline, so later work is measured against something honest.
3. **1b** — the authored cell size, once Zach has answered the ⚑ AUTHOR questions in §2. It is
   independently useful: it makes resolution reachable by law even while sampling stays dense.
4. **1c** steps 1–3 — range evaluation and uniform-leaf pruning, behaviour-preserving throughout.
5. **4.3** — the polyhedron root cause, whenever someone is next in `ObjectCore.cpp`.
6. **1c step 4** — adaptive depth and the crack stitch. The only step that changes output.

**Verification at every step**, because the failure mode here is visual and silent: triangle-count
equality against the dense path on the existing field fixtures, the randomised conservativeness
property test from §3.4, the anisotropic-normal check already in `geometry_ontomath_test`, and
`frame_lag_test` for the cost.

---

## 6. Attribution

The occasion is Zach's — he reported the frozen window, chose the interpreter allocation as the
first fix, asked for the original plan, had Gemini implement it, and asked for this review and
this plan.

Gemini implemented Phases 1a, 2, 3 and 4. The grid-gradient approach in Phase 3 is the one this
plan's §3 builds on top of, and the lazy-build seam it added in `rebuildFieldMesh` is where the
octree will go — sparse tessellation is much easier to land against a lazy builder than against a
call in the hydration path, so that ordering turned out to matter more than it looked.

The observation in §1 — that pruning by `|f(center)|` is unsafe for exactly the reason sphere
tracing already damps its steps — is a reading of the `containsExpr` comment at `Sdf.cpp:528`,
which was in the tree and already right; this plan extends its reasoning from the raymarcher to the
tessellator. The `MathNode::Op::Unsupported` convention likewise supplied the shape of §3.2's
default arm.

Interval/range arithmetic over implicit surfaces, restricted octrees, and dual contouring are
standard practice in the isosurface literature, not inventions here — named in the spirit of
CLAUDE.md's rule against re-deriving solved problems, the same way Rete was reached for in the law
engine. The specific range rules, the noise-floor worked example, the cost table, the crack
analysis and the staging are mine.
