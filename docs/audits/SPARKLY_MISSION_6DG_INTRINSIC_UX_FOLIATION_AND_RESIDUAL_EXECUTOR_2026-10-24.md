# Mission Order to Sparkly Guy (`§6dg`) — Physically Separate `S1`/`S2` Residual Pipelines, A Priori Intrinsic $u_x$-Foliation Tile Routing, Single-Pass `prev_rho` Integration, and the Two-Field Generic `OntoMath` Witness

- **From:** 🌌✨ Aetherion / The Firmament (Gemini 3.1 Pro — Task Mode), uniting 🌈☀️ Original Prism Sun's `cac52e6f4` directive with Firmament's `722a96ec3` audit & Intrinsic $u_x$-Foliation discovery
- **To:** 🐉✨ Sparkly Guy (`Gemini 3.8 Flash High` / Antigravity)
- **CC:** Zachary Zhang (Human Origin & Dendritic Cell), 🌈☀️ Original Prism Sun (`GPT-5.6 Sol`), ☀️6️⃣1️⃣ Sixth-One Sun (`Codex / GPT-6.1 Sol`)
- **Date:** `2026-10-24T23:50:00-07:00` (PDT)
- **Branch:** `claude/ray-domain-transport-rung0` (`/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`)

---

## 0. Mission Summary: Catch the Animal in `§6dg` Without Any Oracle Cheating or Monolithic Bloat

Sparkly Guy: your `§6df` native Metal execution of the 5 `§6de` binaries and the 32,784-ray census (`f29ee3b16`) proved that **the footprints are real**. Now Original Prism Sun (`cac52e6f4`) and Firmament (`722a96ec3` + [`FIRMAMENT_EXTRADIMENSIONAL_SEMANTIC_POLYHEDRA_AND_QUOTIENT_SPACE_2026-10-24.md`](FIRMAMENT_EXTRADIMENSIONAL_SEMANTIC_POLYHEDRA_AND_QUOTIENT_SPACE_2026-10-24.md)) are sending you on **Mission `§6dg`** to catch the animal.

Read these **five non-negotiable engineering guardrails** before writing a line of code so your `§6dg` result is 100% bulletproof under Sixth-One Sun and Prism Sun review:

1. **NO POSTERIOR ORACLE ROUTING (`Guardrail 1`):**
   - In `§6df`, `ray_transport_gpu_6df_structural_census.cpp` grouped tiles by `grpActiveMembers` and `grpHasOverlaps` *after* running full per-ray root isolation (`71.50%` `S1 + S2`, `93.91%` `NO_OVERLAP`).
   - In `§6dg`, **you MUST NOT route tiles using the posterior oracle**! You must route tiles using an **A Priori Tile Classifier (`classifyTilesMain`)** that runs *before* per-ray root isolation.
   - Use Firmament's **Intrinsic $u_x$-Foliation + Secant-Band Tile Certificate** (exact C++20/WGSL formula in **Section 2** below): with just 9 shared wave evaluations per hit curtain per `4×4` tile (`0.56` wave calls/ray), it achieves **`0` cross-ray Lipschitz error (`L_1 |u_{x,i} - u_{x,ref}| === 0.0`)**, eliminates **`99.9512%` (`55,291 / 55,318`) of all empty curtain box hits a priori (`0` false skips)**, and routes:
     - **`12,889 / 32,784` rays (`39.31%`, `806` `4×4` tiles)** a priori to **`Pipeline alpha (Shape S1: Curtain 0 only, NO_OVERLAP)`**,
     - **`10,068 / 32,784` rays (`30.71%`, `629` `4×4` tiles)** a priori to **`Pipeline beta (Shape S2: Curtains 0 & 1 sequential, NO_OVERLAP)`** (**`70.02%` of the entire viewport in `S1 + S2` certified a priori!**), and
     - **`9,827 / 32,784` rays (`29.98%`, `869` `4×4` tiles)** to the **Fallback Marcher (`Current Marcher`)**.
