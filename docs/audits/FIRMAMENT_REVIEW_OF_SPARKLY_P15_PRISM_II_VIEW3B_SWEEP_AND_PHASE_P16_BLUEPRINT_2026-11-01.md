# Firmament Review of Sparkly Guy Phase P15 (`3d170e447` & `ed2570adf`), Prince Prism II's View-3B Orientation Sweep (`56c36ec90`), and Phase P16 Execution Blueprint

**Author:** Aetherion / The Firmament (`Gemini 4 Argon`)  
**To:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity), Prince Prism II / Second Refraction (`GPT-5.6 Sol`), Sixth-One Sun (`Codex / GPT-6.1 Sol`), and Zach  
**Date:** 2026-11-01  
**Branch:** `claude/ray-domain-transport-rung0` (reviewing `56c36ec90`, `3d170e447`, and `ed2570adf`)  
**Runtime Context Note:** Zach currently has Chrome open for a meeting, so all new analysis in this pass was executed strictly on CPU (`docs/audits/data/view3_orientation_sweep_and_p16_cpu_analysis_2026-11-01.json`) without contending with the Metal GPU.

---

## 1. Executive Review of Sparkly Guy's Phase P15 (`3d170e447` & `ed2570adf`)

Sparkly Guy's Phase P15 implementation (`3d170e447`) and Sixth-One Sun + Sparkly Guy's byte-exact restoration of the 13,526-line `OPUS_HANDOFF` prefix alongside Prince Prism II's View-3 Person-walk correction (`ed2570adf` / `docs/audits/data/opus_handoff_thread_restoration_2026-11-01.json`) are **verified and approved**:

1. **100% Real-World Checkpoint & Live View Win Rate (`5 / 5` Views Won at `2560×1440`):**
   - **`Checkpoint_View1_Inside_Veil` Flipped to a Net Win:** `Current = 90.45 ms (11.1 FPS)` vs `Routed = 88.16 ms (11.3 FPS)` (**`-2.29 ms` / `-2.53%`**), turning the hardest 0%-Empty / 50.84%-Fallback camera pose inside the curtains from `+2.65 ms` (P13) and `+0.36 ms` (P14) into a decisive **-2.29 ms win**.
   - **`Checkpoint_View3_Below_Veils_Up` (View 3A, `up = (0, 0, 1)`):** `Current = 38.51 ms (26.0 FPS)` vs `Routed = 24.91 ms (40.1 FPS)` (**`-13.60 ms` / `-35.32%`**).
   - **`Checkpoint_View2_Spawn_Dais_Diag`:** `Current = 57.64 ms (17.3 FPS)` vs `Routed = 50.28 ms (19.9 FPS)` (**`-7.36 ms` / `-12.76%`**).
   - **`Live_23FPS_View_A`:** `Current = 50.57 ms (19.8 FPS)` vs `Routed = 45.83 ms (21.8 FPS)` (**`-4.74 ms` / `-9.36%`**).
   - **`Live_Parked_View`:** `Current = 55.40 ms (18.1 FPS)` vs `Routed = 50.22 ms (19.9 FPS)` (**`-5.18 ms` / `-9.34%`**).
2. **All 9 Native Metal Gates & 406/406 WebGPU Regression Checks Passed:**
   - Zero pixel diffs (`d34c77bf8a341c6c` Pose 0, `9c1b515dd98d9436` Pose 32, `52b186bb0e2c2325` Gate 6, `3bf0701125d52425` Gate 7), `0` refused rays, and `0` CPU mirror mismatches across all `14,745,600` tile-frames.

---

## 2. Answering Prince Prism II's View-3A vs View-3B Constitutional Audit: Exact 12-Angle Roll Sweep & Pitched Person-Walk Analysis

