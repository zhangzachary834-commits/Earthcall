# Far Lands Framework

**How OntoMath authors infinitely layered terrain degeneration as exact mathematics,
not as a precision bug.**

**Status:** Design specified, not yet implemented. Every mechanism named below
exists in the codebase today; no new C++ class, enum value, or directory is needed.
What remains is authored content: a `FunctionDef`, a `FieldNode`, a save file,
and the laws that bind them to a Person's position.

**Companion docs:** `ONTOMATH_FRAMEWORK.md` (the substrate this stands on),
`GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md` (the dual-path compilation),
`ontology/NO_BLACK_BOX.md` (why the GPU telemetry is governable),
`law/LAW_AND_CREATION_SYSTEM.md` (how conditions and actions author change).

**Origin:** Zach asked how OntoMath could make the Far Lands — and infinitely
many layers of them. The answer turned out to be that the substrate already holds
it: `Piecewise` spatial bounds define the layers, `FunctionRegistry` recursion
stacks them, and the dual-path compilation renders them. This document records
how the pieces compose, so anyone extending or seeding a far-land world knows
which authored structures carry the work.

---

## 1. What the Far Lands are, and what they are not

In Minecraft, the Far Lands were terrain at extreme coordinates where floating-point
precision death caused Perlin noise to sample incorrectly, producing walls, folds,
and eventual solid mass. It was a bug — an unintended consequence of IEEE 754
arithmetic at scale.

In Earthcall, the Far Lands are **authored mathematics**. Each layer of degeneration
is a distinct `Piecewise` piece with its own `MathNode` AST, and the number of layers
is unbounded because `FunctionCall` recursion stacks them. The terrain does not
*break* — it **transforms**, because each layer is a deliberate expression, not a
precision failure.

This is not a new subsystem. It is a worked example of three things the substrate
already does:

| Mechanism | Where | Role |
|---|---|---|
| `Piecewise` interval bounds | `ScalarForm.hpp` | cut spatial regions; each piece governs a band of distance |
| `FunctionCall` + `FunctionRegistry` | `ScalarForm.hpp` | recursive self-reference; each call is a deeper layer |
| `whereLEZero` pure guards | `Piecewise::Piece` | the base case; local math deciding when to stop descending |

No new C++ class for a domain noun (Refusal 1). No new enum value (Refusal 3).
The far-land *is* authored content — a `FunctionDef` and a `FieldNode` — sitting on
primitives every subsystem can already see (Refusal 6).

---

## 2. The recursive field: `farLayer`

The core is a single `FunctionDef` registered in the `FunctionRegistry`:

```
name:   "farLayer"
params: ["r", "depth", "freq", "amp"]
body:
  inputVariable: "depth"
  pieces:
    [0] base case — where (depth - 1) <= 0:
        value: sin(freq · x) · sin(freq · z) · amp
               (the terminal terrain, ordinary noise at whatever
                frequency and amplitude this recursion level reached)

    [1] recursive case — everywhere else:
        value: sin(freq · x) · sin(freq · z) · amp
               + farLayer(r, depth - 1, freq · k, amp · m)
```

Where:
- `r` is the distance from the Zone center (or Person position — the law decides).
- `depth` is how many layers deep we are. Each call decrements it.
- `freq` is the spatial frequency. Each recursive call multiplies it by a constant
  `k` (e.g. 3.7), so deeper layers fold at higher frequencies.
- `amp` is the amplitude. Each call scales it by `m` (e.g. 0.6), so deeper layers
  contribute less to the total — or by values > 1 if the author wants the far
  lands to grow more extreme, not less.

The `whereLEZero` guard on piece [0] is:

```json
{
  "op": "Sub",
  "children": [
    { "op": "ValueLeaf", "variableName": "depth" },
    { "op": "ScalarLeaf", "scalarForm": { "terms": [{ "coefficient": 1.0 }] } }
  ]
}
```

This is `depth - 1`. When `depth - 1 <= 0` — i.e., `depth <= 1` — the guard
fires and the base case governs. Otherwise the recursive piece applies.

The anti-Babel ceiling (`kMaxCallDepth = 32`) bounds the recursion. A `depth`
argument of 32 produces 32 distinct layers before the function returns `nullopt`.
That `nullopt` is honest divergence — the substrate says "this is as deep as I go"
rather than approximating.

### Why this is a fractal, not a repetition

Each recursive call passes `freq * k`. If `k = 3.7`, then:

| Layer | Frequency | What it looks like |
|---|---|---|
| 1 | f | gentle rolling terrain |
| 2 | 3.7f | ridges and folds |
| 3 | 13.69f | walls with fine corrugation |
| 4 | 50.65f | micro-walls on the walls |
| ... | ... | ... |
| 32 | f · 3.7³¹ | detail at a scale no viewport can resolve |

