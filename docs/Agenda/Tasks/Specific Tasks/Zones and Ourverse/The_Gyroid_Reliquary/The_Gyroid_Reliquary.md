# The Gyroid Reliquary — a Zone authored entirely in OntoMath

*opencode (space-bunny-free) · session `gyroid-reliquary-2026-09-29` · 2026-09-29*

**Update 2026-09-29, after Zach ran it:** the Zone shipped unable to draw. Every
radiant being refused WGSL compilation, once per frame, forever. Cause and fix in
§4.5 — it is the sharpest thing in this document and the reason the test grew
from 72 checks to 119.

**What was asked:** *"CREATE A NEW ZONE AND MAKE IT INCREIBLY GORGEOUS… MAKE IT SO
WONDERFUL AND MIND BLOWING, UNLIKE ANYTHING THIS EARTH HAS EVER SEEN BEFORE… AS
LONG AS IT'S IN THE SAME ZONE A NEW ONE."*

**What was built:** a cathedral with no walls, whose entire architecture is one
triply-periodic minimal surface — the gyroid — solved, not modelled. Plus the
Radical Transparency: every number that shaped it is a registered property, and
four things the Zone discovered about itself that I would otherwise have written
into it as confident prose.

---

## 1. What it is

The Reliquary is a hollow 60 m sphere of woven stone. There is not one modelled
wall in it. The whole of the architecture is

```
g(x,y,z) = sin(kx)·cos(ky) + sin(ky)·cos(kz) + sin(kz)·cos(kx)
```

A **minimal surface** is the Plateau problem's answer: the soap film between two
wire loops, the shape a surface takes when pulled as tight as it can possibly be.
The gyroid is the only such surface that fills all of space, and it does it
periodically, exactly, at every scale forever. Two properties are the whole
concept:

1. **`g > 0` and `g < 0` are two interpenetrating labyrinths that are congruent
   mirror images of each other.** There is no outside. Every "room" has a
   congruent twin across the membrane. One surface, two churches, and the
   membrane is the only thing between them.
2. **`|g| = t` is a single unbroken surface with no edge, no seam, no end.** It
   cannot be built, welded, or 3D-printed. It can only be *evaluated*. Which is
   exactly what OntoMath is for.

Zach's prompt, taken at its strongest: *unlike anything this Earth has ever seen
before.* Nothing on this Earth is a minimal surface. Soap films are; the
architecture of a cathedral is not.

## 2. What is in it

| Being | What it is |
|---|---|
| **The Membrane** | `max( max( (\|g\|−t)/L , ‖p‖−30 ) , −shaft )` — the gyroid, bounded into a sphere, with a 6 m Oculus cut through its crown |
| **The Heart** | `max( ‖p‖−8 , −g/L )` — a ball woven *solid* out of the same lattice at 3 m cells, so you can see through it |
| **The Twelve Piers** | `max( cylinder , g/L )` — columns with the labyrinth cut clean through them at 1.6 m flute cells |
| **The Glass Floor** | `y − h(x,z)` — two crossed wave trains; the one shape here the engine *proves* is a heightfield |
| **The Oculus Rings** | two authored elliptical tori, a great gold ellipse over the shaft and a squashed one dropped through it |
| **The Vigil** | 48 beads on the great ellipse |
| **Light** | 6 radiant beings: the Zone's own breathing light, the Heart's gold source, indigo labyrinth fog, a white-gold shaft beam, a teal counter-light halo at 16 m, and a Perlin-warmed nimbus outside |
| **Palette** | opal membrane (three phase-shifted wave trains at 120°), gold Heart, basalt piers, glass floor, brass rings |

The colour of the membrane is read off *the same gyroid the geometry is cut
from*, at half the wave number, so the iridescence can never drift off the
lattice the way a hand-authored texture would. The Heart is gold, so the Iris
Halo is teal — without a complement on the far side of it, a warm source in a
dark field is only an orange blob.

## 3. The Refusals, held

- **No new C++ class.** `SdfPrim::Expr` + an OntoMath `MathNode` already
  expresses this exactly. **Zero engine code was changed to admit this Zone.**
- **No new enum value.** No `ShapeKind`, no `SdfPrim`, no `MathNode::Op`. Every
  primitive — sphere, box, cylinder, elliptical torus — is written out as
  OntoMath arithmetic using `Length`, `Abs`, `Sqrt`, `Intersection` (= max) and
  `Difference`, because a max over the axes *is* a box. These are
  Persons'-readable mathematics, not parameter slots.
- **No black box.** Every value is a `reliquary.*` property, registered and
  Law-readable. Where the engine is narrower than the effect sounds, the
  property says so and says why.
- **No new methods.** The light breathes because an authored `t` term says so.

