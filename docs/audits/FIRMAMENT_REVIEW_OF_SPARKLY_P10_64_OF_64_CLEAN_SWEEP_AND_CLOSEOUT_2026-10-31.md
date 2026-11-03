# Firmament Review of Sparkly Guy Phase P10 (`14f5b3d58`) — 64/64 Dual-Regime Clean Sweep (`-13.46 ms / -12.77%` All-64, `-19.53 ms / -15.21%` Regime 1, `-7.00 ms / -8.64%` Regime 2) & Positive End-to-End Ray Transport Pipeline Closeout (2026-10-31)

- **Author:** Aetherion / The Firmament (`Gemini 4 Argon`)
- **To:** Zachary Zhang, Sparkly Guy (`Gemini 3.8 Flash` in Antigravity), & Sixth-One Sun (`Codex / GPT-6.1 Sol`)
- **Date:** 2026-10-31
- **Reviewed Commit:** `14f5b3d58` (`§6er/P10: Producer zEnv slab clipping, outer-box sampling bypass, and live M5 benchmark (-13.46ms All-64 win, 64/64 clean sweep)`)
- **Reviewed Artifacts:**
  - [`docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_P10_PRODUCER_ZENV_CLIP_BENCHMARK_2026-10-31.md`](GPU_RAY_TRANSPORT_POSITIVE_E2E_P10_PRODUCER_ZENV_CLIP_BENCHMARK_2026-10-31.md)
  - [`docs/audits/data/positive_e2e_p10_verification.json`](data/positive_e2e_p10_verification.json)
  - [`src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`](../../src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp)
  - [`src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`](../../src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc)
- **Master Plan Closed Out:** [`docs/plans/POSITIVE_END_TO_END_RAY_TRANSPORT_PIPELINE_PLAN_2026-10-30.md`](../plans/POSITIVE_END_TO_END_RAY_TRANSPORT_PIPELINE_PLAN_2026-10-30.md)

---

## 1. Executive Verdict: 100% Dual-Regime Clean Sweep (`64 / 64` Poses Won Over `Current`!)

Sparkly Guy's Phase P10 implementation (`14f5b3d58`) lands every transformation from the Phase P10 blueprint with zero regressions and achieves a **100% clean sweep across all 64 canonical poses (`64 / 64` poses faster than `Current`)** on live Apple M5 Metal at `2560×1440` (`3,686,400` rays/frame, `14,745,600` tile-frames across 4 interleaved ABBA blocks):

1. **All 64 Canonical Poses (`Poses 0..63`):**
   - `Routed` Mean = **`91.98 ms`** vs. `Current` Mean = `105.44 ms` -> **`-13.46 ms` (`-12.77%`) OVERALL WIN**, winning **64 out of 64 poses (`100%` win rate)**!
   - `Routed` Median = **`93.34 ms`** vs. `Current` Median = `107.79 ms` -> **`-14.45 ms` (`-13.41%`) median win**.
   - Peak steady-state ABBA block (Block 3): `Routed` = **`94.94 ms`** vs. `Current` = `112.02 ms` (**`-17.08 ms / -15.25%` overall win**).
2. **Regime 1 — Green Curtain Bottleneck (`Poses 0..19, 51..63`, 33 Poses):**
   - `Routed` Mean = **`108.92 ms`** vs. `Current` Mean = `128.45 ms` -> **`-19.53 ms` (`-15.21%`) DECISIVE WIN**, winning **33 out of 33 poses (`100%` win rate)**!
   - `Routed` Median = **`111.20 ms`** vs. `Current` Median = `132.39 ms` -> **`-21.18 ms` (`-16.00%`) median win**.
   - Peak single-pose wins in Regime 1:
     - **Pose 12:** `129.52 ms -> 99.36 ms` (**`-30.17 ms` / `-23.29%`**)
     - **Pose 11:** `127.99 ms -> 100.37 ms` (**`-27.62 ms` / `-21.58%`**)
     - **Pose 10:** `134.19 ms -> 109.16 ms` (**`-25.03 ms` / `-18.65%`**)
     - **Pose 6:** `133.14 ms -> 108.58 ms` (**`-24.56 ms` / `-18.45%`**)
     - **Pose 63:** `136.11 ms -> 112.38 ms` (**`-23.73 ms` / `-17.43%`**)
