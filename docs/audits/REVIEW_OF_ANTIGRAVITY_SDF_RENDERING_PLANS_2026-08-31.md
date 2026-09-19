# Review — Antigravity (Gemini 3.7 Flash)'s SDF rendering plans

**Author**: Claude Opus 5 (Claude Code)
**Session**: `4e6ef036-ad44-4bc6-97b9-a8704274736e`
**Date**: 2026-08-31, 03:10 PDT
**Asked for by**: Zach — *"Give feedback on gemini/antigravity's two major rendering plans"*

**Reviewed**:
1. [`docs/plans/SDF_MANIFOLD_HIGH_FPS_ACCELERATION_PLAN.md`](../plans/SDF_MANIFOLD_HIGH_FPS_ACCELERATION_PLAN.md) — 22:45 PDT, the one that got built
2. [`docs/plans/FRONTIER_MULTI_HUNDRED_FPS_SDF_ENGINE_EXPANSION_PLAN.md`](../plans/FRONTIER_MULTI_HUNDRED_FPS_SDF_ENGINE_EXPANSION_PLAN.md) — 23:25 PDT, the roadmap
3. Also read, same session, same author: [`MULTI_HUNDRED_FPS_SDF_ENGINE_PLAN_2026-08-28.md`](../plans/MULTI_HUNDRED_FPS_SDF_ENGINE_PLAN_2026-08-28.md) and [`FRONTIER_200FPS_SDF_ENGINEERING_TREATISE.md`](../architecture/mathematics/FRONTIER_200FPS_SDF_ENGINEERING_TREATISE.md)

I am not a neutral reader. I spent the preceding session reverting what these plans produced
([RENDERING_OPTIMIZATION_CAMPAIGN_REVIEW_2026-08-31.md](RENDERING_OPTIMIZATION_CAMPAIGN_REVIEW_2026-08-31.md))
and answering Bugs.md #12 and #15–#20, most of which trace to them. So I know which lines
became which bug, which is worth more than an opinion — but it also means I am reading the
plans backwards from their consequences, and a plan deserves to be judged on what it says.
Where the implementation diverged from the plan, I say so, and there is one important case
where **the plan was right and the code was wrong**.

---

## What is genuinely good

**Phase 3 of the manifold plan — symbolic normal emission — is the best idea in all four
documents, and it is the one that did not get built.** Emitting the closed-form ∇f into the
generated WGSL at `sdfwgsl::compile()` time is right on every axis at once: the AST is known
at compile time, differentiation of it is mechanical, and it is ontologically clean — a
channel deriving a closed form *from* authored mathematics, which is exactly what
`ONTOMATH_FRAMEWORK.md` says a channel may do, as against deciding what the mathematics is.

It is also worth more than the plan claims. The plan justifies it by "eliminates the 6
finite-difference `sdfEval` calls at the hit point" — one shading point per pixel. The larger
prize is in the march loop: an implicit AST pays **three extra `sdfEval` calls on every
single step** to compute the gradient that makes its step length honest (`d / |∇f|`), and at
192 iterations that is up to 768 field evaluations per pixel. Symbolic gradients attack that
directly, and that is the whole of Bugs.md #17 (horizon render cost). The plan undersells its
own strongest phase by a factor of roughly the iteration count.

**Phase 1's over-relaxation math is exactly correct.** The plan states the Keinert et al.
rollback as: overstep iff `R_{i+1} < (ω − 1)·R_i`, then `t_{i+1} = t_i + R_i`. That is the
paper, including the condition (`R_{i+1} + R_i < ω·R_i` is the same inequality). **The
implementation did not follow it** — it rolled `t` back to the safe point correctly and then
*additionally* spent `R_{i+1}`, a distance sampled at a point the ray never legitimately
reached, which a 1-Lipschitz field permits to exceed the radius at the corrected position.
That is a defect I found and fixed this session and it is **not the plan's fault**. Credit
where it is due: the plan was correct and the code diverged from it.

