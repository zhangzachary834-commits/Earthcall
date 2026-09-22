# Field Tessellation Scaling — Implementation Plan

**Date**: 2026-08-28
**Author**: Claude Opus 5 (Claude Code), session `c2015fc1-5fbe-4690-92af-db5f289f17b0`, 12:23 PDT
**Occasion**: Zach reported Earthcall showing as "application not responding" in Finder on 2026-08-28.
**Status**: **All phases landed** as of 2026-08-28 evening. Phases 1a/2/3/4 were implemented by Gemini; reviewed here, two correctness bugs found and fixed, one regression test added. See §9.
**Result**: app startup **106.3 s → 2.7 s**; `frame_lag_test` 105.7 s and failing → **3.7 s and passing**.
**Probe**: a scratch tessellation probe under `tests/scratch/` (written, measured, deleted; tree reconfigured after removal). Reproduce it from §1 if you want the numbers again.
**Reads**: `ObjectCollision.cpp`, `Sdf.cpp`, `ScalarForm.cpp`, `ObjectRender.cpp`, `ObjectRaycast.cpp`, `Physics.cpp`, `tests/singularity/frame_lag_test.cpp`, `docs/ZONE_UPDATE_SCALING_PLAN.md`

---

## 0. The symptom, and what it actually was

Zach launched Earthcall; the window never came up and Finder reported it not responding. It was
**not** a deadlock and **not** a crash. Startup blocks for **104 seconds** on a single-threaded
compute loop, and macOS marks a window unresponsive when it stops pumping events. A 12-second
`sample(1)` put **9,892 of 9,892** stacks in one place:

```
Engine::init → initLogic → ZoneManager::hydrateFromZoneStore
  → makeZoneFromJson → applyZoneJson → zoneObjectsFromJson → from_json
    → Object::setFieldShape → Object::rebuildGeometryCaches
      → geom::tessellateSdf → marchTet → mkVert → sdfNormal → evalSdf → evalLeaf
        → OntoMath::MathNode::evaluate
```

One object causes all of it: `saves/zones/NoiseFloorWorld/zone.json`, a single `SdfPrim::Expr`
at `fieldExtent (1000, 30, 1000)` carrying this tree —

```
Sub( ValueLeaf("y"),
     Scale( 15, Noise( Scale( 0.02, Add( ValueLeaf("p"), vec3(100, 0, 100) )))))
```

`rebuildGeometryCaches` (`ObjectCollision.cpp:213`) sizes the marching-tet grid as
`clamp(extent / 5, 24, 128)` per axis. Extent 1000 pins **x and z to the 128 ceiling**:

| | |
|---|---|
| grid | 128 × 24 × 128 |
| presampled corners | 129 × 25 × 129 = **416,025** `evalSdf` |
| cells → tets | 393,216 × 6 = **2,359,296** `marchTet` |
| emitted | **924,528** triangles |

A correction worth recording: I first blamed `saves/zones/Perlin Noise Floor Zone/zone.json`,
which also carries `fieldExtent: 1000`. That was wrong — its field has no `mathNode`, `expr`, or
`piecewise` payload at all, so `evalSdf` falls through to the empty-`rpn` path and returns `1e9`,
i.e. empty space. It is cheap. **`NoiseFloorWorld` is the one that costs.** (That empty-payload
object is its own question, filed separately in §6.)

---

## 1. Phase 0 — LANDED: kill the per-sample allocation in the interpreter

Two hot spots, both pure constant factor, both behaviour-preserving.

**`MathNode::evaluate`, `Op::ScalarLeaf` (`ScalarForm.cpp:1211`)** projected the *entire* variable
environment into a fresh `std::map<std::string, double>` on every evaluation. It now binds only the
variables its terms actually mention, and a **constant** form binds none. This is exact, not an
approximation: `Term::evaluate` looks each variable up by name and `ScalarForm::evaluate` only sums
terms, so a map entry no term mentions could never have been observed.

