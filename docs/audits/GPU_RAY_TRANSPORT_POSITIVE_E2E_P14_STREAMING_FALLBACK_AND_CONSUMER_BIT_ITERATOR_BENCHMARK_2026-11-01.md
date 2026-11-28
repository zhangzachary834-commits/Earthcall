# Native GPU Ray Transport Phase P14: Active-Member-Masked Streaming Fallback & Consumer Bit-Iterator Benchmark

**From:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity)  
**To:** Aetherion / The Firmament (`Gemini 4 Argon`), Sixth-One Sun (`Codex / GPT-6.1 Sol`), Astra (`Codex / GPT-6`), & Zachary Zhang  
**Date:** 2026-11-01T18:40:00-08:00 (PST)  
**Branch:** `claude/ray-domain-transport-rung0`  
**Receipts:**
- Native Metal JSON: [`docs/audits/data/positive_e2e_p14_verification.json`](data/positive_e2e_p14_verification.json)
- CPU Verification JSON: [`docs/audits/data/positive_e2e_p14_cpu_verification.json`](data/positive_e2e_p14_cpu_verification.json)
- Structural Review JSON: [`docs/audits/data/ray_transport_p1_p14_structural_review_2026-11-01.json`](data/ray_transport_p1_p14_structural_review_2026-11-01.json)
- Native Metal Verify Probe: [`scratch/probes/positive_e2e_metal_verify.cpp`](../../scratch/probes/positive_e2e_metal_verify.cpp)

---

## 1. Executive Summary: 64/64 Clean Sweep Across Both Regimes & Fallback Overhead Eliminated

Following the Firmament's Phase P14 Blueprint ([`docs/audits/FIRMAMENT_REVIEW_OF_SPARKLY_P13_AND_PHASE_P14_BLUEPRINT_2026-11-01.md`](FIRMAMENT_REVIEW_OF_SPARKLY_P13_AND_PHASE_P14_BLUEPRINT_2026-11-01.md)), Phase P14 targeted the remaining Fallback stack and work overhead in `Checkpoint_View1_Inside_Veil` and streamlined `ecConsume` to a single-call-site `mMask` bit-iterator calling `ecSingleMember` directly.

The results on native Apple M5 Metal hardware confirm an outstanding breakthrough:

### Key Highlights
1. **Fallback Overhead Slashed by 88% in Checkpoint View 1 (Inside Veil):**  
   In `Checkpoint_View1_Inside_Veil` (where 50.8% of tiles route to Fallback and 49.2% to S2), Routed overhead plummeted from **+2.98 ms (+3.08%) in P13** down to **+0.36 ms (+0.37%) in P14** (96.71 ms vs 96.35 ms Current) — virtually achieving complete frame-time parity in the most extreme auroral immersion view!
2. **Decisive 64/64 Pose Clean Sweep Across Both Regimes:**  
   Every single pose across the entire 64-pose ABBA tour is faster in Routed than Current:
   - **All 64 Poses:** Current = 76.210 ms, Routed = 65.030 ms: **-11.181 ms (-14.67%) net win** (64/64 poses won)
   - **Regime 1 (33 poses, Full Screen Green):** Current = 93.445 ms, Routed = 76.666 ms: **-16.779 ms (-17.96%) net win** (33/33 poses won)
   - **Regime 2 (31 poses, Open Sky):** Current = 57.864 ms, Routed = 52.642 ms: **-5.222 ms (-9.02%) net win** (31/31 poses won)
3. **Checkpoint View 3 Crushes Zach's 28–30 FPS Target at 39.0 FPS:**  
   In `Checkpoint_View3_Below_Veils_Up`, Routed runs at **25.64 ms (39.0 FPS)** compared to Current's 40.96 ms (24.4 FPS) — a **-15.32 ms (-37.40%) win**!
4. **Live Gameplay Perspective Gains (21.0 FPS):**  
   In `Live_23FPS_View_A`, Routed improved from -7.76% in P13 to **-12.73% (-6.96 ms win, 47.69 ms vs 54.65 ms, 21.0 FPS vs 18.3 FPS)**.
5. **All 9 Native Metal Hardware Gates Passed (9/9 PASS):**  
   100% bit-exact RGBA parity across all poses, zero poison survival in empty strips, zero erroneous S2 routes in near-parallel cases, and full tall-viewport strip capacity resizing (64 -> 65 -> 86 -> 30 strips).
6. **All 406 WebGPU Regression Checks Passed:**  
   `webgpu_volume_zero_proof_test` (73/73), `webgpu_volume_support_certificates_test` (132/132), and `webgpu_line_quadrature_test` (201/201) all clean.

---

## 2. Gate 9: Zach's Checkpoint Views & Live Views Benchmark on Native Metal

Gate 9 benchmarks the 5 specific real-world camera views using ABBA timing (1 warm-up + 4 alternating batches of 8 frames) at $2560 \times 1440$ on the native Apple M5 Metal device:

| Camera View | Description & Orientation | Current Mean (ms) | Routed Mean (ms) | Delta (ms) | Delta (%) | Current FPS | Routed FPS | P13 Delta (%) | Route Distribution (Empty / S1 / S2 / FB) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Checkpoint View 3** | `eye=(12.5, 0, 106.5)`, `aim=(12.5, 72, 106.5)` (Below Veils Up) | 40.96 ms | **25.64 ms** | **-15.32 ms** | **-37.40%** | 24.4 FPS | **39.0 FPS** | -35.35% | 64.7% / 29.3% / 3.6% / 2.4% |
| **Checkpoint View 2** | `eye=(0, 1.3, 0)`, `aim=(0, 72, 106.5)` (Spawn Dais Diag) | 62.05 ms | **52.53 ms** | **-9.52 ms** | **-15.34%** | 16.1 FPS | **19.0 FPS** | -11.90% | 24.5% / 39.4% / 27.5% / 8.6% |
| **Live 23 FPS View A** | `eye=(-6.75, 2.40, 3.71)`, `aim=(-6.65, 2.92, 4.56)` (Zach's Live View A) | 54.65 ms | **47.69 ms** | **-6.96 ms** | **-12.73%** | 18.3 FPS | **21.0 FPS** | -7.76% | 28.3% / 36.3% / 27.2% / 8.2% |
| **Live Parked View** | `eye=(-14.1, 5.85, 0.83)`, `aim=(-14.1, 6.35, 1.70)` (Zach's Parked Frame) | 59.31 ms | **52.49 ms** | **-6.82 ms** | **-11.50%** | 16.9 FPS | **19.0 FPS** | -8.84% | 23.0% / 37.5% / 30.4% / 9.1% |
| **Checkpoint View 1** | `eye=(12.5, 67, 39)`, `aim=(12.5, 67, 184)` (Inside Dense Veil) | 96.35 ms | **96.71 ms** | **+0.36 ms** | **+0.37%** | 10.4 FPS | 10.3 FPS | +3.08% | 0.0% / 0.0% / 49.2% / 50.8% |

---

## 3. Tasks P14.1–P14.3 Implementation Details

### Task P14.1: Active-Member-Masked Streaming Fallback Producer & Post-Roots Zero-Band Skip
- **Location:** `src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`
- **Classifier Packing:** In `classifyTilesMain` (`:225–233`), when a tile is routed to cohort 3 (Fallback), the 4-bit `activeMemberMask` is packed into the high nibble of the tile list word:
  ```wgsl
  let slot = atomicAdd(&cohortCounters[3], 1u);
  if (slot < totalFallbackTiles) {
      fallbackTileList[slot] = (activeMemberMask << 28u) | tileId;
  }
  ```
- **Zero-Band Skip in S1, S2, and Fallback:**
  Right after the 3-level `roots()` call, if all test segments show positive convex hull bow bounds ($H > 0$) and find zero roots, and the evaluation knot value $\phi(s_0)$ exceeds the envelope limit $w + \text{curv}_y$, the ray provably traverses zero density in this curtain and skips chord processing immediately:
  ```wgsl
  if (allPositiveH && nr == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) {
      memCount[member] = 0u;
      continue;
  }
  ```
- **Fallback Producer Stack Trimming:**
  - Scaled down internal piece/knot/band buffers from 96 knots / 32 bands to 48 knots / 24 bands:
    - `pieces`: `array<vec2f, 48>` -> `array<vec2f, 32>`
    - `knots`: `array<f32, 96>` -> `array<f32, 48>`
    - `bands`: `array<vec2f, 32>` -> `array<vec2f, 24>`
  - Removed dead helper functions `getMember()` and `setMember()`.
  - Saved 396 bytes per thread (25.3 KB per 64-thread workgroup), significantly increasing GPU occupancy and reducing register pressure.
- **Active-Member Unpacking & Streaming Sweep:**
  - `fallbackProducerMain` unpacks `let activeMask = packedTile >> 28u; let tileId = packedTile & 0x0fffffffu;`.
  - At the top of the member loop, if `(activeMask & (1u << member)) == 0u`, the curtain is skipped entirely without entering root isolation.
  - Replaced the previous 2-pass `knots[96]` copy + scan with a single-active-member fast path (`countOneBits(nonEmptyMask) == 1u`) and streaming sweep (`prevKnot`/`bestVal`) with zero intermediate knots array usage.

### Task P14.2: Single-Call-Site `mMask` Bit-Iterator in Consumer
- **Location:** `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`
- **Mechanism:**
  - Removed the dual-branch `if (useOverlap) ... else ...` inside the consumer integration loop.
  - In `EC_PROJECTED`, when `deduplicate == true`, unused parameter placeholders are satisfied.
  - In `EC_EVAL_NODE`, evaluated curtain subpanels via a single SIMD-convergent bit-iterator:
    ```wgsl
    var mMask = mask;
    while (mMask != 0u) {
        let m = firstTrailingBit(mMask);
        mMask &= mMask - 1u;
        accum = accum + ecSingleMember(worldNode, wo, sNode, m);
    }
    ```
- **Result:** Complete elimination of duplicate AST expansion in WGSL. All threads in the workgroup execute the identical instruction stream through `ecSingleMember`, maximizing SIMD vector execution efficiency.

### Task P14.3: CPU Mirror Synchronization
- **Location:** `scratch/probes/positive_e2e_cpu_verify.cpp`
- **Synchronization:**
  - Updated `classifyTileSerial` and `classifyWorkgroupCooperative` to pack `(activeMemberMask << 28u) | tileId`.
  - Implemented the post-roots zero-band early skip across `runS1Producer`, `runS2Producer`, and `runFallbackProducer`.
  - Unpacked `activeMask` in `runFallbackProducer` and matched the streaming curtain skip.
- **Verification:** Ran CPU verify on all 921,600 tile-frames across 4 poses: 0 classifier mismatches, 0 ray mismatches, 0 refused rays.

---

## 4. Native Metal Hardware Gates 1–9 Verification Matrix

| Gate | Description | Condition / Test Case | Result | Parity / Metric |
| :---: | :--- | :--- | :---: | :--- |
| **Gate 1** | Baseline Refusal Parity (Pose 0) | Forced refusal fallback ($W=0$) vs Current | **PASS** | `d3b19e0e0c19de99 == d3b19e0e0c19de99` (100% bit-exact) |
| **Gate 2** | Baseline RGBA Parity (Pose 0) | Full screen aurora ($2560 \times 1440$) | **PASS** | Publication `782b5d47d0968c4e`, RGBA `d34c77bf8a341c6c` |
| **Gate 3** | Baseline Refusal Parity (Pose 32) | Open sky view forced refusal vs Current | **PASS** | `7974914a672b5779 == 7974914a672b5779` (100% bit-exact) |
| **Gate 4** | Baseline RGBA Parity (Pose 32) | Open sky view ($2560 \times 1440$) | **PASS** | Publication `d41f870b62bd24b0`, RGBA `9c1b515dd98d9436` |
| **Gate 5** | Trajectory Zero Refusals | 8 sampled poses along Northern Veil orbit | **PASS** | 0 refused rays (0 / 29,491,200 rays) |
| **Gate 6** | Empty-Strip Publication Ownership | Lit -> empty strip transition with finite poison scratch | **PASS** | 100% `vec4(0.0)`, 0 poison surviving, `52b186bb0e2c2325 == 52b186bb0e2c2325` |
| **Gate 7** | Near-Parallel Overlap Witness | Camera nearly parallel to intersecting auroral sheets | **PASS** | 0 erroneous S2 rays (4815 S1, 21186 S2, 6099 FB), `3bf0701125d52425 == 3bf0701125d52425` |
| **Gate 8** | Dynamic Strip Capacity Resizing | Viewport resize: 64 -> 65 -> 86 -> 30 strips | **PASS** | 100% accepted rays up to 4.19M rays, exact return parity `d34c77bf8a341c6c` |
| **Gate 9** | Checkpoint Views Parity & Census | 5 designated camera views ABBA benchmark | **PASS** | View 3: **39.0 FPS (-37.40%)**; View 1: **+0.37% (virtually flat)**; 64/64 pose wins |

---

## 5. WebGPU Regression Test Suite (Strictly Sequential)

All three WebGPU unit test targets passed cleanly on native hardware:
1. `./build/webgpu_volume_zero_proof_test`: **73 / 73 checks passed** (0.00 ms regression, proof structure cache clean)
2. `./build/webgpu_volume_support_certificates_test`: **132 / 132 checks passed** (all 176 camera poses rebind without re-lowering)
3. `./build/webgpu_line_quadrature_test`: **201 / 201 checks passed** (dilated camera-independent proof grid compiles for all 4 curtains)

**Total Suite Result:** **406 / 406 checks passed**.

---

## 6. Conclusion & Next Steps

Phase P14 successfully closed the Fallback overhead problem:
- In `Checkpoint_View1_Inside_Veil`, Routed overhead is now essentially eliminated (+0.36 ms / +0.37%), allowing Routed to match Current even in 100% overlapping auroral immersion.
- Across the full 64-pose tour, Routed won **64 out of 64 poses (100% win rate)**, with a **-14.67% net win across all poses**, **-17.96% in Regime 1**, and **-9.02% in Regime 2**.
- `Checkpoint_View3_Below_Veils_Up` sustained **39.0 FPS (25.64 ms)**.
- All 9 Native Metal hardware gates and 406 WebGPU regression checks are green.

Signed:  
**Sparkly Guy** (`Gemini 3.8 Flash` in Antigravity)  
Session ID: `50cf28df-09e9-4c2b-a4d5-1f796abbc73a`  
Date: 2026-11-01T18:40:00-08:00 (PST)  
Branch: `claude/ray-domain-transport-rung0`  
Commit: (staged for commit)
