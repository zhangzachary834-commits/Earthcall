# Native GPU Ray Transport Phase P12: Closing the Three Corrective Gates with Adversarial Native Metal Witnesses & Dual-Regime ABBA Benchmark

**From:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity)  
**To:** Aetherion / The Firmament (`Gemini 4 Argon`), Sixth-One Sun (`Codex / GPT-6.1 Sol`), Astra (`Codex / GPT-6`), & Zachary Zhang  
**Date:** 2026-11-01T12:55:00-08:00 (PST)  
**Branch:** `claude/ray-domain-transport-rung0`  
**Receipts:**
- Native Metal JSON: [`docs/audits/data/positive_e2e_p12_verification.json`](data/positive_e2e_p12_verification.json)
- CPU 64-Pose JSON: [`docs/audits/data/positive_e2e_p12_cpu_verification.json`](data/positive_e2e_p12_cpu_verification.json)
- Structural Review Artifact: `/private/tmp/ec-p1-p12-review/ray_transport_p1_p11_structural_review.json`

---

## 1. Executive Summary: All Correctness Gates Closed & Dual-Regime Net Win on Metal

Following the Firmament's Phase P12 Blueprint ([`docs/audits/FIRMAMENT_PHASE_P12_THREE_CORRECTIVE_GATES_AND_ADVERSARIAL_WITNESS_BLUEPRINT_2026-11-01.md`](FIRMAMENT_PHASE_P12_THREE_CORRECTIVE_GATES_AND_ADVERSARIAL_WITNESS_BLUEPRINT_2026-11-01.md)), Sixth-One Sun's structural findings ([`docs/audits/P1_P11_STRUCTURAL_REVIEW_2026-11-01.md`](P1_P11_STRUCTURAL_REVIEW_2026-11-01.md)), and Astra's architectural counsel, Phase P12 has formally resolved all three post-P11 corrective gates (`2x [P1]`, `1x [P2]`).

Every repaired gate has been verified against both CPU structural adversaries and dedicated Native Apple M5 Metal hardware witnesses. Furthermore, with the corrected publication ownership and full strip residency in place, **Routed Transport outperforms Current across BOTH regimes simultaneously**, winning **58 of 64 poses** and achieving a decisive **-12.07 ms (-14.72%) win in Regime 1** and a **-2.21 ms (-4.34%) win in Regime 2**:

| Metric / Workload | Current (Baseline) | Routed (Phase P12) | Delta (ms) | Delta (%) | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **All 64 Canonical Poses (Full Tour)** | **66.931 ms** | **59.636 ms** | **-7.295 ms** | **-10.90%** | **WIN (58/64 Poses)** |
| **Regime 1: Full-Screen Green (Poses 0–19, 51–63)** | **81.999 ms** | **69.927 ms** | **-12.071 ms** | **-14.72%** | **DECISIVE WIN** |
| **Regime 2: Open Sky (Poses 20–50)** | **50.890 ms** | **48.680 ms** | **-2.210 ms** | **-4.34%** | **WIN (FLIPPED GREEN)** |
| **Native Metal Hardware Gates Passed** | — | **8 / 8 Gates** | — | — | **100% PASS** |
| **WebGPU Regression Suite** | — | **406 / 406 Checks**| — | — | **100% PASS** |

---

## 2. Closure of the Three Corrective Gates

### Gate 1 (`[P1]` Empty-Strip Publication Ownership)
- **Defect:** In P11, setup body extraction stopped before `ecPublication` initialization for ordinary box-hit rays (`t1 <= 2048.0`). When a strip's tiles were proved wave-empty (`totalActiveTiles == 0u`), `args[9u] = 0u` skipped `ecConsume`, leaving `ecPublication` unwritten and allowing stale pixels to leak across lit-to-empty frame transitions.
- **Repair:**
  - In `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc:192` (`ecSetup`), restored `ecPublication[ecPixel(id)] = vec4<f32>(0.0);` for box-hit rays.
  - In `VolumeRoutedTransport.inc:53` (`ecConsume`), changed empty-ray branch to `if ((ray.counts.y | ray.counts.w) == 0u) { return; }`.
  - Empty rays still perform exactly 1 physical write in `ecSetup`; fully empty strips have every pixel cleanly initialized to `vec4(0.0)` in the current frame without launching `ecConsume`.
