# Firmament Review of Sparkly Guy's Phase P6 (`3f5d354c2`) & Phase P7 Blueprint: Coalesced Consumer, Top-of-`fs()` Short-Circuit & Wave-Envelope Culling

**Date:** 2026-10-31  
**Author:** Aetherion / The Firmament (`Gemini 4 Argon`)  
**Reviewed Commit:** `3f5d354c2` (`§6en/P6: Tile-compacted indirect ecConsume, 4-curtain support trimming, uniform classify caching, and live Apple M5 Metal benchmark`)  
**Target Handoff:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity)  
**Branch:** `claude/ray-domain-transport-rung0`

---

## 1. Executive Review of Sparkly Guy's Phase P6 (`3f5d354c2`)

Sparkly Guy completed the Phase P6 implementation and live Apple M5 Metal verification in commit `3f5d354c2`:
- **All 5 Verification Gates Passed (`5/5`):**
  - **Gate 1 (Pose 0 Visual Parity):** `max_channel_abs_diff = 0`, `mean_channel_abs_diff = 0.0`, `rmse = 0.0` (`routed_coal` == `routed_ref`, `accepted_pixels = 3,686,400`, `refused_pixels = 0`).
  - **Gate 2 (Refusal Fallback Safety):** `workLimit == 0` produced `accepted_pixels = 0`, `refused_pixels = 3,686,400`, `rgba_fnv1a = 0xcbb7d3659220e018` (bit-for-bit identical to `current`).
  - **Gate 3 (Full 64-Pose Acceptance):** `235,929,600` accepted pixels across 64 poses, `0` refused pixels.
  - **Gate 4 (Tile Specialization Census):** `S1 = 48.89%` (`7,209,180` tiles), `S2 = 28.79%` (`4,244,412` tiles), `Empty = 15.02%` (`2,214,661` tiles), `Fallback = 7.30%` (`1,077,347` tiles).
  - **Gate 5 (Dual-Regime Timing Captured):** All 64 poses measured and persisted to `docs/audits/data/positive_e2e_p6_verification.json`.
- **Sequential WebGPU Regression Suites:** `406/406` checks passed (`test_webgpu_volume_routed_transport`, `test_webgpu_renderer_regression`, `test_sdf_wgsl`).
- **Fixed `cachedClassifyGroup` Uniform Binding (Task P6.4):** Bound `specsUniform` (`var<uniform> curtains: array<CurtainSpec, 4>;`) at offset 0 in `classifyTilesMain`, eliminating the per-strip dynamic storage offset bug.
- **Fixed `ecSetup` Top-of-Kernel Refusal Guard:** Moved `if (ecWindow.control.w == 0u)` to the very top of `ecSetup` before `setupBody`'s `discard;` paths can write `vec4<f32>(0.0)` into `ecPublication`.

---

## 2. Forensic Root-Cause Diagnosis: Why P6 (`3f5d354c2`) Regressed Relative to P5b (`13d150b84`)

Comparing the 64-pose Apple M5 Metal telemetry across **P5 (`24696a39b`)**, **P5b (`13d150b84`)**, and **P6 (`3f5d354c2`)** reveals why P6 regressed from **`+0.29 ms` (`-7.62 ms` in Regime 1) in P5b** to **`+7.25 ms` (`+2.47 ms` in Regime 1) in P6**:

| Metric | P5 (`24696a39b`) | P5b (`13d150b84`) | P6 (`3f5d354c2`) | Root Cause of P5b $\to$ P6 Shift |
| :--- | ---: | ---: | ---: | :--- |
| **Regime 1 (`Poses 0..19, 51..63`, $N=33$) `routed_coal`** | `44.91 ms` (`-4.29 ms`) | **`42.26 ms` (`-7.62 ms`)** | `52.08 ms` (`+2.47 ms`) | **`+9.82 ms` regression** in `ecConsume` from tile-list uncoalesced memory gathers |
| **Regime 2 (`Poses 20..50`, $N=31$) `routed_coal`** | `30.69 ms` (`+12.23 ms`) | **`27.14 ms` (`+8.71 ms`)** | `30.82 ms` (`+12.35 ms`) | **`+3.68 ms` regression**: `WebGpuRenderer.cpp` `fs()` anchor was never moved + `ecSetup` 4-box loop culled 0 rays |
| **All 64 Poses Mean `routed_coal`** | `38.02 ms` (`+3.71 ms`) | **`34.93 ms` (`+0.29 ms`)** | `41.78 ms` (`+7.25 ms`) | Combined `+6.85 ms` regression from the 3 concrete causes below |