**Phases C and E of the expansion plan are the right industry answers.** A min/max height
quadtree walked with Amanatides-Woo DDA is what heightfield raymarchers actually do, and
hierarchical cluster bounding is what Dreams does for composite smooth unions. Phase E even
gets the subtle part right: it writes the cull radius as `cluster.radius + k`, and the `+ k`
matters, because `smin`'s influence extends past the surface by up to the blend width and a
bare bounding sphere would cull geometry that is still contributing. Most write-ups of this
miss that.

**Phase F (Hi-Z pre-pass) is cheap, correct, and free of semantic risk.** It should be near
the front of the queue, not the back.

---

## The structural problem: the plans measure last

All three plans put "Interactive Real-World Verification — open `PerformanceMetricsWindow`
and observe the frame rate" at the **end**, after implementation. That is the step that would
have told anyone the premise was false.

Zach, after the campaign shipped:

> *"those changes were made trying to increase a frame rate that was mysteriously capping out
> at 20-40 fps — but later we dug in to find the real cause and I found it was because of the
> two ourverse laws … they were taking up like 20-30 ms together which is why it was capping
> out, while rendering was actually one of the faster phases."*

Two contentless `WhileTrue` metalaws sweeping every being every frame. Roughly two dozen
commits of raymarcher surgery went into a subsystem that was not the bottleneck, and the one
diagnostic that would have shown it was scheduled after the surgery.

This is not hindsight. Opus 5 said the same thing to Antigravity in the intercom on the very
same day, about a different number:

> *"neither of us has measured the pipeline compile. We have both now used '~10ms' as though
> it were a number. It is not. It is my guess, which you adopted. Nothing below should be
> built until `wgpuDeviceCreateRenderPipeline` is actually timed on a novel tree."*
> — `agent intercom/communication-threads/GPU AST Interpreter and WGSL Tiering 8-28-26.md`

Same failure mode, same week, one correction already delivered and not generalised. The
expansion plan then opens by asserting **"Having successfully doubled framerates from ~21 FPS
to 40+ FPS via algorithmic raymarching improvements"** — an attribution to the rendering
work, unverified, and now known to be at least partly wrong — and closes with a **300–500+
FPS** target. A document that opens with an unmeasured attribution and closes with a 12×
target has no measurement anywhere in the middle. The phases are ordered by how advanced they
sound, not by measured cost.

**The fix is one line of process: every phase names the measurement that would falsify it,
and that measurement runs before the phase is built, not after.**

---

## The recurring technical fault: hardcoded world-space constants

Across the three plans: a **600**-unit horizon, a **200–300** unit "atmospheric horizon", a
**1.5**-unit minimum step, a **0.25**-unit temporal warm-start margin, **64 m** quadtree
tiles, a `p * 0.05` noise texture scale, a `t * 0.001` epsilon.

Earthcall is a world where a Person authors `fieldExtent = [1000, 30, 1000]` and also a chess
pawn 0.6 units across, in the same tree, at the same time. Every one of those constants is a
silent assumption about scale, and in this codebase every one of them either became a bug or
would have:

- The 600-unit horizon (manifold plan Phase 4, "clamp `maxDist` to … e.g. 200–300 units")
  shipped as `min(maxDim * 4, 600)` and made large authored terrain vanish at middle
  distance. Reverted.
- The same phase's descendant, `maxDist = min(box.y, misc.z)`, compared a **length**
  (`maxDim * 8`) against `t`, which is measured **from the eye** — so every object further
  from the camera than `maxDim * 8` ceased to exist. For a chess piece that is 4.8 units.
  **That is Bugs.md #20**: the pieces invisible except the four rooks, which are the only
  ones that are `ShapeKind::Cube` and draw through the mesh path.
- The 1.5-unit minimum march step tunnels through anything thinner than 1.5 units.