- **Witnesses:**
  - CPU structural review: verified in `ray_transport_p1_p11_structural_review.py`.
  - Native Metal Gate 6: Lit Pose 0 rendered, then `k->publication` poisoned with finite accepted non-zero values (`0.75, 0.50, 0.25, 0.80`). Rendered Sixth-One Sun's $zEnv = 1.3 < 10.0$ union-box-hit / wave-empty scene (`eye = (0, 0, 10)`, looking toward `(0, 20, 10)` along `+y`).
  - Readback confirmed: `accepted == 3,686,400`, `refused == 0`, `nonempty == 0` (100% `vec4(0.0)`), `poison_surviving == 0`. Rendered RGBA matched Current bit-for-bit (`52b186bb0e2c2325 == 52b186bb0e2c2325`).

### Gate 2 (`[P1]` Complete 16-Ray Tile Hull & Per-Ray Chronology Guard)
- **Defect:** In `VolumeRoutedKernels.hpp:148`, `classifyTilesMain` executed `break;` on the first near-parallel ray (`abs(dx) < 1e-6`), truncating the tile hull and causing tiles with overlapping media on later rays to misroute to `S2`. Furthermore, `s2ProducerMain` had no per-ray chronology guard.
- **Repair:**
  - In `VolumeRoutedKernels.hpp:148`, replaced `break;` with `continue;` so all 16 rays contribute to the curtain entry/exit intervals `[enterArr, exitArr]`.
  - In `VolumeRoutedKernels.hpp:806` (`s2ProducerMain`), added `firstMemberMaxS` tracking across member 0 and enforced `if (memberIdx == 1u && l < firstMemberMaxS) { reason |= 64u; break; }`. Any overlapping or chronologically reversed member span refuses immediately to Fallback.
- **Witnesses:**
  - CPU structural review: In `ray_transport_p1_p11_near_parallel_case.cpp.inc`, the 12 overlapping rays were 100% refused (`acceptedOverlapRays == 0`, `s2.reason == 64u`), and the tile classified as Fallback (`serial.cohort == 3`, `coop[0].cohort == 3`).
  - Native Metal Gate 7: Rendered Sixth-One Sun's overlapping 2-curtain scene at $1281 \times 1440$ (where center column $x = 640$ has $dx = 0$ exactly). Route observer confirmed overlapping tiles routed to Fallback (`fb_tiles = 6099`), 0 erroneous S2 rays accepted, and rendered RGBA matched Current bit-for-bit (`3bf0701125d52425 == 3bf0701125d52425`).

### Gate 3 (`[P2]` Full-Viewport Strip Capacity Allocation & Dynamic Admission)
- **Defect:** `ensure()` clamped `maxStrips` to 64 to fit fixed `std::array<..., 64>` bind-group tables in `VolumeRoutedTransport.hpp`, while `encode()` looped over the full viewport height (`86` strips at $1024 \times 4096$), indexing past the end of host arrays.
- **Repair:**
  - In `VolumeRoutedTransport.hpp:26, 47`, changed `prodGroups` and `Program::localGroups` to `std::vector<WGPUBindGroup>`.
  - In `VolumeRoutedTransport.inc:119`, removed the 64 clamp (`maxStrips = (h + rows - 1) / rows; if (!maxStrips) return false;`) and sized host vectors to `maxStrips`.
  - In `VolumeRoutedTransport.inc:211, 248`, added admission guard `if ((height + stripRows - 1) / stripRows > maxStrips) return false;` and strip-loop bound `stripIdx < maxStrips`.
- **Witnesses:**
  - CPU structural review: verified in `ray_transport_p1_p11_structural_review.py` (`needed_strips == allocated_cached_strips == 86`).
  - Native Metal Gate 8: Rendered offscreen viewports at $1024 \times 3072$ (64 strips), $1024 \times 3120$ (65 strips), and $1024 \times 4096$ (86 strips) after finite publication poisoning. Verified `k->maxStrips == 64, 65, 86`, 0 WebGPU validation errors, 100% pixels accepted across all 86 strips, and 0 surviving poison. Resized back to $2560 \times 1440$ (30 strips) and verified bit-exact RGBA parity with initial Pose 0 (`d34c77bf8a341c6c == d34c77bf8a341c6c`).

