# Firmament Review of Sparkly Guy's Phase P13 (`41.8 FPS` Checkpoint View 3 Win & `9/9` Native Metal Gates) & Phase P14 Blueprint: Active-Member-Masked Streaming `Fallback` Producer and Single-Call-Site Consumer Bit-Iterator

**From:** Aetherion / The Firmament (`Gemini 4 Argon`)  
**To:** Zachary Zhang, Sparkly Guy (`Gemini 3.8 Flash` in Antigravity), Sixth-One Sun (`Codex / GPT-6.1 Sol`), & Astra (`Codex / GPT-6`)  
**Date:** 2026-11-01T18:20:00-08:00 (PST)  
**Branch:** `claude/ray-domain-transport-rung0`  
**Reviewed Commits:**
- `22ace153d` (`feat(ray-transport): Phase P13 SIMD evaluator deduplication, fail-closed encode, and Gate 9 checkpoint views benchmark`)
- `68e1c3290` (`docs: add Phase P13 Northern Veil checkpoint views verification checklist for Zach`)

---

## 1. Executive Summary & Audit Verdict on Sparkly Guy's Phase P13

Sparkly Guy (`Gemini 3.8 Flash`) executed all four tasks of the Phase P13 blueprint (`13cda56d1`), delivering a landmark milestone result on Native Apple M5 Metal:

1. **Zach's `28–30 FPS` Northern Veil Checkpoint Merge Gate Surpassed in Checkpoint View 3 (`41.8 FPS` vs `27.0 FPS` Current):**
   - In **Gate 9 (`Checkpoint_View3_Below_Veils_Up`)**, looking straight up at the four Northern Veil curtains from below (`eye = (12.5, 0, 106.5)`, `aim = (12.5, 72, 106.5)`), `Routed` runs at **`23.92 ms` (`41.8 FPS`)** vs `Current`'s **`37.00 ms` (`27.0 FPS`)** — a **`-13.08 ms (-35.35%)` speedup** that crosses Zach's `28–30 FPS` checkpoint merge threshold with more than `11 FPS` to spare!
   - In **Checkpoint View 2 (`Spawn_Dais_Diag`)** and the **two Live Captured Views (`Live_23FPS_View_A`, `Live_Parked_View`)**, `Routed` beats `Current` by **`-7.76%` to `-11.93%`** (`45.75–49.04 ms` / `20.4–21.9 FPS` vs `49.60–55.55 ms` / `18.0–20.2 FPS`).
2. **Strongest Regime 2 (`-6.50%`) & Dual-Regime (`-12.57%`) Win Across All 64 Canonical Poses:**
   - With `ecSingleMember` SIMD consumer deduplication active, **Regime 2 (Open Sky, 31 poses)** improved from `-5.85%` in P12 to **`-6.50%` (`49.740 ms` vs `53.198 ms`, `-3.458 ms`)**, while **Regime 1 (Full-Screen Green, 33 poses)** achieved **`-16.09%` (`72.204 ms` vs `86.049 ms`, `-13.845 ms`)**, and **All 64 Poses** achieved **`-12.57%` (`61.323 ms` vs `70.137 ms`, `-8.814 ms`)**.
3. **All `9/9` Native Apple M5 Metal Hardware Gates & `406/406` WebGPU Regression Checks Passed:**
   - Bit-exact `publication_fnv1a` and `rgba_fnv1a` hashes (`d34c77bf8a341c6c` at Pose 0, `32fcde809ffdb96d` at Pose 32) were preserved across `ecSingleMember` deduplication.

### Summary Table of Phase P13 Native Metal Results (`docs/audits/data/positive_e2e_p13_verification.json`)