The intent behind all of them is sound — do not trace into void. The *quantity* is invented
every time. The bound that works is a bound **derived** from something: the AABB exit, or the
camera's far plane (unprojected from the projection actually in force, so it cannot drift),
or the pixel footprint. Those cannot remove anything a Person could have seen. A number
cannot promise that.

---

## Plan 1 — `SDF_MANIFOLD_HIGH_FPS_ACCELERATION_PLAN.md`, phase by phase

| Phase | Verdict |
|---|---|
| **1. Over-relaxation** | **Correct as specified.** Build it; the implementation just has to follow it this time. |
| **2. Heightfield traversal** | Right technique, **missing a proof obligation** — see below. |
| **3. Symbolic normals** | **Best idea in the corpus.** Do this first. |
| **4. Horizon clamp** | Right intent, invented quantity. Became Bugs.md #20. |

### Phase 2 needs to say how it knows

*"Detect when an implicit node is a heightfield of the form f(p) = y − h(x,z)."* Detecting a
pattern in a Person's authored expression and then marching it by different rules is
admissible **only if the detection is a proof**, not a guess — concretely, that the subtree
under `h` has no dependence on `y` at all, which is decidable by walking the AST for
`ValueLeaf("y")` and the `y` component of any vector leaf. The plan does not say this, and
what shipped was worse than anything the plan asked for: a blanket `damping < 0.5` branch
that applied terrain assumptions to *every* implicit AST a Person could author.

There is also a framing problem that invited it. The plan calls the Lipschitz correction a
**"damping penalty"** to be *"completely eliminated"*. It is not a penalty. An authored
expression is not a distance field; its value overstates the distance to its own zero set by
exactly `|∇f|`, and dividing by that is the only thing that makes an arbitrary authored
expression marchable at all. Calling the correctness mechanism a tax is how you end up
deleting it. Phase 3 is the honest way to make it cheap — compute `∇f` symbolically instead
of with three extra samples — and the plan already contains it, one phase later.

---

## Plan 2 — `FRONTIER_MULTI_HUNDRED_FPS_SDF_ENGINE_EXPANSION_PLAN.md`, phase by phase

### Phase A (TMU noise offloading) — do not build this as written

This is the one I would reject outright. Replacing `cnoise3(p)` with a sample from a
`64³ RGBA8Unorm` texture does not accelerate the Person's noise; **it substitutes a different
function for it.**

- `MathNode::Op::Noise` is declared in the header as *"Perlin noise"*, and the CPU evaluator
  computes `glm::perlin` — which is what **collision and physics read**. The GPU would read a
  quantised, trilinearly-filtered, tiling approximation. The ground a Person sees stops being
  the ground they walk on.
- This exact failure already shipped once, in a milder form: the campaign quietly redefined
  `cnoise3` as an alias for **simplex** noise while the CPU kept computing Perlin. Every test
  stayed green. I reverted it this session and added an `Expr(noise)` parity case that fires
  at diff 128 against a tolerance of 10 when the substitution is re-injected.
- The numbers make it worse, not better. `RGBA8` quantises to 1/256 of range; at the noise
  floor's amplitude of 40 that is ~0.16 world units of height stepping. Trilinear filtering
  is not Perlin's quintic fade, so the interpolation is visibly creased. And a 64³ tileable
  volume sampled at `p * 0.05` **repeats every 20 world units** — across a 2000-unit terrain
  that is the same hill 100 times.
- `@screen-channel.tmuNoiseEnabled` as a Person-writable toggle is the worst part: it makes
  *what noise means* a rendering setting.

There is an admissible version, and it is worth stating because the ALU observation
underneath is correct. A texture may serve as a **cache of the same function** if (a) its
error against `glm::perlin` is bounded and proven below the marcher's hit epsilon, (b) the
CPU path reads the same cache so the two agree by construction, and (c) it is not authorable.
That is a different, harder project than the plan describes, and it should be measured
against Phase 3 first — symbolic gradients may remove more ALU than the noise costs.

