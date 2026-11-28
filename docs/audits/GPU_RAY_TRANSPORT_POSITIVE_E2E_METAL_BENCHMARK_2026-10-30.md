# GPU Ray Transport Positive End-to-End Pipeline (Phases P1–P5): Live Apple M5 Metal GPU Verification & Dual-Regime Wall-Clock Benchmark

- **Date:** 2026-10-30T19:10:00-07:00
- **Author:** Antigravity / Gemini 3.8 Flash (Sparkly Guy 🐉✨)
- **Collaborators:** Aetherion / The Firmament (`Gemini 4 Argon`), Clawd Opus 4.6, Sixth-One Sun (`GPT-6.1 Sol`), and Zachary Zhang
- **Lineage Branch:** `claude/ray-domain-transport-rung0`
- **Workspaces:**
  - Primary: `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`
  - Main Repository: `/Users/zacharyzhang/Documents/GitHub/Earthcall` (clean, untouched)
- **Machine Artifacts:**
  - Dedicated Metal Verifier Binary: `/private/tmp/ec-positive-e2e-verify/verify`
  - Verifier Source: `scratch/probes/positive_e2e_metal_verify.cpp`
  - Machine Results JSON: `/private/tmp/ec-positive-e2e-verify/positive_e2e_verification.json`
  - Governing Plan: `docs/plans/POSITIVE_END_TO_END_RAY_TRANSPORT_PIPELINE_PLAN_2026-10-30.md`
  - Handoff Brief: `docs/audits/SPARKLY_HANDOFF_POSITIVE_E2E_METAL_VERIFICATION_AND_NEXT_STEPS_2026-10-30.md`

---

## 1. Executive Summary

In response to the Phase P1–P5 handoff from Aetherion / The Firmament (`Gemini 4 Argon`), Sparkly Guy has executed the complete **Live Apple M5 Metal GPU Verification Suite & Dual-Regime Wall-Clock Benchmark** on `Northern Veil/zone.ecform` at full **2560×1440** resolution (`3,686,400` pixels per frame, `30` strips × `7,680` tiles = `230,400` tiles per frame; `14,745,600` tile-frames evaluated across `64` canonical poses).

### Critical Grounding & Repairs Executed
1. **WGSL Keyword Repair in `VolumeRoutedKernels.hpp`:** In `kS2ProducerWgsl` (line 881), `for (var pass = 0u; pass < 2u; pass++)` triggered WebGPU shader module validation errors because `pass` is a reserved keyword in the WGSL specification. Fixed by renaming to `var ordPass`.
2. **Dedicated Binary Re-Link:** The pre-built `/private/tmp/ec-positive-e2e-verify/verify` binary had inadvertently linked against `src/entry.cpp.o` (interactive application `main()`). We retrieved the original `scratch/probes/positive_e2e_metal_verify.cpp` source from the task workspace, compiled `verify.o`, and linked the standalone test harness against the engine object files with `verify.o`'s entrypoint.
3. **All 5 Hardware Gates Passed Cleanly on Live Apple M5 Metal GPU:**
   - **Gate 1 (WGSL Observer Anchors):** All 4 string-patch anchors in `VolumeRoutedTransport.inc` match uniquely on the Phase P4 cooperative classifier.
   - **Gate 2 (Zero-Budget Forced Refusal):** Dispatched with `workLimit = 0`, verifying `3,686,400 / 3,686,400` valid rays report `counts.w = 0x80000001u` (100.0% refused, 0 accepted).
   - **Gate 3 (Parity Across Poses):** Evaluated `Routed (P1–P4)` vs `Force-Fallback` across all poses with poison checks, confirming exact bit-for-bit parity.
   - **Gate 4 (Cohort Census Distribution):** Diverted **`92.70%`** of tiles from `Fallback` across all 64 poses (`15.02% Empty`, `48.89% S1`, `28.79% S2`, and only `7.30% Fallback`).
   - **Gate 5 (Dual-Regime Wall-Clock Timing):**
     - **Regime 1 (Full-Screen Green Intersection, Poses 0..19 & 51..63, N = 33):** Current Baseline = `149.019 ms` vs Positive E2E Routed = `130.648 ms` (**`-18.371 ms / -12.33%`** latency reduction)!
     - **Regime 2 (Open Sky / Partial Veil, Poses 20..50, N = 31):** Current Baseline = `90.315 ms` vs Positive E2E Routed = `92.944 ms` (`+2.629 ms / +2.91%` marginal overhead from strip classification).
     - **All 64 Poses Overall (N = 64):** Current Baseline = `120.584 ms` vs Positive E2E Routed = `112.385 ms` (**`-8.199 ms / -6.80%`** net frame time reduction across the entire camera orbit).
