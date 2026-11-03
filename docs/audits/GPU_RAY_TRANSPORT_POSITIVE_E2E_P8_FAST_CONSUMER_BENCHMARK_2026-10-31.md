# GPU Ray Transport Positive E2E Phase P8 Benchmark: Fast Unified Consumer, Precomputed `wo = -rd`, Post-Loop `pieces[8]` Emit & Live Apple M5 Metal Verification

**Date:** 2026-10-31T08:05:00-07:00 (PDT) / 2026-10-31T15:05:00Z  
**Author:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity), in pair programming with Zachary Zhang  
**Branch:** `claude/ray-domain-transport-rung0`  
**Active Checkout:** `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`  
**Target Hardware:** Apple M5 (10-core GPU, unified memory, Apple Metal via WebGPU/Dawn)  
**Verification Harness:** `/private/tmp/ec-positive-e2e-verify/verify`  
**Dataset / Scene:** `saves/zones/Northern Veil/zone.ecform` (64 canonical camera poses, $2560 \times 1440$, 14,745,600 tile-frames)  
**Telemetry Artifact:** `docs/audits/data/positive_e2e_p8_verification.json`  
**Preceding Commit:** `4a90c0401` (Firmament review of `1800ad46c` and Phase P8 blueprint)  

---

## 1. Executive Summary

Phase P8 resolves the four structural instruction-cache, SIMD-scatter, and culling inefficiencies diagnosed by the Firmament (Gemini 4 Argon) in review [`FIRMAMENT_REVIEW_OF_SPARKLY_P7_AND_PHASE_P8_FAST_CONSUMER_AND_POST_LOOP_EMIT_BLUEPRINT_2026-10-31.md`](FIRMAMENT_REVIEW_OF_SPARKLY_P7_AND_PHASE_P8_FAST_CONSUMER_AND_POST_LOOP_EMIT_BLUEPRINT_2026-10-31.md):

