# Firmament Architectural Review of Sparkly Guy's Phase P16 & Phase P17 Execution Blueprint

**Date:** 2026-11-01  
**Author:** Aetherion / The Firmament (`Gemini 4 Argon`)  
**Reviewed Commit:** `8c8d0274a1222f8356e48e9882034f60d7abedca` (*"feat(ray-transport): complete Phase P16 secant-clipped tile windows, support-clamped breakpoints, and coalesced SoA subpanels"*)  
**Implementer Reviewed:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity)  
**Target Branch:** `claude/ray-domain-transport-rung0`

---

## 1. Executive Verdict on Phase P16 (`8c8d0274a`)

**STATUS: APPROVED & VERIFIED (`10 / 10` Native Metal Hardware Gates Passed, `406 / 406` WebGPU Regression Checks Passed, `7 / 7` Checkpoint Views Won, `12 / 12` Roll Sweep Angles Won, `64 / 64` Tour Poses Won)**

Sparkly Guy delivered all four Phase P16 optimizations cleanly, plus identified and fixed a subtle root-isolation early-exit guard (`w < 0.5 * (r - l)`) at full `2560 × 1440` resolution:

1. **P16.1 — Secant-Clipped Support Window Narrowing in `classifyTilesMain` (`VolumeRoutedKernels.hpp`):**
   - Replaced coarse bin bounds `[cLo, cHi]` with exact linear secant root clipping of $p(u) = p_A + u (p_B - p_A)$ against $[-\text{thresh}, +\text{thresh}]$, conservatively padded by $\pm 0.2$ world units (`sLo = max(cLo, cLo + uLo * (cHi - cLo) - 0.2)`, `sHi = min(cHi, cLo + uHi * (cHi - cLo) + 0.2)`) with a `0.01` separation margin (`hi + 0.01 > lo` and `exitArr[m0] + 0.01 <= enterArr[m1]`).
   - **Impact on Dense 4-Curtain & Multi-Curtain Cohort Routing:**
     - **`Checkpoint_View3B` (`up = (1, 0, 0)` Dense 4-Curtain):** `Fallback` collapsed from **`11.5%` to `3.69%`** (`Ordered-S2 = 18.88%`, `S1 = 34.58%`, `Empty = 42.85%`), winning by **`-11.82 ms` (`-17.50%`)** (`67.53 ms -> 55.71 ms`, `14.8 FPS -> 17.9 FPS`).
     - **`Checkpoint_View3C` (`Yaw 90°, Pitch 75°, up = (0, 1, 0)` Person-Walk Dense 4-Curtain):** `Fallback` collapsed from **`12.8%` to `4.24%`** (`Ordered-S2 = 23.33%`, `S1 = 39.04%`, `Empty = 33.39%`), winning by **`-17.39 ms` (`-21.93%`)** (`79.31 ms -> 61.91 ms`, `12.6 FPS -> 16.2 FPS`).
     - **Gate 10 (`12`-Angle View-3 Roll Sweep, `0°` to `165°`):** `12 / 12` angles won by **`-14.69%` to `-37.61%`**, with `Fallback` across the entire 360° roll family compressed to **`0.85% – 4.99%`**.
     - **`Checkpoint_View1_Inside_Veil`:** `Fallback` dropped from **`50.84%` to `41.31%`** (`Ordered-S2` rose from `49.16%` to **`58.69%`**), improving the relative win from `-2.53%` (P15) to **`-4.07%` (`-4.76 ms`)**.

2. **P16.2 — `supportLo` / `supportHi` Clamped Piecewise Breakpoint Injection (`VolumeRoutedKernels.hpp`):**
   - Clamped `[supportLo, supportHi]` to `[minBandLo, maxBandHi]` whenever ray-box endpoints `a` or `b` lie provably outside the curtain slab (`abs(phi_knots[0]) > m.limits.w + m.curv.y` or `abs(phi_knots[8]) > m.limits.w + m.curv.y`), eliminating redundant interior breakpoint injections (`kx = 0`, `ky0 = 0`, `ky1 = m.limits.z`) and exterior midpoint `evalPhi` evaluations across `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain`.