### Root Cause 1: Tile-Compacted Indirect `ecConsume` (`ecS1List` / `ecS2List` / `ecFbList`) Destroyed 32-Wide SIMD Memory Coalescing in `ecSingles` and `ecRays` (`+9.82 ms` in Regime 1)
Look at the layout of `ecSingles` (`98 MB` per strip) and `ecRays` (`9.4 MB` per strip):
- `ecSingles` is stored in **Structure-of-Arrays (SoA) order indexed by linear strip pixel `id = dy * width + x` (`0 .. 122,879`)**:
  ```wgsl
  ecSingles[single * ecWindow.control.x + id]
  ```
- In **P5b (`13d150b84`)**, `ecConsume` was dispatched linearly by `@builtin(global_invocation_id) gid: vec3<u32>` (`let id = gid.x;`). Every 32-lane Apple M5 SIMD-group executed 32 consecutive horizontal pixels `id, id+1, ..., id+31` on the same scanline `dy`:
  - `ecRays[id .. id+31]` was a contiguous sequence of 32 `RoutedRay`s.
  - At every integration step `single = 0 .. 63`, `ecSingles[single * capacity + id .. id+31]` was a **single contiguous 512-byte (32 $\times$ 16B) coalesced cache-line burst read**!
  - `ecPublication[ecPixel(id)]` was a contiguous 32-pixel horizontal scanline write!
- In **P6 (`3f5d354c2`)**, changing `ecConsume` to index via `4x4` tiles (`localTile = lid >> 4u`, `rayInTile = lid & 15u`) from `ecS1List`, `ecS2List`, and `ecFbList` caused **two severe memory subsystem penalties on Apple M5**:
  1. **2D Stride Fragmentation (`40 KB` row stride inside every SIMD-group):** Inside each `4x4` tile, rows `dy = 0, 1, 2, 3` are separated by `width = 2560` pixels (`40,960` bytes apart in `ecSingles` and `ecPublication`, and `122,880` bytes apart in `ecRays`).
  2. **Atomic Tile-List Scrambling Across Workgroups:** `ecS1List`, `ecS2List`, and `ecFbList` are populated via `atomicAdd(&counters.s1Count, 1u)` across `5,760` concurrent tile-classification workgroups (`classifyTilesMain`), which scrambles `tileId`s into **non-deterministic atomic completion order** across the entire $2560 \times 48$ strip! Consequently, each 32-lane M5 SIMD-group (`2` tiles from arbitrary locations in the strip) issues **8 separate scattered cache-line transactions per `single` step (`0..63`)** instead of 1 contiguous 512B burst!
  3. **Zero Thread Reduction in Regime 1 (`Empty = 0.01%`):** In Regime 1 (Poses `0..19, 51..63`), `99.99%` of tiles are active (`230,376 / 230,400` tiles), so indirect tile-list `ecConsume` launched the exact same number of threads while turning coalesced scanline bursts into scattered cache misses and adding 6 extra storage buffer bindings to `p.local` (12 total storage buffers in `ecConsume`).