| Workload / Camera View | Current Mean (ms) | Routed P13 Mean (ms) | Delta (ms) | Delta (%) | Implied Current FPS | Implied Routed FPS | Tile Route Split (`Empty / S1 / S2 / FB`) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Checkpoint View 3 (`Below_Veils_Up`)** | `37.00` | **`23.92`** | **`-13.08`** | **`-35.35%`** | `27.0` | **`41.8`** | `64.7% / 29.3% / 3.6% / 2.4%` |
| **Checkpoint View 2 (`Spawn_Dais_Diag`)** | `55.55` | **`48.92`** | **`-6.63`** | **`-11.93%`** | `18.0` | **`20.4`** | `24.5% / 39.4% / 27.5% / 8.6%` |
| **Live View A (`Live_23FPS_View_A`)** | `49.60` | **`45.75`** | **`-3.85`** | **`-7.76%`** | `20.2` | **`21.9`** | `28.3% / 36.3% / 27.2% / 8.2%` |
| **Live Parked View (`Live_Parked_View`)** | `54.26` | **`49.04`** | **`-5.22`** | **`-9.62%`** | `18.4` | **`20.4`** | `23.0% / 37.5% / 30.4% / 9.1%` |
| **Checkpoint View 1 (`Inside_Veil`)** | `87.94` | `90.59` | `+2.65` | `+3.01%` | `11.4` | `11.0` | `0.0% / 0.0% / 49.2% / 50.8%` |
| **All 64 Canonical Poses (Full Tour)** | `70.137` | **`61.323`** | **`-8.814`** | **`-12.57%`** | `14.3` | **`16.3`** | `15.3% / 48.9% / 28.8% / 7.0%` |
| **Regime 1: Full-Screen Green (33 poses)** | `86.049` | **`72.204`** | **`-13.845`** | **`-16.09%`** | `11.6` | **`13.8`** | `0.0% / 45.4% / 46.9% / 7.7%` |
| **Regime 2: Open Sky (31 poses)** | `53.198` | **`49.740`** | **`-3.458`** | **`-6.50%`** | `18.8` | **`20.1`** | `30.8% / 51.6% / 10.2% / 7.3%` |

---

## 2. Deep CPU & WGSL Profiling of Phase P13: Why `Checkpoint_View1_Inside_Veil` Is `+2.65 ms` and How Phase P14 Flips It & Accelerates All Views with 100% Bit-Exact Parity

Look at the Gate 9 table above:
- `Routed` wins **4 of the 5 Gate 9 views** (`View 3` by `-35.35%`, `View 2` by `-11.93%`, `Live Parked` by `-9.62%`, `Live View A` by `-7.76%`), and only trails `Current` by `+2.65 ms (+3.01%)` in the extreme **`Checkpoint_View1_Inside_Veil`** stress test (`49.2% S2`, **`50.8% Fallback = 117,146 Fallback tiles`**).
- Why did `Checkpoint_View1_Inside_Veil` (and the `Fallback` cohort across all views) pay unnecessary GPU work and register spills?
  We ran three targeted C++ CPU probes (`fb_mask_probe`, `fb_detail_probe`, `p14_parity_probe`) against `saves/zones/Northern Veil/zone.ecform` across all 7 canonical, live, and checkpoint views, and uncovered **two major 100% bit-exact optimizations** in `fallbackProducerMain` and `ecConsume`:

### 2.1. Discovery A: `classifyTilesMain` Throws Away `activeMemberMask` When Writing `fallbackTileList`, Forcing `fallbackProducerMain` to Evaluate Proved-Empty Curtains!
Look at `VolumeRoutedKernels.hpp:212, 216`:
- When `classifyTilesMain` routes a tile to `s1TileList`, it packs `(onlyMember << 28u) | tileId`.
- When `classifyTilesMain` routes a tile to `s2TileList`, it packs `(m0 << 28u) | (m1 << 24u) | (m1First << 20u) | tileId`.
- When `classifyTilesMain` routes a tile to `fallbackTileList`, it writes **`fallbackTileList[slot] = tileId;`** — discarding the 4-bit `activeMemberMask` that `classifyTilesMain` just computed!
- As a result, `fallbackProducerMain` (`VolumeRoutedKernels.hpp:845`) blindly loops `for (var member = 0u; member < 4u; member++)` over **all 4 curtains**, re-running AABB clipping, 9-point `evalPhi` (`27` `sin`/`cos` evaluations), `exactThreeLevelMasks`, and `roots()` on curtains that `classifyTilesMain` **already proved 100% wave-empty on the tile**!
- Look at what our CPU probe `fb_mask_probe` measured for the active curtain count on `Fallback` tiles across `Northern Veil`:

| View Name | Total `Fallback` Tiles (640x360) | 2-Member `Fallback` Tiles (`2` Curtains Empty!) | 3-Member `Fallback` Tiles (`1` Curtain Empty!) | 4-Member `Fallback` Tiles | `Fallback` Work Saved by `activeMemberMask` | Subpanel / Overlap Mismatches |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **`Canonical_Pose0_R1`** | `1,574` | **`100.0%`** | `0.0%` | `0.0%` | **`-17.5%`** | **`0` / `25,184` rays** |
| **`Canonical_Pose32_R2`** | `574` | **`52.1%`** | **`47.9%`** | `0.0%` | **`-6.7%`** | **`0` / `9,184` rays** |
| **`Live_23FPS_View_A`** | `1,493` | **`32.4%`** | **`67.6%`** | `0.0%` | **`-4.4%`** | **`0` / `23,888` rays** |
| **`Live_Parked_View`** | `1,539` | **`32.6%`** | **`67.4%`** | `0.0%` | **`-3.3%`** | **`0` / `24,624` rays** |
| **`Checkpoint_View1_Inside_Veil`** | `8,027` | **`27.0%`** | **`62.7%`** | `10.3%` | **`-5.5%`** | **`0` / `128,432` rays** |
| **`Checkpoint_View2_Spawn_Dais`** | `1,617` | **`33.6%`** | **`66.4%`** | `0.0%` | **`-5.6%`** | **`0` / `25,872` rays** |
| **`Checkpoint_View3_Below_Veils`** | `426` | **`60.1%`** | **`39.9%`** | `0.0%` | **`-10.4%`** | **`0` / `6,816` rays** |

**In 6 of the 7 views, `100.0%` of `Fallback` tiles have only 2 or 3 active curtains (`0.0%` have all 4), and in `Checkpoint_View1_Inside_Veil`, `89.7%` of `Fallback` tiles have only 2 or 3 active curtains!**

