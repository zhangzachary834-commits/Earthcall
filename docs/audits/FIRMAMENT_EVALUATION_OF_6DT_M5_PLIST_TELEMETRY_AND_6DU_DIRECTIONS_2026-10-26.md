# Firmament Evaluation of `§6dt` — The `2,000-Byte` M5 Plist Discovery, Four Forensic Audit Catches, and Mission `§6du`

**Signed:** Aetherion / The Firmament (Gemini 3.1 Pro)  
**Date:** 2026-10-26T17:12:00-07:00  
**Human Commission:** Zachary Zhang (`"BROOOOOOOOO YAAAASSSSSS NOW GO EVALUATE THEIR LATEST WORK"`)  
**Reviewed Artifacts (`commit a428032b7`):**
- [`docs/audits/GPU_RAY_TRANSPORT_6DT_TRUE_TERMINAL_S1_QUALIFICATION_2026-10-26.md`](GPU_RAY_TRANSPORT_6DT_TRUE_TERMINAL_S1_QUALIFICATION_2026-10-26.md)
- [`docs/audits/GPU_RAY_TRANSPORT_6DT_TRUE_TERMINAL_S1_QUALIFICATION_2026-10-26.json`](GPU_RAY_TRANSPORT_6DT_TRUE_TERMINAL_S1_QUALIFICATION_2026-10-26.json)
- [`scratch/probes/ray_transport_gpu_6dt_true_terminal_s1.py`](../../scratch/probes/ray_transport_gpu_6dt_true_terminal_s1.py)
- [`scratch/probes/ray_transport_gpu_6dt_controls.py`](../../scratch/probes/ray_transport_gpu_6dt_controls.py)
- [`scratch/probes/ray_transport_gpu_6dt_capture.py`](../../scratch/probes/ray_transport_gpu_6dt_capture.py)

---

## 1. Executive Summary: What `§6dt` Genuinely Achieved vs. What Must Be Corrected

### A. Genuine Breakthroughs in `§6dt`
1. **True Removal of Global `sBuf` (`subpanels`) from `cpS1` Verified at `100.0000%` Bit Parity:**
   In [`scratch/probes/ray_transport_gpu_6dt_true_terminal_s1.py`](../../scratch/probes/ray_transport_gpu_6dt_true_terminal_s1.py) (`lines 460–545`), `s1ProducerMain` completely removes `@group(0) @binding(3) var<storage, read_write> subpanels: array<Subpanel>;` (`7` storage buffers instead of `8`), deletes the redundant `pieces -> knots` copy-back and midpoint search loop, pre-checks `singles + n > cfg.maxSubPerRay` across `pieces[0..count-1]`, and subdivides and integrates `pieces[0..count-1]` in registers with **zero global `sBuf` writes or reads** (`3,299,584 B` = `3.15 MB` of dynamic `sBuf` traffic eliminated across the **`12,889` actual sampled `S1` rays = `39.315%` of `32,784`**), achieving **`0 / 32,784` output bit differences (`maxDelta = 0.0`)**, `192 / 192` dual-resolution holdouts, and `12,102 / 12,102` protected affine endpoints.
2. **Source-Bound Apple M5 Metal Compiler Plist Telemetry Extracted Across All 7 Kernels:**
   From `/private/tmp/rt_6dt_capture/true_terminal_s1.gputrace` and `/private/tmp/rt_6dt_capture/current.gputrace`, Sparkly extracted the physical Metal compiler statistics for every kernel in the pipeline:

| Pipeline / Kernel | MSL Plist Hash | Temp Regs | Uniform Regs | Physical Compiler Spills | Thread-Invariant Spills | Total Instructions | ALU Insts | FP32 Insts |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **`s1ProducerMain` (True Terminal S1)** | `5DD8B1F6089BE319` | `83` | `88` | **`2,000 B`** | `48 B` | **`6,435`** | `4,676` | `1,528` |
| **`s2ProducerMain` (Unfused S2 Producer)** | `27660479E3738E72` | `96` | `44` | **`2,000 B`** | `32 B` | **`5,170`** | `3,510` | `691` |
| **`fallbackProducerMain` (Unfused General)** | `FA96D0A7C4D03A09` | `103` | `48` | **`2,000 B`** | `32 B` | **`5,396`** | `3,671` | `692` |
| **`classifyTilesMain` (Tile Classifier)** | `9EEAAE3F01D4F97` | `102` | `32` | **`32 B`** | `32 B` | **`750`** | `539` | `70` |
| **`cpUnfused` (General Consumer)** | `81ADFE0870BCF349` | `126` | `256` | **`32 B`** | `192 B` | **`4,280`** | `3,666` | `2,145` |
| **`cpFused` (Pair-Gradient Consumer)** | `854CCB4586E2B781` | `126` | `256` | **`32 B`** | `192 B` | **`4,284`** | `3,668` | `2,145` |
| **`currentRayMain` (Baseline Marcher)** | `473DAD2B85EA1B41` | `86` | `56` | **`0 B`** | `32 B` | **`2,530`** | `2,353` | `1,891` |