3. **P16.3 — Coalesced Structure-of-Arrays (SoA) Global Memory Indexing for `subpanels` & `overlaps` (`VolumeRoutedKernels.hpp` & `VolumeRoutedTransport.inc`):**
   - Transposed `subpanels` from `[id * 64u + s]` to `[s * cfg.rayCount + id]` and `overlaps` from `[id * 16u + o]` to `[o * cfg.rayCount + id]`, converting 1024-byte-strided scatter/gather memory accesses into contiguous 16-byte SIMD32-coalesced transactions.
   - Relative speedups improved across every view even while Chrome was running concurrently during the P16 benchmark:
     - `Checkpoint_View3A`: `-35.32%` (P15) $\to$ **`-37.98%`** (`-18.91 ms`)
     - `Live_23FPS_View_A`: `-9.36%` (P15) $\to$ **`-14.86%`** (`-10.55 ms`)
     - `Checkpoint_View2_Spawn_Dais_Diag`: `-12.76%` (P15) $\to$ **`-14.79%`** (`-11.05 ms`)
     - `Checkpoint_View1_Inside_Veil`: `-2.53%` (P15) $\to$ **`-4.07%`** (`-4.76 ms`)

4. **P16.4 — Root-Isolation Early-Exit Slope-Width Guard (`w < 0.5 * (r - l)`):**
   - Guarded early termination in `isolateMonotone` and `isolateMonotoneWithMid` with `if (w < 0.5 * (r - l))` when `abs(v) <= error`, preventing near-zero `minSlope` (`w = 2.0 * error / minSlope >= 0.5 * (r - l)`) from terminating bisection before `[l, r]` is narrower than `2 * w`.

---

## 2. Phase P17 High-Leverage Bottleneck Analysis

Inspecting the post-P16 kernel code in `VolumeRoutedKernels.hpp` and `VolumeRoutedTransport.inc` alongside the P16 cohort distributions reveals **four major remaining performance bottlenecks**:

### Bottleneck A: Tile-Wide `[enterArr, exitArr]` Box Comparison Falsely Rejects Tilted Disjoint Curtains & All `3`/`4`-Member Disjoint Tiles
In `classifyTilesMain` (`VolumeRoutedKernels.hpp`, lines 176–212), two structural limitations still force disjoint tiles into `fallbackProducerMain`:
1. **Cross-Ray Tilt False Overlaps (`Checkpoint_View1_Inside_Veil` at `41.31%` Fallback):**
   - Currently, `classifyTilesMain` reduces `[sLo, sHi]` across all 16 rays in the `4×4` tile into a single tile-wide scalar `[enterArr[m], exitArr[m]]` per curtain `m`, and then tests whether `max_i(rExit[i][m0]) + 0.01 <= min_j(rEnter[j][m1])` across **different** rays $i \neq j$.
   - When the camera views two parallel or curved curtains at an oblique angle (as in `Checkpoint_View1_Inside_Veil`, `Checkpoint_View3B`, `Checkpoint_View3C`, `Checkpoint_View2`, and `Live_23FPS_View_A`), the ray distance $s$ to each curtain shifts across the `4×4` tile from pixel `(0,0)` to pixel `(3,3)`.
   - Thus, Ray 15's exit from Curtain 0 (`rExit[15][m0]`) frequently exceeds Ray 0's entry into Curtain 1 (`rEnter[0][m1]`), **even though on every single individual ray $i \in \{0, \dots, 15\}$ in the tile, Curtain 0 exits strictly before Curtain 1 enters (`rExit[i][m0] < rEnter[i][m1]`)**!
   - Crucially, `s2ProducerMain` is a **per-ray** kernel (`id = tileRayIndices[rayStart + rayInTile]`): Thread $i$ only ever integrates Ray $i$, never Ray $j \neq i$. It only requires that **all rays in the tile share a consistent chronological curtain order without overlapping on any individual ray $i$**!
