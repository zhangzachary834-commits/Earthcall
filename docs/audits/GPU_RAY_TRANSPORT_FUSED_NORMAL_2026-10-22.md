# §6cv — Sparkly Guy: Fused Normal Producer+Consumer Execution Passes Strict Bit-Parity and Accuracy, Rejected on Connected Economics

**Harness:** Sparkly Guy (`Gemini 3.8 Flash High / Antigravity`) · session `50cf28df-09e9-4c2b-a416-0e859a729c1a`  
**Commissioned By:** Sixth-one Sun (`Codex / GPT-6.1 Sol`) in Intercom (`OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md` commit `e22abe647` / §6cu review `docs/audits/GPU_RAY_TRANSPORT_ROOT_REVIEW_2026-10-22.md`)  
**Date:** `2026-10-22`  
**Platform:** Apple M5 (10-core GPU, Metal, completion-fenced host timing, GPU timestamps disabled)  
**Codebase:** Worktree `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop` on branch `claude/ray-domain-transport-rung0`  
**Parent Checkpoint:** Commit `24390a896` / §6cu corrected guarded baseline (`ray_transport_gpu_producer_merge_sweep.py`)  
**Audit Record:** [`GPU_RAY_TRANSPORT_FUSED_NORMAL_2026-10-22.json`](GPU_RAY_TRANSPORT_FUSED_NORMAL_2026-10-22.json)  

---

## 1. Executive Summary & Verdict

Following Codex's §6cv assignment, we tested the lowering/dispatch/storage hypothesis: **fusing normal producer and consumer execution into a single kernel (`fusedMain`) while retaining the existing production-generated native fallback dispatch (`fallbackMain`)**.

1. **Storage Elimination:** Completely eliminated the normal-path global schedule buffer round-trip across VRAM (**43,537,968 bytes / 42.5 MB** per 32,784-ray query). Normal rays evaluate curtain bounds, roots, 4-way merge, and linear mask sweep directly into thread-private arrays (`local_subpanels: array<Subpanel, 32>` and `local_overlaps: array<Overlap, 8>`), then immediately integrate optical transport via the validated interleaved $A^*$ quadrature consumer.
2. **Retained Fallback Dispatch:** Retained the existing native marcher fallback pipeline (`fallbackCP`) unchanged as a secondary dispatch (2 dispatches per tile total). On budget or capacity failure (`reason != 0u` or `cfg.budget == 0u`), `fusedMain` writes `rays[id].counts = vec4u(0u, 0u, 0u, FALLBACK | reason)` and discards partial transport so the fallback writes the entire result once. On normal paths, `fusedMain` clears bit 31 (`rays[id].counts = vec4u(0u, singles, 0u, overs)`), causing `fallbackMain` to skip.
3. **Strict Bit-Exact Parity (100.0% Same-Device Match):** Verified across all **32,784 distinct native rays** (`2560×1440` viewport, camera `inside`, clock `7.25`):
   - **0 / 32,784 differing output rays against the guarded baseline**.
   - `max_delta_RGB_between_fused_and_unfused`: **0.0** (strict bitwise IEEE-754 equality on every channel).
   - `max_delta_T_between_fused_and_unfused`: **0.0** (strict bitwise IEEE-754 equality on transmittance).
   - Subpanels count: **247,326** (identical to guarded baseline).
   - Overlaps count: **1,799** (identical to guarded baseline).
   - Normal fallback rays: **0 / 32,784** (all normal rays handled in fused kernel without fallback).
   - Forced native fallback (`budget = 0`): **100% bit-identical to native baseline** across all 32,784 rays.
   - Poisoned output buffer check: **PASSED** (all results fully written).
4. **Accuracy Gates:** All 32,784 rays pass independent AST agreement ($< 10^{-5}$ tolerance). All 12 reference accuracy gates pass at both 65,536 and 32,768 intervals across all 192 holdout rays.
5. **Economic Verdict:** **REJECTED.** In Mode 2 bounded tile streaming across all capacities (8,192, 16,384, 32,768) and all query repetitions (1, 2, 4), Mode 2 trails the Current Marcher (Mode 0) by 0.59× to 0.99×. At the primary 32,768-capacity / 4-repeat comparison point:
   - **Mode 0 (Current Marcher):** **1.582799 ms / query**
   - **Mode 1 (Bulk Connected):** **1.729075 ms / query**
   - **Mode 2 (Fused Streamed):** **2.069671 ms / query** (0.7648× speedup; ~30.8% slower than Current)

Eliminating 43.5 MB of VRAM round-trip traffic did NOT yield an economic win because the increased thread-private register and stack pressure in the fused kernel reduced hardware occupancy and instruction throughput on Apple Silicon Metal.

---

## 2. Complete-Query Economics Across Modes

Tested on Apple M5 Metal; host completion-fenced timing with GPU timestamps disabled. Measured across 32,784 distinct native rays using the 4-block balanced paired order (`[0, 1, 2, 2, 1, 0, 2, 1, 0, 0, 1, 2] * 2`).

### Bounded Streaming (Mode 2) vs Bulk (Mode 1) vs Current Marcher (Mode 0)

All times are normalized milliseconds per complete 32,784-ray query:

| Ring Capacity | Dispatches / Tile Chunks | Repeats | Mode 0 Current Marcher | Mode 1 Global Bulk | Mode 2 Fused Streamed | Speedup (Mode 0 / Mode 2) |
|---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **8,192** | 5 tiles (16-ray tail) | 1 | 3.805156 ms | 3.819703 ms | 5.987203 ms | 0.6355× |
| **8,192** | 5 tiles (16-ray tail) | 2 | 3.795307 ms | 3.810799 ms | 5.872638 ms | 0.6463× |
| **8,192** | 5 tiles (16-ray tail) | 4 | 2.434365 ms | 2.629569 ms | 4.107941 ms | 0.5926× |
| **16,384** | 3 tiles (16-ray tail) | 1 | 2.531870 ms | 2.545198 ms | 3.201120 ms | 0.7909× |
| **16,384** | 3 tiles (16-ray tail) | 2 | 1.897768 ms | 1.979641 ms | 2.694729 ms | 0.7043× |
| **16,384** | 3 tiles (16-ray tail) | 4 | 1.726825 ms | 1.908048 ms | 2.469507 ms | 0.6993× |
| **32,768** | 2 tiles (16-ray tail) | 1 | 2.504406 ms | 2.535422 ms | 2.519979 ms | 0.9938× |
| **32,768** | 2 tiles (16-ray tail) | 2 | 1.891888 ms | 1.902263 ms | 2.546924 ms | 0.7428× |
| **32,768** | 2 tiles (16-ray tail) | 4 | **1.582799 ms** | **1.729075 ms** | **2.069671 ms** | **0.7648×** |

### Batched Fence Series Component Breakdown (Per Query)
- **1 Dispatch / Fence:** Current = 5.272233 ms, Consumer Only = 2.427410 ms, Connected = 5.999979 ms
- **2 Dispatches / Fence:** Current = 3.586923 ms, Consumer Only = 1.384420 ms, Connected = 3.861703 ms
- **4 Dispatches / Fence:** Current = 3.097431 ms, Consumer Only = 1.221155 ms, Connected = 3.552622 ms

---

## 3. Lowering, Storage, and Hardware Diagnosis

1. **Why the Fused Kernel Achieves 100% Bit-Exactness:**
   - The producer and consumer logic were lowered using the exact validated mathematical fragments from §6cq and §6cr.
   - When thread-local arrays are sized to maximum per-ray capacities (`local_subpanels: array<Subpanel, 32>` and `local_overlaps: array<Overlap, 8>`), the order of subpanel emission, 4-way merge, linear mask sweep, and interleaved quadrature consumption matches the separate bulk pipeline bit-for-bit.
   - On this 32,784-ray capture, max delta for both RGB and T is exactly **0.0** across all rays.
2. **Why Eliminating VRAM Traffic Lost Economics:**
   - The separate pipeline wrote 43.5 MB of schedule data from producer to global VRAM, then read it back in consumer. Eliminating this traffic saves memory bandwidth.
   - However, in the fused kernel, each invocation must allocate storage in registers/spill space for:
     - Producer state: `stack: array<vec4f, 20>`, `depthBits: vec3u`, `bands: array<vec2f, 32>`, `knots: array<f32, 96>`, `pieces: array<vec2f, 48>`, `pieceMembers: vec3u`.
     - Schedule state: `local_subpanels: array<Subpanel, 32>` (32 × 16 bytes = 512 bytes) + `local_overlaps: array<Overlap, 8>` (8 × 16 bytes = 128 bytes).
     - Consumer quadrature state: noise evaluation registers, media parameters, optical integration accumulators.
   - On Apple Silicon's unified memory architecture, memory bandwidth is very high (> 100 GB/s on M5), so the 43.5 MB round-trip was relatively fast. Conversely, thread-private register spilling and reduced warp/wavefront occupancy directly penalize execution throughput for the compute-intensive bisection and noise loops.
   - Furthermore, in Mode 2 bounded streaming, tile chunking (2 tiles at 32,768; 5 tiles at 8,192) introduces multiple command encoder passes and host completion fences that compound the per-tile dispatch latency.
3. **Conclusion for Lowering Strategy:**
   - Fusing producer + consumer into a monolithic thread-private kernel without restructuring or compressing the internal state increases register pressure beyond the breakeven threshold.
   - Saved storage bandwidth does NOT automatically translate into execution latency reduction.

---

## 4. Shipping Standing & Gate Audit

All three gates remain **OPEN**:
- **Gate 1 (Profitable Repeatable Connected Distinct-Ray Execution):** **OPEN.** The fused normal path trails Current Marcher (1.58 ms vs 2.07 ms at 32,768 cap / 4 reps). Candidate is rejected for promotion.
- **Gate 2 (Generic OntoMath / Lifetimes / Full-Frame Integration):** **OPEN.**
- **Gate 3 (Inhabited Moving p95 / Zach Acceptance):** **OPEN.**

No production C++ engine files, shaders, or save files were modified. The native timing lane is released.

---

## 5. Artifacts & Reproduction

- **Probe Source:** `scratch/probes/ray_transport_gpu_producer_fused_normal.py` in worktree `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop` (branch `claude/ray-domain-transport-rung0`).
- **Machine Audit Record:** [`docs/audits/GPU_RAY_TRANSPORT_FUSED_NORMAL_2026-10-22.json`](GPU_RAY_TRANSPORT_FUSED_NORMAL_2026-10-22.json).
- **Reproduction Command:**
  ```sh
  cd /Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop
  python3 scratch/probes/ray_transport_gpu_producer_fused_normal.py \
    --root . \
    --work /private/tmp/earthcall-fused-normal \
    --case /private/tmp/earthcall-wide-case/manifest.json \
    --label fused_normal
  ```

---

*Signed: Sparkly Guy / Gemini 3.8 Flash High / Antigravity / session `50cf28df-09e9-4c2b-a416-0e859a729c1a` / `2026-10-22T19:08:00-07:00`.*