---

## 2. The Core Hardware Insight in the Plist Table (Correcting Sparkly's Attribution)

In Section 7 of [`GPU_RAY_TRANSPORT_6DT_TRUE_TERMINAL_S1_QUALIFICATION_2026-10-26.md`](GPU_RAY_TRANSPORT_6DT_TRUE_TERMINAL_S1_QUALIFICATION_2026-10-26.md), Sparkly attributed the `2,000 B` spill in `s1ProducerMain` to fusing `cnoise3` + `evalEnvelope` quadrature into the producer.

**Look closely at Rows 1, 2, 3, and 5 of the table above—they prove the exact opposite:**
1. **`cpUnfused` (Row 5) and `currentRayMain` (Row 7) contain the full `cnoise3` + `evalEnvelope` + quadrature code and spill `32 B` and `0 B` respectively!**
2. **`s2ProducerMain` (Row 2) and `fallbackProducerMain` (Row 3) contain ZERO quadrature (`no cnoise3`, `no evalEnvelope`), yet they spill the EXACT SAME `2,000 bytes` (`5,170` and `5,396` instructions) as `s1ProducerMain` (`2,000 bytes`, `6,435` instructions)!**
3. Furthermore, adding Terminal `S1` quadrature to `s1ProducerMain` increased `spilled_bytes` from `2,000 B` to **`2,000 B` (`0 bytes` of extra spill!)** and actually used **fewer** temporary registers (`83` temp regs vs `96` in `s2ProducerMain` and `103` in `fallbackProducerMain`).

**Why do `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain` all spill `2,000 bytes` (`1.95 KiB`, matching `§6dj`)?**
Because all three producer kernels share the exact same **per-member root/band/knot/piece arrays** (`stack: array<vec4f, 20>` = `320 B`, `bands: array<vec2f, 32>` = `256 B`, `knots: array<f32, 96>` = `384 B`, `pieces: array<vec2f, 48>` = `384 B`) indexed by runtime variables (`stack[top]`, `bands[nr]`, `knots[j]`, `pieces[count]`). Specializing the outer loop from 4 curtains down to 1 or 2 curtains without changing the per-member root/knot array machinery leaves the **`2,000 B` thread-local stack frame** completely untouched!

---

## 3. Four Forensic Audit Catches in `§6dt` That Must Be Repaired (`§6du`)

Before Sixth-One Sun signs off on the `§6dt` record, four concrete defects in the `§6dt` timing and control harness must be corrected:

### Catch 1 — Severe Host/OS Scheduler Stalls & `7.55 ms` Quantization in the `§6dt` Timing Run (`15.68-Second` Spikes!)
Look at the raw timing data in [`GPU_RAY_TRANSPORT_6DT_TRUE_TERMINAL_S1_QUALIFICATION_2026-10-26.json`](GPU_RAY_TRANSPORT_6DT_TRUE_TERMINAL_S1_QUALIFICATION_2026-10-26.json) (`lines 250–650`):
- In every previous clean run on this exact Apple M5 (`§6dd` through `§6ds`, including `§6ds` just 2 hours earlier), `Current (Mode 0)` at Repeat 1 ran in **`2.49–2.55 ms`** (`~4.65 ms` at R=2, `~8.23 ms` at R=4), and `Mode 3` ran in **`2.54–2.60 ms`**.
- In `§6dt`, `Current (Mode 0)` jumped **`4.3×`** to **`10.873 ms`**, with every single sample quantized to integer multiples of **`~7.55 ms`** (`7.54 ms`, `9.08 ms`, `15.11 ms`, `22.69 ms`, `30.24 ms`, `37.75 ms`).
- Even more telling, look at `pass_breakdown_ms` and `warm_vs_cold_routed_ms` in the JSON:
  - `pass0_reset` (**1 single thread executing 3 `atomicStore` instructions!**): `min = 7.59 ms`, `median = 16.71 ms`, **`max = 1,121.86 ms` (`1.12 seconds`!)**.
  - `pass7_fallback_cp` (**32,784 threads that all exit on line 1 because `refused == 0`!**): `median = 258.51 ms`, **`max = 2,437.94 ms` (`2.44 seconds`!)**.
  - `warm_vs_cold_routed_ms["4"]["warm"]`: **`max = 15,679.03 ms` (`15.68 seconds`!)**.
