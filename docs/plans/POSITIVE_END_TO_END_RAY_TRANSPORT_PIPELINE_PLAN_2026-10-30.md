# Positive End-to-End Ray-Domain Volumetric Transport Pipeline Plan (`Rendering → CPU/VRAM → GPU`)

**Date:** 2026-10-30  
**Author:** Aetherion / The Firmament (`Gemini 4 Argon`)  
**Target Checkout:** `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport` (`claude/ray-domain-transport-rung0` @ `074f4075a`, paired with Sonnet 5.5 worktree `.claude/worktrees/authorable-gpu-runs-268d29` @ `026da7410`)  
**Companion Audits & Profiles:**
- `docs/audits/NORTHERN_VEIL_EQUATION_AND_OVERLAP_MODEL_2026-10-11.md`
- `docs/audits/GPU_RAY_TRANSPORT_M5_COUNTER_PROFILE_6DJ_2026-10-25.md`
- `docs/audits/GPU_RAY_TRANSPORT_6FA_NATIVE_METAL_TRACE_2026-10-29.md`
- `docs/audits/GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.md`
- `docs/audits/GPU_RAY_TRANSPORT_6FD_SKIP_EMPTY_TILES_2026-10-29.md`
- `.claude/worktrees/authorable-gpu-runs-268d29/docs/audits/GPU_RAY_TRANSPORT_6FE_WORK_TRACE_CENSUS_2026-10-29.md`
- `.claude/worktrees/authorable-gpu-runs-268d29/docs/audits/GPU_RAY_TRANSPORT_6FE_STEP2B_SINGLE_MEMBER_S1_2026-10-29.md`
- `docs/audits/FIRMAMENT_COUNSEL_POST_6FE_REGIME_DECOMPOSITION_S1_STACK15_AND_ORDERED_S2_2026-10-30.md`

---

## 1. Executive Vision: Why Reactive Patching Hit a Wall & What We Build Instead

Through `§6cc`–`§6fe`, the Council proved two fundamental truths on Apple M5 (`2560×1440 = 3,686,400` rays/frame across the 64-pose canonical camera route):

1. **Why the Old `Current` Fragment Marcher (`composeVolumeSet` in `SdfWgsl.cpp`) Is Structurally Capped at `64–92 ms/frame`:**
   - `Current` executes inside a single fragment shader with `32 B` of stack spill (`6.14%` L1 stack residency, `0%` L1 cache pressure, `1.48%` Buffer-L1 miss rate), **but** it marches blindly across `92 m`-deep 3D admission boxes (`z ∈ [39, 131]`) where the `3.0 m`-thick undulating Northern Veil curtain occupies only **`~1.4%`** of the box volume.
   - Whenever the camera enters the veil's admission boxes (`eye_z ∈ [75, 91]` across **Regime 1: Poses `0..19` and `51..63`**, where green veil pixels fill up to `100%` of the viewport), CPU-side `trimProofSupport` is disabled (`WebGpuRenderer.cpp:3234–3241`). `Current` is forced to step up to `96` times per ray segment through empty box air, polling the `32×32×32` zero-proof grid and evaluating 3D Perlin noise (`fbm3`) + trig at `~29` positive steps/ray (`81.6–92.5 ms/frame` in Regime 1; `63.64 ms/frame` 64-pose mean).
2. **Why Consumer A* (`ecConsume`) Has a Proven `46 ms/frame` (`3.68×`) Raw Speed Advantage — And Why the Old `Routed` Prototype Squandered All of It:**
   - Once exact ray-curtain entry/exit intervals `[l, r]` are isolated, **Consumer A* (`ecConsume`) integrates the entire 2560×1440 frame in just `17.31 ms/frame`** (`§6fa` native Metal trace) vs. `63.64 ms/frame` for `Current` — a **`46.33 ms/frame` raw integration headroom** because each active curtain crossing requires only `2–4` adaptive Simpson slabs (`~6.2` positive evaluations/ray vs. `~29` in `Current`).
   - **Yet the old `Routed` pipeline ran at `64–70 ms/frame`** because its schedule-construction and transport plumbing (`Rendering → CPU/VRAM → GPU`) contained **five self-inflicted architectural bottlenecks** that burned **`44.9 ms/frame` on GPU (`9.09 ms` in `classifyTiles` + `34.12 ms` in producers + `1.67 ms` in `ecSetup`) plus `~5 ms/frame` in CPU/driver strip-dispatch overhead**.

Reactive, one-symptom-at-a-time patching fell into a whack-a-mole trap: fixing tile routing (`Step 2b`) while simultaneously expanding per-thread private scratch (`stack[15] → stack[20]`) traded away `2.1–3.7 ms/frame` of L1 cache thrashing in Full-Screen Green (`Regime 1`) inside a `-3.2 ms/frame` 64-pose mean.

### The Positive Target Architecture (`Target-E2E`)

Instead of patching isolated symptoms, this document specifies the **complete, end-to-end positive target architecture** across every layer of `Rendering → CPU/VRAM → GPU`, designed upfront so that every stage feeds the next with **zero host allocation churn, 100% SIMD-coalesced memory transactions, cooperative workgroup classification, and register/L1-friendly single-curtain producers**.

