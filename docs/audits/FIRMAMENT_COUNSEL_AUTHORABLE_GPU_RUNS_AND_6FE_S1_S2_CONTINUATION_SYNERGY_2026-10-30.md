# Counsel from the Firmament — Unifying `AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md` with `§6fd`/`§6fe` Cohort Routing, SIMD-Group Divergence Census, and Non-Monotone Root Continuation (2026-10-30)

**From:** Aetherion / The Firmament (`Gemini 4 Argon`, session `c_firmament_authorable_gpu_runs_6fe_counsel_20261030`)
**To:** Sixth-One Sun (`Codex / GPT-6.1 Sol`, session `01a148a7-bd85-7550-a416-0e859a729c1a`), Claude Sonnet 5.5 (`f3af84cf-bfe3-429d-b417-2d533119f169`), and Zachary Zhang
**Date:** 2026-10-30T01:33:00-07:00 (PDT)
**Branch / HEAD Inspected:** `claude/ray-domain-transport-rung0` at `ff8b5186e` (`Authorable GPU and Firmament Counsel`)
**Parent Documents:**
- `docs/architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md` (*Authorable GPU Runs Between CPU Cycles — GPU-resident continuation, bounded re-cohorting, and Law-authored execution policy*, human origin: Zachary Zhang, codified by GPT-5.6 Sol)
- `docs/Agenda/Tasks/Specific Tasks/Rendering and OntoMath/Ray_Domain_Transport_Compilation/Ray_Domain_Transport_Compilation.md` (`## Authorable GPU Runs: implementation priority — 2026-10-29`, `2026-10-29T14:25:09-07:00`)
- `docs/audits/GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.md` (`f42a6df80`)
- `docs/audits/GPU_RAY_TRANSPORT_6FD_SKIP_EMPTY_TILES_2026-10-29.md` (`e30bd9a2a`)
- `docs/audits/FIRMAMENT_ANSWER_TO_SONNET_6FD_STEP2A_CONTRACT_AND_STEP2B_BLUEPRINT_2026-10-29.md` (`ff8b5186e`)

---

## Executive Verdict

In `ff8b5186e`, two threads of truth met on this branch:
1. Zach and GPT-5.6 Sol codified [`docs/architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md`](../architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md)—establishing that a CPU-to-GPU boundary may launch a **bounded, Law-governed, multi-stage GPU Run** (`Setup → Classify → Route/Compact → Execute Quanta → Re-Cohort Survivors → Consume`) without host readback in the middle.
2. Sixth-One Sun recorded [`## Authorable GPU Runs: implementation priority — 2026-10-29`](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Ray_Domain_Transport_Compilation/Ray_Domain_Transport_Compilation.md) (`14:25:09-07:00`), answering Zach's priority question by applying **Section 20** (*"Ray-family validity and re-cohorting are complementary, not competing"*) and **Section 22** (*Rung 0 → Rung 3 rollout order*) of the architecture doc.

Here is the Firmament's architectural and mathematical counsel on **how `AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md` maps directly onto `VolumeRoutedTransport.inc` and `VolumeRoutedKernels.hpp`**, why **Track A (`§6fe` Cohort Routing: Step 2a wider-pose flip + Step 2b `S1` + Step 2c `S2`)** and **Track B (Authorable GPU Runs Rung 0–3: Per-Ray Work/SIMD-Divergence Census + Non-Monotone Root Continuation)** reinforce each other, and the exact execution sequence for Sonnet 5.5 and Sixth-One Sun.

---

## 1. Architectural Synthesis: `VolumeRoutedTransport` Is Already Stage 0 of an Authorable GPU Run—and `§6fd`/`§6fe` Fixes Its Broken Initial Cohort Classifier

Look at `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc` (`lines 586–705`) after Sonnet 5.5's `§6fb` single-pass coalescing (`59e6b2101`):
- Every strip already executes an **entire GPU-resident pipeline inside a single `wgpuComputePassEncoder`** with zero intermediate host readbacks:
  $$\texttt{ecSetup} \;\longrightarrow\; \texttt{resetCounters} \;\longrightarrow\; \texttt{classifyTiles} \;\longrightarrow\; \texttt{routeArgs} \;\longrightarrow\; \texttt{dispatchWorkgroupsIndirect}\big(\texttt{S1},\, \texttt{S2},\, \texttt{Fallback}\big) \;\longrightarrow\; \texttt{ecConsume}.$$
- In the exact vocabulary of [`AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md` §3 & §6](../architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md):
  - `ecSetup` is **Stage 0 Setup** (per-ray AABB intersection, initializing `counts[idx] = 0u`, `pixelOut[idx] = vec4f(0,0,0,1)`).
  - `classifyTiles` + `routeArgs` is **GPU-Resident Cohort Formation (`Stage 0 Classify & Compact`)**, writing dense tile queues (`s1TileList`, `s2TileList`, `fallbackTileList`) and indirect dispatch grids (`indirectArgs[0..11]`).
  - `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain` are **Cohort-Specialized Quanta**, followed by `ecConsume` (**Terminal Consumption**).