The total field is the **sum** of all these layers — like a Fourier series where
each harmonic is a deliberate authored expression, not a frequency bin. This is
self-similar terrain defined by exact symbolic math, not by an iterative noise
algorithm.

---

## 3. Spatial binding: `Piecewise` over distance

The `farLayer` function defines what terrain looks like at a given recursion depth.
The *outer* `Piecewise` on the `FieldNode`'s `astDefinition` decides how many layers
are active at a given distance from the Zone center:

```
inputVariable: "r"   (distance from origin)

Piece 0:  r in [0, 1000)
  → farLayer(r, depth=1, freq=1.0, amp=1.0)
    One layer. Normal terrain.

Piece 1:  r in [1000, 5000)
  → farLayer(r, depth=4, freq=1.0, amp=1.0)
    Four layers. The landscape folds.

Piece 2:  r in [5000, 20000)
  → farLayer(r, depth=12, freq=1.0, amp=1.0)
    Twelve layers. Walls form. Fine corrugation is visible.

Piece 3:  r in [20000, ∞)
  → farLayer(r, depth=32, freq=1.0, amp=1.0)
    Full depth. The terrain is a solid fractal mass.
```

The transition between bands is a `Piecewise` boundary — first applicable piece
wins. The author controls whether the transition is abrupt (disjoint intervals) or
blended (overlapping pieces with `whereLEZero` guards that interpolate).

This is the **infinitely many layers** Zach asked about. The layers are not
hardcoded bands with different parameters. They are recursion depth, and the
`FunctionDef` itself decides what each depth level contributes. Adding a 33rd layer
means raising `kMaxCallDepth` by one integer — no new classes, no new enum values,
no new files.

---

## 4. The dual path: how it renders

`ONTOMATH_FRAMEWORK.md` §3 describes the two evaluation modes. Both apply here:

### Path A — Procedural (law-governed parameters)

The quick version. A law modulates the `FieldNode`'s properties via `PropertyPath`
as the Person moves:

```
when:  @person.distanceFromOrigin > 1000
do:    flow @far-field.field.frequency += 0.01 * dt
```

The frequency increases continuously. The GPU shader reads the updated uniform
each frame — no recompile. This is the smooth, real-time version: the terrain
shifts gradually, the Far Lands creep in, and the law text is the complete record
of how they arrived. A Person can read the law and know what will happen at
distance 5000 without going there.

### Path B — AST compilation (the full recursive field)

The exact version. `SdfWgsl.cpp` traverses the `Piecewise` tree and transpiles
each piece into WGSL conditionals. The recursive `farLayer` calls become inlined
WGSL function calls (WGSL supports functions), each layer a distinct branch of
the shader. The GPU raymarches the exact authored field —
`WebGpuRenderer::rendersImplicitExactly()` returns `true`.

The tradeoff: changing the AST triggers a shader recompile. But the `SdfPipeline`
cache in `WebGpuRenderer` (`_sdfPipes`, keyed on the generated WGSL string) means
a stable field definition compiles once and is reused every frame. Only a law that
rewrites the `astDefinition` via `AstBridge::setValue` triggers a new compile.

---

## 5. Governance and the ScreenChannel

The CPU-GPU mastery work (`GpuBufferPool`, `GpuMeshCache`, `ScreenChannel`) makes
the far-land rendering governable by ontology:

- `@screen-channel.trianglesDrawn` — a law can read how many triangles the far
  lands are producing. If the count exceeds a threshold, a law can reduce
  `depth` or push the far-land threshold further out.
- `@screen-channel.vramAllocatedBytes` — the `GpuBufferPool`'s total VRAM
  footprint is visible. A Zone with aggressive far-land depth can watch its
  own memory cost.
- `@screen-channel.wireframe` — writable. A law can flip the far lands to
  wireframe rendering based on distance or GPU load, making the fractal
  structure visible as a debugging or artistic choice.

This is not a hardcoded LOD system. It is law text — authored, serializable,
introspectable — governing the rendering substrate through the same property
paths that govern everything else. The GPU adapts to what the GPU can handle,
and the adaptation is part of the world's law, not part of the engine.

---

## 6. The sound of the Far Lands

`ONTOMATH_FRAMEWORK.md` §7 establishes that the same `Piecewise` that defines a
density field can be handed to `renderForm` and become a waveform. The far-land
field is no exception:

The recursive `farLayer` function, evaluated with time as a variable instead of
spatial coordinates, produces a pressure wave. Each recursion layer is a harmonic.
The base case is a tone; each deeper layer adds a higher-frequency overtone scaled
by `amp * m^n`. The Far Lands have a **chord** — and it changes as the Person moves
through them, because the law that binds `depth` to distance binds it for both
channels.

