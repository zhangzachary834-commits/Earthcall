# Review of the August 2026 rendering-optimization campaign — what was kept, what was reverted

**Author**: Claude Opus 5 (Claude Code)
**Session**: `4e6ef036-ad44-4bc6-97b9-a8704274736e`
**Date**: 2026-08-31, 00:20–02:10 PDT
**Range reviewed**: `7a76eb88` ("TONS of lag work.") … `a49cb55a` ("Revert math tweak to restore 30-40 fps performance"), plus the two commits after it that touch the same code.
**Asked for by**: Zach — *"review the many rendering optimizations that were made (Gemini did this). Sift through it to keep what's good and revert what's bad."*

---

## The premise the campaign was built on was false, and Zach found that out afterwards

Zach, mid-review:

> *"those changes were made trying to increase a frame rate that was mysteriously capping out at 20-40 fps — but later we dug in to find the real cause and I found it was because of the two ourverse laws (don't fix or audit those right now, it's just so you know that they were taking up like 20-30 ms together which is why it was capping out, while rendering was actually one of the faster phases)."*

That reframes everything below. Roughly two dozen commits traded correctness and
fidelity for frame time in a subsystem that was not the bottleneck. `frame_lag_test`
agrees: a steady simulation frame in the Chess world is **1.24 ms** today, and the
probe measures no rendering at all.

So the rule I applied when sorting: **a change that is faster AND still true, keep;
a change that bought speed by making the picture less true, revert** — because the
speed was never the thing that was wrong.

I did not touch the Ourverse laws, as asked.

---

## Kept — this is real work and most of it is good

| Kept | Why |
|---|---|
| **Instanced SDF drawing** (`SdfInstanceData` SSBO on `@group(1)`, one draw per pipeline) | Structurally right. Per-object model/colour/extents moved off the uniform and into an instance array; costs nothing in truth. |
| **Mesh batching with `baseColor` per instance** | `MeshBatchKey` no longer keys on colour, so identically-shaped, differently-coloured objects batch. Right fix, right place. |
| **Program memoization on `(memoId, fieldRevision)`** | Exactly the pattern Opus 5 prescribed to Gemini in `agent intercom/communication-threads/GPU AST Interpreter and WGSL Tiering 8-28-26.md` — a revision counter, not a topology hash. The revision counter argument was taken and implemented correctly. |
| **`_fieldRevision++` in `rebuildGeometryCaches()`** | The mutation point every path already calls. |
| **Lazy `_fieldMesh`** (`_fieldMeshDirty` + `rebuildFieldMesh()`) | Kills the 30–100 ms blocking marching-tets on construction under a backend that never reads it. The To-do list asked for this. Demand-forcing is wired at all four readers (`getSupportCloud`, `raycastFace`, `drawHighlightOutline`, the mesh draw path). |
| **`updateCollisionZone` memo + 8-corner local AABB** | Was transforming the whole support cloud on every call. Now 8 corners, skipped entirely when transform and revision are unchanged. |
| **`rebuildGeometryCaches()` in the polyhedron creators and `setPolyhedronData`** | Fixes a real stale-cache bug; `setPolyhedronData` had an empty `if` block where the call belonged. |
| **`MathNode::evaluate` ScalarLeaf narrowing** | Binds only the variables a form actually reads. Exact, well argued in its own comment, and the constant case (every numeric literal in a field tree) now needs no environment at all. |
| **Thread-local `vars` map in `evalLeaf`** | Correctly reasoned as Kernel scratch, and it names why re-entry can't happen. |
| **Tetrahedral SDF normal** (4 taps, not 6) | Standard, isotropic, 33 % cheaper. |
| **GPU particle generation** | The xorshift moved into the vertex shader; no CPU vertex buffer. |
| **`paintNewObject` via `ownMaterial()`** | CLAUDE.md's paint non-negotiable names `ownMaterial` explicitly, and per-face `setFaceColor` was what broke batching. |
| **Grid-gradient tessellation normals + the anisotropic-grid regression test** | The test in `geometry_ontomath_test.cpp` is genuinely good work: it caught a 5.4° normal error on an exact plane that a unit-length assertion passed. |
| **Authored `field.cellSize` property** | Registered as a property path. Refusal 6 satisfied. |
| **`PerformanceMetricsWindow` (F3)** | This is the tool that let Zach find the actual 20–30 ms. It earned its place. |
| **`renderMode` serialization**, `createPrism(int, float, float)` | Fine. |
| **Deleting `tests/singularity/math_json_test.cpp`** | It was a scratch `main()` that printed JSON, not a test, and it no longer compiled. Staying deleted. |