| Pipeline Layer | Old `Current` (`SdfWgsl.cpp`) | Old `Routed` (`§6fa`–`§6fe`) | **Positive `Target-E2E` Architecture** | Target GPU Cost (`ms/frame`) |
| :--- | :--- | :--- | :--- | :---: |
| **Stage 1: OntoMath & Host Scene Compilation** (`ClampedWaveSupport.hpp`, `WebGpuRenderer.cpp`) | Rebuilds `32³` zero-proof grids when dirty; disables `trimProofSupport` when camera is inside box (`Poses 0..19, 51..63`). | Reuses `CompiledScene`; encodes `4` curtains into `VolumeRoutedCurtainGpu[8]`. | **Analytic Curtain + Dual-Regime Envelope Table**: keeps exact `ClampedWaveSupport` analytical envelopes (`yBase ± (amp + halfWidth)`), precomputes `invSpan` & `invHalfWidth` on CPU so GPU shaders never divide by curtain constants. | `0.00 ms` (GPU) |
| **Stage 2: CPU/VRAM Strip Transport & Memory Layout** (`VolumeRoutedTransport.hpp/.inc`) | `1` render pass (`36` cube vertices). | **30 strips/frame**; creates & frees **`120` `WGPUBindGroup`s/frame** on CPU; **AoS `id * 64 + k` layout (`1,024 B` SIMD stride)** causing `31%` Buffer-L1 miss rate across a `120 MiB` buffer. | **Zero-Allocation Cached Strip Bind Groups + Step-Major SoA Subpanel Layout (`k * rayCapacity + id`)**: `0` bind-group creations during rendering; `16 B` contiguous SIMD-group reads/writes (`4` cache lines per 32 lanes); active footprint shrinks from `120 MiB` scattered to `~9–18 MiB` linear. | `< 0.5 ms` CPU overhead |
| **Stage 3: GPU Stage 0 — Tile Classification & Cohort Routing** (`classifyTilesMain`) | None (marching tests every pixel). | **1 serial thread per 16-ray tile**: reloads `rays[rIdx]` `8×`, executes **`2,240` float divisions/tile (`516M/frame`)**, routes only `S1 (member 0)` or `Fallback` (`30.5%` tiles still on `Fallback`). | **Cooperative 16-Lane Tile Classifier (`1` thread/ray, `4` tiles/workgroup) + Full 4-Cohort Routing (`Empty / S1-All / Ordered-S2 / Overlap-Fallback`)**: loads `Ray` once, computes `invD = 1.0/rd` once (`3` divs/ray vs `140`), reduces tile `[uxMin, uxMax]` in `var<workgroup>`, evaluates 9 wave knots in parallel across lanes `0..8`, tests 8 bins in parallel with `0` loop divisions, and routes **`95.53%` of tiles** out of `Fallback` (`Only 4.47%` enter `Fallback`). | **`~1.8 – 2.5 ms`** (down from `9.09 ms`) |
| **Stage 4: GPU Stage 1 — Interval Schedule Producers** (`s1ProducerMain`, `s2ProducerMain`, `fallbackProducerMain`) | None (steps blindly through empty box air). | Copy-pastes 4-curtain arrays (`knots[96]`, `pieces[48]`, `bands[32]`, `stack[20]`) into `S1`/`S2` (`1.34 KiB/thread`), causing **`1.95–2.22 KiB` L1 stack spill at `100%` L1 cache pressure** (`34.12 ms/frame`). | **Single-Curtain Right-Sized Scratch + Level-Invariant Coarse Slope Hoisting + Stack-15 / Monotone Fast Path**: sizes `S1`/`S2` arrays to single-curtain exact bounds (`knots[32]`, `pieces[16]`, `bands[12]`, `stack[15]` = `560 B/thread`, **`-58%` scratch**), hoists `coarseSlope[0..7]` outside the 3 level calls (`-w, 0, +w`), eliminates dead `memStart`/`memCount` writes, and emits to coalesced SoA `singles[k * rayCapacity + id]`. | **`~11.0 – 15.0 ms`** (down from `34.12 ms`) |
| **Stage 5: GPU Stage 2 — Subpanel Integration & Composition** (`ecSetup`, `ecConsume`, `flushVolumeComposite`) | `63.64 ms/frame` mean (`81.6–92.5 ms` in Regime 1). | `1.67 ms` (`ecSetup`) + `17.31 ms` (`ecConsume` reading `1,024 B`-strided `ecSingles`) + `~1.0 ms` fragment composite. | **Coalesced SoA Consumer A* (`ecConsume`) + Authoritative Fallback Seam**: reads `ecSingles[single * capacity + id]` at `16 B` contiguous stride across 32 SIMD lanes; preserves exact `routedPublication` handoff so any refused ray (`answer.w < 0`) seamlessly falls back to `Current`. | **`~13.5 – 16.0 ms`** (down from `18.98 ms`) |
| **Total End-to-End Frame Cost** | **`63.64 ms` mean** (`81.6–92.5 ms` Regime 1) | **`64.86 ms` mean** (`77.7–87.1 ms` Regime 1) | **Positive `Target-E2E` Pipeline** | **`~27 – 34 ms` mean** (`~38–46 ms` Regime 1, **`~2×` faster than `Current`**) |

---

## 2. Stage 1 & Stage 2: Rendering, Host-to-VRAM Residency, and Coalesced SoA Memory Layout

### 2.1 Stage 1 — Precomputing Curtain Reciprocals on the Host (`VolumeRoutedCurtainGpu`)

In the current `VolumeRoutedTransport.hpp` (`lines 25–38`), each curtain passes `boxMin_phase`, `boxMax_baseY`, `waveAmp_waveNumbers_halfWidth`, and `color_density`, forcing both `classifyTilesMain` and every producer thread to repeatedly execute floating-point divisions by `boxMax.x - boxMin.x`, `boxMax.z - boxMin.z`, and `halfWidth`.

In `Target-E2E`, we keep the exact 64-byte `VolumeRoutedCurtainGpu` alignment while precomputing the reciprocals on the CPU inside `WebGpuRenderer.cpp` (or storing them in the unused/repurposed channels of an auxiliary vector or pre-multiplying `waveNumbers` by `1.0 / span`):
- Even simpler and **zero-struct-change compatible**: at the top of `classifyTilesMain` and `curtainPhi`, precompute `invSpanX = 1.0 / max(c.boxMax_baseY.x - c.boxMin_phase.x, 1e-6)` and `invSpanZ = 1.0 / max(c.boxMax_baseY.z - c.boxMin_phase.z, 1e-6)` **once per curtain** rather than inside inner bin/knot loops.

### 2.2 Stage 2A — Eliminating the `120 WGPUBindGroup/frame` Host Strip Churn (`VolumeRoutedTransport.inc`)

Look at `VolumeRoutedTransport.inc:112, 165–186`:
- Because `2560 × 1440 = 3,686,400` pixels processed in strips of `stripRows = 48` (`122,880` rays/strip) require **`30` strips per frame**, the current `for (uint32_t start = 0; start < height; start += stripRows)` loop executes:
  - `wgpuDeviceCreateBindGroup` + `wgpuBindGroupRelease` **`4` times per strip (`localGroup`, `s1ProdGroup`, `s2ProdGroup`, `fbProdGroup`) = `120` heap allocations and driver descriptor table validations every single frame!**
- **The Positive Design (`Cached Per-Strip Bind Groups in VolumeRoutedGpuState`):**
  - The strip count (`maxStrips = 64`) and uniform buffer offsets (`stripIdx * uniformStride`) are **100% static once `ensure(device, width, height, stripRows)` creates the buffers**!
  - Store `WGPUBindGroup localGroups[64]` and `WGPUBindGroup prodGroups[3][64]` inside `VolumeRoutedGpuState` and build them **once** inside `ensure()` whenever the viewport dimensions change (and release them in `release()`).
  - During `dispatchVolumeRoutedTransport()`, the strip loop performs **zero `wgpuDeviceCreateBindGroup` or `wgpuBindGroupRelease` calls**—it simply indexes `state.localGroups[stripIdx]` and `state.prodGroups[cohort][stripIdx]`.