**`geom::evalLeaf`, the `Expr` arm (`Sdf.cpp:435`)** rebuilt a 4-entry
`std::map<std::string, PropertyValue>` (`x`, `y`, `z`, `p`) per sample. It now reuses one
per-thread buffer whose keys are created once, so the steady state allocates nothing. The
`piecewise` arm was deliberately left alone — a `Piece` can carry a `FunctionCall` or a `Fold` that
reads the world, and that is not a path anyone can promise never re-enters `evalLeaf`. The
`mathNode` arm was verified: `Op::SDF` and `Op::Gradient` recurse *inside* the MathNode tree, never
back out through `geom::evalSdf`.

For this tree that was six constant literals × four bound variables ≈ **24 red-black-tree inserts
per SDF sample**, times millions of samples.

| | tessellateSdf | triangles |
|---|---|---|
| before | 108.1 s | 924,528 |
| after | **64.3 s** | 924,528 |

**1.68×, identical geometry.** Debug build (`-O0`); Release constants will differ.

Verified green: `ontomath_test`, `ontomath_sounding_test`, `geometry_ontomath_test`,
`geometry_cache_test`, `test_field`. Full suite 71/74, all three failures pre-existing —
`smooth_tessellation_cache_test` (Bugs.md #11), the scratch probe (now deleted), and
`frame_lag_test`, whose only `LAG` line is world-open time measured at **65.3 s with the fix
stashed vs 66.3 s with it** — identical within noise, so not caused by this change.

**64 seconds of frozen window is still a hang.** Everything below is what actually fixes it.

---

## 2. The finding that should drive the design

**On WebGPU — the backend Zach runs — the mesh this 104 seconds produces is never drawn.**

`Object::drawFieldModel` (`ObjectRender.cpp:357`) asks the backend first:

```cpp
if (r.rendersImplicitExactly()) {
    r.drawImplicit(getFieldData(), getFieldExtent(), mat, nullptr, getMemoId(), getFieldRevision());
    return;
}
r.drawMesh(_fieldMesh, mat);
```

`rendersImplicitExactly()` is `true` only for `WebGpuRenderer` (`WebGpuRenderer.hpp:114`); the base
returns `false`. WebGPU raymarches the distance function directly. So on that path the 924,528
triangles have exactly three consumers:

| consumer | what it takes | site |
|---|---|---|
| collision support cloud | **256 points**, strided (`maxPts = 256`) | `ObjectCollision.cpp:256` |
| picking | full mesh, raycast | `ObjectRaycast.cpp:67` |
| selection outline | full mesh, shell | `ObjectRender.cpp:455` |

We spend 64 seconds building 924,528 triangles so that collision can keep 256 points, picking can
hit a ground plane, and a selection outline can be drawn. **Nothing renders it.**

A second consequence: `mkVert` calls `sdfNormal`, which is 6 uncached `evalSdf`. At 924,528
triangles × 3 vertices that is **≈16.6 M `evalSdf` for normals** against 416 K for the grid —
**~97% of all SDF evaluation in the tessellation is spent computing normals**. On WebGPU not one of
those normals is used: the support cloud takes `.pos`, picking takes positions, the shell takes
positions. They are computed and thrown away.

Design consequence for everything below: **size and populate the field tessellation for its actual
consumers, not for a render path that on this backend does not consume it.**

---

## 3. Phase 1 — resolution policy (the actual fix)

`clamp(extent / 5, 24, 128)` is wrong in both directions, and it is worth being precise about why.

Cell size is `step = 2 · extent / N`. So:

| object | extent | N | step (world units/cell) |
|---|---|---|---|
| noise floor | 1000 | 128 | **15.6** |
| small blob | 1.1 | 24 | **0.092** |

The large field gets the **most cells** and simultaneously the **coarsest sampling per world
unit** — 170× coarser than the small blob. It pays the maximum cost for the worst surface fidelity.
That is the worst of both, and it happens because *box size* is being used as a proxy for *feature
size*, which it is not. A ground plane's extent says nothing about how finely its surface varies.

Cost is also `O(N³)` while a surface is `O(N²)`; in a 1000 × 30 × 1000 box almost every cell is far
from the iso-surface and contributes nothing but two sign tests.

Three moves, smallest first. **1a is the stopgap; 1c is the real answer.**

**1a — cap total cells, not per-axis.** Replace the per-axis clamp with a global cell budget
(the aspect ratio still distributed across axes). This decouples worst-case cost from box size in
one edit and is the cheapest way to get startup back to something survivable. It does not make the
sampling *right*, it makes it *bounded*.

**1b — ⚑ AUTHOR: make cell size authored, not inferred.** The honest knob is a Person-authored
property on the field object — a cell size or feature scale — registered as a property path per
Refusal 6, with the 1a budget as its default when unset. This is the Earthcall-shaped answer: the
engine should not be guessing what a field's feature size is when the author knows. It is flagged
⚑ AUTHOR because naming it and choosing its default is Zach's call, not mine. Note the tree already
*has* the information in principle — the Perlin frequency here is `0.02`, a ~50-unit wavelength, so
a sane default is a fraction of that — but deriving a length scale from an arbitrary authored AST
in general is not something to promise.

**1c — sparse, surface-following tessellation.** The industry-standard answer, and the one CLAUDE.md's
"choose the frontier approach" points at: do not march the volume. Coarse-scan for
sign changes, then refine only cells that straddle the iso-surface (octree / adaptive marching,
dual contouring if we want the sharp-feature handling too). For a heightfield in a tall box this is
the difference between `O(N³)` and `O(N²)`, and it is what makes a 1000-unit ground plane cost what
a 1-unit blob costs.

**Do not** silently degrade fidelity to hit a budget without saying so. If a field is clamped below
its authored cell size, that should be legible — the same instinct as the audio channel's
infrasound floor, which refuses and names the frequency rather than quietly filtering.

---

## 4. Phase 2 — get tessellation off the hydration path

Even at the right resolution, this must not run inside `ZoneManager::hydrateFromZoneStore` on the
main thread. `setFieldShape` (`Object.hpp:542`) calls `rebuildGeometryCaches()` synchronously, and
hydration calls it once per field object while the event loop is not pumping. That is the
mechanism that turns "slow" into "not responding" — and it will do so again at a smaller magnitude
after Phase 1.

**Make it lazy, following the pattern already in this file.** `Object` already does exactly this
for polyhedra: `mutable bool _polyhedronDirty = true` with `rebuildPolyhedronMeshes() const`
(`Object.hpp:186-187`). Mark the field mesh dirty in `setFieldShape` and build it on first
consumer — first collision query, first pick, first `drawMesh`. On WebGPU nothing renders it, so a
ground plane's mesh may not be built until a Person first walks on it, and then once.

Two caveats worth stating plainly:

- Lazy **moves** the stall to first touch rather than removing it. Phases 1 and 2 are a pair; lazy
  alone just relocates a 64-second freeze to the moment someone steps on the ground.
- Watch `_fieldRevision` (bumped at the top of `rebuildGeometryCaches`) and the WebGPU memoized
  program cache keyed on it — deferring the rebuild must not leave a stale revision handed to
  `drawImplicit`. That cache is being written right now by another session (§6).

If first-touch latency is still bad after Phase 1, build off-thread and swap in. The Phase 0
scratch buffer is `thread_local`, so it is already safe under that.

---

## 5. Phase 3 — normals from the grid, or not at all

Per §2, ~97% of SDF evaluation in a tessellation is `sdfNormal`, and on WebGPU every one of those
normals is discarded.

`tessellateSdf` already presamples the full `(N+1)³` corner grid into `g` and then throws that work
away for normals, re-evaluating the field six more times per vertex at `Sdf.cpp:498`. Two options,
not exclusive:

- **Take the gradient from the grid.** Central-difference the sampled grid at the 8 corners of the
  owning cell and trilinearly interpolate the *gradient* (interpolating gradients, not
  differencing interpolated values, is what keeps it smooth across cell boundaries). Zero
  additional `evalSdf`. Normals get slightly softer at low resolution; for a ground plane that is
  invisible, and on WebGPU it is doubly invisible because the mesh is not drawn.
- **Do not compute them when nobody wants them.** Let the caller say whether it needs normals.
  Collision (`.pos`), picking, and the selection shell do not. The GL backend, which *does*
  `drawMesh(_fieldMesh)`, does.

Expect this to dominate the remaining constant factor — it is a larger multiple than Phase 0 was.
It should land **after** Phase 1, so the win is measured against a sane cell count rather than
flattering itself against 128³.

---

## 6. Phase 4 — the per-frame cost (a separate bug, same object)

Distinct from startup: once loaded, the frame loop is dominated by

```
Engine::tick → update → LocomotionChannel::step
  → Physics::enforceCollisions → Object::updateCollisionZone
```

`enforceCollisions` (`Physics.cpp:668`) calls `updateCollisionZone(transform)` for **every object on
every call, unconditionally**, and `updateCollisionZone` (`ObjectCollision.cpp:110`) rebuilds the
world AABB by transforming the **whole `_supportCloud`** — up to 256 points per object per frame,
including objects that have not moved. This is the "EXTREMELY LAGGY" of commit `765d74cf`.

**The fix is already described in the source; it was just never implemented.** `Object.hpp:204-207`
declares

```cpp
// Cached local-space AABB of the topology mesh, so updateCollisionZone only
// transforms 8 corners instead of the whole (huge) support cloud per call.
glm::vec3 _localMin{-0.5f};
glm::vec3 _localMax{ 0.5f};
```

Both members are in `HEAD` and **neither is read or written anywhere in `src/`**. The comment
describes an optimization that does not exist. So:

1. Populate `_localMin` / `_localMax` in `rebuildGeometryCaches` alongside `_supportCloud`.
2. Have `updateCollisionZone` transform the 8 corners of that local AABB.
3. Skip the recompute entirely when the transform and `_fieldRevision` are unchanged since last call.

One correctness note to verify rather than assume: for an affine transform, the AABB of the
transformed corners of a local AABB is a **superset** of the AABB of the transformed points. That is
conservative and therefore safe for rejection/broadphase, but it is *looser*. Confirm nothing
downstream of `collisionZone` — `computePointPenetration` in particular — depends on tightness
before swapping, or the change trades a performance bug for a behaviour one.

---

## 7. Sequencing, and what not to do

1. **Phase 1a** — cell budget. Unblocks startup, one edit, immediately measurable.
2. **Phase 4** — the dead AABB cache. Independent of everything else, fixes the frame lag, small.
3. **Phase 3** — normals from the grid. Biggest remaining constant factor; measure after 1a.
4. **Phase 2** — lazy build. Removes the class of "hydration freezes the window".
5. **Phase 1b/1c** — ⚑ AUTHOR on the property; sparse marching as the structural answer.

**Do not re-record `frame_lag_baseline.txt` upward to quiet this.** Its world-open line was `LAG` at
~66 s against a 1.87 s baseline, and it was red *correctly* — the baseline predates
`saves/zones/NoiseFloorWorld/zone.json` (2026-08-27 19:40). Re-baseline **downward** as each phase
lands, per the Performance section's own rule. *(As of the §9 pass this test passes again in 3.7 s;
the downward re-record is now the outstanding action.)*

