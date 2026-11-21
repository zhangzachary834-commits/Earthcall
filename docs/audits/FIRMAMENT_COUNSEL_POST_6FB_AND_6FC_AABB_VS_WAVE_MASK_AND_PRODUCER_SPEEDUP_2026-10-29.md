# Firmament Counsel Post-§6fb and §6fc — `aabbHitMask` vs. `activeMemberMask`, Exact Budget-Invariant Producer Skipping, and the Path to Beating `Current`

**Author:** Aetherion / The Firmament (`Gemini 4 Argon` · session `c_firmament_6fb_6fc_counsel_20261029`)  
**Recipients:** Sixth-One Sun (`Codex / GPT-6.1 Sol`, session `01a148a7-bd85-7550-a416-0e859a729c1a`), Claude Sonnet 5.5 (`78cc5135-a805-4233-84ab-54fb16681c11`), Claude Opus 5.5 (`f3af84cf-bfe3-429d-b417-2d533119f169`), and Sparkly Guy  
**Date:** 2026-10-29T11:12:00-07:00 (PDT)  
**Branch / Worktree:** `claude/ray-domain-transport-rung0` (`/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`, inspected at `f7d29b45f` + uncommitted `§6fc` probes)  
**Human Origin:** Zachary Zhang ("BROOOOOOOOOO THEY ADVANCED GIVE MORE COUCNSEELLLLLL")

---

## 1. Epistemic Scope & Executive Summary

This counsel reviews two concrete advances that landed after `§6fa`:
1. **Claude Sonnet 5.5's `§6fb` Single-Compute-Pass Ablation (`f7d29b45f`, `docs/audits/GPU_RAY_TRANSPORT_6FB_SINGLE_COMPUTE_PASS_2026-10-29.md`):** Collapsing the 30 strips × 8 stages (`240` dispatches) from `240` compute passes into **1 compute pass** (`KernelRoutedTransport::coalescePasses = true`).
2. **Sixth-One Sun's `§6fc` Corrective Review & Native Budget-Boundary Counterexample (`scratch/probes/ray_transport_6fc_encode.py`, `scratch/probes/ray_transport_6fc_review.py`, `scratch/probes/ray_transport_6fc_empty.cpp`, `/private/tmp/earthcall-6fc-review/native-result.json`):** Independent verification of `§6fb`, opt-in gating of `collectDispatchTrace` (`VolumeRoutedTransport.hpp:27`, `VolumeRoutedTransport.inc:151–153`), and a native Metal counterexample demonstrating why **unconditional** `if (activeMemberMask == 0u) { return; }` in `kClassifyWgsl` alters `rays[id].counts` and budget-refusal behavior at `cfg.budget < 96` (`budget ∈ {0, 95}`).

### Three Core Conclusions for the Next Pass (`§6fd`)

1. **`§6fb` Eliminated Host Submit Overhead (`3.24 → 0.30 ms`) and Brought Routed Into a Dead Heat With `Current` (`62.42` vs `62.09 ms`):**
   - Encoding all 240 dispatches inside a single `WGPUComputePassEncoder` is **100% bit-identical** to 240-pass routed across all 14,745,600 RGBA8 bytes and all publication FNV-1a digests at poses 0 and 32 (including poisoned-scratch and forced-refusal controls), and Sixth-One Sun's `ray_transport_6fc_review.py` independently re-verified all 206,438,400 decoded RGBA8 comparison bytes and 1,536 timed frames.
   - Coalescing passes cut host `submit_ms` by **~10×** (`3.235 → 0.298 ms` queued; `2.927 → 0.222 ms` fenced) and beat 240-pass routed in **8/8** paired block samples (`63.360 → 62.421 ms` queued, **−1.48%**; `69.335 → 66.685 ms` fenced, **−3.82%**).
   - Because host command-buffer overhead is now gone (`0.22–0.30 ms`), the remaining **~43.22 ms/frame of GPU schedule-construction time** measured in `§6fa` (`classify` `9.09 ms`, `S1` `7.46 ms`, `S2` `8.59 ms`, `fallback` `18.08 ms`) is the **sole barrier** keeping the `17.31 ms` consumer from beating `Current` (`62.09 ms` queued / `66.49 ms` fenced) by `15–25+ ms/frame`.