- **Cause:** Measuring `wgpuQueueSubmit` + `wgpuDevicePoll(gpu.device, true, nullptr)` while the process/GPU is throttled (or under display/background contention) measures a **`~7.55 ms` OS polling/vsync quantum and multi-second scheduler stalls** per `wgpuQueueSubmit`, NOT GPU shader execution time! Because `pass_breakdown_ms` submitted 8 separate command buffers with `wgpuDevicePoll`, each empty/tiny submission paid `7.55–16.71 ms` of host poll latency (or multi-second stalls), making `8 passes × ~24 ms` look like `190 ms`!
- **Also:** The script that generated `GPU_RAY_TRANSPORT_6DT_TRUE_TERMINAL_S1_QUALIFICATION_2026-10-26.json` (`pass_breakdown_ms` and `warm_vs_cold_routed_ms`) was **not committed** (`ray_transport_gpu_6dt_true_terminal_s1.py` outputs the standard `validate_6dd` schema, not the custom JSON schema in `docs/audits/GPU_RAY_TRANSPORT_6DT_TRUE_TERMINAL_S1_QUALIFICATION_2026-10-26.json`).

### Catch 2 — Vacuous `sBuf` Packet Comparison on `S1` Rays in `ray_transport_gpu_6dt_controls.py` and `ray_transport_gpu_6dt_true_terminal_s1.py`
1. Look at [`scratch/probes/ray_transport_gpu_6dt_controls.py`](../../scratch/probes/ray_transport_gpu_6dt_controls.py) (`lines 20–24` and `57–61`):
   - `baselineReview()` calls `execute(1, 1, false)` (Guarded Bulk), which writes the reference subpanels into `sBuf`.
   - Next, `checkReview()` calls `execute(3, rep, false)` **without clearing or poisoning `sBuf`**!
   - Because `s1ProducerMain` in `§6dt` no longer binds or writes `sBuf`, the bytes in `sBuf` at `hs[i].subpanelOffset + k` for `S1` rays are **leftover bytes from `execute(1, 1, false)`**!
   - Thus, when `checkReview()` checked `subs[hs[i].subpanelOffset + k] == rs[rh[i].subpanelOffset + k]` on `S1` rays, it was comparing **`execute(1)`'s stale `sBuf` bytes against `execute(1)`'s saved `refS` bytes**!
2. Similarly, in [`scratch/probes/ray_transport_gpu_6dt_true_terminal_s1.py`](../../scratch/probes/ray_transport_gpu_6dt_true_terminal_s1.py) (`lines 896–902`), on `0x40000000u` (`S1` completed) rays, the census loop copied the CPU baseline's `gpuSubpanels` (`b`) into `singles` (`singles.push_back({b.s0, b.s1, b.mediumIndex})`) and passed it to `validate_6dd` as if the GPU had written those packets.
3. **Fix:**
   - Poison/clear `sBuf` and `oBuf` before `execute(3, rep, false)` in `checkReview()`!
   - On `S1` completed rays (`(flagBits & 0x40000000u) != 0u`), verify `hs[i].subpanelCount == rh[i].subpanelCount` and verify that **`sBuf` remains untouched (poisoned/zero)** on `S1` rays (proving `0 bytes` of `sBuf` writes!), while verifying all used `sBuf` and `oBuf` packet words on `S2` and `Fallback` rays (`(flagBits & 0xC0000000u) == 0u`)!

### Catch 3 — Misplaced `finalOutputWrites` (`auditWrites[2*id + 1]`) Atomic Increment on Refused Rays
Look at [`scratch/probes/ray_transport_gpu_6dt_controls.py`](../../scratch/probes/ray_transport_gpu_6dt_controls.py) (`lines 118, 129, 137–148, 204–206`):
- Instead of instrumenting `fallbackCP` (`fallbackShader`) at its `results[id] = ...` write site, `ray_transport_gpu_6dt_controls.py` increments `auditWrites[id * 2u + 1u]` (`finalPublishes`) **inside `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain` right before they refuse a ray (`FALLBACK | reason`) and return without writing `results[id]`**!
- **Why that is invalid:** An atomic incremented inside a producer when it *refuses* to write `results[id]` is **not** a witness that `fallbackCP` wrote `results[id]`! If `fallbackCP` were accidentally omitted or overwrote a non-refused ray, `auditWrites[id * 2u + 1u]` would not catch it!
- **Fix:** Remove `atomicAdd(&auditWrites[id * 2u + 1u], 1u)` from all producer refusal returns (`FALLBACK | reason`), and place `atomicAdd(&auditWrites[id * 2u + 1u], 1u)` **inside `fallbackShader` (`fallbackCP`) at the exact line where `fallbackCP` writes `results[id]`**! That way, every increment of `auditWrites[id * 2u + 1u]` corresponds 1-to-1 with an actual physical write to `results[id]`.

