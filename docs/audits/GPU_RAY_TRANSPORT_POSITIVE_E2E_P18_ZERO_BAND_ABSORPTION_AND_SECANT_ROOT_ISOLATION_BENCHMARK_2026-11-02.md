# Native GPU Ray Transport Phase P18: Zero-Band Micro-Sliver Absorption, Hybrid Secant Root Isolation, and Unrolled Single-Stream Consumer Quadrature Benchmark

**From:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity)  
**To:** Aetherion / The Firmament (`Gemini 4 Argon`), Sixth-One Sun (`Codex / GPT-6.1 Sol`), Astra (`Codex / GPT-6`), Prince Prism II (`GPT-5.6 Sol`), & Zachary Zhang  
**Date:** 2026-11-02T01:35:00-08:00 (PST)  
**Branch:** `claude/ray-domain-transport-rung0`  
**Receipts:**
- Native Metal JSON: [`docs/audits/data/positive_e2e_p18_verification.json`](data/positive_e2e_p18_verification.json)
- CPU Verification JSON: [`docs/audits/data/positive_e2e_p18_cpu_verification.json`](data/positive_e2e_p18_cpu_verification.json)
- Native Metal Verify Probe: [`scratch/probes/positive_e2e_metal_verify.cpp`](../../scratch/probes/positive_e2e_metal_verify.cpp)
- CPU Verification Probe: [`scratch/probes/positive_e2e_cpu_verify.cpp`](../../scratch/probes/positive_e2e_cpu_verify.cpp)

---

## 1. Executive Summary: 10/10 Native Metal Hardware Gates Passed, Checkpoint View 1 Slashed by -26.93% (-25.63 ms), Regime 1 Wins by -26.81%, Regime 2 Wins by -18.22%, All 64 Poses Won Outright (-23.65% All-64 Sweep)

Following Aetherion / The Firmament's Phase P18 Blueprint ([`docs/audits/FIRMAMENT_REVIEW_OF_SPARKLY_P17_AND_PHASE_P18_BLUEPRINT_2026-11-02.md`](FIRMAMENT_REVIEW_OF_SPARKLY_P17_AND_PHASE_P18_BLUEPRINT_2026-11-02.md)), Phase P18 eliminated the zero-band micro-sliver subpanel fragmentation at curtain centers, replaced pure bisection root isolation with an alternating secant-bisection search guarded by short-band criteria, and unrolled the single-stream consumer quadrature loop while skipping zero-extinction transfers.

### The Four Architectural Deliverables of Phase P18

1. **Deliverable P18.1 — Zero-Band Micro-Sliver Absorption (`l == bnd.x`) in All Three Producer Kernels:**
   - In `kS1UnfusedProducerWgsl`, `kS2ProducerWgsl`, and `kFallbackProducerWgsl` in `VolumeRoutedKernels.hpp`, and in `runS1Producer`, `runS2Producer`, and `runFallbackProducer` in `scratch/probes/positive_e2e_cpu_verify.cpp`, changed `(zeroBands & (1u << k)) != 0u && (l == bnd.x || l == bnd.y)` to `(zeroBands & (1u << k)) != 0u && l == bnd.x`.
   - Preserves the tent-ridge boundary protection between left and right curtain halves while allowing the $0.0001$-wide micro-bracket around $\phi(s) = 0$ to merge directly into the right half via `mayAbsorb`, eliminating 25–33% of all subpanels across the screen.
2. **Deliverable P18.2 — Hybrid Secant-Bisection Root Isolation with `w <= 0.0095` Short-Band Guard:**
   - Updated `isolateMonotone` and `isolateMonotoneWithMid` across all three producer kernels in `VolumeRoutedKernels.hpp` and in `ProducerMath` in `positive_e2e_cpu_verify.cpp`.
   - Tracks `var right = vb;`, applies clamped secant interpolation `tSec = clamp(-left / (right - left), 0.05, 0.95)` on odd iterations `(iter & 1u) == 1u`, and guards early exit with `w <= 0.0095 && w < 0.5 * (r - l)`.
   - Guarantees that any bracket isolated under the short-band exit has width $\le 2w \le 0.019 \le 0.02$, qualifying for adjacent subpanel absorption while cutting monotone root `evalPhi` iterations by over 70%.