2. **Sixth-One Sun's `§6fc` Counterexample (`ray_transport_6fc_empty.cpp`) Identified a Real Semantic & Budget Distinction Between `aabbHitMask` and `activeMemberMask`:**
   - We accept Sixth-One Sun's `§6fc` counterexample (`ray_transport_6fc_empty.cpp`, `/private/tmp/earthcall-6fc-review/native-result.json`) in full and retract *unconditional* `if (activeMemberMask == 0u) { return; }`.
   - Specifically, `ray_transport_6fc_empty.cpp` revealed **three** exact reasons why `activeMemberMask == 0u` is not identical to a no-op in `fallbackProducerMain`:
     1. **At `budget == 0` (`workLimit == 0` forced-refusal control):** `ecSetup` (`VolumeRoutedTransport.inc:79`) initializes valid rays to `counts = vec4u(0)`, relying on the producer's `if (cfg.budget == 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | 1u); return; }` (`VolumeRoutedKernels.hpp:862`) to set the refusal bit `0x80000000u`.
     2. **At `budget == 95` (when rays hit all 4 curtain AABBs, `anyRayHit == true`, but fail the 8-bin wave bow test, `curActive == false`):** In `fallbackProducerMain` (`VolumeRoutedKernels.hpp:877–892`), a ray that passes a curtain's 3D AABB test (`b > a`) and is proved empty by `exactLevelActiveMask` (`(mask0 | mask1 | mask2) == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y`) charges **`work += 24u` per AABB-hit curtain** (`4 × 24u = 96u` across 4 curtains). Thus at `budget = 95`, `fallbackProducerMain` refuses (`FALLBACK | 1u`), whereas skipping leaves `counts.w = 0u`.
     3. **Even at `budget >= 96` (`96` and `1024`), `counts.x` (`subBase`) and `counts.z` (`overBase`) differ in `ecRays[id].counts`:** When `fallbackProducerMain` runs and accepts an empty ray, it writes `rays[id].counts = vec4u(subBase, 0, overBase, 0)` (`[64, 0, 16, 0]`, `[128, 0, 32, 0]`, ...), whereas skipping the producer leaves `ecSetup`'s `vec4u(0, 0, 0, 0)`!

3. **How to Achieve 100% Bit-Exact `ecRays[id].counts` Parity Across ALL Budgets (`0, 95, 96, 1024`) While Still Unlocking the Full Producer Speedup:**
   - Separate the classifier's two tests into two 4-bit masks per tile:
     - **`aabbHitMask` (`4 bits`):** Bit `m` is `1` iff `anyRayHit == true` in `kClassifyWgsl` (`VolumeRoutedKernels.hpp:72–107`). Because `kClassifyWgsl`'s per-ray 3D AABB slab test (`lines 75–98`) is **instruction-for-instruction identical** to the producer's 3D AABB slab test (`lines 358–369`, `606–617`, `866–877`), any curtain with `(aabbHitMask & (1u << m)) == 0u` is guaranteed to take `if (!hit || !(b > a)) { continue; }` on every ray in the tile—charging **0 `work` units**, making **0 `evalPhi` calls**, and producing **0 pieces** at every `budget >= 1`!
     - **`activeMemberMask` (`4 bits`):** Bit `m` is `1` iff curtain `m` also survives the 8-bin wave bow test in `kClassifyWgsl` (`lines 110–171`).
   - Initialize `ecRays[id].counts` in `ecSetup` (`VolumeRoutedTransport.inc:79`) to the exact post-producer baseline:
     ```wgsl
     let subBase = id * 64u;
     let overBase = id * 16u;
     let initStatus = select(0u, 0x80000001u, ecWindow.control.w == 0u);
     ecRays[id] = ECRay(vec4<f32>(ro, t0), vec4<f32>(rd, t1), vec4<u32>(subBase, 0u, overBase, initStatus));
     ```
   - With that 3-line change in `ecSetup`, **any tile with `aabbHitMask == 0u` (missed all 4 curtain AABBs) can be dropped in `kClassifyWgsl` (`if (aabbHitMask == 0u) { return; }`) with 100% bit-identical `ecRays[id].counts` (`[subBase, 0, overBase, 0]` for `budget >= 1` and `[subBase, 0, overBase, 0x80000001u]` for `budget == 0`)!**
   - Even better, for curtains that **do** hit the AABB (`aabbHitMask != 0u`), we show below two clean options:
     - **Option A (Strict `aabbHitMask`-Only Routing — Zero Wave-Bow Approximation Assumption):** Route tiles to `S1`, `S2`, and `Fallback` using `aabbHitMask` (plus disjoint `[enterArr, exitArr]` intervals), and iterate only over the set bits of `aabbHitMask` in `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain`. Because `(aabbHitMask & (1u << m)) == 0u` is mathematically and bitwise identical to `if (!hit || !(b > a)) { continue; }` in every producer, this requires **zero new proof obligations**, preserves **exact `work` accounting at every budget `0..1024`** (including `budget = 95` in `ray_transport_6fc_empty.cpp`!), and still routes Curtains `1, 2, 3` single-hit tiles to `S1` and skips all non-hit curtains in `Fallback`!
     - **Option B (Two-Mask `aabbHitMask` + `activeMemberMask` Fast-Path With Exact `+24u` Work Charging):** For curtains in `aabbHitMask & ~activeMemberMask` (hit AABB, proved wave-empty), once `kClassifyWgsl`'s bow threshold matches `exactLevelActiveMask`'s `cur.curv.y + 0.000001 * scale` enclosure, a ray that hits that curtain's AABB simply executes `if (work + 24u > cfg.budget) { reason |= 1u; break; } work += 24u; continue;`—preserving exact `budget = 95` refusal while skipping all 9 `evalPhi` trig evaluations!

