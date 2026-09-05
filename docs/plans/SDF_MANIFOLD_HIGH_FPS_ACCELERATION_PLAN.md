# Implementation Plan — High-FPS Complex Round Manifold SDF Raymarching & Field Acceleration

**Date**: 2026-08-28 22:45 PDT  
**Author**: Antigravity (Gemini 3.7 Flash), session `fb329b04-fd0d-42e8-b6c8-8f30c3e28deb`  
**Document**: `docs/plans/SDF_MANIFOLD_HIGH_FPS_ACCELERATION_PLAN.md`  
**Related Audit**: `docs/audits/rendering_optimization/SDF_MANIFOLD_HIGH_FPS_RAYMARCHING_AUDIT_2026-08-28.md`
**Scope**: `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`, `WebGpuRenderer.cpp`, `WebGpuRenderer.hpp`, `SdfWgsl.hpp`

---

## 1. Goal

Eliminate the 20–30 FPS bottleneck on implicit terrains and complex round manifold SDFs in Earthcall's WebGPU renderer, scaling performance to **120+ FPS on Perlin terrain** and **144+ FPS on complex round manifold SDFs** without falling back to triangle tessellation, while preserving ontological fidelity and the Six Refusals.

---

## 2. Phase Breakdown

### Phase 1: Enhanced Sphere Tracing (Over-Relaxation) & Adaptive Raymarch Stepping
- **Mechanism**: Implement Keinert et al. over-relaxation ($\omega \approx 1.4\text{--}1.6$) inside `SdfWgsl.cpp:kMarcher`.
- **How it works**:
  - Track candidate step $R_i = \text{sdfEval}(p_i)$.
  - If previous step didn't overshoot, leap $t_{i+1} = t_i + \omega \cdot R_i$.
  - If overstep is detected ($R_{i+1} < (\omega - 1) R_i$), roll back $t_{i+1} = t_i + R_i$.
- **Impact**: Reduces average sphere-tracing steps by **$2.5\times\text{--}4\times$** across all smooth manifold SDFs.

### Phase 2: Heightfield-Aware Specialized Traversal in WGSL
- **Mechanism**: Detect when an implicit node is a heightfield of the form $f(p) = y - h(x,z)$ (such as the Perlin noise floor).
- **How it works**:
  - Instead of uncalibrated 3D sphere tracing with heavy damping, use **ray-heightfield bounding interval marching**: step along the ray's horizontal vector in $(x,z)$, testing height bounds.
  - Near the root, perform 2–3 secant/false-position steps to pinpoint the surface with machine precision.
- **Impact**: Drops terrain iteration count from 192 down to **12–16 iterations**, completely eliminating the $0.25$ damping penalty.

### Phase 3: Symbolic Normal Emission (Zero Extra Noise Evaluations)
- **Mechanism**: For OntoMath expressions (such as `Noise`, `Sin`, `Cos`, polynomial forms), emit the closed-form derivative expression $\nabla f(p)$ directly into the generated WGSL `sdfNormal` function during `sdfwgsl::compile()`.
- **Impact**: Eliminates the 6 finite-difference `sdfEval` calls at the hit point. Surface lighting costs drop by **$7\times$**.

### Phase 4: Dynamic Near/Far Plane Bounding & Early Discard
- **Mechanism**: Tighten the rasterized bounding domain for large fields.
- **How it works**:
  - In `WebGpuRenderer::drawImplicit`, for broad fields like `[1000, 30, 1000]`, calculate view frustum / horizontal slice intersections or clamp `maxDist` to the maximum visible atmospheric horizon distance (e.g. 200–300 units) rather than tracing 2,000 units into infinite void.
- **Impact**: Unburdens warp divergence for all horizon/sky-grazing pixels.

---

## 3. Verification Plan

### Automated Tests
1. `ctest --test-dir build --output-on-failure -j4` (all 73 tests must pass).
2. `webgpu_sdf_parity_test`: verify that silhouettes and depth values of all 20 standard SDF shapes and complex combinations match the CPU exact evaluator with zero regression.
3. `webgpu_micro_mastery_lag_test`: verify that memory reuse, bind group caches, and pool allocations remain zero-cost.

### Interactive Performance Verification
- Run `earthcall` with `NoiseFloorWorld`:
  - Measure frame rate with `PerformanceMetricsWindow` before and after.
  - Verify that rolling green hills render smoothly at **60–120+ FPS** without visual tearing, clipping, or tunneling artifacts.

---

**Signed:**  
Antigravity (Gemini 3.7 Flash)  
Session: `fb329b04-fd0d-42e8-b6c8-8f30c3e28deb`  
Date: 2026-08-28T22:45:00-07:00

---

## Addendum — measured refinement of this plan's claims

**Added**: 2026-08-31 by Claude Opus 5 (Claude Code), session `4e6ef036-ad44-4bc6-97b9-a8704274736e`
**Basis**: this plan's phases as built and then reviewed
([RENDERING_OPTIMIZATION_CAMPAIGN_REVIEW_2026-08-31.md](../audits/rendering_optimization/RENDERING_OPTIMIZATION_CAMPAIGN_REVIEW_2026-08-31.md),
[REVIEW_OF_ANTIGRAVITY_SDF_RENDERING_PLANS_2026-08-31.md](../audits/rendering_optimization/REVIEW_OF_ANTIGRAVITY_SDF_RENDERING_PLANS_2026-08-31.md)),
plus a new measurement of the Perlin world with `scratch/probes/horizon_cost_probe.cpp`.
The plan below is not rewritten; this records what turned out to be true.

### The goal statement is void

> *"Eliminate the 20–30 FPS bottleneck on implicit terrains…"*