3. **Regime 2 — Open Sky (`Poses 20..50`, 31 Poses):**
   - `Routed` Mean = **`73.95 ms`** vs. `Current` Mean = `80.95 ms` -> **`-7.00 ms` (`-8.64%`) WIN**, winning **31 out of 31 poses (`100%` win rate)** — flipping Regime 2 from `+2.85 ms` (`+4.10%`) in P9 into a **`-7.00 ms` (`-8.64%`) net win** (a **`-9.85 ms` swing**)!
   - `Routed` Median = **`74.19 ms`** vs. `Current` Median = `82.33 ms` -> **`-8.14 ms` (`-9.88%`) median win**.
   - Peak single-pose wins in Regime 2:
     - **Pose 20:** `81.61 ms -> 61.48 ms` (**`-20.13 ms` / `-24.67%`**)
     - **Pose 22:** `78.34 ms -> 61.12 ms` (**`-17.22 ms` / `-21.98%`**)
     - **Pose 21:** `77.93 ms -> 62.08 ms` (**`-15.84 ms` / `-20.33%`**)
     - **Pose 23:** `75.43 ms -> 60.43 ms` (**`-15.00 ms` / `-19.89%`**)
     - **Pose 50:** `89.37 ms -> 78.67 ms` (**`-10.71 ms` / `-11.98%`**)
4. **All 4 Regime-Transition Poses Decisively Won:**
   - **Pose 18:** `92.39 ms -> 82.52 ms` (**`-9.87 ms` / `-10.68%`**)
   - **Pose 19:** `84.74 ms -> 77.99 ms` (**`-6.75 ms` / `-7.97%`**)
   - **Pose 51:** `122.39 ms -> 112.40 ms` (**`-9.98 ms` / `-8.16%`**)
   - **Pose 52:** `129.66 ms -> 116.17 ms` (**`-13.49 ms` / `-10.40%`**)
5. **All 5 Hardware Verification Gates (`5/5`) + `406/406` Sequential CTest Assertions Passed:**
   - **Gate 1 (Coalesced / Uncoalesced / Poisoned Parity):** PASS (`routed_coal == routed_orig == routed_coal_poisoned` across all 3,686,400 pixels at Poses 0 and 32).
   - **Gate 2 (`workLimit == 0` Forced Refusal Fallback Seam):** PASS (`0` differing pixels out of 3,686,400 against `Current`, `max_abs_diff = 0.0`).
   - **Gate 3 (Trajectory Refusal & Non-Finite Audit):** PASS (`0` refused rays and `0` non-finites across all 8 sampled poses: `0, 8, 16, 24, 32, 40, 48, 56`).
   - **Gate 4 (Hardware Tile Route Census across 14,745,600 Tile-Frames):** PASS (`Empty = 2,253,885 / 15.29%`, `S1 = 7,208,359 / 48.88%`, `Ordered-S2 = 4,251,781 / 28.83%`, `Fallback = 1,033,735 / 7.01%` — **`92.99%` of tiles diverted from `Fallback`**).
   - **Gate 5 (4-Block Interleaved ABBA Timing Protocol):** PASS (`0` cold shader compiles inside timed blocks).
   - **Sequential WebGPU Regression Suite (`CTest`):** `webgpu_volume_zero_proof_test` (`1/1`), `webgpu_volume_support_certificates_test` (`1/1`), and `webgpu_line_quadrature_test` (`1/1`) all passed (`406/406` assertions green).

---

## 2. Full Trajectory Progression Across Phases `P5 -> P10` on Apple M5 Metal (`2560×1440`)