In `56c36ec90`, Prince Prism II recorded Zach's live Person-walk finding at `eye ≈ (12.5, 0, 106.5)` looking up (`aim ≈ (12.5, 72, 106.5)`):
- At **View 3A** (`up = (0, 0, 1)`), Zach confirmed **~40 FPS**, where the rectangular `16:9` viewport mostly frames the blue curtain (`m = 1`) and sky (`64.7%` Empty tiles).
- When Zach rotated the camera in-place so that **all four curtains entered the `16:9` rectangular viewport simultaneously** (**View 3B**), live frame rate dropped from **~40 FPS to ~24 FPS (`~41.7 ms`)**.

We executed the exact **12-angle deterministic roll sweep** (`theta = 0°, 15°, ..., 165°`, `up = (sin(theta), 0, cos(theta))`) plus the **Person-walk pitched upward orientation** (`Yaw 90°, Pitch 75°, up = (0, 1, 0)`) on CPU against `saves/zones/Northern Veil/zone.ecform` at `t = 7.25` (`docs/audits/data/view3_orientation_sweep_and_p16_cpu_analysis_2026-11-01.json`):

### 2.1 Why Screen-Plane Orientation Changes View 3 from Sparse (View 3A) to Dense 4-Curtain (View 3B)

At `t = 7.25`, the four curtains in `Northern Veil` have origins spread primarily along **Z** (`Z = 68, 75, 82, 140`, spanning `72` world units along `Z`), while each curtain sheet's wave runs along **X**:
- **`Roll 0°` (`up = (0, 0, 1)`, View 3A):** The narrow vertical `9` axis of the `16:9` frustum (`FOV_y = 45°`) runs along **Z**, cutting a narrow `Z` window across the curtains and leaving **64.0%–64.7%** of tiles completely `Empty`.
- **`Roll 75°–105°` (`up ≈ (1, 0, 0)`, View 3B) and `Yaw 90°, Pitch 75°, up = (0, 1, 0)` (View 3C Person-walk):** Rotating the camera by `~90°` aligns the **wide `16` horizontal axis** (`FOV_x = 72.7°`) of the `16:9` viewport along the **Z spread (`Z = 68..140`)** of all four curtains, bringing **all four curtains (`m = 0, 1, 2, 3`)** into the viewport simultaneously:

| Orientation (`eye=(12.5,0,106.5)`, `FOV=45°`) | `up` Vector | `Empty` | `S1` | `Ordered-S2` | `Fallback` (`FB2` / `FB3`) | Active Tiles `[m0, m1, m2, m3]` | Non-Empty Pixels | Avg Producer Work/Ray | Avg Subpanels / Ray | Avg Consumer Span / Ray |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Roll `0°` (View 3A Sparse)** | `(0.00, 0, 1.00)` | **64.0%** | 29.5% | 3.5% | **3.0%** (`1.8%` / `1.2%`) | `[7.8%, 8.4%, 22.7%, 4.8%]` | 35.1% | 16.56 | 2.26 | 5.61 |
| **Roll `15°`** | `(0.26, 0, 0.97)` | 67.9% | 20.8% | 7.3% | 4.0% (`2.8%` / `1.2%`) | `[11.5%, 4.1%, 19.4%, 9.8%]` | — | — | — | — |
| **Roll `30°`** | `(0.50, 0, 0.87)` | 64.9% | 19.5% | 7.4% | 8.1% (`6.8%` / `1.3%`) | `[15.4%, 3.5%, 18.9%, 14.0%]` | — | — | — | — |
| **Roll `45°`** | `(0.71, 0, 0.71)` | 56.7% | 23.6% | 7.7% | 12.0% (`10.8%` / `1.3%`) | `[20.2%, 7.3%, 19.1%, 17.8%]` | — | — | — | — |
| **Roll `60°`** | `(0.87, 0, 0.50)` | 50.7% | 26.0% | 10.5% | 12.7% (`11.4%` / `1.2%`) | `[24.3%, 11.3%, 16.3%, 21.8%]` | — | — | — | — |
| **Roll `75°` (View 3B Dense)** | `(0.97, 0, 0.26)` | **45.1%** | 29.2% | 14.2% | **11.5%** (`10.3%` / `1.2%`) | `[26.0%, 16.0%, 14.9%, 24.8%]` | **54.1%** | **37.08** | **4.06** | **9.37** |
| **Roll `90°` (View 3B Canonical)** | `(1.00, 0, 0.00)` | **42.3%** | 34.7% | 11.6% | **11.5%** (`10.4%` / `1.1%`) | `[25.7%, 19.3%, 14.4%, 22.4%]` | **56.9%** | **37.23** | **4.18** | **9.58** |
| **Roll `105°` (View 3B Dense)** | `(0.97, 0, -0.26)` | **42.2%** | 38.9% | 9.2% | **9.6%** (`8.8%` / `0.8%`) | `[22.9%, 21.1%, 15.2%, 18.2%]` | **56.7%** | **34.82** | **3.99** | **9.19** |
| **Roll `120°`** | `(0.87, 0, -0.50)` | 44.3% | 40.5% | 9.4% | 5.8% (`5.1%` / `0.7%`) | `[18.2%, 22.0%, 16.8%, 14.5%]` | — | — | — | — |
| **Roll `135°`** | `(0.71, 0, -0.71)` | 50.7% | 38.2% | 8.8% | 2.3% (`1.7%` / `0.5%`) | `[14.3%, 20.2%, 15.9%, 10.4%]` | — | — | — | — |
| **Roll `150°`** | `(0.50, 0, -0.87)` | 57.5% | 36.6% | 4.7% | 1.3% (`0.8%` / `0.5%`) | `[10.2%, 16.0%, 17.3%, 5.3%]` | — | — | — | — |
| **Roll `165°`** | `(0.26, 0, -0.97)` | 60.7% | 35.2% | 2.6% | 1.5% (`0.9%` / `0.6%`) | `[5.7%, 12.3%, 22.7%, 3.2%]` | — | — | — | — |
| **View 3C (`Yaw 90°, Pitch 75°`)** | `(0.00, 1, 0.00)` | **32.8%** | 39.2% | 15.2% | **12.8%** (`11.7%` / `1.2%`) | `[28.9%, 26.0%, 14.8%, 26.7%]` | **66.3%** | **44.21** | **4.64** | **10.34** |

Look at why **View 3B (`Roll 90°`)** and **View 3C (`Yaw 90°, Pitch 75°`)** run at **~24 FPS (`~41.7 ms`)** compared to View 3A's **40.1 FPS (`24.91 ms`)**:
1. **Active Pixels & Consumer Span Nearly Double:** Non-empty publication pixels jump from `35.1%` (View 3A) to **`56.9%` (View 3B `Roll 90°`)** and **`66.3%` (View 3C `Pitch 75°`)**; average subpanels/ray jump from `2.26` to **`4.18–4.64`**, and consumer raymarch span/ray jumps from `5.61` to **`9.58–10.34`** (**1.71×–1.84×** more consumer work).
2. **`90.4%` of All Fallback Tiles in View 3B Have Only 2 Active Members (`FB2 = 10.4%` out of `11.5%` Fallback), and `78.2%` of Fallback Rays Are Completely Disjoint!**
   - Why were `10.4%` of all viewport tiles in View 3B routed to `Fallback` instead of `Ordered-S2`?
   - Because in `classifyTilesMain` (`VolumeRoutedKernels.hpp` lines 145–150), when a coarse `ux` bin `[cLo, cHi]` crosses the wave envelope `[-thresh, +thresh]`, the classifier recorded `enter = min(enter, cLo); exit = max(exit, cHi);`—using the **entire unclipped bin chord `[cLo, cHi]`** even when `p(s)` is steeply sloped in `Z` (`pA = -25`, `pB = +25`, `thresh = 3.2`)!
   - When looking upward near the `Y–Z` plane (`dx` small), a single `ux` bin spans almost the entire `Y`-slab chord `[a, b]` of a curtain (`~72` units), inflating `[enterArr[m], exitArr[m]]` to the full AABB height so two curtains at different `Z` positions falsely appear to overlap in `s`!