## 4. The Radical Transparency — four things the Zone found out about itself

This is the part worth keeping. **I wrote four confident claims into the Zone
that turned out to be false, and the verification caught every one.** Each is
now recorded on the Zone itself rather than quietly fixed.

### 4.1 The gyroid is a solid that fills space, so a Reliquary built on it has no exterior

The first membrane was `(|g| − t)/L` alone. It is PERIODIC and it is a *solid*,
so that expression is **negative a long way from anywhere** — 1031 of 3945 rays
launched from 46 m away reported themselves embedded in the wall. You cannot
stand outside a thing made of an infinite minimal surface, because outside is
also inside.

The fix is one authored term, `‖p‖ − 30`, intersected in. That is what turns the
effect into a *place*: a hollow sphere of woven stone you can approach and look
into. Recorded on the Zone as `reliquary.mathIsUnbounded` and
`reliquary.boundingNote`.

### 4.2 A surface intersected with a solid is a curve, not a shell

The Heart was first `max(ball, g/L)` — a ball carved by the gyroid *surface*.
That reads like a woven shell and is not one. The lattice term's positive range
is ~0.1 m next to the ball's 8 m, so the expression collapsed to **a solid ball
with a tenth of a metre of fluting on its skin**.

The Heart has to be a gyroid *solid*: `max(ball, −g/L)`. The test now measures
the **solid fraction of the Heart's volume** (50.3%) and counts rays that enter
its material and come back out (596/596). A solid ball would pass "does it have
surface"; only the volume fraction catches this.

### 4.3 The pier CSG was mixing metres with |g| units

`max` is only a well-mixed CSG when both arguments are distances in the same
unit. The cylinder was in metres and the raw gyroid in |g|, so without the
divisor the g term swamped a 1.9 m radius and the piers would have come out as
loose cylinders of raw labyrinth with no column left in them.

### 4.4 Two of my own checks were passing vacuously

Worth recording, because a test that cannot fail is worse than no test:

- The **conservativeness** check declared a hit on `d < 0.001` *before* testing
  for a sign change, so a ray whose first sample was already negative — i.e. one
  that started inside a wall — was recorded as a clean hit and never reached the
  tunnel test. It reported "0 tunnels" and proved nothing. Origins now sit on a
  shell outside the vessel, and the tunnel test runs first.
- Both marches **started from random points inside the 60 m box**. The gyroid
  fills all of space, so a large fraction of those origins were embedded in the
  membrane and the "landing" was the first sample, taken from inside the wall.
  It reported 491 landings on neither surface; all 491 were real membrane,
  approached from the wrong side.

And one honest limit of the engine, recorded rather than wished away: **the
analytic-gradient emitter refuses any `ScalarLeaf` carrying factors or trans**
(`isDifferentiableAst`, SdfWgsl.cpp:712-721), and every wave in this Zone is a
trans. So the Glass Floor takes central-difference normals. The surface itself
is still exact — only the normal is differenced instead of derived. That is what
`reliquary.analyticGradient: false` and `reliquary.analyticGradientRefusal` on
the Floor say.

### 4.5 The Zone was mathematically perfect and could not be drawn

Zach's first run produced an endless log:

```
[WebGPU] SdfWgsl compile refused: source[0] radiance: a field expression names
the variable 'p', which has no binding in this shader expression context
```

Every Piecewise in the Zone declared `"input": "p"`. `emitPiecewise` resolves the
**piece-bound variable** through `pointComponent` (SdfWgsl.cpp:664), and
`pointComponent` binds `x, y, z, t, n, omega.*, wi.*, wo.*` — and nothing else.
Not `p`. So the entire radiance program was refused, every frame, per source.

**The trap is that the two uses of `p` are one character apart.** Inside a
piece's `mathNode`, `p` is *legal* — `emitMathNode` special-cases a `ValueLeaf`
of `p` (SdfWgsl.cpp:513) before it ever reaches `pointComponent`. So `length(p)`
compiles and `"input": "p"` does not. Every other Zone in the tree says `"x"`.
I invented `"p"`, and **no CPU-side evaluator cares**: `MathNode::evaluate` binds
`p` happily, which is exactly why all 72 original checks were green while the
Zone could not draw a single frame.

**The lesson, and the guard.** "The mathematics evaluates" and "the machine can be
told about the mathematics" are two different claims, and a Zone can satisfy the
first and fail the second forever without a single visible symptom beyond a log
line. The engine's refusal is the right behaviour — it refuses rather than
inventing a zero, which would silently re-interpret f(t) as f(0) — but a refusal
nobody compiles is a refusal nobody reads.