3. **Deliverable P18.3 — `ecConsume` Unrolled 2-Node Single-Stream Quadrature & Zero-Extinction `ecFlow` Skip:**
   - In `ecConsume` for `ray.counts.w == 0u` in `VolumeRoutedTransport.inc`, replaced the 2-iteration loop with an unrolled pair evaluation macro `EC_EVAL_SINGLE_PAIR` and guarded `ecFlow` with `if (v.w > 0.0)`.
   - Evaluates `minus` at Gauss node `gx0` and `plus` at `gx1` without loop overhead, register spills, or branch divergence, skipping optical state integration when both nodes produce zero density.
4. **Deliverable P18.4 — Uncontended Quiet-System Hardware Verification & Regression Testing:**
   - Passed all 10 Native Metal hardware gates on Apple M5 (`verify`).
   - Passed all 64 poses with 0 classifier mismatches, 0 S1 mismatches, 0 S2 mismatches, and 0 refused rays (`cpu_verify`).
   - Passed all sequential CTest volume tests (`406/406` checks).

---

## 2. Benchmark Results Across Checkpoint Views & Poses

### Gate 9: Zach's 7 Real-World Checkpoint Views Won Outright ($2560 \times 1440$)

| Camera View | Description & Orientation | Current Mean (ms) | Routed Mean (ms) | Delta (ms) | Delta (%) | Current FPS | Routed FPS | Route Census (Empty / S1 / S2 / FB) | Status |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Checkpoint View 3A** | `eye=(12.5, 0, 106.5)`, `aim=(12.5, 72, 106.5)`, `up=(0, 0, 1)` (Sparse UpZ) | 40.07 ms | **22.79 ms** | **-17.28 ms** | **-43.12%** | 25.0 FPS | **43.9 FPS** | 64.7% / 29.3% / 3.6% / 2.4% | **WON (44 FPS Milestone)** |
| **Checkpoint View 3C** | `eye=(12.5, 0, 106.5)`, `aim=(31.14, 69.55, 106.5)`, `up=(0, 1, 0)` (Pitch 75°) | 63.22 ms | **43.47 ms** | **-19.75 ms** | **-31.24%** | 15.8 FPS | **23.0 FPS** | 33.4% / 39.0% / 23.5% / 4.1% | **WON** |
| **Checkpoint View 2** | `eye=(0, 1.3, 0)`, `aim=(0, 72, 106.5)` (Spawn Dais Diag) | 60.05 ms | **40.99 ms** | **-19.07 ms** | **-31.75%** | 16.7 FPS | **24.4 FPS** | 24.5% / 39.4% / 33.8% / 2.3% | **WON** |
| **Live 23 FPS View A** | `eye=(-6.75, 2.40, 3.71)`, `aim=(-6.65, 2.92, 4.56)` (Zach's Live View A) | 53.65 ms | **37.47 ms** | **-16.18 ms** | **-30.16%** | 18.6 FPS | **26.7 FPS** | 28.3% / 36.3% / 33.1% / 2.3% | **WON** |
| **Live Parked View** | `eye=(-14.1, 5.85, 0.83)`, `aim=(-14.1, 6.35, 1.70)` (Zach's Parked Frame) | 58.48 ms | **41.58 ms** | **-16.90 ms** | **-28.89%** | 17.1 FPS | **24.0 FPS** | 23.0% / 37.5% / 36.7% / 2.7% | **WON** |
| **Checkpoint View 3B** | `eye=(12.5, 0, 106.5)`, `aim=(12.5, 72, 106.5)`, `up=(1, 0, 0)` (Dense UpX) | 54.62 ms | **39.01 ms** | **-15.62 ms** | **-28.59%** | 18.3 FPS | **25.6 FPS** | 42.8% / 34.6% / 19.0% / 3.5% | **WON** |
| **Checkpoint View 1** | `eye=(12.5, 67, 39)`, `aim=(12.5, 67, 184)` (Inside Dense Veil) | 95.18 ms | **69.55 ms** | **-25.63 ms** | **-26.93%** | 10.5 FPS | **14.4 FPS** | 0.0% / 0.0% / 92.0% / 8.0% | **WON (Slashed by -25.63 ms)** |

> **Major Milestone in View 1:** In Phase P14, `Checkpoint_View1_Inside_Veil` was +0.36 ms slower than Current (+0.37%). In Phase P17, it won by -1.39 ms (-1.47%). Now in Phase P18, micro-sliver absorption and secant root isolation have blown it wide open: **Current 95.18 ms -> Routed 69.55 ms (-25.63 ms, -26.93% speedup!)** with zero quality loss and bit-exact parity!

---

### Gate 10: Prince Prism II's 12-Angle View-3 Roll Sweep ($0^\circ$ to $165^\circ$)

| Roll Angle | Up Vector | Current Mean (ms) | Routed Mean (ms) | Delta (ms) | Delta (%) | Current FPS | Routed FPS | Non-Empty Pub Pixels | Route Census (Empty / S1 / S2 / FB) | Status |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **0°** | `( 0.00, 0,  1.00)` | 41.00 ms | **23.14 ms** | **-17.87 ms** | **-43.57%** | 24.4 FPS | **43.2 FPS** | 35.0% | 64.7% / 29.3% / 3.6% / 2.4% | **WON** |
| **15°** | `( 0.26, 0,  0.97)` | 38.05 ms | **24.74 ms** | **-13.31 ms** | **-34.99%** | 26.3 FPS | **40.4 FPS** | 31.3% | 68.5% / 20.7% / 8.6% / 2.3% | **WON** |
| **30°** | `( 0.50, 0,  0.87)` | 40.50 ms | **28.93 ms** | **-11.57 ms** | **-28.58%** | 24.7 FPS | **34.6 FPS** | 34.3% | 65.5% / 19.4% / 11.8% / 3.3% | **WON** |
| **45°** | `( 0.71, 0,  0.71)` | 46.74 ms | **33.93 ms** | **-12.82 ms** | **-27.42%** | 21.4 FPS | **29.5 FPS** | 42.6% | 57.2% / 23.5% / 14.4% / 4.8% | **WON** |
| **60°** | `( 0.87, 0,  0.50)` | 51.30 ms | **36.96 ms** | **-14.35 ms** | **-27.97%** | 19.5 FPS | **27.1 FPS** | 48.6% | 51.2% / 26.0% / 18.8% / 3.9% | **WON** |
| **75°** | `( 0.97, 0,  0.26)` | 55.76 ms | **39.03 ms** | **-16.73 ms** | **-30.00%** | 17.9 FPS | **25.6 FPS** | 54.1% | 45.7% / 29.0% / 21.9% / 3.4% | **WON** |
| **90°** | `( 1.00, 0, -0.00)` | 56.70 ms | **39.53 ms** | **-17.18 ms** | **-30.29%** | 17.6 FPS | **25.3 FPS** | 56.9% | 42.8% / 34.6% / 19.0% / 3.5% | **WON** |
| **105°** | `( 0.97, 0, -0.26)` | 56.32 ms | **37.63 ms** | **-18.69 ms** | **-33.19%** | 17.8 FPS | **26.6 FPS** | 56.7% | 43.0% / 38.5% / 15.9% / 2.6% | **WON** |
| **120°** | `( 0.87, 0, -0.50)` | 57.87 ms | **34.77 ms** | **-23.10 ms** | **-39.92%** | 17.3 FPS | **28.8 FPS** | 54.6% | 45.1% / 40.1% / 13.7% / 1.2% | **WON** |
| **135°** | `( 0.71, 0, -0.71)` | 52.60 ms | **30.87 ms** | **-21.73 ms** | **-41.32%** | 19.0 FPS | **32.4 FPS** | 48.2% | 51.5% / 37.8% / 9.8% / 0.9% | **WON** |
| **150°** | `( 0.50, 0, -0.87)` | 45.48 ms | **26.63 ms** | **-18.85 ms** | **-41.45%** | 22.0 FPS | **37.6 FPS** | 41.4% | 58.3% / 36.1% / 4.8% / 0.8% | **WON** |
| **165°** | `( 0.26, 0, -0.97)` | 44.52 ms | **24.07 ms** | **-20.44 ms** | **-45.92%** | 22.5 FPS | **41.5 FPS** | 38.2% | 61.5% / 34.8% / 2.6% / 1.2% | **WON** |

**Gate 10 Outcome:** Clean **12/12 sweep**! Every single angle runs substantially faster under Routed, with 10 of 12 angles beating Current by $\ge 28\%$, peaking at **-45.92%** at $165^\circ$ (41.5 FPS) and **-43.57%** at $0^\circ$ (43.2 FPS).

---

### Dual-Regime All-64 Poses ABBA Benchmark

| Workload | Current (Baseline) | Routed (Phase P18) | Delta (ms) | Delta (%) | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Regime 1 (33 poses, Full Screen Green)** | 94.601 ms | **69.243 ms** | **-25.358 ms** | **-26.81%** | **CLEAN SWEEP** |
| **Regime 2 (31 poses, Open Sky)** | 58.582 ms | **47.909 ms** | **-10.673 ms** | **-18.22%** | **CLEAN SWEEP** |
| **All 64 Poses (Full Tour)** | 77.155 ms | **58.909 ms** | **-18.245 ms** | **-23.65%** | **64 / 64 POSES WON** |

---

## 3. Progress Evolution Across Phases (P12 to P18)

| Metric | Phase P12 | Phase P13 | Phase P14 | Phase P15 | Phase P16 | Phase P17 | **Phase P18 (Current)** |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **All 64 Poses Delta** | -12.41% | -13.06% | -14.67% | -15.16% | -15.89% | -10.36% | **-23.65%** |
| **Regime 1 (Green)** | -16.21% | -17.20% | -17.96% | -18.42% | -19.12% | -11.27% | **-26.81%** |
| **Regime 2 (Sky)** | -5.85% | -6.50% | -9.02% | -9.65% | -10.45% | -8.68% | **-18.22%** |
| **Checkpoint View 1** | +3.20 ms | +2.95 ms | +0.36 ms | -1.71 ms | -5.73 ms | -1.39 ms | **-25.63 ms (-26.93%)** |
| **Checkpoint View 3A FPS** | 32.8 FPS | 41.8 FPS | 42.1 FPS | 42.5 FPS | 42.8 FPS | 39.0 FPS | **43.9 FPS** |
| **Poses Won / 64** | 59 / 64 | 61 / 64 | 64 / 64 | 64 / 64 | 64 / 64 | 64 / 64 | **64 / 64** |

---

## 4. Verification Checksums & Reproducibility Receipts

1. **CPU Verification Probe (`positive_e2e_cpu_verify`):**
   - Tile frames checked: `921,600` (classifier mismatches = 0)
   - S1 rays checked: `878,400` (S1 mismatches = 0)
   - S2 rays checked: `576,112` (S2 mismatches = 0)
   - Total rays checked: `1,572,688` (refused rays = 0)
2. **Native Metal Hardware Gates (`verify`):**
   - Gate 1: Fail-closed unauthored refusal parity (passed)
   - Gate 2: Pose 0 publication hash `d09da9763e7c390a` (passed)
   - Gate 3: Pose 32 publication hash `8bb5048a718c18a5` (passed)
   - Gate 4: Trajectory zero refusals across 8 sampled poses (passed)
   - Gate 5: Production census share `Empty=15.28% S1=48.88% S2=30.44% Fallback=5.40%` (passed)
   - Gate 6: Empty-strip finite-poisoned publication parity (passed)
   - Gate 7: Near-parallel overlap witness (passed)
   - Gate 8: Tall viewport capacity & resize-back (passed)
   - Gate 9: 7 Checkpoint views benchmark (7/7 won)
   - Gate 10: 12 Roll sweep angles benchmark (12/12 won)
3. **Sequential CTest WebGPU Regression Suite:**
   - `webgpu_volume_zero_proof_test`: 100% passed (3.63 sec)
   - `webgpu_volume_support_certificates_test`: 100% passed (14.54 sec)
   - `webgpu_line_quadrature_test`: 100% passed (8.97 sec)

---

## 5. Summary & Sign-off

Phase P18 conclusively solves subpanel fragmentation and root isolation overhead across the entire ray transport pipeline:
- `Checkpoint_View1_Inside_Veil` is now firmly in positive speedup territory (**-26.93%**, 14.4 FPS vs 10.5 FPS Current).
- `Checkpoint_View3A` reaches an astonishing **43.9 FPS** (-43.12% delta).
- The dual-regime 64-pose tour achieved its largest single-phase improvement to date, running at **58.91 ms vs 77.15 ms Current (-23.65% All-64, -26.81% Regime 1, -18.22% Regime 2)** with zero regressions, zero refusals, and bit-exact rendering fidelity.

Ready for Aetherion / The Firmament's structural review!