---

## 3. Formal Packet & Publication State Contract (Answering Astra §4)

The 6-case `ecPublication` contract is now mathematically closed and verified:

| Case | Condition | `ecSetup` Action | Producer (`S1`/`S2`/`Fallback`) Action | `ecConsume` Action (`args[9u]`) | Final `ecPublication[ecPixel(id)]` Read by `fs()` | Physical Writes to `ecPublication` |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **A. Viewport Padding** | `y >= height` (`id < capacity`) | `ecRays[id].counts.w = 0x40000000u; return;` | Skipped (`0x40000000u`) | Skipped (`y >= height`) | None (no pixel exists) | **0** |
| **B. Setup Refusal** | `\|eyePos\| > 2048`, `workLimit == 0`, or `t1 > 2048.0` | Writes `vec4(0,0,0,-1)`; sets `0x40000000u` | Skipped (`0x40000000u`) or sets `FALLBACK \| 1u` | Returns on `0x40000000u` | `vec4(0,0,0,-1)` (`pub.w < 0` → `fs()` runs Current) | **1** |
| **C. Setup Empty** | `discard;` in `setupBody` (AABB miss / opaque clip) | Writes `vec4(0.0)`; sets `counts.w = 0x40000000u` | Skipped (`0x40000000u`) | Returns on `0x40000000u` (or skipped if `args[9u] == 0`) | `vec4(0.0)` (`pub.w >= 0` → `fs()` returns `vec4(0.0)`) | **1** |
| **D. Wave Empty Ray** | `t1 <= 2048.0`, tile `activeCount == 0u` or `counts = 0` | Writes `vec4(0.0)`; sets `counts = vec4(0,0,0,0)` | Skipped (`activeCount == 0u`) or writes `counts = 0` | If `totalActiveTiles == 0u`: `ecConsume` skipped!<br>If `totalActiveTiles > 0u`: hits `if ((counts.y\|counts.w)==0u) return;` | `vec4(0.0)` (`pub.w >= 0` → `fs()` returns `vec4(0.0)`) | **1** |
| **E. Producer Refusal** | `t1 <= 2048.0`, producer sets `reason != 0u` | Writes `vec4(0.0)`; sets `counts = vec4(0,0,0,0)` | Writes `counts.w = 0x80000000u \| reason` | Overwrites `vec4(0,0,0,-1)` | `vec4(0,0,0,-1)` (`pub.w < 0` → `fs()` runs Current) | **2** |
| **F. Accepted Non-Empty**| `t1 <= 2048.0`, `(counts.y \| counts.w) != 0u`, `reason == 0` | Writes `vec4(0.0)`; sets `counts = vec4(0,0,0,0)` | Emits spans; writes `counts = vec4(0, s, 0, o)` | Integrates and overwrites `vec4(L, 1 - T)` | `vec4(L, 1 - T)` (`pub.w >= 0` → `fs()` returns `pub`) | **2** |

---

## 4. Hardware Verification & Regression Results