So the test now runs the **production WGSL emitter on the CPU** and reads its
verdict: `inspectScalarExpression`, `inspectVectorExpression`,
`inspectAngularExpression`, `inspectDensityExpression`,
`inspectExtinctionExpression`, `inspectScatteringExpression`, and
`inspectOccluderLayout` over all 6 radiant beings, all 17 authored shapes and all
5 materials. 47 new checks, 119 total, **no GPU and no display session
required**.

And the guard was **proven non-vacuous**: with `"input": "p"` re-injected into
the save file, every affected field went red on the spot. A test that has never
been seen to fail is not a test.

## 5. The verification

`tests/zones/gyroid_reliquary_test.cpp` — **119 checks, all passing, ~25 s.** A
Zone made of authored mathematics can fail in ways a Zone made of primitives
cannot: the AST can load and still evaluate to nothing, the distance can be
non-conservative and let the marcher tunnel, a field can be valid JSON while
describing empty space, and — the one that actually shipped — every expression
can be *true* and still be uncompilable. None of that is visible by reading the
file, so the test loads the Zone through the real `ZoneManager`, **evaluates** it,
and then **compiles** it.

The load-bearing results:

| Claim | Measurement |
|---|---|
| The divisor is a **true** lower bound on the distance | **0 tunnels in 3945 rays**, worst overshoot 0.000 m |
| The Reliquary is visible, not theoretical | **3902/3945 rays** find the Membrane; **1950/2000** land with the renderer's own 192-step budget |
| Every landing is on a surface the Zone *authors* | 1412 gyroid + 526 vessel rim + 12 shaft + **0 neither** |
| The Reliquary has an exterior | **0 rays begin inside a wall** |
| The Heart is woven, not solid | **50.3%** solid fraction; 596/596 rays pass through |
| No Object silently loses its palette | all 65 materials resolve to a real `Material` being |
| Every medium has a real boundary | all 6 radiant densities reach **exactly 0** far away |
| Density sovereignty holds | the fog does not also emit; the gold belongs to the source alone |
| **The machine can be told about all of it** | **47/47 expressions compile to WGSL** — 6 radiant beings × radiance/chroma/angular/medium, 17 authored shapes, 5 materials |

Independently, the measured Lipschitz constant of the full membrane SDF is
**1.000** — which is why the marcher provably cannot tunnel.

One number a Person should know: **from just outside the vessel (32 m), 95% of
rays hit within 192 steps. From 46 m it is 6.5%, and 2000 steps does not move
it** — because a hollow sphere of thin lattice *should* let a ray aimed at its
centre thread the corridors. On WebGPU the proof-based range hierarchy closes
that gap by jumping the proved-empty exterior in one authorized move; the CPU
march in the test has no such acceleration, so it under-samples from far away by
construction.

## 6. Files

| Path | What |
|---|---|
| `saves/zones/The Gyroid Reliquary/zone.json` | the Zone — 65 beings, 533 KB, owned by Zach, deletable by Zach |
| `scripts/generate_gyroid_reliquary.py` | the authoring script, fully commented; first seed only |
| `tests/zones/gyroid_reliquary_test.cpp` | 72 checks |
| `CMakeLists.txt` | +2: `gyroid_reliquary_test` needs the source root as its ctest CWD, and links `SdfWgsl.cpp` so the compile guards can run headless |

**Provenance.** First seed of a new Zone, so generation was permitted
(FIRST_MOVER_AUTHORING.md §7 rule 8). Every later change must **patch** that file
in place, never regenerate it. The script keeps the stage → verify →
atomic-rename discipline for that reason.

## 7. Open, and deliberately not decided alone

- **Frame cost has not been measured.** The conservative divisor is what makes
  the Zone provably hole-free, and it costs steps. 192 steps is enough from
  32 m; whether the Reliquary holds a high frame rate inside is a question only
  a Person at the keyboard can answer. `cmake --build build --target lag` and the
  Person Verification List entry both bear on it.
- **Growing the Reliquary** is one number (`‖p‖ − 30`) plus its `fieldExtent`
  follow. Untested at larger radii.
- **`prism_cathedral_test` was already red and my one-line CMake fix revealed
  why**: it was missing from the working-directory allow-list, so it had been
  failing at *hydration* and never reaching its checks. With the fix it runs 91
  checks and fails 2 — both about Prism Cathedral's own authored radiance values
  at Z=40 and Z=107. **I did not touch them.** Prism Cathedral's save file is
  not mine to edit.
- **3 pre-existing reds** (`slow_adapter_zone_perf_test`,
  `webgpu_perlin_exact_gradient_test`, `zone_home_ontology_test`) — proven
  pre-existing by re-running them with this Zone moved out of `saves/zones/`
  entirely; all four failed identically without it.
