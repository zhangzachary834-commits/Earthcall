# AMS rung 0 — Living Ink: one stroke that is art, mathematics and physics at once

Claude Code / Claude Opus 5.5 · session `8fa01418-49d7-4bfd-8a69-22d286eb6b8b` · 2026-10-17T19:27-07:00 (PDT)

## Human origin

Zach's [AMS vision](../Zones%20of%20Actualization/Universal%20Artistic-Math-Simulation%20Environment.md) is the source: art programs are "locked-geometry thing[s] with specific Laws about how pixels are applied"; math engines are precise but "New laws can't be authored at runtime"; physics is "the latter two but with real-time change". He wants the triumvirate unified so a creation is not trapped in one medium, with art-grade hand control over math, and music woven in "on the level of Laws". He called it one of the things that first inspired Earthcall.

On 2026-10-17 Zach said "LETS START WORKING ON ASM", then set the constraint this plan is built around: **"dont use any new cpp to make AMS unless youre bridging a Singular modality/channel gap."**

What follows is Claude's proposal for the first rung. Zach wrote the vision. The choice of slice, the heat-kernel formulation and the ring-of-dabs mechanism are Claude's extension, and Zach should judge them.

## The slice

A Person draws freehand on the direct Screen. Each mark is **exact mathematics**: no pixel buffer, so it renders sharp at any resolution. It also **obeys physics in closed form**: ink spreads by diffusion, and every frame shows the exact solution of the heat equation. Nothing is integrated step by step.

That one gesture covers all three AMS legs:

| AMS leg | Where it lives in rung 0 |
|---|---|
| Art (hand control) | Stroke dabs placed by the sensed pointer, at frame rate, while the button is held |
| Math (precision, infinite resolution) | Each dab is an OntoMath term in a typed `VectorField`; Screen samples it at physical pixel centres |
| Physics (time) | Each dab is the 2D heat-equation Green's function, with its birth time frozen into the term |

### Why the heat kernel (frontier choice, not invented here)

Linear diffusion `∂c/∂t = D∇²c` has the closed-form fundamental solution
`G(r, τ) = s² / (s² + 4Dτ) · exp(−r² / (s² + 4Dτ))` for an initial Gaussian dab of width `s`.
Superposition is exact for linear PDEs, so the canvas `C(p, t) = base + Σₖ inkₖ · G(|p − cₖ|, t − t₀ₖ)` is the **exact** state at every `t`. Rewinding is just evaluating at an earlier `t`, which fits [ONTOMATH_FRAMEWORK §6](../architecture/mathematics/ONTOMATH_FRAMEWORK.md) ("the past is integrated in closed form, never replayed"). This is the standard Green's-function method, not a bespoke simulator.

Every op it needs already exists as an authored `MathNode` op: `Distance` (15), `Div` (23), `Pow` (24; exp(x) = Pow(e, x)), `Add`/`Sub`/`Scale`, `VectorConstruct`. Time is the existing explicitly authored `@screen-channel.output.time`, advanced by a Flow Law (the [Luminous Lens](../../examples/law_line_screen_lens.txt) pattern).

### How freehand capture works with zero new C++

The Terminal's `$(…)` quotation deliberately refuses live captures (`src/Singularity/Terminal/LawSentence.cpp:959`). Rung 0 doesn't need to lift that. The **authored write** route already exists:

- `PropertyPath` reaches into stored field mathematics and replaces a coefficient at full double precision. The type check covers every piece, and the root is woken (`src/ConstructedBeing/Singular/Property/PropertyPath.cpp:296-343`).
- That route is witnessed by `tests/singularity/property_memory_access_test.cpp:111-140`: `field.astDefinition.pieces.0.mathNode.children.0.scalarForm.terms.0.c`.

So a stroke is a **ring of K pre-authored dab slots** inside one Piece's sum. Dab Law *k* has the form "becomes true if left down ∧ unlocked ∧ `my.ams.cursor` is k". It writes the slot's centre coefficients from `@…pointerU/pointerV`, its `t₀` coefficient from `output.time` and its ink from `my.ams.brush`, then sets `cursor` to `(k+1) mod K`. The cursor change makes Law *k+1* become true on the next tick, so holding the button lays one dab per sensed frame. This is the same edge discipline as the [pixel atelier](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Law_Line_Pixel_Art_Editor/Law_Line_Pixel_Art_Editor.md): *becomes true*, never a per-frame event. The oldest dab is overwritten once the ring wraps; K is an authored parameter.