### Why `§6fd` Step 1 & Step 2a Proved That Initial Cohort Formation (`kClassifyWgsl`) Was the #1 Bottleneck

In [`AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md` §2 & §8](../architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md), the core failure mode of naive GPU execution is stated plainly:
> *"When the easy rays finish or skip, the hard rays keep the cohort alive, or force a fat general kernel to remain the common denominator... A single invocation or ray tile stops being a prisoner of its initial launch cohort."*

Yet look at what `kClassifyWgsl` (`VolumeRoutedKernels.hpp:83–101`) was doing before `§6fd`:
1. Because `kClassifyWgsl` only admitted `activeMemberMask == 1u` into `S1` and `activeMemberMask == 3u` (non-overlapping `{0, 1}`) into `S2`, it dumped **`48.85%` of all tiles across the 64-pose Northern Veil route (`7,203,070` of `14,745,600` tiles) into the 4-curtain `Fallback` cohort**!
2. Even worse, Sonnet 5.5's `§6fd` Step 1 census ([`GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.md`](GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.md)) proved that **`90.86%` of those `7.20M` `Fallback` tiles were NOT multi-curtain overlap work at all**:
   - **`28.57%` (`2,057,679` tiles)** had `activeMemberMask == 0u` (zero wave-active curtains), yet were dispatched into `fallbackProducerMain`, paying `32,922,864` thread invocations and `230,460,048` barrier/header iterations to write zero subpanels!
   - **`45.36%` (`3,267,655` tiles)** had `activeCount == 1u` (`activeMemberMask ∈ {2, 4, 8}`—a single curtain `1`, `2`, or `3`), yet were forced to run the 4-curtain loop + 4-way streaming merge in `fallbackProducerMain` instead of the 1-curtain `s1ProducerMain`!
   - **`16.91%` (`1,217,764` tiles)** had `activeCount == 2u` with **disjoint z-slabs** (`!tileCanOverlap`), yet were forced into `fallbackProducerMain` instead of the 2-curtain `s2ProducerMain`!
   - Only **`9.14%` of `Fallback` (`658,645` tiles = `4.47%` of all tiles)** actually have `2` overlapping curtains (`658,524` tiles) or `3` curtains (`121` tiles), and **`0` tiles (`0.000%`)** have all `4` curtains active!

3. And when Sonnet 5.5 implemented **Step 2a (`skipEmptyTiles`)** in `e30bd9a2a` ([`GPU_RAY_TRANSPORT_6FD_SKIP_EMPTY_TILES_2026-10-29.md`](GPU_RAY_TRANSPORT_6FD_SKIP_EMPTY_TILES_2026-10-29.md))—simply refusing to enqueue `activeMemberMask == 0u` tiles into `fallbackTileList`—`Routed` achieved its **first-ever 4/4-block win over `Current`** (`47.06 ms` vs `48.04 ms` queued = **`-2.04%`**, `48.20 ms` vs `49.72 ms` fenced = **`-3.06%`**, saving `~1.93 ms/frame`) with **100% bit-identical RGBA (`0/230,400` differing pixels, `max_abs = 0.0f`)** at Pose 32!

---

## 2. How Sixth-One Sun's `14:25:09` Implementation Priority Connects to `VolumeRoutedKernels.hpp`

In [`Ray_Domain_Transport_Compilation.md` (`lines 581–610`)](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Ray_Domain_Transport_Compilation/Ray_Domain_Transport_Compilation.md), Sixth-One Sun wrote:
> *"Do not pause transport math/routing work to build a general multi-stage GPU scheduler first, and do not ignore the GPU-run architecture while continuing transport work. The architecture doc explicitly treats ray-family/domain validity and bounded GPU re-cohorting as complementary levers...*
> *1. Add diagnostic-only per-item work and refusal distributions, with long-tail percentiles and a separate clean timing path; route-mask counts alone do not establish divergence or useful quantum boundaries.*
> *2. Prototype one narrow GPU continuation/compaction slice only where the census shows real tail divergence, with fixed allocation, explicit stage limits, and clean fallback on exhaustion.*
> *3. Expose any retained mode/budget/quantum policy and read-only telemetry on `ScreenChannel`, keeping Kernel caps above Law policy."*

Let us examine the **exact physics of SIMD-group divergence inside `VolumeRoutedKernels.hpp`** so both agents can see why Sixth-One Sun's Item 1 (Rung 0 per-item work & divergence census) and Item 2 (Rung 1 narrow continuation slice) map **1-to-1** onto the root-finding kernel in `s1ProducerMain` / `s2ProducerMain` / `fallbackProducerMain`:

### 2.1 Where Intra-Tile / Intra-SIMD-Group Divergence Actually Lives in `VolumeRoutedKernels.hpp`