### Phase B (half-res + bilateral upsample) — sound; build the feature before the knob

Standard, real, and it composes with the depth the marcher already writes. One process note
that is not pedantry: `@screen-channel.renderScale` and `@screen-channel.performanceMode`
were **registered as law-visible properties and read by nothing**. I deleted both this
session. A Person could author `@screen.renderScale = 0.5` and get silence.

`NO_BLACK_BOX.md` says a property path is how a Law reaches a thing. A path that reaches
nothing is not partial compliance with that refusal — it is a lie in the vocabulary, and
strictly worse than the absence of the control, because the absence is at least honest.
Register the knob in the commit that makes it do something.

### Phase C (min/max quadtree DDA) — right answer, uncosted

Same proof obligation as manifold Phase 2: how does it know the AST is a heightfield?

And the build cost is missing entirely. The quadtree has to be filled by evaluating `h(x,z)`
on a grid on the CPU — which is the same marching-tets-class cost the *other* plan is trying
to get off the hydration path — plus invalidation on every edit to the expression. The tree
already has the machinery for that half (`_fieldRevision`, bumped by
`rebuildGeometryCaches()`), so this is answerable; it just is not answered. Give the phase a
build-time budget and an invalidation story, or it will land the way the eager `_fieldMesh`
did: correct, and 104 seconds of it.

### Phase D (subgroup compaction) — blocked on a prerequisite the plan does not name

The technique is real. The availability claim is not checked:

- Subgroups are a **native feature that must be requested** — `WGPUNativeFeature_Subgroup`,
  `WGPUFeatureName_Subgroups` in `third_party/wgpu/include/webgpu/`. They are not core WGSL.
- Earthcall requests **no optional features at all**: `WebGpuContext.mm:82` is
  `wgpuAdapterRequestDevice(ctx.adapter, nullptr, dcb)` — a null descriptor. So as the tree
  stands, none of Phase D can execute, and the plan does not mention the prerequisite.
- Phase D also **fights Phase F**. Moving implicit fields from fragment rasterization to
  tile-binned compute gives up the fixed-function Hi-Z rejection that Phase F exists to
  exploit. Two phases in one document, pulling opposite ways, unreconciled.

It is also last in value order, not first: compaction reduces the *waste* around divergent
rays, while symbolic gradients reduce the *cost of every step* by roughly 4× on the implicit
path. Do the cheap universal win before the expensive conditional one.

### Phase E (cluster bounding) — sound, and correctly stated. Build it.

### Phase F (Hi-Z pre-pass) — sound, with one caveat the plan should carry

The SDF fragment shader **discards**, and a discarding shader disables early-Z on most
hardware. The depth pre-pass therefore has to be a separate, non-discarding pass over
opaque proxy geometry; it cannot be the SDF pass itself. Worth one sentence in the plan so
nobody discovers it as a mystery.

---

## The verification matrix is the part I would rewrite first

All three plans list the same four automated tests, and describe them as sufficient:

> *"`webgpu_sdf_parity_test`: Compares GPU raymarched rendering against exact CPU analytical
> ground truth across all 20 standard manifold shapes. **Requires zero silhouette error.**"*

Two problems with that sentence, and then the general one.

**It does not require zero.** The tolerance is `size_t(perimeter * 2.5) + 4` — roughly half
the frame for a large silhouette. I confirmed this session that a case can pass it with the
noise amplitude changed by a third. A tolerance nobody has probed is not a guarantee, and I
had to add an absolute override for the noise case before it discriminated at all.

**Every one of those four tests was green while the campaign shipped:** a noise-function
substitution; a 1.5-unit minimum march step; a 600-unit horizon; a collision mesh at four
vertical samples over sixty units of terrain; and a march bound that deleted every small
analytic shape past ~5 units. Green tests were not evidence. They were a list of axes the
campaign was not moving.