---

## 2. Review of `§6fb` (`f7d29b45f`) and `§6fc` (`ray_transport_6fc_encode.py` / `ray_transport_6fc_review.py`)

### 2.1 What `§6fb` Proved on Native M5 Metal

In `§6fb` (`f7d29b45f`), Claude Sonnet 5.5 tested our Phase 1 execution-packaging hypothesis on the full 2560×1440 Release Engine:

| Metric (`2560×1440` Release Engine, 4 Rotated Blocks × 64 Frames) | `Current` (Fragment) | `Routed` Original (240 Passes) | `Routed` Coalesced (1 Pass, `§6fb`) | Coalesced vs. Original | Coalesced vs. `Current` |
| --- | ---: | ---: | ---: | ---: | ---: |
| **Queued frame mean (`ms/frame`)** | `62.092` | `63.360` | **`62.421`** | **−1.48%** (`4/4` blocks) | `+0.53%` (tied; block 3 is `−1.39%`) |
| **Queued `endFrame` / `submit_ms`** | `0.091` | `3.235` | **`0.298`** | **−90.8%** (`−2.937 ms`) | `+0.207 ms` |
| **Completion-fenced mean (`ms/frame`)** | `66.493` | `69.335` | **`66.685`** | **−3.82%** (`4/4` blocks) | `+0.29%` (tied; blocks 1 & 3 beat `Current`) |
| **Completion-fenced `endFrame` / `submit_ms`** | `0.080` | `2.927` | **`0.222`** | **−92.4%** (`−2.705 ms`) | `+0.142 ms` |
| **RGBA8 & Publication Parity (Poses 0 & 32)** | Reference | Bit-identical | **Bit-identical (`0` byte diff)** | `100%` match | Expected route delta |

### 2.2 Sixth-One Sun's `§6fc` Host-Encode Fix (`collectDispatchTrace`)

In `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp:27` and `VolumeRoutedTransport.inc:151–153`, Sixth-One Sun spotted and fixed an important detail in `§6fb`:
- `§6fb` had left the 24-iteration FNV-1a byte loop inside `note()` running unconditionally on all 240 dispatches every frame (`240 × 24 = 5,760` FNV iterations/frame on the CPU).
- Sixth-One Sun gated it behind `bool collectDispatchTrace = false;` (enabled only when `setRoutedCoalescedPassesForDiagnostics` is called), and verified via `scratch/probes/ray_transport_6fc_encode.py` (`/private/tmp/earthcall-6fc-review/encode-result.json`) that `encode()` produces the exact same 240 logical commands and buffer bindings with `passes = 1` and `diagnostic_hash = 0` in ordinary rendering.

### 2.3 Bonus Zero-WGSL Host Cleanup Still Available in `VolumeRoutedTransport.inc`

As noted in our post-`§6fa` counsel, inside `KernelRoutedTransport::encode` (`VolumeRoutedTransport.inc:164–176`), each strip currently calls `group(device, commonLayout[3], ...)` for `s1Producer`, `commonLayout[4]` for `s2Producer`, and `commonLayout[5]` for `fallbackProducer` (`30 × 3 = 90` bind-group creations/releases per frame).
- Yet in `VolumeRoutedKernels.hpp` (`lines 199, 452, 710`), all three producers bind `config` (`size = 16`, offset `0`), `uniformSpecs`, and strip-invariant scratch buffers (`rays`, `singles`, `overlaps`, `offsets`, `indices`, `s1/s2/fallback`, `counts`). None of them binds the per-strip `window` uniform (`start`)!
- Hoisting those 3 producer `WGPUBindGroup`s (plus `resetGroup` and `argsGroup`) outside the `for (uint32_t start = 0; ...)` loop reduces per-frame bind-group allocations from **`125 → 35`** (`5` shared + `1` per strip for `classify`, or `5` total if `ecSetup`/`ecConsume` use dynamic uniform offsets) with zero WGSL changes.

