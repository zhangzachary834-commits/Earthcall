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