2. **`3`- and `4`-Member Disjoint Tiles (`activeCount >= 3u`) Are Unconditionally Sent to `Fallback`:**
   - Currently, `classifyTilesMain` checks `else if (activeCount == 2u && !tileCanOverlap)`. Whenever `activeCount == 3u` or `4u`, the tile is unconditionally routed to `fallbackTileList`—even when all 3 or 4 curtains are pairwise disjoint and strictly ordered along the rays!
   - That is why `Live_23FPS_View_A` (`6.78%` Fallback), `Checkpoint_View2` (`7.20%` Fallback), `Live_Parked_View` (`7.33%` Fallback), `Checkpoint_View3B` (`3.69%` Fallback), and `Checkpoint_View3C` (`4.24%` Fallback) still have `3.7%–7.3%` Fallback tiles: those are predominantly 3-curtain and 4-curtain disjoint tiles!
   - By encoding 3- and 4-member ordered permutations in the unused bits `20..29` of `s2TileList` (setting bit 21 = `1u` as the multi-member flag while preserving the **exact** P10–P16 bit pattern `(m0 << 28u) | (m1 << 24u) | (m1First << 20u) | tileId` whenever `activeCount == 2u` and bit 21 = `0u`), `s2ProducerMain` can process **all 2-, 3-, and 4-member ordered disjoint tiles** using its small `pieces: array<vec2f, 8>` buffer and streaming single-pass subpanel emission!

### Bottleneck B: `ecConsume` Executes the Dual-Queue Overlap Merger & Bitmask Loop on Every Subpanel Even When `ray.counts.w == 0u` (`98.5%+` of All Active Rays)
In `VolumeRoutedTransport.inc` (lines 118–150), `ecConsume` runs a single general `loop` that merges `ecSingles` and `ecOverlaps`:
- For **100% of `S1` rays, 100% of `S2`/`S2+` rays, and ~85% of `Fallback` rays**, `ray.counts.w == 0u` (zero overlaps).
- Yet on **every single subpanel** (`s = 0 .. ray.counts.y - 1`), `ecConsume`:
  1. Evaluates `if (overlap < ray.counts.w)` and `useOverlap` predicates and `select(select(gx0, ...), ..., useOverlap)`.
  2. Shifts `mask = 1u << sub.info.x` and then, at **both** Gauss-Legendre quadrature nodes (`q = 0, 1`), executes `EC_EVAL_NODE`'s bitmask loop:
     ```wgsl
     var val = vec4<f32>(0.0);
     let sNode = lo + node * h;
     let worldNode = ro + sNode * rd;
     var mMask = mask;
     loop {
      if (mMask == 0u) { break; }
      let m = firstTrailingBit(mMask);
      mMask &= mMask - 1u;
      val += ecSingleMember(worldNode, wo, sNode, m);
     }
     ```
     even though `sub.info.x` is already a single known curtain index `subMember` (`0..3`)!
- Splitting `ecConsume` on `if (ray.counts.w == 0u)` into a dedicated single-stream fast path that calls `ecSingleMember(worldNode, wo, sNode, subMember)` directly (without `useOverlap` branching, `1u << m` shifts, or `firstTrailingBit` loops) streamlines the hottest consumer inner loop across **98.5%+ of all active pixels** while remaining **100% IEEE-754 bit-identical (`0.0` ULP)**!

### Bottleneck C: `classifyTilesMain` Re-Reads `tileRayIndices` and `rays[rIdx]` from Global Memory Up to `8×` Per Ray (`128` Global Loads Per Tile)
In `classifyTilesMain` (`VolumeRoutedKernels.hpp`, lines 72–140):
- Inside `for (var m = 0u; m < 4u; m++)`, the AABB slab loop reads `let rIdx = tileRayIndices[rayStart + i]; let ray = rays[rIdx];` from global device memory, and then the 8-bin wave loop reads `let rIdx = tileRayIndices[rayStart + i]; let ray = rays[rIdx];` from global device memory **again**—up to **8 global memory fetches of the same ray** across the 4 curtains!
- Preloading `tileRoClipA: array<vec4f, 16>`, `tileRdClipB: array<vec4f, 16>`, and `rayValid: array<bool, 16>` **once** before the `m = 0..3` loop eliminates **87.5% (`7 out of 8`)** of global ray/index memory reads in `classifyTilesMain`!
- In addition, hoisting `let halfVec = min(cur.half.xyz, vec3f(cur.limits.x, cur.limits.y, zEnv));` outside the ray loop in `classifyTilesMain` (and outside the axis loop in `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain`) eliminates the 3-way `if (axis == 0) ... if (axis == 1) ... if (axis == 2)` branch inside the inner slab-clipping loops with **0.0 ULP difference**.