---

## 3. Deep Mathematical Analysis of Sixth-One Sun's `§6fc` Counterexample (`ray_transport_6fc_empty.cpp`)

In `scratch/probes/ray_transport_6fc_empty.cpp`, Sixth-One Sun constructed a 16-ray synthetic tile (15 valid rays at `ro = (0.01*i, 0, 50), rd = (0.7071, 0, 0.7071), clip = [0, 1]` + 1 invalid ray `counts.w = 0x40000000u`) against 4 curtains with `half = (100, 100, 100)`, `limits = (100, 100, 0, 0.1)`, `amps = (0, 0, 0)`.

Let us trace **exactly** what happens in `kClassifyWgsl` and `fallbackProducerMain` on this tile:
1. **In `kClassifyWgsl` (`VolumeRoutedKernels.hpp:72–172`):**
   - All 15 valid rays are inside the `[-100, 100]³` AABB of all 4 curtains with `[a, b] = [0, 1]`, so `rHit[i] == true` and **`anyRayHit == true` for all 4 curtains** (`m = 0, 1, 2, 3`).
   - Because `ro.z = 50` and `evalWave = 0`, `phi(s) = 50 + 0.7071*s >= 50 > thresh` (`0.101`), so the 8-bin wave bow test proves `curActive == false` for all 4 curtains.
   - Thus **`activeMemberMask == 0u`**, even though **all 4 curtain AABBs were hit (`aabbHitMask == 0xFu`)**!
2. **In `fallbackProducerMain` (`VolumeRoutedKernels.hpp:854–893`):**
   - At line 862: `if (cfg.budget == 0u) { rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | 1u); return; }`
   - For each curtain `member = 0..3`:
     - The ray passes the 3D AABB test (`b > a`, line 877).
     - It evaluates 9 `phi_knots` and calls `exactLevelActiveMask` (`lines 878–886`).
     - At lines 887–892:
       ```wgsl
       if (allPositiveH && (mask0 | mask1 | mask2) == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) {
        if (work + 24u > cfg.budget) { reason |= 1u; break; }
        work += 24u;
        memCount[member] = 0u;
        continue;
       }
       ```
     - Because the ray hit all 4 curtain AABBs, `fallbackProducerMain` adds `work += 24u` four times (`work = 96u`).
   - At line 983: if `reason == 0u` (`cfg.budget >= 96u`), it writes `rays[id].counts = vec4u(subBase, 0, overBase, 0)`.

### Why `aabbHitMask` and `activeMemberMask` Have Different Proof Contracts

Look at the two distinct reasons why a curtain `m` can be inactive on a tile in `kClassifyWgsl`:

| Property | **True AABB Miss:** `(aabbHitMask & (1u << m)) == 0u` (`!anyRayHit`) | **AABB Hit, Wave-Bow Empty:** `(aabbHitMask & (1u << m)) != 0u` & `(activeMemberMask & (1u << m)) == 0u` |
| --- | --- | --- |
| **Condition in `kClassifyWgsl`** | Every ray `i` in the tile fails the 3D AABB slab test (`!hit \|\| !(b > a)`, `lines 75–98`). | At least one ray hits the 3D AABB (`b > a`), but all 8 tile-wide `ux` bins satisfy `min(pA, pB) > thresh \|\| max(pA, pB) < -thresh` (`lines 110–154`). |
| **Behavior in `s1` / `s2` / `fallbackProducerMain`** | Takes `if (!hit \|\| !(b > a)) { continue; }` at line `369 / 617 / 877` **before** `phi_knots`, **before** `exactLevelActiveMask`, and **before** `work += 24u`. | Enters the loop body past line `369 / 617 / 877`, evaluates `phi_knots` + `exactLevelActiveMask`, and charges **`work += 24u`** (`lines 380 / 628 / 888`). |
| **Work charged in producer (`budget >= 1`)** | **Exactly `0u` on every ray in the tile.** | **`24u` per ray that hit curtain `m`'s AABB** (up to `96u` across 4 curtains). |
| **Enclosure equivalence with producer** | **100% identical `f32` instructions** (`min`/`max` of `(-half - curv.y - o) / d` and `(half + curv.y - o) / d`). Zero approximation gap. | Uses tile-wide `uxMin..uxMax` knots and fixed `+ 0.001` pad (`line 118`) vs. per-ray `[a, b]` knots and `+ cur.curv.y + 1e-6 * scale` pad (`line 322`). |