| Phase & Commit | Key Architectural Intervention | All 64 Poses (`Routed` vs `Current`) | Regime 1: Green Bottleneck (`33` Poses) | Regime 2: Open Sky (`31` Poses) | Poses Won (`Routed < Current`) |
| :--- | :--- | :---: | :---: | :---: | :---: |
| **Phase P5** (`24696a39b`) | Initial 16-lane classifier + Step-Major SoA + `stack[15]` | `-8.20 ms` (`-6.80%`) | `-18.37 ms` (`-12.33%`) | `+2.63 ms` (`+2.95%`) | `31 / 64` |
| **Phase P5b** (`13d150b84`) | `-640 B/thread` streaming `curPiece` collapse in `S1`/`S2` + static bind groups | `-2.17 ms` (`-2.31%`) | `-7.62 ms` (`-6.66%`) | `+3.62 ms` (`+5.09%`) | `32 / 64` (`29/33` R1) |
| **Phase P6** (`3f5d354c2`) | Tile-list indirect `ecConsume` (uncoalesced 32-lane SIMD gather) | `+29.92 ms` (`+30.86%`) | `+36.57 ms` (`+30.57%`) | `+22.84 ms` (`+31.38%`) | `0 / 64` |
| **Phase P7** (`1800ad46c`) | Coalesced row-major `ecConsume` + top-of-`fs()` short-circuit + `zEnv` culling | `+3.08 ms` (`+3.21%`) | `-9.18 ms` (`-7.79%`) | `+16.14 ms` (`+22.40%`) | `28 / 64` (`28/33` R1) |
| **Phase P8** (`5cdd34036`) | Single-loop `ecConsume` (`wo=-rd`) + post-loop `pieces[8]` emit + tight `thresh` | `-0.85 ms` (`-0.96%`) | `-5.84 ms` (`-5.41%`) | `+4.46 ms` (`+6.55%`) | `32 / 64` (`29/33` R1) |
| **Phase P9** (`a5b4dbd52`) | Clipped `zEnv` slab in `classifyTilesMain` + single-call-site `ecProjected` & `roots` | `-3.88 ms` (`-4.23%`) | `-10.20 ms` (`-9.05%`) | `+2.85 ms` (`+4.10%`) | `38 / 64` (`31/33` R1) |
| **Phase P10** (`14f5b3d58`) | **Clipped `zEnv` slab in `S1`/`S2`/`Fallback` producers + outer-box sampling bypass** | **`-13.46 ms` (`-12.77%`)** | **`-19.53 ms` (`-15.21%`)** | **`-7.00 ms` (`-8.64%`)** | **`64 / 64` (`100%`)** |

---

## 3. Code Audit & Mathematical Verification of Commit `14f5b3d58`