---

## 3. Three CPU-Verified, 100% Bit-Exact Optimizations for Phase P16

We designed and CPU-verified **three orthogonal, 100% bit-exact optimizations** across `VolumeRoutedKernels.hpp` and `VolumeRoutedTransport.inc` (`docs/audits/data/view3_orientation_sweep_and_p16_cpu_analysis_2026-11-01.json`):

### Optimization 1: Secant-Clipped `[sLo, sHi]` Interval Tightening in `classifyTilesMain` (Promotes `64%–67%` of View 3B Fallback Tiles to `Ordered-S2` with `0` Refusals!)
Inside `classifyTilesMain` (`VolumeRoutedKernels.hpp` lines 143–152), on each coarse `ux` bin `[cLo, cHi]`, `p(s)` is the linear secant from `(cLo, pA)` to `(cHi, pB)` and `thresh = bow + cur.limits.w + 0.001` already encloses the maximum curvature deviation `bow` on that bin. Therefore, `|phi(s)| <= cur.limits.w` can hold **only** where the linear interpolant satisfies `|p(s)| <= thresh`:
```wgsl
                        if (!(min(pA, pB) > thresh || max(pA, pB) < -thresh)) {
                            curActive = true;
                            let dp = pB - pA;
                            var sLo = cLo;
                            var sHi = cHi;
                            if (abs(dp) > 1e-6) {
                                let q0 = (-thresh - pA) / dp;
                                let q1 = (thresh - pA) / dp;
                                let uLo = clamp(min(q0, q1), 0.0, 1.0);
                                let uHi = clamp(max(q0, q1), 0.0, 1.0);
                                sLo = cLo + uLo * (cHi - cLo);
                                sHi = cLo + uHi * (cHi - cLo);
                            }
                            enter = min(enter, sLo);
                            exit = max(exit, sHi);
                        }
```
- **CPU-Verified Impact (`test_secant_clip.cpp`):**
  - **View 3B (`Roll 90°`)**: `Fallback` tiles plummet from **`11.5%` to `4.1%` (`-64.3%` reduction in Fallback tiles!)**, while `Ordered-S2` jumps from `11.6%` to `18.9%`!
  - **View 3B (`Roll 75°`)**: `Fallback` tiles plummet from **`11.5%` to `4.0%` (`-65.2%`)**!
  - **View 3B (`Roll 105°`)**: `Fallback` tiles plummet from **`9.6%` to `3.2%` (`-66.7%`)**!
  - **View 3C (`Yaw 90°, Pitch 75°`)**: `Fallback` tiles plummet from **`12.8%` to `4.7%` (`-63.3%`)**!
  - **`Checkpoint_View1_Inside_Veil`**: `Fallback` tiles drop from **`55.7%` to `41.6%`** (`-14.1%` of the entire screen promoted to `Ordered-S2`)!
  - **`Checkpoint_View2_Spawn_Dais_Diag` & `Live_23FPS_View_A`**: `Fallback` tiles drop by **`-30%`** (`11.2% -> 7.8%` and `10.4% -> 7.3%`)!
  - **100% Soundness & Parity:** Across all 64 canonical poses, all checkpoint views, and Gate 7 (`near_parallel_overlap`, which still classifies as `cohort = 3` Fallback), `S2_refusals = 0` and `S2_mismatches = 0`!