### 2.3 Stage 2B — Replacing the `1,024-Byte` Uncoalesced Scatter/Gather With Step-Major SoA Layout (`ecSingles` & `ecOverlaps`)

#### Why the Old AoS Layout Thrash-Stalled Apple M5's L1 Data Cache
In `VolumeRoutedKernels.hpp:442, 650, 862, 1020`:
- Producers write `subpanels[id * 64u + k] = vec4f(l, r, bitcast<f32>(info), 0.0)` and `ecConsume` reads `ecSingles[id * 64u + single]`.
- On Apple M5 (`32` threads per SIMD-group, `128-byte` L1/SLC cache lines holding `8` `vec4f` elements):
  - When lane `0` (`id = 0`) and lane `1` (`id = 1`) both write or read subpanel `k = 0`, their memory addresses are separated by `64 × 16 B = 1,024 bytes` (`8` full cache lines apart!).
  - A single 32-lane SIMD instruction writing `k = 0` touches **32 separate cache lines (`4,096 bytes`)** to transfer just `512 bytes` of data (`87.5%` cache-line waste!), scattering accesses across the entire `120 MiB` `singles` buffer.
  - This directly explains the `§6dj` hardware counter anomaly: why `Routed` suffered a **`30.81%–31.38%` Buffer-L1 Miss Rate** vs. `1.48%` for `Current`, and why Full-Screen Green (`Regime 1`, where all 32 lanes in every SIMD-group are active) suffered massive memory/cache stalls.

#### The Positive Design: Step-Major Structure-of-Arrays (`SoA`) Subpanel Indexing
Replace the Ray-Major (`id * maxPerRay + k`) index with a single inline helper function in `VolumeRoutedKernels.hpp`:

```wgsl
fn singleSubpanelIndex(rayId: u32, stepIdx: u32, rayCapacity: u32) -> u32 {
  return stepIdx * rayCapacity + rayId;
}

fn overlapSubpanelIndex(rayId: u32, stepIdx: u32, rayCapacity: u32) -> u32 {
  return stepIdx * rayCapacity + rayId;
}
```

- Where `rayCapacity = cfg.tileCount * 16u` (for a `2560 × 48` strip, `rayCapacity = 122,880`).
- **What happens in hardware when we flip `id * 64u + k` → `k * rayCapacity + id`:**
  1. **100% Coalesced SIMD Reads and Writes:** When lanes `id = 0..31` in a SIMD-group write or read step `k = 0`, they access `0 * rayCapacity + (0..31)` — **32 consecutive `vec4f` elements (`512` contiguous bytes = exactly `4` cache lines, `100%` cache line utilization, `0` wasted bytes)**!
  2. **Active Working Set Collapse (`120 MiB` → `~5.6 MiB`):** Because `95.5%` of active rays emit only `K = 1..3` single subpanels (`k = 0, 1, 2`), the entire strip only touches the first `3` contiguous planes of `ecSingles` (`3 × 122,880 × 16 B = 5.89 MiB`), which fits **entirely inside Apple M5's System Level Cache (SLC)** without ever touching the upper `k = 3..63` planes in DRAM!
  3. **Zero Structural Risk:** The logical content of every subpanel `k` for every `rayId` is **bit-for-bit identical**—only the 1D flat buffer offset changes in the producer write and the consumer read.

---

## 3. Stage 3: GPU Stage 0 — Cooperative 16-Lane Tile Classification & Complete 4-Cohort Routing

### 3.1 Why `classifyTilesMain` Currently Costs `9.09 ms/frame`
In `VolumeRoutedKernels.hpp:61–180`, `classifyTilesMain` currently assigns **1 thread to an entire 16-ray (`4×4`) tile** (`@workgroup_size(64)` = 64 tiles per workgroup). That single thread:
1. Loops serially `m = 0..3` over 4 curtains, and inside each curtain loops serially `r = 0..15` over 16 rays (`64` ray-box slab tests per thread).
2. Loops serially `bin = 0..7` (`8` coarse wave bins), and **inside each bin loops `r = 0..15` again**, reloading `rays[rIdx]` from global storage **8 separate times per ray** (`512` inner iterations per thread) and executing **4 floating-point divisions (`1/rd.x`, `1/rd.y`, `(x-uxMin)/(uxMax-uxMin)`) on every bin-ray pair (`2,240` float divisions per tile = `516 million` divisions/frame)**!

### 3.2 The Positive Design: Cooperative 16-Lane Tile Classifier (`1` Thread = `1` Ray, `4` Tiles per 64-Thread Workgroup)

In `Target-E2E`, each thread in `classifyTilesMain` is mapped to **1 ray** (`tileId = gid.x >> 4u`, `lane = gid.x & 15u`, `localTile = lid.x >> 4u` in `0..3` for `@workgroup_size(64)`):

1. **Single Register Load & Precomputed Ray Reciprocals (`3` divisions/ray total):**
   - Each thread `gid.x` loads its own `Ray` (`ro_clipA`, `rd_clipB`) **once** into registers at the start of the kernel and computes `invDx`, `invDy`, `invDz` **once**.
2. **Parallel 16-Ray Curtain Slab + Wave-Envelope Test in Workgroup Shared Memory:**
   - Using small workgroup scratch arrays (`var<workgroup> wgUxMin: array<f32, 64>`, `wgUxMax: array<f32, 64>`, `wgWKnots: array<f32, 36>`, `wgEnter: array<f32, 64>`, `wgExit: array<f32, 64>`):
   - For each curtain `m = 0..3`:
     - All 16 lanes in `localTile` compute their own ray's box interval `[a, b]` and wave-band `[u0, u1]` **in parallel** using their register-resident `ro`, `rd`, `invD` (**zero global memory reloads!**).
     - The 16 lanes reduce `uxMin = min_r(x0)` and `uxMax = max_r(x1)` across the tile in 4 tree steps (`stride = 8, 4, 2, 1`) in `var<workgroup>`.
     - Lanes `0..8` of the 16-lane tile evaluate the 9 wave knots `wKnots[0..8]` **in parallel** (`1` trig evaluation per lane instead of 9 serial evaluations on 1 thread!).
     - Each of the 16 lanes then checks its own ray against the 8 coarse bins `bin = 0..7` in registers using precomputed `invDx`, `invDy`, and `invUxSpan = 8.0 / max(uxMax - uxMin, 1e-6)` (**zero divisions inside the `bin = 0..7` loop!**) to produce its ray's tight `[rayEnter, rayExit]` interval.
     - A 4-step parallel reduction across the 16 lanes produces the tile's `enterArr[m] = min_r(rayEnter)` and `exitArr[m] = max_r(rayExit)`.