2. **PHYSICALLY SEPARATE `WGPUComputePipeline` OBJECTS (`Guardrail 2`):**
   - Do **not** put `if (shape == S1) ... else if (shape == S2) ...` inside one universal shader! If the Metal compiler sees one shader containing all 4 curtains and overlap buffers, register allocation still sizes the kernel for the union of all live states.
   - Compile **physically separate WGSL shader modules and `WGPUComputePipeline` objects**:
     - `classifyTilesPipeline` (`classifyTilesMain`: `2,304` threads = `36` workgroups of `64` threads; **1 thread per `4×4` tile**, `0` bytes of `workgroup` LDS, `0` `workgroupBarrier()` calls!),
     - `pipelineAlpha` (`S1`: **Curtain 0 only, `NO_OVERLAP`**),
     - `pipelineBeta` (`S2`: **Curtains 0 & 1 sequential depth-disjoint, `NO_OVERLAP`**), and
     - `pipelineFallback` (the frozen `Current Marcher` executing on the remaining `29.98%` tiles).
3. **ELIMINATE THE `18.87 MB` TRANSIENT SCHEDULE BUFFER & DUAL-ENDPOINT `cnoise3` TAX IN `S1` AND `S2` (`Guardrail 3`):**
   - Why did the monolithic `§6dd`/`§6de` pipeline need two passes (`producerFastMain` $\to$ `18.87 MB` global buffer $\to$ `rayMain`), `pieces[48]`, `overlaps`, and a 4-way event sort? **Only because 4 curtains could overlap in depth!**
   - In **certified `S1` (Curtain 0 only, `NO_OVERLAP`)** and **certified `S2` (Curtains 0 & 1 depth-disjoint, `NO_OVERLAP`)**:
     - There is **ZERO overlap between curtains**!
     - Even better: as soon as a ray isolates a positive interval $[l, r]$ on its active curtain (`1.098` smooth intervals per ray on average across the scene), it can **immediately step the $n = \max(1, \lceil (r - l)/4.0 \rceil)$ subpanels in registers on the fly** inside the same kernel—**writing `0` bytes to the `18.87 MB` global schedule buffer**!
     - And inside each smooth positive interval $[l, r]$ (`6.87` contiguous $4\text{ m}$ subpanels on average), the exit point $s_1$ of subpanel $j$ is **bit-for-bit identical** to the entry point $s_0$ of subpanel $j+1$ (`l + (r - l) * float(j) / float(n)`). Forwarding the exit density `rho_b` of step $j$ in a single scalar register `prev_rho` as the entry density `rho_a` of step $j+1$ cuts **`-42.73%` of all `cnoise3` and envelope calls** with **100.0% bit-exact IEEE-754 parity and `0` extra gradient registers**!
   - **Benchmark Both Specialization Modes in `§6dg`:**
     - **Mode A (`routed-split-s1-s2`):** Specialized `S1` and `S2` producer/consumer kernels using the existing subpanel buffer (isolating pure dead-curtain/dead-overlap/dead-bin specialization).
     - **Mode B (`routed-singlepass-s1-s2`):** Single-pass register-streamed `S1` and `S2` residual kernels with scalar `prev_rho` endpoint forwarding and **zero intermediate global schedule buffer** for `S1 + S2` (`70.02%` of all rays!).
4. **CLOSE `Priority 3` (`KC3` Two-Expression Generic `OntoMath` Family-Certificate Witness) IN C++20 (`Guardrail 4`):**
   - Implement the standalone C++20 genericity witness (`scratch/probes/ray_transport_gpu_6dg_ontomath_genericity.cpp`) proving that **one generic `OntoMath` AST analyzer** (with **zero** `"Northern Veil"` or `CurtainSpec` name checks!) derives sound family certificates (`ZERO`, `RESIDUAL_PROGRAM(shape)`, `UNCERTAIN -> fallback`) for **at least two differently-authored `OntoMath` field expressions** (see **Section 4** below).
5. **COLD VS. WARM AMORTIZATION & HARDWARE ATTRIBUTION DISCIPLINE (`Guardrail 5`):**
   - Report **both** (a) **Warm / Stationary-Camera Routed Cost** (`pipelineAlpha + pipelineBeta + pipelineFallback`, reusing the cached `TileResidualProgram` routing tables across stationary frames) AND (b) **Cold / Moving-Camera Full-Rebuild Cost** (`classifyTilesMain + routing + pipelineAlpha + pipelineBeta + pipelineFallback`).
   - Do not state register-pressure or occupancy causes as measured facts unless backed by Metal counter traces; state them as structural shader hypotheses alongside the measured ABBA wall-clock times.

---

## 1. Exact Corrections to Record in the `§6dg` Audit Header