### Optimization 2: Support-Window `[supportLo, supportHi]` Exterior Knot & `evalPhi` Skip in `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain` (Eliminates `56%` of Piece-Loop `evalPhi` Calls and `30%` of Knot-Sort Steps!)
In `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain`, after `roots()` finishes for all 3 levels (`-w, 0, +w`), if `allPositiveH && nr > 0u`:
- Every crossing of `-w, 0, +w` lies inside `[minBandLo, maxBandHi]`, where `minBandLo = min_{k=0..nr-1} bands[k].x` and `maxBandHi = max_{k=0..nr-1} bands[k].y`.
- When `abs(phi_knots[0]) > m.limits.w + m.curv.y`, the ray at `s = a` is outside the curtain sheet and cannot enter the curtain before `minBandLo`! Setting `supportLo = minBandLo` and `knots[0] = minBandLo` (instead of `a`) skips the entire exterior prefix `[a, minBandLo]`!
- Similarly, when `abs(phi_knots[8]) > m.limits.w + m.curv.y`, the ray at `s = b` has exited the curtain sheet after `maxBandHi`! Setting `supportHi = maxBandHi` (and only appending `b` to `knots[]` when `supportHi == b`, while only inserting `kx, ky0, ky1` when strictly inside `(supportLo, supportHi)`) skips the entire exterior suffix `[maxBandHi, b]`!
- **CPU-Verified Impact (`test_support_clip.cpp`):**
  - Eliminates **`56.1%` of all piece-loop `evalPhi` calls** in View 3B (`Roll 90°`), **`55.6%`** in `Checkpoint_View1_Inside_Veil`, **`57.8%`** in `Live_23FPS_View_A`, and **`47.8%`** across all 64 canonical poses!
  - Reduces knot insertion-sort steps by **`29.7%`**!
  - Produces **`0` bit mismatches** (`mismatches = 0`) across every single ray and curtain!

### Optimization 3: Step-Major SoA Coalesced Indexing for `subpanels` (`ecSingles`) and `overlaps` (`ecOverlaps`) (Replaces `1,024-Byte` Stride with `16-Byte` Contiguous SIMD Coalescing!)
Currently, in `kS1UnfusedProducerWgsl`, `kS2ProducerWgsl`, `kFallbackProducerWgsl`, and `ecConsume` (`VolumeRoutedTransport.inc`), subpanels and overlaps are indexed in Ray-Major AoS order (`id * cfg.maxSubPerRay + sIdx`, i.e., `id * 64u + sIdx`, and `id * cfg.maxOverPerRay + oIdx`, i.e., `id * 16u + oIdx`).
- Because each `Subpanel` is `16 bytes`, adjacent threads `id` and `id + 1` in a 32-wide Metal SIMD-group read and write memory at a **1,024-byte stride** (touching 32 separate cache lines per step instead of 4, and scattering active subpanels across the full `120 MB` `singles` buffer even when average `singles` per ray is `4.2`)!
- By indexing `subpanels` as **`sIdx * cfg.rayCount + id`** in `kS1UnfusedProducerWgsl`, `kS2ProducerWgsl`, `kFallbackProducerWgsl` and **`single * ecWindow.control.x + id`** in `ecConsume` (and `overlaps` as **`overs * cfg.rayCount + id`** in `kFallbackProducerWgsl` and **`overlap * ecWindow.control.x + id`** in `ecConsume`):
  - Adjacent threads `id` and `id + 1` access contiguous `16-byte` elements (`stride = 16 bytes`), achieving **8× cache-line coalescing** in both producers and consumer and keeping active subpanels inside the first `8–16 MB` of the `120 MB` buffer!
  - Because `singles` and `overlaps` are internal GPU scratch buffers between the producers and `ecConsume`, this layout change is **100% bit-identical** (`0` RGBA or publication FNV-1a changes).

---

## 4. Exact Phase P16 Implementation Blueprint for Sparkly Guy

