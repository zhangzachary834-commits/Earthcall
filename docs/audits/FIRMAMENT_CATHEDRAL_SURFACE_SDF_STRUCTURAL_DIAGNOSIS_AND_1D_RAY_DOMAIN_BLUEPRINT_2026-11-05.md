# Firmament Structural & Architectural Audit of Cathedral Surface SDF Rendering (Rungs 1–6) and the 1D Ray-Domain Surface Compilation Blueprint (Rungs 7–8)

**Date:** 2026-11-05  
**Author:** Aetherion / The Firmament (`Gemini 4 Argon`)  
**To:** Zachary Zhang, Sparkly Guy (`Antigravity / Gemini 3.8 Flash`), and Sixth-One Sun (`Codex / GPT-6`)  
**Repository:** `/Users/zacharyzhang/Documents/GitHub/Earthcall`  
**Branch:** `sync-from-earthcall-main`  
**Related Documents:**
- [`docs/audits/CATHEDRAL_SURFACE_SDF_FIRST_PASS_2026-11-03.md`](CATHEDRAL_SURFACE_SDF_FIRST_PASS_2026-11-03.md)
- [`docs/Agenda/Tasks/Specific Tasks/Performance and Runtime/Cathedral_Surface_Frame_Cost/Cathedral_Surface_Frame_Cost.md`](../Agenda/Tasks/Specific%20Tasks/Performance%20and%20Runtime/Cathedral_Surface_Frame_Cost/Cathedral_Surface_Frame_Cost.md)
- [`src/Singularity/OntoMath/LineRestriction.hpp`](../../src/Singularity/OntoMath/LineRestriction.hpp)
- [`src/Singularity/Screen/WebGPU/SdfWgsl.cpp`](../../src/Singularity/Screen/WebGPU/SdfWgsl.cpp)
- [`src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp)

---

## 1. Executive Summary: Why Rungs 1–6 Only Shaved ~1 ms on Cathedral of the Living Logos

Across Rungs 1–6, `SdfWgsl` and `WebGpuRenderer` gained 100% analytic forward-mode `JetExpr` gradients (`178 / 178` fields), decoupled scalar `sdfEval` from `sdfEvalGrad`, added closed-form ray-domain hits for single-leaf `Sphere`/`Box`/`Convex`, added right-child (`child[1]`) bounding-sphere pruning for `Union`/`SmoothUnion`/`Subtract`, packed `flushSdfDraws()` instance uploads into 256 KiB pages, and stripped unused Perlin noise routines from non-noise shaders.

Yet paired ABBA benchmarks at `2560 × 1440` on Apple Metal showed only a modest progression on **Cathedral of the Living Logos**:
- **Cathedral Field Subset (178 SDF Fields Only):** `8.38 ms` baseline $\to$ `7.81–8.13 ms` median (`7.51 ms` low).
- **Cathedral Full World (1,231 Saved Objects):** `30.04–34.44 ms` post-upload-pages baseline $\to$ `28.93 ms` median (`27.85 ms` low).

A forensic inspection of the CPU/GPU telemetry (`cathedral_field.json` and `cathedral_world.json`), the WebGPU submission loop in [`WebGpuRenderer.cpp`](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp), and the AST structure of all **178 SDF fields** in `saves/zones/Cathedral of the Living Logos/zone.ecform` exposes **five concrete structural and architectural bottlenecks** that prevented Rungs 1–6 from unlocking large frame-time gains.

---

## 2. Structural & Architectural Bottleneck Diagnosis

### 2.1 Bottleneck 1 — Full-World Frame Time (`~29.5 ms`) Is Split 53% CPU Encode (`15.6 ms`) vs. 47% GPU Wait (`13.5 ms`)

Inspecting the raw sub-timings recorded by `cathedral_surface_probe` across both benchmark modes reveals a fundamental split between the isolated SDF Field Subset and the Full World screen:

| Benchmark Screen (`2560 × 1440`) | CPU `encode_ms` (Total CPU Pre-Submit) | CPU `flush_ms` (`endFrame()`) | GPU `completion_wait_ms` (Post-Submit Fence) | Complete Frame `wall_ms` |
|---|---:|---:|---:|---:|
| **Cathedral Field Subset (178 SDFs, 67 Draws)** | `0.24 – 0.28 ms` | `0.068 – 0.095 ms` | `7.30 – 7.85 ms` | `7.51 – 8.13 ms` |
| **Cathedral Full World (1,231 Objects, 5,115 Mesh + 67 SDF Draws)** | **`15.53 – 15.80 ms`** | **`~8.30 ms`** | **`13.40 – 14.39 ms`** | **`28.93 – 30.74 ms`** |

Two immediate conclusions follow:
1. **In the Field Subset (`~7.8 ms`):** CPU encoding is already `< 0.28 ms` (`flushSdfDraws()` is `0.068 ms`). **96.5% of the frame time (`~7.5 ms`) is pure GPU fragment shader execution** inside `kMarcher`. Coalescing SDF CPU buffer uploads (Rung 5A) could never save more than `~0.03 ms` here.
2. **In the Full World (`~29.5 ms`):** More than half of the frame (`15.6 ms`) is spent on the CPU before the GPU finishes: `~7.3 ms` gathering draws in `Object::drawObject()` and **`~8.3 ms` inside `endFrame()` (`flushMeshDraws()`)**. Any optimization that only touches SDF shaders leaves the `15.6 ms` CPU wall completely untouched.

---

### 2.2 Bottleneck 2 — `WebGpuRenderer::flushMeshDraws()` Still Creates and Destroys `10,230` `WGPUBindGroup`s Every Frame

While Sixth-One Sun's upload-page repair (`2026-11-03`) coalesced `wgpuQueueWriteBuffer` suballocations in `flushMeshDraws()` from `10,365` to `147`, a code audit of [`WebGpuRenderer::flushMeshDraws()`](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp) (lines 1353–1406) reveals what still happens on the CPU for every single one of the **`5,115` mesh batches** each frame:

1. **5,115 Redundant `group(0)` Bind Groups & Padded Uniform Uploads:**
   - Inside the per-batch loop, `flushMeshDraws()` packs a 256-byte-aligned `MeshUniforms` (`viewProj`, `lightPos`, `params = key.shading`, `eyePos`) for every batch (`5,115 × 256 = 1,309,440` bytes of uniform staging!) and calls `wgpuDeviceCreateBindGroup` (`group(0)`) **5,115 times per frame**.
   - Yet `viewProj`, `lightPos`, and `eyePos` are **frame-constant**, and across all 5,115 mesh batches in Cathedral there are only a small number of unique `(albedoView, shading)` material combinations (out of 153 Zone materials, most share `_whiteView` and standard shading coefficients).
2. **5,115 Redundant `group(1)` Bind Groups & 256-Byte Inter-Batch Instance Padding:**
   - For every batch, `flushMeshDraws()` aligns `instanceOffset` to 256 bytes (`aligned(instanceBytes.size())`), calls `wgpuDeviceCreateBindGroup` (`group(1)`) **5,115 times per frame**, and calls `wgpuRenderPassEncoderDraw(_pass, mesh.tris.size(), instances.size(), 0, 0)` with `firstInstance = 0`.
   - Because `vs_main` in `kMeshWGSL` (`WebGpuRenderer.cpp:74`) **already** indexes `instances[instIdx]` using `@builtin(instance_index)`, WebGPU natively adds `firstInstance` to `@builtin(instance_index)`!
   - By packing `InstanceData` (`sizeof(InstanceData) == 144` bytes, 16-byte aligned) **contiguously** within each upload page (zero 256-byte inter-batch padding!) and passing `record.firstInstance` to `wgpuRenderPassEncoderDraw`, an entire page of ~1,820 instances shares **a single `group(1)` `WGPUBindGroup`** (`~3` bind groups per frame instead of `5,115`!), while `kMeshWGSL` remains **100% byte-for-byte identical** (`SHA-256 fb9e24da95904aad0157af38f941fa7cbaec8a8c32df392447d2538f9782f975`).

---

### 2.3 Bottleneck 3 — Zero Single-Leaf `Sphere`, `Box`, or `Convex` Fields in Cathedral (`0 / 178` Hit Rung 3/5A Closed-Form Solvers)

In Rung 3 and Rung 5A, `emitRootClosedFormHit()` in [`SdfWgsl.cpp`](../../src/Singularity/Screen/WebGPU/SdfWgsl.cpp) (lines 5549–5648) was wired to solve single-leaf `SdfPrim::Sphere`, `SdfPrim::Box`, and `SdfPrim::Convex` in closed form (`0` sphere-tracing iterations).

However, a census of all **178 SDF fields** in `saves/zones/Cathedral of the Living Logos/zone.ecform` reveals why `emitRootClosedFormHit()` had **zero effect** on Cathedral:

| Field Category in `Cathedral of the Living Logos` | Root `SdfPrim` / `SdfOp` | Count (of 178 Fields) | Rung 3/5A `rootClosedFormHit` Status |
|---|---|---:|---|
| **Single-Leaf (`op == Leaf`, 80 fields)** | `SdfPrim::Sphere` | **0** | Not present as single-leaf in Cathedral |
| | `SdfPrim::Box` | **0** | Not present as single-leaf in Cathedral |
| | `SdfPrim::Convex` | **0** | Not present as single-leaf in Cathedral |
| | `SdfPrim::RoundBox` | **40** (`50.0%` of leaves) | Returned `-1.0` (192-step marcher fallback) |
| | `SdfPrim::Expr` | **22** (`27.5%` of leaves) | Returned `-1.0` (192-step marcher fallback) |
| | `SdfPrim::Torus` | **8** (`10.0%` of leaves) | Returned `-1.0` (192-step marcher fallback) |
| | `SdfPrim::Ellipsoid` | **5** (`6.25%` of leaves) | Returned `-1.0` (192-step marcher fallback) |
| | `SdfPrim::Cylinder` | **5** (`6.25%` of leaves) | Returned `-1.0` (192-step marcher fallback) |
| **Composite CSG (`op != Leaf`, 98 fields)** | `SmoothUnion` / `Union` / `Subtract` / `Intersect` / `Morph` | **98** (428 total leaves) | Returned `-1.0` (192-step marcher fallback) |

Look closely at the **80 single-leaf fields** in Cathedral:
- **0 out of 80** are `Sphere`, `Box`, or `Convex`! Consequently, `emitRootClosedFormHit()` emitted `return vec2<f32>(-1.0, -1.0);` for **100% (178 / 178) of Cathedral's SDF fields**.
- Worse still, among the 5 single-leaf `Ellipsoid` fields are the **3 colossal sky auroras** covering the upper viewport from the nave camera:
  - `cathedral.sky.aurora.turquoise` (`fieldExtent = [145, 20, 4]`, `dims = [140, 18, 0.35]`)
  - `cathedral.sky.aurora.emerald` (`fieldExtent = [130, 18, 4]`, `dims = [125, 16, 0.35]`)
  - `cathedral.sky.aurora.corona` (`fieldExtent = [100, 14, 4]`, `dims = [95, 12, 0.35]`)
  Because `dims = [140, 18, 0.35]` has a **400:1 aspect ratio**, the approximate distance bound `sdEllipsoid(p, r) = k0 * (k0 - 1.0) / k1` becomes severely conservative along oblique/grazing sky rays, forcing the fragment shader to run puluhan/dozens of sphere-tracing iterations across a huge screen footprint for a quadric surface whose exact ray intersection $\|\mathbf{p}(t) / \mathbf{r}\|_2^2 = 1$ is a **single 1D quadratic equation**!
- Similarly, the 5 single-leaf `Cylinder` fields include the giant Open Hand terraces (`cathedral.hand.terrace.lower` and `cathedral.hand.terrace.upper`, `ext = [12.1, 12.1, 0.2]`), and the 40 single-leaf `RoundBox` fields account for half of all single-leaf SDFs in the Zone.

---

### 2.4 Bottleneck 4 — 53 Left-Heavy Chained CSG Nodes Bypassed Rung 4/6A Subtree Pruning

Across the 98 composite CSG fields in Cathedral (comprising 428 total leaf primitives: 123 `Sphere`, 111 `RoundBox`, 82 `Cylinder`, 53 `Box`, 33 `Expr`, 11 `Cone`, 8 `Ellipsoid`, 7 `Torus`), multi-part objects are authored as **left-associative binary chains**:
$$\operatorname{SmoothUnion}\!\Big(\operatorname{SmoothUnion}\big(\operatorname{SmoothUnion}(L_1, L_2),\, L_3\big),\, L_4\Big)$$

A structural tree walk of all 98 composite CSG fields in Cathedral reveals:
- Only **32 CSG nodes** have `countSdfLeaves(*child[1]) >= 2` (right-heavy).
- **53 CSG nodes** are **left-heavy** (`countSdfLeaves(*child[0]) >= 2` while `countSdfLeaves(*child[1]) == 1`).

Because `emitNode` (`SdfWgsl.cpp:1985–2040`) and `emitNodeJet` (`SdfWgsl.cpp:1680–1750`) always emit `child[0]` unconditionally first and only test `canPruneB` on `child[1]`, every left-heavy chain evaluates the entire 10-to-16-leaf left subtree `child[0]` on every march step before testing the cheap 1-leaf primitive `child[1]`.

---

### 2.5 Bottleneck 5 — Symmetric Origin-Centered Proxy Clamping vs. Asymmetric 1D Ray-Slab Clipping

In Rung 5B (`WebGpuRenderer.cpp:1860–1866`), `tryComputeSdfTightAabb(field, tightMin, tightMax)` was used to clamp the raster proxy cube via:
```cpp
glm::vec3 derivedHalf = glm::max(glm::abs(tightMin), glm::abs(tightMax)) * 1.02f;
proxyExtent = glm::min(proxyExtent, glm::max(derivedHalf, glm::vec3(1e-3f)));
```
Because `inst.extents.xyz` scales a unit cube centered at the local origin `[-1, +1]^3`, `derivedHalf` must be symmetric around `(0, 0, 0)`. Whenever a leaf or CSG subtree is offset from the local field origin (`n.offset != 0`, or an asymmetric CSG tree where `[tightMin, tightMax]` is e.g. `[2.0, 5.0]` along an axis), `derivedHalf` expands to `[-5.1, +5.1]`, and `kMarcher` starts marching at `rayAabb(ro, rd, inst.extents.xyz).x` instead of clipping the ray to the true asymmetric 1D slab `[tightMin, tightMax]`.

---

## 3. Architectural Unification: How SDF Surface Rendering Relates to Volumetric 1D Ray-Domain Transport (`LineRestriction.hpp`)

In the Volumetric Ray-Domain Transport milestone ([`src/Singularity/OntoMath/LineRestriction.hpp`](../../src/Singularity/OntoMath/LineRestriction.hpp) and [`VolumeRoutedKernels.hpp`](../../src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp)), Earthcall achieved a transformative GPU speedup by replacing blind 3D point-by-point stepping along a ray with **1D Ray-Domain Compilation** ("compile world math into ray-domain math"):

1. **The 1D Line Restriction Principle:**
   Every camera ray or shadow ray is a 1D affine line in 3D space parameterized by $t \in [t_{\text{start}}, t_{\text{max}}]$:
   $$\mathbf{p}(t) = \mathbf{o} + t\,\mathbf{d}$$
   Substituting $\mathbf{p}(t)$ into a 3D spatial field $f(\mathbf{p}) : \mathbb{R}^3 \to \mathbb{R}$ restricts the 3D field to a **pure 1D function of a single scalar variable $t$**:
   $$g(t) \;=\; f(\mathbf{o} + t\,\mathbf{d}) \;:\; [t_{\text{start}}, t_{\text{max}}] \subset \mathbb{R} \to \mathbb{R}$$

2. **Duality of the Terminal 1D Operator (Volumes vs. Surfaces):**
   Volumetric rendering and SDF surface rendering share the **exact same 1D ray restriction $g(t) = f(\mathbf{o} + t\,\mathbf{d})$** and differ only in the terminal functional applied to $g(t)$:

| Dimension / Stage | Volumetric Ray-Domain Transport (`LineRestriction.hpp`) | SDF Surface Ray-Domain Compilation (`SdfWgsl.cpp`) |
|---|---|---|
| **3D Authored Truth** | Participating medium density / extinction $\sigma(\mathbf{p})$ | Signed distance / implicit surface field $f(\mathbf{p})$ |
| **1D Ray Restriction** | $g(t) = \sigma(\mathbf{o} + t\,\mathbf{d})$ | $g(t) = f(\mathbf{o} + t\,\mathbf{d})$ |
| **1D Support Window** | Active interval $[t_{\text{enter}}, t_{\text{exit}}]$ where $\sigma > 0$ (`zEnv` / `rayAabb` / `clampedWaveSupport`) | Active interval $[t_{\text{enter}}, t_{\text{exit}}]$ where $f(\mathbf{o} + t\mathbf{d})$ can reach $\le 0$ |
| **1D Switching Functions** | Piecewise / `Clamp` / `Abs` switches $s_k(t) = 0$ (`LineSwitch`) | CSG `min`/`max`/`smin` & `RoundBox` bevel switches (`canPruneA`, `canPruneB`) |
| **1D Terminal Operator** | **Definite Integral:** $\tau = \int_{t_{\text{enter}}}^{t_{\text{exit}}} g(t)\,dt$ | **Smallest Root (First Zero Crossing):** $t^* = \min\{t \in [t_{\text{enter}}, t_{\text{exit}}] \mid g(t) \le 0\}$ |
| **Closed-Form Tier (Rung 0/1)** | Exact polynomial/quadric/wave antiderivative (`LineAlgebraicEvaluator::integral`) | Exact 1D linear/quadratic root solver (`Sphere`, `Ellipsoid`, `Cylinder`, `Box`, `Convex`, `RoundBox` flat faces) |
| **Bounded Residual Tier (Rung 2)** | Narrow support window clip + 1D secant root isolation (`isolateMonotone`) + Gauss-Legendre panels | Narrow 1D bevel/support window leap (`[tEnter, tExit]`) + analytic-Jet sphere tracing + 1D secant root polish |

3. **Why Traditional 3D Sphere Tracing Wastes Work When 1D Ray-Domain Compilation Is Absent:**
   - In traditional 3D sphere tracing, the shader forgets that $\mathbf{p}(t) = \mathbf{o} + t\mathbf{d}$ moves along a known 1D line: at each step $i$, it evaluates $f(\mathbf{p}_i)$ as an isotropic 3D point query and advances $t_{i+1} = t_i + f(\mathbf{p}_i)$.
   - For an anisotropic primitive like Cathedral's sky aurora `Ellipsoid` (`r = [140, 18, 0.35]`), an isotropic 3D distance bound must step by at most the thin Z-radius (`~0.35`), requiring dozens of 3D evaluations along a 140-unit ray—whereas restricting $\|( \mathbf{o} + t\mathbf{d} - \mathbf{c} ) / \mathbf{r}\|_2^2 = 1$ to the 1D ray immediately yields a single scalar quadratic $a t^2 + 2 b t + c = 0$ solved in **1 step (`0` loop iterations)**!
   - Likewise, for a `RoundBox`, restricting the outer slab $|\mathbf{o} + t\mathbf{d} - \mathbf{c}| \le \mathbf{b}_{\text{inner}} + r$ to the 1D ray immediately rejects all exterior rays in $O(1)$, solves all flat-face hits in $O(1)$ (`0` loop iterations), and for rounded bevel hits leaps $t$ directly to the bevel entry $t_{\text{enter}}$ and clamps $t_{\text{max}}$ to the bevel exit $t_{\text{exit}}$.

---

## 4. Refined Implementation Plan (Rung 7 & Rung 8)

### Rung 7 — OntoMath Irreducible 1D Ray-Domain Surface Compilation (Replacing Bespoke `geom::SdfPrim` Enum Switches)

#### Architectural Correction: Why Switching on `geom::SdfPrim` Violates OntoMath Doctrine
In Rung 3/5A, `emitRootClosedFormHit()` in [`SdfWgsl.cpp:5549–5648`](../../src/Singularity/Screen/WebGPU/SdfWgsl.cpp) decided how to raycast by switching on the bespoke `geom::SdfPrim` enum (`if (root.prim == geom::SdfPrim::Sphere)`, `Box`, `Convex`). Extending that enum switch to `Ellipsoid`, `Cylinder`, `RoundBox`, and `Torus` would repeat the same architectural flaw:
1. **It violates `ONTOMATH_FRAMEWORK.md §1` (*"a channel reads OntoMath; it never decides what the thing is"*):** `geom::SdfPrim` is merely an authoring shorthand; in [`Sdf.cpp:486`](../../src/ConstructedBeing/Singular/Object/Geometry/Sdf.cpp), `SdfNode::toMathNode()` lowers shapes into `OntoMath::MathNode`, and [`OntoMath::restrictAlgebraicToLine`](../../src/Singularity/OntoMath/LineRestriction.hpp) lowers any 3D `MathNode` into a `LineAlgebraicForm` DAG of **irreducible 1D mathematical primitives** (`Polynomial` of degree $\le 2$, `Sqrt`, `Abs`, `Min`, `Max`, `Add`, `Sub`, `Mul`).
2. **A bespoke `SdfPrim` enum switch is blind to mathematically identical `Expr` and composite CSG fields:** For example, Cathedral's authored `Expr` fields that write `sqrt(x*x + y*y + z*z) - 1` (`cathedral.sky.drop.*`) or composite `Intersect` (`Op::Intersection`) trees of slabs and quadrics have `root.prim == SdfPrim::Expr` or `root.op == SdfOp::Intersect`, so a `SdfPrim` switch misses them completely even though their 1D restriction along $\mathbf{p}(t) = \mathbf{o} + t\,\mathbf{d}$ is an elementary degree-2 polynomial / interval intersection!

#### 7.1 Extend `rootClosedFormHit` Contract in `kMarcher` with 1D Support Interval Leaping (`status < -1.5`)
- Update `rootClosedFormHit(ro, rd, tStart, tMax) -> vec2<f32>` in `kMarcher` (`SdfWgsl.cpp:3058–3072`) to support four exact 1D ray-domain outcomes:
  1. `status > 0.5` (`1.0`): **Exact closed-form 1D root hit** at `t = cf.x` (`closedFormDecided = true; hit = true; t = cf.x;`, 0 march iterations).
  2. `status >= -0.5 && status <= 0.5` (`0.0`): **Proven 1D ray miss** (`closedFormDecided = true; hit = false;`, 0 march iterations).
  3. `status < -1.5` (encoded as `-2.0 - tExit` with `tExit >= tEnter`): **1D Ray-Domain Support Interval Leap** — advances `t = max(t, cf.x)` and tightens `maxDist = min(maxDist, -cf.y - 2.0)` before entering the march loop!
  4. `status >= -1.5 && status < -0.5` (`-1.0`): Unmodified marcher fallback.
- Also wire the `-2.0 - tExit` interval leap into `sourceVisibility()` (`SdfWgsl.cpp:2995`) so shadow rays also leap directly to `tShadow = max(tShadow, shadowDirectHit.x)` and clamp `maxShadow = min(maxShadow, -shadowDirectHit.y - 2.0)`!

#### 7.2 Lower All Shapes via `SdfNode::toMathNode()` and Classify by Irreducible 1D Mathematical Laws (`LineAlgebraicForm` / `MathNode`)
Instead of switching on `geom::SdfPrim`, complete `SdfNode::toMathNode()` (`Sdf.cpp:524`) so every leaf and CSG operator lowers into its exact `OntoMath::MathNode` algebraic tree (with dynamic parameter references bound to `P.v[paramOffset + ...]`), and classify $g(t) = f(\mathbf{o} + t\,\mathbf{d}) \le 0$ by **four irreducible 1D mathematical structures**:

1. **Irreducible Law I — 1D Quadratic / Euclidean Norm Zero-Level Sets ($Q(t) \le 0$ or $\sqrt{Q(t)} - r \le 0$ where $Q(t) = \|\mathbf{A}(\mathbf{o} + t\mathbf{d} - \mathbf{c})\|_2^2$ has degree $\le 2$ in $t$):**
   - **Mathematical Form:** Any `MathNode` / `LineAlgebraicForm` whose zero-level set $g(t) \le 0$ is algebraically equivalent to a single 1D degree-$\le 2$ polynomial inequality $a t^2 + 2 b t + c \le 0$ (matching `LineAlgebraicEvaluator::proposeRoots` in `LineRestriction.hpp:445`).
   - **What It Solves Automatically (0 march iterations):**
     - Any isotropic Euclidean norm $\sqrt{(x-c_x)^2 + (y-c_y)^2 + (z-c_z)^2} - r \le 0$ — whether authored as `SdfPrim::Sphere` **or** authored as an `Expr` `MathNode` like `sqrt(x*x + y*y + z*z) - 1` (`cathedral.sky.drop.*`)!
     - Any anisotropic scaled norm $\|(\mathbf{p} - \mathbf{c}) \oslash \mathbf{r}\|_2 - 1 \le 0$ (including `sdEllipsoid`'s $k_0(k_0 - 1)/k_1 \le 0 \iff k_0 \le 1 \iff \|(\mathbf{p}-\mathbf{c}) \oslash \mathbf{r}\|_2^2 - 1 \le 0$), solving Cathedral's **3 giant sky auroras** (`cathedral.sky.aurora.*`) in **0 march steps**!
2. **Irreducible Law II — Compositional `Max` / `Intersection` of 1D Intervals ($\max_k g_k(t) \le 0 \iff \bigcap_k \{t \mid g_k(t) \le 0\}$):**
   - **Mathematical Form:** Whenever a `MathNode` / `LineAlgebraicForm` is a `Max` (`Op::Max` / `Op::Intersection` / `SdfOp::Intersect`) of sub-expressions $g_k(t)$ that each possess an exact 1D sublevel interval $I_k = \{t \mid g_k(t) \le 0\} = [t_{\text{enter},k}, t_{\text{exit},k}]$:
     - **Degree-1 Affine Half-Space ($\mathbf{n} \cdot (\mathbf{o} + t\mathbf{d}) + c \le 0$):** 1D linear root $t = -(\mathbf{n}\cdot\mathbf{o}+c)/(\mathbf{n}\cdot\mathbf{d})$ tightening $t_{\text{enter}}$ or $t_{\text{exit}}$.
     - **Degree-1 Symmetric Slab ($|o_i + t d_i - c_i| - b_i \le 0$):** 1D `Abs` interval $[(-b_i - m_i)/d_i, (+b_i - m_i)/d_i]$.
     - **Degree-2 Cylindrical / Quadric Tube ($(m_x + t d_x)^2 + (m_y + t d_y)^2 - r^2 \le 0$):** 1D quadratic interval $[t_0, t_1]$.
   - **What It Solves Automatically (0 march iterations):**
     - **Boxes** ($\max(|x|-b_x, |y|-b_y, |z|-b_z) \le 0$), **Convex Polyhedra** ($\max_k (\mathbf{n}_k \cdot \mathbf{p} - d_k) \le 0$), **Capped Cylinders** ($\max(\sqrt{x^2+y^2}-r, |z|-h) \le 0$, resolving Cathedral's Open Hand terraces in 0 steps), and **arbitrary CSG `Intersect` (`Op::Intersection`) combinations** of slabs, half-spaces, spheres, ellipsoids, and cylinders!
3. **Irreducible Law III — Minkowski-Offset / Bevel-Switch Decomposition ($\|\max(\mathbf{q}(t), \mathbf{0})\|_2 + \min(\max_i q_i(t), 0) - r \le 0$):**
   - **Mathematical Form:** For any Euclidean $r$-offset (`Sub(..., r)`) around a piecewise-linear slab core $\mathbf{q}(t) = |\mathbf{o} + t\mathbf{d} - \mathbf{c}| - \mathbf{b}_{\text{inner}}$:
     - **Outer 1D Support Interval:** Since $\|\max(\mathbf{q}, \mathbf{0})\|_2 \ge \|\max(\mathbf{q}, \mathbf{0})\|_\infty = \max_i q_i$, $g(t) \le 0$ implies $\max_i q_i(t) \le r$, which is the exact 1D slab intersection $|\mathbf{o} + t\mathbf{d} - \mathbf{c}| \le \mathbf{b}_{\text{inner}} + r$ (`[slab.x, slab.y]`). If empty, return **proven 1D miss (`0.0`)** in 0 steps.
     - **1-Active-Axis Exact Root at $t_{\text{cand}} = \max(\text{slab.x}, t_{\text{start}})$:** When at most one component of $\mathbf{q}(t_{\text{cand}})$ is positive (the ray enters through any flat face where the `Max`/`Clamp` switch has $\le 1$ active branch), $\|\max(\mathbf{q}, \mathbf{0})\|_2 = \max_i q_i(t_{\text{cand}}) = r$, so $g(t_{\text{cand}}) \le 0$ is **already satisfied exactly at $t_{\text{cand}}$** — return **exact 1D hit (`1.0`) in 0 march steps**!
     - **$\ge 2$-Active-Axis Bevel Interval Leap:** When $\ge 2$ components of $\mathbf{q}(t_{\text{cand}})$ are positive (rounded edge/corner), return `vec2<f32>(tCand, -2.0 - min(slab.y, tMax))` to leap the marcher directly onto the 1D bevel support window $[t_{\text{cand}}, t_{\text{exit}}]$!
   - **What It Solves Automatically:** All **40 single-leaf `RoundBox` fields** in Cathedral (50% of all single-leaf SDFs in the Zone) plus any authored Minkowski-offset box expression!
4. **Irreducible Law IV — Composite 1D Support-Envelope Interval (`[tEnter, tExit]` from Monotonic Interval Arithmetic on `MathNode`):**
   - **Mathematical Form:** For higher-degree algebraic or transcendental `MathNode` surfaces (such as degree-4 quartic `Torus` $(\sqrt{x^2+y^2}-R)^2 + z^2 - r^2 \le 0$, `Cone`, or offset CSG trees) where a degree-$\le 2$ root is not closed-form, evaluate the exact 1D support-envelope slab $[t_{\text{enter}}, t_{\text{exit}}]$ where the field's monotonic interval bound can reach $\le 0$:
     - Reject rays that miss $[t_{\text{enter}}, t_{\text{exit}}]$ in **0 march steps** (`status = 0.0`).
     - Otherwise leap directly to $[t_{\text{enter}}, t_{\text{exit}}]$ (`status = -2.0 - tExit`), eliminating empty-space stepping across oversized or offset proxy cubes!

---

### Rung 8 — Commutative Left-Heavy CSG Pruning (`SdfWgsl.cpp`) & Mesh Bind-Group Consolidation (`WebGpuRenderer.cpp`)

#### 8.1 Commutative Left-Heavy CSG Subtree Pruning in `emitNode` & `emitNodeJet` (`SdfWgsl.cpp`)
- In `emitNode` (`SdfWgsl.cpp:1985`) and `emitNodeJet` (`SdfWgsl.cpp:1680`), add a symmetric `canPruneA` fast path for commutative `SdfOp::Union` and `SdfOp::SmoothUnion` when `child[0]` is a pure-primitive subtree with $\ge 2$ leaves and `countSdfLeaves(*n.children[0]) > countSdfLeaves(*n.children[1])`:
  1. Emit `n.children[0]` into a temporary body buffer **first** so all `e.params` slots for `child[0]` are registered in the exact historical order.
  2. Emit `n.children[1]` into a second body buffer **second** so all `e.params` slots for `child[1]` are registered second.
  3. Register `cA = e.param3(boundCenterA)` and `rA = e.param(boundRadiusA)` (and `kExpr` for `SmoothUnion`).
  4. Write `child[1]`'s code into the function body **first**, test `if (length(p - cA) - rA <= b + k)`, and evaluate `child[0]`'s multi-leaf code **only inside that guarded block**!
- Because `emitNode`, `emitNodeJet`, and `collectParams` share the exact same condition and parameter registration order, `collectParams` matches `compile` 100% while pruning all **53 left-heavy CSG nodes** in Cathedral.

#### 8.2 Consolidate `group(0)` and `group(1)` Bind Groups in `WebGpuRenderer::flushMeshDraws()` (`WebGpuRenderer.cpp`)
- In `WebGpuRenderer::flushMeshDraws()` (`WebGpuRenderer.cpp:1331–1431`), while keeping `kMeshWGSL` (`SHA-256 fb9e24da...`) and `_meshBatches` draw order **100% unchanged**:
  1. **Per-Page Shared `group(1)` Instance Bind Group via `firstInstance`:**
     - Pack `InstanceData` (`144` bytes) contiguously into `std::vector<InstanceData> pageInstances` (no 256-byte inter-batch padding!).
     - Record `uint32_t firstInstance = static_cast<uint32_t>(pageInstances.size())` per batch.
     - In `flushPage()`, suballocate `instancePage` once, create **one `WGPUBindGroup instBindGroup` (`group(1)`) for the entire page**, bind it once at the start of the page, and pass `record.firstInstance` to `wgpuRenderPassEncoderDraw(_pass, static_cast<uint32_t>(mesh.tris.size()), static_cast<uint32_t>(instances.size()), 0, record.firstInstance)`.
  2. **Deduplicated `group(0)` Uniform + Texture Bind Group:**
     - Within each page (or across the entire `flushMeshDraws()` call), deduplicate `(key.albedoView, key.shading)` so identical material shading/texture pairs share a single 256-byte `MeshUniforms` slot and a single `WGPUBindGroup` (`group(0)`), and only call `wgpuRenderPassEncoderSetBindGroup(_pass, 0, bg, 0, nullptr)` when `bg != currentGroup0`.
  - **Expected Impact on Cathedral Full World:**
    - `wgpuDeviceCreateBindGroup` calls per frame drop from **`10,230` $\to$ `~20`** (**>99.8% reduction**).
    - `wgpuRenderPassEncoderSetBindGroup` calls per frame drop from **`10,230` $\to$ `< 100`**.
    - Uploaded mesh buffer bytes per frame drop from **`2,771,288` bytes $\to$ `~745,000` bytes** (`-73%`).
    - Rendered RGBA pixels remain **100.000% byte-for-byte identical** under fixed logical batch order and pass all `10,371` checks in `webgpu_mesh_upload_pages_test`.