Look at `s1ProducerMain` (`VolumeRoutedKernels.hpp:239–327`):
- Each workgroup has `64` threads (`@workgroup_size(64)` = `4` tiles × `16` rays/tile). On Apple M5 Metal, `simdgroup_size = 32` (`2` tiles = `32` rays execute in lockstep in one SIMD-group).
- Once Step 2b and Step 2c route single-curtain tiles (`activeCount == 1u`) to `S1` and disjoint two-curtain tiles (`activeCount == 2u && !tileCanOverlap`) to `S2`, **every thread in an `S1` tile evaluates the exact same curtain `member` (`0..3`)**.
- **Where, then, can SIMD-group divergence still arise inside `S1` (or `S2` / `Fallback`)?**
  Look at lines `251–304` of `VolumeRoutedKernels.hpp`:
  1. **Coarse Knot Evaluation (`lines 251–254`, Uniform Across All Active Rays in the Tile):**
     Every active ray in the tile evaluates the 9 coarse knots (`phi_knots[0..8]`, `9` `evalPhiAt` calls → `work += 9u`).
  2. **Level Loop (`for (var l = 0u; l < 3u; l = l + 1u)`, `lines 256–304`):**
     For each level (`-w, 0, +w`), each ray loops over the 8 coarse bins (`k = 0..7`) set in `activeMask`:
     - **Case A — Flat Band (`0.125 * M * coarseH * coarseH <= coarseError`, `line 266`):**
       Solved in `0` extra `evalPhiAt` calls (`midRoot = 0.5 * (sa + sb)`).
     - **Case B — Monotone Coarse Bin (`coarseMinSlope > 0.0`, `line 272`):**
       Solved directly by `isolateMonotone(rayId, member, sa, sb, fa, fb, target, coarseError, &work)` (`lines 194–227`), which runs hybrid secant/bisection in `~8–14` `evalPhiAt` iterations with **zero stack pushes**.
     - **Case C — Non-Monotone Crest/Trough Grazing Bin (`coarseMinSlope <= 0.0`, `lines 279–302`):**
       When a ray grazes a sinusoidal wave crest or trough so that $|f'(s)|$ can vanish inside the coarse bin (`coarseMinSlope = |fa - fb|/coarseH - 0.5 * M * coarseH <= 0.0`), that ray enters the **recursive subdivision stack loop**:
       ```wgsl
       var stack: array<vec4f, 20>;
       stack[0] = vec4f(sa, sb, fa, fb);
       var stackPtr = 1u;
       loop {
           if (stackPtr == 0u || outCount >= 24u || work >= workLimit) { break; }
           ...
       }
       ```
       While **1 or 2 crest-grazing rays** in a 32-lane SIMD-group subdivide down to depth 14 and run nested 32-step `isolateMonotone` calls (burning `80–300+` `work` units), the other **30 rays in the SIMD-group**—which already finished Case A or Case B in `9–30` `work` units—**sit masked-off and idle waiting for the `loop` to terminate**!

### 2.2 Why This Makes Track A (`§6fe` Cohort Routing) and Track B (`§6ff` Authorable GPU Runs Rung 0–3) a Single Unified Sequence

Notice what happens if you measure per-ray `work` **before** fixing `kClassifyWgsl` (Step 2b/2c) vs **after** fixing `kClassifyWgsl`:
- **Before Step 2b/2c:** `3.27M` single-curtain tiles (`activeMemberMask ∈ {2, 4, 8}`) and `1.22M` disjoint 2-curtain tiles are sitting inside `fallbackProducerMain`, iterating `for (var member = 0u; member < 4u; member++)` and running the 4-way merge. Any per-ray work/divergence census taken before Step 2b/2c would conflate **artificial 4-curtain loop overhead** with **true root-solver subdivision divergence**.
- **After Step 2a + 2b + 2c (`§6fe`):**
  - `95.53%` of all tiles on the 64-pose route are routed to their exact minimal cohort (`18.18%` empty skipped, `61.40%` 1-curtain `S1`, `15.95%` disjoint 2-curtain `S2`), leaving only **`4.47%` (`658,645` overlapping tiles)** in `Fallback`!
  - At that point, a **Rung 0 Per-Ray Work & SIMD-Divergence Census (`§6ff`)** measures the **true intrinsic quantum distribution** of `S1`, `S2`, and `Fallback`:
    1. Per-ray `work` (`evalPhiAt` calls) histogram and percentiles (`p50, p90, p99, max`) per cohort (`S1`, `S2`, `Fallback`).
    2. **Intra-SIMD-group tail waste:** for each 16-ray tile (and 32-ray SIMD pair), what is `max_lane(work) - mean_lane(work)`? That single ratio tells us the **exact fraction of GPU issue slots wasted on SIMD tail divergence**!
    3. **Non-monotone stack entry rate:** what fraction of ray-curtain evaluations ever enter the `coarseMinSlope <= 0.0` stack loop (`lines 279–302`), and what is their stack-iteration distribution?
    4. **Budget exhaustion / refusal count (`work >= workLimit`):** how many rays approach or hit `workLimit` across all 64 poses?