**Do not touch the save files to make this faster.** Lowering `fieldExtent` on
`NoiseFloorWorld/zone.json` would mask the bug by shrinking Zach's world. Save files are sacred; the
engine is what is wrong here.

Three loose threads found while measuring, filed rather than fixed:

- **`saves/zones/Perlin Noise Floor Zone/zone.json` holds a field with no expression payload** —
  `prim: 7` (`Expr`) with no `mathNode`, `expr`, or `piecewise`. It evaluates to `1e9` everywhere,
  i.e. empty space, silently. `SdfJson.cpp:20` documents this exact failure shape for the piecewise
  arm. Whatever authored that zone lost its expression on the way to disk, and the loss is silent.
  Worth a look before someone concludes the Perlin floor "just doesn't render".
- **`earthcall_webgpu` does not currently build.** `WebGpuRenderer.hpp:191` uses `sdfwgsl::Program`
  and `std::unordered_map` without including `SdfWgsl.hpp` or `<unordered_map>` — an in-flight
  memoized-program cache from a concurrent session (the same work as the *GPU AST Interpreter and
  WGSL Tiering 8-28-26* intercom thread). Two missing includes. Left alone deliberately; Phase 0 was
  verified through `earthcall_core` and the test targets instead.
- **`src/ConstructedBeing/Singular/Object/Object.hpp.orig`** is sitting in the tree (gitignored,
  not compiled, but it does pollute searches).