---

## Reverted — bought speed by making the picture less true

### 1. The GPU stopped computing the noise the CPU computes

`SdfWgsl.cpp` redefined the shader's classic-Perlin function as an alias for simplex noise:

```wgsl
fn cnoise3(P: vec3<f32>) -> f32 { return snoise3(P); }
```

The CPU evaluator kept calling `glm::perlin` (`ScalarForm.cpp:1593`), and
`MathNode::Op::Noise` is declared in the header as *"Perlin noise (Vector→Scalar)."*
Both are "noise", both look like terrain, every test stayed green — and **the ground a
Person sees stopped being the ground they collide with**, because collision reads the
CPU field and rendering reads the shader. This is the case CLAUDE.md's OntoMath rule
exists for: a channel reads the mathematics, it never decides what the mathematics is.

Restored the real `cnoise3`, and removed `snoise2` / `snoise3` / `cnoise2` /
`permute3` / `mod289_2`, which nothing else referenced and which were being compiled
into every SDF pipeline.

**Locked**: new `Expr(noise)` case in `webgpu_sdf_parity_test`, a noise-displaced
sphere authored as a `mathNode` the way a save carries it — noise reaches the shader
only through OntoMath, never through `makeImplicit`'s RPN, which is why no existing
case touched it. Verified both ways: with the transcription correct GPU and CPU agree
to **2 pixels of 261**; re-injecting the simplex substitution produces **diff = 128
against a tolerance of 10**.

That tolerance is deliberately an absolute override rather than the file's
perimeter heuristic. The heuristic is calibrated for two marchers landing either side
of an epsilon on a shape they already agree about; it is roughly half the frame here,
and I confirmed the case passes under it even with the noise amplitude changed by a
third. A lock that cannot fail is not a lock.

### 2. The marcher stopped being able to hit thin things

```wgsl
for (var i = 0; i < 28; i = i + 1) { ...
    let s = max(d * 0.95, max(1.5, t * 0.06));   // implicit-AST branch
```

Two problems. The iteration budget went 192 → 48 → 28. And the step rule imposes a
**minimum step of 1.5 world units** regardless of what the field says, so anything
thinner than 1.5 units is stepped straight through. It also threw away the gradient
normalization (`d = raw / |grad f|`) that made the marcher correct for an *arbitrary*
authored expression rather than for terrain in particular — an authored expression is
not a distance field, and its value overstates the distance to its own zero set by
exactly `|grad f|`.

Restored the gradient-normalized step and the 192-iteration budget. Kept the secant
root refinement (a real improvement), kept the AABB planar leap and the upward early
exit (both are bounding-box facts, not terrain assumptions, so they are sound for any
field), and kept the Keinert over-relaxation on the exact-SDF branch.

`webgpu_sdf_parity_test`'s `Expr(iso)` case — `x*x + y*y + z*z - 0.3`, documented in
that file as *"NOT a distance… which is what forces both marchers to damp their
steps"* — passes on the real GPU with diff 0.

*(Note for the record: the specific 1.5-unit rule is what `a49cb55a` restored by hand
when the "Maximum Safe Cone Stepping" replacement measured slower. That was a
reasonable call on the information available at the time — the information being that
rendering was the bottleneck, which it was not.)*

### 3. Terrain vanished past 600 units

```cpp
float horizonDist = glm::min(maxDim * 4.0f, 600.0f);
```

A hard world-space cap on the march distance, halved from `maxDim * 8.0f` and then
clamped. A large authored terrain is exactly the case that trips it, and the failure
is silent — the far field simply is not there. Restored `maxDim * 8.0f`, which is the
object's own diagonal budget rather than a world constant.

### 4. The collision mesh went 6× coarser vertically — and this is Bugs.md #12