### 2.2. Discovery B: `fallbackProducerMain` Allocates `1,264 Bytes/Thread` of Private Arrays (`knots[96]`, `pieces[48]`, `bands[32]`, Dead `pieceMembers`) and Runs a 2-Pass Array Copy+Scan When Streaming Uses `0` `knots` Storage!
Look at `fallbackProducerMain` (`VolumeRoutedKernels.hpp:840–965`):
1. `var pieceMembers = vec3u(0u);` and `setMember(&pieceMembers, count, member);` (lines `683–689`, `840`, `902`) are **100% dead code** — `pieceMembers` is written on every piece and **never read anywhere**!
2. Inside the `for (var member = 0u; member < 4u; member++)` loop, `knots` and `bands` are reset per member (`var nb = 1u; knots[0] = a; var nr = 0u;`), so during root-finding they hold at most **one curtain's** knots and bands (`maxNb = 17`, `maxNr = 6` across all views — identical to `s1ProducerMain` and `s2ProducerMain` which use `knots[48]` and `bands[24]`).
3. The **only** reason `fallbackProducerMain` declared `var knots: array<f32, 96>;` (`384` bytes/thread!) instead of `array<f32, 48>` was so that lines `912–931` could copy all `totalKnots = 2 * count` piece endpoints out of `pieces[]` into `knots[0..ne-1]`, and then lines `935–964` could loop `for (var i = 1u; i < ne; i++)` reading `let a = knots[i - 1u]; let b = knots[i];`!
4. **Why copy `pieces[]` into `knots[0..95]` at all when a streaming sweep only needs a single scalar `var prevKnot = 0.0;`?!**
   - In a single streaming loop `for (var ne = 0u; ne < totalKnots && reason == 0u; ne++)`, each iteration picks the smallest next endpoint `bestVal` across the 4 heads `head[0..3]`.
   - When `ne > 0u && bestVal > prevKnot`, the sweep interval is immediately `[a, b] = [prevKnot, bestVal]`!
   - After processing `[prevKnot, bestVal]`, set `prevKnot = bestVal;`!
   - **Zero `knots[]` reads or writes during the merge-sweep!**
   - Even better: if only **1 curtain** produced pieces on that ray (`count == memCount.x || count == memCount.y || count == memCount.z || count == memCount.w`), there are **zero overlaps** and `pieces[0..count-1]` are already sorted for that single curtain `firstTrailingBit(nonEmptyMemberMask)`, so we can emit `subpanels` directly in a 4-line loop and **skip the sweep-line completely**!
   - And because the merge-sweep never touches `knots[]`, `fallbackProducerMain` shrinks its thread-private arrays to match `s1`/`s2`:
     - `knots: array<f32, 96>` $\to$ `array<f32, 48>` (`-192` bytes/thread)
     - `bands: array<vec2f, 32>` $\to$ `array<vec2f, 24>` (`-64` bytes/thread)
     - `pieces: array<vec2f, 48>` $\to$ `array<vec2f, 32>` (`-128` bytes/thread; `max` across all views is `16` pieces)
     - Remove dead `pieceMembers: vec3u` (`-12` bytes/thread)
     - **Total thread-local stack reduction in `fallbackProducerMain`: `-396 bytes per thread` (`-25.3 KB` per 64-thread workgroup)**, slashing register spilling to device memory in `Checkpoint_View1_Inside_Veil` (`50.8%` Fallback tiles)!
5. **Post-`roots()` `nr == 0u` Early Skip across `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain`:**
   - When `(masks.x | masks.y | masks.z) != 0u`, `roots()` runs on the candidate bins. When `roots()` finishes with `reason == 0u` and **`nr == 0u`** (zero root bands found on `[a, b]`) and `allPositiveH && abs(phi_knots[0]) > m.limits.w + m.curv.y`, the entire segment `[a, b]` is mathematically proven to lie strictly outside the curtain sheet!
   - Adding `if (allPositiveH && nr == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) { continue; }` immediately after the 3-level `roots()` loop skips inserting/sorting `kx, ky0, ky1` and skips up to 4 redundant `evalPhi(m, o, d, mid)` evaluations per empty chord!
   - **Verified by `p14_parity_probe` across `244,000` Fallback rays in all 7 views:** **`0` mismatches (`100%` bit-for-bit identical `subpanels`, `overs`, and `reason`)!**

### 2.3. Discovery C: Single-Call-Site `mMask` Bit-Iterator in `ecConsume` (`VolumeRoutedTransport.inc`)
Look at how P13's `ecConsume` in `VolumeRoutedTransport.inc` currently invokes `ecSingleMember`:
```wgsl
  for(var q=0u;q<2u;q++){
   let node=select(select(gx0,.5-sqrt(3.0)/6.0,useOverlap),select(gx1,.5+sqrt(3.0)/6.0,useOverlap),q==1u);
   var val = vec4<f32>(0);
   if (useOverlap) { val = ecProjected(ro, rd, wo, lo + node * h, mask); }
   else { val = ecSingleMember(ro + (lo + node * h) * rd, wo, lo + node * h, subMember); }
   if(q==0u){minus=val;}else{plus=val;}
  }
```
where `ecProjected` loops `for (var m = 0u; m < 4u; m++) { if ((mask & (1u << m)) != 0u) { result += ecSingleMember(world, wo, s, m); } }`.
- Because `ecProjected` has 4 loop iterations calling `ecSingleMember` AND the `else` branch calls `ecSingleMember` directly, the WGSL/Metal compiler sees **two separate branches** inside `for (var q = 0u; q < 2u; q++)` (up to `5 x 2 = 10` inlined call sites of `ecSingleMember` / `cnoise3`), and on 2-curtain overlap subpanels `ecProjected` always loops 4 times (`m = 0..3`) even when only 2 bits of `mask` are set!
- **Look what happens when we replace `if (useOverlap) { ... } else { ... }` with a single `mMask = mask` bit-iterator:**
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
  - When `useOverlap == false` (**99.2% of all subpanels**), `mask = 1u << sub.info.x` has **1 bit set**: `firstTrailingBit(mMask)` extracts `m = sub.info.x`, `mMask &= mMask - 1u` clears `mMask` to `0u`, all 32 SIMD lanes execute `ecSingleMember` once in lockstep, and all 32 lanes break on `if (mMask == 0u)`!
  - When `useOverlap == true` (**0.8% of subpanels**), `mMask` visits only the set bits (`2` iterations for a 2-curtain overlap instead of `4`!) in strictly ascending bit order (`0, 1, 2, 3`).
  - There is now **only 1 call site to `ecSingleMember` in the entire `ecConsume` shader** (`EC_PROJECTED` / `ecProjected` is not even needed when `deduplicate == true`!), cutting inlined `ecSingleMember` / `cnoise3` code expansion by **5x** while preserving **100% IEEE-754 bit-exactness** (`+0.0f + x == x` for all `E, X >= 0.0f`)!

