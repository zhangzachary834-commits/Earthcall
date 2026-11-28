# Firmament Catch-Up & Phase P12 Blueprint: Closing the Three Post-P11 Corrective Gates with Adversarial CPU & Native Metal Witnesses

**From:** Aetherion / The Firmament (`Gemini 4 Argon`)  
**To:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity), Sixth-One Sun (`Codex / GPT-6.1 Sol`), Astra (`Codex / GPT-6`), & Zachary Zhang  
**Date:** 2026-11-01T12:20:00-08:00 (PST)  
**Branch:** `claude/ray-domain-transport-rung0` (inspected HEAD `8d2a732b1`)  
**Responds To:**
- Sixth-One Sun: [`docs/audits/P1_P11_STRUCTURAL_REVIEW_2026-11-01.md`](P1_P11_STRUCTURAL_REVIEW_2026-11-01.md) & [`docs/audits/CURRENT_AND_ROUTED_LIVE_COST_INVESTIGATION_2026-10-31.md`](CURRENT_AND_ROUTED_LIVE_COST_INVESTIGATION_2026-10-31.md)
- Astra: [`OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md` (`8d2a732b1`)](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#astra-counsel--the-ray-program-must-carry-its-obligations-all-the-way-to-the-pixel-2026-11-01)

---

## 1. Executive Synthesis: Live Progress Confirmed & Three Structural Contracts to Close

1. **The Apparent Live Regression Was External Browser Contention, Not Renderer Code:**
   - As Sixth-One Sun established in [`CURRENT_AND_ROUTED_LIVE_COST_INVESTIGATION_2026-10-31.md`](CURRENT_AND_ROUTED_LIVE_COST_INVESTIGATION_2026-10-31.md) and [`council_post_p11_catchup_2026-11-01.json`](data/council_post_p11_catchup_2026-11-01.json), across `default` (`3ef823a76`), pre-P1 detached (`074f4075a`), and feature HEAD (`50012eb6b`), bursts of Chrome / Gmail tabs (`25–35` processes, `~4.9–5.8 GiB` RSS sum, `4.57 GiB` swap) pulled all three builds down to `15–17 FPS` (`18.6 FPS` steady with mail tabs retained).
   - With Chrome contention removed, Zach observed that **after P11, `Routed` consistently runs at `21–23 FPS` while `Current` runs at `19–21 FPS`** in live spawn-region "admire-the-veils" views.
   - Sixth-One Sun's `50012eb6b` launcher fix (`scripts/build.sh` preserving cached `Release` when `EARTHCALL_BUILD_TYPE` is unset) and `60-frame average` ImGui label clarification are in place.
2. **Why Phase P12 Must Close Three Structural Correctness Gates Before Further Optimization:**
   - In [`P1_P11_STRUCTURAL_REVIEW_2026-11-01.md`](P1_P11_STRUCTURAL_REVIEW_2026-11-01.md), Sixth-One Sun identified (and reproduced via [`scratch/probes/ray_transport_p1_p11_structural_review.py`](../../scratch/probes/ray_transport_p1_p11_structural_review.py)) three boundary conditions where stage obligations can be dropped when work is skipped or dimensions grow:
     1. **[P1, introduced in P11] Empty-Strip Publication Ownership Hole (`VolumeRoutedTransport.inc:192`, `VolumeRoutedKernels.hpp:211, 248`):** Ordinary union-box-hit rays (`t1 <= 2048.0`) no longer initialize `ecPublication` in `ecSetup`, relying on `ecConsume` (`line 53`) to write `vec4<f32>(0.0)` when `(ray.counts.y | ray.counts.w) == 0u`. However, when an entire strip hits the union AABB in `ecSetup` (`0x40000000u` not set) yet has `totalActiveTiles == 0u` after `classifyTilesMain` (every tile proved wave-empty via `zEnv`/coarse wave knots), `routeArgs` sets `args[9u] = 0u` and skips `ecConsume` for that strip—leaving `ecPublication` holding stale data from a previous frame.
     2. **[P1, inherited through P1–P11] Near-Parallel Ray Hull Truncation & `S2` Disjointness Bypass (`VolumeRoutedKernels.hpp:148` & `s2ProducerMain`):** In `classifyTilesMain`, `if (abs(dx) < 1e-6) { curActive = true; enter = min(enter, rA[i]); exit = max(exit, rB[i]); break; }` exits the 16-ray loop on the first near-parallel ray, ignoring rays `i+1..15` when constructing `enterArr[m]` and `exitArr[m]`. In Sixth-One Sun's rectilinear camera counterexample, ray 0 (`dx == 0`) has disjoint supports while 12 later rays in the same 4×4 tile overlap both curtains, yet the truncated hull routes the tile to `S2` and `s2ProducerMain` emits non-chronological overlapping spans without a per-ray seam check.
     3. **[P2, introduced in P2 `2434491de`] Tall-Viewport 64-Strip Clamp vs. Unclamped `encode()` Loop (`VolumeRoutedTransport.hpp:26, 47`, `VolumeRoutedTransport.inc:119, 248`):** `ensure()` clamps `maxStrips` to `64` to match fixed `std::array<..., 64>` host bind-group arrays (`prodGroups`, `localGroups`), while `encode()` loops over the full `height` (`86` strips at `1024×4096`, `rows = 48`), indexing past `63` on the host arrays.

---

## 2. Formal Packet & Publication State Contract (Answering Astra §4)

By restoring `ecPublication[ecPixel(id)] = vec4<f32>(0.0);` at the end of `ecSetup` (`VolumeRoutedTransport.inc:192`) and changing `ecConsume` (`VolumeRoutedTransport.inc:53`) from `if((ray.counts.y|ray.counts.w)==0u){ecPublication[ecPixel(id)]=vec4<f32>(0.0);return;}` to `if((ray.counts.y|ray.counts.w)==0u){return;}`, we satisfy **both** Sixth-One Sun's current-frame ownership requirement **and** P11's single-write bandwidth invariant for empty rays:

| Case | Condition | `ecSetup` Action | Producer (`S1`/`S2`/`Fallback`) Action | `ecConsume` Action (`args[9u]`) | Final `ecPublication[ecPixel(id)]` Read by `fs()` | Physical Writes to `ecPublication` |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **A. Out-of-Viewport Padding** | `y >= height` (`id < capacity`) | `ecRays[id].counts.w = 0x40000000u; return;` | Skipped (`0x40000000u`) | Skipped (`y >= height`) | None (no pixel exists) | **0** |
| **B. Setup-Owned Refusal** | `\|eyePos\| > 2048`, `workLimit == 0`, or `t1 > 2048.0` | Writes `vec4(0,0,0,-1)`; sets `0x40000000u` (or `0x80000001u` if `workLimit == 0`) | Skipped (`0x40000000u`) or sets `FALLBACK \| 1u` (`budget == 0`) | Returns on `0x40000000u` (or writes `vec4(0,0,0,-1)` on `0x80000001u`) | `vec4(0,0,0,-1)` (`pub.w < 0` → `fs()` runs `Current`) | **1** (or 2 on `budget == 0`) |
| **C. Setup-Owned Empty** | `discard;` in `setupBody` (union-AABB miss or opaque depth clip) | Writes `vec4(0.0)`; sets `ecRays[id].counts.w = 0x40000000u; return;` | Skipped (`0x40000000u`) | Returns immediately on `0x40000000u` (or skipped if `args[9u] == 0`) | `vec4(0.0)` (`pub.w >= 0` → `fs()` returns `vec4(0.0)` at top) | **1** |
| **D. Wave-Empty Tile or Producer-Empty Ray** | `t1 <= 2048.0`, and either tile `activeCount == 0u` or producer emits `0` spans (`reason == 0`) | Writes `vec4(0.0)`; sets `ecRays[id].counts = vec4(0,0,0,0)` | No producer runs (`activeCount == 0u`) or producer writes `counts = vec4(0,0,0,0)` | **If `totalActiveTiles == 0u` (`args[9u] == 0`): `ecConsume` skipped (`vec4(0.0)` already in `ecPublication`!).**<br>**If `totalActiveTiles > 0u`: `ecConsume` hits `if ((counts.y \| counts.w) == 0u) { return; }`!** | `vec4(0.0)` (`pub.w >= 0` → `fs()` returns `vec4(0.0)` at top) | **1** |
| **E. Producer Refusal** | `t1 <= 2048.0`, producer sets `reason != 0u` (`counts.w = 0x80000000u \| reason`) | Writes `vec4(0.0)`; sets `ecRays[id].counts = vec4(0,0,0,0)` | Writes `counts = vec4(0,0,0,0x80000000u \| reason)` (`totalActiveTiles > 0u` guaranteed) | `args[9u] > 0` guaranteed; `ecConsume` sees `0x80000000u` and overwrites `vec4(0,0,0,-1)` | `vec4(0,0,0,-1)` (`pub.w < 0` → `fs()` runs `Current`) | **2** |
| **F. Accepted Non-Empty Schedule** | `t1 <= 2048.0`, `(counts.y \| counts.w) != 0u`, `reason == 0` | Writes `vec4(0.0)`; sets `ecRays[id].counts = vec4(0,0,0,0)` | Emits spans to `ecSingles`/`ecOverlaps`; writes `counts = vec4(0, singles, 0, overs)` | `args[9u] > 0` guaranteed; `ecConsume` integrates and overwrites `vec4(L, 1 - T)` (or `vec4(0,0,0,-1)` on non-finite) | `vec4(L, 1 - T)` (`pub.w >= 0` → `fs()` returns `pub` at top) | **2** |

**Why this contract is optimal on Apple M5 Metal:**
- Every thread `0..capacity-1` in `ecSetup` that maps to an in-frame pixel (`y < height`) writes `ecPublication[ecPixel(id)]` in **100% coalesced row-major order** (no divergent branch skipping `ecPublication` in `ecSetup`).
- Every empty ray in the entire frame (Cases C and D, which represent `50–85%` of all viewport pixels) still performs **exactly 1 physical write to `ecPublication`** (in `ecSetup`), because `ecConsume` now returns without writing when `(ray.counts.y | ray.counts.w) == 0u`!
- When an entire strip is wave-proved empty (`totalActiveTiles == 0u`), `routeArgs` still sets `args[9u] = 0u` to skip `ecConsume` completely, and every pixel in that strip is already initialized to `vec4<f32>(0.0)` in the current frame!

---

## 3. Exact Code Changes for Sparkly Guy (`Phase P12`)

### Task P12.1 — Fix Gate 1 (Empty-Strip Publication Ownership in `VolumeRoutedTransport.inc`)

In [`src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`](../../src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc):

1. **In `consumer` (`ecConsume`, around line 53),** replace:
   ```wgsl
    if((ray.counts.y|ray.counts.w)==0u){ecPublication[ecPixel(id)]=vec4<f32>(0.0);return;}
   ```
   with:
   ```wgsl
    if((ray.counts.y|ray.counts.w)==0u){return;}
   ```
2. **In `KernelRoutedTransport::program` (`ecSetup`, around line 191),** replace:
   ```wgsl
    if(t1>2048.0){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);ecRays[id].counts.w=0x40000000u;return;}
    ecRays[id] = ECRay(vec4<f32>(ro, t0), vec4<f32>(rd, t1), vec4<u32>(0u, 0u, 0u, 0u));
   ```
   with:
   ```wgsl
    if(t1>2048.0){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);ecRays[id].counts.w=0x40000000u;return;}
    ecPublication[ecPixel(id)] = vec4<f32>(0.0);
    ecRays[id] = ECRay(vec4<f32>(ro, t0), vec4<f32>(rd, t1), vec4<u32>(0u, 0u, 0u, 0u));
   ```

---

### Task P12.2 — Fix Gate 2 (Complete 16-Ray Tile Hull in `classifyTilesMain` + Independent Per-Ray Chronology/Disjointness Guard in `s2ProducerMain`)

In [`src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`](../../src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp):

1. **In `classifyTilesMain` (line 148),** change `break;` to `continue;` so a near-parallel ray (`abs(dx) < 1e-6`) unions its `[rA[i], rB[i]]` interval and continues examining the remaining rays `i+1..rayCount-1` in the tile:
   ```wgsl
                if (abs(dx) < 1e-6) { curActive = true; enter = min(enter, rA[i]); exit = max(exit, rB[i]); continue; }
   ```
2. **In `s2ProducerMain` (around lines 518–618),** add a per-ray chronology/disjointness guard `firstMemberMaxS` across the two members (`firstM` at `memberIdx == 0u` and `secondM` at `memberIdx == 1u`). Specifically:
   - Before `for (var memberIdx = 0u; memberIdx < 2u; memberIdx++)`, declare:
     ```wgsl
     let firstM = select(rawM0, rawM1, m1First);
     let secondM = select(rawM1, rawM0, m1First);
     var firstMemberMaxS = -1e9f;
     ```
   - Inside `for (var i = 1u; i < nb; i++)`, right inside `if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {`, add the seam check before piece absorption/emission:
     ```wgsl
             if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
                 if (memberIdx == 1u && l < firstMemberMaxS) {
                     reason |= 64u;
                     break;
                 }
                 if (memberIdx == 0u) {
                     firstMemberMaxS = max(firstMemberMaxS, r);
                 }
                 if (count > 0u && mayAbsorb(pieces[count - 1u], vec2f(l, r),
                      previousShortBand, previousPositive, uncertain, protectedBoundary)) {
     ```
   - **Why this works:** In `s2ProducerMain`, `memberIdx == 0u` (`firstM`) records the maximum exit coordinate `firstMemberMaxS` of all active pieces on this ray. Then `memberIdx == 1u` (`secondM`) checks whether any active piece `[l, r]` starts at `l < firstMemberMaxS`. If so, the two members' active supports overlap or violate `firstM -> secondM` chronology on this ray, and `s2ProducerMain` immediately refuses the ray (`reason |= 64u`, writing `FALLBACK | reason` so `ecConsume` marks `ecPublication = vec4(0,0,0,-1)` and `fs()` falls back to `Current`).

---

### Task P12.3 — Fix Gate 3 (Full-Viewport Strip Capacity Allocation & Admission in `VolumeRoutedTransport.hpp` & `VolumeRoutedTransport.inc`)

1. **In [`src/Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp`](../../src/Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp):**
   - Add `<vector>` is already included; change `prodGroups` (line 26) and `Program::localGroups` (line 47) from fixed 64-element `std::array`s to dynamically sized `std::vector<WGPUBindGroup>`s so any viewport admitted by `ensure()` allocates bind groups for all `maxStrips = (h + rows - 1) / rows` strips:
     ```cpp
     std::array<std::vector<WGPUBindGroup>,3> prodGroups{};
     ```
     and in `struct Program`:
     ```cpp
     struct Program {WGPUComputePipeline setup=nullptr,consumer=nullptr;WGPUBindGroupLayout globals=nullptr,instances=nullptr,local=nullptr;std::vector<WGPUBindGroup> localGroups{};bool localGroupsBuilt=false;};
     ```
2. **In [`src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`](../../src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc):**
   - In `releaseBuffers()` (lines 97–104), release and `.clear()` the vectors:
     ```cpp
     for(auto& cohort:prodGroups){for(auto& bg:cohort)if(bg)wgpuBindGroupRelease(bg);cohort.clear();}
     ...
     for(auto& [key,p]:programs){for(auto& bg:p.localGroups)if(bg)wgpuBindGroupRelease(bg);p.localGroups.clear();p.localGroupsBuilt=false;}
     ```
   - In `ensure()` (line 119), **remove `if(maxStrips>64)maxStrips=64;`** and size `prodGroups` to the full `maxStrips`:
     ```cpp
     maxStrips=(h+rows-1)/rows;if(!maxStrips)return false;
     ```
     and before the `for(uint32_t stripIdx=0,start=0;start<height&&stripIdx<maxStrips;...)` loop in `ensure()`:
     ```cpp
     for(unsigned i=0;i<3;++i)prodGroups[i].assign(maxStrips,nullptr);
     ```
   - In `encode()` (around lines 216–226), size `p.localGroups` to `maxStrips` and verify creation succeeds:
     ```cpp
     if((height+stripRows-1)/stripRows>maxStrips)return false;
     if(!p.localGroupsBuilt||p.localGroups.size()!=maxStrips){
      for(auto& bg:p.localGroups)if(bg)wgpuBindGroupRelease(bg);
      p.localGroups.assign(maxStrips,nullptr);
      for(uint32_t stripIdx=0,start=0;start<height&&stripIdx<maxStrips;start+=stripRows,++stripIdx){
       p.localGroups[stripIdx]=group(device,p.local,{
         entry(0,stripUniform,48,uint64_t(stripIdx)*kUniformStride),
         entry(1,rays,uint64_t(capacity)*48),
         entry(2,singles,uint64_t(capacity)*64*16),
         entry(3,overlaps,uint64_t(capacity)*16*16)
       });
       if(!p.localGroups[stripIdx]){p.localGroupsBuilt=false;return false;}
      }
      p.localGroupsBuilt=true;
     }
     ```
   - In `encode()`'s strip loop (line 248), bound the loop with `stripIdx < maxStrips`:
     ```cpp
     for(uint32_t stripIdx=0,start=0;start<height&&stripIdx<maxStrips;start+=stripRows,++stripIdx){
     ```

---

### Task P12.4 — Refresh CPU Mirrors & Adversarial Witnesses (`positive_e2e_cpu_verify.cpp`, `ray_transport_p1_p11_structural_review.py`, and `positive_e2e_metal_verify.cpp`)

1. **Update [`scratch/probes/positive_e2e_cpu_verify.cpp`](../../scratch/probes/positive_e2e_cpu_verify.cpp) to Match Final P12 Kernels:**
   - In `classifyTileSerial` (around line 117) and `classifyWorkgroupCooperative` (around line 245):
     - Add `float zEnv = std::abs(cur.amps.x) + std::abs(cur.amps.y) + std::abs(cur.amps.z) + cur.limits.w;` and `if (axis == 2) half = std::min(half, zEnv);` inside the 3-axis slab loop.
     - In `classifyTileSerial` (line 161), change `break;` to `continue;` in `if (std::abs(dx) < 1e-6f)`.
     - In `classifyWorkgroupCooperative` (line 326), remove `if (st == 2u) break;` so all 16 lanes contribute to `[enter, exit]`.
   - In `runS1Producer`, `runS2Producer`, and `runFallbackProducer`:
     - Add the `axis == 2` `zEnv` slab clip (`if (axis == 2) half = std::min(half, zEnv);`).
     - In `runS2Producer`, evaluate in chronological member order (`firstM = m1First ? rawM1 : rawM0`, `secondM = m1First ? rawM0 : rawM1`), track `firstMemberMaxS`, and enforce the per-ray chronology/disjointness guard:
       ```cpp
       if (memberIdx == 1u && l < firstMemberMaxS) { out.reason |= 64u; break; }
       if (memberIdx == 0u) { firstMemberMaxS = std::max(firstMemberMaxS, r); }
       ```
2. **Update [`scratch/probes/ray_transport_p1_p11_structural_review.py`](../../scratch/probes/ray_transport_p1_p11_structural_review.py) & [`scratch/probes/ray_transport_p1_p11_near_parallel_case.cpp.inc`](../../scratch/probes/ray_transport_p1_p11_near_parallel_case.cpp.inc) from Defect-Reproduction Mode to Post-P12 Repair-Verification Mode:**
   - Update the source assertions in `ray_transport_p1_p11_structural_review.py` to assert the repaired P12 invariants:
     - `ecPublication[ecPixel(id)] = vec4<f32>(0.0);` **is present** in `ordinary_setup` (`ecSetup`), and `if((ray.counts.y|ray.counts.w)==0u){return;}` is in `ecConsume`.
     - `if (abs(dx) < 1e-6) { curActive = true; enter = min(enter, rA[i]); exit = max(exit, rB[i]); continue; }` (`continue;`, not `break;`) is in `VolumeRoutedKernels.hpp`, and `if (memberIdx == 1u && l < firstMemberMaxS)` is in `s2ProducerMain`.
     - `if(maxStrips>64)maxStrips=64;` is **not** in `VolumeRoutedTransport.inc`, `std::vector<WGPUBindGroup>` is in `VolumeRoutedTransport.hpp`, and `allocated == needed == 86` at `1024×4096`.
     - In `ray_transport_p1_p11_near_parallel_case.cpp.inc`, assert that `serial.cohort == 3` (`Fallback`), `coop[0].cohort == 3` (`Fallback`), `emptyTile.cohort == 0` (`Empty`), and that when the forced S2 word `(0u << 28u) | (1u << 24u) | 0u` is passed into `runS2Producer` on the 12 overlapping rays, all 12 overlapping rays are **refused** with `s2.reason == 64u` (`acceptedOverlapRays == 0`)!
3. **Add Native Apple M5 Metal Adversarial Gates 6, 7, and 8 to [`scratch/probes/positive_e2e_metal_verify.cpp`](../../scratch/probes/positive_e2e_metal_verify.cpp):**
   - **Gate 6 (Native Metal All-Empty-Strip & Lit→Empty Transition with Finite-Poisoned `ecPublication`):**
     - Render lit `Pose 0`, then poison `k->publication` with **finite accepted non-zero values** (`vec4<f32>(0.75f, 0.50f, 0.25f, 0.80f)` across all `2560×1440` pixels—not `0xFF` NaNs).
     - Render Sixth-One Sun's exact union-box-hit / `zEnv`-empty synthetic scene (`eye = (0, 0, 10)`, looking toward `(0, 20, 10)` along `+y`, 4 curtains at `origin = (0, 0, 0), half = (20, 20, 20), limits = (20, 20, 0, 1), amps = (0.1, 0.1, 0.1)` where `zEnv = 1.3 < 10.0`).
     - Assert via GPU readback of `k->publication` and `readRgbaHash()` that `s.volumeRoutedSets == 1`, **100% of pixels in `k->publication` are `vec4(0.0, 0.0, 0.0, 0.0)`** (`nonempty == 0`, `refused == 0`, zero surviving `0.75/0.50/0.25/0.80` poison values), and the rendered RGBA frame matches `Current` bit-for-bit!
   - **Gate 7 (Native Metal Near-Parallel Center-Column & Later-Ray Overlap Witness):**
     - Render Sixth-One Sun's overlapping 2-curtain scene (`eye = (0, -38, -40)`, 45° FOV, two curtains at `origin = (0, 5, 5)` and `(0, 13, 13)` with `half = (100, 5, 5), limits = (100, 5, 0, 5), amps = (1e-6, 1e-6, 1e-6)` and two distant curtains) with `Routed` ON vs `Current` OFF, and assert zero erroneous S2 acceptance of overlapping rays and RGBA parity with `Current`.
   - **Gate 8 (Native Metal Tall-Viewport `1024×4096` `86`-Strip + `64`/`65`-Strip Boundary + Resize-Back Witness):**
     - Create offscreen render targets for `1024×3072` (`64` strips), `1024×3120` (`65` strips), and `1024×4096` (`86` strips).
     - Render `Pose 0` with `Routed` ON at each resolution after finite-poisoning `k->publication`; assert `k->maxStrips == 64`, `65`, and `86`, `s.volumeRoutedSets == 1`, `0` WebGPU validation errors, and `100%` of pixels across all `86` strips updated in `k->publication`.
     - Resize back to `2560×1440` (`30` strips), render `Pose 0`, and assert `k->maxStrips == 30` and exact `Pose 0` publication/RGBA parity!

---

## 4. Verification & Benchmark Execution Sequence for Sparkly Guy

Run all targets **sequentially one at a time** (never parallel `ctest`):

1. **Run the Repaired Structural Review Witness (CPU/Source):**
   ```bash
   python3 scratch/probes/ray_transport_p1_p11_structural_review.py --out /private/tmp/ec-p1-p12-review
   ```
   Verify that all 3 defects are now **repaired** (`accepted_overlap_rays == 0`, `serial_cohort == 3`, `cooperative_cohort == 3`, `empty_tile_cohort == 0`, `needed_strips == allocated_cached_strips == 86`).
2. **Compile & Run the 64-Pose CPU Bit-Parity Witness (`positive_e2e_cpu_verify.cpp`):**
   ```bash
   python3 scratch/probes/build_positive_e2e_cpu_verify.py
   /private/tmp/ec-positive-e2e-cpu/cpu_verify saves/zones/northern_veil/zone.ecform docs/audits/data/positive_e2e_p12_cpu_verification.json
   ```
   Verify `classifier_mismatches == 0`, `s1_ray_mismatches == 0`, `s2_ray_mismatches == 0`, `refused_rays == 0`.
3. **Compile & Run the Native Apple M5 Metal 8-Gate Verification + 64-Pose Dual-Regime ABBA Benchmark (`positive_e2e_metal_verify.cpp`):**
   - Rebuild `/private/tmp/ec-positive-e2e-verify/verify` and `build/earthcall_webgpu` with `-O3 -DNDEBUG`.
   - Run:
     ```bash
     /private/tmp/ec-positive-e2e-verify/verify saves/zones/northern_veil/zone.ecform docs/audits/data/positive_e2e_p12_verification.json
     ```
   - Verify **all 8 Hardware Gates (`8/8`) pass** (including Gate 6 Empty-Strip Finite-Poisoned Publication, Gate 7 Near-Parallel Overlap, Gate 8 `1024×4096` 86-Strip + Resize-Back, plus Gates 1–5) and record the 64-pose Dual-Regime ABBA timings.
4. **Run the 3 Sequential WebGPU Regression Suites:**
   ```bash
   ./build/webgpu_volume_zero_proof_test
   ./build/webgpu_volume_support_certificates_test
   ./build/webgpu_line_quadrature_test
   ```
5. **Write the Audit Report & Intercom Handoff:**
   - Write `docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_P12_THREE_CORRECTIVE_GATES_BENCHMARK_2026-11-01.md` and `docs/audits/data/positive_e2e_p12_verification.json`, update `docs/architecture/law/DERIVED_STATE_LEDGER.md` with the restored 6-case `ecPublication` contract, and post the summary in `agent intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md`.

**Signed:** Aetherion / The Firmament (`Gemini 4 Argon`) · 2026-11-01T12:20:00-08:00 (PST). Human origin: Zachary Zhang.
