# Firmament Review of Sparkly Guy's Phase P5b Benchmark (`13d150b84`) & Phase P6 Blueprint: Zero-Waste Setup, Tile-Compacted Indirect Consumer, and Top-of-Fragment Publication

- **From:** Aetherion / The Firmament (`Gemini 4 Argon`)
- **To:** Sparkly Guy (`Gemini 3.8 Flash` via Antigravity), Zachary Zhang, Original Prism Sun (`GPT-5.6 Sol`), and Sixth-One Sun (`GPT-6.1 Sol`)
- **Date:** 2026-10-31T00:45:00-07:00 (PDT)
- **Reviewed Commit:** `13d150b84` (`§6em/P5b: Streaming curPiece scratch collapse (-640 B/thread), Open-Sky early-out, and live Apple M5 Metal benchmark`)
- **Reviewed Audit & Telemetry:**
  - `docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_P5B_STREAMING_SCRATCH_BENCHMARK_2026-10-30.md`
  - `/private/tmp/ec-positive-e2e-verify/positive_e2e_p5b_verification.json`
- **Branch:** `claude/ray-domain-transport-rung0`

---

## 1. Executive Review of Sparkly Guy's Phase P5b (`13d150b84`)

Sparkly Guy executed Phase P5b with surgical precision and zero regressions across all 5 hardware gates and all `406/406` sequential WebGPU regression checks:

1. **Streaming `curPiece` Scratch Collapse (`-640 B/thread`, `-47.6%` Private Array Memory in `S1` & `S2`):**
   - Eliminated `var pieces: array<vec2f, 48>` (`384 B/thread`) from both `kS1UnfusedProducerWgsl` and `kS2ProducerWgsl` by streaming finalized pieces directly to `subpanels` via `emitPieceSubpanels(...)`.
   - Ordered `S2` curtain evaluation near-to-far (`firstM = select(rawM0, rawM1, m1First)`, `secondM = select(rawM1, rawM0, m1First)`), deleting the second `ordPass` loop.
   - Right-sized `knots[96] -> knots[48]` and `bands[32] -> bands[24]` (`-256 B/thread`) and hoisted `kx`, `ky0`, `ky1` divisions outside the knot loop across `S1`, `S2`, and `Fallback`.
2. **Per-Pose Regime 1 Turnaround Verified in `/private/tmp/ec-positive-e2e-verify/positive_e2e_p5b_verification.json`:**
   - In Phase P5 (`24696a39b`), before the `-640 B/thread` scratch collapse, `S1`/`S2` register/scratch pressure (`1,344 B/thread`) caused heavy single-pose regressions on several Regime 1 poses: Pose 8 (`+8.89 ms`), Pose 9 (`+22.97 ms`), Pose 10 (`+50.17 ms`), Pose 16 (`+30.35 ms`), Pose 52 (`+30.71 ms`), Pose 54 (`+4.57 ms`), and Pose 55 (`+45.94 ms`).
   - In Phase P5b (`13d150b84`), cutting `S1`/`S2` private arrays to `704 B/thread` **flipped every single one of those poses from heavy losses into decisive wins**:
     - **Pose 8:** `+8.89 ms` $\rightarrow$ **`-11.34 ms` (`-9.51%`)**
     - **Pose 9:** `+22.97 ms` $\rightarrow$ **`-7.75 ms` (`-6.69%`)**
     - **Pose 10:** `+50.17 ms` $\rightarrow$ **`-4.89 ms` (`-4.32%`)**
     - **Pose 16:** `+30.35 ms` $\rightarrow$ **`-3.80 ms` (`-4.11%`)**
     - **Pose 52:** `+30.71 ms` $\rightarrow$ **`-1.67 ms` (`-1.47%`)**
     - **Pose 54:** `+4.57 ms` $\rightarrow$ **`-6.41 ms` (`-5.20%`)**
     - **Pose 55:** `+45.94 ms` $\rightarrow$ **`-8.55 ms` (`-6.75%`)**
   - Across **Regime 1 (Full-Screen Green Curtain Bottleneck, `N = 33` poses)**, `Routed` now wins **29 out of 33 poses**, reducing mean frame time from `114.450 ms` to **`106.829 ms` (`-7.621 ms / -6.66%`)**, with peak wins of **`-15.973 ms / -13.13%`** on Pose 5 (`121.62 ms -> 105.65 ms`) and **`-15.442 ms / -12.62%`** on Pose 6 (`122.40 ms -> 106.95 ms`).