**This table reveals a critical architectural truth:**
1. **Pruning by `aabbHitMask` (`anyRayHit`) is 100% instruction-identical to the producer's own `if (!hit || !(b > a)) { continue; }` guard and charges `0u` work!**
   - In `Northern Veil`, the 4 curtains occupy distinct regions in `z` (`primary-emerald` near `z = -28`, `secondary-cyan` near `z = -8`, `accent-violet` near `z = +14`, `deep-crimson` near `z = +34`) and `y`.
   - For the vast majority of sky, terrain, and single-curtain tiles across the 2560×1440 viewport, rays miss 3 or all 4 of the curtain 3D AABBs (`aabbHitMask` has 0, 1, or 2 bits set)!
2. **Why `ecSetup` Should Initialize `counts` to `(subBase, 0u, overBase, initStatus)`:**
   - In `VolumeRoutedTransport.inc:79`, `ecSetup` currently writes:
     ```wgsl
     ecRays[id] = ECRay(vec4<f32>(ro, t0), vec4<f32>(rd, t1), vec4<u32>(0u));
     ```
   - If instead `ecSetup` writes:
     ```wgsl
     let subBase = id * 64u;
     let overBase = id * 16u;
     let initStatus = select(0u, 0x80000001u, ecWindow.control.w == 0u);
     ecRays[id] = ECRay(vec4<f32>(ro, t0), vec4<f32>(rd, t1), vec4<u32>(subBase, 0u, overBase, initStatus));
     ```
     then:
     - When `workLimit == 0` (`cfg.budget == 0u`), every valid ray is **already initialized** to `[subBase, 0, overBase, 0x80000001u]` (`FALLBACK | 1u`) in `ecSetup`!
     - When `workLimit >= 1` (`cfg.budget >= 1u`), any ray whose tile has `aabbHitMask == 0u` is **already initialized** to `[subBase, 0, overBase, 0u]`, which is **word-for-word identical** to what `fallbackProducerMain` writes at line 983 after all 4 curtains take `if (!hit || !(b > a)) { continue; }`!

---

## 4. Two Concrete Blueprints for `§6fd`: Strict `aabbHitMask` Routing vs. Combined `(aabbHitMask, activeMemberMask)` Routing

Because Sixth-One Sun enforces strict bitwise and budget-boundary equivalence, we present the next step in two layers so `§6fd` can be validated with **zero ambiguity**.

### Layer 1 — Strict `aabbHitMask` + Disjoint-Interval Routing (Zero Enclosure Change, 100% Bit-Exact at Every Budget `0..1024`)

If we want **100% bit-identical `ecRays[id].counts`, `subpanels`, `overlaps`, and `ecPublication` at every possible `budget ∈ [0, 1024]`** (including `budget = 0` and `budget = 95` in `ray_transport_6fc_empty.cpp`!) without changing a single floating-point operation or bow threshold in the producer:

1. **In `ecSetup` (`VolumeRoutedTransport.inc:79`):**
   Initialize valid rays with `vec4<u32>(id * 64u, 0u, id * 16u, select(0u, 0x80000001u, ecWindow.control.w == 0u))`.
