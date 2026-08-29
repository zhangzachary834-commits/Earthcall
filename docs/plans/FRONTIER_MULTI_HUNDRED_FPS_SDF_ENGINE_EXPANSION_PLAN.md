# Implementation Plan — Frontier Multi-Hundred FPS SDF Engine & Hardware-Accelerated Micro-Architecture

**Date**: 2026-08-28 23:25 PDT  
**Author**: Antigravity (Gemini 3.7 Flash), session `fb329b04-fd0d-42e8-b6c8-8f30c3e28deb`  
**Document**: `docs/plans/FRONTIER_MULTI_HUNDRED_FPS_SDF_ENGINE_EXPANSION_PLAN.md`  
**Scope**: `src/Singularity/Screen/WebGPU/`, `src/Singularity/Screen/ScreenChannel.*`, `src/ConstructedBeing/Singular/Object/Geometry/`

---

## 1. Executive Summary & Context

Earthcall represents geometry and manifolds as pure Person-authored OntoMath ASTs (`geom::SdfNode`, `geom::FieldNode`). The WebGPU renderer is the sensory projection substrate that evaluates these fields on hardware.

Having successfully doubled framerates from ~21 FPS to 40+ FPS via algorithmic raymarching improvements (2D lattice noise, coarse-to-fine multi-tier stepping, secant root refinement, analytic AABB CPU picking, and bounding plane early-outs), this plan outlines the roadmap to transition Earthcall into the **200–500+ FPS tier** by implementing the deep GPU micro-architecture techniques used in frontier commercial SDF engines (*Claybook*, *Media Molecule's Dreams*, and *Unreal Engine 5 Lumen*).

---

## 2. Architectural Pillars & Phasing

```mermaid
graph TD
    subgraph "Phase A: TMU Offloading"
        A1[Procedural ALU Hashes] -->|Pre-baked Volume Texture| A2[1-Cycle Hardware Trilinear Sampling]
    end

    subgraph "Phase B: Decoupled Resolution"
        B1[Native Retina 4K: 5.1M Rays] -->|Half-Res Compute Dispatch| B2[1.3M Rays + Bilateral Depth Upsampling]
    end

    subgraph "Phase C: Quadtree DDA Traversal"
        C1[Point Sphere Probing] -->|Min/Max Mipmap Quadtree| C2[Macro-Tile Segment Skipping]
    end

    subgraph "Phase D: Warp Compaction"
        D1[SIMD Warp Divergence] -->|subgroupBallot & Re-packing| D2[100% Dense Active Warps]
    end

    subgraph "Phase E: Cluster Bounding & Hi-Z"
        E1[Full AST Evaluation] -->|Hierarchical Cluster OBBs| E2[Subtree Culling & Hi-Z Pre-pass]
    end
```

---

### Phase A: Hardware Texture Mapping Unit (TMU) Offloading ($3\times\text{--}5\times$ ALU Reduction)

#### Problem
Procedural Perlin noise evaluates ~150 ALU instructions per step (`floor`, `fract`, `mod289`, `permute4`, quintic polynomials, 7 linear interpolations). This saturates the GPU's Arithmetic Logic Units (ALUs) while dedicated fixed-function Texture Mapping Units (TMUs) sit idle at 0% utilization.

#### Technical Solution
1. **Pre-baked Tileable 3D Noise Volume**:
   - Allocate a single persistent $64 \times 64 \times 64$ `RGBA8Unorm` 3D texture containing four pre-computed gradient noise octaves across the R, G, B, and A channels.
   - Upload once at renderer initialization (`WebGpuRenderer::init`).
2. **Hardware Trilinear Filtering**:
   - In [`SdfWgsl.cpp`](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/src/Singularity/Screen/WebGPU/SdfWgsl.cpp), replace arithmetic `cnoise3(p)` with:
     ```wgsl
     fn textureNoise(p: vec3<f32>) -> f32 {
         let sample = textureSampleLevel(noiseTex, noiseSampler, p * 0.05, 0.0);
         return (sample.r - 0.5) * 2.0;
     }
     ```
   - Filtering is performed in dedicated silicon in **1 cycle**, freeing 90% of the ALU pipeline for lighting, physics, and complex AST evaluation.

---

### Phase B: Decoupled Half-Resolution Compute & Depth-Aware Bilateral Upsampling ($4\times$ Ray Reduction)

#### Problem
Retina viewports on modern displays render over 5.1 million pixels ($2880 \times 1800$). Tracing 5.1M rays per frame saturates memory bandwidth and fragment fillrate.

#### Technical Solution
1. **Half-Resolution Offscreen Attachment**:
   - When `@screen-channel.renderScale` is set to $0.5$ (or dynamically on Retina viewports), allocate a half-resolution color/depth target ($1440 \times 900 = 1.3\text{M pixels}$).
2. **Edge-Aware Bilateral Upsample Pass**:
   - A lightweight full-screen compute pass reconstructs the final image to native 4K using $3 \times 3$ depth/normal bilateral weights:
     $$w_i = \exp\left(-\frac{\|\Delta p_i\|^2}{2\sigma_s^2}\right) \cdot \exp\left(-\frac{|\Delta z_i|^2}{2\sigma_z^2}\right) \cdot \max(0, n_i \cdot n_{\text{center}})^4$$
   - Prevents edge bleeding and preserves razor-sharp manifold contours while evaluating $75\%$ fewer raymarch steps.

---

### Phase C: Hierarchical Min/Max Height Quadtree DDA Traversal

#### Problem
Sphere-tracing heightfields across flat ground uses point-based radius checks, requiring multiple micro-steps even with adaptive scaling.

#### Technical Solution
1. **Hierarchical 2D Min/Max Quadtree**:
   - Generate a 2D mipmap where each pixel $(u, v)$ stores $(h_{\text{min}}, h_{\text{max}})$ for the corresponding spatial block.
2. **Amanatides & Woo Grid DDA**:
   - The ray steps along its 2D footprint across the quadtree. At each quad cell:
     - If $\text{ray.minY} > \text{cell.maxY}$, the ray skips the entire $64\text{m} \times 64\text{m}$ tile in **1 single step**.
   - Traversal speed becomes independent of terrain surface roughness.

---

### Phase D: Subgroup / Wavefront Compaction via `subgroupBallot()`

#### Problem
SIMD warp divergence: when 1 thread in a 32-thread warp takes 30 steps while 31 threads finish in 2 steps, all 32 threads must remain active for 30 cycles, wasting 95% of execution resources.

#### Technical Solution
1. **Compute Shader Dispatch ($16 \times 16$ Workgroups)**:
   - Transition implicit field rendering from fragment rasterization to tile-binned compute dispatches.
2. **Subgroup Compaction**:
   - Every 4 iterations, compute active ray masks using `subgroupBallot(active)`.
   - Compact and re-index active rays into dense contiguous warps using `subgroupShuffle()` or shared memory scratchpads.
   - Fully resolved warps terminate immediately, releasing execution units back to the GPU scheduler.

---

### Phase E: Hierarchical Cluster Bounding for Composite Smooth Unions

#### Problem
An authored creature or machine consisting of 50 primitives joined by `smin()` evaluates all 50 AST nodes for every point probe across the world.

#### Technical Solution
1. **Spatial Cluster Partitioning**:
   - The AST compiler groups primitives into local spatial clusters (bounding spheres/OBBs).
2. **Subtree Culling**:
   - In WGSL, if $\|p - \text{cluster.center}\| > \text{cluster.radius} + k$, the entire 40-node subtree is evaluated as its coarse bounding sphere, bypassing evaluation of inactive branches.

---

### Phase F: Early-Z & Hi-Z Occlusion Pre-Pass

#### Problem
Terrain rays execute across all pixels inside the terrain bounding box, even when occluded by foreground objects (characters, walls, buildings).

#### Technical Solution
1. **Depth Pre-Pass**:
   - Render foreground opaque meshes to the depth buffer before drawing implicit fields.
2. **Hardware Hi-Z Rejection**:
   - The GPU's fixed-function Hierarchical-Z (Hi-Z) hardware rejects occluded terrain bounding box fragments before the fragment shader ever launches.

---

## 3. Property Paths & Law System Governance (Refusal #6 — No Black Box)

All new rendering controls and performance switches are registered as first-class properties on `ScreenChannel` (`@screen-channel.*`), ensuring total governance by Person-authored Laws:

| Property Path | Type | Access | Description |
|---|---|---|---|
| `@screen-channel.renderScale` | `double` | Read / Write | Resolution scaling factor ($0.25$ to $2.0$, default $1.0$) |
| `@screen-channel.performanceMode` | `bool` | Read / Write | Toggles half-res bilateral compute & aggressive culling |
| `@screen-channel.tmuNoiseEnabled` | `bool` | Read / Write | Toggles hardware volume texture sampling vs procedural arithmetic |
| `@screen-channel.warpCompactionEnabled` | `bool` | Read / Write | Toggles subgroup compaction in compute pipelines |
| `@screen-channel.trianglesDrawn` | `int` | Read-Only | Telemetry: total geometric triangles drawn |
| `@screen-channel.drawCalls` | `int` | Read-Only | Telemetry: total GPU draw passes executed |
| `@screen-channel.vramAllocatedBytes` | `double` | Read-Only | Telemetry: active VRAM allocations in bytes |

---

## 4. Verification & Testing Matrix

### Automated Regression Tests
1. **`webgpu_sdf_parity_test`**:
   - Compares GPU raymarched rendering against exact CPU analytical ground truth across all 20 standard manifold shapes. Requires zero silhouette error.
2. **`webgpu_micro_mastery_lag_test`**:
   - Renders 4,500 dynamic objects and 1,500 implicit fields to ensure zero VRAM fragmentation and verify frame time stays below the 108.7 ms baseline.
3. **`frame_lag_test`**:
   - Engine-wide multi-subsystem probe validating that no timing regressions or broken invariant contracts are introduced.
4. **Full Test Suite (`ctest`)**:
   - All 73 registered test suites must pass (72 pass, 1 known pre-existing failure in Bugs.md #11).

### Interactive Real-World Verification
- Launch `./build/earthcall` on `NoiseFloorWorld` (`saves/zones/NoiseFloorWorld/zone.json`).
- Open `PerformanceMetricsWindow` and verify framerate targets:
  - **Tier 1 (Current)**: 40+ FPS
  - **Tier 2 (TMU + Half-Res Bilateral)**: **160–240+ FPS**
  - **Tier 3 (Compute Compaction & Cluster Culling)**: **300–500+ FPS**

---

**Signed:**  
Antigravity (Gemini 3.7 Flash)  
Session: `fb329b04-fd0d-42e8-b6c8-8f30c3e28deb`  
Date: 2026-08-28T23:25:00-07:00