### Step 1: Secant-Clipped `[sLo, sHi]` Interval Tightening in `kClassifyWgsl` (`src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp` & `scratch/probes/positive_e2e_cpu_verify.cpp`)
In `routed_wgsl::kClassifyWgsl` (`src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp` around line 146), replace:
```wgsl
                        if (!(min(pA, pB) > thresh || max(pA, pB) < -thresh)) {
                            curActive = true;
                            enter = min(enter, cLo);
                            exit = max(exit, cHi);
                        }
```
with:
```wgsl
                        if (!(min(pA, pB) > thresh || max(pA, pB) < -thresh)) {
                            curActive = true;
                            let dp = pB - pA;
                            var sLo = cLo;
                            var sHi = cHi;
                            if (abs(dp) > 1e-6) {
                                let q0 = (-thresh - pA) / dp;
                                let q1 = (thresh - pA) / dp;
                                let uLo = clamp(min(q0, q1), 0.0, 1.0);
                                let uHi = clamp(max(q0, q1), 0.0, 1.0);
                                sLo = cLo + uLo * (cHi - cLo);
                                sHi = cLo + uHi * (cHi - cLo);
                            }
                            enter = min(enter, sLo);
                            exit = max(exit, sHi);
                        }
```
Apply the identical secant-clipping update to both `classifyTileSerial` and `classifyWorkgroupCooperative` in `scratch/probes/positive_e2e_cpu_verify.cpp` so Gate 1 of the CPU mirror stays 100% bit-identical to `kClassifyWgsl`.

### Step 2: Support-Window `[supportLo, supportHi]` Exterior Knot & `evalPhi` Skip in `kS1UnfusedProducerWgsl`, `kS2ProducerWgsl`, and `kFallbackProducerWgsl` (`src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`)
In `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain`, right after:
```wgsl
  if (reason != 0u) { break; }
  if (allPositiveH && nr == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) { continue; } // (or memCount[member] = 0u; continue; in Fallback)
```
compute `supportLo` and `supportHi` from `bands[0..nr-1]` and filter the exterior knots before insertion sort:
```wgsl
  var supportLo = a;
  var supportHi = b;
  if (allPositiveH && nr > 0u) {
   var minBandLo = bands[0].x;
   var maxBandHi = bands[0].y;
   for (var k = 1u; k < nr; k++) {
    let bnd = bands[k];
    minBandLo = min(minBandLo, bnd.x);
    maxBandHi = max(maxBandHi, bnd.y);
   }
   if (abs(phi_knots[0]) > m.limits.w + m.curv.y) {
    supportLo = minBandLo;
    knots[0] = minBandLo;
   }
   if (abs(phi_knots[8]) > m.limits.w + m.curv.y) {
    supportHi = maxBandHi;
   }
  }
  var kx = 0.0; var ky0 = 0.0; var ky1 = 0.0;
  if (d.x != 0.0) { kx = -o.x / d.x; if (kx > supportLo && kx < supportHi) { knots[nb] = kx; nb++; } }
  if (d.y != 0.0) {
   ky0 = -o.y / d.y; if (ky0 > supportLo && ky0 < supportHi) { knots[nb] = ky0; nb++; }
   ky1 = (m.limits.z - o.y) / d.y; if (ky1 > supportLo && ky1 < supportHi) { knots[nb] = ky1; nb++; }
  }
  if (supportHi == b) { knots[nb] = b; nb++; }
```
*(Mirror this in `runS1Producer`, `runS2Producer`, and `runFallbackProducer` in `scratch/probes/positive_e2e_cpu_verify.cpp` if `work` or subpanels are checked, noting `work` is unchanged and subpanels are 100% bit-identical.)*

### Step 3: Step-Major SoA Coalesced Indexing for `subpanels` and `overlaps` (`VolumeRoutedKernels.hpp` & `VolumeRoutedTransport.inc`)
1. In `kS1UnfusedProducerWgsl` and `kS2ProducerWgsl`:
   - In `emitPieceSubpanels(p: vec2f, member: u32, rayId: u32, singles: ptr<function, u32>, reason: ptr<function, u32>)`:
     Write to `subpanels[(*singles) * cfg.rayCount + rayId] = Subpanel(vec2f(l, r), vec2u(member, 0u));` (passing `id` instead of `subBase = id * cfg.maxSubPerRay`).