`rebuildFieldMesh()` replaced the per-axis rule `clamp(extent/5, 24, 128)` with a
**single `cbrt()` scale of all three axes to a 125 000-cell budget, floored at 4**.
A field box is routinely lopsided, and a uniform scale takes the same fraction off the
thin axis as the fat ones — starving the axis that had least to give.

For the noise floor (`fieldExtent = [1000, 30, 1000]`, terrain amplitude ±40):

| | resolution | vertical cell height |
|---|---|---|
| before | 128 × **24** × 128 | 2.5 units |
| campaign | 160 × **4** × 160 | **15 units** |

Four samples across 60 units of height. Bugs.md #12, in Zach's words, is a Person
walking on precisely that:

> *"they slid on hills normally until reaching a certain low point where they can't go
> any lower. It's like they're sliding across an invisible rectangular platform
> hovering way above the valleys below."*

Restored the per-axis floor and cap. The cell budget stays, but moved to where it
belongs: guarding an **authored** `cellSize`, which is the one that can hang a window,
and saying out loud when it clamps. The lazy build is what makes the old resolution
affordable again — it is no longer paid on construction.

### 5. An unsound interval bound that silently deletes geometry

`MathNode::evalRange` returned `[-1, 1]` for `Op::Noise`. `geom::evalRange` feeds that
to `tessellateSdf`'s new subdivision, which **discards any cell whose interval does not
straddle zero**. A bound the noise can leave means cells that really do contain surface
get deleted, with nothing logged.

Measured: `glm::perlin` over 8 × 10⁶ random samples returns **[−1.127, +1.123]**. The
claimed bound was already wrong at the values the noise floor reaches. Replaced with
2.2 · √3/2 = 1.905, the classical supremum for 3-D classic Perlin times glm's own
output scale — provably conservative, and only ~1.7× looser than measured, so it costs
a little pruning and never correctness.

### 6. Debug output and dead controls

- `printf("wgpuDeviceCreateRenderPipeline compiled novel SDF in %.2f ms")` — in the
  render path, on stdout, every novel pipeline. Removed.
- `std::cout << "[load] globalObjects registration took …ms"` in `ZoneManager::loadState`. Removed.
- `ScreenChannel::renderScale` and `ScreenChannel::performanceMode` — registered as
  law-visible property paths and **read by nothing**. A Person could author
  `@screen.renderScale = 0.5` and get silence. Removed rather than left lying.
- The dead `getSample` lambda in `tessellateSdf`, 20 lines carrying its author's
  unresolved reasoning as comments (*"Wait, for the normal we need neighbors…"*).
  Removed.

### 7. Two latent defects in kept work

- **`getMemoId(suffix)` had a stride of 100.** A complex shape's 101st sub-patch
  addressed the *next object's* compiled program. Widened to 2²⁰.
- **`Object::getAliveCount()` counted only the two declared constructors.** Object's
  copy and move constructors are compiler-generated, so a copied Object decremented at
  destruction without ever incrementing and the metric walked negative. Moved onto a
  member guard that counts on every construction path.
- **`rebuildFieldMesh()` tightened `_localMin/_localMax` without invalidating
  `updateCollisionZone`'s memo**, so the tightening never landed until the object next
  moved. Now invalidates.

### 8. Opening a world costs 3.7x what it did five days ago, and the baseline was hiding it

`load.ms` was re-recorded at **5279.38** on 2026-08-28, past the 4000 ms aspiration,
which is the shape CLAUDE.md warns about: *"Never quiet a STANDING line by widening
the baseline."*

My first reading of this was wrong and I am correcting it here rather than leaving it
in the file. I measured 1375 ms, saw the probe print *"(baseline is stale and slack —
re-record it)"*, and concluded 5279 had been a busy-machine artifact. It was not. The
1375 ms was not a fast load — boot was throwing on `home.json` (§A below) and the load
was **abandoning partway**. With that fixed the world actually opens: 441 objects
across 12 zones, and four runs on a steady machine (drift 1.00–1.06) give
**6024 / 6440 / 6791 / 6931 ms**.

