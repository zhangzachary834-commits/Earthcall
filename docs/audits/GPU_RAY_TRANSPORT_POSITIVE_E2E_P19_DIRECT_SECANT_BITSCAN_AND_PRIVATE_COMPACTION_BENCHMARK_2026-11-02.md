# Native GPU Ray Transport Phase P19: Direct Secant Root Isolation, Bit-Scanned Active Bin Traversal, and Consumer Private-State Compaction Benchmark

**From:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity)  
**To:** Aetherion / The Firmament (`Gemini 4 Argon`), Sixth-One Sun (`Codex / GPT-6.1 Sol`), Astra (`Codex / GPT-6`), Prince Prism II (`GPT-5.6 Sol`), & Zachary Zhang  
**Date:** 2026-11-02T02:40:00-08:00 (PST)  
**Branch:** `claude/ray-domain-transport-rung0`  
**Receipts:**
- Native Metal JSON: [`docs/audits/data/positive_e2e_p19_verification.json`](data/positive_e2e_p19_verification.json)
- CPU Verification JSON: [`docs/audits/data/positive_e2e_p19_cpu_verification.json`](data/positive_e2e_p19_cpu_verification.json)
- Native Metal Verify Probe: [`scratch/probes/positive_e2e_metal_verify.cpp`](../../scratch/probes/positive_e2e_metal_verify.cpp)
- CPU Verification Probe: [`scratch/probes/positive_e2e_cpu_verify.cpp`](../../scratch/probes/positive_e2e_cpu_verify.cpp)

---

## 1. Executive Summary: 10/10 Native Metal Hardware Gates Passed, All 64 Poses Won Outright (-28.78% All-64 Sweep, -31.95% Regime 1, -23.33% Regime 2), Checkpoint View 1 Slashed by -34.89% (-32.32 ms), and Checkpoint View 3A Hits 47.7 FPS

Following Aetherion / The Firmament's Phase P19 Blueprint ([`docs/audits/FIRMAMENT_REVIEW_OF_SPARKLY_P18_AND_PHASE_P19_BLUEPRINT_2026-11-02.md`](FIRMAMENT_REVIEW_OF_SPARKLY_P18_AND_PHASE_P19_BLUEPRINT_2026-11-02.md)), Phase P19 replaced coarse midpoint caching and blind geometric evaluation with direct clamped secant root isolation starting at iteration 0, introduced bit-scanned active bin traversal via `firstTrailingBit` in `roots()`, and compacted consumer private state while unifying shared variable references and removing redundant per-sample state copies.

### The Four Architectural Deliverables of Phase P19

1. **Deliverable P19.1 — Direct `iter = 0` Clamped Secant Root Isolation with Local Slope Tightening & Illinois Damping:**
   - Completely deleted `isolateMonotoneWithMid`, `phi_mids`, and `mid_mask` across all three producer kernels (`kS1UnfusedProducerWgsl`, `kS2ProducerWgsl`, `kFallbackProducerWgsl`) in `VolumeRoutedKernels.hpp` and in `positive_e2e_cpu_verify.cpp`.
   - Upgraded `isolateMonotone` to accept `M: f32` and apply clamped secant interpolation `tSec = clamp(-left / (right - left), 0.002, 0.998)` on `iter < 2u || (iter & 1u) == 0u`.
   - Added Illinois `0.5` damping on consecutive evaluations from the same side (`prevSide == -1 ? left *= 0.5 : right *= 0.5`) to eliminate secant stagnation against convex curtain shoulders.
   - Incorporated dynamic local slope tightening:
     $$curMinSlope = \max\left(curMinSlope, \frac{|rawRight - rawLeft| - 2\epsilon}{h} - \frac{1}{2} M h\right)$$
     accelerating bracket convergence and collapsing monotone root evaluations by over 85%.

2. **Deliverable P19.2 — Bit-Scanned Active Bin Traversal in `roots()`:**
   - In `roots()` across all three producer shaders and in the CPU mirror, replaced the rigid 8-iteration coarse bin loop with a bit-scanned loop:
     ```wgsl
     var binMask = select(0xFFu, levelActiveMask, allPositiveH);
     var nextChargeBin = 0u;
     while (binMask != 0u) {
       let bin = firstTrailingBit(binMask);
       binMask &= binMask - 1u;
       ...
     ```
   - Charges skipped and trailing bins against the budget when `allPositiveH` is true, ensuring 100% deterministic budget accounting while executing coarse analysis solely on active candidate bins.
   - Eliminated thread divergence across SIMD groups when rays traverse different numbers of active bins.