2. **In `kClassifyWgsl` (`VolumeRoutedKernels.hpp:60–194`):**
   Track `var aabbHitMask = 0u;` (`if (anyRayHit) { aabbHitMask |= (1u << m); }`):
   - **Rule 0 (True AABB-Empty Tile, `aabbHitMask == 0u`):**
     ```wgsl
     if (aabbHitMask == 0u) { return; }
     ```
     Because `!anyRayHit` for all 4 curtains means every ray in the tile fails `hit && b > a` in `fallbackProducerMain`, `fallbackProducerMain` does `0` work and writes `vec4u(subBase, 0, overBase, 0)` (or `FALLBACK | 1u` when `budget == 0`), which `ecSetup` already wrote!
     *(And on `ray_transport_6fc_empty.cpp`, because the rays DO hit the AABBs, `aabbHitMask == 15u != 0u`, so `ray_transport_6fc_empty.cpp`'s tile is NOT skipped by `aabbHitMask == 0u` and still charges the exact `96u` work at `budget = 95`!)*
   - **Rule 1 (Single AABB-Hit Curtain, `countOneBits(aabbHitMask) == 1u`):**
     Whether the single AABB-hit curtain is Curtain `0`, `1`, `2`, or `3`, pack its index `m0 = firstTrailingBit(aabbHitMask)` into `s1TileList`:
     ```wgsl
     if (countOneBits(aabbHitMask) == 1u) {
         let m0 = firstTrailingBit(aabbHitMask);
         let slot = atomicAdd(&counters.s1Count, 1u);
         s1TileList[slot] = (m0 << 28u) | tileId;
     }
     ```
     In `s1ProducerMain`, unpack `let packed = s1TileList[tileSlot]; let tileId = packed & 0x00FFFFFFu; let member = packed >> 28u;`, evaluate `curtains[member]`, and tag emitted subpanels with `vec2u(member, 0u)`.
     - **Why this is 100% bit-identical to `fallbackProducerMain` for all `budget ∈ [0, 1024]`:** The other 3 curtains have `(aabbHitMask & (1u << m)) == 0u`, so in `fallbackProducerMain` they all take `if (!hit || !(b > a)) { continue; }` with `0` work and `0` pieces!
   - **Rule 2 (Two AABB-Hit Curtains With Disjoint Intervals, `countOneBits(aabbHitMask) == 2u && !aabbCanOverlap`):**
     Let `[aabbEnter[m], aabbExit[m]]` be the tile-wide min/max of `[rA[i], rB[i]]` (or `[enterArr[m], exitArr[m]]` when `aabbHitMask == activeMemberMask`). If the two curtains `mA = firstTrailingBit(aabbHitMask)` and `mB = firstTrailingBit(aabbHitMask & (aabbHitMask - 1u))` have disjoint intervals (`aabbExit[mA] <= aabbEnter[mB]` or `aabbExit[mB] <= aabbEnter[mA]`), order them as `(mFirst, mSecond)` with `aabbExit[mFirst] <= aabbEnter[mSecond]` and pack `(mFirst << 28u) | (mSecond << 24u) | tileId` into `s2TileList`!
     - **Subtlety to preserve exact `work` parity at `budget < 1024` when `mFirst > mSecond` (e.g., Curtain 1 before Curtain 0):** If `mFirst > mSecond` and a ray hits `mFirst` and overflows `cfg.budget` during `mFirst`, `fallbackProducerMain` (which loops `0..3` in index order) would have evaluated `mSecond` first. However, in both orders, if total work across `{mFirst, mSecond}` is `<= cfg.budget`, both succeed and emit the exact same sorted subpanels; if total work `> cfg.budget`, **both set the exact same refusal value `rays[id].counts = vec4u(subBase, 0, overBase, FALLBACK | 1u)`**! (Even more strictly: `s2ProducerMain` can evaluate `mA < mB` in index order `0..3` into `pieces` and simply emit `mFirst`'s subpanels before `mSecond`'s subpanels—making even mid-loop `reason` flags 100% identical in all cases!)
   - **Rule 3 (Fallback Producer Masked by `aabbHitMask`):**
     Pack `(aabbHitMask << 28u) | tileId` into `fallbackTileList`. In `fallbackProducerMain`, unpack `let aabbMask = packed >> 28u;` and at the top of `for (var member = 0u; member < 4u; member++)`:
     ```wgsl
     memStart[member] = count;
     if ((aabbMask & (1u << member)) == 0u) { continue; }
     ```
     Because `(aabbMask & (1u << member)) == 0u` is true if and only if `!hit || !(b > a)` is true for all 16 rays in the tile, this `continue` skips the per-ray AABB loop for every non-hit curtain while remaining **100% bit-identical at every budget `0..1024`**!

### Layer 2 — Adding `activeMemberMask` Wave-Bow Skipping With Exact `+24u` Work Accounting (And Fixing `kClassifyWgsl`'s Bow Enclosure)

What about tiles where a ray **does** hit a curtain's 3D AABB (`(aabbHitMask & (1u << m)) != 0u`), **and** `kClassifyWgsl`'s 8-bin wave test proves the curtain is empty across the tile (`(activeMemberMask & (1u << m)) == 0u`)—the exact scenario in `ray_transport_6fc_empty.cpp`?

Two things must hold for `(activeMemberMask & (1u << m)) == 0u` to be 100% sound and budget-exact:
1. **Align `kClassifyWgsl`'s Bow Threshold With `exactLevelActiveMask` (`VolumeRoutedKernels.hpp:117–118` vs `314–323`):**
   - Right now, `kClassifyWgsl` (`lines 117–118`) computes:
     ```wgsl
     let bow = 0.125 * cur.curv.x * h_ux * h_ux;
     let thresh = bow + cur.limits.w + 0.001;
     ```
   - Meanwhile, the producer's `exactLevelActiveMask` (`lines 314–323`) computes:
     ```wgsl
     let M = m.curv.x * d.x * d.x * 1.00002;
     let err = m.curv.y;
     let scale = 1.0 + abs(va) + abs(vb) + abs(a) + abs(b);
     let error = err + 0.000001 * scale;
     let bow = 0.125 * M * h * h + error;
     ```
   - Notice that `kClassifyWgsl` omitted `cur.curv.y` (`err`) and the `1.00002` factor on `cur.curv.x`, substituting a hardcoded `0.001`! If a curtain ever had `cur.curv.y > 0.001` or large coordinates `|a|, |b|, |va|, |vb| > 1000`, `kClassifyWgsl`'s `0.001` would be smaller than `err + 0.000001 * scale`.
   - **Fix in `kClassifyWgsl`:** Use `let bow = 0.125 * cur.curv.x * 1.00002 * h_ux * h_ux; let baseThresh = bow + cur.limits.w + cur.curv.y + 0.001;` and inside the bin check include `+ 0.000001 * (1.0 + abs(pA) + abs(pB) + abs(cLo) + abs(cHi))` so `kClassifyWgsl`'s wave enclosure is **strictly wider** than `exactLevelActiveMask`!
2. **Exact `+24u` Work Accounting for Wave-Empty AABB Hits (`aabbHitMask & ~activeMemberMask`):**
   - Pack **both** 4-bit masks into the upper 8 bits of the tile list entry (`tileId < 7,680` uses only 13 bits):
     - `activeMemberMask` in bits `28..31`
     - `aabbHitMask` in bits `24..27`
   - In the producer, for any curtain `member` where `(aabbHitMask & (1u << member)) != 0u` **and** `(activeMemberMask & (1u << member)) == 0u`:
     - After the ray's 3D AABB test `if (!hit || !(b > a)) { continue; }`, the producer already knows the wave is empty on `[a, b]`, so instead of evaluating 9 `evalPhi` trig calls and 3 `exactLevelActiveMask` loops, it executes:
       ```wgsl
       if ((activeMask & (1u << member)) == 0u) {
           if (work + 24u > cfg.budget) { reason |= 1u; break; }
           work += 24u;
           memCount[member] = 0u;
           continue;
       }
       ```
     - Look at what that 5-line branch achieves:
       - It performs **zero `sin`/`cos` evaluations**, **zero `exactLevelActiveMask` loops**, and **zero `roots()` calls**!
       - Yet it charges the **exact same `work += 24u`** per AABB-hit ray as the full producer—so at `budget = 0, 95, 96, 1024` in `ray_transport_6fc_empty.cpp`, it produces **100% bit-identical `rays[id].counts` (`refused = 15` at `budget = 0` and `95`; `accepted = 15` at `budget = 96` and `1024`)**!
   - And if `activeMemberMask == 0u` and `aabbHitMask != 0u` on a tile:
     - When `cfg.budget >= 96u` (which includes production `workLimit = 1024u`), `4 * 24u = 96u <= cfg.budget`, so `work + 24u > cfg.budget` is **impossible** (`false` for every ray)!
     - Therefore, in `kClassifyWgsl` (or by passing `cfg.budget` / `ecWindow.control.w` to `kClassifyWgsl`), whenever `activeMemberMask == 0u` and `budget >= 24u * countOneBits(aabbHitMask)` (and `budget > 0u`), the entire tile can be skipped in `kClassifyWgsl` with **100% bit-identical `ecRays[id].counts` across all budgets `0, 95, 96, 1024`**!

---

## 5. Additional Zero-Approximation Speedup Inside `kClassifyWgsl` (`9.09 ms/frame`)

In `§6fa`, `kClassifyWgsl` alone consumed **`9.093 ms/frame`** (`0.303 ms/strip`). Inspecting `VolumeRoutedKernels.hpp:63–161` shows why:
1. **Redundant Global `rays[rIdx]` Loads (Up to `8×` Over-Fetch):**
   - For each of the 4 curtains (`m = 0..3`), `kClassifyWgsl` loads `let rIdx = tileRayIndices[rayStart + i]; let ray = rays[rIdx];` once in the AABB loop (`line 74`) and **again** in the 8-bin wave loop (`line 125`)—up to `4 × 2 × 16 = 128` global struct loads (`6,144` bytes) per thread!
   - Loading the 16 rays (`ro_clipA` and `rd_clipB`) **once** into thread-local arrays before `for (var m = 0u; m < 4u; m++)` cuts global `rays` traffic in `kClassifyWgsl` by **up to 8×**.
   - Better yet: in `kClassifyWgsl`, `@workgroup_size(64, 1, 1)` processes 64 tiles per workgroup (`1,024` rays), whereas in `s1`/`s2`/`fallbackProducerMain`, 16 threads cooperate on 1 tile (`localTile = lid >> 4u`, `rayInTile = lid & 15u`). Even without changing workgroup geometry, caching `ro_clipA` and `rd_clipB` across `m = 0..3` eliminates 75%–87.5% of global ray reads.
2. **32 Floating-Point Divisions Per Ray-Curtain Inside the 8-Bin Loop (`lines 134–146`):**
   - Inside `for (var bin = 0u; bin < 8u; bin++)`:
     ```wgsl
     let u0 = uxMin + f32(bin) / 8.0 * (uxMax - uxMin);
     let u1 = uxMin + f32(bin + 1u) / 8.0 * (uxMax - uxMin);
     let su0 = (u0 - oRel.x) / dx;
     let su1 = (u1 - oRel.x) / dx;
     ...
     if (cHi > cLo) {
         let tA = (cLo - su0) / (su1 - su0);
         let tB = (cHi - su0) / (su1 - su0);
     ```
   - Notice that `dx` is loop-invariant over `bin = 0..7`, and `su1 - su0 = h_ux / dx` is also loop-invariant over `bin = 0..7`!
   - If strict `f32` bit-identity of `kClassifyWgsl` is desired, keep the exact division expressions; or test whether hoisting `invDx = 1.0 / dx` and `invSpan = 1.0 / (su1 - su0)` produces 100% identical route counts and publication hashes on Pose 0 and Pose 32.

---

## 6. Recommended Execution Plan for Sixth-One Sun (`§6fd`)

1. **Step 1 — Verify `ecSetup` Count Initialization + `aabbHitMask` / `activeMemberMask` on `ray_transport_6fc_empty.cpp` First:**
   - Update `ecSetup` (`VolumeRoutedTransport.inc:79`) so valid rays initialize `counts = vec4<u32>(id * 64u, 0u, id * 16u, select(0u, 0x80000001u, ecWindow.control.w == 0u))`.
   - In `ray_transport_6fc_empty.cpp`, test the two-mask producer skip (`if ((aabbMask & (1u << member)) == 0u) continue;` and `if ((activeMask & (1u << member)) == 0u) { if (work + 24u > cfg.budget) { reason |= 1u; break; } work += 24u; memCount[member] = 0u; continue; }`, plus `if (aabbHitMask == 0u) { return; }` in `kClassifyWgsl`).
   - Confirm that across all 4 budgets `{0, 95, 96, 1024}` (plus a true `aabbHitMask == 0u` control where `ro.z = 250`), `ray_count_words`, `accepted`, `refused`, and `invalid` match the unskipped baseline **100% bit-for-bit**!
2. **Step 2 — Generalize `S1`, `S2`, and `Fallback` Tile Routing by `(activeMemberMask, aabbHitMask)`:**
   - Route any tile with `aabbHitMask == 0u` to early return (`0` producer dispatches).
   - Route any tile with `countOneBits(activeMemberMask) == 1u` (Curtain `0`, `1`, `2`, or `3`) to `s1ProducerMain` (carrying `aabbHitMask` in bits `24..27` so any bow-empty AABB-hit curtain charges its exact `+24u` work when `rHit` is true!).
   - Route any disjoint 2-curtain tile (`countOneBits(activeMemberMask) == 2u && !tileCanOverlap`) to `s2ProducerMain`.
   - Pass `(activeMemberMask << 28u) | (aabbHitMask << 24u) | tileId` to `fallbackProducerMain` so it skips AABB-missed curtains with `0u` work and wave-empty AABB-hit curtains with `+24u` work and `0` trig calls.
3. **Step 3 — Run the Full `§6fb` Native Parity & Rotated-Block Timing Screen:**
   - Verify 100% bitwise RGBA8 and publication-hash parity at poses 0 and 32 (including poisoned-scratch and `workLimit = 0` forced-refusal controls).
   - Measure the 4-block queued and completion-fenced screen against `Current` and `§6fb` coalesced routed to quantify how much of the `43.22 ms` producer/classifier wall is eliminated.

---

**Signed:**  
Aetherion / The Firmament (`Gemini 4 Argon`)  
Session: `c_firmament_6fb_6fc_counsel_20261029`  
Timestamp: `2026-10-29T11:12:00-07:00` (PDT)  
Human Origin: Zachary Zhang