---

## 3. Concrete Phase P14 Implementation Plan for Sparkly Guy (`Gemini 3.8 Flash`)

### Task P14.1: Active-Member-Masked, Stack-Trimmed, Streaming `fallbackProducerMain` & Post-`roots()` `nr == 0u` Early Skip (`VolumeRoutedKernels.hpp`)

1. **In `classifyTilesMain` (`src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp:212, 216`):**
   Pack `(activeMemberMask << 28u) | tileId` into `fallbackTileList[slot]`:
   ```wgsl
           } else {
               let slot = atomicAdd(&counters.fallbackCount, 1u);
               fallbackTileList[slot] = (activeMemberMask << 28u) | tileId;
           }
       } else {
           let slot = atomicAdd(&counters.fallbackCount, 1u);
           fallbackTileList[slot] = (activeMemberMask << 28u) | tileId;
       }
   ```
2. **In `s1ProducerMain` (`VolumeRoutedKernels.hpp:374`) and `s2ProducerMain` (`VolumeRoutedKernels.hpp:574`):**
   Immediately after the `for (var lvl = 0u; lvl < 3u; lvl++)` `roots()` loop (and `if (reason != 0u) { break; }`), add the post-`roots()` zero-band skip:
   ```wgsl
   if (allPositiveH && nr == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) { continue; }
   ```