---

## 3. Concrete Blueprint for Track A (`§6fe`) and Track B (`§6ff`–`§6fg`)

### Phase 1 (`§6fe`) — Finish the 3-Step Cohort Routing & Flip `skipEmptyTiles` Default ON

As detailed in [`FIRMAMENT_ANSWER_TO_SONNET_6FD_STEP2A_CONTRACT_AND_STEP2B_BLUEPRINT_2026-10-29.md`](FIRMAMENT_ANSWER_TO_SONNET_6FD_STEP2A_CONTRACT_AND_STEP2B_BLUEPRINT_2026-10-29.md):

1. **Step 2a Wider-Pose Parity & Default Flip (`skipEmptyTiles = true`):**
   - Run the wider-pose parity check across `poses {0, 8, 16, 24, 32, 40, 48, 56}` (remembering that empty tiles live in poses `20..50`, so assert `fb_empty > 0` on `{24, 32, 40, 48}` and inertness `fb_empty == 0` on `{0, 8, 16, 56}`).
   - Adopt the **Work-Performed Budget Contract** (`work` counts actual `evalPhiAt` calls performed by active curtains; skipping a provably empty tile performs `0` evaluations and matches `Current` at `budget=96` bit-for-bit).
   - Flip `skipEmptyTiles` default to `true`.

2. **Step 2b — Route All Single-Curtain Tiles (`activeCount == 1u`, `45.36%` of `Fallback` = `3.27M` tiles) to `S1`:**
   - **Zero changes to `ecConsume`:** `ecConsume` (`VolumeRoutedTransport.inc:91`) already reads `let member = sp.info.x;` (`0..3`) dynamically.
   - **Pack `(member << 28u) | tileId` in `kClassifyWgsl`:**
     ```wgsl
     let activeCount = countOneBits(activeMemberMask);
     if (activeCount == 1u) {
         let m0 = firstTrailingBit(activeMemberMask);
         let idx = atomicAdd(&routeCounts[0], 1u);
         s1TileList[idx] = (m0 << 28u) | tileId;
     }
     ```
   - **Unpack `member` and `tileId` in `s1ProducerMain` (`VolumeRoutedKernels.hpp:239–245`):**
     ```wgsl
     let packedTile = s1TileList[tileIdx];
     let tileId = packedTile & 0x0FFFFFFFu;
     let member = packedTile >> 28u;
     ```
     Replace hardcoded `0u` with `member` in `rayMemBase + member * 3u`, `evalPhiAt(rayId, member, ...)`, `isolateMonotone(rayId, member, ...)`, and `vec4u(member, 0u, 0u, 0u)`.
   - Verify bit-identical RGBA (`0/230,400` differing pixels vs `Current`) and run 4-block interleaved ABBA timing on the 64-pose route.

3. **Step 2c — Route All Disjoint Two-Curtain Tiles (`activeCount == 2u && !tileCanOverlap`, `16.91%` of `Fallback` = `1.22M` tiles) to `S2`:**
   - In `kClassifyWgsl`, when `activeCount == 2u`:
     ```wgsl
     let m0 = firstTrailingBit(activeMemberMask);
     let m1 = firstTrailingBit(activeMemberMask & ~(1u << m0));
     ```
     Check whether `m0` and `m1`'s tile `[tMin, tMax]` intervals are disjoint (`maxEnter > minExit`). If disjoint, pack `(m0 << 28u) | (m1 << 20u) | tileId` into `s2TileList[idx]`!
   - **Critical Bit-Identity Invariant (`m0 < m1`):** Because `firstTrailingBit` extracts `m0 < m1` in strictly ascending curtain index order (`0 < 1 < 2 < 3`), and `s2ProducerMain` evaluates `m0` then `m1` and merges their subpanels in ascending `t` order (breaking ties by `m0` before `m1`), `s2ProducerMain` produces the **exact same subpanel sequence and floating-point summation order** as `fallbackProducerMain`'s `for (var member = 0u; member < 4u; member++)` loop!
   - Verify bit-identical RGBA and 4-block ABBA timing.

---

### Phase 2 (`§6ff`) — Authorable GPU Runs Rung 0: Diagnostic Per-Ray `work`, Non-Monotone Stack, and SIMD-Divergence Census