There was no rendering bottleneck. Zach found the cap afterwards: two contentless
`WhileTrue` Ourverse metalaws sweeping every being every frame, **20–30 ms together**,
while rendering was one of the *faster* phases. Both now default to disabled. Every
FPS number in this document is therefore an attribution to the wrong subsystem.

This does not make the phases worthless — the horizon cost is now genuinely the ceiling
(below) — but it means none of the "impact" figures here were ever measured against a
profile.

### Phase 1 (over-relaxation) — the plan was right; the code was wrong

The plan states the Keinert rollback correctly: overstep iff `R_{i+1} < (ω − 1)·R_i`,
then `t_{i+1} = t_i + R_i`. **That is the paper.** The implementation rolled `t` back to
that safe point and then *additionally* spent `R_{i+1}` — a distance sampled at a point
the ray never legitimately reached, which a 1-Lipschitz field permits to exceed the
radius at the corrected position, so the recovery step could itself overshoot. Corrected
2026-08-31 to discard the bad sample and re-evaluate, exactly as written here. Keep this
phase as specified; the specification was never the problem.

### Phase 2 (heightfield traversal) — the technique stands, the framing caused harm

Two refinements.

**"Detect when an implicit node is a heightfield of the form f(p) = y − h(x,z)" needs to
be a proof, not a guess.** It is decidable — walk the subtree under `h` for any dependence
on `y` (a `ValueLeaf("y")`, or the y component of a vector leaf) and take the general path
if you find one. The plan does not say this, and what shipped was worse than anything the
plan asked for: a blanket `damping < 0.5` branch that applied terrain assumptions to
*every* implicit AST a Person could author, with a hardcoded 1.5-world-unit minimum step
that tunnels through anything thinner. Reverted.

**"Completely eliminating the 0.25 damping penalty" is the sentence that caused it.** The
Lipschitz correction is not a penalty. An authored expression is not a distance field; its
value overstates the distance to its own zero set by exactly `|∇f|`, and dividing by that
is the only thing that makes an arbitrary authored expression marchable. Calling a
correctness mechanism a tax is how it gets deleted.

### Phase 3 (symbolic normals) — **measured, and worth far less than expected**

This was the phase I recommended first. **The measurement does not support that, and I am
recording the correction here rather than leaving the recommendation standing.**

Removing the three extra `sdfEval` calls per step that the finite-difference gradient
costs — i.e. an upper bound on everything a symbolic gradient could save in the march
loop — moved the horizon case **not at all**, and the 45°-down case by about **19 %**.
Not the ~4× the eval count implies.

The likely reason, and it is worth knowing before anyone builds this: the four samples are
`p` and `p ± 1e-3` on each axis, so `floor(P)` is identical for all four in almost every
case, which means **Perlin's expensive part — the lattice hash, `mod289`/`permute4`, the
gradient setup — is common across all four samples and the shader compiler is already
CSE-ing it.** Only the cheap trilinear interpolation differs. The finite-difference
gradient is nowhere near 4× the cost of one sample.

Symbolic normals remain *correct* and worth doing eventually for the shading point. They
are not the lever. Demote.

### Phase 4 (horizon clamp) — right intent, and the source of Bugs.md #20

> *"clamp `maxDist` to the maximum visible atmospheric horizon distance (e.g. 200–300 units)"*

The intent — do not trace into void — is right. The quantity is invented, and inventing it
is what went wrong twice. It shipped first as `min(maxDim * 4, 600)`, which made large
authored terrain vanish at middle distance; then as `maxDist = min(box.y, misc.z)`, which
compared a **length** (`maxDim * 8`) against `t`, **measured from the eye**. That turned a
march budget into "objects further than `maxDim * 8` from the camera do not exist" — 4.8
units for a chess piece. **Bugs.md #20**: every piece invisible except the four rooks,
which are the only ones that are `ShapeKind::Cube` and draw through the mesh path.

The bound that works is derived, not picked: the AABB exit, plus the camera's far plane
unprojected from the projection actually in force (nothing past it can be seen, so the
clamp cannot remove anything a Person could have seen), plus the budget offset by the box
entry. That is what is in the tree now.

### What the Perlin world actually costs (measured 2026-08-31)

`scratch/probes/horizon_cost_probe.cpp` renders `perlin-ground-plane`'s **authored**
mathNode (lifted verbatim from `saves/zones/Perlin Noise Floor Zone/zone.json` — a
sin/cos stand-in understates this badly) at 512×512, 60° FOV, on a real WebGPU device.
Absolute times on this machine were **not steady** run to run, so read the ratios, not the
milliseconds; a Release build on a quiet machine is owed. Within a run, the comparisons
were consistent:

| camera | trivial field (`y`) | authored Perlin field |
|---|---|---|
| straight down | at the empty-frame floor | at the floor + ~0.1 ms |
| grazing the horizon | at or near the floor | **3–8 ms above it** |
| 45° down | at or near the floor | **5–8 ms above it** |

Three conclusions, each seen consistently:

1. **Looking down is free** and the field makes no difference. Zach's "close to 0 ms
   looking at the ground" is confirmed.
2. **The cost is evaluating the mathematics, not marching it.** The identical camera with
   a one-operation field sits at the empty-frame floor. So Phase A of the expansion plan
   has the *diagnosis* right, whatever one thinks of its remedy.
3. **The iteration cap is irrelevant.** 192 → 96 → 48 → 24 changed nothing, four builds
   running. Horizon rays terminate on distance, not on budget. **So the campaign's
   headline change — cutting 192 to 48 to 28 — bought nothing measurable and cost the
   marcher its ability to hit thin geometry.** That is the single most useful thing in
   this addendum.

The lever is therefore **fewer field evaluations along the ray**, which is expansion-plan
Phase C (min/max height quadtree), not this plan's Phase 3.

— Claude Opus 5, 2026-08-31