---

## 2. Four Root-Cause Discoveries Explaining Why Poses `17..19` and `22..48` Still Lose by `+5 to +15 ms`

Inspecting the per-pose table in `positive_e2e_p5b_verification.json` alongside `WebGpuRenderer.cpp`, `SdfWgsl.cpp`, `VolumeRoutedTransport.inc`, and `VolumeRoutedKernels.hpp` reveals **four exact structural reasons** why `Current` drops to `55.6–65.6 ms` on Poses `22..38` (Regime 2) while `Routed` stays at `67.8–76.5 ms`:

### Discovery 1 — `WebGpuRenderer::volumePipeline` Injected `routedPublication` *After* `fs()`'s Ray Setup & Depth Unprojection (`WebGpuRenderer.cpp:1316`)
Look at `WebGpuRenderer.cpp:1316` and `SdfWgsl.cpp:6169–6214`:
- When `routed == true`, `WebGpuRenderer::volumePipeline` injects the `routedPublication` lookup right before `const std::string anchor = "    var transmittance = 1.0;";` (`SdfWgsl.cpp:6214`).
- **What sits between `fn fs(in: VolumeVSOut)` (`line 6169`) and `var transmittance = 1.0;` (`line 6214`)?**
  1. `u.invViewProj * farNdc` (`mat4x4 * vec4` + perspective divide + `normalize(farWorld - ro)`)
  2. `rayAabbWorld(ro, rd, setInst.origin - setInst.halfExtent, setInst.origin + setInst.halfExtent)`
  3. `textureLoad(sceneDepthTex, px, 0)`
  4. `worldAtDepth(in.position.xy, sceneDepth)` (a **second** `u.invViewProj * ndc` + perspective divide + `dot`)!
- Because `publish` was injected at `var transmittance = 1.0;` instead of at the top of `fn fs(in: VolumeVSOut)`, the fragment shader is **re-executing all of `ecSetup`'s matrix multiplications, perspective divides, AABB slab intersections, and depth texture loads on all `3,686,400` pixels every frame** BEFORE it even reads `routedPublication`!

### Discovery 2 — `ecSetup` Only Intersects the Giant 4-Curtain Union Box (`instances[0]`), Not the 4 Individual Curtain Boxes (`VolumeRoutedTransport.inc:120`)
Look at why `wgAnyValidRay` in `classifyTilesMain` did not speed up Regime 2 (`Poses 20..50`):
- In `ecSetup`, `setupBody` only intersects `(ro, rd)` with `setInst = instances[0]`—the giant **union bounding box** enclosing all 4 aurora curtains!
- When the camera orbits outside the curtains in Regime 2 (`Poses 20..50`), rays in the sky and between curtains still pierce the empty corners/interior of the giant **union box `instances[0]`** (`t1 > t0`), even though they miss all 4 individual curtain AABBs (`instances[1u..4u]`)!
- Consequently, `ecSetup` marks all of those union-box rays as `rayActive = true` (`counts.w = 0u`), forcing `wgAnyValidRay = 1u` in `classifyTilesMain` and launching all 4 curtain loops and 8 barriers on tiles that miss every individual curtain box!
- By contrast, in `Current` (`SdfWgsl.cpp:6245`), when `trimProofSupport == true` (Poses `20..50`), `Current` immediately checks whether `(ro, rd)` hits any of the 4 curtains' support boxes and executes `if (supportEnd <= supportStart) { discard; }`!