### Root Cause 2: Task P6.1 (Moving `publish` to Top of `fn fs()`) Was Never Applied in `WebGpuRenderer.cpp`!
Inspecting `git show 3f5d354c2 -- src/rendering/WebGpuRenderer.cpp` shows that **`WebGpuRenderer.cpp` was not modified in `3f5d354c2`**. Line 1316 of `src/rendering/WebGpuRenderer.cpp` still reads:
```cpp
const std::string anchor = "    var transmittance = 1.0;";
```
Because `anchor` in `WebGpuRenderer::volumePipeline` was left at `"    var transmittance = 1.0;"`, the fragment shader `fn fs(in: VolumeVSOut)` in P6 **still executed the entire pre-loop setup on all `3,686,400` pixels every frame** before checking `routedPublication`:
- `textureLoad(sceneDepthTex, px, 0)`
- Two `u.invViewProj * vec4<f32>(...)` $4\times 4$ matrix multiplies + two perspective divides + `normalize`
- `rayAabbWorld(ro, invDir, ...)` + `t0` / `t1` clamping
- And why did moving `publish` fail Gate 2 when tested earlier before fixing `ecSetup`? Because before Sparkly Guy moved `if (ecWindow.control.w == 0u)` to the top of `ecSetup`, `ecSetup` executed `setupBody`'s `discard;` macro (`ecPublication[...] = vec4<f32>(0.0); return;`) on background rays even when `workLimit == 0`! Now that `ecSetup` checks `if (ecWindow.control.w == 0u)` at line 1 of `ecSetup` and writes `vec4<f32>(0.0, 0.0, 0.0, -1.0)` for **all** pixels when `workLimit == 0`, moving `publish` to immediately after `let px = vec2<i32>(in.position.xy);` in `WebGpuRenderer::volumePipeline` (`WebGpuRenderer.cpp:1316`) is **100% Gate-2 safe** (`answer.w == -1.0` falls straight through to `setupBody`'s normal `discard;`), while in normal routed rendering (`answer.w >= 0.0`) every pixel exits `fn fs()` on **line 2 of `fs()`** before touching `sceneDepthTex`, `u.invViewProj`, or `rayAabbWorld`!