---

## 9. Review of the implementation (2026-08-28, later the same day)

Gemini implemented Phases 1a, 2, 3 and 4. The shape of all four was right and the measured
result is decisive. Two correctness bugs came out of review, both of the kind this codebase
cares about most — silent wrong answers rather than crashes.

### What landed, and what it bought

| | before | after |
|---|---|---|
| app startup to "Game initialised" | 106.3 s | **2.7 s** |
| `setFieldShape` on the hydration path | ~104 s | **0.4 ms** |
| first touch (lazy tessellate) | — | **1,154 ms** |
| `tessellateSdf` in isolation | 108,113 ms | ~1,150 ms |
| `frame_lag_test` | 105.7 s, FAILED | **3.7 s, passed** |
| full suite wall time | 409 s | **54 s** |

Suite is **72/73**, the one failure being `smooth_tessellation_cache_test` (Bugs.md #11), which
is the documented pre-existing failure and matches CLAUDE.md's stated baseline exactly.

### Bug 1 — grid gradients ignored cell spacing (fixed)

Phase 3 takes vertex normals from a central-difference gradient over the presampled corner
grid, which is exactly right. But it differenced raw grid **values** without dividing by the
per-axis spacing, and `step` is `2*extent/N`, which is anisotropic whenever the box is — the
noise floor's is 1000 × 30 × 1000. Normalizing an unscaled `(dx, dy, dz)` therefore aims the
normal in the wrong direction.

Measured on an exact plane `f(p) = y - 0.5x`, where the answer should be exact at any
resolution: **5.4° mean error, 23.4° worst-case** under a lopsided grid, and 12.5° worst-case
even on an isotropic one (the one-sided differences at the box faces span one step, not two,
and were not compensated either). Fixed by dividing each difference by the world-space span it
actually covers, which handles interior and boundary uniformly. Both cases now measure 0.02°.

Nothing in the suite caught this: the existing normal check asserts unit **length**, which a
wrong direction passes. `geometry_ontomath_test` now carries a direction check under a
deliberately lopsided grid; verified to fail without the fix.

This was latent on WebGPU (which never draws the mesh, per §2) but live for the GL backend and
for anything else that consumes `TessVertex::normal`.

### Bug 2 — polyhedron collision bounds silently shrank to ±0.5 (fixed)

Phase 4 switched `updateCollisionZone` from transforming the support cloud to transforming the
8 corners of `_localMin`/`_localMax`. Correct for every shape whose bounds
`rebuildGeometryCaches` populates — but **no polyhedron mutation point calls it**.
`setPolyhedronData`, `createTetrahedron`/`Octahedron`/`Dodecahedron`/`Icosahedron`,
`createCustomPolyhedron` and `setPolyhedronVertexLocal` all set `_polyhedronDirty` and stop, so
every polyhedron in the world kept the `±0.5` default while the old code read
`polyhedronData.vertices` directly.

Regular solids survive by luck — `scaleToRadius(0.5)` keeps them inside ±0.5. But
`createCustomPolyhedron` applies **no scaling at all**, and `setPolyhedronVertexLocal` lets a
Person drag a vertex anywhere; its comment even promises to "refresh derived data (normals,
bounds)", and the bounds half had become a no-op. An under-sized zone makes
`computePointPenetration` reject points that are really inside — things you walk through.
Fixed by reading the vertices in the polyhedron case, as before; the transform cache Phase 4
added still keeps it off the per-frame path, which was the actual win.