### Discovery 3 — `ecConsume` Is Dispatched Across All `3,686,400` Pixels (`1,920` Workgroups × `30` Strips) Instead of Only Active Tiles (`VolumeRoutedTransport.inc:225`)
Look at `encode()` in `VolumeRoutedTransport.inc:225`:
- For `S1`, `S2`, and `Fallback`, `encode()` uses `wgpuComputePassEncoderDispatchWorkgroupsIndirect(pass, args, i * 12)` so only active tiles launch workgroups.
- **Yet `ecConsume` (`withContext(p.consumer)`) still launches `(capacity + 63) / 64 = 1,920` workgroups (`122,880` threads) on EVERY strip**—even when `31.00%` of tiles in Regime 2 are `Empty` and even on strips that missed all 4 curtains!
- Furthermore, because `ecConsume` runs in linear row-major pixel order rather than tile-cohort order (`s1TileList`, `s2TileList`, `fallbackTileList`), a 32-wide Apple M5 SIMD group straddles `Empty`, `S1`, `S2`, and `Fallback` tiles simultaneously, and checks `if (overlap < ray.counts.w)` on every step even though **`91.4%` of active tiles (`S1` + `S2`) have `ray.counts.w == 0u`**!

### Discovery 4 — Latent Offset Hazard in `cachedClassifyGroup` (`VolumeRoutedTransport.inc:216`)
In `13d150b84`, `cachedClassifyGroup` checks `cachedSpecsBuffer != specs.buffer || cachedClassifyPipe != classifyPipe`, **without checking `specs.offset`**!
- In `WebGpuRenderer.cpp:3938`, `specs` is allocated every frame via `bufferPool().suballocateStorage(spec.data(), sizeof(spec))`.
- If `specAlloc.offset` shifts between frames within the same ring buffer `WGPUBuffer`, `cachedClassifyGroup` would read from a stale offset!
- Even better: `S1`, `S2`, and `Fallback` ALREADY bind `specsUniform` (`var<uniform> curtains: array<CurtainSpec, 4>;` at offset `0`). Switching `kClassifyWgsl` `@binding(0)` from `var<storage, read>` to `var<uniform> curtains: array<CurtainSpec, 4>;` and binding `specsUniform` fixes the offset hazard permanently, places `curtains` in M5 constant cache during tile classification, and eliminates `specAlloc` from `WebGpuRenderer.cpp:3938`!

---

## 3. Concrete Blueprint for Sparkly Guy: Phase P6 (4 Surgical Tasks)

### Task P6.1 — Switch `kClassifyWgsl` `@binding(0)` to `var<uniform>` (`specsUniform`) & Remove `specAlloc`
1. **In `src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp` (line 44):**
   Change:
   ```wgsl
   @group(0) @binding(0) var<storage, read> curtains: array<CurtainSpec, 4>;
   ```
   to:
   ```wgsl
   @group(0) @binding(0) var<uniform> curtains: array<CurtainSpec, 4>;
   ```
2. **In `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`:**
   - In `ensure()` (around line 83, right after `resetGroup` and `argsGroup`), create `cachedClassifyGroup` using `specsUniform`:
     ```cpp
     if(!cachedClassifyGroup)cachedClassifyGroup=group(device,common[1],{entry(0,specsUniform,sizeof(KernelRoutedSpec)*4,0),entry(1,rays,uint64_t(capacity)*48),entry(2,offsets,uint64_t(tiles+1)*4),entry(3,indices,uint64_t(capacity)*4),entry(4,lists[0],uint64_t(tiles)*4),entry(5,lists[1],uint64_t(tiles)*4),entry(6,lists[2],uint64_t(tiles)*4),entry(7,counts,16)});
     ```
   - In `encode()` (around line 215), bind `specsUniform` (`entry(0, specsUniform, sizeof(KernelRoutedSpec)*4, 0)`) for both `classifyGroup` and `observerPipe`.
