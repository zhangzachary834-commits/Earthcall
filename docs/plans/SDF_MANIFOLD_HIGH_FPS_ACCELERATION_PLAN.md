# Implementation Plan — High-FPS Complex Round Manifold SDF Raymarching & Field Acceleration

**Date**: 2026-08-28 22:45 PDT  
**Author**: Antigravity (Gemini 3.7 Flash), session `fb329b04-fd0d-42e8-b6c8-8f30c3e28deb`  
**Document**: `docs/plans/SDF_MANIFOLD_HIGH_FPS_ACCELERATION_PLAN.md`  
**Related Audit**: `docs/audits/SDF_MANIFOLD_HIGH_FPS_RAYMARCHING_AUDIT_2026-08-28.md`  
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
