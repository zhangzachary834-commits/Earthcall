# Audit — High-FPS Complex Round Manifold SDF Raymarching & Field Acceleration — 2026-08-28

**Document:** `docs/audits/SDF_MANIFOLD_HIGH_FPS_RAYMARCHING_AUDIT_2026-08-28.md`  
**Reporter / Requester:** Zach (session 2026-08-28 22:37 PDT)  
**Author:** Antigravity (Gemini 3.7 Flash), session `fb329b04-fd0d-42e8-b6c8-8f30c3e28deb`  
**Scope:** `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`, `WebGpuRenderer.cpp`, `saves/zones/NoiseFloorWorld/zone.json`, OntoMath AST evaluation, GPU raymarching pipeline, and comparison to frontier real-time SDF engines (*Claybook*, *Dreams*, UE5 Lumen Mesh SDFs).

---

## 1. Executive Summary

Zach asked:
> *"I heard it's entirely possible to have complex round manifold SDFs on an engine with hundreds of fps, even without falling back to tessmessh. Right now earthcall only renders the perlin zone's green hills ground with like 20-30 fps. What are we missing?"*

Earthcall is currently achieving **20–30 FPS** on the Perlin green hills zone (`saves/zones/NoiseFloorWorld/zone.json`) because the WebGPU renderer executes **naive, brute-force screen-space sphere tracing** across the entire camera frustum without hierarchical empty-space skipping or algorithmic raymarching acceleration.

Engines that achieve **hundreds of FPS** on complex manifold SDFs and implicit landscapes without polygonal tessellation do not brute-force analytic formulas per fragment. They combine **hierarchical spatial acceleration (coarse distance brickmaps/clipmaps)**, **over-relaxation (Enhanced Sphere Tracing)**, **symbolic gradient compilation (0-tap normals)**, **tight proxy hull bounds**, and **tile-binned compute marching**.

This audit details the exact bottlenecks in Earthcall today and the exact architectural additions required to reach hundreds of FPS on pure implicit manifolds while strictly upholding the [Six Refusals](../../GEMINI.md).

---

## 2. Root Cause Analysis of Current 20–30 FPS Performance

### A. Full-Frustum Bounding Cube Rasterization & Massive Overdraw
In `WebGpuRenderer.cpp:945`, field rendering scales a unit cube by `fieldExtent * 1.05f`. For `NoiseFloorWorld`, `fieldExtent` is `[1000.0, 30.0, 1000.0]`.
When the player is standing in the world, this bounding box encloses the camera or spans almost 100% of the viewport. Consequently, the fragment shader is invoked for **millions of pixels** every frame (especially on high-DPI Retina displays).

### B. Per-Fragment 192-Iteration Fixed Loop
In `SdfWgsl.cpp:698`:
```wgsl
for (var i = 0; i < 192; i = i + 1) {
    let p = ro + rd * t;
    let raw = sdfEval(p);
    ...
    let density = fieldEval(p);
    ...
}
```
Every fragment inside the bounding box can iterate up to 192 times. Rays looking into the horizon or traveling across empty air step forward incrementally without any coarse jumping.

### C. GPU SIMD / Warp Divergence Saturation
GPU hardware schedules execution in 32-thread (NVIDIA / Apple Silicon) or 64-thread (AMD) warps/wavefronts that execute lockstep SIMD instructions.
- If 31 rays in an $8 \times 4$ pixel tile hit the ground in 8 steps, but **1 single ray** grazes a hill crest or shoots into empty horizon air for 192 steps, **all 32 threads in the warp are forced to execute all 192 loop iterations**.
- This completely destroys GPU ALU throughput and memory latency hiding.

### D. Extreme Arithmetic Density per Step (3D Classic Perlin Noise)
Each of the 192 iterations evaluates `sdfEval(p)`, which calls `cnoise3(P)` (`SdfWgsl.cpp:101-167`).
`cnoise3` evaluates:
- 8 corner hash coordinate modulos (`mod289`)
- Permutation lookups (`permute4`)
- Taylor inverse square roots (`taylorInvSqrt`)
- Quintic polynomial fade curves: `Pf0 * Pf0 * Pf0 * (Pf0 * (Pf0 * 6.0 - 15.0) + 10.0)`
- 7 trilinear lerp (`mix`) operations.

At 192 steps $\times$ 2 to 8 million fragments, the GPU is forced to crunch **hundreds of millions to billions of 3D noise evaluations every single frame**.