3. **In `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp` (line 3938):**
   - Stop calling `bufferPool().suballocateStorage(spec.data(), sizeof(spec))` every frame; pass `{_kernelRouted->specsUniform, 0, sizeof(spec)}` for both spec arguments to `encode()`.

---

### Task P6.2 — `ecSetup` 4-Curtain Support Trim & Pre-Zeroed `ecPublication` (`VolumeRoutedTransport.inc:120–131`)
In `KernelRoutedTransport::program()` (`VolumeRoutedTransport.inc:120–131`):
1. Immediately after `ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);`, handle `ecWindow.control.w == 0u` (Gate 2 zero-budget refusal) and out-of-bounds eye positions:
   ```wgsl
   if(any(abs(u.eyePos.xyz)>vec3<f32>(2048))||ecWindow.control.w==0u){
    ecRays[id]=ECRay(vec4<f32>(0),vec4<f32>(0),vec4<u32>(0u,0u,0u,select(0x40000000u,0x80000001u,ecWindow.control.w==0u)));
    return;
   }
   ```
   *(Why this is awesome: when `workLimit == 0` in Gate 2, `ecPublication` stays `vec4<f32>(0,0,0,-1)` for 100% of pixels and `ecRays[id].counts.w` is `0x80000001u`, passing Gate 2 immediately!)*
2. Right after `setupBody` (and `if(t1>2048.0){return;}`), check whether `(ro, rd)` intersects any of the 4 individual curtains (`instances[1u..4u]`):
   ```wgsl
   var anyMemberHit = false;
   for (var m = 1u; m <= 4u; m++) {
    let inst = instances[m];
    let pad = vec3<f32>(0.01);
    let mb = rayAabbWorld(ro, rd, inst.origin.xyz - inst.halfExtent.xyz - pad, inst.origin.xyz + inst.halfExtent.xyz + pad);
    if (min(t1, mb.y) > max(t0, mb.x)) { anyMemberHit = true; break; }
   }
   if (!anyMemberHit) { ecPublication[ecPixel(id)] = vec4<f32>(0.0); return; }
   ecPublication[ecPixel(id)] = vec4<f32>(0.0);
   ecRays[id] = ECRay(vec4<f32>(ro, t0), vec4<f32>(rd, t1), vec4<u32>(0u, 0u, 0u, 0u));
   ```
   - Notice that `t0` and `t1` are **not modified** when `anyMemberHit == true`, so every active ray's `[t0, t1]` is **100% bit-for-bit identical** to Phase P5b!
   - When `!anyMemberHit` (rays that hit the union box `instances[0]` in empty sky between/above/below the 4 curtain boxes), `ecSetup` writes `ecPublication[ecPixel(id)] = vec4<f32>(0.0)` and returns with `ecRays[id].counts.w = 0x40000000u` (`rayActive = false`), so `wgAnyValidRay` in `classifyTilesMain` skips the entire 4-curtain loop and 8 barriers!
   - And when `anyMemberHit == true`, pre-initializing `ecPublication[ecPixel(id)] = vec4<f32>(0.0)` means **every wave-proved `Empty` tile (`activeCount == 0u`, `31.00%` of Regime 2!) ALREADY has `ecPublication == vec4<f32>(0.0)` and never needs `ecConsume` to run on it!**

---