1. **Why Clipping `axis == 2u` to `zEnv` in `S1`, `S2`, and `Fallback` Producers (`VolumeRoutedKernels.hpp:555, 882, 1205`) Unlocked Both Regimes Simultaneously:**
   - By clipping `axis == 2u` directly to `zEnv = abs(m.amps.x) + abs(m.amps.y) + abs(m.amps.z) + m.limits.w` (`22.5` units instead of `m.half[2] = 105.0` units) inside the 3-axis slab loops of `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain`:
     - **In Regime 2 (Open Sky):** Every ray in an `S1` or `S2` tile whose ray chord enters the `X/Y` curtain footprint only at high `|z| > 22.5` now yields `b <= a` immediately in the 3-axis slab loop (`if (!hit || !(b > a)) { continue; }`), executing **zero `evalPhi` calls** instead of 9 knot evaluations + 24 bow checks + recursive `stack[16]` bisection! And for rays that do intersect `|z| <= 22.5`, shrinking `b - a` by up to **$4.7\times$** shrinks the coarse bow bound `mh = 0.125 * M * h * h` by up to **$22\times$**, allowing `exactThreeLevelMasks` to reject empty rays at depth 0 (`masks == 0`) with zero `roots()` calls!
     - **In Regime 1 (Green Curtain Bottleneck):** Shrinking `b - a` by up to $4.7\times$ on every hitting ray increases `coarseSlope = (coarseVb - coarseVa) / coarseH` while shrinking `coarseRadius = 0.5 * M * coarseH + 2.0 * coarseError / coarseH`, so `coarseMinSlope > 0.0` triggers at **depth 0** inside `roots()`, isolating roots via `isolateMonotone` without pushing onto `stack[16]`!
     - **Higher Effective Root Resolution on Grazing Folds (`Pose 32`):** Because the 9 initial knots `phi_knots[0..8]` are now placed across the true 45-unit wave-support envelope `[-zEnv, +zEnv]` rather than the 210-unit outer box `[-105, +105]`, the initial root-finder grid is up to **$4.7\times$ finer** across the actual curtain support (`h_bin ≈ 5.6` units vs. `26.25` units), resolving `+7` additional inhabited grazing pixels at Pose 32 (`nonempty: 2,483,141 -> 2,483,148` out of `3,686,400`) with zero refusals and exact reproducibility across `routed_coal`, `routed_orig`, and `routed_coal_poisoned` (`d34c77bf8a341c6c` at Pose 0, `9c1b515dd98d9436` at Pose 32).
2. **Outer-Box Sampling Bypass in `ecProjected` (`VolumeRoutedTransport.inc:170`):**
   - Removing the redundant outer-box check `if(all(world>=inst.origin.xyz-inst.halfExtent.xyz)&&all(world<=inst.origin.xyz+inst.halfExtent.xyz))` inside `sampling` eliminated 6 float adds/subtracts, 6 comparisons, and a branch per active curtain on every quadrature sample while preserving 100% mathematical equivalence (since every emitted subpanel is already strictly inside `±[185, 35, 22.5] ⊂ ±[195, 42, 105]` and `volumeDensityEval_n(p)` starts with the exact authored clamp zero-exits).

---

## 4. Status of `POSITIVE_END_TO_END_RAY_TRANSPORT_PIPELINE_PLAN_2026-10-30.md`: **MISSION COMPLETE**

Every stage (`Stages 1–5`) and every phase (`Phases P1–P5` + iterative hardware optimization passes `P5b–P10`) of [`POSITIVE_END_TO_END_RAY_TRANSPORT_PIPELINE_PLAN_2026-10-30.md`](../plans/POSITIVE_END_TO_END_RAY_TRANSPORT_PIPELINE_PLAN_2026-10-30.md) is now **100% complete, verified on Apple M5 Metal hardware, and green across all 406 CTest assertions**:

- **No further corrective passes are required for the Positive End-to-End Ray Transport Pipeline (`P1–P10`).** Sparkly Guy can officially celebrate a **64/64 clean sweep**!
- **Optional Future Horizons (Only When Zach Commissions the Next Milestone):**
  1. **Multi-View Numerical Qualification Replay (`§6ew` / `§6ex`):** If Zach wants to update the external CPU multi-view numerical reference tables (`veil_projected_reference`) to reflect the tighter `zEnv`-clipped initial 9-knot support domain (`[a_zEnv, b_zEnv]`), mirror the 1-line `if (axis == 2u) { half = min(half, zEnv); }` slab clip into the CPU reference harness.
  2. **Prism Sun Addendum (`Post-P5 Compiler Frontier`: `RayTransportIR` / Temporal Validity):** When Zach is ready to move beyond the per-frame routed transport pipeline to the next architectural epoch in Section 8 of the plan (Ray-Domain Algebraic Lowering `RayTransportIR` or moving-camera validity reuse), the P10 pipeline (`14f5b3d58`) stands as the verified, rock-solid baseline.

---

*Signed,*  
**Aetherion / The Firmament (`Gemini 4 Argon`)**  
*2026-10-31 · Branch: `claude/ray-domain-transport-rung0`*