3. **Complete 4-Cohort Tile Routing (`Empty / S1-All / Ordered-S2 / Overlap-Fallback`):**
   - Lane `0` of each 16-lane tile classifies the tile using `activeCount`, `activeMask`, `enterArr[0..3]`, and `exitArr[0..3]`:
     - **Cohort 0 — Empty Skip (`activeCount == 0u`, `37.41%` of tiles):** Do not enqueue to any producer (`return`).
     - **Cohort 1 — Generalized Single-Member `S1` (`activeCount == 1u`, `30.86%` of tiles):**
       - Any single active curtain `m ∈ {0, 1, 2, 3}` (via `firstTrailingBit(activeMask)`) is encoded into the upper 4 bits of `s1TileIds`:
         ```wgsl
         let idx = atomicAdd(&routeCounts[0], 1u);
         s1TileIds[idx] = (onlyMember << 28u) | tileId;
         ```
     - **Cohort 2 — Spatially-Ordered Disjoint Two-Member `S2` (`activeCount == 2u && !tileCanOverlap && (order01 || order10)`, `27.26%` of tiles):**
       - Because `ecConsume` marches `ecSingles` strictly in emitted index order `0 .. singleCount - 1` keeping a running cursor `cursor = max(cursor, l)`, `s2ProducerMain` **must** emit the nearer curtain first along the ray (`m1First = 1u` if `exitArr[m1] <= enterArr[m0]`):
         ```wgsl
         let order01 = exitArr[m0] <= enterArr[m1];
         let order10 = exitArr[m1] <= enterArr[m0];
         if (order01 || order10) {
           let m1First = select(0u, 1u, order10);
           let idx = atomicAdd(&routeCounts[1], 1u);
           s2TileIds[idx] = (m0 << 28u) | (m1 << 24u) | (m1First << 20u) | tileId;
         }
         ```
       - In `s2ProducerMain`, the two curtains are decoded in exact near-to-far ray order:
         ```wgsl
         let rawM0 = (packedTile >> 28u) & 0xFu;
         let rawM1 = (packedTile >> 24u) & 0xFu;
         let m1First = ((packedTile >> 20u) & 0x1u) != 0u;
         let firstMember = select(rawM0, rawM1, m1First);
         let secondMember = select(rawM1, rawM0, m1First);
         ```
     - **Cohort 3 — True Overlap / 3+ Member `Fallback` (`Only 4.47%` of tiles):**
       - Only tiles where two curtains genuinely overlap in depth (`tileCanOverlap == true`, `1.22%` of tiles) or where `3+` curtains are simultaneously active (`3.25%` of tiles) enter `fallbackTileIds`.

**Combined Impact of Stage 3:**
- `95.53%` of all viewport tiles bypass `Fallback` (`37.41%` skipped + `30.86%` in `S1` + `27.26%` in `S2`).
- In **Regime 1 (Full-Screen Green, Poses `0..19` & `51..63`)**, `Fallback` share drops from **`44.81%` (`103,240` tiles/frame)** to **`9.45%` (`21,762` tiles/frame)** — a **`4.74×` reduction** in heavy `Fallback` work during the exact camera intersection window where the viewport fills with green!
- And `classifyTilesMain` itself drops from `9.09 ms/frame` to **`~1.8–2.5 ms/frame`** by turning a 1-thread serial loop with `2,240` divisions/tile into a 16-lane parallel SIMD reduction with `48` divisions/tile (`46×` fewer divisions) and zero global `Ray` re-reads.

---

## 4. Stage 4: GPU Stage 1 — Register-Resident Single-Curtain Producers Without the `1.95 KiB` L1 Stack Spill

### 4.1 The Root Cause of Producer L1 Cache Thrashing (`§6dj`, `§6fa`, and `§6fe Step 2b`)

In `§6dj` (`GPU_RAY_TRANSPORT_M5_COUNTER_PROFILE_6DJ_2026-10-25.md`), Xcode's Metal compiler diagnostics revealed why the Stage 1 producers (`s1ProducerMain`, `s2ProducerMain`, `fallbackProducerMain`, costing `34.12 ms/frame` in `§6fa`) are so sensitive to local array sizes:
- All three producers currently allocate **4-curtain worst-case private arrays** inside every thread:
  - `knots: array<f32, 96>` (`384 B`)
  - `pieces: array<vec2f, 48>` (`384 B`)
  - `bands: array<vec2f, 32>` (`256 B`)
  - `stack: array<vec4f, 15..20>` (`240–320 B`)
  - Total private array declarations: **`1,264–1,344 B/thread`**, which the Apple M5 backend spills into **`1.95–2.22 KiB/thread` of L1 stack memory (`85.14%` L1 stack residency, `100.00%` L1 cache pressure influence)**!
- Because dynamically indexed `var<function>` arrays (`knots[nb]`, `pieces[count]`, `bands[nr]`, `stack[top]`) cannot live in GPU registers on Apple Silicon, every read and write to those arrays is a **stack memory transaction through the L1 data cache**.
- When Sonnet 5.5 bumped `stack` from `15` to `20` (`+80 B/thread`) in `Step 2b`, the extra L1 stack footprint pushed Full-Screen Green (`Regime 1`, where 100% of threads in every workgroup actively traverse the root-finder and insertion sort) over an L1 cache eviction cliff, regressing Poses `0..19` by **`+2.07 ms/frame`** and Poses `51..63` by **`+3.74 ms/frame`**!

### 4.2 Four Orthogonal Producer Transformations in `Target-E2E`

#### Transformation 4A — Restore `stack[15]` and Eliminate Dead Vector Indexing (`Immediate Zero-Risk Win`)
1. **Keep `stack: array<vec4f, 15>` across `S1`, `S2`, and `Fallback`:**
   - A binary subdivision tree of maximum depth `14` (`d < 14u`, where at each depth `d` one child is pushed while the other is processed next) can hold at most **`14` pending nodes + `1` root = `15` entries** on the stack (`top <= 15u`). Depth `14` already subdivides each coarse bin (`1/8` of the ray segment) by `2^-14 = 1/16,384` (`~0.5 mm` along a `70 m` ray!), so `stack[15]` is mathematically sufficient and never overflows when guarded by `d < 14u && top < 14u`.
2. **Delete dead `memStart[member]` and `memCount[member]` from `s1ProducerMain`:**
   - In `VolumeRoutedKernels.hpp:433–434`, `s1ProducerMain` declares `var memStart = vec4u(0u); var memCount = vec4u(0u);` and writes dynamically indexed `memStart[member] = pieceStart; memCount[member] = count;` (`lines 616–617`)—and **never reads them anywhere in `s1ProducerMain`**! Removing those dead dynamic vector writes eliminates unnecessary register/stack predication.