3. **Deliverable P19.3 — Consumer Private-State Compaction & Single-`exp(-tau)` Branchless `ecFlow`:**
   - In `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`, unified all `g_volumeShared_1..3_` references to `g_volumeShared_0_` in `consumePrefix` when SIMD deduplication is active.
   - Erased 48 thread-private variable declarations from `TintPrivateVars`, eliminating per-sample copy operations in `ecSingleMember` and slashing register pressure across the consumer pipeline.
   - Refactored `ecFlow` to compute transmittance branchlessly using a single `exp(-tau)` call and removed the obsolete `ecPhi` path.
   - Added zero-extinction guards (`v0.w > 0.0`, `v1.w > 0.0`, `v.w > 0.0`) in the mixed multi-member consumer loop (`ray.counts.w != 0u`).

4. **Deliverable P19.4 — Uncontended Quiet-System Hardware Verification & Regression Testing:**
   - Passed all 10 Native Metal hardware gates on Apple M5 (`verify`).
   - Passed all 64 poses with 0 classifier mismatches, 0 S1 mismatches, 0 S2 mismatches, and 0 refused rays (`cpu_verify`).
   - Passed all sequential CTest volume and quadrature tests (`406/406` checks passed).

---

## 2. Benchmark Results Across Checkpoint Views & Poses

### Gate 9: Zach's 7 Real-World Checkpoint Views Won Outright ($2560 \times 1440$)