The infrasound floor (§7a) applies: if a deep layer's frequency falls below 20 Hz,
the symbolic pass catches it (the `TransFactor` scale is `2π·freq`, the frequency
is `scale/2π`), and the channel refuses rather than filtering. The mathematics is
untouched — only the path to the Person's body is guarded.

---

## 7. Reversibility

`ONTOMATH_FRAMEWORK.md` §6 establishes that a `Flow` action's rate, when it has a
closed-form antiderivative, lets the world be read backwards without a log.

The Path A far lands (law-governed frequency drift) are reversible: `flow
@field.frequency += 0.01 * dt` integrates to `0.01 * t`, and
`ActionNode::valueSecondsAgo` can recover the frequency at any past moment from
the law text alone. The Person can ask "what did this terrain look like an hour
ago?" and the substrate answers exactly.

The Path B far lands (recursive AST) are a different question. The AST itself does
not change over time — it is a static definition, like a mathematical formula
written on paper. What changes is the *input* (`r`, the Person's distance), and
the Person's position history is the law system's to record or reverse, not
the field's.

The `integrable()` judgement is honest about the boundary: a recursive
`FunctionCall` piece is marked non-integrable (§6's table: "a piece whose value is
a function call"), because the antiderivative of a recursive function is not, in
general, expressible. This is correct. The far-land field is a *static* mapping from
space to density; it is the landscape, not the journey. The journey (the Person's
`Flow` through it) is what reversal acts on.

---

## 8. Per-Zone character

Each Zone can carry its own `FieldNode` with its own `astDefinition` calling its
own `FunctionDef`. Two Zones may define `farLayer` with different `k` and `m`
constants — one with `k = 3.7, m = 0.6` (fractal decay, the terrain folds gently
into density) and another with `k = 2.0, m = 1.5` (fractal growth, each layer
is louder than the last — the Far Lands *explode*).

The `FunctionRegistry` is global, so a shared `farLayer` definition works across
Zones. But a Zone that wants its own character defines a `farLayerMountain` or
`farLayerVoid` — different functions, different mathematics, same mechanism.

The Ourverse's gathering Zone (§0 of `OURVERSE.md`) is deliberately unowned.
Its far-land field, if seeded, belongs to no Person — it is the shared landscape
that no one governs alone, exactly as the Ourverse intends.

---

## 9. What needs to be built

Every mechanism named above exists today. The implementation is authored content
on top of the existing substrate:

| Task | Kind | Touches |
|---|---|---|
| Seed `farLayer` in the `FunctionRegistry` | Save file / First Mover | `saves/`, `FunctionRegistry::define` |
| Create a `FieldNode` whose `astDefinition` calls `farLayer` | Save file / First Mover | `saves/`, `FieldNode::fromJson` |
| A computed property or `Fold` for distance-from-origin on the Person | Small C++ | `Person/Person.hpp`, or a law that binds `r` |
| A law binding `r` to the Person's distance and evaluating the field | Law text | `saves/`, `Law::fromJson` |
| Verify WGSL compilation of recursive `FunctionCall` pieces | Test | `tests/singularity/` |
| Verify `renderForm` on a recursive `farLayer` waveform | Test | `tests/` |

The C++ item — a distance-from-origin property — is the only one that touches
source. It belongs on the Person because the Person is the one moving through the
world; it is a derived property (position relative to Zone origin), not a new
domain noun.

---

## 10. The general principle

The Far Lands are a worked example of a broader pattern: **Piecewise spatial
partitioning with recursive function calls produces infinitely layered,
self-similar, exactly reversible mathematical terrain from authored content alone.**

This pattern applies beyond terrain:

- **Sound fields** that grow more harmonically complex with distance.
- **Physics fields** (gravity, wind) that degenerate into chaos at the edges.
- **Visual fields** (fog, light scattering) with fractal structure.
- **Category boundaries** — the far edge of a `Piecewise` could carry a
  `ConditionNode` guard that changes what *kind* of terrain this is, not just
  its shape. The mathematics branches on ontology.

All of these are authored by Persons, serialized in save files, introspectable by
law, renderable by any channel, and reversible where the algebra holds.

---

*Authored by Claude (Opus 4.6), session `session_013brZDoSej3ttim5PnXWiDS`, 2026-08-28,
responding to Zach's question about using OntoMath to make the Far Lands and
infinitely many layers of Far Lands. The mechanisms described are Zach's existing
architecture — `Piecewise`, `FunctionRegistry`, `ScalarField`, `ScreenChannel`,
the dual-path compilation, the infrasound floor, exact integration. What this
document adds is the composition: how those pieces, which were built for separate
purposes, stack into one pattern that does what Zach asked about. The recursive-
function-as-fractal observation is new here but follows directly from the
`FunctionCall` specification (primitive recursion via argument mutation, §
`ScalarForm.hpp:157–162`), which already names escape-time fractals as an
expressible case.*