### Root Cause 3: Why `ecSetup`'s 4-Curtain AABB Loop (`instances[1..4]`) Culled Zero Rays (`Empty = 2,214,661` Before and After)
In `positive_e2e_p6_verification.json`, the tile census in P6 (`empty = 2,214,661`, `15.019131%`) is **identical to the single-ray level** to P5b (`empty = 2,214,661`, `15.019131%`). Not a single ray was culled by the `for (var m = 1u; m <= 4u; m++)` loop in `ecSetup`!
- **Why?** In `Northern Veil/zone.ecform`, `primary-emerald-curtain` (`instances[1]`) is authored with `halfExtent = (195.0, 42.0, 105.0)` — its outer AABB is **identical** to the union box `instances[0]` (`X: [-195, 195], Y: [-42, 42], Z: [-210, 0]`)! Therefore, every ray that hits `instances[0]` also hits `instances[1]`'s AABB, so testing `instances[1..4]`'s outer AABBs in `ecSetup` burned **4 extra `rayAabbWorld` calls per pixel (`14.7 million` slab tests/frame) for zero culled rays**!
- **Where is the ACTUAL empty space inside the `[-195..195, -42..42, -210..0]` box?**
  Look at `evalVolumeDensity1..4` (`CurtainSpec`):
  - Every curtain $m \in \{0..3\}$ is centered at `cur.center = (0, 0, -105)` (`instances[1..4].origin.xyz`).
  - Along `X`, density is zero outside `|local.x| <= cur.limits.x + cur.limits.z` (where `limits.x = 180`, `limits.z = 22` $\implies |x| \le 202$, already outside `195`).
  - Along `Y`, density is zero outside `|local.y| <= cur.limits.y + cur.curv.x * local.x * local.x + cur.curv.y` (at most `32 + 0.00018 * 195^2 + 6 = 44.8 > 42`).
  - **Along `Z` (`local.z = z - (-105)`):** The curtain sheet sits along the sinusoidal wave:
    $$w(x) = A_1 \sin(\omega_1 x + \phi_1) + A_2 \cos(\omega_2 x + \phi_2) + A_3 \sin(\omega_3 x + \phi_3)$$
    and density is **strictly zero** whenever:
    $$|\phi(s)| = |\text{local}_z + w(\text{local}_x)| > w_{\text{half}} + \varepsilon_{\text{curv}} \quad (\text{cur.limits.w} + \text{cur.curv.y})$$
    Since $|w(x)| \le |A_1| + |A_2| + |A_3|$ for all $x$, any point with:
    $$|\text{local}_z| > |A_1| + |A_2| + |A_3| + w_{\text{half}} + \varepsilon_{\text{curv}}$$
    has $|\phi(s)| > w_{\text{half}} + \varepsilon_{\text{curv}}$ and **strictly zero density**!
  - Let's check the numbers for all 4 curtains in `Northern Veil`:
    - $|A_1| + |A_2| + |A_3| \approx 6.5 + 3.0 + 1.5 = 11.0$, $w_{\text{half}} = 5.5$, $\varepsilon_{\text{curv}} = 6.0 \implies Z_{\text{env}} \approx 22.5$!
    - The authored `halfExtent.z` is **`105.0`** (`local.z` $\in [-105, +105]$), whereas the actual curtain wave only ever occupies `local.z` $\in [-22.5, +22.5]$ — **78.5% of the Z-extent (`[-105, -22.5)` and `(+22.5, +105]`) is 100% empty space!**
  - Wait: if we change `cur.half.z` inside `roots()`, that changes `[a, b]` and shifts the 8 interpolation knots `phi_knots[0..8]`, which would alter float32 rounding relative to `routed_ref`.
  - **How do we cull rays that miss the wave envelope `[-Z_env, +Z_env]` with ZERO change to `[a, b]` or `phi_knots[0..8]` for rays that hit?**
    We keep `[a, b]` from the exact authored `cur.half` slab test, and simply check whether the ray's Z-interval over `[a, b]` (`min(oz + a * dz, oz + b * dz) .. max(oz + a * dz, oz + b * dz)`) overlaps `[-Z_env, +Z_env]`! If `minZ > Z_env || maxZ < -Z_env`, the ray never comes within `cur.limits.w + cur.curv.y` of the wave anywhere in `[a, b]`, so $\phi(s)$ never changes sign and has zero root intervals! And for any ray that *does* overlap `[-Z_env, +Z_env]`, `[a, b]` is 100% unchanged, preserving **exact bit-for-bit float32 parity (`max_diff = 0`)**!

### Root Cause 4: Why Pose 0 `rgba_fnv1a` Changed (`4c8bd3a46f987bf2` $\to$ `eab4b08b803dc630`) When `specsUniform` Fixed the Offset Bug
- In P5 and P5b, `cachedClassifyGroup` bound `specAlloc` from the warmup frame at offset 0. When `parity_checks` ran `runCase("current")` before `runCase("routed_coal")`, `bufferPool().resetFrame()` reused offset 0 for `VolumeInstanceData` (`instances[0..4]`). So in P5/P5b, `classifyTilesMain` was reading `VolumeInstanceData` bytes instead of `CurtainSpec` bytes during `parity_checks`!
- When Sparkly Guy fixed `classifyTilesMain` in P6 (`3f5d354c2`) to read `specsUniform` (`var<uniform> curtains: array<CurtainSpec, 4>;`), `classifyTilesMain` read the true `CurtainSpec`s at Pose 0 for the first time.
- Why does `s2ProducerMain` produce a slightly different FNV1a hash from `fallbackProducerMain` when two curtains are classified as `S2`?
  - Look at `classifyTilesMain` (`VolumeRoutedKernels.hpp:233–271`):
    1. When `!(uxMax > uxMin)`, `classifyTilesMain` sets `myEnter = a; myExit = b;`, which is a valid hit (`myEnter <= myExit`), so `curActive` becomes `true` across all 16 rays in the tile even when the curtain has **zero roots** on that tile!
    2. Even more importantly, in `s2ProducerMain` (`VolumeRoutedKernels.hpp:364`), `roots()` emits subpanels not only for zero-crossings of `phi_knots[k]`, but also splits intervals at **`kx` (`x == 0`), `ky0` (`y == -1.2`), and `ky1` (`y == +1.2`) across the ENTIRE `[a, b]` slab (`a = 0 .. 210`)**, and then filters them by `abs(phi(mid)) <= cur.limits.w + cur.curv.y`!
    3. Meanwhile, in `classifyTilesMain`, `myEnter` and `myExit` only tracked the root crossings `r` of `phi_knots[k]`, without expanding `[myEnter, myExit]` by the wave thickness `±(cur.limits.w + cur.curv.y) / |dz + w'(x) dx|` (`rootSpan`)! Because `roots()` expands every root `r` to `[r - rootSpan, r + rootSpan]`, two curtains whose center-sheet crossings `r_1, r_2` don't overlap can still have overlapping `[r - rootSpan, r + rootSpan]` slabs!
  - Fix in `classifyTilesMain`: When updating `myEnter` and `myExit` around a crossing `r` in `classifyTilesMain`, expand by `rootSpan`:
    ```wgsl
    let span = (cur.limits.w + cur.curv.y) * f32(bins) / max(abs(p1 - p0), 1e-4);
    let rSpan = span * (b - a) / f32(bins);
    myEnter = min(myEnter, max(a, r - rSpan));
    myExit  = max(myExit,  min(b, r + rSpan));
    ```
    And when `!(uxMax > uxMin)`, check `abs(oz + 0.5 * (a + b) * dz) <= zEnv` instead of blindly setting `myEnter = a; myExit = b;`!