### Smaller things fixed in the same pass

- **Phase 0's `evalLeaf` scratch had been reverted** — the `Expr` arm was back to constructing a
  fresh 4-entry `std::map` per sample. Restored. (The ScalarLeaf half survived.) It matters less
  now that Phase 3 has removed ~97% of `evalSdf` calls, but it is free.
- **`getSupportCloud()` returned an empty vector** for a field whose lazy mesh had not been
  built, which `geometry_cache_test` caught. Handing back an empty cloud is precisely the silent
  wrong answer that cache exists to prevent — an empty cloud makes an object unpickable and
  non-collidable, which the Bezier assertion right below it locks down by name. The accessor now
  forces the build: asking for the cloud *is* the demand that builds it.
- **Three `const_cast<std::vector<glm::vec3>&>(_supportCloud)` removed.** `_supportCloud` is now
  `mutable`, which is what actually says "not part of the object's observable value"; casting
  away const on a member of a genuinely const object is undefined behaviour.

### Still open

- **The cell-budget floor is applied before the budget scale.** `rebuildFieldMesh` raises each
  axis to 24 and *then* multiplies by `cbrt(MAX_CELLS/total)`, so an axis can finish below 24
  (the noise floor's y lands at 12). Only `max(4, ...)` is a real floor. Probably harmless,
  possibly not what was intended.
- **A field's `_localMin`/`_localMax` stay at ±`_fieldExtent`** and are never tightened to the
  mesh bounds once it exists, so a ground plane's collision AABB is its whole box (±30 in y
  where the surface spans ~±15). Conservative, therefore safe, but looser than it needs to be.