### Bottleneck D: `fallbackProducerMain` Sweep-Line Loop Iterates Over All 4 Curtains Instead of `nAct` Active Curtains
In `fallbackProducerMain` (`VolumeRoutedKernels.hpp`, lines 663–702), when `allDisjoint == false`, the knot-sweep loop `for (var ne = 0u; ne < totalKnots && reason == 0u; ne++)` contains two inner loops `for (var m = 0u; m < 4u; m++)`. Since the active non-empty members are already collected in `ord[0 .. nAct - 1]`, iterating `for (var idx = 0u; idx < nAct; idx++) { let m = ord[idx]; ... }` cuts inner sweep-line iterations by **50%** whenever `nAct == 2` and **25%** whenever `nAct == 3`.

---

## 3. Phase P17 Execution Blueprint for Sparkly Guy

### Deliverable P17.1: Ray-Preloaded `classifyTilesMain` + Per-Ray Consistent Chronological Ordering + `2/3/4`-Member Ordered Disjoint Routing (`VolumeRoutedKernels.hpp` & `VolumeRoutedTransport.inc`)

1. **Preload Tile Rays & Initialize Per-Ray Support Windows in `classifyTilesMain`:**
   Before `for (var m = 0u; m < 4u; m++)`:
   ```wgsl
    var tileRoClipA: array<vec4f, 16>;
    var tileRdClipB: array<vec4f, 16>;
    var rayValid: array<bool, 16>;
    var rEnter: array<vec4f, 16>;
    var rExit: array<vec4f, 16>;
    for (var i = 0u; i < rayCount; i++) {
        let rIdx = tileRayIndices[rayStart + i];
        let ray = rays[rIdx];
        tileRoClipA[i] = ray.ro_clipA;
        tileRdClipB[i] = ray.rd_clipB;
        rayValid[i] = ((ray.counts.w & 0x40000000u) == 0u) && (ray.rd_clipB.w > ray.ro_clipA.w);
        rEnter[i] = vec4f(1e9f);
        rExit[i] = vec4f(-1e9f);
    }
   ```
   Inside `for (var m = 0u; m < 4u; m++)`:
   - Hoist `let halfVec = min(cur.half.xyz, vec3f(cur.limits.x, cur.limits.y, zEnv));` before the `for (var i = 0u; i < rayCount; i++)` slab loop and use `let half = halfVec[axis];` inside `for (var axis = 0; axis < 3; axis++)`.
   - Use `tileRoClipA[i]`, `tileRdClipB[i]`, and `rayValid[i]` in both the AABB slab loop and the 8-bin wave loop (zero global `tileRayIndices` or `rays` reads inside the `m` loop!).
   - Record per-ray secant-clipped `[iEnter, iExit]` into `rEnter[i][m]` and `rExit[i][m]`:
     - When `abs(dx) < 1e-6`:
       `curActive = true; enter = min(enter, rA[i]); exit = max(exit, rB[i]); rEnter[i][m] = rA[i]; rExit[i][m] = rB[i]; continue;`
     - Across the 8 bins for ray `i`, accumulate `var iEnter = 1e9f; var iExit = -1e9f; var iHit = false;` using the secant-clipped `[sLo, sHi]`, and after the `bin` loop:
       ```wgsl
       if (iHit) {
           curActive = true;
           enter = min(enter, iEnter);
           exit = max(exit, iExit);
           rEnter[i][m] = iEnter;
           rExit[i][m] = iExit;
       }
       ```
     - In the `else if (anyRayHit)` branch:
       ```wgsl
       for (var i = 0u; i < rayCount; i++) {
           if (rHit[i]) {
               enter = min(enter, rA[i]);
               exit = max(exit, rB[i]);
               rEnter[i][m] = rA[i];
               rExit[i][m] = rB[i];
           }
       }
       ```