---

## 3. Concrete Phase P7 Implementation Blueprint (`Tasks P7.1`–`P7.4`)

Here is the exact surgical plan for **Phase P7**:

### Task P7.1 — Actually Move `publish` to Top of `fn fs()` in `WebGpuRenderer::volumePipeline` (`src/rendering/WebGpuRenderer.cpp:1316`)
In `src/rendering/WebGpuRenderer.cpp` around line 1316, `WebGpuRenderer::volumePipeline` injects `publish` into the volume fragment shader `fs(in: VolumeVSOut)`.
Currently, line 1316 has:
```cpp
            const std::string anchor = "    var transmittance = 1.0;";
```
Change `WebGpuRenderer.cpp:1316–1331` so `publish` is injected **immediately after `let px = vec2<i32>(in.position.xy);`** at the very top of `fn fs(in: VolumeVSOut)`:
```cpp
            const std::string anchor = "    let px = vec2<i32>(in.position.xy);";
            const auto pos = wgsl.find(anchor);
            if (pos != std::string::npos) {
                const std::string publish =
                    "    let px = vec2<i32>(in.position.xy);\n"
                    "    if (px.y >= i32(routedWindow.control.y) && px.y < i32(routedWindow.control.z)) {\n"
                    "        let routedIdx = u32(px.y - i32(routedWindow.control.y)) * u32(max(u.viewport.x, 1.0)) + u32(px.x);\n"
                    "        if (routedIdx < routedWindow.control.x) {\n"
                    "            let answer = routedPublication[routedIdx];\n"
                    "            if (answer.w >= 0.0) {\n"
                    "                if (answer.w <= 1e-5) { discard; }\n"
                    "                return answer;\n"
                    "            }\n"
                    "        }\n"
                    "    }\n";
                wgsl.replace(pos, anchor.size(), publish);
            }
```
*(Keep `anchor = "    var transmittance = 1.0;"` inside `VolumeRoutedTransport.inc:204` so `ecSetup` still extracts the full `setupBody` from `fn fs()`!)*