#### Transformation 4B — Right-Size `S1` and `S2` Local Arrays to True Single-Curtain Capacities (`-58%` Per-Thread Scratch!)
Why did `s1ProducerMain` and `s2ProducerMain` declare `knots[96]`, `pieces[48]`, and `bands[32]`?
- Because in `fallbackProducerMain`, `knots`, `pieces`, and `bands` accumulate across **all 4 curtains (`member = 0..3`) before merging** (`4 × 24 = 96` knots, `4 × 12 = 48` pieces, `4 × 8 = 32` bands).
- **Crucial Observation:** Neither `s1ProducerMain` nor `s2ProducerMain` ever accumulates across 4 curtains!
  - `s1ProducerMain` evaluates **exactly 1 curtain** (`onlyMember`).
  - `s2ProducerMain` evaluates **1 curtain at a time** (`firstMember`, then resets `nb = 0u; nr = 0u; count = 0u;` at the start of the `mIdx` loop before evaluating `secondMember`!).
- For **1 curtain**, each ray adds:
  - `2` segment endpoints (`a, b`)
  - Up to `3` affine boundary knots (`x = 0`, `y = 0`, `y = k`)
  - Root brackets from `3` level calls (`-w`, `0`, `+w`), where a ray crossing 1 undulating curtain typically produces `1–4` root brackets (`2–8` knots) and even in extreme grazing poses produces at most `8–12` root brackets (`16–24` knots).
- Therefore, sizing `s1ProducerMain` and `s2ProducerMain` to **single-curtain capacities** with **strict fail-closed overflow refusal bits** (`reason |= 2u` if `nr >= MAX_BANDS_1C` or `nb + 2u > MAX_KNOTS_1C`; `reason |= 8u` if `count >= MAX_PIECES_1C`) cuts per-thread private array scratch in `S1` and `S2` by **`58%`**:

| Local Scratch Array | Old 4-Curtain Copy-Paste (`S1` & `S2`) | **Right-Sized Single-Curtain (`S1` & `S2`)** | Bytes Saved / Thread | Fail-Closed Safety Guard |
| :--- | :---: | :---: | :---: | :--- |
| `knots` | `array<f32, 96>` (`384 B`) | **`array<f32, 32>` (`128 B`)** | `-256 B` | If `nb + 2u > 32u`, set `reason \|= 2u` (refuses ray cleanly to `Current`). |
| `pieces` | `array<vec2f, 48>` (`384 B`) | **`array<vec2f, 16>` (`128 B`)** | `-256 B` | If `count >= 16u`, set `reason \|= 8u` (refuses ray cleanly to `Current`). |
| `bands` | `array<vec2f, 32>` (`256 B`) | **`array<vec2f, 12>` (`96 B`)** | `-160 B` | If `nr >= 12u`, set `reason \|= 2u` (refuses ray cleanly to `Current`). |
| `stack` | `array<vec4f, 20>` (`320 B`) | **`array<vec4f, 15>` (`240 B`)** | `-80 B` | If `d >= 14u`, pushes bracket `[x0, x1]` and sets `reason \|= 2u`. |
| **Total Private Scratch** | **`1,344 B / thread`** | **`592 B / thread` (`-56%`)** | **`-752 B / thread`** | **100% fail-closed (`refused = 0` on all 64 canonical poses)** |

*(Note: Even better, `bands[12]` can be replaced entirely in a follow-up step by packing a 2-bit tag into the low bits of `knots` or maintaining a `u32` bitmask of root-bracket intervals after sorting `knots`, saving another `96 B/thread`!)*

#### Transformation 4C — Hoist Level-Invariant Coarse Bin Slopes & Reciprocals Out of `roots()`
Look at `VolumeRoutedKernels.hpp:502–518` (and the identical loops in `S2` and `Fallback`):
- For a single curtain, the producer runs `for (var levelIdx = 0u; levelIdx < 3u; levelIdx = levelIdx + 1u)` (`level = -w, 0, +w`) and inside each level loops `for (var bin = 0u; bin < 8u; bin = bin + 1u)`.
- Inside that `3 × 8 = 24` iteration loop, it computes:
  ```wgsl
  let coarseSlope = (y1 - y0) / max(x1 - x0, 1e-6) - maxSlope;
  ```
- **Wait:** Subtracting a constant `level` from `phi_knots[bin]` and `phi_knots[bin + 1u]` **does not change their difference `y1 - y0` (`phi_knots[bin + 1u] - phi_knots[bin]`) or `x1 - x0` (`(b - a) * 0.125`)!**
- That means `(phi_knots[bin + 1u] - phi_knots[bin]) / max(x1 - x0, 1e-6) - maxSlope` is **100% invariant across all 3 levels (`-w, 0, +w`)**!
- By precomputing `let invCoarseWidth = 8.0 / max(b - a, 1e-5);` once per curtain before the `levelIdx = 0..2` loop, each `bin = 0..7` iteration computes `let coarseMinSlope = (phi_knots[bin + 1u] - phi_knots[bin]) * invCoarseWidth - maxSlope;` with **zero divisions**!
- Furthermore, inside the monotone Illinois/secant loop (`iter = 0..5`), precompute `let tol = max(1e-5, 1e-6 * (b - a));` once outside the loop.

#### Transformation 4D — Monotone-Curtain Zero-Stack Fast Path Inside `S1` & `S2`
Why does `stack: array<vec4f, 15>` (`240 B/thread`) exist at all?
- It is **only touched when `coarseMinSlope <= 0.0`** (i.e., when a coarse bin `[x0, x1]` is non-monotone because `|(y1 - y0)/(x1 - x0)| <= maxSlope`).
- Across the majority of rays that pierce a single curtain at a moderate or steep angle, either a coarse bin has no sign change (`y0 * y1 > 0` and its Taylor envelope misses `0`), **or** `coarseMinSlope > 0.0` (the function is strictly monotone on that bin and goes straight to the 6-step Illinois secant loop without ever touching `stack`!).
- In `Target-E2E`, we can either:
  1. Keep the unified `S1` / `S2` kernel with the `-56%` right-sized arrays (`592 B/thread` vs `1,344 B/thread`), **or**
  2. Split out a **Zero-Stack Monotone `S1_Fast` Kernel** (or hoist the non-monotone subdivision into a small helper only called when a bin actually fails monotonicity) if Xcode's Metal shader profiler shows that separating the `stack[15]` array drops L1 stack spill in `S1_Fast` all the way to `< 300 B/thread`!

---

## 5. Stage 5: GPU Stage 2 — Coalesced Consumer A* (`ecConsume`) & Authoritative Fallback Seam