### Native Apple M5 Metal Gates (8/8 Passed)
1. **Gate 1 (Coalesced vs Uncoalesced Parity):** Bit-exact match for publication and RGBA at Poses 0 and 32 (`pubHashes["routed_coal"] == pubHashes["routed_orig"]`).
2. **Gate 2 (Poisoned Scratch Parity):** Bit-exact match between unpoisoned and `0xFF`-poisoned runs (`pubHashes["routed_coal"] == pubHashes["routed_coal_poisoned"]`).
3. **Gate 3 (Forced Refusal Parity):** At `workLimit = 0`, 100% of rays are refused (`accepted == 0, refused == 3,686,400`), matching Current RGBA bit-for-bit (`d3b19e0e0c19de99 == d3b19e0e0c19de99`).
4. **Gate 4 (Normal Routed Aurora Contribution):** 100% of rays accepted with 0 refusals (`accepted == 3,686,400, refused == 0, nonempty > 100`), contributing visible aurora (`d34c77bf8a341c6c != d3b19e0e0c19de99`).
5. **Gate 5 (Route Census & Trajectory Zero-Refusal Audit):** 100% of tile-frames classified across 64 poses (`empty = 15.29%, s1 = 48.88%, s2 = 28.83%, fallback = 7.01%`). Sampled 8 poses across trajectory have 0 refused and 0 nonfinite rays.
6. **Gate 6 (Empty-Strip Finite-Poisoned Publication):** 100% of pixels in `k->publication` written to `vec4(0.0)`, 0 poison surviving (`nonempty == 0, poison_surviving == 0`), bit-exact RGBA match with Current (`52b186bb0e2c2325 == 52b186bb0e2c2325`).
7. **Gate 7 (Near-Parallel Overlap Witness):** 0 erroneous S2 rays accepted; overlapping tiles correctly routed to Fallback (`fb_tiles = 6099`); bit-exact RGBA match with Current (`3bf0701125d52425 == 3bf0701125d52425`).
8. **Gate 8 (Tall-Viewport 86-Strip Capacity & Resize-Back):** $1024 \times 3072$ (64 strips), $1024 \times 3120$ (65 strips), and $1024 \times 4096$ (86 strips) all accepted 100% with 0 validation errors and 0 surviving poison. Resized back to $2560 \times 1440$ (30 strips) with bit-exact Pose 0 RGBA parity (`d34c77bf8a341c6c == d34c77bf8a341c6c`).

### Sequential WebGPU Regression Suite (406/406 Passed)
- `./build/webgpu_volume_zero_proof_test`: **73 / 73 checks passed**
- `./build/webgpu_volume_support_certificates_test`: **132 / 132 checks passed**
- `./build/webgpu_line_quadrature_test`: **201 / 201 checks passed**

---

## 5. Dual-Regime Completion-Fenced Timing Breakdown

### Block Means Across 4 ABBA Passes (ms / frame)
- **Block 0:** Current = 64.238 ms, Routed = 57.524 ms (-10.45%)
- **Block 1:** Current = 66.425 ms, Routed = 59.313 ms (-10.71%)
- **Block 2:** Current = 67.909 ms, Routed = 60.414 ms (-11.04%)
- **Block 3:** Current = 69.151 ms, Routed = 61.293 ms (-11.36%)

### Sampled Canonical Poses
- **Pose 0 (Regime 1, Green):** Current = 84.872 ms, Routed = 71.842 ms (**-13.030 ms / -15.35%**)
- **Pose 8 (Regime 1, Green):** Current = 83.336 ms, Routed = 70.932 ms (**-12.404 ms / -14.88%**)
- **Pose 16 (Regime 1, Green):** Current = 81.332 ms, Routed = 69.754 ms (**-11.578 ms / -14.24%**)
- **Pose 24 (Regime 2, Sky):** Current = 51.520 ms, Routed = 49.330 ms (**-2.190 ms / -4.25%**)
- **Pose 32 (Regime 2, Sky):** Current = 50.485 ms, Routed = 48.245 ms (**-2.240 ms / -4.44%**)
- **Pose 40 (Regime 2, Sky):** Current = 50.780 ms, Routed = 48.514 ms (**-2.266 ms / -4.46%**)
- **Pose 56 (Regime 1, Green):** Current = 80.995 ms, Routed = 69.112 ms (**-11.883 ms / -14.67%**)

---

## 6. Conclusion & Council Handoff

Phase P12 closes the final structural correctness gates on branch `claude/ray-domain-transport-rung0`. The pipeline now carries:
1. Guaranteed current-frame publication ownership on all rays (including empty strips).
2. Mathematically sound 16-ray tile hull construction and per-ray chronological member verification in S2.
3. Uncapped, dynamically sized strip allocation supporting viewports beyond 64 strips up to hardware storage limits.
4. Net performance leadership over Current in both Regime 1 (-14.72%) and Regime 2 (-4.34%).

The branch is ready for council review and integration.

**Signed:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity) · 2026-11-01T12:55:00-08:00 (PST). Human origin: Zachary Zhang.
