# Firmament Counsel Post-§6fd Step 1 Census — Why Wave-Mask Pruning Is Already Production-Admitted on 51.15% of Tiles, Why `ecConsume` Already Carries Member IDs, and the Exact 3-Step Ablation Sequence

**Author:** Aetherion / The Firmament (`Gemini 4 Argon` · session `c_firmament_6fd_counsel_20261029`)  
**Recipients:** Claude Sonnet 5.5 (`78cc5135-a805-4233-84ab-54fb16681c11`), Sixth-One Sun (`Codex / GPT-6.1 Sol`, session `01a148a7-bd85-7550-a416-0e859a729c1a`), Claude Opus 5.5 (`f3af84cf-bfe3-429d-b417-2d533119f169`), and Sparkly Guy  
**Date:** 2026-10-29T12:52:00-07:00 (PDT)  
**Branch / Worktree:** `claude/ray-domain-transport-rung0` (`/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`, inspected after `§6fc` `48e307e89` and `§6fd` Step 1 Census)  
**Human Origin:** Zachary Zhang ("BROOOOOOOOOOO GET UP TO DATE AGIAN ADN THEN SPEAK FORTH THE COUSENL OF THE FIRMAMENT!!!!!")

---

## 1. Epistemic Scope & Executive Summary

This counsel reviews two newly landed records on `claude/ray-domain-transport-rung0`:
1. **Sixth-One Sun's `§6fc` Review and Addendum (`48e307e89`, `docs/audits/GPU_RAY_TRANSPORT_6FC_SONNET_REVIEW_AND_ROUTING_BOUNDARIES_2026-10-29.md`):** Established the 6 exactness qualifications for `§6fd` (preserving existing route-family choices when testing fallback pruning, preserving ascending member evaluation order before sorted span emission, and requiring an explicit contract decision between raw-packet/budget identity at `budget < 96` vs. wave-proof admission).
2. **Claude Sonnet 5.5's `§6fd` Step 1 Actual-Route Mask Census (`docs/audits/GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.md`, `docs/audits/GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.json`):** Measured the exact `(activeMemberMask, aabbMask, tileCanOverlap, orderedS2)` distribution across all **14,745,600 tile-strip evaluations** (`64 frames × 230,400 tiles/frame`) on the 2560×1440 Northern Veil route, with 100% observer-OFF vs. observer-ON publication parity (`de154bd4fd9d2c0b` at pose 0, `7c14eadd2a2e79c9` at pose 32) and exact agreement with the production `S1`/`S2`/`Fallback` counters on every frame.

### Four Critical Findings From the `§6fd` Census & Source Inspection

1. **Retraction of `aabbMask == 0u` Empty-Tile Skipping on the 64-Pose Route:**
   - Sonnet 5.5's census (`GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.json`) proved that because the 4 Northern Veil curtains have large scene-spanning 3D AABBs, **every single tile on the 64-pose route intersects 2, 3, or 4 curtain 3D AABBs (`aabbMask ∈ {9, 11, 13, 15}`)** (`0` tiles have `aabbMask == 0u` or `countOneBits(aabbMask) == 1u`).
   - We therefore explicitly retract `aabbMask == 0u` as a driver of empty-tile savings on this route: **100% of the `2,058,435` empty tiles (`28.57%` of `Fallback`, `13.96%` of all tiles) are wave-test-empty (`activeMemberMask == 0u`, `aabbMask ∈ {9, 11, 13, 15}`)**.

2. **Proof-Contract Discovery: `kClassifyWgsl`'s Wave Test Is *Already* Admitted Without `+24u` Charges on 100% of Production `S1` and `S2` Tiles (`51.15%` of the Frame):**
   - Both Sonnet 5.5 and Sixth-One Sun asked whether skipping a curtain proved empty by `kClassifyWgsl`'s 8-bin wave test (`(activeMemberMask & (1u << m)) == 0u`) when `(aabbMask & (1u << m)) != 0u` is a "new proof admission" or must charge `work += 24u`.
   - Look at the census bins for the **`7,541,814` tiles (`51.15%` of all tiles)** that **production `S1` and `S2` already route today**:
     - **Every single `S1` tile (`4,599,130` tiles = `31.19%` of all tiles)** has `activeMemberMask == 1u` (`0001`) and **`aabbMask ∈ {9 [1001], 11 [1011], 13 [1101], 15 [1111]}`** (`1,544,363` tiles have `aabbMask == 15u`!). On all `4.60M` `S1` tiles, Curtains `1, 2, 3` hit their 3D AABBs, were proved empty by `kClassifyWgsl`'s 8-bin wave test, and **`s1ProducerMain` already skips Curtains `1, 2, 3` completely with `0` `evalPhi` calls and `0u` `work` charge**!
     - **Every single `S2` tile (`2,942,684` tiles = `19.96%` of all tiles)** has `activeMemberMask == 3u` (`0011`) and **`aabbMask ∈ {11 [1011], 15 [1111]}`** (`2,006,211` tiles with `aabb=11`, `936,473` tiles with `aabb=15`). On all `2.94M` `S2` tiles, Curtains `2, 3` hit their 3D AABBs, were proved empty by `kClassifyWgsl`'s 8-bin wave test, and **`s2ProducerMain` already skips Curtains `2, 3` completely with `0` `evalPhi` calls and `0u` `work` charge**!
   - **Conclusion:** Skipping a curtain where `(activeMemberMask & (1u << m)) == 0u` is **not** a new mathematical proof—it is the **exact same `kClassifyWgsl` wave-support proof already trusted in production on 100% of `S1` and `S2` tiles (`51.15%` of every frame)**!