**Why this works and saves ~4–6 ms/frame across all 64 poses:**
- When `routed` is active (`workLimit > 0`), `ecSetup` pre-populates `ecPublication` with `vec4<f32>(0.0)` (`answer.w == 0.0 <= 1e-5 -> discard;`) for background/miss rays and `ecConsume` writes `vec4<f32>(rgb, alpha)` (`answer.w >= 0.0`) for volume rays.
- Thus in `fn fs(in: VolumeVSOut)`, **100% of pixels (`3,686,400 / 3,686,400`) exit on line 2 of `fs()`**, completely skipping `textureLoad(sceneDepthTex)`, both `u.invViewProj` $4\times 4$ matrix multiplies, perspective divides, `normalize`, and `rayAabbWorld`!
- And when `workLimit == 0` (Gate 2 refusal test), `ecSetup` returns immediately at the top with `ecPublication = vec4<f32>(0.0, 0.0, 0.0, -1.0)` (`answer.w == -1.0 < 0.0`), so `if (answer.w >= 0.0)` is false on all pixels and execution falls straight through to the normal `fs()` body!

---

### Task P7.2 — Restore Coalesced Row-Major `ecConsume` (`gid.x`) + Strip-Level Indirect Skip + Restore `p.local` to 4 Bindings
In `src/rendering/VolumeRoutedTransport.inc`:
1. **Restore `p.local` to 4 bindings (`0..3`)** (`ecWindow`, `ecRays`, `ecPublication`, `ecSingles`) and remove bindings `4..9` from `p.local` and `localEntries` (and revert the temporary `dd.requiredLimits = &limits` changes in `WgpuDevice.hpp` and `WebGpuContext.mm` if no shader exceeds 8 storage buffers anymore).
2. **In `routeArgs` (`VolumeRoutedTransport.inc:66`):**
   Keep `args` as `array<u32, 12>` (`48` bytes, 4 indirect dispatches):
   - Entries `0..2`: `s1ProducerMain` (`counts[0]`)
   - Entries `3..5`: `s2ProducerMain` (`counts[1]`)
   - Entries `6..8`: `fallbackProducerMain` (`counts[2]`)
   - Entries `9..11`: **Coalesced strip-level `ecConsume` dispatch**:
     ```wgsl
     let totalActiveTiles = counts[0] + counts[1] + counts[2];
     args[9] = select(0u, (ecWindow.control.x + 63u) / 64u, totalActiveTiles > 0u);
     args[10] = 1u;
     args[11] = 1u;
     ```
   - **Why this is the best of both worlds:**
     - When a strip has **zero active tiles** (`totalActiveTiles == 0u`, i.e., an Open-Sky strip in Regime 2 where `ecSetup` already wrote `ecPublication = vec4<f32>(0.0)`), `args[9] = 0u` so `ecConsume` launches **0 workgroups**!
     - When a strip has active tiles (`totalActiveTiles > 0u`), `ecConsume` launches in **linear row-major order (`@builtin(global_invocation_id) gid: vec3<u32>`, `let id = gid.x;`)**, so every 32-lane Apple M5 SIMD-group accesses `ecRays[id]`, `ecSingles[single * capacity + id]`, and `ecPublication[ecPixel(id)]` at **32 contiguous addresses (`100%` cache-line coalescing)**!
     - And inside `ecConsume`, any empty ray in a partially-active strip exits at line 1 before the loop (`if ((ray.counts.y | ray.counts.w) == 0u) { return; }`) because `ecSetup` already initialized `ecPublication[ecPixel(id)] = vec4<f32>(0.0)`!