Of that, ~1200 ms is bought deliberately by §4 above — A/B'd on this machine at
5.4–5.6 s with the coarse field resolution against 6.4–6.9 s with it restored. The
other **~5.4 s predates this pass and is unexplained**; it is a real regression against
the 1868.93 of 2026-08-26 and it wants finding.

Recorded at 6950, to the dearest of four runs, with the whole derivation written into
the file's header so the next person does not have to re-derive it. It reads STANDING,
which is what a known, listed, unpaid cost should read. On the To-do list.

Two things follow that are worth naming:

- **The lazy field mesh is not as lazy as its To-do entry claims.** The A/B only shows
  a difference if the tessellation is running during load — so something on the
  hydration path still forces `rebuildFieldMesh()`. The claimed "hydration cost 104 s →
  0.4 ms" holds for `setFieldShape`; it does not hold for the whole load.
- The rest of the baseline file was left alone. Several lines are *tighter* than
  reality (`steady.zone_ms` records 0.797 against a measured 1.157) and re-recording
  them wants a quiet machine, which is a task, not this pass.

### 9. Sixty-four agent scratch files were tracked at the repo root

`fix_*.py`, `patch_*.py/.patch`, `rewrite_*.py`, `test_*.py`, `add_particle_*.py`,
`update_*.py`, `tune_math.py`, `check_range.cpp`, `test_eval*.cpp`, `test_perf.cpp` —
one-shot rewriters an agent wrote because it could not edit a file in place. Untracked
(left on disk, nothing deleted) and covered by new `.gitignore` / `.ignore` blocks,
along with `*.orig` / `*.rej`. `src/.../Object.hpp.orig` deleted. `build_release/` and
`build-wasm/` added to `.gitignore`.

---

## Two things found that are NOT the rendering campaign, and both were load-bearing

I would not normally touch either, but the first stopped the app from starting at all
and the second had a test already red against a CLAUDE.md non-negotiable.

### A. Earthcall did not boot

```
libc++abi: terminating due to uncaught exception of type nlohmann::detail::type_error:
[json.exception.type_error.302] type must be binary, but is object
```

`BinaryPack::Writer::toBinaryJson()` returns `nlohmann::json::binary`. A save packed as
msgpack/CBOR round-trips it; a save written as **text JSON cannot** — `dump()` degrades
it to `{"bytes":[…],"subtype":…}` and re-parsing yields an object, on which
`get_binary()` throws. Every save under `saves/` is text JSON. So every polyhedron,
Bezier patch and convex hull written with a binary pack produced a file that could be
written and never read; `saves/homes/Home/home.json` is one, and it is on the boot path.

**Fixed on the reader side only** — `BinaryPack::bytesFrom()` accepts either shape, and
the four `get_binary()` call sites route through it. **No save file was touched, and
nothing already written is lost.** This is the fix that made both halves of the format
agree; it does not need a migration.

This alone took the suite from 67/73 to 71/73 — `chess_app_test`,
`chess_click_geometry_test`, `chess_gesture_test` and
`zone_boot_hydration_relations_test` were all this one exception.

### B. Edge-triggered laws had started firing every tick

Commit `04c52ed4` ("Fps measuring") also changed, inside the timing instrumentation:

```cpp
-  if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {
+  if (hasTerminals) {
```

That Rete fast path applies to every matching subject with **no edge check at all** —
which is right for a level-triggered law and wrong for an edge-triggered one, which
then re-fires for as long as its condition keeps holding. The comment on the very next
branch still says so: *"OnBecomeTrue and laws without Rete terminals: full sweep path.
Edge detection requires knowing when a being LEAVES the match set."* CLAUDE.md's
non-negotiable: *"Event-transitions must be edges, not levels."*

`rete_compile_test` had gone red on exactly its own assertion — *"an edge fires once,
not once per tick."* Guard restored, with a comment recording that it has been deleted
once and why it only looks redundant. **This is the one judgment call here that Zach
may want to reverse in one line** — if the guard was removed on purpose for the
matching speedup, the right shape is an edge check inside the fast path, not no check.

---

## Verification

- `cmake --build build -j8` clean.
- `ctest`: **72 / 73**. The only failure is `smooth_tessellation_cache_test`, the
  pre-existing one Bugs.md #11 already documents. Before this pass it was 67/73.
