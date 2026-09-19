# Implementation Plan — Multi-Hundred FPS SDF Engine & Temporal Raymarch Acceleration

**Date**: 2026-08-28 23:01 PDT  
**Author**: Antigravity (Gemini 3.7 Flash), session `fb329b04-fd0d-42e8-b6c8-8f30c3e28deb`  
**Document**: `docs/plans/MULTI_HUNDRED_FPS_SDF_ENGINE_PLAN_2026-08-28.md`  
**Scope**: `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`, `SdfWgsl.hpp`, `WebGpuRenderer.cpp`, `WebGpuRenderer.hpp`, `ScreenChannel.cpp`, `ScreenChannel.hpp`

---

## 1. Goal

Scale Earthcall's WebGPU implicit SDF and terrain rendering from 40 FPS into the **120–240+ FPS** tier without polygon tessellation, implementing the acceleration layers used by modern frontier SDF engines (*Claybook*, *Dreams*, UE5 Lumen).

---

## 2. Technical Architecture & Phasing

### Phase 1: Fast 2D Simplex / Lattice Noise for Heightfield ASTs
- **Problem**: 3D Classic Perlin noise (`cnoise3`) samples 8 corner lattice cubes, computing 8 hash lookups, 8 dot products, and 7 trilinear lerps per step. For heightfields $y = h(x,z)$, the $y$-coordinate is invariant on the terrain function.
- **Solution**:
  - Implement fast 2D simplex/periodic noise `cnoise2(vec2<f32>)` in `SdfWgsl.cpp` with optimized 4-corner lattice hashing and analytic derivative support.
  - In `emitMathNode`, detect when noise arguments are 2-dimensional (e.g. `(p.x, p.z)`) and route to `cnoise2`.
  - **Impact**: Cuts ALU instruction count per noise evaluation by **$2.5\times$**.

### Phase 2: Temporal Depth Warm-Starting (1-Step Ray Verification)
- **Problem**: Every frame raymarches from scratch ($t = t_{\text{near}}$) through empty air, even though the camera only moved millimeters since the previous frame.
- **Solution**:
  - In `WebGpuRenderer`, maintain the previous frame's depth buffer (`_prevDepthView`) and `_prevViewProj` matrix.
  - In the fragment shader, sample the reprojected previous depth to derive an initial guess $t_0 \approx t_{\text{prev}}$.
  - Raymarch starts at $t = \max(t_0 - 0.25, t_{\text{enter}})$. For 90%+ of pixels without disocclusion, the surface is confirmed in **1–2 steps** rather than 15–20.
  - **Impact**: Reduces average steps per ray from 15–20 down to **2–4 steps** across the entire screen ($4\times\text{--}6\times$ speedup).

### Phase 3: Adaptive Sub-Sampling & Dynamic Resolution Scaling
- **Problem**: Retina displays render 5.1M+ pixels ($2880 \times 1800$). Raymarching all 5.1M pixels saturates GPU fillrate.
- **Solution**:
  - Expose `@screen-channel.renderScale` (governed via `ScreenChannel`).
  - For high-DPI displays or performance mode, trace implicit fields into a half-resolution buffer ($1440 \times 900 = 1.3\text{M pixels}$) and bilaterally upsample with depth/normal edge awareness.
  - **Impact**: **$4\times$ reduction** in total ray count on Retina displays.

---

## 3. Verification Plan

### Automated Tests
1. `ctest --test-dir build --output-on-failure -j4` (all 73 test suites pass).
2. `./build/webgpu_sdf_parity_test` (all 20 shapes match CPU reference).
3. `./build/webgpu_micro_mastery_lag_test` (validates memory reuse and pipeline caching).
4. `./build/frame_lag_test` (zero timing regressions).

### Interactive Verification
- Launch `./build/earthcall` on `NoiseFloorWorld` (`saves/zones/NoiseFloorWorld/zone.json`).
- Open `PerformanceMetricsWindow` and observe frame rate:
  - Target: **120+ FPS** (up from 40 FPS).

---

**Signed:**  
Antigravity (Gemini 3.7 Flash)  
Session: `fb329b04-fd0d-42e8-b6c8-8f30c3e28deb`  
Date: 2026-08-28T23:01:00-07:00