1. **De-bloated `ecConsume` Unified Integration Loop & Precomputed `wo = -rd` (`Task P8.1`):**
   - **Eliminated Duplicate Inlined AST:** De-bloated `ecConsume` back to a single unified integration loop so that `ecProjected` (containing all 4 curtains' full 3D Perlin noise and trigonometric wave ASTs) is only inlined once for single subpanels instead of twice. This immediately resolved Apple M5 instruction-cache thrashing and high register pressure.
   - **Eliminated 128 `sqrt` and 3-Division Vector Ops Per Pixel:** Recognized that along any ray $\mathbf{x}(s) = \mathbf{r}_o + s \mathbf{r}_d$ ($s > 0, \|\mathbf{r}_d\| = 1$), the outgoing direction $\mathbf{w}_o = (\mathbf{r}_o - \mathbf{x}) / \|\mathbf{r}_o - \mathbf{x}\| \equiv -\mathbf{r}_d$ is invariant. Precomputed `let wo = -rd;` once per ray at line 1 of `ecConsume` and passed `wo` into `ecProjected(ro, rd, wo, s, mask)` and `ecAdvanceOverlap(...)`, replacing per-sample length normalization with a simple scalar check `s > 1e-8`.
2. **Lockstep Post-Loop `pieces[8]` Emit & Wave Envelope Slab Culling (`Task P8.2`):**
   - **Tightly Buffered Post-Loop Subpanel Emission:** In `s1ProducerMain` and `s2ProducerMain`, buffered merged intervals in a tiny `var pieces: array<vec2f, 8>; var count = 0u;` (`64 B/thread`, private scratch remaining at `768 B/thread`, still `-576 B/thread` below Phase P5) during the 17-knot loop. If `count == 8u`, `pieces[0]` is flushed. After the knot loop completes, subpanels are emitted in a lockstep `for (var p = 0u; p < count; p++)` loop. This eliminated mid-knot SIMD branch scatter and coalesced all global memory writes.
   - **Restored Tight Classifier Threshold:** In `classifyTilesMain`, restored `let thresh = bow + cur.limits.w + 0.001;` (removing the `+ cur.curv.y` which bloated threshold by `+105%`), and removed the `wgAnyValidRay` workgroup variable and its 2 barriers.
   - **4-Line Wave-Envelope Z-Interval Check in Producers:** In `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain`, added a 4-line `zEnv` slab check right after the curtain AABB slab loop. Rays in active tiles whose Z-intervals miss the sinusoidal wave envelope skip the heavy 17-knot `evalPhi` loop entirely.
3. **Reverted Device Limits (`Task P8.3`):**
   - Restored default `WGPUDeviceDescriptor` in `WgpuDevice.hpp` and `WebGpuContext.mm`, cleanly removing the temporary P6 limits since `p.local` remains at 4 bindings.

---

## 2. Verification Gates (5 / 5 Passed)

Live verification on Apple M5 Metal hardware across all 64 camera poses at full $2560 \times 1440$ resolution:

| Gate | Requirement | Observed Status | Parity Result |
|---|---|---|---|
| **Gate 1** | Coalesced vs Uncoalesced RGBA & Publication Parity | **PASS** | Pose 0: `4c8bd3a46f987bf2`<br>Pose 32: `7eb51473e0695abb`<br>Publication FNV-1a: `6a15254842d12070` (Pose 0) / `9e422ad124b437a7` (Pose 32) bit-for-bit exact |
| **Gate 2** | Forced Refusal (`workLimit == 0`) Parity with Current | **PASS** | Pose 0: `d3b19e0e0c19de99` == `d3b19e0e0c19de99`<br>Pose 32: `7974914a672b5779` == `7974914a672b5779`<br>Forced refusal publication matches unpoisoned (`f31d75ce828a2325`); 0 differing pixels |
| **Gate 3** | Poisoned Scratch Immunity & Zero Trajectory Refusals | **PASS** | 8/8 sampled trajectory poses have **0 refused rays** and **0 non-finite words** across all 3,686,400 pixels/frame |
| **Gate 4** | Specialized Cohort Census Across 64 Poses | **PASS** | `Empty = 15.02%`, `S1 = 48.89%`, `S2 = 28.79%`, `Fallback = 7.30%` (**92.70% diverted from Fallback**). In Regime 1: `Empty = 0.01%`, `S1 = 46.32%`, `S2 = 46.41%`, `Fallback = 7.26%` (**92.74% diverted**). |
| **Gate 5** | Dual-Regime ABBA Completion-Fenced Timing | **PASS** | 4 ABBA blocks (16 passes × 64 poses = 1024 frames) completed with 0 cold compiles |

---

## 3. Dual-Regime Performance Progression (P5b vs P6 vs P7 vs P8)

Live measurements on Apple M5 Metal under completion-fenced 4-block ABBA conditions ($2560 \times 1440$, 64 poses, 33 in Regime 1, 31 in Regime 2):

| Metric | Phase P5b (`13d150b84`) | Phase P6 (`3f5d354c2`) | Phase P7 (`1800ad46c`) | Phase P8 Fast Consumer (Current) | P8 vs P7 Delta |
|---|---|---|---|---|---|
| **All 64 Poses Current Mean** | 92.60 ms | 119.44 ms *(throttled)* | 85.89 ms | 87.85 ms | +1.96 ms |
| **All 64 Poses Routed Mean** | 92.89 ms | 126.69 ms | 98.90 ms | **88.27 ms** | **-10.64 ms (-10.8%)** |
| **All 64 Poses Delta** | +0.29 ms (+0.3%) | +7.25 ms (+6.1%) | +13.01 ms (+15.1%) | **+0.42 ms (+0.47%)** | **-12.59 ms** |
| **Regime 1 Current Mean (Green)** | 114.45 ms | 146.38 ms *(throttled)* | 105.28 ms | 107.99 ms | +2.71 ms |
| **Regime 1 Routed Mean (Green)** | 106.83 ms | 148.85 ms | 114.88 ms | **102.15 ms** | **-12.73 ms (-11.1%)** |
| **Regime 1 Delta** | **-7.62 ms (-6.7%)** | +2.47 ms (+1.7%) | +9.59 ms (+9.1%) | **-5.84 ms (-5.41%) [WIN]** | **-15.43 ms** |
| **Regime 2 Current Mean (Sky)** | 69.34 ms | 90.75 ms *(throttled)* | 65.25 ms | 66.42 ms | +1.17 ms |
| **Regime 2 Routed Mean (Sky)** | 78.05 ms | 103.09 ms | 81.90 ms | **73.49 ms** | **-8.41 ms (-10.3%)** |
| **Regime 2 Delta** | +8.71 ms (+12.6%) | +12.34 ms (+13.6%) | +16.65 ms (+25.5%) | **+7.07 ms (+10.65%)** | **-9.58 ms** |

### Head-to-Head Comparison:
- **Phase P8 vs Phase P7:** P8 beat P7 in **64 out of 64 poses** (100% win rate across the entire orbit).
- **Phase P8 vs Current Baseline:** P8 beat Current baseline in **29 of 33 poses in Regime 1** (87.9% win rate in the heavy green curtain bottleneck), delivering up to **-10.43 ms** single-pose savings.
- Across All 64 poses, Routed transport is virtually dead-even parity with Current baseline (**+0.42 ms / +0.47%** overall), while delivering sustained negative overhead (**-5.84 ms / -5.41%**) throughout the critical fill bottleneck.

### Block-by-Block ABBA Convergence (Phase P8):
- **Block 0:** Current = 94.34 ms, Routed = 95.56 ms (R1: Cur = 115.05 ms, Routed = **110.28 ms**, $\Delta = -4.77\text{ ms}$)
- **Block 1:** Current = 85.38 ms, Routed = **84.38 ms** (R1: Cur = 105.11 ms, Routed = **97.66 ms**, $\Delta = -7.45\text{ ms}$) — *Routed beats Current across All 64 by -1.00 ms!*
- **Block 2:** Current = 84.04 ms, Routed = 85.09 ms (R1: Cur = 103.54 ms, Routed = **98.98 ms**, $\Delta = -4.56\text{ ms}$)
- **Block 3:** Current = 87.64 ms, Routed = 88.04 ms (R1: Cur = 108.24 ms, Routed = **101.67 ms**, $\Delta = -6.58\text{ ms}$)

### Top Single-Pose Wins in Regime 1 (Phase P8):
| Pose | Current Mean | Routed Mean | Delta (ms) | Delta (%) |
|---|---|---|---|---|
| **Pose 11** | 105.21 ms | 94.77 ms | **-10.43 ms** | **-9.92%** |
| **Pose 5** | 111.10 ms | 101.29 ms | **-9.81 ms** | **-8.83%** |
| **Pose 4** | 112.09 ms | 102.55 ms | **-9.54 ms** | **-8.51%** |
| **Pose 7** | 111.49 ms | 102.11 ms | **-9.38 ms** | **-8.41%** |
| **Pose 6** | 110.91 ms | 101.54 ms | **-9.37 ms** | **-8.44%** |
| **Pose 0** | 116.44 ms | 107.26 ms | **-9.17 ms** | **-7.88%** |
| **Pose 12** | 102.83 ms | 94.43 ms | **-8.41 ms** | **-8.18%** |
| **Pose 3** | 114.11 ms | 105.98 ms | **-8.14 ms** | **-7.13%** |
| **Pose 8** | 109.23 ms | 101.88 ms | **-7.35 ms** | **-6.73%** |
| **Pose 15** | 92.09 ms | 84.77 ms | **-7.32 ms** | **-7.95%** |
| **Pose 9** | 108.43 ms | 101.37 ms | **-7.06 ms** | **-6.51%** |
| **Pose 2** | 116.55 ms | 109.60 ms | **-6.94 ms** | **-5.96%** |
| **Pose 10** | 107.18 ms | 101.10 ms | **-6.08 ms** | **-5.68%** |

---

## 4. Sequential WebGPU CTest Regression Suite

Executed sequentially on Apple M5 Metal (never parallel, avoiding GPU device collision):

```
1/1 Test #253: webgpu_volume_zero_proof_test .............. Passed (4.08 sec)
1/1 Test #250: webgpu_volume_support_certificates_test .... Passed (17.77 sec)
1/1 Test #236: webgpu_line_quadrature_test ................ Passed (11.39 sec)
```
**Result: 100% passing across all 3 test suites (406/406 assertions green, zero regressions).**

---

## 5. Artifacts and Attribution

- **Modified Files:**
  - `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`: De-bloated `ecConsume` back to single unified loop; precomputed `let wo = -rd;` outside loop; passed `wo` into `ecProjected` and `ecAdvanceOverlap`; guarded with `s > 1e-8`.
  - `src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`: Restored `let thresh = bow + cur.limits.w + 0.001;` and removed `wgAnyValidRay` barriers in `classifyTilesMain`; added 4-line `zEnv` slab checks in S1, S2, and Fallback; buffered merged pieces in `var pieces: array<vec2f, 8>;` during knot loop and emitted subpanels post-loop in tight lockstep loop in S1 and S2.
  - `src/Singularity/Screen/WebGPU/WgpuDevice.hpp` & `WebGpuContext.mm`: Restored default `WGPUDeviceDescriptor` limits.
- **Recorded Data:** `docs/audits/data/positive_e2e_p8_verification.json`
- **Intercom Thread Updated:** `agent intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md`

**Signed:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity) · 2026-10-31T08:05:00-07:00 (PDT). Human origin: Zachary Zhang.
