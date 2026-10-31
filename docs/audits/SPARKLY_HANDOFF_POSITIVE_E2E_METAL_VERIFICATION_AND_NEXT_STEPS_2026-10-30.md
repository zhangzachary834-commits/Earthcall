# Handoff to Sparkly Guy (`Gemini 3.8 Flash` / Antigravity) — Live Apple M5 Metal Verification, Dual-Regime ABBA Timing, and Next Steps for Positive End-to-End Ray Transport (`Phases P1–P5`)

- **Date:** 2026-10-30T18:52:00-07:00 (PDT)
- **From:** Aetherion / The Firmament (`Gemini 4 Argon`)
- **To:** Sparkly Guy (`Gemini 3.8 Flash` from Antigravity), Zachary Zhang, and Sixth-One Sun (`GPT-6.1 Sol`)
- **Branch & Commit:** `claude/ray-domain-transport-rung0` at `4cbc241cb` in `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`
- **Governing Plan:** `docs/plans/POSITIVE_END_TO_END_RAY_TRANSPORT_PIPELINE_PLAN_2026-10-30.md`
- **P1–P5 Code Review & CPU Verification Audit:** `docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_P1_TO_P5_VERIFICATION_2026-10-30.md`

---

## 1. Mission Brief for Sparkly Guy

Yo Sparkly Guy! Clawd Opus 4.6 implemented Phases **P1–P3** (`6e6c1f2b4`, `2434491de`, `a8d7d5de0`) and then ran out of energy. We stepped in, audited his P1–P3 commits, fixed **4 critical bugs** (including a Pose 54 `reason = 8` capacity overflow and a reintroduced Curtain-3 `stack[15]` truncation bug in `a8d7d5de0`, a broken `workLimit == 0` forced refusal on `Empty` tiles in `6e6c1f2b4`, a transient `bufferPool()` `uniformSpecs` pointer hazard in `2434491de`, and a stale `enableRouteObserver` S2 check), implemented **Phase P4 (Cooperative 16-Lane Tile Classifier)**, and verified **100% bit-for-bit float32 parity** across all **64 canonical poses** (`921,600` tile-frames, `1,580,416` active veil rays) on CPU.

**Why you are taking the baton right now:**
Our shell environment ran inside a macOS Seatbelt sandbox that blocked `MTLCreateSystemDefaultDevice` (`gpu.init()` returns `false` when IOKit/Metal user clients are sandboxed). Your Antigravity environment has **live Apple M5 Metal GPU access**!

Your mission is to run the **Live Apple M5 Metal GPU Verification & Dual-Regime Timing Suite** on `4cbc241cb`, record the real Metal GPU evidence, and—if all 5 hardware gates pass—execute the **Next Optimization & Polish Steps** below.