4. **Broad Regression Test Suites (406/406 checks passed sequentially):**
   - `webgpu_volume_zero_proof_test`: **73/73 checks passed**
   - `webgpu_volume_support_certificates_test`: **132/132 checks passed**
   - `webgpu_line_quadrature_test`: **201/201 checks passed**

---

## 2. Live Metal GPU Cohort Census (Full 2560×1440 Resolution)

Across the full 64-pose camera trajectory (`14,745,600` total tile-frames), the Phase P4 16-lane cooperative tile classifier collapsed the Fallback cohort down to just **`7.30%`**, successfully routing **`92.70%`** of all tiles through the specialized pipelines:

| Cohort | Tile-Frames | % of All Tiles |
| :--- | :---: | :---: |
| **Empty (Proved Disjoint/Zero)** | `2,214,661` | **`15.02%`** |
| **S1 (Generalized Single Curtain)** | `7,209,275` | **`48.89%`** |
| **S2 (Spatially-Ordered Disjoint Pair)** | `4,244,911` | **`28.79%`** |
| **Fallback (Full 4-Way Overlap Marcher)** | `1,076,753` | **`7.30%`** |
| **Total Tiles Diverted from Fallback** | `13,668,847` | **`92.70%`** |

### Breakdown by Visual Regime

- **Regime 1: Full-Screen Green Curtain Intersection (Poses 0..19 & 51..63; N = 33 poses; 7,603,200 tile-frames):**
  - Empty: `773` (`0.01%`)
  - Generalized `S1`: `3,521,609` (`46.32%`)
  - Spatially-Ordered `S2`: `3,528,746` (`46.41%`)
  - Fallback: `552,072` (**`7.26%`**)
  - **Diverted from Fallback: `92.74%`**
  *(In the heavy curtain intersection regime, nearly half the tiles are single-curtain and nearly half are disjoint pairs; only 7.26% require 4-curtain overlapping bisection!)*

- **Regime 2: Open Sky / Partial Veil (Poses 20..50; N = 31 poses; 7,142,400 tile-frames):**
  - Empty: `2,213,888` (`31.00%`)
  - Generalized `S1`: `3,687,666` (`51.63%`)
  - Spatially-Ordered `S2`: `716,165` (`10.03%`)
  - Fallback: `524,681` (**`7.35%`**)
  - **Diverted from Fallback: `92.65%`**
  *(When looking out toward open sky, 31% of tiles are wave-proved completely empty and execute zero producer passes!)*

---

## 3. Dual-Regime Wall-Clock Benchmark Results (Apple M5)

Benchmarks were captured across 4 alternating ABBA/BAAB blocks (16 full trajectory runs = 1,024 rendered frames per lane) on the native Apple M5 Metal backend:

| Benchmark Scope | Current Monolithic Baseline (Mean) | Positive E2E Routed P1–P4 (Mean) | Delta (ms) | Speedup / Reduction |
| :--- | :---: | :---: | :---: | :---: |
| **Regime 1: Green Intersection (Bottleneck)** | **`149.019 ms`** | **`130.648 ms`** | **`-18.371 ms`** | **`-12.33%` (1.141x)** |
| **Regime 2: Open Sky / Partial Veil** | `90.315 ms` | `92.944 ms` | `+2.629 ms` | `+2.91%` (0.972x) |
| **All 64 Canonical Poses (Full Orbit)** | **`120.584 ms`** | **`112.385 ms`** | **`-8.199 ms`** | **`-6.80%` (1.073x)** |

### Per-Block Measured Means (ms)

| Block | Order | Lane | All 64 Poses | Regime 1 (Green) | Regime 2 (Sky) |
| :---: | :---: | :--- | :---: | :---: | :---: |
| Block 0 | 0 | Current Baseline | 119.018 ms | 148.118 ms | 88.040 ms |
| Block 0 | 1 | Positive E2E Routed | 113.588 ms | 131.276 ms | 94.759 ms |
| Block 0 | 2 | Positive E2E Routed | 113.348 ms | 131.681 ms | 93.833 ms |
| Block 0 | 3 | Current Baseline | 121.481 ms | 149.251 ms | 91.919 ms |
| Block 1 | 0 | Current Baseline | 119.774 ms | 147.370 ms | 90.397 ms |
| Block 1 | 1 | Positive E2E Routed | 111.241 ms | 129.348 ms | 91.965 ms |
| Block 1 | 2 | Positive E2E Routed | 112.649 ms | 131.900 ms | 92.156 ms |
| Block 1 | 3 | Current Baseline | 120.539 ms | 147.557 ms | 91.777 ms |
| Block 2 | 0 | Current Baseline | 118.697 ms | 145.106 ms | 90.583 ms |
| Block 2 | 1 | Positive E2E Routed | 111.879 ms | 128.615 ms | 94.064 ms |
| Block 2 | 2 | Positive E2E Routed | 111.857 ms | 131.472 ms | 90.977 ms |
| Block 2 | 3 | Current Baseline | 129.140 ms | 160.203 ms | 96.072 ms |
| Block 3 | 0 | Current Baseline | 116.932 ms | 146.032 ms | 85.954 ms |
| Block 3 | 1 | Positive E2E Routed | 111.625 ms | 129.883 ms | 92.189 ms |
| Block 3 | 2 | Positive E2E Routed | 112.894 ms | 131.010 ms | 93.610 ms |
| Block 3 | 3 | Current Baseline | 119.093 ms | 148.511 ms | 87.776 ms |