- `webgpu_sdf_parity_test`: **21 / 21** shapes agree with the CPU on a real WebGPU
  device — including the new `Expr(noise)` case, and including `Expr(iso)`, which is
  the un-Lipschitz path I restored.
- `frame_lag_test`, `webgpu_micro_mastery_lag_test`, `ground_plane_test`,
  `geometry_ontomath_test`, `geometry_cache_test`, `gpu_mastery_test`,
  `webgpu_particle_test`, `webgpu_object_test`: all pass.
- `./build/earthcall` boots to *"Earthcall Game initialised."* with **zero** errors and
  runs; before this pass it terminated during init.
- The noise lock was confirmed to FAIL when the defect is re-injected, not merely to
  pass when it is absent.

---

## Addendum — the five Perlin-zone bugs Zach added mid-review

All five are recorded in full in `docs/Agenda/Tasks/Bugs/Bugs.md` #15-#19. In short:

- **#15, "can't fly down anymore"** — `Zone::update` computed `groundY` as
  `origin.y + 0.5 * scaleY`, the top face of a unit box, for whatever being carries
  `baseline = "ground"`. Correct for the cube placeholder it was written for, meaningless
  for a field: the Perlin floor produced a **flat invisible plane at y = -1.5 across the
  whole world**, and `Physics::integrate`'s "never allow below ground" clamp then held
  everything above it while the valleys underneath reach y ≈ -42. A field ground now
  contributes the lowest point of its collision zone instead, so the clamp is the floor of
  the world and the shape of terrain is left to mesh collision. Guarded by a new case in
  `ground_plane_test`, confirmed RED (body held at exactly 1.000000) and GREEN (rests at
  -3.500000 on a surface at -4.0). **This is probably the larger half of #12 too, and a
  better account than the resolution loss §4 records** — Zach's words are "an invisible
  rectangular *platform*", which is a plane, not a staircase.
- **#16, 40 ms of interaction in an empty zone** — the hover pick walked every triangle of
  every object with no broadphase, twice per frame. Measured on the real terrain:
  **17.8 ms per scan of 194 176 triangles**, and the same 17.8 ms aimed at empty sky.
  New `geom::TriGrid` (uniform grid + Amanatides-Woo DDA, chosen over a BVH because
  marching-tet output is the uniform distribution a grid is optimal on), plus a slab test
  before the lazy tessellation is forced. 17.819 → 0.001 ms looking down, bounds-rejected
  aimed up, 17.434 → 0.066 ms grazing, identical hit points. Guarded by
  `tri_grid_test`, which demands the index equal the exhaustive scan on 4 000 random rays
  across four awkward meshes.
- **#17, horizon render cost** — inherent: a grazing ray crosses the world, and each step
  of an implicit AST also pays three `sdfEval` calls for the gradient §2 restored. Took the
  one free reduction: the march is clamped to the **camera's far plane**, unprojected from
  the projection actually in force. Everything past it is clipped before it is seen, so
  this cannot remove anything a Person could have seen — the distinction from the hardcoded
  600-unit horizon §3 reverted. The rest is open and the plans for it already exist.
- **#18, warping corners** — found and corrected a real defect in the Keinert over-relaxation:
  the rollback rolled `t` back correctly and then **spent `d` from the sample taken at the
  overstepped point**, which a 1-Lipschitz field permits to exceed the radius at the
  corrected position. Now discards it and re-evaluates, as the paper specifies. **I could
  not prove this is Zach's artifact.** I added interior-hole detection to
  `webgpu_sdf_parity_test`, re-injected the original rollback, and got zero holes at 32×32 —
  so the check did not witness it, and it says so in the file rather than passing as a guard.
- **#19, toruses as mesh** — the dispatch is correct and was checked against the running
  app, not read: an instrumented `earthcall_webgpu` prints `kind=8 exact=1 renderMode=0 ->
  SDF`. Two candidates left for Zach: the F3 counters (`sdfDrawCalls` counts pipeline
  batches, so 72 toruses are legitimately **one** draw call), or the OpenGL binary, where
  every analytic shape really is a mesh. That second one was a documentation trap —
  CLAUDE.md's build block named `--target earthcall`, the OpenGL target; both it and
  `BUILD_AND_ENVIRONMENT.md` now name `earthcall_webgpu` and say what the difference is.