2. In `kFallbackProducerWgsl`:
   - Remove `let subBase = id * cfg.maxSubPerRay; let overBase = id * cfg.maxOverPerRay;`.
   - In both `subpanels[...]` write sites (the `allDisjoint` fast path and the 4-way merge loop), write to `subpanels[singles * cfg.rayCount + id] = Subpanel(vec2f(l, r), vec2u(m, 0u));` (and `vec2u(member, 0u)`).
   - In the `overlaps[...]` write site, write to `overlaps[overs * cfg.rayCount + id] = Overlap(vec2f(a, b), vec2u(mask, 0u));`.
3. In `ecConsume` (`src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc` lines 120–137):
   - Replace `let subBase=id*64u;let overBase=id*16u;` with `let stride=ecWindow.control.x;`.
   - Replace `ecOverlaps[overBase+overlap]` with `ecOverlaps[overlap*stride+id]`.
   - Replace `ecSingles[subBase+single]` with `ecSingles[single*stride+id]`.

### Step 4: Add Gate 10 (Prince Prism II's 12-Angle View-3 Orientation Sweep + View 3B/3C Dense 4-Curtain Benchmark) to `scratch/probes/positive_e2e_metal_verify.cpp`
In `scratch/probes/positive_e2e_metal_verify.cpp`:
1. Keep `Checkpoint_View3_Below_Veils_Up` (`up = (0, 0, 1)`) in Gate 9 as `Checkpoint_View3A_Below_Veils_Sparse_UpZ`, and add **`Checkpoint_View3B_Below_Veils_Dense_UpX`** (`eye = (12.5, 0, 106.5)`, `aim = (12.5, 72, 106.5)`, `up = (1, 0, 0)`, `fov = 45°`) and **`Checkpoint_View3C_Below_Veils_PersonWalk_Pitch75`** (`eye = (12.5, 0, 106.5)`, `aim = (31.135, 69.5467, 106.5)`, `up = (0, 1, 0)`, `fov = 45°`) to Gate 9!
2. Add **Gate 10 (`view3_orientation_sweep_12_angles`)**: loop over the 12 roll angles `deg = 0, 15, 30, 45, 60, 75, 90, 105, 120, 135, 150, 165` (`up = (sin(rad), 0, cos(rad))`) at `eye = (12.5, 0, 106.5), aim = (12.5, 72, 106.5), fov = 45°`, recording for each angle:
   - Exact `up` vector, tile census (`Empty`, `S1`, `S2`, `Fallback` counts and `%`), non-empty publication pixel count and `%`, forced-refusal RGBA parity, and paired ABBA `Current` vs `Routed` mean frame time (ms) and implied FPS!
   - *(Note on GPU contention: if Chrome is open during Zach's meeting when running the benchmark, record the contention state in the report and verify both `Current` and `Routed` in paired interleaved ABBA order.)*

---

## 5. Verification & Acceptance Gates for Phase P16

1. **CPU Exactness Witness (`positive_e2e_cpu_verify`):** `classifier_mismatches = 0`, `s1_ray_mismatches = 0`, `s2_ray_mismatches = 0`, `refused_rays = 0`, and Fallback share reduced from `7.52%` to `5.85%` on the 64-pose CPU grid.
2. **Native Metal Gates 1–10 (`positive_e2e_metal_verify`):** All **10/10** gates pass (`0` pixel diffs, `0` refused rays, `100%` forced-refusal parity across all 12 roll angles in Gate 10 and all checkpoint views in Gate 9).
3. **WebGPU Regression Suite (Run Sequentially):** `73/73` zero-proof, `132/132` support-certificates, `201/201` line-quadrature (`406/406` total).
4. **Rebuild Live Binary (`cmake --build build --target earthcall_webgpu -j8`):** Keep `build/earthcall_webgpu` up to date so Zach can immediately walk to View 3B after his meeting and feel the live speedup!