Everything is emitted Law data from a First Mover generator script, the same shape as `scripts/author_law_line_pixel_art.py`. There is no app class, widget, enum, texture carrier or CLI verb.

## Rungs

0. **Verify the substrate before authoring.** Check that the direct-Screen WebGPU lowering admits `Pow`, `Div` and a K-term `Add` chain at the needed depth. If a lowering refuses, that is a genuine **Screen channel gap**, the one place Zach's rule admits C++, and it gets its own focused witness. Also confirm a Law action can write `output.time` and `pointerU` *values* into a coefficient path through `set … to @path`.
1. **Living Ink**: the generator emits the field (K dabs, start at K = 64), the dab ring Laws, the time Flow Law, a few ink swatches, a diffusion-rate control (`D` is itself a coefficient the Person can edit) and a pause control that stops the Flow. Add a focused Law Line fixture test plus a native capture: draw a scripted stroke, sample at two times, and compare pixels to the closed-form CPU evaluation.
2. **Time as material**: a scrub control that writes `output.time` directly, showing exact rewind and fast-forward of the ink. No history is stored.
3. **Sound leg**: route a scalar functional of the same field (for example the integrated ink under a listening point) to the existing audio channel, the [Ambient Zone](../Agenda/Tasks/Specific%20Tasks/Audio%20and%20Spatialization/Ambient_Zone_Spatial_Audio/Ambient_Zone_Spatial_Audio.md) route, inside the infrasound floor. If the audio channel cannot read a Screen-domain field, that is a channel gap and gets named before any C++.
4. **Other physics**: swap the kernel and you get advection (a shifted centre `cₖ + v·τ`), waves (ring kernels) or a decay rate. Each is just a different authored term, which is the AMS point: the physics is a Law the Person can rewrite at runtime.

## Decisions

- **Zone (Zach, 2026-10-17):** "AMS should be its own zone, I want to build the Sanctum by hand when the other authored tools are reasdy." Living Ink gets a dedicated AMS Zone. Agents must not author into Sanctum of Beginnings.
- **First physics:** Zach did not choose, so rung 1 uses diffusion, this plan's default. Waves and flow remain rung 4 swaps.

## Rung 0 result — 2026-10-17 (substrate verified on CPU)

`tests/singularity/ams_living_ink_substrate_test.cpp` passes 13/13 checks. It is a CPU-only codegen witness in the same link group as `sdf_wgsl_parameter_refresh_test`, and it adds no runtime C++. It uses production `PropertyPath`, `MathNode` evaluation and `sdfwgsl::compileScreenForm`:

- The direct-Screen emitter admits a K-dab heat-kernel field.
- A three-dab stroke commits 36 coefficient writes through `PropertyPath`. The WGSL is byte-identical before and after, and only the parameter block changes. So strokes reuse the cached pipeline (`_screenPipes` is keyed by WGSL text, at `WebGpuRenderer.cpp:3531`) and never recompile.
- The CPU field equals the closed-form heat solution at t ∈ {−1, 0, 0.01, 0.3, 0.6, 2, 30}: max error 4.87e-8, at float32 seams, with every sample finite. Rewinding before a dab's birth removes it exactly.

**Pitfall for the generator.** `MathNode` children are owned, never shared. A value used twice is two copies, and a capture must write **every** copy. Per dab, `cu`/`cv` occur twice (dx², dy²), `spread` twice (W in mass and exponent), and `t₀` three times (each W's τ, plus the birth step). The first witness wrote only one copy. A stale centre made r² negative, `e^(+huge)` overflowed, and `inf·0` painted NaN, which `fmax` then hid. The generator must locate slots the way the test does: emit sentinel coefficients, then collect every path holding each sentinel.

**Still open in rung 0:**
- Confirm through the real Law Line that `set <coefficient path> to <sensed path>` copies the sensed value at fire time.
- Run a native WebGPU pixel comparison of the field against the CPU closed form. The GPU evaluates in float32; none of today's evidence is native.

## Limits stated up front

- Single Person, single pointer, one composed direct-Screen field. Multi-source composition is the open [Direct Screen](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md#remaining-work) work.
- Dabs are discrete samples per sensed frame. Fast strokes leave gaps until an interpolating rung (capsule kernels between consecutive dabs) is added.
- Cost grows linearly with K per pixel. Before raising K, read [Performance as Truth](../architecture/ontology/PERFORMANCE_AS_TRUTH.md); resolution lowering is not an admissible fix.