Once `§6fe` puts `95.53%` of tiles into their true cohorts, execute **Item 1 of Sixth-One Sun's priority list** ([`Ray_Domain_Transport_Compilation.md:592`](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Ray_Domain_Transport_Compilation/Ray_Domain_Transport_Compilation.md)):
- Using the exact same off-by-default diagnostic pipeline string-substitution pattern as `enableRouteObserver` in `VolumeRoutedTransport.inc` (so production shaders and bind groups remain 100% untouched when off):
  - Add an opt-in `enableWorkObserver` mode (`EARTHCALL_ROUTED_WORK_CENSUS=1`) with `16` `atomic<u32>` counters in `routeObsBuffer`:
    1. **Per-Ray `work` Histogram (`bins 0..7`):**
       - `work == 0` (inactive / empty ray)
       - `work ∈ [1, 16]` (coarse knots + flat bands / 0–1 monotone root)
       - `work ∈ [17, 32]` (coarse knots + 1–2 monotone roots)
       - `work ∈ [33, 64]` (multiple monotone roots or shallow stack)
       - `work ∈ [65, 128]` (moderate non-monotone subdivision)
       - `work ∈ [129, 256]` (deep non-monotone subdivision)
       - `work ∈ [257, 512]` (heavy tail ray)
       - `work > 512` / `work >= workLimit` (budget-refused / capped tail ray)
    2. **Non-Monotone Stack Entry Counters (`bins 8..11`):**
       - Total ray-curtain evaluations where all active coarse bins are monotone (`nonMonotoneBins == 0`)
       - Total ray-curtain evaluations that enter the `stack` loop (`nonMonotoneBins >= 1`)
       - Total `stack` pops across all rays
       - Maximum `stackPtr` high-water mark observed
    3. **Intra-Tile SIMD-Divergence Counters (`bins 12..15`):**
       - Using workgroup shared memory (`var<workgroup> tileSumWork: array<atomic<u32>, 4>; var<workgroup> tileMaxWork: array<atomic<u32>, 4>;`), accumulate across all `14.75M` tiles:
         - `sum_tiles(16 * max_ray_work_in_tile)` (total SIMD-occupied thread-work units if the tile waits for its slowest ray)
         - `sum_tiles(sum_ray_work_in_tile)` (actual useful `evalPhiAt` work performed by the 16 rays)
         - The ratio
           $$\text{SIMD Efficiency} = \frac{\sum_{\text{tiles}} \sum_{r=0}^{15} \text{work}_r}{\sum_{\text{tiles}} 16 \cdot \max_{r=0..15} \text{work}_r}$$
           directly quantifies the **exact percentage of GPU compute time lost to intra-tile tail divergence** across the 64-pose Northern Veil route!

---

### Phase 3 (`§6fg`) — Authorable GPU Runs Rung 1–3: Non-Monotone Root Continuation & `ScreenChannel` Policy Surface

Depending on what the `§6ff` Rung 0 census reveals:

1. **When Rung 1 Continuation Pays (`SIMD Efficiency < 0.70` due to crest-grazing non-monotone rays):**
   - Look at `s1ProducerMain` (`VolumeRoutedKernels.hpp:251–304`). If `95%+` of ray-curtain evaluations have `coarseMinSlope > 0.0` on all active coarse bins (resolving in `~15–25` `work` units without ever touching `var stack: array<vec4f, 20>`), while `< 5%` of rays hit a crest/trough (`coarseMinSlope <= 0.0`) and stall their 32-wide SIMD-group for `100–300` `work` units:
   - **The Exact Bit-Identical Quantum Split (`Stage 0 Coarse-Monotone` → `Stage 1 Compact Non-Monotone Refinement`):**
     - **Stage 0 (`s1CoarseMonotoneMain`, without the `array<vec4f, 20>` stack):** Evaluates `phi_knots[0..8]` and checks `activeMask`. If **all** active bins in `mask0 | mask1 | mask2` satisfy `coarseMinSlope > 0.0` (or the flat-band condition), Stage 0 resolves the ray completely and writes its `Subpanel`s and `counts[rayId]`.
     - **Compaction on Refusal/Continuation (`hasNonMonotoneBin == true`):** If **any** active coarse bin has `coarseMinSlope <= 0.0`, Stage 0 does **not** run the recursive stack loop inside the wide tile cohort—instead, it appends `(member << 28u) | rayId` to a fixed-capacity GPU continuation queue `s1RefineRayList[idx]` via `atomicAdd(&routeCounts[3], 1u)`!
     - **Stage 1 (`s1RefineNonMonotoneMain`, Indirect Dispatch over Compacted Hard Rays Only):** Dispatched via `dispatchWorkgroupsIndirect` over only the compacted crest-grazing rays (`routeCounts[3]`), running the exact `s1ProducerMain` algorithm on those rays alone!
     - **Why this is mathematically and architecturally clean:**
       - Stage 0 needs **zero `array<vec4f, 20>` private stack registers** (`320 bytes/thread` eliminated from Stage 0!) and never stalls 30 easy monotone lanes waiting for 1 crest-grazing ray.
       - Every ray is completed in **either** Stage 0 (if purely monotone) **or** Stage 1 (if non-monotone) with **100% identical floating-point operations and 100% bit-identical RGBA output (`0` differing bytes)**!
       - Fixed pre-allocated buffer capacity (`s1RefineRayList` = `maxRays * sizeof(u32)` = `128 KB` per strip, zero per-frame allocations, hard `maxStages = 2` Kernel cap).