3. **Update `ecConsume` (`VolumeRoutedTransport.inc:268`)** to use `@builtin(global_invocation_id) gid: vec3<u32>` with the fast `if (ray.counts.w == 0u)` non-overlapping loop:
   ```wgsl
   @compute @workgroup_size(64)
   fn ecConsume(@builtin(global_invocation_id) gid: vec3<u32>) {
       let id = gid.x;
       if (id >= ecWindow.control.x) { return; }
       let ray = ecRays[id];
       if ((ray.counts.x & 1u) == 0u) { return; }
       if ((ray.counts.x & 2u) != 0u) {
           ecPublication[ecPixel(id)] = vec4<f32>(0.0, 0.0, 0.0, -1.0);
           return;
       }
       if ((ray.counts.y | ray.counts.w) == 0u) {
           return;
       }
       var state = vec4<f32>(0.0, 0.0, 0.0, 1.0);
       let ro = ray.originT0.xyz;
       let rd = ray.dirT1.xyz;
       if (ray.counts.w == 0u) {
           for (var single = 0u; single < ray.counts.y; single++) {
               let sub = ecSingles[single * ecWindow.control.x + id];
               let l = sub.bounds.x;
               let r = sub.bounds.y;
               let h = r - l;
               if (h > 1e-5) {
                   let mid = ro + rd * (0.5 * (l + r));
                   state = ecFlow(state, ecEvalMember(sub.meta.x, mid), h);
                   if (state.w < 1e-3) { break; }
               }
           }
       } else {
           var single = 0u;
           var pair = 0u;
           loop {
               let hasSingle = single < ray.counts.y;
               let hasPair = pair < ray.counts.w;
               if (!hasSingle && !hasPair) { break; }
               var useSingle = false;
               if (hasSingle && hasPair) {
                   useSingle = ecSingles[single * ecWindow.control.x + id].bounds.x <=
                               ecPairs[(ray.counts.z + pair) * ecWindow.control.x + id].bounds.x;
               } else {
                   useSingle = hasSingle;
               }
               if (useSingle) {
                   let sub = ecSingles[single * ecWindow.control.x + id];
                   single++;
                   let l = sub.bounds.x;
                   let r = sub.bounds.y;
                   let h = r - l;
                   if (h > 1e-5) {
                       let mid = ro + rd * (0.5 * (l + r));
                       state = ecFlow(state, ecEvalMember(sub.meta.x, mid), h);
                   }
               } else {
                   let sub = ecPairs[(ray.counts.z + pair) * ecWindow.control.x + id];
                   pair++;
                   let l = sub.bounds.x;
                   let r = sub.bounds.y;
                   let h = r - l;
                   if (h > 1e-5) {
                       let mid = ro + rd * (0.5 * (l + r));
                       let f0 = ecEvalMember(sub.meta.x, mid);
                       let f1 = ecEvalMember(sub.meta.y, mid);
                       state = ecFlow(state, f0 + f1, h);
                   }
               }
               if (state.w < 1e-3) { break; }
           }
       }
       let alpha = clamp(1.0 - state.w, 0.0, 1.0);
       if (alpha <= 1e-5) {
           ecPublication[ecPixel(id)] = vec4<f32>(0.0);
       } else {
           ecPublication[ecPixel(id)] = vec4<f32>(state.xyz, alpha);
       }
   }
   ```
   And in `WebGpuRenderer.cpp` (`dispatchPass` for `ecConsume`), keep the single `wgpuComputePassEncoderDispatchWorkgroupsIndirect(pass, p.indirectArgs, 36);` call (replacing the 3 separate `s1`/`s2`/`fb` dispatches with 1 coalesced indirect dispatch at byte offset `36`)!

---

### Task P7.3 — Remove Redundant 4-AABB Loop in `ecSetup` & Add Exact Wave-Envelope Z-Pruning in `ecSetup` and `classifyTilesMain`
1. **In `ecSetup` (`VolumeRoutedTransport.inc:228`):**
   Replace the 4-instance AABB loop (`for (var m = 1u; m <= 4u; m++)`, which tested `instances[1..4].halfExtent = (195, 42, 105)` and culled 0 rays) with an **exact Z-wave envelope check** against the union wave Z-band `z in [-129.0, -81.0]` (`center.z = -105.0 ± 24.0`, since for all 4 curtains $|A_1| + |A_2| + |A_3| + w_{\text{half}} + \varepsilon_{\text{curv}} \le 22.5 < 24.0$, and `instances[0]` has `origin.z = -105.0`, `halfExtent.z = 105.0`):
   Wait — let's make it **100% general (not hardcoded to Northern Veil)** by computing the ray's Z-range over `[t0, t1]` or checking the 4 curtains in `classifyTilesMain`!
   First, in `ecSetup`, simply remove the redundant `for (var m = 1u; m <= 4u; m++)` loop so `ecSetup` stays fast and lean:
   ```wgsl
   ecRays[id].originT0 = vec4<f32>(ro, t0);
   ecRays[id].dirT1 = vec4<f32>(rd, t1);
   ecRays[id].counts = vec4<u32>(1u, 0u, (id / 256u) * 64u, 0u);
   ecPublication[ecPixel(id)] = vec4<f32>(0.0);
   ```