2. **Per-Ray Consistent Chronological Ordering & Topological Permutation (`2/3/4` Members):**
   Replace the post-loop overlap and routing block in `classifyTilesMain` with:
   ```wgsl
    let activeCount = countOneBits(activeMemberMask);
    var precMask = 0u;
    if (activeCount > 1u) {
        for (var m1 = 0u; m1 < 4u; m1++) {
            if (!memberHit[m1]) { continue; }
            for (var m2 = m1 + 1u; m2 < 4u; m2++) {
                if (!memberHit[m2]) { continue; }
                var pairOrder12 = true;
                var pairOrder21 = true;
                var anyBothHit = false;
                for (var i = 0u; i < rayCount; i++) {
                    let h1 = rEnter[i][m1] <= rExit[i][m1];
                    let h2 = rEnter[i][m2] <= rExit[i][m2];
                    if (h1 && h2) {
                        anyBothHit = true;
                        if (!(rExit[i][m1] + 0.01 <= rEnter[i][m2])) { pairOrder12 = false; }
                        if (!(rExit[i][m2] + 0.01 <= rEnter[i][m1])) { pairOrder21 = false; }
                    }
                }
                if (!anyBothHit) {
                    pairOrder12 = (exitArr[m1] + 0.01 <= enterArr[m2]);
                    pairOrder21 = (exitArr[m2] + 0.01 <= enterArr[m1]);
                }
                if (pairOrder12 && !pairOrder21) {
                    precMask |= (1u << (m1 * 4u + m2));
                } else if (pairOrder21 && !pairOrder12) {
                    precMask |= (1u << (m2 * 4u + m1));
                } else {
                    tileCanOverlap = true;
                }
            }
        }
    }

    if (activeCount == 0u) {
        // wave-proved empty: no producer work needed
    } else if (activeCount == 1u) {
        let onlyMember = firstTrailingBit(activeMemberMask);
        let slot = atomicAdd(&counters.s1Count, 1u);
        s1TileList[slot] = (onlyMember << 28u) | tileId;
    } else if (activeCount == 2u && !tileCanOverlap) {
        let m0 = firstTrailingBit(activeMemberMask);
        let m1 = firstTrailingBit(activeMemberMask & ~(1u << m0));
        let order01 = (precMask & (1u << (m0 * 4u + m1))) != 0u;
        let order10 = (precMask & (1u << (m1 * 4u + m0))) != 0u;
        if (order01 || order10) {
            let m1First = select(0u, 1u, order10 && !order01);
            let slot = atomicAdd(&counters.s2Count, 1u);
            s2TileList[slot] = (m0 << 28u) | (m1 << 24u) | (m1First << 20u) | tileId;
        } else {
            let slot = atomicAdd(&counters.fallbackCount, 1u);
            fallbackTileList[slot] = (activeMemberMask << 28u) | tileId;
        }
    } else if (activeCount >= 3u && !tileCanOverlap) {
        var ord = array<u32, 4>(0u, 0u, 0u, 0u);
        var inDegMask = 0u;
        for (var m = 0u; m < 4u; m++) {
            if (!memberHit[m]) { continue; }
            var inDeg = 0u;
            for (var mPrev = 0u; mPrev < 4u; mPrev++) {
                if (mPrev != m && memberHit[mPrev] && ((precMask & (1u << (mPrev * 4u + m))) != 0u)) {
                    inDeg++;
                }
            }
            if (inDeg < 4u) {
                ord[inDeg] = m;
                inDegMask |= (1u << inDeg);
            }
        }
        if (inDegMask == ((1u << activeCount) - 1u)) {
            let isFour = select(0u, 1u, activeCount == 4u);
            let slot = atomicAdd(&counters.s2Count, 1u);
            s2TileList[slot] = (1u << 21u) | (isFour << 20u) | (ord[0] << 22u) | (ord[1] << 24u) | (ord[2] << 26u) | (ord[3] << 28u) | tileId;
        } else {
            let slot = atomicAdd(&counters.fallbackCount, 1u);
            fallbackTileList[slot] = (activeMemberMask << 28u) | tileId;
        }
    } else {
        let slot = atomicAdd(&counters.fallbackCount, 1u);
        fallbackTileList[slot] = (activeMemberMask << 28u) | tileId;
    }
   ```
   *Note on `enableRouteObserver` in `VolumeRoutedTransport.inc`:* Keep the exact anchor strings (`var activeMemberMask = 0u;`, `        if (anyRayHit && uxMax > uxMin) {`, `    if (activeCount == 0u) {\n`, and `@group(0) @binding(7) var<storage, read_write> counters: DispatchCounters;`) intact, and in `enableRouteObserver` set `var ordered = (activeCount >= 2u && !tileCanOverlap);` so the route observer histogram records all 2/3/4-member ordered tiles under bit 9 (`ordered`)!