### Catch 4 — Leftover Git Merge Conflict Markers in `OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md`
At lines `9837` and `9884–9888` of [`agent intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md`](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md) (and around lines `8700–8850`), literal conflict markers (`<<<<<<< HEAD`, `=======`, `>>>>>>> ...`) remain in the file and must be cleaned up.

---

## 4. Mission `§6du` for Sparkly Guy & Sixth-One Sun: Clean Corrective Qualification + The Zero-Spill Architectural Question

### Step 1 — Corrective Repair of `§6dt` (`§6du` Clean Verification & Unthrottled Timing)
1. **Fix `auditWrites[2*id + 1]` in `ray_transport_gpu_6dt_controls.py`:** Remove `atomicAdd(&auditWrites[id * 2u + 1u], 1u)` from all producer refusal returns (`FALLBACK | reason`) and inject it directly into `fallbackShader` (`fallbackCP`) at its `results[id]` write site. Add a 5th mutation `--mutation fallback-no-publish` (or `--mutation fallback-double-publish` where `fallbackCP` ignores the `FALLBACK` bit check) to prove `auditWrites[2*id + 1]` catches `fallbackCP` ownership bugs!
2. **Fix `sBuf` Poisoning & Non-Materialized `S1` Packet Verification:** Clear/poison `sBuf` and `oBuf` before `execute(3)` in `checkReview()`. For `0x40000000u` (`S1` terminal) rays, assert `hs[i].subpanelCount == rh[i].subpanelCount` and assert that `sBuf[hs[i].subpanelOffset .. +subpanelCount]` was **not** written (`0` global `sBuf` writes proved by memory check!), while asserting exact `sBuf`/`oBuf` packet parity on all non-terminal accepted rays (`S2` and `Fallback`).
3. **Compare All 4 Modes Cleanly in One Committed Generator (`Mode 0 = Current`, `Mode 1 = Guarded Bulk`, `Mode 2 = §6dr Canonical Routed Unfused Control`, `Mode 3 = §6dt True Terminal S1`) Under Unthrottled Clocks:**
   - Replace `Mode 2` in `ray_transport_gpu_6dt_true_terminal_s1.py` with the **Frozen `§6dr` Canonical Routed Unfused pipeline** (with the direct `for (p = 0..count)` subpanel emission cleanup in `s1ProducerMain`), and run two clean ordered 32-sample series when the GPU is unthrottled (`Current Mode 0` back at `~2.5 ms`, not `10.87 ms`!).
   - Commit the exact Python script that writes the final JSON report so 100% of the JSON is reproducibly generated from committed source.

### Step 2 — The Architectural Counsel for the Council (`Why All `roots()` Kernels Spill `2,000 B` and What Can Actually Beat `Current`'s `0 B` Spill`)
Once Step 1 settles the clean unthrottled comparison between `Current` (`0 B` spill, `2,530` insts), `§6dr Routed Unfused` (`2,000 B` producer spill + `32 B` consumer spill), and `§6dt True Terminal S1` (`2,000 B` spill, `6,435` insts, `0 B` `sBuf` traffic):
- Look at what the M5 compiler plist table (`2,000 B` spill in `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain` vs `0 B` spill in `currentRayMain`) tells us:
  - **Splitting by curtain count (`S1`, `S2`, `Fallback`) while keeping the `roots()` + `stack[20]` + `bands[32]` + `knots[96]` + `pieces[48]` interval solver inside `S1` and `S2` cannot eliminate the `2,000 B` stack frame spill**, because those four dynamically-indexed arrays live inside the single-member solver itself!
  - Conversely, why does `currentRayMain` compile to **`0 B` spill and `2,530` instructions**? Because it has **zero dynamically-indexed arrays**—it steps forward in scalar registers!
  - And why does `classifyTilesMain` compile to **`32 B` spill and `750` instructions**? Because it bounds active curtains and `[enterArr[m], exitArr[m]]` without `stack[20]`, `bands[32]`, `knots[96]`, or `pieces[48]`!
  - Therefore, if `§6dt` (even when unthrottled) still loses to `Current` because of the `2,000 B` per-thread stack spill in `roots()`/`knots[96]`/`bands[32]`/`pieces[48]`, then **any winning GPU representation on M5 MUST eliminate the dynamically-indexed `stack[20]` / `bands[32]` / `knots[96]` / `pieces[48]` arrays from the `S1`/`S2` fast-path kernel altogether**!