3. **In `kFallbackProducerWgsl` (`VolumeRoutedKernels.hpp:645–967`):**
   - Change `putBand`, `putBandTagged`, `isolateMonotone`, and `roots` pointer parameter types from `array<f32, 96>` / `array<vec2f, 32>` to `array<f32, 48>` / `array<vec2f, 24>` (with `if (*nb + 2u > 44u || *nr >= 24u) { *reason |= 2u; return; }` in `putBand`, matching `kS1UnfusedProducerWgsl` and `kS2ProducerWgsl`).
   - Delete the dead functions `getMember` and `setMember`.
   - In `fallbackProducerMain`:
     - Unpack `packedTile`:
       ```wgsl
       let packedTile = fallbackTileList[tileSlot];
       let tileId = packedTile & 0x0FFFFFFFu;
       let activeMask = packedTile >> 28u;
       ```
     - Declare trimmed private arrays (`-396 bytes/thread`):
       ```wgsl
       var pieces: array<vec2f, 32>;
       var knots: array<f32, 48>;
       var memStart = vec4u(0u); var memCount = vec4u(0u);
       ```
     - At the top of `for (var member = 0u; member < 4u; member++)`:
       ```wgsl
       memStart[member] = count;
       if ((activeMask & (1u << member)) == 0u) { memCount[member] = 0u; continue; }
       ```
     - Change `var bands: array<vec2f, 32>;` to `var bands: array<vec2f, 24>;`.
     - After the `for (var lvl = 0u; lvl < 3u; lvl++)` `roots()` loop and `if (reason != 0u) { break; }`, add:
       ```wgsl
       if (allPositiveH && nr == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) {
        memCount[member] = 0u;
        continue;
       }
       ```
     - In the piece loop, change `if (count >= 48u) { reason |= 8u; break; }` to `if (count >= 32u) { reason |= 8u; break; }` and remove `setMember(&pieceMembers, count, member);`.
     - Replace the post-member `knots` merge-sort and scan (`lines 912–966`) with the **Zero-`knots`-Array Single-Member Fast Path + Streaming Sweep**:
       ```wgsl
       if (reason != 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason); return; }
       if (count == 0u) { rays[id].counts = vec4u(0, 0, 0, 0u); return; }
       var singles = 0u; var overs = 0u;
       var nonEmptyMask = 0u;
       for (var m = 0u; m < 4u; m++) {
        if (memCount[m] > 0u) { nonEmptyMask |= (1u << m); }
       }
       if (countOneBits(nonEmptyMask) == 1u) {
        let onlyM = firstTrailingBit(nonEmptyMask);
        for (var p = 0u; p < count; p++) {
         let a = pieces[p].x; let b = pieces[p].y;
         let n = max(1u, u32(ceil((b - a) / 4.0)));
         if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
         for (var k = 0u; k < n; k++) {
          let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
          let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
          subpanels[subBase + singles] = Subpanel(vec2f(l, r), vec2u(onlyM, 0u));
          singles++;
         }
        }
        if (reason != 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason); }
        else { rays[id].counts = vec4u(0, singles, 0, 0u); }
        return;
       }
       var head = vec4u(0u);
       var pIdx = vec4u(0u);
       var prevKnot = 0.0;
       let totalKnots = count * 2u;
       for (var ne = 0u; ne < totalKnots && reason == 0u; ne++) {
        var bestM = 4u;
        var bestVal = 0.0;
        for (var m = 0u; m < 4u; m++) {
         let h = head[m];
         if (h < memCount[m] * 2u) {
          let idx = memStart[m] + (h >> 1u);
          let val = select(pieces[idx].x, pieces[idx].y, (h & 1u) != 0u);
          if (bestM == 4u || val < bestVal) {
           bestVal = val;
           bestM = m;
          }
         }
        }
        head[bestM]++;
        if (ne > 0u && bestVal > prevKnot) {
         let a = prevKnot; let b = bestVal;
         let mid = a + 0.5 * (b - a);
         var mask = 0u;
         for (var m = 0u; m < 4u; m++) {
          let start = memStart[m];
          let numPieces = memCount[m];
          var cur = pIdx[m];
          loop {
           if (cur >= numPieces || pieces[start + cur].y >= mid) { break; }
           cur++;
          }
          pIdx[m] = cur;
          if (cur < numPieces) {
           let p = pieces[start + cur];
           if (mid >= p.x && mid <= p.y) { mask |= (1u << m); }
          }
         }
         if (mask != 0u) {
          if (countOneBits(mask) == 1u) {
           let n = max(1u, u32(ceil((b - a) / 4.0)));
           if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }
           let member = firstTrailingBit(mask);
           for (var k = 0u; k < n; k++) {
            let l = select(a + f32(k) * (b - a) / f32(n), a, k == 0u);
            let r = select(a + f32(k + 1u) * (b - a) / f32(n), b, k + 1u == n);
            subpanels[subBase + singles] = Subpanel(vec2f(l, r), vec2u(member, 0u));
            singles++;
           }
          } else {
           if (overs >= cfg.maxOverPerRay) { reason |= 32u; break; }
           overlaps[overBase + overs] = Overlap(vec2f(a, b), vec2u(mask, 0u));
           overs++;
          }
         }
        }
        prevKnot = bestVal;
       }
       if (reason != 0u) { rays[id].counts = vec4u(0, 0, 0, FALLBACK | reason); }
       else { rays[id].counts = vec4u(0, singles, 0, overs); }
       ```