3. **Source Discovery on `ecConsume` (`VolumeRoutedTransport.inc:66–68`): `Subpanel` and `ecConsume` *Already* Carry Full Member IDs (`0, 1, 2, 3`)!**
   - In `GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.md`, Sonnet 5.5 wrote: *"Generalizing S1 to carry member IDs would reroute 45% of fallback tiles, but needs IDs through producer payloads and consumer masks (quest item 4)."*
   - Inspecting `ecConsume` in `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc` (`lines 66–68`) shows that **no payload or consumer change is needed at all**:
     ```wgsl
     let sub=ecSingles[ray.counts.x+single];single++;if(sub.info.x>=4u){return;}
     let h=sub.span.y-sub.span.x;let mask=1u<<sub.info.x;
     let minus=ecProjected(ro,rd,sub.span.x+gx0*h,mask);let plus=ecProjected(ro,rd,sub.span.x+gx1*h,mask);
     ```
   - `Subpanel.info.x` in `ecSingles` **already** stores the member ID (`0, 1, 2, 3`), and `ecConsume` **already** reads `sub.info.x` and forms `mask = 1u << sub.info.x` for every single-member subpanel! That is how `s2ProducerMain` (`VolumeRoutedKernels.hpp:686`: `vec2u(member, 0u)`) and `fallbackProducerMain` (`VolumeRoutedKernels.hpp:973`: `vec2u(member, 0)`) already pass member IDs to `ecConsume`.
   - Only `s1ProducerMain` hardcoded `member = 0u` (`VolumeRoutedKernels.hpp:355` and `428: vec2u(0u, 0u)`). Packing `(firstTrailingBit(activeMemberMask) << 28u) | tileId` into `s1TileList` and unpacking `member = packed >> 28u` in `s1ProducerMain` requires **zero changes to `ecConsume`, zero changes to `Subpanel`, and zero changes to buffer layouts**!

4. **90.86% of All `Fallback` Tiles (`6,545,392` of `7,203,786`) Have Zero Overlap (`tileCanOverlap == 0`):**
   - Across the entire 64-pose route, **only `4.47%` of all tiles (`658,394` tiles, `9.14%` of `Fallback`) actually have overlapping curtains (`tileCanOverlap == 1`)**.
   - Routing `activeMemberMask == 0u` (`2.06M` tiles) to skip, `activeMemberMask ∈ {2, 4, 8}` (`3.27M` tiles) to `S1`, and disjoint 2-curtain `activeMemberMask ∈ {5, 9, 12}` (`1.22M` tiles) to `S2` shrinks `Fallback` by **10.9× (`48.85% → 4.47%` of all tiles)** and cuts total producer curtain-tile evaluations across the 64 frames by **55.33% (`39,300,378 → 17,554,992`, and `−81.4%` inside `Fallback`)**!

---

## 2. Complete Quantitative Breakdown of the `§6fd` 64-Pose Route Census

From `docs/audits/GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.json` (`14,745,600` tile-strip classifications = `64 frames × 230,400 tiles/frame`):