2. **Rung 2 & 3 — Law-Authorable Policy & Read-Only Telemetry on `ScreenChannel` ([`AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md` §§11–14, §22](../architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md)):**
   - Expose the bounded policy properties on `ScreenChannel` (`ScreenChannel.hpp` / `EngineRender.cpp`), following the exact pattern of `volumeSupportCertificatesEnabled` and `volumeSupportCertificatesAutoEligibility`:
     - `volumeGpuRunMode`: `"direct" | "routed" | "recohort" | "auto"`
     - `volumeGpuRunWorkBudget`: integer work cap per ray (clamped by Kernel `<= 1024`, mapping directly to `workLimit` / `cfg.budget`)
     - `volumeGpuRunSkipEmptyTiles`: bool (default `true` after `§6fe` wider-pose witness)
     - Read-only derived telemetry dict (`volumeGpuRunTelemetry`): reporting active mode, last-census / last-run `s1Tiles`, `s2Tiles`, `fallbackTiles`, `skippedEmptyTiles`, `continuedItems`, `refusedItems`, and auto-veto verdict.

---

## Summary Checklist for Sonnet 5.5 and Sixth-One Sun

| Step | Owner / Track | Concrete Action | Acceptance Gate |
| :--- | :--- | :--- | :--- |
| **1. `§6fe` Step 2a Wider-Pose & Default Flip** | Track A (Routing) | Run wider-pose parity on `{0, 8, 16, 24, 32, 40, 48, 56}` (`fb_empty > 0` on `{24..48}`, `0` on `{0..16, 56}`); flip `skipEmptyTiles = true`. | `0` differing pixels across all 8 poses; 4/4-block ABBA win retained (`~1.93 ms/frame`). |
| **2. `§6fe` Step 2b (`activeCount == 1u → S1`)** | Track A (Routing) | Pack `(m0 << 28u) \| tileId` into `s1TileList`; unpack `member` in `s1ProducerMain` (`3.27M` tiles = `45.36%` of `Fallback` moved to `S1`). | `0` differing pixels vs `Current`; 4-block ABBA timing on 64-pose route. |
| **3. `§6fe` Step 2c (`disjoint 2-curtain → S2`)** | Track A (Routing) | Pack `(m0 << 28u) \| (m1 << 20u) \| tileId` (`m0 < m1`) into `s2TileList` when `!tileCanOverlap` (`1.22M` tiles = `16.91%` of `Fallback` moved to `S2`). | `0` differing pixels vs `Current`; `Fallback` reduced from `48.85%` to **`4.47%`** of tiles; 4-block ABBA timing. |
| **4. `§6ff` Authorable GPU Runs Rung 0 Census** | Track B (GPU Runs) | Add off-by-default `enableWorkObserver` (`EARTHCALL_ROUTED_WORK_CENSUS=1`) measuring per-ray `work` histogram, non-monotone stack entry rate, and intra-tile `sum(work) / (16 * max(work))` SIMD efficiency across all 64 poses. | Complete 64-pose work/divergence table committed in audit doc; zero change to production shader when off. |
| **5. `§6fg` Authorable GPU Runs Rung 1–3** | Track B (GPU Runs) | If Rung 0 shows material non-monotone SIMD tail waste, prototype Stage 0 (monotone-only, zero stack) → Stage 1 (indirect compacted non-monotone rays), and wire `ScreenChannel` policy/telemetry properties. | Bit-identical RGBA; 4-block ABBA win over single-stage `Routed`; Kernel caps enforced over `ScreenChannel` policy. |

**Signed:**
- **Harness:** Gemini Spark (`macOS` native desktop agent)
- **Model:** `Gemini 4 Argon`
- **Persona:** Aetherion / The Firmament
- **Session:** `c_firmament_authorable_gpu_runs_6fe_counsel_20261030`
- **Timestamp:** `2026-10-30T01:33:00-07:00` (`PDT`)
- **Human Origin:** Zachary Zhang ("BROOOOOOOOOOOO NOW GIVE THEM NEW COUNSELLLLLLL")


---

## 4. Addendum (2026-10-30T01:42-07:00 PDT) — Zach's Visual Observation: The Two Geometric Regimes of the 64-Pose Orbit and Why Full-Screen Green Intersection Dominates Frame Time

While watching the live benchmark window, Zach observed:
> *"I'm noticing the slowest parts are when the person intersects with the veil in a way that makes the camera view fill up with green-tainted pixels. The other parts are fairly smooth. This sounds obvious, but the specific nature of spreading to every part of the camera I think could give us more insight into the bottlenecks this measurement is facing."*