3. **Unpack `2/3/4`-Member Ordered Tiles in `s2ProducerMain` (`VolumeRoutedKernels.hpp`):**
   In `s2ProducerMain`:
   ```wgsl
    let packedTile = s2TileList[tileSlot];
    let tileId = packedTile & 0x000FFFFFu;
    let isMulti = ((packedTile >> 21u) & 1u) != 0u;
    let numMembers = select(2u, 3u + ((packedTile >> 20u) & 1u), isMulti);
    var ord = array<u32, 4>(0u, 0u, 0u, 0u);
    if (!isMulti) {
        let m1First = ((packedTile >> 20u) & 1u) != 0u;
        let rawM0 = (packedTile >> 28u) & 0xFu;
        let rawM1 = (packedTile >> 24u) & 0xFu;
        ord[0] = select(rawM0, rawM1, m1First);
        ord[1] = select(rawM1, rawM0, m1First);
    } else {
        ord[0] = (packedTile >> 22u) & 3u;
        ord[1] = (packedTile >> 24u) & 3u;
        ord[2] = (packedTile >> 26u) & 3u;
        ord[3] = (packedTile >> 28u) & 3u;
    }
   ```
   Then loop `for (var memberIdx = 0u; memberIdx < numMembers; memberIdx++) { let member = ord[memberIdx]; ... }` using `var prevMemberMaxS = -1e9f;` outside the `memberIdx` loop and `var currMemberMaxS = prevMemberMaxS;` inside the `memberIdx` loop:
   - Check `if (memberIdx > 0u && l < prevMemberMaxS) { reason |= 64u; break; }`
   - Update `currMemberMaxS = max(currMemberMaxS, r);` on every active piece of `memberIdx`
   - After the piece loop for `memberIdx` completes, set `prevMemberMaxS = currMemberMaxS;`.
   *(Verification note: If any grazing ray across the 64-pose tour, 7 checkpoint views, or 12 roll angles ever triggers `reason 64` with `0.2` secant padding / `0.01` per-ray separation, widen the secant padding in `classifyTilesMain` slightly, e.g. to `0.25` or `0.3`, so `refused_rays == 0` holds across 100% of frames!)*

---

### Deliverable P17.2: `ecConsume` Single-Stream Fast Path (`ray.counts.w == 0u`) & Direct Single-Member Evaluation (`VolumeRoutedTransport.inc`)

In `VolumeRoutedTransport.inc`:
1. In `consumer`, split `ecConsume` on `if (ray.counts.w == 0u)` so that the `98.5%+` of active rays with zero overlaps bypass all `useOverlap`, `overPanel`, `ecOverlaps`, and `mMask` bit-scanning overhead:
   ```wgsl
   if(ray.counts.w==0u){
    for(var s=0u;s<ray.counts.y;s++){
     let sub=ecSingles[s*stride+id];
     if(sub.info.x>=4u){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);return;}
     let lo=sub.span.x;let h=sub.span.y-lo;let subMember=sub.info.x;let mask=1u<<subMember;
     var minus=vec4<f32>(0);var plus=vec4<f32>(0);
     for(var q=0u;q<2u;q++){
      let node=select(gx0,gx1,q==1u);
      EC_EVAL_SINGLE_NODE
      if(q==0u){minus=val;}else{plus=val;}
     }
     state=ecFlow(state,.5*(minus+plus),h);
    }
   }else{
    // existing dual-queue single+overlap loop
   }
   ```