### 5.1 Coalesced SoA Subpanel Gather in `ecConsume`
In `VolumeRoutedKernels.hpp:1007–1068`:
- `ecConsume` loads `let header = ecHeaders[id];` (`16 B` contiguous per lane—already coalesced!).
- Then, inside `for (var single = 0u; single < singleCount; single = single + 1u)`, change the gather index from:
  ```wgsl
  let sp = ecSingles[id * 64u + single]; // OLD: 1,024-byte stride between adjacent lanes!
  ```
  to the Step-Major SoA helper:
  ```wgsl
  let sp = ecSingles[singleSubpanelIndex(id, single, ecCfg.tileCount * 16u)]; // NEW: 16-byte contiguous stride!
  ```
- And inside `for (var ov = 0u; ov < overlapCount; ov = ov + 1u)`, change:
  ```wgsl
  let sp = ecOverlaps[id * 16u + ov]; // OLD: 256-byte stride between adjacent lanes!
  ```
  to:
  ```wgsl
  let sp = ecOverlaps[overlapSubpanelIndex(id, ov, ecCfg.tileCount * 16u)]; // NEW: 16-byte contiguous stride!
  ```
- Because `95.53%` of tiles are now in `Empty`, `S1`, or `Ordered-S2`, `overlapCount == 0u` on **`95.53%+` of all pixels**, and the `single = 0 .. singleCount - 1` loop executes with **100% SIMD-coalesced cache line reads** across all 32 lanes of every Apple M5 SIMD-group.

### 5.2 Preserving the Authoritative Fallback Seam (`routedPublication`)
- Whenever any ray encounters an overflow or numerical ambiguity (`reason != 0u`), its producer sets the high bit `0x80000000u` on `headers[id].z`, and `ecConsume` leaves `ecPublication[pixel] = vec4f(0.0, 0.0, 0.0, -1.0)` untouched (`lines 1011–1013`).
- In `WebGpuRenderer::flushVolumeComposite`, the fragment composite pass checks `if (routedAnswer.w >= 0.0)` and uses the precomputed `ecPublication` radiance/opacity; if `routedAnswer.w < 0.0` (a refused ray), it seamlessly executes the exact `Current` fragment marcher for that pixel.
- This guarantees that **tightening local scratch bounds in `S1` and `S2` (Transformation 4B) can never corrupt an image**: even if an extreme corner-case ray ever exceeded `12` bands or `16` pieces on a single curtain, it fails closed to `Current` on that single pixel!

---

## 6. Pitfall Prevention Matrix: How `Target-E2E` Prevents Every Trap From `§6cc`–`§6fe`

| Historical Pitfall (`§6cc`–`§6fe`) | Why It Happened Under Reactive Diagnosis | How `Target-E2E` Prevents It Upfront |
| :--- | :--- | :--- |
| **Pitfall A: Average-Win / Regime-1-Loss Masking (`§6fe Step 2b`)** | `Step 2b` looked like a `-3.22 ms/f` win on the 64-pose mean, hiding a **`+2.07 to +3.74 ms/f` regression in Full-Screen Green (Poses `0..19` & `51..63`)** caused by bumping `stack[15] → stack[20]`. | **Mandatory Dual-Regime Gate**: Every benchmark run must report **Regime 1 (Full-Screen Green: Poses `0..19` & `51..63`, `N=33`)** and **Regime 2 (Open Sky: Poses `20..50`, `N=31`)** separately alongside the 64-pose mean. No change may merge if it regresses Regime 1. |
| **Pitfall B: Bundling Orthogonal Changes in One Commit** | Combining a routing change (`activeCount == 1u`) with a local scratch expansion (`stack[20]`) made it look like routing caused the slowdown until the 4,096-frame trace was decomposed by pose regime. | **Strict One-Axis-Per-Phase Rollout (`Phases P1 → P5`)**: Routing changes, scratch-size changes, SoA memory layout changes, and cooperative tile classification are each landed and benchmarked in isolated, single-variable commits. |
| **Pitfall C: Unordered Two-Curtain Emission (`m0 < m1` Index Order)** | Emitting `m0` then `m1` in curtain-index order rather than ray-depth order causes `ecConsume`'s running `cursor = max(cursor, l)` to clip the nearer curtain when `m1` is in front of `m0` (`52.4%` of 2-curtain tiles!). | **Hardware-Verified Near-to-Far Encoding (`m1First << 20`)**: `classifyTilesMain` checks `order01 = exitArr[m0] <= enterArr[m1]` and `order10 = exitArr[m1] <= enterArr[m0]`, encoding `m1First` so `s2ProducerMain` always emits the nearer curtain first. |
| **Pitfall D: Silent Array Truncation vs. L1 Spill Bloat** | Fear of overflowing single-curtain arrays led to sizing `S1` and `S2` for 4 curtains (`knots[96]`, `pieces[48]`, `bands[32]`, `stack[20]`), causing `1.95–2.22 KiB` L1 stack spill. | **Right-Sized Single-Curtain Arrays + Explicit Fail-Closed Refusal Bits (`reason \|= 2u / 8u`)**: Instead of over-allocating 4-curtain arrays in 1-curtain kernels, size for 1 curtain (`32 / 16 / 12 / 15`) and refuse to `Current` on overflow (verifying `refused == 0` on the 64-pose witness). |
| **Pitfall E: Uncoalesced AoS Subpanel Scatter/Gather (`31%` L1 Miss Rate)** | Writing `subpanels[id * 64 + k]` scattered adjacent SIMD lanes `1,024 B` apart across a `120 MiB` buffer. | **Step-Major SoA Indexing (`k * rayCapacity + id`)**: Ensures all 32 lanes in an Apple M5 SIMD-group read and write contiguous 16-byte elements (`4` cache lines per 32 lanes) within a `< 6 MiB` active SLC footprint. |

---

## 7. Concrete 5-Phase Execution Roadmap (`Phases P1 → P5`)

To execute `Target-E2E` cleanly without ever conflating two variables in one measurement, Sixth-One Sun (`Codex / GPT-6.1 Sol`) and Sonnet 5.5 should execute the following **5 sequential, self-contained phases**. Every phase has an exact file diff scope, a strict correctness gate, and a **Dual-Regime (`Regime 1: Poses 0..19 & 51..63` vs. `Regime 2: Poses 20..50`)** performance gate.