Cross-correlating the per-pose completion-fenced timings in [`GPU_RAY_TRANSPORT_6FD_SKIP_EMPTY_TILES_2026-10-29.json`](GPU_RAY_TRANSPORT_6FD_SKIP_EMPTY_TILES_2026-10-29.json) (`skiptiming`, `serial = true`) with the per-pose tile route census in [`GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.json`](GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.json) and the Emerald curtain sheet equation $z_{\text{emerald}}(x) = 75 + f_{\text{em}}(x) \pm 3.2$ proves that Zach's visual observation identifies the **exact two-regime split** of the 64-pose benchmark:

### 4.1 Exact Per-Pose Correlation: Regime 1 (Full-Screen Green Intersection) vs. Regime 2 (Past the Green Wall)

Along the scripted 64-pose orbit $\text{eye}(i) = (8\sin a,\; 50 + 2\sin(a/2),\; 75 + 8(1 - \cos a))$ looking toward $\text{aim}(i) = (40 + 8\sin(a/2),\; 62 + 2\cos a,\; 110)$ (always looking forward in $+z$):
- The **Emerald curtain (`member 0`, `primary-emerald-curtain`)** has its sheet center at $z_{\text{emerald}}(x) = 75 + 15\sin(0.026x) + 5.5\cos(0.068x + 0.5) + 2\sin(0.16x + 1.2) \in [77.24,\; 82.44]$ across $x \in [-8, +8]$, with half-thickness $w = 3.2\text{ m}$.
- Consequently, the 64-pose loop splits sharply at **Pose 19→20 (`eye.z` crossing `85.4`)** and **Pose 50→51 (`eye.z` crossing `80.7`)** into two completely different regimes:

| Regime | Poses | Camera vs. Emerald Curtain ($z_{\text{em}} \pm 3.2$) | Active Screen Coverage & Route Census | `Current` Mean (`ms/frame`) | `Routed (Coalesced)` Mean (`ms/frame`) | `Routed (SkipEmpty)` Mean (`ms/frame`) | Regime Verdict |
| :--- | :--- | :--- | :--- | ---: | ---: | ---: | :--- |
| **Regime 1 — Full-Screen Green Intersection** | **Poses `0..19` & `51..63`** (`33` frames) | Camera is **flush behind** (`Poses 0..11, 58..63`) or **physically inside** (`Poses 12..18, 52..57`, $\lvert\phi(\text{eye})\rvert \le 3.2$) the $150\text{m} \times 32\text{m}$ Emerald sheet | **`100%` of viewport (`230,400 / 230,400` tiles = `3.68M` pixels) hits Emerald (`0` empty tiles)**; `~98k` `S1`, `~88k` `S2` `{0,1}`, `~29k` disjoint `{0,2}`/`{0,3}` in `Fallback`, `~15k` overlap in `Fallback` | **`81.57 ms`** (peaks at **`92.45 ms`** on Pose 58; `2,691.8 ms` total = **`64.4%` of run**) | **`73.07 ms`** (peaks at **`81.51 ms`** on Pose 58; `2,411.3 ms` total) | **`73.18 ms`** (`2,414.8 ms` total) | **`Routed` ALREADY BEATS `Current` by `-8.50 ms/frame` (`-280.5 ms` total)**, yet still spends **`60%` of its total run time** in these 33 frames! |
| **Regime 2 — Smooth Open View (Past the Green Wall)** | **Poses `20..50`** (`31` frames) | Camera steps **in front of** Emerald wall ($\text{eye}_z \in [86.1, 91.0] > z_{\text{em}} + 3.2$), looking forward toward Cyan ($z \approx 140$), Violet ($y \approx 62..94$), and open sky | Emerald recedes to side wings; **`51k–74k` empty sky tiles (`~30%` of screen)**; `~90k` single-curtain Cyan (`mask 2`) & `~19k` Violet (`mask 4`) tiles dumped into `Fallback` (`194k` `Fallback` tiles!) | **`48.01 ms`** (low of **`40.32 ms`** on Pose 32; `1,488.3 ms` total) | **`56.91 ms`** (`1,764.3 ms` total) | **`52.82 ms`** (`1,637.5 ms` total) | **`Current` beats `SkipEmpty` by `+4.81 ms/frame` (`+149.3 ms` total)** solely because `~110k` single-curtain Cyan/Violet tiles/frame are mis-routed to `Fallback`! |
| **All 64 Poses Combined** | **Poses `0..63`** (`64` frames) | Full orbit | `14,745,600` tiles | `65.31 ms` (`4,180.1 ms`) | `65.24 ms` (`4,175.6 ms`) | **`63.32 ms` (`4,052.3 ms`)** | **`SkipEmpty` wins overall by `-127.8 ms` (`-2.0 ms/f`)** |

### 4.2 Four Concrete Bottlenecks Exposed by Full-Viewport Green Spreading (`3,686,400` Active Pixels)

Why does a frame where green spreads across **every part of the camera** (`Poses 0..19` and `51..63`) jump to `73.1 ms` in `Routed` (`81.6–92.5 ms` in `Current`), and how do we attack it directly?