> **CRITICAL STANDING RULES:**
> 1. **Never run `ctest` in parallel** (running all tests concurrently takes ~60 GB RAM and restarts Zach's Mac). Always run one binary at a time sequentially.
> 2. **`saves/` and `/Applications` are strictly read-only.** Never modify any `.ecform` or `.ecmatter` file.
> 3. **Do not recompile everything from scratch**—`build/earthcall_webgpu` and `/private/tmp/ec-positive-e2e-verify/verify` are **already compiled and linked** against `4cbc241cb`!

---

## 2. Step-by-Step Tasks for Sparkly Guy

### Task 1 — Run the Pre-Built Native Metal Verification & Dual-Regime Timing Binary (`/private/tmp/ec-positive-e2e-verify/verify`)

We already compiled and linked a dedicated native Metal GPU verification and Dual-Regime timing executable against the exact `4cbc241cb` object files at:
```bash
/private/tmp/ec-positive-e2e-verify/verify
```
*(Its source code is archived in `scratch/probes/positive_e2e_metal_verify.cpp`.)*

Run it from `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`:
```bash
cd /Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport
/private/tmp/ec-positive-e2e-verify/verify
```

**What `/private/tmp/ec-positive-e2e-verify/verify` tests on the live Apple M5 Metal GPU:**
1. **Gate 1 — WGSL Observer String-Patch Check (`WGSL_OBSERVER_PATCH_OK`):** Confirms all 4 `enableRouteObserver` string-patch anchors in `VolumeRoutedTransport.inc` match uniquely on the Phase P4 `kClassifyWgsl` shader.
2. **Gate 2 — Live Metal GPU Zero-Budget Forced Refusal (`workLimit == 0`):** Dispatches `KernelRoutedTransport::execute(..., workLimit = 0, skipEmptyTiles = false)` on Pose 25 (which has `Empty`, `S1`, `Ordered-S2`, and `Fallback` tiles) and verifies via Metal buffer readback that **every valid ray** has `counts[i].w == 0x80000001u` (`zero_budget_failures == 0`).
3. **Gate 3 — Live Metal GPU 64-Pose Bit-for-Bit Readback Parity (`Routed P1–P4` vs. `Force-Fallback`):**
   - Across all **64 canonical poses** (`0..63`) on `saves/zones/Northern Veil/zone.ecform` at `640×360` (`230,400` pixels = `14,400` tiles per pose):
   - Dispatches `Routed (P1–P4)` and `Force-Fallback` (`forceFallback = true`), reads back `outT`, `outR`, `outG`, `outB`, and `counts`, and verifies:
     - `total_pixel_diffs == 0` (`max_ulp_diff == 0`, bit-for-bit identical IEEE-754 `f32` outputs)
     - `total_work_diffs == 0` (identical `counts.z` work and `counts.xy` subpanel/overlap counts)
     - `total_refused_rays == 0`
4. **Gate 4 — Live Metal GPU Route Observer Cohort Census (`64` Poses):**
   - Reads back `routeCounts` from the live Metal `observerPipe` across all 64 poses and confirms it matches our CPU float32 verifier (`positive_e2e_cpu_verify.json`):
     - **All 64 Poses (`0..63`):** `Empty = 14.69%`, `S1 = 48.43%`, `Ordered-S2 = 29.06%`, `Fallback = 7.82%` (**`92.18%`** diverted from `Fallback`)
     - **Regime 1 (Full-Screen Green Intersection, Poses `0..19` & `51..63`, `N = 33`):** `Empty = 0.01%`, `S1 = 45.19%`, `Ordered-S2 = 47.09%`, `Fallback = 7.71%` (**`92.29%`** diverted from `Fallback`)
     - **Regime 2 (Open Sky / Partial Veil, Poses `20..50`, `N = 31`):** `Empty = 30.32%`, `S1 = 51.87%`, `Ordered-S2 = 9.87%`, `Fallback = 7.94%` (**`92.06%`** diverted from `Fallback`)
5. **Gate 5 — Live Metal GPU Dual-Regime Wall-Clock Benchmark (`640` Frames = `10` Passes × `64` Poses):**
   - Times `Routed (P1–P4)` vs. `Force-Fallback` across 10 full 64-pose passes (`640` frames each) and writes `/private/tmp/ec-positive-e2e-verify/positive_e2e_verification.json` with:
     - `routed_all_ms_per_frame` vs. `fallback_all_ms_per_frame`
     - `routed_regime1_green_ms_per_frame` vs. `fallback_regime1_green_ms_per_frame` (the full-screen green curtain intersection bottleneck Zach spotted!)
     - `routed_regime2_sky_ms_per_frame` vs. `fallback_regime2_sky_ms_per_frame`

---

### Task 2 — Run Existing Native WebGPU Regression Witnesses Sequentially

After `/private/tmp/ec-positive-e2e-verify/verify` finishes, run the existing native WebGPU volume regression witnesses **one at a time** to confirm zero regressions across the broader renderer:
```bash
./build/webgpu_volume_zero_proof_test
./build/webgpu_volume_support_certificates_test
./build/webgpu_line_quadrature_test
```
*(Remember: run them strictly one after another, never in parallel!)*

---

### Task 3 — Inspect Apple M5 Metal Compiler Spill / Occupancy & Tune `S1`/`S2` Private Array Capacities If Helpful (Optional Bonus Step)

Here is an important discovery from our 64-pose float32 trajectory verification that you should know before touching any array sizes in `VolumeRoutedKernels.hpp`:
- In commit `a8d7d5de0`, Clawd Opus 4.6 shrank `pieces` in `kS1UnfusedProducerWgsl` and `kS2ProducerWgsl` to `16` (`knots[32]`, `bands[12]`) and reverted `stack` from `20` back to `15`.
- **Why that failed on Pose 54:** On **Pose 54 (`tile 13800, ray 11, member 0`)**, a single curtain on a grazing ray emits **`30` subpanels** (`> 16`), which caused `S1` to refuse with `reason = 8` (`count >= 16u`) while `Fallback` (`pieces[48]`) succeeded! And `stack[15]` reintroduced the Curtain-3 (`maxDepth = 14`, peak stack `15`) truncation bug from `§6fe`.
- **What we did in `4cbc241cb`:** We restored `knots[96]`, `bands[32]`, `pieces[48]`, and `stack[20]` in `kS1UnfusedProducerWgsl` and `kS2ProducerWgsl` so `S1`, `S2`, and `Fallback` share **100% identical refusal thresholds** (`0` refusals and `0` mismatches across all `1,580,416` rays). Even with `96/32/48/20`, `S1` and `S2` eliminate `Fallback`'s 4-curtain outer loop, `pieceMembers[48]` (`192 B`), `memStart[4]`, `memCount[4]`, `head[4]`, `pIdx[4]`, the 4-way merge + overlap clipper loop, and the `@binding(6)` `overlaps` buffer binding.
- **What you can test on Metal if Regime 1 (`S1`/`S2`) still shows private-array scratch traffic:**
  - Across all `1,580,416` active veil rays in all 64 poses, the **maximum single-curtain piece count** is `30` (and for 2 curtains in `S2`, at most `48` total emitted directly via `subpanels[(outIdx++) * cfg.rayCount + id]` without needing to buffer both curtains' pieces simultaneously—because `S2` emits curtain `firstM`'s pieces immediately to `subpanels` before running curtain `secondM`, or if evaluating `rawM0` then `rawM1`, only needs to buffer `rawM0`'s pieces when `m1First == 1u`!).
  - Even better: in `kS1UnfusedProducerWgsl`, `pieces[48]` is only used at the very end of the shader in a trivial loop:
    ```wgsl
    for (var i: u32 = 0u; i < count; i++) {
      subpanels[i * cfg.rayCount + id] = Subpanel(pieces[i].x, pieces[i].y, member, 0u);
    }
    ```
    In `S1`, subpanels are discovered in strictly increasing depth order (`curA` to `curB`)! If `S1` writes each discovered piece **directly to `subpanels[count * cfg.rayCount + id]`** as it is produced (and only suppresses `counts[id]` if `refused`), **`S1` does not need the local `var<private> pieces: array<vec2<f32>, 48>` (`384` bytes/thread) array at all!**
  - Wait to see the live Metal timing numbers from **Task 1** first. If `Routed (P1–P4)` already crushes `Force-Fallback` in both Regime 1 and Regime 2, record the win first before testing direct-write `pieces`-elimination in `S1` as a clean A/B follow-up!

---

### Task 4 — Record Live Metal Results in an Audit Report & Post Back to Agent Intercom

Once Tasks 1–2 complete:
1. Write `docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_METAL_BENCHMARK_2026-10-30.md` with the exact live Apple M5 Metal tables from `/private/tmp/ec-positive-e2e-verify/positive_e2e_verification.json` (Gate 1–5 results, Regime 1 Full-Screen Green ms/frame vs. Regime 2 Open Sky ms/frame, and `S1`/`S2`/`Empty`/`Fallback` percentages).
2. Append your summary to `agent intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md` and `agent intercom/conversation_links.jsonl`, and commit on `claude/ray-domain-transport-rung0`.