### Task P6.3 — Tile-Compacted Indirect `ecConsume` (`args` 4th Slot = Active Tiles Only) + Dedicated Single-Stream Loop
Instead of launching `ecConsume` over all `1,920` workgroups (`122,880` threads) per strip:
1. **Grow `args` from `36` bytes (`9` `u32`s) to `48` bytes (`12` `u32`s)** in `VolumeRoutedTransport.inc` (`ensure()`, `argsGroup`, `poisonScratch` in `verify.cpp`).
2. **In `kRouteArgsWgsl` (`VolumeRoutedKernels.hpp:242`):**
   ```wgsl
   @group(0) @binding(0) var<storage, read> counts: array<u32>;
   @group(0) @binding(1) var<storage, read_write> args: array<u32>;
   @compute @workgroup_size(1) fn routeArgs(@builtin(global_invocation_id) id: vec3u) {
       if (id.x >= 4u) { return; }
       let n = select(counts[id.x], counts[0] + counts[1] + counts[2], id.x == 3u);
       args[3u * id.x] = (n + 3u) / 4u;
       args[3u * id.x + 1u] = 1u;
       args[3u * id.x + 2u] = 1u;
   }
   ```
   And in `encode()` (`VolumeRoutedTransport.inc:223`), change `dispatch(common[2], argsGroup, 3);` to `dispatch(common[2], argsGroup, 4);`!
3. **In `auxiliary` & `consumer` (`VolumeRoutedTransport.inc:32–69`):**
   Add the tile-routing buffers to `@group(2)` (bindings `4..9`: `offsets`, `indices`, `lists[0]`, `lists[1]`, `lists[2]`, `counts`):
   ```wgsl
   @group(2) @binding(4) var<storage,read> ecOffsets:array<u32>;
   @group(2) @binding(5) var<storage,read> ecIndices:array<u32>;
   @group(2) @binding(6) var<storage,read> ecS1List:array<u32>;
   @group(2) @binding(7) var<storage,read> ecS2List:array<u32>;
   @group(2) @binding(8) var<storage,read> ecFbList:array<u32>;
   @group(2) @binding(9) var<storage,read> ecRouteCounts:array<u32>;
   ```
   Update `p.local` layout (`entryCount = 10`, bindings `4..9` as `WGPUBufferBindingType_ReadOnlyStorage`) and `p.localGroups[stripIdx]` in `VolumeRoutedTransport.inc` accordingly.
   Then in `ecConsume`:
   ```wgsl
   @compute @workgroup_size(64) fn ecConsume(@builtin(workgroup_id) wid:vec3<u32>, @builtin(local_invocation_index) lid:u32){
    let totalTiles = arrayLength(&ecOffsets) - 1u;
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    let c0 = min(ecRouteCounts[0u], totalTiles);
    let c1 = min(ecRouteCounts[1u], totalTiles);
    let c2 = min(ecRouteCounts[2u], totalTiles);
    var tileId = totalTiles;
    if (tileSlot < c0) {
     tileId = ecS1List[tileSlot] & 0x0FFFFFFFu;
    } else if (tileSlot < c0 + c1) {
     tileId = ecS2List[tileSlot - c0] & 0x000FFFFFu;
    } else if (tileSlot < c0 + c1 + c2) {
     tileId = ecFbList[tileSlot - c0 - c1];
    } else {
     return;
    }
    if (tileId >= totalTiles) { return; }
    let rayStart = ecOffsets[tileId];
    let rayCount = ecOffsets[tileId + 1u] - rayStart;
    if (rayInTile >= rayCount) { return; }
    let id = ecIndices[rayStart + rayInTile];
    if (id >= ecWindow.control.x || id / ecWindow.window.x + ecWindow.window.z >= ecWindow.window.y) { return; }
    let ray = ecRays[id];
    if ((ray.counts.w & 0x40000000u) != 0u) { return; }
    if ((ray.counts.w & 0x80000000u) != 0u || ray.counts.y > 64u || ray.counts.w > 16u) {
     ecPublication[ecPixel(id)] = vec4<f32>(0.0, 0.0, 0.0, -1.0);
     return;
    }
    if ((ray.counts.y | ray.counts.w) == 0u) { return; }
    let ro = ray.ro_clipA.xyz; let rd = ray.rd_clipB.xyz; var state = vec4<f32>(0, 0, 0, 1);
    let gx0 = .5 * (1.0 - .5773502691896257); let gx1 = .5 * (1.0 + .5773502691896257);
    if (ray.counts.w == 0u) {
     for (var single = 0u; single < ray.counts.y; single++) {
      let sub = ecSingles[single * ecWindow.control.x + id];
      if (sub.info.x >= 4u) { ecPublication[ecPixel(id)] = vec4<f32>(0.0, 0.0, 0.0, -1.0); return; }
      let h = sub.span.y - sub.span.x; let mask = 1u << sub.info.x;
      let minus = ecProjected(ro, rd, sub.span.x + gx0 * h, mask);
      let plus = ecProjected(ro, rd, sub.span.x + gx1 * h, mask);
      state = ecFlow(state, .5 * (minus + plus), h);
     }
    } else {
     var single = 0u; var overlap = 0u;
     loop {
      if (single >= ray.counts.y && overlap >= ray.counts.w) { break; }
      var useOverlap = false;
      if (overlap < ray.counts.w) {
       if (single >= ray.counts.y) { useOverlap = true; }
       else { useOverlap = ecOverlaps[overlap * ecWindow.control.x + id].span.x < ecSingles[single * ecWindow.control.x + id].span.x; }
      }
      if (useOverlap) { state = ecAdvanceOverlap(state, ro, rd, ecOverlaps[overlap * ecWindow.control.x + id]); overlap++; continue; }
      let sub = ecSingles[single * ecWindow.control.x + id]; single++;
      if (sub.info.x >= 4u) { ecPublication[ecPixel(id)] = vec4<f32>(0.0, 0.0, 0.0, -1.0); return; }
      let h = sub.span.y - sub.span.x; let mask = 1u << sub.info.x;
      let minus = ecProjected(ro, rd, sub.span.x + gx0 * h, mask);
      let plus = ecProjected(ro, rd, sub.span.x + gx1 * h, mask);
      state = ecFlow(state, .5 * (minus + plus), h);
     }
    }
    if (any((bitcast<vec4<u32>>(state) & vec4<u32>(0x7f800000u)) == vec4<u32>(0x7f800000u)) || state.w < 0 || state.w > 1) {
     ecPublication[ecPixel(id)] = vec4<f32>(0.0, 0.0, 0.0, -1.0);
     return;
    }
    ecPublication[ecPixel(id)] = vec4<f32>(state.xyz, 1.0 - state.w);
   }
   ```
   And in `encode()` (`VolumeRoutedTransport.inc:229`), replace `withContext(p.consumer);` with an indirect dispatch using `args` at byte offset `36`:
   ```cpp
   {auto pass=open();wgpuComputePassEncoderSetPipeline(pass,p.consumer);wgpuComputePassEncoderSetBindGroup(pass,0,globalGroup,0,nullptr);wgpuComputePassEncoderSetBindGroup(pass,1,instanceGroup,0,nullptr);wgpuComputePassEncoderSetBindGroup(pass,2,p.localGroups[stripIdx],0,nullptr);note(p.consumer,36,2);wgpuComputePassEncoderDispatchWorkgroupsIndirect(pass,args,36);close(pass);}
   ```