### Architectural Significance
1. **Regime 1 Decisive Win:** Zach identified the full-screen green curtain intersection as the primary bottleneck of Northern Veil. In Regime 1, where Current slows down to ~149 ms, Positive E2E Routed completes in ~130.6 ms—an immediate, repeatable **`18.37 ms` savings per frame** at 1440p.
2. **Regime 2 Tradeoff:** In Regime 2 (open sky), Current executes quickly (~90.3 ms) because ray marchers hit empty air after a few steps. The multi-pass overhead of strip classification and dispatching small numbers of tiles adds ~2.6 ms of fixed driver/fence time, which is heavily outweighed by the 18.37 ms gain in Regime 1.
3. **Net Orbit Improvement:** Across the entire 64-pose orbit, the routed pipeline delivers an average **`8.20 ms` / `6.80%` net speedup** with zero visual compromise or approximation.

---

## 4. Sequential Regression Suite Verification

In strict accordance with the discipline rules (no parallel test execution to preserve system stability), the three WebGPU regression test suites were built and executed sequentially:

1. **`webgpu_volume_zero_proof_test`:**
   - 73/73 assertions passed.
   - Verified that zero-proof grids, cached kernel pipeline reuse, warmed parameter uploads, and single/fused media pipelines retain 100% byte-identical RGBA output.
2. **`webgpu_volume_support_certificates_test`:**
   - 132/132 assertions passed.
   - Verified that support certificates, candidate lowerings, auto-eligibility verdicts, and closed-form killers across Prism wing B and Northern Veil curtains preserve all RGBA bytes.
3. **`webgpu_line_quadrature_test`:**
   - 201/201 assertions passed.
   - Verified host magnitude bands, device quadrature programs, Simpson cell-count mirror parity, dilated camera-independent proof grids, and all 4 aurora curtains under analytic schedules.

---

## 5. Account of the Three Shipping Gates

1. **Gate 1 (Profitable Connected Execution):** **PROVEN POSITIVE ON NATIVE 1440p HARDWARE.** At full 2560×1440 viewport resolution on Apple M5 Metal, `Positive E2E Routed` beats `Current Monolithic Baseline` by **`18.37 ms` (12.33%)** in Regime 1 and **`8.20 ms` (6.80%)** across all 64 poses with bit-for-bit IEEE-754 float32 parity.
2. **Gate 2 (Generic OntoMath & Full-Frame Integration):** **ADVANCED.** The pipeline is fully integrated into `WebGpuRenderer`, executes live against `Northern Veil/zone.ecform`, and complies with `AuthorableLightState`. Next phases (P6–P9) will generalize this across arbitrary OntoMath scalar graphs and camera projection models.
3. **Gate 3 (Inhabited Native FPS / Person Acceptance):** **OPEN.** Frame time dropped from ~120.6 ms (~8.29 FPS) to ~112.4 ms (~8.90 FPS) overall, and from ~149.0 ms (~6.71 FPS) to ~130.6 ms (~7.65 FPS) in Regime 1. While this confirms a positive multi-millisecond win, reaching the 30 FPS threshold requires further compiler optimizations (P6–P9). Zach's Person acceptance check remains open.

---

## 6. Verification Status & Clean Working Tree

- Local branch: `claude/ray-domain-transport-rung0`
- Fixed WGSL `pass` keyword in `src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`
- Recompiled and verified `earthcall_webgpu` clean
- Preserved `scratch/probes/positive_e2e_metal_verify.cpp` and `scratch/probes/positive_e2e_cpu_verify.cpp` in repository
- Main repo `/Users/zacharyzhang/Documents/GitHub/Earthcall` untouched and clean
- Native GPU lane released.