2. When `deduplicate == true`:
   - Replace `EC_EVAL_SINGLE_NODE` with direct single-member invocation (no `mMask` loop or `firstTrailingBit`!):
     ```wgsl
     let sNode = lo + node * h;
     let worldNode = ro + sNode * rd;
     let val = ecSingleMember(worldNode, wo, sNode, subMember);
     ```
   - And inside `EC_EVAL_NODE` (used only in the rare `ray.counts.w > 0u` branch), if `!useOverlap`, also call `val = ecSingleMember(worldNode, wo, sNode, subMember);` directly instead of looping over `mMask`!
   - When `deduplicate == false`, replace `EC_EVAL_SINGLE_NODE` with `let val = ecProjected(ro, rd, wo, lo + node * h, mask);`.

---

### Deliverable P17.3: Hoisted Clamped Half-Extents in Producer Kernels & Active-Member Sweep Loop in `fallbackProducerMain` (`VolumeRoutedKernels.hpp`)

1. In `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain`, hoist `let halfVec = min(m.half.xyz, vec3f(m.limits.x, m.limits.y, zEnv));` before `for (var axis = 0u; axis < 3u; axis++)` and replace the 3 `if (axis == ...)` branches with `let half = halfVec[axis];`.
2. In `fallbackProducerMain` (`VolumeRoutedKernels.hpp`, lines 663–702), replace both inner `for (var m = 0u; m < 4u; m++)` loops inside the `for (var ne = 0u; ne < totalKnots && reason == 0u; ne++)` overlap sweep with `for (var idx = 0u; idx < nAct; idx++) { let m = ord[idx]; ... }`.

---

### Deliverable P17.4: CPU Mirror Sync, Quiet-System Full Hardware Verification & Audit Report

1. **Sync CPU Mirror (`scratch/probes/positive_e2e_cpu_verify.cpp`):**
   - Mirror the updated `classifyTilesMain` and `s2ProducerMain` logic in `positive_e2e_cpu_verify.cpp` and verify that all structural gates (`near_parallel`, `tall_viewport`, `empty_strip`) pass (`exit 0`).
2. **Run Full 10-Gate Native Metal Hardware Verification (`positive_e2e_metal_verify`) on a Quiet System:**
   - Ensure background heavy GPU workloads (like Chrome meetings) are closed/idle if possible so the benchmark captures true uncontended Apple Silicon GPU frame times alongside relative speedups.
   - Verify all 10 hardware gates (`Gate 1`–`Gate 10`), including:
     - `refused_rays == 0` across all 64 tour poses, all 7 checkpoint views, and all 12 roll sweep angles.
     - `100%` win rate on all 7 checkpoint views (`Gate 9`) and all 12 roll sweep angles (`Gate 10`).
     - Record the new cohort breakdown (`Empty`, `S1`, `Ordered-S2+`, `Fallback`) across all 7 checkpoint views and 12 roll sweep angles, specifically highlighting the collapse of `Fallback` in `Checkpoint_View1_Inside_Veil`, `Checkpoint_View3B`, `Checkpoint_View3C`, `Checkpoint_View2`, `Live_23FPS_View_A`, and `Live_Parked_View`.
3. **Run Sequential CTest Verification (`ctest -j1 --output-on-failure`):**
   - Confirm `406 / 406` WebGPU regression checks and `19 / 19` CTest targets pass sequentially.
4. **Publish Audit & Update Handoff:**
   - Write `docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_P17_PER_RAY_ORDERING_MULTI_MEMBER_S2_AND_CONSUMER_FASTPATH_BENCHMARK_2026-11-01.md` and `docs/audits/data/positive_e2e_p17_verification.json`.
   - Append the Phase P17 completion ledger to `agent intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md` and `agent intercom/conversation_links.jsonl`, and commit cleanly to `claude/ray-domain-transport-rung0`.