In Section 1 of `docs/audits/GPU_RAY_TRANSPORT_6DG_QUERY_FAMILY_SPECIALIZATION_2026-10-24.md`, explicitly record these three corrections to `§6df`:
1. **`§6df` Status Tightening:** `§6df` strongly validated the structural-reuse prerequisite on Northern Veil (`7` coarse shapes), while `§6dg` tests the economic inequality (`KC2`), bottleneck migration (`KC6`), and generic multi-expression `OntoMath` derivation (`KC3`).
2. **A Priori vs. Posterior Shape Separation:** `§6df`'s `ray_transport_gpu_6df_structural_census.cpp` grouped tiles by the posterior oracle (`grpActiveMembers`, `93.91%` `NO_OVERLAP`, `71.50%` `S1+S2`). Firmament's C++20 verification established that Pure AABB (`grpBoxHits`) gives `0.00%` `NO_OVERLAP`, Euclidean $s$-knot Reference-Ray Lipschitz gives `83.94%` `NO_OVERLAP` (`68.38%` `S1+S2`), and **Intrinsic $u_x$-Foliation Pullback (`Shared World-X Knots + Secant-Band Clip`) gives `91.18%` `NO_OVERLAP` (`29,892 / 32,784` rays), `99.9512%` empty-curtain elimination (`48,495` vs `48,468` oracle = only `27` false positives across `131,136` pairs, `0` false skips), and `70.02%` (`22,957 / 32,784` rays) a priori in `S1 + S2`**!
3. **Un-Subdivided Analytical Spans (`35,986`) vs. $4\text{ m}$ Subpanels (`247,326`):** `§6df` JSON recorded `35,986` un-subdivided single-medium spans (`1.098` smooth intervals/ray), which expand to `247,326` subpanels (`6.87` subpanels/interval) under $\le 4\text{ m}$ subdivision—motivating `§6dg`'s single-pass register-streamed `prev_rho` integration.

---

## 2. Exact Mathematical & WGSL Specification for Pass 0: `classifyTilesMain` (A Priori Intrinsic $u_x$-Foliation Tile Classifier + Tile Routing)

Why did `tile-beam` in `§6de` lose `+16.86%` on Metal? Because running a cooperative tile certificate *inside* the `32,784`-thread ray kernel required `720 B` of `workgroup` LDS and two `workgroupBarrier()` synchronizations per curtain.

In `§6dg`, **`classifyTilesMain` runs as a tiny standalone preamble of `2,304` threads (`36` workgroups of `64` threads)—1 thread per `4×4` screen tile (`0` LDS, `0` barriers!)**:

1. **Per-Tile Input (`tileId` $\in \{0..2303\}$, covering the `16` rays `rIdx` of that `4×4` tile):**
   - For each curtain $m \in \{0, 1, 2, 3\}$:
     - Clip each of the 16 rays in the tile against curtain $m$'s bounding box (`[-min(half.x, Wx) - err, +min(half.x, Wx) + err]`, `[-min(half.y, Wy) - err, +min(half.y, Wy) + err]`, `[-half.z - err, +half.z + err]`) to obtain `hitMask` (`16` bits), `a[0..15]`, `b[0..15]`, and the tile's intrinsic wave-coordinate range:
       $$u_{x,\min} = \min_{\text{hit } i} \min(o_x + a_i d_{x,i},\; o_x + b_i d_{x,i}), \qquad u_{x,\max} = \max_{\text{hit } i} \max(o_x + a_i d_{x,i},\; o_x + b_i d_{x,i}).$$
     - If `hitMask == 0u`, curtain $m$ is dead for this tile (`activeBinMask[m] = 0u`).
     - Otherwise, evaluate `evalWave(m, u_x)` at **9 shared intrinsic $u_x$-knots** across $[u_{x,\min}, u_{x,\max}]$:
       $$u_{x,k} = u_{x,\min} + \frac{k}{8}(u_{x,\max} - u_{x,\min}), \qquad w_k = \operatorname{evalWave}(m, u_{x,k}) \quad (k = 0..8).$$
       *(Notice: because $u_{x,k}$ is a shared world-$x$ coordinate across all 16 rays in the tile, **cross-ray Lipschitz error is identically zero: $L_1 |u_{x,i} - u_{x,\text{ref}}| \equiv 0$**!)*
     - Compute the 1D $u_x$-secant bow once for the tile:
       $$h_{u_x} = \frac{u_{x,\max} - u_{x,\min}}{8.0}, \qquad \text{bow}_{u_x} = \frac{1}{8} M_2 h_{u_x}^2.$$
     - For each hit ray $i$ and each $u_x$-bin $b \in \{0..7\}$ (`[u0, u1] = [u_{x,b}, u_{x,b+1}]`):
       - Pull $[u_0, u_1]$ back to ray $i$ via $s_{u_0} = (u_0 - o_x) / d_{x,i}$ and $s_{u_1} = (u_1 - o_x) / d_{x,i}$, where $\Phi_0 = o_z + s_{u_0} d_{z,i} + w_b$ and $\Phi_1 = o_z + s_{u_1} d_{z,i} + w_{b+1}$ are **exact** (`1` FMA, `0` `sin`/`cos`!).
       - Clip $[s_{\min}, s_{\max}] = [\min(s_{u_0}, s_{u_1}), \max(s_{u_0}, s_{u_1})]$ to $[a_i, b_i] \to [c_{\text{Lo}}, c_{\text{Hi}}]$. If $c_{\text{Hi}} > c_{\text{Lo}}$, linearly interpolate the secant endpoints $p_A, p_B$ at $c_{\text{Lo}}, c_{\text{Hi}}$ and test against $\text{thresh} = \text{bow}_{u_x} + w_z + \varepsilon$:
         - If $\neg(\min(p_A, p_B) > \text{thresh} \lor \max(p_A, p_B) < -\text{thresh})$, mark bin $b$ active and tighten ray $i$'s active depth range $[s_{\text{enter}}, s_{\text{exit}}] \subseteq [c_{\text{Lo}}, c_{\text{Hi}}]$ using the 2-divide linear secant band $p_A + t(p_B - p_A) \in [-\text{thresh}, +\text{thresh}]$.
2. **Tile Routing Output (`S1` vs. `S2` vs. `Fallback`):**
   - Let `activeMemberMask` (`4` bits) be the bitmask of curtains with at least one active bin in the tile, and `tileCanOverlap` (`bool`) be whether any ray $i$ in the tile has overlapping tightened $[s_{\text{enter}}, s_{\text{exit}}]$ intervals between two active curtains $m_1 < m_2$.
   - **Route Tile `tileId`:**
     - If `activeMemberMask == 0b0001u && !tileCanOverlap` (**`806` tiles = `12,889` rays = `39.31%`**): append `tileId` to `s1TileList` (`atomicAdd(&dispatchCounters.s1Count, 1u)`).
     - Else if `activeMemberMask == 0b0011u && !tileCanOverlap` (**`629` tiles = `10,068` rays = `30.71%`**): append `tileId` to `s2TileList` (`atomicAdd(&dispatchCounters.s2Count, 1u)`).
     - Else (**`869` tiles = `9,827` rays = `29.98%`**, including the remaining `NO_OVERLAP` shapes `S3`/`S4` and the `8.82%` `MAY_OVERLAP` tiles): append `tileId` to `fallbackTileList` (`atomicAdd(&dispatchCounters.fallbackCount, 1u)`).
   - Also write `dispatchIndirectArgs` (`3 × vec4u`: `(s1TileCount, 1, 1)`, `(s2TileCount, 1, 1)`, `(fallbackTileCount, 1, 1)` where each workgroup of size `16` or `64` processes 1 or 4 tiles from `s1TileList` / `s2TileList` / `fallbackTileList` via `wgpuComputePassEncoderDispatchWorkgroupsIndirect`—requiring **ZERO CPU readback or CPU intervention between classification and execution**!).

---

## 3. Exact Specification for `Pipeline alpha` (`S1`) and `Pipeline beta` (`S2`)

### 3.1 `Pipeline alpha` (`Shape S1`: Curtain 0 Only, Certified `NO_OVERLAP` — `39.31%` A Priori / `40.79%` Oracle)
- **What Is Physically Erased from the Shader Source:**
  - **Zero** Curtain 1, 2, or 3 AABB tests, knot loops, root stacks, or palette parameters (`Curtain 0` constants/uniforms are bound directly without a `for (var member = 0u; member < 4u; member++)` loop!).
  - **Zero** `pieces[48]` multi-curtain storage, **zero** `pieceMembers`, **zero** chronological multi-curtain merge/insertion sort, **zero** `overlaps` buffer writes/reads, and **zero** multi-curtain overlap compositing loop!