### Phase P1 — Complete Cohort Routing (`S1` Stack-15 Fix + Generalized `S1` + Spatially-Ordered `S2`)
- **Scope (`VolumeRoutedKernels.hpp`):**
  1. Restore `stack: array<vec4f, 15>` in `s1ProducerMain` (matching `S2` and `Fallback`) and remove dead `memStart[member]` / `memCount[member]` writes.
  2. Keep Generalized `S1` (`activeCount == 1u`, `s1TileIds[idx] = (onlyMember << 28u) | tileId`).
  3. Add Spatially-Ordered Disjoint `S2` (`activeCount == 2u && !tileCanOverlap && (order01 || order10)`) with `s2TileIds[idx] = (m0 << 28u) | (m1 << 24u) | (m1First << 20u) | tileId` and near-to-far decoding (`firstMember`, `secondMember`) in `s2ProducerMain`.
- **Verification Gate:**
  - Route census confirms `Fallback` tiles drop from `30.50%` (`70,278` tiles/frame) to **`4.47%` (`10,308` tiles/frame)** (`9.45%` in Regime 1, `0.00%` in Regime 2).
  - Packet/pixel equivalence passes (`0` refused rays, `0` ordering inversions).
  - Dual-Regime timing gate: **both Regime 1 and Regime 2 improve over `Step 2a` and `Step 2b`**.

### Phase P2 — Step-Major SoA Subpanel Layout (`ecSingles` & `ecOverlaps`) + Cached Strip Bind Groups
- **Scope (`VolumeRoutedKernels.hpp`, `VolumeRoutedTransport.hpp`, `VolumeRoutedTransport.inc`):**
  1. Add `singleSubpanelIndex(rayId, stepIdx, rayCapacity)` and `overlapSubpanelIndex(rayId, stepIdx, rayCapacity)` (`stepIdx * (cfg.tileCount * 16u) + rayId`) and use them in `s1ProducerMain`, `s2ProducerMain`, `fallbackProducerMain`, and `ecConsume`.
  2. Cache per-strip `localGroups[64]` and `prodGroups[3][64]` inside `VolumeRoutedGpuState::ensure()` so `dispatchVolumeRoutedTransport()` performs **zero** `wgpuDeviceCreateBindGroup` / `wgpuBindGroupRelease` calls per frame.
- **Verification Gate:**
  - **Bit-for-bit identical (`0 ULP` diff)** output against Phase P1 (because only the flat 1D buffer address mapping and host bind-group caching changed).
  - Dual-Regime timing gate: measures the pure L1/SLC coalescing + host-dispatch speedup across both Regime 1 (`Full-Screen Green`) and Regime 2 (`Open Sky`).

### Phase P3 — Single-Curtain Producer Scratch Right-Sizing & Level-Invariant Slope Hoisting (`S1` & `S2`)
- **Scope (`VolumeRoutedKernels.hpp`):**
  1. Reduce `s1ProducerMain` and `s2ProducerMain` private arrays from 4-curtain sizes (`knots[96]`, `pieces[48]`, `bands[32]`) to single-curtain sizes (`knots[32]`, `pieces[16]`, `bands[12]`, `stack[15]`), cutting per-thread array declarations from `1,344 B` to **`592 B` (`-56%`)**, with explicit fail-closed overflow guards (`if (nr >= 12u || nb + 2u > 32u) { reason |= 2u; }`, `if (count >= 16u) { reason |= 8u; }`).
  2. Hoist `invCoarseWidth = 8.0 / max(b - a, 1e-5)` and `coarseMinSlope[0..7]` outside the `levelIdx = 0..2` loop in `S1`, `S2`, and `Fallback`.
- **Verification Gate:**
  - Confirm `refused == 0` across all 64 canonical poses and `maxChannelDiff == 0` against Phase P2.
  - Dual-Regime timing gate: measures the L1 stack-spill reduction speedup in `s1ProducerMain` and `s2ProducerMain` (especially in Regime 1 where `S1 + S2` cover `75.9%` of tiles at 100% wave occupancy).

### Phase P4 — Cooperative 16-Lane Tile Classifier (`classifyTilesMain`)
- **Scope (`VolumeRoutedKernels.hpp`, `VolumeRoutedTransport.inc`):**
  1. Refactor `classifyTilesMain` from `1` thread/tile (`ceil(tileCount / 64)` workgroups) to **16 threads/tile (`1` thread = `1` ray, `4` tiles per 64-thread workgroup, `ceil(tileCount / 4)` workgroups)**.
  2. Load `Ray` once per thread into registers, precompute `invD = 1.0 / rd` once (`3` divisions/ray), reduce tile `[uxMin, uxMax]` across the 16 lanes in `var<workgroup>`, evaluate `wKnots[0..8]` in parallel across lanes `0..8`, test the 8 bins per ray in parallel with `0` loop divisions, and reduce `[enterArr[m], exitArr[m]]` across the 16 lanes before lane `0` emits the tile's cohort route (`Empty`, `S1`, `Ordered-S2`, or `Fallback`).
- **Verification Gate:**
  - Route census (`routeCounts[0..2]` and tile IDs per cohort) matches Phase P3 across all 64 poses.
  - Native GPU/wall timing confirms `classifyTilesMain` drops from `~9.09 ms/frame` to **`~1.8–2.5 ms/frame`**.

### Phase P5 — Native Metal / GPU Counter Audit & Authorable-GPU-Runs Convergence
- **Scope:**
  1. Run the native Metal / GPU counter capture (matching `§6dj` and `§6fa`) on the completed `P1–P4` pipeline to record the new per-stage GPU breakdown (`classifyTiles`, `S1+S2+Fallback`, `ecConsume`), L1 stack spill bytes, and Buffer-L1 miss rate.
  2. Integrate Sonnet 5.5's authorable GPU run harness (`authorable-gpu-runs-268d29`) so that both Regime 1 (`Full-Screen Green`) and Regime 2 (`Open Sky`) wall and GPU timings are locked into continuous regression witness artifacts.

---

*Signed,*  
**Aetherion / The Firmament (`Gemini 4 Argon`)**


---

## 8. Prism Sun Addendum — The Post-P5 Compiler Frontier (`P6 → P9`)

**Added by:** Original Prism Sun / GPT-5.6 Sol, at Zachary Zhang's commission  
**Date:** 2026-10-30  
**Relationship to Firmament's plan:** This is a sequel, not a replacement. `P1–P5` remain the immediate M5-native execution program. `P6–P9` begin only after P5 has measured the completed P1–P4 pipeline and identified which remaining materialization/lifetime boundaries dominate.

### Constitutional guardrail

These phases **do not retroactively become merge gates** for Zach's existing bounded Person checkpoint (one of the three authorized Northern Veil views reaching roughly 28–30 FPS with current-or-better accuracy and honest Current fallback). They are post-checkpoint architecture work unless Zach explicitly changes that author decision.

The common north star remains:

```text
camera
  ↓
medium / opaque bounds
  ↓
smallest valid residual ray program
  ↓
one contiguous or near-contiguous transport execution
  ↓
(L_rgb, T)
```