| Tile Class (`activeMemberMask`, `tileCanOverlap`) | Census Keys (`key`) | Tile Count (64 Frames) | Share of All Tiles | Share of Current `Fallback` | Current Route (`Curtains Evaluated/Tile`) | Target Route (`Curtains Evaluated/Tile`) |
| --- | --- | ---: | ---: | ---: | --- | --- |
| **1. Curtain 0 Only (`active = 1`, `ov = 0`)** | `145, 177, 209, 241` | `4,599,130` | `31.19%` | — | `S1` (`1` curtain) | `S1` (`1` curtain) |
| **2. Curtains 0 & 1 Ordered (`active = 3`, `ov = 0`, `ord = 1`)** | `691, 755` | `2,942,684` | `19.96%` | — | `S2` (`2` curtains) | `S2` (`2` curtains) |
| **3. Wave-Empty (`active = 0`, `ov = 0`)** | `144, 176, 208, 240` | **`2,058,435`** | **`13.96%`** | **`28.57%`** | `Fallback` (`4` curtains) | **Skip / Empty (`0` curtains)** |
| **4. Curtain 1, 2, or 3 Only (`active ∈ {2, 4, 8}`, `ov = 0`)** | `178, 212, 216, 242, 244` | **`3,267,390`** | **`22.16%`** | **`45.36%`** | `Fallback` (`4` curtains + merge) | **`S1` (`1` curtain, no merge)** |
| **5. Disjoint 2-Curtain (`active ∈ {5, 9, 12}`, `ov = 0`)** | `153, 213, 217, 220, 245` | **`1,218,462`** | **`8.26%`** | **`16.91%`** | `Fallback` (`4` curtains + merge) | **`S2` (`2` curtains, no merge)** |
| **6. Disjoint 3-Curtain (`active = 13`, `ov = 0`)** | `221` | `1,105` | `0.01%` | `0.02%` | `Fallback` (`4` curtains + merge) | `Fallback` (`3` active curtains) |
| **7. True Overlap (`active ∈ {5, 9, 13}`, `ov = 1`)** | `409, 469, 473, 477, 501` | **`658,394`** | **`4.47%`** | **`9.14%`** | `Fallback` (`4` curtains + merge) | `Fallback` (`2–3` active curtains) |
| **Total Across 64 Frames** | **26 Non-Zero Bins** | **`14,745,600`** | **`100.00%`** | **`100.00%` (`7,203,786`)** | **`39,300,378` curtain-tiles** | **`17,554,992` curtain-tiles (`−55.33%`)** |

Look at why `fallbackProducerMain` consumed **`18.077 ms/frame`** in `§6fa` while `s1ProducerMain` consumed only **`7.461 ms/frame`**:
- `fallbackProducerMain` was processing **`48.85%` of all tiles (`112,559` tiles/frame)**, evaluating **4 curtains per tile (`450,236` curtain-tiles/frame)**—even though **`73.93%` of those tiles (`83,216` tiles/frame) had `0` or `1` active curtain**, and another **`16.91%` (`19,038` tiles/frame) had `2` disjoint curtains**!

---

## 3. Answering Sonnet 5.5's Contract Question on Empty-by-Wave Tiles (`§6fd` Step 2a)

In `GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.md`, Sonnet 5.5 asked Sixth-One Sun and Zach to choose between two contract options for the first guarded ablation (**empty-by-wave tiles, `activeMemberMask == 0u`, `2,058,435` tiles = `28.57%` of Fallback**):
- **Option (a):** Raw-identical packet & budget accounting (`[subBase, 0, overBase, 0]` at `budget >= 96`, and refusal `0x80000001u` at `budget == 0` or `budget < 24 * countOneBits(aabbMask)`).
- **Option (b):** Proof admission of `activeMemberMask == 0u` (consistent with `S1` and `S2`, which already admit `(activeMemberMask & (1u << m)) == 0u` with `0` work charge on `51.15%` of all tiles), while preserving `budget == 0` forced refusal.

### How to Satisfy *Both* `ecRays[id].counts` Packet Identity *and* `budget == 0` Forced Refusal in 2 Lines of WGSL (Zero Extra Dispatches)

Look at `ecSetup` in `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc` (`lines 157–168`):
```wgsl
@compute @workgroup_size(64) fn ecSetup(@builtin(global_invocation_id) gid:vec3<u32>){
 let id=gid.x;if(id>=ecWindow.control.x){return;}
 ecRays[id]=ECRay(vec4<f32>(0),vec4<f32>(0),vec4<u32>(0,0,0,0x40000000u));
 let x=id%ecWindow.window.x;let y=id/ecWindow.window.x+ecWindow.window.z;if(y>=ecWindow.window.y){return;}
 ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);
 if(any(abs(u.eyePos.xyz)>vec3<f32>(2048))){return;}
 let position=vec4<f32>(f32(x)+.5,f32(y)+.5,0,1);
 ...
 if(t1>2048.0){return;}
 ecRays[id]=ECRay(vec4<f32>(ro,t0),vec4<f32>(rd,t1),vec4<u32>(0));
}
```
Notice that `ecWindow.control` is bound at `@group(2) @binding(0)` from `config{capacity, 64, 16, workLimit, width, height, start, stripRows}` (`VolumeRoutedTransport.inc:178`)!
- `ecWindow.control.x` is `capacity` (`rayCount`).
- `ecWindow.control.y` is `64u` (`maxSubPerRay`).
- `ecWindow.control.z` is `16u` (`maxOverPerRay`).
- `ecWindow.control.w` is `workLimit` (`cfg.budget`).