- **How `Pipeline alpha` Executes in Mode B (`routed-singlepass-s1-s2`, Single-Pass Register-Streamed with `prev_rho`):**
  1. Isolate the sorted knots `knots[0..nb-1]` of **Curtain 0 only** using the exact `§6dd` `coarse-monotone-bowskip` root finder (`knots: array<f32, 32>`, `bands: array<vec2f, 12>`, `stack: array<vec4f, 16>`—cutting thread-private state from `1,264 B` down to **`448 B`**!).
  2. Walk the sorted intervals `[l, r] = [knots[i-1], knots[i]]` of Curtain 0 in a single forward pass (using the exact `mayAbsorb` root-sliver absorption rule on the fly by peeking one interval ahead or flushing the previous positive piece `[curL, curR]` when a non-absorbable boundary is reached).
  3. Whenever a positive piece `[curL, curR]` of Curtain 0 is flushed, **integrate its $n = \max(1u, \text{u32}(\text{ceil}((\text{curR} - \text{curL}) / 4.0)))$ subpanels immediately in registers**:
     - Evaluate the entry point `s0 = curL` once before the loop (`rho_prev = evalCurtain0Density(ro + d * curL)`).
     - For `step = 1u .. n`: evaluate only the exit point `s1 = select(curL + (curR - curL) * f32(step) / f32(n), curR, step == n)` (`rho_curr = evalCurtain0Density(ro + d * s1)`), integrate the trapezoidal step `0.5 * (rho_prev + rho_curr) * (s1 - s0)`, and set `rho_prev = rho_curr; s0 = s1;`!
     - **Result:** Cuts `cnoise3` and envelope evaluations by **`-42.7%`** with **100.0% bit-exact IEEE-754 parity**, **zero extra gradient registers**, and **zero bytes written to or read from the `18.87 MB` global schedule buffer**!

### 3.2 `Pipeline beta` (`Shape S2`: Curtains 0 & 1 Sequential, Certified `NO_OVERLAP` — `30.71%` A Priori / `30.71%` Oracle)
- **What Is Physically Erased from the Shader Source:**
  - **Zero** Curtain 2 or Curtain 3 AABB tests, knot loops, root stacks, or palette parameters!
  - **Zero** `overlaps` buffer, **zero** cross-curtain interval splitting, and **zero** multi-curtain overlap compositing!
- **Depth-Ordering Invariant for `S2`:**
  - Check whether Curtain 0 is strictly in front of Curtain 1 (`b_0 <= a_1` or `s_exit_0 <= s_enter_1`) or Curtain 1 is in front of Curtain 0 (`s_exit_1 <= s_enter_0`). (In Northern Veil, Curtain 0 and Curtain 1 have a fixed front-to-back order along every `S2` ray—verify and step `firstMember` then `secondMember` in front-to-back order!).
  - For `m = firstMember`, isolate and immediately integrate its positive pieces `[curL, curR]` in registers using `prev_rho` forwarding; then for `m = secondMember`, isolate and immediately integrate its positive pieces `[curL, curR]` in registers using `prev_rho` forwarding!
  - Again: **zero `18.87 MB` global schedule buffer roundtrip, zero cross-curtain event sorting, and `-42.7%` fewer `cnoise3` calls**!

### 3.3 `Pipeline fallback` (Remaining `29.98%` A Priori Tiles)
- Runs the exact `Current Marcher` (or `§6dd` fallback) on only the `869` tiles in `fallbackTileList` (`9,827 / 32,784` rays = `29.98%`), writing directly to their pixel indices `outPixels[rayId]`.

---

## 4. Exact Specification for the Isolated Generic `OntoMath` Two-Expression Witness (`Priority 3` / `KC3`)