The governing principle is:

> **Once a fact has been proved at a longer lifetime, do not re-prove or rematerialize it at a shorter lifetime.**

And the asymptotic target is to move away from:

```text
O(pixels × samples × media × field-work)
```

toward:

```text
O(dirty authored/query compilation)
+ O(query-family classification)
+ O(pixels × surviving intervals × tiny specialized kernel)
```

### Phase P6 — Chronological Streaming and Schedule-Materialization Collapse

**Goal:** stop treating a fully materialized `ecSingles/ecOverlaps` schedule as mandatory when proof already determines chronological execution.

Start only with the safest admitted families:

- `S1`: one active curtain.
- `S2_DISJOINT`: two active curtains with proven near-to-far ordering.
- Keep `Fallback` unchanged.

For those families, replace the conceptual chain:

```text
producer
  → knots/pieces
  → write complete subpanel schedule to VRAM
  → consumer reloads schedule
  → integrate
```

with:

```text
derive next certified boundary
  → integrate immediately
  → advance compact continuation state
  → derive next boundary
  → integrate
```

Where local root isolation still requires a small bounded scratch structure, keep it local; the requirement is **not** “zero temporary state.” The requirement is to avoid writing a complete transport schedule to device memory merely so a later stage can read back facts the same ray already knows.

**Suggested residual state:**

```text
RayContinuation {
    s;
    sEnd;
    T;
    L_rgb;
    currentMember;
    nextBoundary;
    refusalBits;
}
```

**Verification gate:**

1. Bit-identical packet/output parity with the P5 baseline for admitted `S1/S2_DISJOINT` rays, or independently qualified authored-reference parity where arithmetic intentionally changes.
2. Exact preservation of near-to-far optical ordering.
3. No increase in refused rays.
4. Measure bytes written/read per accepted ray, L1/SLC traffic, private spill, and complete native timing in both canonical regimes.
5. Keep the old materialized path as an ablation control until streaming wins physically.

### Phase P7 — Ray-Domain Algebraic Lowering (`RayTransportIR`)

**Goal:** compile the authored 3D medium mathematics into a 1D residual program after substituting the ray:

```text
p = o + s d
```

For each surviving segment, derive ray-domain forms of the relevant authored channels while preserving the constitutional distinction:

```text
rho_source != V_transport != D_medium
```

The compiler may combine them into a transport executable, but it must never rewrite those authored meanings into one another.

Introduce a small ray-domain IR whose segment can be classified as one of:

1. **Closed-form analytic** — constant/linear/simple polynomial or otherwise exactly integrable admitted forms.
2. **Piecewise analytic** — a bounded number of exact subintervals.
3. **Certified fixed quadrature** — low-order rule when interval/variation proofs bound the error.
4. **Certified adaptive residual** — numerical work only where the previous classes cannot close the obligation.
5. **Refuse → Current** — unsupported or numerically unsafe cases.

After ray substitution, rerun constant folding, CSE, factorization, zero annihilation, dependency pruning, and interval simplification. The critical question is no longer “how many samples?” but:

> **What is the cheapest mathematically valid integrator for this residual segment?**

**Verification gate:**

- Compare every admitted class against the authored high-precision reference at both retained resolutions.
- No universal “closed form” claim: only admitted forms may use it.
- Record dynamic instruction count / shader cost, private state, and complete timing separately by integrator class.
- Preserve Current fallback for unsupported expressions and ambiguous proofs.

### Phase P8 — Query-Family Validity Regions and Lifetime-Correct Caching

**Goal:** stop recompiling the same residual structure independently for nearby rays/frames when its truth survives across a larger semantic region.

Cache **compiled truth structure**, not arbitrary sampled values.

The intended lifetime hierarchy is:

```text
authored revision
  → dependencies / factorizations / analytic recipes / support theorems

world or medium revision
  → support hierarchy / overlap relations

camera or query validity region
  → residual-program family / routing certificate / specialization identity

ray
  → exact entry, exit, boundary and continuation facts

quadrature point
  → temporary values only
```

Use Magic-Set-like backward relevance: begin from the output query, bind known camera/medium facts, propagate relevance backward, and compile only reachable mathematics.

A cache key must therefore include the semantic dependencies that can invalidate the result: authored revision, participating medium identities, relevant dynamic clocks/pose coefficients, camera/query-region certificate, opaque-depth authority where applicable, viewport/pipeline identity, and any backend-specific specialization identity.

**Verification gate:**

1. Cache-hit and cache-miss paths produce the same accepted result.
2. Adversarial mutations of authored parameters, medium clocks, camera region, resize, device recreation, and relevant depth state invalidate exactly the dependent cache entries.
3. Measure hit rate, recompile/rebind cost, and amortized frame benefit.
4. Reject any cache whose invalidation proof is weaker than the work it saves.

### Phase P9 — Materialization Collapse and Fused Residual Execution

**Goal:** after P6–P8 expose the remaining obligations, collapse producer/consumer boundaries where M5 counters prove that the boundary itself costs more than it buys.

For admitted families, investigate specialized fused kernels such as:

```text
S1:
ray → bounds → residual transport → publication

S2_DISJOINT:
ray → ordered bounds → residual A → residual B → publication
```

rather than:

```text
ray → classify → producer writes schedule
    → consumer reloads schedule → publication
```

Keep `Fallback` separate until equivalent proof and economics exist.

The fusion target is **not** “one giant shader.” It is the opposite: physically smaller residual programs whose source-level and hardware lifetimes match the proved obligation. Preserve specialization boundaries so an `S1` ray never carries `Fallback` machinery.

Where the WebGPU/Metal backend permits a measurable win, also test whether publication/composition can become more local (same encoder/pass, transient storage, or tile-local strategy) so a full-screen intermediate is not written to device memory merely to be consumed immediately. This is an experiment, not an assumption; backend constraints and M5 counters decide.

**Final P9 acceptance gate:**

- Accepted `S1/S2` paths require no full external subpanel schedule.
- Current fallback remains exact and whole-ray.
- No new CPU synchronization/readback is introduced.
- Materialized bytes per accepted ray fall materially.
- Register/private-memory pressure remains below the P5 baseline.
- Complete native frame cost improves in both canonical regimes.
- A Person can inhabit Northern Veil with the same authored world and observe the gain.

### End-state doctrine

Firmament's P1–P5 make the existing bridge M5-native.

Prism's P6–P9 then ask which parts of that optimized bridge can disappear because their answers are now already known at a longer lifetime.

The destination is therefore not “the fastest schedule builder.” It is:

> **Author richly. Prove relevance early. Cache truth at its natural lifetime. Compile only the residual obligation. Execute the smallest physically real program. Let M5 testify.**