### E. Mathematical Misalignment: Implicit Heightfields vs. Euclidean SDFs
The Perlin ground plane is authored as:
$$f(x, y, z) = y - 40 \cdot \text{noise}(0.008 \cdot (p + \text{offset}))$$
This is an **implicit isosurface heightfield**, **not** a true Euclidean distance function.
- A true Euclidean SDF satisfies the Lipschitz condition $|\nabla f| = 1$ everywhere; $f(p)$ is the exact radius of an uncollided sphere.
- For an implicit heightfield, $|\nabla f| = \sqrt{1 + |\nabla h|^2} \ge 1$. The vertical distance $|y - h(x,z)|$ can severely underestimate or overestimate 3D shortest distance depending on ray angle and surface slope.
- To prevent visual tunneling on steep slopes, `WebGpuRenderer.cpp:949` injects `damping = 0.25` (`inst.misc.w`), forcing `d < current_eps * 8.0` and reducing step sizes. This multiplies the required iteration count by $4\times$.

### F. 6-Tap Finite Differences for Normals on Hit
When a ray hits the surface, `sdfNormal(pf)` (`SdfWgsl.cpp:640-650`) evaluates:
```wgsl
let g = vec3<f32>(
    sdfEval(p + vec3<f32>(e, 0.0, 0.0)) - sdfEval(p - vec3<f32>(e, 0.0, 0.0)),
    sdfEval(p + vec3<f32>(0.0, e, 0.0)) - sdfEval(p - vec3<f32>(0.0, e, 0.0)),
    sdfEval(p + vec3<f32>(0.0, 0.0, e)) - sdfEval(p - vec3<f32>(0.0, 0.0, e)));
```
This performs **6 additional full evaluations of 3D Perlin noise** per hit fragment solely to obtain the lighting normal.

---

## 3. What Frontier Engines Do (The Missing Pieces)

| Technique | What Earthcall Does Today | What High-FPS SDF Engines Do | Speedup Multiplier |
|---|---|---|---|
| **Spatial Acceleration** | Blind screen-space marching through 1000 units of empty air | **Coarse Hierarchical Distance Field (Brickmap / 3D Clipmap)** skips empty air in 1–2 steps | **$5\times - 10\times$** |
| **Sphere Tracing Algorithm** | Naive fixed-step sphere tracing ($t \mathrel{+}= d$) | **Enhanced Sphere Tracing (Over-relaxation $\omega \in [1.2, 1.8]$)**; Segment Tracing | **$3\times - 5\times$** |
| **Bounding Proxy Hulls** | Monolithic $1000 \times 30 \times 1000$ bounding cube | **Tiled / Chunked Proxy Slabs** ($[t_{\text{enter}}, t_{\text{exit}}]$ spans $\approx 10\text{--}30$ units) | **$2\times - 4\times$** |
| **Surface Normals** | 6-tap finite-difference numerical gradient (6 extra noise evals) | **Analytic Symbolic Derivative / Dual Numbers** emitted directly from OntoMath AST (0 extra noise taps) | **$3\times - 7\times$ at surface** |
| **Terrain Intersection** | 3D sphere tracing over non-distance heightfield with $0.25$ damping | **2D Footprint DDA / Secant Root Refinement** (converges in 8–16 steps without damping) | **$4\times - 8\times$ on terrains** |
| **Work Distribution** | Full-resolution fragment shader per bounding-box pixel | **Screen-Tile Compute Binning & Half-Res March with Bilateral Depth Reconstruction** | **$2\times - 4\times$** |

---

## 4. Architectural Analysis & Ontological Integrity

### Does this violate Earthcall's Doctrine?

1. **Refusal #1 & #7 (No domain nouns in C++, data-driven behavior)**:
   - The authored representations remain pure `OntoMath::MathNode` ASTs and `geom::SdfNode` trees authored in save files.
   - The acceleration structures (distance clipmaps, proxy slabs, symbolic gradients) live entirely inside `Singularity/Screen/WebGPU/`, which is the sensory projection substrate.
   - As established in `GPU_MICRO_MASTERY_ARCHITECTURE.md`, modality channels govern *how the machine senses and projects* to hardware, not *what a being is*.
2. **Refusal #6 (No black boxes)**:
   - Renderer settings (e.g. `@screen-channel.sdfRaymarchQuality`, `@screen-channel.emptySpaceSkipping`) can be registered as governable property paths under `Singularity/Screen/`.

---

## 5. Verification Metrics Target

- **Current State**: `NoiseFloorWorld` renders at 20–30 FPS on high-DPI WebGPU.
- **Target State**:
  - `NoiseFloorWorld` Perlin terrain rendering at **120+ FPS** on standard displays, **60+ FPS** on 4K Retina.
  - Complex round manifold SDFs (e.g. 100 smooth-unioned toruses / gyroids) maintaining **144+ FPS**.
  - All 73 tests green with zero regression.

---

**Signed:**  
Antigravity (Gemini 3.7 Flash)  
Session: `fb329b04-fd0d-42e8-b6c8-8f30c3e28deb`  
Date: 2026-08-28T22:45:00-07:00