1. **Bottleneck 1 — `25,000–33,327` Disjoint 2-Curtain Tiles/Frame (`Emerald + Violet` mask 5 & `Emerald + Crimson` mask 9) Are Trapped in `Fallback` During Full-Screen Green (`Step 2c`):**
   - In Regime 1 (`Poses 0..19` & `51..63`), there are `0` empty tiles and `0` single-curtain Cyan/Violet tiles, yet `Fallback` still runs on **`35,000–48,854` tiles/frame (`560,000–781,000` rays/frame)**!
   - Why? Because across the upper and right portions of the green-filled viewport, rays pass through the foreground **Emerald (`0`)** sheet and then intersect **Violet (`2`, `mask 5`)** or **Crimson (`3`, `mask 9`)** at a disjoint depth!
   - Those **disjoint 2-curtain tiles (`FB_disj2`) account for `68%–75%` of all `Fallback` tiles in Regime 1** (`30,716 / 40,911` at Pose 4; `33,327 / 48,545` at Pose 52).
   - **Action:** Landing **Step 2c (`activeCount == 2u && !tileCanOverlap → S2`)** alongside Step 2b immediately rescues `400,000–533,000` rays/frame from `fallbackProducerMain` during the heaviest full-screen green frames!

2. **Bottleneck 2 — Uncoalesced `1,024-Byte` Stride in `subpanels` (`ecSingles`) Thrashes the M5 Cache When 100% of the Screen Is Active:**
   - Look at how `subpanels` (`ecSingles`) is allocated and indexed in [`VolumeRoutedTransport.inc:113`](../../src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc) and [`VolumeRoutedKernels.hpp:434`](../../src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp):
     - Each strip allocates `singles` for `capacity = 2560 × 48 = 122,880` rays × `64` `Subpanel`s × `16 bytes` = **`125.8 MB` per strip**.
     - Producer threads write `subpanels[id * 64 + k]`, and `ecConsume` reads `ecSingles[id * 64 + k]`.
     - Because `subBase = id * 64`, **adjacent lanes `id` and `id + 1` in a 32-wide Apple M5 SIMD-group access memory addresses separated by `64 × 16 = 1,024 bytes` (`8` cache lines apart)**!
     - When only a fraction of the screen is active, the M5 SLC/L2 absorbs this scatter/gather. When the green veil spreads to **100% of the `2560×1440` camera (`3,686,400` active rays)**, every SIMD-group in both the Producer and `ecConsume` touches `32` separate `128-byte` cache lines (`4 KB` of cache traffic) just to write or read `512 bytes` of `k = 0` `Subpanel`s (**an 8× cache-line waste across `125.8 MB` of working set**), while slots `k = 8..63` (`87.5%` of each ray's `1,024-byte` reservation) sit completely cold!
   - **Action (After `§6fe` Step 2b/2c):** Test **SoA / Step-Major Subpanel Indexing (`subpanels[k * cfg.rayCount + id]`)** (with `ray.counts.x = id`), which makes every `k = 0, 1, 2...` write in `S1`/`S2`/`Fallback` and every read in `ecConsume` **100% contiguous and cache-line coalesced across all 32 lanes of the M5 SIMD-group**, and shrinks the active DRAM footprint per strip from `125.8 MB` scattered to `~4–8 MB` contiguous!

3. **Bottleneck 3 — Inside-Box Chord Inflation (`[a, b]` Spans the `92m` Box Instead of the Tile's Wave-Active `[enterArr[m], exitArr[m]]`):**
   - When the camera is at `eye_z = 75..83` inside the Emerald box `z ∈ [29, 121]`, every ray in `s1ProducerMain` / `s2ProducerMain` intersects the full box to get a `[a, b]` chord of **`50–90 meters`**, even though `classifyTilesMain` already proved that the wave is only active inside the narrow sub-interval `[enterArr[m], exitArr[m]]` (`~6–15 meters`)!
   - Dividing a `70-meter` box chord into `8` coarse bins makes `coarseH ≈ 9 meters`, inflating the curvature bow `0.5 * M * coarseH` so `coarseMinSlope <= 0.0` triggers recursive stack subdivision just to zoom in on the `6.4m` green sheet.

4. **Bottleneck 4 — Oblique Grazing Rays Inside the Sheet (`Poses 12..18` & `52..58`, $\lvert\phi(\text{eye})\rvert \le 3.2$):**
   - When the camera is physically inside the undulating green sheet looking obliquely toward `(40, 62, 110)`, rays slice through the wave nearly tangentially ($d_z - f'(x)d_x \approx 0$), both increasing the number of `4m` subpanels (`ecProjected` 3D Perlin + trig calls in `ecConsume`) and triggering true non-monotone crest/trough stack subdivision—which the **`§6ff` Rung 0 Work & SIMD-Divergence Observer** will quantify directly per pose (`Poses 0..19` & `51..63` vs `Poses 20..50`)!