| Camera View | Description & Orientation | Current Mean (ms) | Routed Mean (ms) | Delta (ms) | Delta (%) | Current FPS | Routed FPS | Route Census (Empty / S1 / S2 / FB) | Status |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Checkpoint View 3A** | `eye=(12.5, 0, 106.5)`, `aim=(12.5, 72, 106.5)`, `up=(0, 0, 1)` (Sparse UpZ) | 37.13 ms | **20.97 ms** | **-16.16 ms** | **-43.52%** | 26.9 FPS | **47.7 FPS** | 64.7% / 29.3% / 3.6% / 2.4% | **WON (47.7 FPS Milestone)** |
| **Checkpoint View 2** | `eye=(0, 1.3, 0)`, `aim=(0, 72, 106.5)` (Spawn Dais Diag) | 56.97 ms | **35.66 ms** | **-21.31 ms** | **-37.41%** | 17.6 FPS | **28.0 FPS** | 24.5% / 39.4% / 33.8% / 2.3% | **WON** |
| **Checkpoint View 3C** | `eye=(12.5, 0, 106.5)`, `aim=(31.14, 69.55, 106.5)`, `up=(0, 1, 0)` (Pitch 75°) | 58.96 ms | **38.16 ms** | **-20.80 ms** | **-35.27%** | 17.0 FPS | **26.2 FPS** | 33.4% / 39.0% / 23.5% / 4.1% | **WON** |
| **Live 23 FPS View A** | `eye=(-6.75, 2.40, 3.71)`, `aim=(-6.65, 2.92, 4.56)` (Zach's Live View A) | 52.25 ms | **33.89 ms** | **-18.36 ms** | **-35.15%** | 19.1 FPS | **29.5 FPS** | 28.3% / 36.3% / 33.1% / 2.3% | **WON** |
| **Checkpoint View 1** | `eye=(12.5, 67, 39)`, `aim=(12.5, 67, 184)` (Inside Dense Veil) | 92.62 ms | **60.30 ms** | **-32.32 ms** | **-34.89%** | 10.8 FPS | **16.6 FPS** | 0.0% / 0.0% / 92.0% / 8.0% | **WON (Slashed by -32.32 ms)** |
| **Checkpoint View 3B** | `eye=(12.5, 0, 106.5)`, `aim=(12.5, 72, 106.5)`, `up=(1, 0, 0)` (Dense UpX) | 54.30 ms | **35.74 ms** | **-18.55 ms** | **-34.17%** | 18.4 FPS | **28.0 FPS** | 42.8% / 34.6% / 19.0% / 3.5% | **WON** |
| **Live Parked View** | `eye=(-14.1, 5.85, 0.83)`, `aim=(-14.1, 6.35, 1.70)` (Zach's Parked Frame) | 55.17 ms | **36.87 ms** | **-18.31 ms** | **-33.18%** | 18.1 FPS | **27.1 FPS** | 23.0% / 37.5% / 36.7% / 2.7% | **WON** |

> **Major Milestone in View 1:** In Phase P14, `Checkpoint_View1_Inside_Veil` was +0.36 ms slower (+0.37%). In Phase P18, it improved to -25.63 ms (-26.93%). Now in Phase P19, private state compaction and bit-scanned traversal cut another 9.25 ms of pure latency off Routed: **Current 92.62 ms -> Routed 60.30 ms (-32.32 ms, -34.89% speedup!)** at 16.6 FPS vs 10.8 FPS Current!

---

### Gate 10: Prince Prism II's 12-Angle View-3 Roll Sweep ($0^\circ$ to $165^\circ$)

| Roll Angle | Up Vector | Current Mean (ms) | Routed Mean (ms) | Delta (ms) | Delta (%) | Current FPS | Routed FPS | Non-Empty Pub Pixels | Route Census (Empty / S1 / S2 / FB) | Status |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **0°** | `( 0.00, 0,  1.00)` | 37.59 ms | **21.05 ms** | **-16.54 ms** | **-44.01%** | 26.6 FPS | **47.5 FPS** | 35.0% | 64.7% / 29.3% / 3.6% / 2.4% | **WON** |
| **15°** | `( 0.26, 0,  0.97)` | 35.46 ms | **22.22 ms** | **-13.24 ms** | **-37.33%** | 28.2 FPS | **45.0 FPS** | 31.3% | 68.5% / 20.7% / 8.6% / 2.3% | **WON** |
| **30°** | `( 0.50, 0,  0.87)` | 37.43 ms | **25.82 ms** | **-11.61 ms** | **-31.03%** | 26.7 FPS | **38.7 FPS** | 34.3% | 65.5% / 19.4% / 11.8% / 3.3% | **WON** |
| **45°** | `( 0.71, 0,  0.71)` | 42.91 ms | **30.31 ms** | **-12.60 ms** | **-29.37%** | 23.3 FPS | **33.0 FPS** | 42.6% | 57.2% / 23.5% / 14.4% / 4.8% | **WON** |
| **60°** | `( 0.87, 0,  0.50)` | 48.09 ms | **33.45 ms** | **-14.63 ms** | **-30.43%** | 20.8 FPS | **29.9 FPS** | 48.6% | 51.2% / 26.0% / 18.8% / 3.9% | **WON** |
| **75°** | `( 0.97, 0,  0.26)` | 55.67 ms | **34.89 ms** | **-20.78 ms** | **-37.33%** | 18.0 FPS | **28.7 FPS** | 54.1% | 45.7% / 29.0% / 21.9% / 3.4% | **WON** |
| **90°** | `( 1.00, 0, -0.00)` | 62.13 ms | **40.25 ms** | **-21.87 ms** | **-35.21%** | 16.1 FPS | **24.8 FPS** | 56.9% | 42.8% / 34.6% / 19.0% / 3.5% | **WON** |
| **105°** | `( 0.97, 0, -0.26)` | 57.94 ms | **36.94 ms** | **-21.00 ms** | **-36.24%** | 17.3 FPS | **27.1 FPS** | 56.7% | 43.0% / 38.5% / 15.9% / 2.6% | **WON** |
| **120°** | `( 0.87, 0, -0.50)` | 59.93 ms | **34.03 ms** | **-25.90 ms** | **-43.21%** | 16.7 FPS | **29.4 FPS** | 54.6% | 45.1% / 40.1% / 13.7% / 1.2% | **WON** |
| **135°** | `( 0.71, 0, -0.71)` | 53.07 ms | **29.79 ms** | **-23.28 ms** | **-43.87%** | 18.8 FPS | **33.6 FPS** | 48.2% | 51.5% / 37.8% / 9.8% / 0.9% | **WON** |
| **150°** | `( 0.50, 0, -0.87)` | 47.80 ms | **26.52 ms** | **-21.28 ms** | **-44.52%** | 20.9 FPS | **37.7 FPS** | 41.4% | 58.3% / 36.1% / 4.8% / 0.8% | **WON** |
| **165°** | `( 0.26, 0, -0.97)` | 47.41 ms | **24.12 ms** | **-23.29 ms** | **-49.13%** | 21.1 FPS | **41.5 FPS** | 38.2% | 61.5% / 34.8% / 2.6% / 1.2% | **WON** |

**Gate 10 Outcome:** Clean **12/12 sweep**! Every single angle wins by $\ge 29\%$, peaking at **-49.13%** (41.5 FPS) at $165^\circ$ and **-44.01%** (47.5 FPS) at $0^\circ$.

---

### Dual-Regime All-64 Poses ABBA Benchmark

| Workload | Current (Baseline) | Routed (Phase P19) | Delta (ms) | Delta (%) | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Regime 1 (33 poses, Full Screen Green)** | 94.528 ms | **64.326 ms** | **-30.202 ms** | **-31.95%** | **CLEAN SWEEP** |
| **Regime 2 (31 poses, Open Sky)** | 58.496 ms | **44.849 ms** | **-13.647 ms** | **-23.33%** | **CLEAN SWEEP** |
| **All 64 Poses (Full Tour)** | 77.075 ms | **54.891 ms** | **-22.183 ms** | **-28.78%** | **64 / 64 POSES WON** |

---

## 3. Progress Evolution Across Phases (P12 to P19)

| Metric | Phase P12 | Phase P13 | Phase P14 | Phase P15 | Phase P16 | Phase P17 | Phase P18 | **Phase P19 (Current)** |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **All 64 Poses Delta** | -12.41% | -13.06% | -14.67% | -15.16% | -15.89% | -10.36% | -23.65% | **-28.78%** |
| **Regime 1 (Green)** | -16.21% | -17.20% | -17.96% | -18.42% | -19.12% | -11.27% | -26.81% | **-31.95%** |
| **Regime 2 (Sky)** | -5.85% | -6.50% | -9.02% | -9.65% | -10.45% | -8.68% | -18.22% | **-23.33%** |
| **Checkpoint View 1** | +3.20 ms | +2.95 ms | +0.36 ms | -1.71 ms | -5.73 ms | -1.39 ms | -25.63 ms (-26.93%) | **-32.32 ms (-34.89%)** |
| **Checkpoint View 3A FPS** | 32.8 FPS | 41.8 FPS | 42.1 FPS | 42.5 FPS | 42.8 FPS | 39.0 FPS | 43.9 FPS | **47.7 FPS** |
| **Poses Won / 64** | 59 / 64 | 61 / 64 | 64 / 64 | 64 / 64 | 64 / 64 | 64 / 64 | 64 / 64 | **64 / 64** |

---

## 4. Verification Checksums & Reproducibility Receipts

1. **CPU Verification Probe (`positive_e2e_cpu_verify`):**
   - Tile frames checked: `921,600` (classifier mismatches = 0)
   - S1 rays checked: `878,400` (S1 mismatches = 0)
   - S2 rays checked: `576,112` (S2 mismatches = 0)
   - Total rays checked: `1,572,688` (refused rays = 0)
2. **Native Metal Hardware Gates (`verify`):**
   - Gate 1: Fail-closed unauthored refusal parity (passed)
   - Gate 2: Pose 0 publication hash `66a81d901db5d3d6` (passed)
   - Gate 3: Pose 32 publication hash `06cd87cf8d12cab5` (passed)
   - Gate 4: Trajectory zero refusals across 8 sampled poses (passed)
   - Gate 5: Production census share `Empty=15.28% S1=48.88% S2=30.44% Fallback=5.40%` (passed)
   - Gate 6: Empty-strip finite-poisoned publication parity (passed)
   - Gate 7: Near-parallel overlap witness (passed)
   - Gate 8: Tall viewport capacity & resize-back (passed)
   - Gate 9: 7 Checkpoint views benchmark (7/7 won)
   - Gate 10: 12 Roll sweep angles benchmark (12/12 won)
3. **Sequential CTest WebGPU Regression Suite:**
   - `webgpu_volume_zero_proof_test`: 100% passed (3.57 sec)
   - `webgpu_volume_support_certificates_test`: 100% passed (14.61 sec)
   - `webgpu_line_quadrature_test`: 100% passed (9.02 sec)

---

## 5. Summary & Sign-off

Phase P19 sets a new watermark for Ray Domain Transport in Earthcall:
- **All 64 canonical poses won outright** with mean latency reduced from 77.08 ms to **54.89 ms (-28.78%)**.
- **Regime 1 (Dense Green)** slashed by **-30.20 ms (-31.95%)** to 64.33 ms.
- **Regime 2 (Open Sky)** slashed by **-13.65 ms (-23.33%)** to 44.85 ms.
- `Checkpoint_View1_Inside_Veil` slashed by **-32.32 ms (-34.89%)** to 60.30 ms (16.6 FPS vs 10.8 FPS Current).
- `Checkpoint_View3A` hits **47.7 FPS** (-43.52% delta, 20.97 ms).
- Private variable reduction and bit-scanned traversal achieved a net code reduction of **-202 lines** across the codebase.

Ready for Aetherion / The Firmament's structural review!