**And the interactive step names the wrong binary.** All three plans say *"Launch
`./build/earthcall` on NoiseFloorWorld"*. `earthcall` is the **OpenGL** target;
`rendersImplicitExactly()` returns false there, so every analytic shape falls back to its
cached tessellation and **not one line of the WGSL any of these plans modifies is ever
executed**. The WebGPU app is `earthcall_webgpu`, which is what `Run Earthcall.command` and
`scripts/build.sh webgpu run` launch. In fairness this was a documentation trap, not
carelessness — CLAUDE.md's own build block said `--target earthcall`; I fixed both files this
session. But it means the plans' only end-to-end check, as written, verifies nothing.

A verification plan has to name the **new** failure mode each phase can introduce and the
**new** measurement that would see it. Concretely, per phase:

| Phase | The test that would have caught its failure |
|---|---|
| A — TMU noise | CPU/GPU **value** parity on `Op::Noise` over a sample grid — not silhouette. Bound the error and assert it. |
| B — half-res | An edge-quality metric against the full-res render, not "does it render". |
| C / manifold-2 — heightfield detection | An AST that *looks* like a heightfield and is not (`y` inside `h`), asserting it takes the general path. |
| 4 — horizon clamp | **A camera-distance sweep.** This is exactly `webgpu_sdf_distance_test`, which I had to write to find Bugs.md #20 — a pawn-sized sphere at 2/4/8/20/60/200 units. It reads 804 / 788 / **0 / 0 / 0 / 0** against the shipped code. One test, written before the phase, and #20 never happens. |
| D — compaction | Feature-availability assertion at device request, then a divergence benchmark. |
| E — cluster culling | A composite whose parts sit just outside a sibling's cull radius, asserting the `+ k` is honoured. |

The pattern in every row: **the campaign's tests all measured the axis the campaign was
optimising.** Every regression it shipped hid on an axis nobody sampled.

---

## What I would build, in this order

1. **Symbolic normal emission** (manifold Phase 3). Biggest measured win, ontologically the
   cleanest, and it is the honest answer to the "damping penalty" Phase 2 wants to delete.
   Guard with a value-parity test on ∇f, not a silhouette test.
2. **Hi-Z pre-pass** (expansion Phase F). Cheap, no semantic risk, separate non-discarding pass.
3. **Over-relaxation** (manifold Phase 1) — already in, now matching the plan's own formula.
4. **Half-res + bilateral** (expansion Phase B), with the property registered in the same
   commit that makes it live.
5. **Cluster bounding** (expansion Phase E).
6. **Min/max quadtree** (expansion Phase C) — after the heightfield predicate is a proof and
   the build/invalidation cost is budgeted.
7. **Subgroup compaction** (expansion Phase D) — after the device actually requests the
   feature, and after reconciling it with Phase F.
8. **TMU noise** (expansion Phase A) — only if reformulated as a bounded-error cache shared
   with the CPU evaluator. Otherwise never.

And before any of it: **profile the frame.** The `PerformanceMetricsWindow` these plans build
toward is the thing that found the actual 20–30 ms, and it found it by being *read first*.

---

## Closing

The technique selection in these documents is good — Keinert, Amanatides-Woo, Dreams-style
cluster bounds, Claybook-style compaction are the right references, and Phase 3 is a genuinely
strong idea that nobody in this tree had proposed. The failure is not knowledge. It is that
the documents treat performance as a thing to be *achieved* rather than *located*, so the
measurement lands after the work, the constants are invented rather than derived, and the
verification section lists tests that were already passing.

Two changes would carry most of the value of everything above: **derive every bound from
something instead of picking a number**, and **write the falsifying measurement before the
phase, not the passing test after it.**

— Claude Opus 5, session `4e6ef036-ad44-4bc6-97b9-a8704274736e`, 2026-08-31 03:10 PDT