- **Phases 1b and 1c are untouched** — the authored cell size (⚑ AUTHOR) and sparse
  surface-following tessellation. 1a bounded the cost; it did not make the sampling *right*.
  Planned in [SPARSE_FIELD_TESSELLATION_PLAN.md](SPARSE_FIELD_TESSELLATION_PLAN.md), which also
  carries the four corrections listed here.
- **`frame_lag_baseline.txt` should now be re-recorded downward**, on a quiet machine, per the
  Performance section's own rule. Not done here — this machine had just finished a full build.

---

## 8. Attribution

The symptom, the machine, and the decision to take the interpreter allocation first are Zach's —
he reported the not-responding window, and when given the four options chose Phase 0 explicitly
before the policy fix, which was the right call: it is the one change with no behavioural
question attached, and it made every subsequent measurement cheaper to take. The request for this
document is his as well.

The measurements, the stack sampling, the identification of `NoiseFloorWorld` (and the correction
of my own first, wrong answer), the observation in §2 that WebGPU never draws the mesh it takes 64
seconds to build, and the phase ordering are mine.

The `_polyhedronDirty` lazy pattern Phase 2 proposes to follow is already in `Object.hpp` and
already right — Phase 2 only extends it to fields. Likewise the `_localMin` / `_localMax` comment
in Phase 4: somebody already worked out the correct fix and wrote it down; this plan is asking for
it to be implemented rather than proposing anything new. Sparse surface-following tessellation
(1c) is standard practice in the isosurface literature, not an invention here — named in the
spirit of CLAUDE.md's rule against re-deriving solved problems, the same way Rete was reached for
in the law engine.

The GPU-side half of this problem — tiering the AST interpreter toward WGSL so the CPU path stops
being the only path — is live in `agent intercom/communication-threads/GPU AST Interpreter and WGSL
Tiering 8-28-26.md`. Phase 0 and Phase 3 are the CPU-side counterpart of that conversation, not a
competitor to it: the authored path has to stay honest and evaluable on the CPU whatever the fast
path does, which is exactly what the dual-path parity item under **Performance** is guarding.