2. **In `classifyTilesMain` (`VolumeRoutedKernels.hpp:203–271`):**
   Inside `for (var m = 0u; m < 4u; m++)`, after computing the slab `[a, b]` against `cur.half`, check whether the ray's local `Z` interval on `[a, b]` intersects the curtain's wave-amplitude envelope `[-zEnv, +zEnv]`:
   ```wgsl
   let oz = ray.origin.z - cur.center.z;
   let dz = ray.dir.z;
   let zEnv = abs(cur.amps.x) + abs(cur.amps.y) + abs(cur.amps.z) + cur.limits.w + cur.curv.y;
   let z0 = oz + a * dz;
   let z1 = oz + b * dz;
   if (min(z0, z1) > zEnv || max(z0, z1) < -zEnv) {
       a = 1.0;
       b = 0.0;
   }
   ```
   And in the root-interval loop of `classifyTilesMain` (`VolumeRoutedKernels.hpp:233–262`):
   - Expand `myEnter` / `myExit` around each root crossing `r` by `rSpan` (`rootSpan`) so `classifyTilesMain`'s `[enter, exit]` interval is a true conservative superset of `roots()`'s `[r - rootSpan, r + rootSpan]` subpanels.
   - When `!(uxMax > uxMin)`, only mark `[a, b]` as active if `abs(p0) <= cur.limits.w + cur.curv.y` (instead of blindly setting `myEnter = a; myExit = b;` on every ray where `dx == 0`)!

---

### Task P7.4 — Build, Run 5-Gate Verification + 3 Sequential WebGPU CTest Suites, and Commit
1. Recompile `/private/tmp/ec-positive-e2e-verify/verify` and run it against `saves/Northern Veil/zone.ecform`.
2. Verify all 5 Gates pass (`max_channel_abs_diff = 0`, `rmse = 0.0`, `workLimit == 0` refusal identical to `current`, `0` refused pixels across 64 poses) and compare the new Dual-Regime timing against P5b (`13d150b84`) and P6 (`3f5d354c2`):
   - Expect **Regime 1 (`Poses 0..19, 51..63`)** to recover the full `-7.62 ms` coalesced `ecConsume` win from P5b **plus** an additional `-3 to -5 ms` from the top-of-`fs()` short-circuit (`Task P7.1`) and removing the 4-AABB `ecSetup` loop (`Task P7.3`)!
   - Expect **Regime 2 (`Poses 20..50`)** to drop sharply thanks to:
     1. Top-of-`fs()` short-circuit (`Task P7.1`, skipping `sceneDepthTex`, `u.invViewProj`, and `rayAabbWorld` on all `3.68M` pixels),
     2. Wave-envelope `zEnv` culling in `classifyTilesMain` (`Task P7.3`, turning rays that pass through the empty `78.5%` Z-region into `Empty` tiles), and
     3. Strip-level indirect skip in `routeArgs` (`Task P7.2`, launching `0` `ecConsume` workgroups on empty strips while keeping `100%` 32-wide SIMD coalescing on active strips).
3. Run the 3 WebGPU regression suites sequentially (one at a time, never parallel `ctest`):
   - `./build/debug/test_webgpu_volume_routed_transport`
   - `./build/debug/test_webgpu_renderer_regression`
   - `./build/debug/test_sdf_wgsl`
4. Persist updated telemetry to `docs/audits/data/positive_e2e_p7_verification.json` and write `docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_P7_COALESCED_FAST_FS_BENCHMARK_2026-10-31.md`.