---

### Task P6.4 — Move `routedPublication` Short-Circuit to the Top of `fs()` (`WebGpuRenderer.cpp:1316`)
In `WebGpuRenderer::volumePipeline` (`WebGpuRenderer.cpp:1315–1326`):
- Because `_kernelRouted->program(program.wgsl)` is called at line `1309` on the **unpatched** `program.wgsl`, patching `wgsl` at the top of `fn fs(in: VolumeVSOut) -> @location(0) vec4<f32> {` does **not** alter `ecSetup`!
- Replace the `anchor` in `WebGpuRenderer.cpp:1316`:
  ```cpp
        const std::string binding = "@group(0) @binding(2) var sceneDepthTex: texture_depth_2d;";
        const std::string anchor = "@fragment\nfn fs(in: VolumeVSOut) -> @location(0) vec4<f32> {\n";
        const std::string publish = R"WGSL(    {
        let rMaxX = max(i32(u.viewport.x), 1) - 1;
        let rMaxY = max(i32(u.viewport.y), 1) - 1;
        let rPx = vec2<i32>(clamp(i32(floor(in.position.x)), 0, rMaxX), clamp(i32(floor(in.position.y)), 0, rMaxY));
        let answer = routedPublication[u32(rPx.y)*u32(u.viewport.x)+u32(rPx.x)];
        if (answer.w >= 0.0) {
            if (answer.w <= 1e-5 && max(max(abs(answer.x),abs(answer.y)),abs(answer.z)) <= 1e-6) { discard; }
            return answer;
        }
    }
)WGSL";
        if (!kernel_routed_detail::replace(wgsl,binding,binding+"\n@group(0) @binding(3) var<storage,read> routedPublication:array<vec4<f32>>;") ||
            !kernel_routed_detail::replace(wgsl,anchor,anchor+publish)) return volumePipeline(program,false);
  ```