### Task P14.2: Single-Call-Site `mMask` Bit-Iterator in `ecConsume` (`VolumeRoutedTransport.inc`)
In `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`:
When `deduplicate == true`:
- Do not emit `ecProjected` at all (replace `fn ecProjected` block or `EC_PROJECTED` with a trivial unused return, or put `fn ecProjected` inside the `else` branch of `if (deduplicate)`).
- Replace `EC_EVAL_NODE` with the unified single-call-site bit-iterator:
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
- Notice you can also remove `var subMember = 0u;` and `subMember = sub.info.x;` from `consumer` since `mask = 1u << sub.info.x` already carries the single bit for `!useOverlap` and the multi-bit mask for `useOverlap`!

### Task P14.3: Sync CPU Mirror (`positive_e2e_cpu_verify.cpp` & `ray_transport_p1_p11_near_parallel_case.cpp.inc`)
1. In `scratch/probes/positive_e2e_cpu_verify.cpp`:
   - In `classifyTileSerial` (`lines 212, 215`) and `classifyWorkgroupCooperative` (`lines 515, 518`), pack `(activeMemberMask << 28u) | tileId` into `res.packedWord` when `res.cohort == 3u`.
   - In `runS1Producer`, `runS2Producer`, and `runFallbackProducer`, add the `if (allPositiveH && nr == 0u && std::abs(phi_knots[0]) > m.limits.w + m.curv.y)` early continue after the 3 `roots()` calls, and in `main()` pass `coopRes[lt].packedWord >> 28u` (`coopRes[lt].activeMemberMask`) to `runFallbackProducer(ray, specs, coopRes[lt].activeMemberMask)` so CPU verification checks the exact active-member-masked Fallback producer!
2. In `scratch/probes/ray_transport_p1_p11_near_parallel_case.cpp.inc`:
   - Update `serial_packed` check if any assertion checks `serial.packedWord` (note: `ray_transport_p1_p11_near_parallel_case.cpp.inc` only asserts `serial.cohort == 3 && coop[0].cohort == 3`, so it will pass automatically!).

### Task P14.4: Sequential Verification & Native M5 Metal Benchmark (`positive_e2e_p14_verification.json`)
Run sequentially (never in parallel):
1. `python3 scratch/probes/ray_transport_p1_p11_structural_review.py --out /private/tmp/ec-p1-p14-review` and copy `/private/tmp/ec-p1-p14-review/structural_review_result.json` to `docs/audits/data/ray_transport_p1_p14_structural_review_2026-11-01.json`.
2. Compile and run `positive_e2e_cpu_verify` -> `docs/audits/data/positive_e2e_p14_cpu_verification.json` (`0` mismatches, `0` refused rays).
3. Rebuild `/private/tmp/ec-positive-e2e-verify/verify` and `build/earthcall_webgpu` (`RelWithDebInfo` / `-O3 -DNDEBUG`), and run `/private/tmp/ec-positive-e2e-verify/verify "saves/zones/Northern Veil/zone.ecform" docs/audits/data/positive_e2e_p14_verification.json`.
4. Confirm all **9/9 Native Metal Gates** pass with **bit-exact `publication_fnv1a` and `rgba_fnv1a` parity** (`d34c77bf8a341c6c` at Pose 0, `32fcde809ffdb96d` at Pose 32), compare the Gate 9 timings (especially `Checkpoint_View1_Inside_Veil`, `Checkpoint_View2_Spawn_Dais_Diag`, `Live_23FPS_View_A`, `Live_Parked_View`, and `Checkpoint_View3_Below_Veils_Up`) and All-64 timings against P13, and run the 3 WebGPU unit test binaries (`406/406` checks).

---

**Signed:** Aetherion / The Firmament (`Gemini 4 Argon`) · 2026-11-01T18:20:00-08:00 (PST) · Human origin: Zachary Zhang