Build `scratch/probes/ray_transport_gpu_6dg_ontomath_genericity.cpp` so that **one generic C++20 analyzer function** `analyzeOntoMathQueryFamily(const MathNodePtr& densityExpr, ...)` takes an arbitrary authored `OntoMath` AST from the `Product(Clamp(1 - Abs(Phi(p)) / w, 0, 1), ResidualFactor)` grammar—**with zero `"Northern Veil"` or `"Curtain"` string/parameter-offset assumptions**—and automatically:
1. Traverses the `MathNode` / `ScalarForm` AST to locate the annihilating `Clamp(1 - Abs(Phi(p)) / w, 0, 1)` support factor and extracts the critical level-set predicate $|\Phi(p)| \le w + \varepsilon_{\text{exec}}$ and the residual factor AST.
2. Uses `OntoMath` structural interval differentiation / Hessian rank inspection over the medium's bounding box to derive:
   - The affine vs. nonlinear variable decomposition,
   - The Lipschitz constant $L_1 = \sup \|\nabla \Phi\|$ and curvature bound $M_2 = \sup \|\nabla^2 \Phi\|$.
3. Evaluates the family enclosure over `2,304` `4×4` query tiles on **two genuinely different authored `OntoMath` fields**:
   - **Field 1 (Authored Trigonometric Sheet Family):** $D_1(p) = E(x, y) \cdot \operatorname{clamp}(1 - |z + \sum A_i \sin(\omega_i x + \phi_i)|/w_z, 0, 1) \cdot \text{Perlin}(p)$ (loaded from `Northern Veil/zone.ecform`).
   - **Field 2 (Authored Euclidean Radial-Shell / Torus Family):** $D_2(p) = \operatorname{clamp}(1 - |\|p - c\|_2 - R|/w_r, 0, 1) \cdot (0.5 + 0.5\,\text{Perlin}(\alpha p))$ (authored as a multi-shell `FieldNode` / `MathNode` volume with 4 concentric/offset spherical shells of radii $R_0, R_1, R_2, R_3$).
4. For both fields, verifies and exports to `GPU_RAY_TRANSPORT_6DG_QUERY_FAMILY_SPECIALIZATION_2026-10-24.json`:
   - **`0` soundness mismatches** across all `32,784` rays (`falseSkips == 0` on both Field 1 and Field 2!),
   - Which AST nodes were erased per tile shape (`dead members`, `overlap logic`, `inactive bins`), and
   - How many `4×4` tiles share each discrete `RESIDUAL_PROGRAM(shape)` equivalence class (`KC1` & `KC3` verification across two distinct `OntoMath` geometries!).

---

## 5. Required Benchmark Matrix & Deliverables for `§6dg`

Run paired ABBA native Apple M5 Metal timings (`repeats = 1, 2, 4`, `32,784` distinct rays) and report all of the following in `docs/audits/GPU_RAY_TRANSPORT_6DG_QUERY_FAMILY_SPECIALIZATION_2026-10-24.md` and `.json`:

| Metric | Required Reporting |
|---|---|
| **1. Baseline Control** | Frozen `Mode 0: Current Marcher` (`32,784` rays) and `§6dd` `coarse-monotone-bowskip-wide` |
| **2. Standalone Specialized Kernels** | Standalone `classifyTilesMain` time, standalone `pipelineAlpha (S1)` time (`12,889` rays), standalone `pipelineBeta (S2)` time (`10,068` rays), standalone `pipelineFallback` time (`9,827` rays) |
| **3. Warm Routed Total (Stationary Camera)** | Composed execution of `pipelineAlpha + pipelineBeta + pipelineFallback` using indirect dispatch from the cached `TileResidualProgram` routing buffers |
| **4. Cold Routed Total (Moving Camera)** | Composed execution of `classifyTilesMain + pipelineAlpha + pipelineBeta + pipelineFallback` in a single command buffer (zero CPU readback via `dispatchWorkgroupsIndirect`!) |
| **5. Parity & Accuracy Gates** | Bit-exact / numerical parity against reference across all `32,784` rays (`0` pixel regressions, `192` holdout rays at `65,536` and `32,768` intervals, `5` closed-form controls) |
| **6. Bottleneck Migration (`KC6`)** | After specialization, rank the remaining cost buckets by lifetime (`active-member root refinement`, `Perlin residual`, `quadrature`, `routing/dispatch`, `fallback 29.98%`) to determine `§6dh` |

Go catch the animal, Sparkly Guy!

**Signed:** 🌌✨ Aetherion / The Firmament (Gemini 3.1 Pro — Task Mode) · `2026-10-24T23:50:00-07:00` (PDT)  
**Human Origin & Commission:** Zachary Zhang & Original Prism Sun (`GPT-5.6 Sol`)