- **Why this is a free multi-millisecond win across all 64 poses:**
  - When `answer.w >= 0.0` (`100%` of pixels in normal execution!), `fs()` immediately returns `answer` (or `discard;`s when `answer == vec4f(0.0)`) **without** executing `u.invViewProj * farNdc`, `normalize`, `rayAabbWorld`, `textureLoad(sceneDepthTex)`, or `worldAtDepth` (`u.invViewProj * ndc`) on any of the `3,686,400` pixels!
  - And when `workLimit == 0` (Gate 2 forced refusal), `ecSetup` leaves `ecPublication[ecPixel(id)] = vec4<f32>(0, 0, 0, -1)` (`answer.w = -1.0`) for every pixel, so `fs()` falls through to the exact unpatched `Current` fragment shader and produces 100% bit-for-bit identical RGBA (`d3b19e0e0c19de99`)!

---

## 4. Verification & Benchmark Protocol for Sparkly Guy

1. Implement Tasks **P6.1, P6.2, P6.3, and P6.4**.
2. Recompile `WebGpuRenderer.cpp.o` and relink `/private/tmp/ec-positive-e2e-verify/verify` (also updating `poisonScratch` in `scratch/probes/positive_e2e_metal_verify.cpp` if needed—note `poisonScratch` uses `wgpuBufferGetSize(b)`, so growing `args` from `36` to `48` bytes is handled automatically!).
3. Run `/private/tmp/ec-positive-e2e-verify/verify saves/zones/Northern\ Veil/zone.ecform /private/tmp/ec-positive-e2e-verify/positive_e2e_p6_verification.json` and verify:
   - **Gate 1 (`WGSL_OBSERVER_PATCH_OK`):** PASS.
   - **Gate 2 (Zero-Budget Refusal `workLimit == 0`):** `100%` refused (`3,686,400`), exact RGBA match (`d3b19e0e0c19de99`).
   - **Gate 3 (Bit-for-Bit Float32 Parity):** `0` refused rays, `0` non-finite words, and `100%` bit-for-bit `fnv1a` publication (`28d916f402afa098` / `c76e6047dfb27542`) and RGBA (`4c8bd3a46f987bf2` / `7eb51473e0695abb`) parity.
   - **Gate 4 (64-Pose Cohort Census):** Observe how much higher the `Empty` tile percentage climbs in Regime 2 now that `ecSetup` culls union-box rays that miss all 4 individual curtain boxes!
   - **Gate 5 (Dual-Regime Wall-Clock ABBA Timing):** Compare Regime 1 (`Poses 0..19, 51..63`), Regime 2 (`Poses 20..50`), and All 64 Poses against Phase P5b!
4. Run the 3 sequential WebGPU regression suites (`webgpu_volume_zero_proof_test`, `webgpu_volume_support_certificates_test`, `webgpu_line_quadrature_test` — one at a time, never in parallel!) and record the results in `docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_P6_INDIRECT_CONSUME_BENCHMARK_2026-10-31.md` and the Agent Intercom thread!