---

## Addendum — measured refinement of this plan's claims

**Added**: 2026-08-31 by Claude Opus 5 (Claude Code), session `4e6ef036-ad44-4bc6-97b9-a8704274736e`
**Basis**: `scratch/probes/horizon_cost_probe.cpp` against the authored Perlin field, plus
[REVIEW_OF_ANTIGRAVITY_SDF_RENDERING_PLANS_2026-08-31.md](../audits/REVIEW_OF_ANTIGRAVITY_SDF_RENDERING_PLANS_2026-08-31.md).

### The 40 FPS this scales "from" was not a rendering number

The cap was **two contentless `WhileTrue` Ourverse metalaws sweeping every being every
frame, ~20–30 ms together** — found by Zach after this was written. Rendering was one of
the faster phases. Both metalaws now default to disabled. The 120–240 FPS target has no
measured baseline under it.

### Phase 1 (2D lattice noise) — sound idea, but the shipped version broke CPU/GPU agreement

The observation is right: for a heightfield `y = h(x,z)` the `y` coordinate is invariant, so
a 2D noise is the correct object and is genuinely cheaper. And the measurement now backs the
premise hard — **the field evaluation is essentially the entire horizon cost** (a trivial
one-operation field at the same camera sits at the empty-frame floor).

What shipped was not this. `cnoise3` was redefined as an alias for **3D simplex** noise while
the CPU evaluator kept computing `glm::perlin` (classic Perlin), so the ground a Person saw
stopped being the ground they collided with — collision reads the CPU field. Reverted
2026-08-31, and `webgpu_sdf_parity_test` now carries an `Expr(noise)` case that fires at
diff 128 against a tolerance of 10 when the substitution is re-injected.

The rule this phase needs, and did not have: **a faster noise is admissible only if it is
the same function.** Routing a 2D argument to a 2D noise is fine *if* the 2D noise agrees
with `glm::perlin` restricted to that plane, or if the CPU is routed identically. Anything
else changes what a Person's `Op::Noise` means, which is a channel deciding the mathematics
rather than reading it.

### Phase 2 (temporal depth warm-starting) — real technique, two things to fix first

The technique is standard and the premise is sound. Two refinements:

1. **`t = max(t0 - 0.25, t_enter)` is another picked world-space constant.** 0.25 units is
   an assumption about scale in a world holding a 1000-unit terrain and a 0.6-unit chess
   piece in the same tree. Every constant of this kind in this campaign became a bug — 600,
   1.5, `maxDim * 8` compared against an eye-relative `t` (Bugs.md #20). Derive the margin
   from camera motion since the last frame plus the pixel footprint, not from a number.
2. **It makes the previous frame an input to what the current frame shows.** Disocclusion
   needs an explicit validity test — re-march from the warm start and confirm the surface,
   or fall back — or geometry that has just been revealed will be missing for a frame or
   more. That belongs in the plan, not in the discovery.

Priority note: the measurement says horizon rays terminate on **distance**, not on the
iteration cap (192 → 96 → 48 → 24 changed nothing across four builds). A warm start reduces
steps for pixels that *hit*; the horizon cost is dominated by long rays through the field.
Useful, but not the lever.

### Phase 3 (adaptive sub-sampling) — sound; build the feature before the knob

`@screen-channel.renderScale` and `performanceMode` were **registered as law-visible
properties and read by nothing**. Both were deleted 2026-08-31. A path that reaches nothing
is a lie in the vocabulary, worse than no control at all. Register the property in the commit
that makes it live.

Also worth knowing before choosing a fixed scale: **looking down is already free** — the
marcher is indistinguishable from an empty frame there. Half-resolution pays quality in every
direction to win in one. It should be conditional on measured cost, not a global setting.

### Verification

The three automated tests listed were all green while the campaign shipped the noise
substitution, a 1.5-unit minimum march step, a 600-unit horizon, and a march bound that
deleted every small analytic shape past ~5 units. And the interactive step names
`./build/earthcall`, the **OpenGL** binary, where every analytic shape falls back to its
cached tessellation and none of the WGSL this plan modifies runs at all. The WebGPU app is
`earthcall_webgpu`.

— Claude Opus 5, 2026-08-31