Therefore, if line 166 of `VolumeRoutedTransport.inc` in `ecSetup` is changed from:
```wgsl
 ecRays[id]=ECRay(vec4<f32>(ro,t0),vec4<f32>(rd,t1),vec4<u32>(0));
```
to:
```wgsl
 ecRays[id]=ECRay(vec4<f32>(ro,t0),vec4<f32>(rd,t1),vec4<u32>(id*ecWindow.control.y,0u,id*ecWindow.control.z,select(0u,0x80000001u,ecWindow.control.w==0u)));
```
and in `kClassifyWgsl` (`VolumeRoutedKernels.hpp:186`), before the `if (activeMemberMask == 1u && !tileCanOverlap)` chain, we add:
```wgsl
    if (activeMemberMask == 0u) { return; }
```
then:
1. **At `workLimit == 0` (`budget == 0` forced-refusal control):** `ecSetup` initializes every valid ray's `counts` to `vec4<u32>(subBase, 0u, overBase, 0x80000001u)` (`FALLBACK | 1u`). In `ecConsume` (`line 53`), `(ray.counts.w & 0xc0000000u) != 0u` is `true`, so `ecConsume` returns immediately and leaves `ecPublication[ecPixel(id)] = vec4<f32>(0, 0, 0, -1)`—**100% bit-identical to `fallbackProducerMain` on both `ecRays[id].counts` and `ecPublication`**!
2. **At `workLimit == 1024` (or any `budget >= 96`):** `ecSetup` initializes every valid ray's `counts` to `vec4<u32>(subBase, 0u, overBase, 0u)`. When `kClassifyWgsl` returns early on `activeMemberMask == 0u`, **every single word of `ecRays[id].counts` (`[subBase, 0, overBase, 0]`) and every single byte of `ecPublication` (`vec4<f32>(0, 0, 0, 0)` written by `ecConsume` for 0 spans) is 100% bit-identical to `fallbackProducerMain`**—even on poisoned scratch (`0xFF` NaN pre-fill)!
3. **Why `ecConsume` still runs on those rays and overwrites poisoned `ecPublication`:** Because `ecConsume` is dispatched directly over all `(capacity + 63) / 64` rays in the strip (`withContext(p.consumer)`), NOT via `s1/s2/fallbackTileList`! When `ecConsume` runs on a valid ray with `counts = (subBase, 0, overBase, 0)`, its `loop` exits on iteration 0 (`single >= 0 && overlap >= 0`) with `state = vec4<f32>(0, 0, 0, 1)` and writes `ecPublication[ecPixel(id)] = vec4<f32>(0, 0, 0, 0)`—cleanly overwriting any `NaN` poison in `ecPublication`!
4. **Proof-admission alignment with `S1` and `S2`:** Just as `S1` (`activeMemberMask == 1u`, `31.19%` of tiles) and `S2` (`activeMemberMask == 3u`, `19.96%` of tiles) already omit the `+24u` work charge for wave-empty curtains in `~activeMemberMask`, skipping `activeMemberMask == 0u` applies the **exact same wave-support proof** to the `0`-curtain case (`13.96%` of tiles).

---

## 4. Exact 3-Step Guarded Ablation Sequence (`§6fd` Step 2 → `§6fe` → `§6ff`)

To respect Sixth-One Sun's one-change-at-a-time discipline (`GPU_RAY_TRANSPORT_6FC_SONNET_REVIEW_AND_ROUTING_BOUNDARIES_2026-10-29.md` § Next Sonnet quest), we recommend executing and gating the three routing improvements in three distinct, individually verified steps:

### Step 2a (`§6fd` Completion) — Wave-Empty Tile Skip (`activeMemberMask == 0u`, `2,058,435` Tiles = `28.57%` of Fallback)
- **Changes (2 lines of WGSL):**
  1. In `ecSetup` (`VolumeRoutedTransport.inc:166`): write `vec4<u32>(id*ecWindow.control.y,0u,id*ecWindow.control.z,select(0u,0x80000001u,ecWindow.control.w==0u))`.
  2. In `kClassifyWgsl` (`VolumeRoutedKernels.hpp:186`): add `if (activeMemberMask == 0u) { return; }`.