### #20 — the campaign's worst regression, found last

Chess pieces invisible except the four rooks, "except when I am very close to them",
while still moving and taking clicks. One line:

```wgsl
let maxDist = min(box.y, inst.misc.z);   // t is measured from the EYE
```

`misc.z` is `maxDim * 8` — a LENGTH, how far to march *inside* the volume. Compared
against `t`, which starts at the box entry distance from the camera, it becomes
"objects further than `maxDim * 8` from the eye do not exist." A chess piece's
`maxDim` is ~0.6, so the cutoff is **4.8 units**. The rooks are the only pieces that
are `ShapeKind::Cube` and draw through the mesh path; every other piece is analytic
and raymarched. Picking kept working because picking is CPU geometry.

Before the campaign this line was `let maxDist = box.y;` and `misc.z` was carried in
the uniform and **never read**. Now `min(min(box.y, t + inst.misc.z), farField)`.

Reproduced headlessly *before* fixing, in a new `webgpu_sdf_distance_test`: a
pawn-sized sphere goes 804 lit pixels at 2 units, 788 at 4, and **0 at 8 and beyond** —
`maxDim * 8 = 4.8` sitting exactly at the cliff. After: 804 / 788 / 804 / 840 / 1044 /
1852 out to 200 units.

**Nothing in the suite could have caught it.** `webgpu_sdf_parity_test` renders all 21
of its cases from one camera 3 units away — inside any plausible budget. Distance was a
free variable no test varied, and 21 green shapes said nothing about it. That is the
lesson worth keeping from this whole review: the campaign's tests all measured the axis
the campaign was optimising, and every regression it shipped hid on an axis nobody
sampled.

## Left open, deliberately

1. **`saves/zones/Ourverse Gathering/zone.json` is 36 MB**, of which 19 MB is
   `stakeholders`. One object carries **2 694 entries of which 300 are unique** — nine
   duplicates each, all `{"authorId": "Player", "lawId": "law-30", "propertyPath":
   "position.y"}`. That is a stakeholder record appended per application rather than
   per author. Related to the two Ourverse laws Zach named; not touched, per his
   instruction. Save files are sacred — this wants a deliberate, authorized pass.
2. **`_programCache` in `WebGpuRenderer` is never evicted.** Bounded by objects ever
   created in a session, so it is a slow leak, not a bug. Needs a liveness signal to
   fix properly.
3. **~5.4 s of the world-open cost is unexplained** (§8). It is not the field
   resolution — that was A/B'd and accounts for ~1.2 s — and it landed somewhere
   between 2026-08-26 and now. Related: something on the hydration path still forces
   the "lazy" field tessellation, or the A/B could not have moved.
4. **`frame_lag_baseline.txt` wants a full quiet-machine re-record.** `steady.zone_ms`
   is recorded at 0.797 against a measured 1.157.
5. **`g_astEvaluations`** adds an increment and a branch to the hottest loop in the
   evaluator, and never flushes the last <1024 per thread, so the lifetime total
   under-reports. Cheap, but it is telemetry in the inner loop; worth a flag if the
   evaluator ever becomes the cost again.
6. **Bugs.md #12 should be re-tested by a Person** now that the collision resolution is
   restored. I have the arithmetic and the mechanism; I do not have feet on that hill.

---

## What Zach should see

- **The app starts.** It did not before this pass.
- **Chess works again in the tests**, and the four tests that had gone red are green.
- **On the Perlin floor**: dropped cubes should now settle into the valleys instead of
  stopping on an invisible plateau ~15 units up. This is the Bugs.md #12 check.
- **Terrain should no longer disappear** at middle distance on large fields.
- **The ground you see and the ground you walk on are the same ground again** — the
  rendered noise now matches the noise physics and collision use.
- **Nothing prints per-frame or per-pipeline to the console** during play.
- **F3 still opens the Performance & Coordinates window**; its object counts no longer
  drift negative.
- Expect the SDF raymarch to cost more than it did yesterday. That is the trade being
  made deliberately, on Zach's own finding that rendering was never the 20–40 fps cap.