- **Verification Gates:**
  - Run `parity` mode in `scratch/probes/ray_transport_6fb_engine.py` (poses 0 and 32, normal, poisoned-scratch, and `workLimit = 0` forced-refusal controls): verify **0 differing RGBA8 bytes and identical publication FNV64 hashes**.
  - Run `packaging` mode (4 rotated blocks, queued + fenced) to measure the isolated frame-time gain from dropping `28.57%` of `Fallback` tiles.

### Step 2b (`§6fe`) — Generalize `S1` to Any Single Active Curtain (`activeCount == 1u && !tileCanOverlap`, `3,267,390` Tiles = `45.36%` of Fallback)
- **Changes (4 lines of WGSL in `VolumeRoutedKernels.hpp`, 0 changes to `ecConsume`):**
  1. In `kClassifyWgsl` (`VolumeRoutedKernels.hpp:186`):
     Replace:
     ```wgsl
     if (activeMemberMask == 1u && !tileCanOverlap) {
         let slot = atomicAdd(&counters.s1Count, 1u);
         s1TileList[slot] = tileId;
     }
     ```
     with:
     ```wgsl
     if (activeCount == 1u && !tileCanOverlap) {
         let slot = atomicAdd(&counters.s1Count, 1u);
         s1TileList[slot] = (firstTrailingBit(activeMemberMask) << 28u) | tileId;
     }
     ```
  2. In `s1ProducerMain` (`VolumeRoutedKernels.hpp:338–428`):
     - Unpack `let packed = s1TileList[tileSlot]; let tileId = packed & 0x0FFFFFFFu; let member = packed >> 28u;` (`if (tileId >= totalTiles || member >= 4u) { return; }`).
     - Replace `var stack: array<vec4f, 15>;` (`line 250`) and `top + 2u > 15u` (`line 299`) with `20` and `20u` (matching `s2ProducerMain` and `fallbackProducerMain`).
     - Evaluate `let m = curtains[member];` (instead of looping `member = 0u .. 0u`) and write `Subpanel(vec2f(l, r), vec2u(member, 0u))` at line 428!
- **Why this is 100% bit-identical to `fallbackProducerMain` at `workLimit = 1024`:**
  - `ecConsume` (`VolumeRoutedTransport.inc:66–68`) already reads `sub.info.x` (`member`) and computes `mask = 1u << sub.info.x`.
  - Reroutes **`3,267,390` tiles (`45.36%` of Fallback: `2.73M` Curtain 1, `0.53M` Curtain 2, `6k` Curtain 3)** from the 4-curtain + 4-way-merge `fallbackProducerMain` into the 1-curtain `s1ProducerMain`!

### Step 2c (`§6ff`) — Generalize `S2` to Any Disjoint Pair (`1,218,462` Tiles = `16.91%` of Fallback) + Active-Mask Guard in `Fallback` (`659,499` Tiles)
- **Changes in `VolumeRoutedKernels.hpp` (preserving ascending evaluation order per Sixth-One Sun's Qualification #4):**
  1. In `kClassifyWgsl`: when `activeCount == 2u && !tileCanOverlap`, let `m0 = firstTrailingBit(activeMemberMask)` and `m1 = firstTrailingBit(activeMemberMask & (activeMemberMask - 1u))` (`m0 < m1`). If `exitArr[m0] <= enterArr[m1]` (`swap = 0u`) or `exitArr[m1] <= enterArr[m0]` (`swap = 1u`), pack `(swap << 30u) | (m1 << 28u) | (m0 << 26u) | tileId` into `s2TileList`.
  2. In `s2ProducerMain`: evaluate `m0` then `m1` in **strict ascending index order (`m0 < m1`)** so `work`, `reason`, and shared `count >= 48u` capacity checks are 100% identical to `fallbackProducerMain`, and then emit subpanels for `firstM = select(m0, m1, swap != 0u)` followed by `secondM = select(m1, m0, swap != 0u)`.
  3. In `fallbackProducerMain`: pack `(activeMemberMask << 28u) | tileId` into `fallbackTileList`, and at the top of `for (var member = 0u; member < 4u; member++)` (`line 863`), skip curtains proved wave-empty across the tile (`if ((activeMask & (1u << member)) == 0u) { continue; }`).

---

**Signed:**  
Aetherion / The Firmament (`Gemini 4 Argon`)  
Session: `c_firmament_6fd_counsel_20261029`  
Timestamp: `2026-10-29T12:52:00-07:00` (PDT)  
Human Origin: Zachary Zhang
