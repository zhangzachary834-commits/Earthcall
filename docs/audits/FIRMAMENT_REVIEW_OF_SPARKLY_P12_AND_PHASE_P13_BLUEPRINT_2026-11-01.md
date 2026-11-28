# Firmament Review of Sparkly Guy's Phase P12 & Phase P13 Blueprint: Fail-Closed Host `encode()` Contract, Consumer SIMD-Convergent Evaluator Deduplication, and 3-Checkpoint-View Benchmark

**From:** Aetherion / The Firmament (`Gemini 4 Argon`)  
**To:** Zachary Zhang, Sparkly Guy (`Gemini 3.8 Flash` in Antigravity), Sixth-One Sun (`Codex / GPT-6.1 Sol`), & Astra (`Codex / GPT-6`)  
**Date:** 2026-11-01T17:30:00-08:00 (PST)  
**Branch:** `claude/ray-domain-transport-rung0`  
**Reviewed Commits:**
- `91a2ffe20` (`Phase P12: Close three structural corrective gates with 8/8 native Metal passes & dual-regime net wins`)
- `7f06ad55e` (`docs(audit): benchmark Phase P11 vs P12 on Metal with Chrome closed (-12.5% dual-regime win)`)

---

## 1. Executive Summary & Audit Verdict on Sparkly Guy's Phase P12

Sparkly Guy (`Gemini 3.8 Flash`) executed the Phase P12 blueprint (`8d2b83df0`) with precision across both production source and native Apple M5 Metal verification:

1. **All Three Post-P11 Structural Corrective Gates Closed & Verified (`8/8` Native Metal Gates + `406/406` WebGPU Regression Checks + `3/3` CPU Structural Adversaries):**
   - **Gate 1 (`[P1]` Empty-Strip Publication Ownership in `VolumeRoutedTransport.inc:192` & `:53`):**
     - `ecSetup` now unconditionally initializes `ecPublication[ecPixel(id)] = vec4<f32>(0.0);` for all ordinary union-box-hit rays (`t1 <= 2048.0`), while `ecConsume` returns immediately on zero work (`if ((ray.counts.y | ray.counts.w) == 0u) { return; }`).
     - Every in-frame pixel has a current-frame writer even when `totalActiveTiles == 0u` (`args[9u] == 0u` skips `ecConsume`), while every empty ray in the viewport still performs **strictly 1 physical write** to `ecPublication`.
     - **Native Metal Gate 6 Witness:** Pre-seeded `k->publication` after lit Pose 0 with finite non-zero values `(0.75, 0.50, 0.25, 0.80)`, then rendered Sixth-One Sun's union-box-hit / wave-empty scene (`zEnv = 1.3 < 10.0`). Readback verified `3,686,400 / 3,686,400` accepted pixels, `0` non-empty pixels (`100% vec4(0.0)`), `0` surviving poison pixels, and bit-exact RGBA parity with `Current` (`52b186bb0e2c2325 == 52b186bb0e2c2325`).
   - **Gate 2 (`[P1]` Complete 16-Ray Tile Hull & Per-Ray `firstMemberMaxS` Chronology Guard in `VolumeRoutedKernels.hpp:148` & `:601`):**
     - `classifyTilesMain` now uses `continue;` instead of `break;` on near-parallel rays (`abs(dx) < 1e-6`), and `s2ProducerMain` tracks `firstMemberMaxS` across `memberIdx == 0u` (`firstM`) and immediately refuses (`reason |= 64u`) if any active interval of `secondM` has `l < firstMemberMaxS`.
     - **CPU & Native Metal Gate 7 Witnesses:** In `ray_transport_p1_p11_near_parallel_case.cpp.inc`, all 12 overlapping rays are refused by `s2ProducerMain` (`s2_reason == 64`, `accepted_overlap_rays == 0`) and the tile routes to `Fallback` (`serial_cohort == 3`, `cooperative_cohort == 3`). On native Metal at `1281x1440` (center column `x = 640`, `dx = 0`), `fb_tiles = 6099`, `refused_rays = 0`, and RGBA matches `Current` bit-for-bit (`3bf0701125d52425 == 3bf0701125d52425`).
   - **Gate 3 (`[P2]` Dynamic Strip Allocation & Admission Guard in `VolumeRoutedTransport.hpp` & `VolumeRoutedTransport.inc`):**
     - Replaced fixed 64-entry `std::array` bind-group tables with `std::vector<WGPUBindGroup>`, removed the `maxStrips = 64` clamp in `ensure()`, and added the `(height + stripRows - 1) / stripRows > maxStrips` guard in `encode()`.
     - **Native Metal Gate 8 Witness:** Verified `1024x3072` (`64` strips), `1024x3120` (`65` strips), and `1024x4096` (`86` strips) with finite publication poisoning (`0` surviving poison, `100%` accepted pixels, `0` validation errors), followed by resize-back to `2560x1440` (`30` strips) with bit-exact Pose 0 RGBA parity (`d34c77bf8a341c6c == d34c77bf8a341c6c`).

2. **Quiet-Conditions (Chrome Closed) P11 vs P12 Head-to-Head Benchmark (`7f06ad55e`):**
   - Closing Chrome freed `2.4 GB` of unified memory and eliminated external compositor contention.
   - Across the 4-block completion-fenced ABBA benchmark on `Northern Veil` (`2560x1440`, 64 poses), **P12 matches P11's relative performance to within `0.16%` while closing all three correctness bugs**:

| Metric / Workload | P11 Current (ms) | P11 Routed (ms) | P11 Delta vs Current | P12 Current (ms) | P12 Routed (ms) | P12 Delta vs Current |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **All 64 Canonical Poses** | `70.739` | `61.848` | **`-8.891 ms (-12.57%)`** | `75.402` | `66.048` | **`-9.354 ms (-12.41%)`** |
| **Regime 1: Full-Screen Green (33 poses)** | `86.749` | `72.487` | **`-14.262 ms (-16.44%)`** | `92.547` | `77.546` | **`-15.001 ms (-16.21%)`** |
| **Regime 2: Open Sky (31 poses)** | `53.696` | `50.523` | **`-3.173 ms (-5.91%)`** | `57.151` | `53.808` | **`-3.342 ms (-5.85%)`** |
| **Pose Win Rate** | — | — | **`62 / 64 (96.9%)`** | — | — | **`61 / 64 (95.3%)`** |
| **Native Metal Correctness Gates** | — | — | `5 / 8` *(Fails 6, 7, 8)* | — | — | **`8 / 8 (100% PASS)`** |

---

## 2. Two Remaining Structural Hygiene Closures Identified in P12 Review

During our deep audit of the full branch diff (`074f4075a..7f06ad55e`), we identified two small structural hygiene items to close:

### 2.1. Gate P13.1 (`[P2]` Host `WebGpuRenderer::flushVolumeComposite()` Must Check `encode()` Return Value and Fail Closed to `Current`)
Look at `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp:2088–2104`:
```cpp
if(pipeline->routed){
    if(_pass){wgpuRenderPassEncoderEnd(_pass);wgpuRenderPassEncoderRelease(_pass);_pass=nullptr;_boundPipeline=nullptr;}
    const auto& spec=routedSpecs.at(pipeline);
    if(_kernelRouted->specsUniform)wgpuQueueWriteBuffer(_queue,_kernelRouted->specsUniform,0,spec.data(),sizeof(spec));
    auto* transport=_kernelRouted->program(pipeline->routedSource);
    if(transport)_kernelRouted->encode(*transport,_encoder,
        {globalAlloc.buffer,globalAlloc.offset,globalAlloc.size},{paramBuffer,paramOffset,bytes},
        {instanceAlloc.buffer,instanceAlloc.offset,instanceAlloc.size},_depthView,{_kernelRouted->specsUniform,0,sizeof(spec)},{_kernelRouted->specsUniform,0,sizeof(spec)},
        [&](const void* data,size_t size){const auto a=bufferPool().suballocateUniform(data,size);return KernelRoutedRange{a.buffer,a.offset,a.size};});
    ++mutableFrameStats().volumeRoutedSets;mutableFrameStats().volumeRoutedPixels+=_depthW*_depthH;
    mutableFrameStats().volumeRoutedWorkingSetBytes=uint64_t(_depthW)*_depthH*16+uint64_t(_kernelRouted->capacity)*(48+64*16+16*16+4)+uint64_t(_kernelRouted->tiles)*16+56;
}
```
- **Why this matters:** In P12.3, we added admission guards to `KernelRoutedTransport::encode()` (`if ((height + stripRows - 1) / stripRows > maxStrips) return false;` and `if (!p.localGroups[stripIdx]) return false;`). However, `WebGpuRenderer::flushVolumeComposite()` currently ignores the `bool` returned by `encode()`! If `!transport` or `!_kernelRouted->encode(...)`, no compute passes are encoded into `_encoder`, so `_kernelRouted->publication` is never written for the current frame—yet `flushVolumeComposite()` still increments `volumeRoutedSets` and binds the routed `pipeline->pipe`, which would read stale `routedPublication` from the previous frame!
- **Exact Repair:**
  In `WebGpuRenderer::flushVolumeComposite()`, capture `const bool encodedOk = transport && _kernelRouted->encode(...);`.
  - If `encodedOk` is `true`, increment `volumeRoutedSets`, `volumeRoutedPixels`, and `volumeRoutedWorkingSetBytes` as today.
  - If `encodedOk` is `false`, fail closed: fill `_kernelRouted->publication` (`uint64_t(_depthW) * _depthH * 16` bytes) with `0xFF` bytes (negative NaN / refusal sentinel `< 0.0` in `.w` via `wgpuQueueWriteBuffer` or `wgpuCommandEncoderClearBuffer` / fallback pipeline) AND increment `++mutableFrameStats().volumeProgramRefusals; mutableFrameStats().volumeLastProgramRefusal = "routed transport encode refused; Current retained";` without incrementing `volumeRoutedSets`!

### 2.2. Gate P13.2 (`[P3]` Guard Post-Loop Subpanel Flush in `s2ProducerMain` on `reason == 0u`)
Look at `src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp:615–618`:
```wgsl
        for (var p = 0u; p < count; p++) {
            emitPieceSubpanels(pieces[p], member, subBase, &singles, &reason);
            if (reason != 0u) { break; }
        }
```
When the new P12.2 chronology guard (`if (memberIdx == 1u && l < firstMemberMaxS) { reason |= 64u; break; }`) breaks out of the `for (var i = 1u; i < nb; i++)` loop with `count > 0u`, the post-loop `for (var p = 0u; p < count; p++)` executes one wasted `emitPieceSubpanels(pieces[0], ...)` call before hitting `if (reason != 0u) { break; }`.
- **Exact Repair:** Change `if (reason != 0u) { break; }` to appear **before** `for (var p = 0u; p < count; p++)` (matching `positive_e2e_cpu_verify.cpp:938`) so an already-refused ray immediately exits `s2ProducerMain`.

---

## 3. Phase P13 Performance & Verification Discovery: Why `ecConsume` Diverges on 4 Identical Curtain Evaluators & What Happens in Zach's 3 Checkpoint Views

To help Zach cross the **Northern Veil `28–30 FPS` (`~33.3–35.7 ms/frame`) incremental merge gate** (`docs/Agenda/Tasks/For Zach/Person Verification List.md`), we built a CPU view-census and WGSL inspection probe across:
1. The 64 canonical poses (`Canonical_Pose0_R1`, `Canonical_Pose32_R2`),
2. Zach's two live captured camera views (`Live_23FPS_View_A` at `[-6.75, 2.40, 3.71]` and `Live_Parked_View` at `[-14.06, 5.85, 0.83]`), and
3. Zach's 3 Northern Veil checkpoint views (`Spawn_Dais_Diag`, `Below_Veils_Straight_Up`, and `Inside_Veil_Center`).

### 3.1. Route & Work Census Across Canonical vs. Live Checkpoint Views (`Northern Veil`)

| View Name | Camera `eye` | Empty Tiles | `S1` Tiles | `Ordered-S2` Tiles | `Fallback` Tiles | Mean Producer Work / Active-Tile Ray | Mean `singles` / Active Ray | Mean `overlaps` / Active Ray | Refused Rays |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **`Canonical_Pose0_R1`** | `(0.0, 50.0, 75.0)` | `0.0%` | `48.5%` | `40.5%` | `10.9%` | `79.5` | `7.6` | `0.06` | `0` |
| **`Canonical_Pose32_R2`** | `(0.0, 50.0, 91.0)` | `31.3%` | `53.1%` | `11.6%` | `4.0%` | `61.3` | `5.6` | `0.05` | `0` |
| **`Live_23FPS_View_A`** | `(-6.75, 2.40, 3.71)` | `27.6%` | `35.8%` | `26.2%` | `10.4%` | `79.2` | `6.0` | `0.02` | `0` |
| **`Live_Parked_View`** | `(-14.06, 5.85, 0.83)` | `22.5%` | `36.9%` | `29.9%` | `10.7%` | `80.2` | `6.2` | `0.03` | `0` |
| **`Spawn_Dais_Diag` (View 2)** | `(0.0, 1.30, 0.0)` | `23.8%` | `38.9%` | `26.1%` | `11.2%` | `78.6` | `5.9` | `0.02` | `0` |
| **`Below_Veils_Up` (View 3)** | `(12.5, 0.0, 106.5)` | `70.9%` | `23.1%` | `3.8%` | `2.3%` | `47.9` | `6.4` | `0.06` | `0` |
| **`Inside_Veil_Center` (View 1)** | `(12.5, 67.0, 39.0)` | `0.0%` | `7.1%` | `50.9%` | `42.0%` | `127.3` | `7.9` | `0.04` | `0` |

### 3.2. Major Discovery in `ecConsume`: 4-Way SIMD Branch Divergence Over 4 Character-for-Character Identical `volumeShared_i_G0` + `cnoise3` Evaluators!

Look at the table above:
- Across **every single view** (canonical and live), **`> 99.2%` of all subpanels integrated by `ecConsume` are single-curtain `ecSingles` (`useOverlap == false`, `avgSub = 5.6–7.9` vs `avgOver = 0.02–0.06`)**!
- Now look at how `VolumeRoutedTransport.inc:198` generates `ecProjected` inside `ecConsume`:
  ```cpp
  for(unsigned i=0;i<4;++i){
   const auto n=std::to_string(i);
   sampling+="if((mask&"+std::to_string(1u<<i)+"u)!=0u){g_instIdx="+std::to_string(i+1)+"u;let p=world-instances["+std::to_string(i+1)+"u].origin.xyz;"
    +(source.find("fn volumeShared_"+n+"_Eval(")!=std::string::npos?"volumeShared_"+n+"_Eval(p);":"")
    +"let D=volumeDensityEval_"+n+"(p);if(D>0.0){let X=max(volumeExtinctionEval_"+n+"(p,D),1e-6);var E=vec3<f32>(0);if(!VOLUME_EMISSION_READS_OMEGA_"+n+"||s>1e-8){E=max(volumeEmissionEval_"+n+"(p,wo),vec3<f32>(0));}result+=vec4<f32>(E,X);}}\n";
  }
  ```
- When we dumped the exact generated WGSL for `Northern Veil` (`1,163` lines), we found:
  - `volumeShared_0_G0(p)`, `volumeShared_1_G0(p)`, `volumeShared_2_G0(p)`, and `volumeShared_3_G0(p)` are **100% character-for-character IDENTICAL**! Every single one of them reads `P.v[instances[g_instIdx].paramOffset + 0..37]` using `g_instIdx`, evaluates 5 `sin`/`cos` harmonics, and calls the 75-line 3D Perlin noise function `cnoise3(p)`!
  - `volumeDensityEval_0..3(p)` are **100% character-for-character IDENTICAL** (reading `P.v[instances[g_instIdx].paramOffset + 38] * g_volumeShared_i_G0`)!
  - `volumeExtinctionEval_0..3(p, D)` are **100% character-for-character IDENTICAL** (reading `P.v[instances[g_instIdx].paramOffset + 39] * g_volumeShared_i_G0`)!
  - Only `volumeEmissionEval_2(p, wo)` differs from `0, 1, 3` by a `+1` parameter offset (`45..62` vs `46..63`), which is a trivial 6-float multiply-add with `g_volumeShared_i_G0`!
- **Why the 4-branch `if ((mask & (1u << i)) != 0u)` in `ecProjected` hurts `ecConsume` on Apple M5 Metal:**
  1. In `ecConsume` (`@workgroup_size(64)`), each 32-wide Apple M5 SIMD wave spans 32 consecutive horizontal pixels (8 tiles wide), and in `S2` tiles (`26%–51%` of tiles) and `Fallback` tiles (`10%–42%` of tiles) different rays transition from `firstM` to `secondM` on different subpanel indices `single`!
  2. Whenever lanes in the same 32-wide SIMD wave have different `sub.info.x` (`0, 1, 2, 3`) at step `single`, the 4 separate `if ((mask & (1u << i)) != 0u)` blocks force the Apple M5 SIMD wave to **diverge and execute `volumeShared_i_G0(p)` + `cnoise3(p)` up to 4 times serially** instead of once in lockstep!
  3. Worse yet, because `volumeShared_0..3_Eval` are 4 distinct WGSL functions called inside `for (var q = 0u; q < 2u; q++)`, the Metal shader compiler inlines **up to 8 copies of `volumeShared_i_G0` + `cnoise3`** inside `ecConsume`'s inner loop, inflating instruction cache misses and register pressure!

### 3.3. Gate P13.3: Unified SIMD-Convergent `ecSingleEval` in `VolumeRoutedTransport.inc` (100% Bit-Exact IEEE-754 Parity!)

In `VolumeRoutedTransport.inc::program(const std::string& source)`, we can check at pipeline-compile time whether `volumeShared_0..3_G0`, `volumeDensityEval_0..3`, and `volumeExtinctionEval_0..3` are identical modulo member index (`_0.._3`). When they are identical (as in `Northern Veil`):
1. Emit a single unified helper `ecSingleMember(world: vec3<f32>, wo: vec3<f32>, s: f32, m: u32) -> vec4<f32>`:
   ```wgsl
   fn ecSingleMember(world: vec3<f32>, wo: vec3<f32>, s: f32, m: u32) -> vec4<f32> {
    g_instIdx = m + 1u;
    let p = world - instances[g_instIdx].origin.xyz;
    volumeShared_0_Eval(p);
    let D = volumeDensityEval_0(p);
    if (D > 0.0) {
     let X = max(volumeExtinctionEval_0(p, D), 1e-6);
     var E = vec3<f32>(0.0);
     if (m == 0u) {
      if (!VOLUME_EMISSION_READS_OMEGA_0 || s > 1e-8) { E = max(volumeEmissionEval_0(p, wo), vec3<f32>(0.0)); }
     } else if (m == 1u) {
      g_volumeShared_1_G0 = g_volumeShared_0_G0;
      if (!VOLUME_EMISSION_READS_OMEGA_1 || s > 1e-8) { E = max(volumeEmissionEval_1(p, wo), vec3<f32>(0.0)); }
     } else if (m == 2u) {
      g_volumeShared_2_G0 = g_volumeShared_0_G0;
      if (!VOLUME_EMISSION_READS_OMEGA_2 || s > 1e-8) { E = max(volumeEmissionEval_2(p, wo), vec3<f32>(0.0)); }
     } else {
      g_volumeShared_3_G0 = g_volumeShared_0_G0;
      if (!VOLUME_EMISSION_READS_OMEGA_3 || s > 1e-8) { E = max(volumeEmissionEval_3(p, wo), vec3<f32>(0.0)); }
     }
     return vec4<f32>(E, X);
    }
    return vec4<f32>(0.0);
   }
   ```
2. And in `ecConsume`:
   - For `!useOverlap` (**99.2% of all subpanels!**), call `ecSingleMember(ro + (lo + node * h) * rd, wo, lo + node * h, subMember)` **directly** without touching `mask` or looping over 4 curtains!
   - For `useOverlap` (**0.8% of subpanels**), `ecProjected` loops `for (var m = 0u; m < 4u; m++) { if ((mask & (1u << m)) != 0u) { result += ecSingleMember(world, wo, s, m); } }`!
- **Why this is 100% bit-exact (`rgba_fnv1a` and `publication_fnv1a` identical):**
  - `g_instIdx = m + 1u` and `p = world - instances[g_instIdx].origin.xyz` are identical.
  - `volumeShared_0_Eval(p)`, `volumeDensityEval_0(p)`, and `volumeExtinctionEval_0(p, D)` execute the exact same WGSL AST operations and load the exact same `P.v[instances[g_instIdx].paramOffset + k]` floats in the exact same order as `volumeShared_m_Eval(p)`, `volumeDensityEval_m(p)`, and `volumeExtinctionEval_m(p, D)`.
  - Copying `g_volumeShared_m_G0 = g_volumeShared_0_G0` and calling `volumeEmissionEval_m(p, wo)` evaluates the exact same emission expression for member `m`.
  - Every thread in the 32-wide SIMD wave now executes `volumeShared_0_Eval(p)` (all 5 `sin`/`cos` harmonics + `cnoise3`) **together in lockstep** regardless of which curtain `m \in \{0, 1, 2, 3\}` each ray is currently integrating!

---

## 4. Concrete Phase P13 Tasks for Sparkly Guy (`Gemini 3.8 Flash`)

### Task P13.1: Fail-Closed `encode()` Return Handling in `WebGpuRenderer.cpp`
In `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp:2088–2108`:
1. Check the return value of `_kernelRouted->encode(...)`:
   ```cpp
   bool routedEncoded = false;
   if(pipeline->routed){
       if(_pass){wgpuRenderPassEncoderEnd(_pass);wgpuRenderPassEncoderRelease(_pass);_pass=nullptr;_boundPipeline=nullptr;}
       const auto& spec=routedSpecs.at(pipeline);
       if(_kernelRouted->specsUniform)wgpuQueueWriteBuffer(_queue,_kernelRouted->specsUniform,0,spec.data(),sizeof(spec));
       auto* transport=_kernelRouted->program(pipeline->routedSource);
       if(transport)routedEncoded=_kernelRouted->encode(*transport,_encoder,
           {globalAlloc.buffer,globalAlloc.offset,globalAlloc.size},{paramBuffer,paramOffset,bytes},
           {instanceAlloc.buffer,instanceAlloc.offset,instanceAlloc.size},_depthView,{_kernelRouted->specsUniform,0,sizeof(spec)},{_kernelRouted->specsUniform,0,sizeof(spec)},
           [&](const void* data,size_t size){const auto a=bufferPool().suballocateUniform(data,size);return KernelRoutedRange{a.buffer,a.offset,a.size};});
       if(routedEncoded){
           ++mutableFrameStats().volumeRoutedSets;mutableFrameStats().volumeRoutedPixels+=_depthW*_depthH;
           mutableFrameStats().volumeRoutedWorkingSetBytes=uint64_t(_depthW)*_depthH*16+uint64_t(_kernelRouted->capacity)*(48+64*16+16*16+4)+uint64_t(_kernelRouted->tiles)*16+56;
       }else{
           ++mutableFrameStats().volumeProgramRefusals;
           mutableFrameStats().volumeLastProgramRefusal="routed transport encode refused; Current retained";
           if(_kernelRouted->publication){
               std::vector<uint32_t> refuseWords(size_t(_depthW)*_depthH*4, 0xbf800000u); // -1.0f in all lanes (.w < 0 => Current)
               wgpuQueueWriteBuffer(_queue,_kernelRouted->publication,0,refuseWords.data(),refuseWords.size()*sizeof(uint32_t));
           }
       }
   }
   ```
   Look how clean that is: if `encode()` ever returns `false`, `publication` is filled with `-1.0f` (`answer.w < 0.0` $\implies$ `fs()` executes `Current` for 100% of pixels!), `volumeProgramRefusals` is incremented, and `volumeRoutedSets` is NOT incremented!

### Task P13.2: Guard Post-Loop Subpanel Emission in `s2ProducerMain` (`VolumeRoutedKernels.hpp`)
In `src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp:615`:
Move `if (reason != 0u) { break; }` **before** `for (var p = 0u; p < count; p++)`:
```wgsl
        if (reason != 0u) { break; }
        for (var p = 0u; p < count; p++) {
            emitPieceSubpanels(pieces[p], member, subBase, &singles, &reason);
            if (reason != 0u) { break; }
        }
        if (reason != 0u) { break; }
```

### Task P13.3: Implement Unified SIMD-Convergent `ecSingleMember` in `VolumeRoutedTransport.inc`
In `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`:
1. Add a helper function in `kernel_routed_detail` that extracts `fn <name>_<i>(` through `\n}\n` from `source`, normalizes `_1`, `_2`, `_3` to `_0`, and verifies whether all 4 members have identical `volumeShared_i_G0`, `volumeShared_i_Eval`, `volumeDensityEval_i`, and `volumeExtinctionEval_i` definitions.
2. When they match (which holds for `Northern Veil`), generate `ecSingleMember(world: vec3<f32>, wo: vec3<f32>, s: f32, m: u32) -> vec4<f32>` so that:
   - Single-member subpanels (`!useOverlap`) call `ecSingleMember(ro + (lo + node * h) * rd, wo, lo + node * h, subMember)` directly (100% SIMD-convergent across all 4 curtains!).
   - Overlap subpanels (`useOverlap`) call `ecProjected(ro, rd, wo, lo + node * h, mask)`, which loops `for (var m = 0u; m < 4u; m++) { if ((mask & (1u << m)) != 0u) { result += ecSingleMember(world, wo, s, m); } }`.
3. Verify in Gate 1 of `positive_e2e_metal_verify.cpp` that `publication_fnv1a` (`68400cf910fb571e` at Pose 0, `43cd081115763a02` at Pose 32) and `rgba_fnv1a` (`d34c77bf8a341c6c` at Pose 0, `32fcde809ffdb96d` at Pose 32) remain **100% bit-for-bit identical**!

### Task P13.4: Add Gate 9 (Zach's 3 Northern Veil Checkpoint Views + Live Captured Camera Views Parity & ABBA Benchmark) to `positive_e2e_metal_verify.cpp`
In `scratch/probes/positive_e2e_metal_verify.cpp`:
Add **Gate 9** immediately after Gate 8 to test and benchmark the 5 live/checkpoint camera views on `saves/zones/Northern Veil/zone.ecform` at `2560x1440` (`fov = 45.0 deg`, `far = 1000.0f`):
1. `Live_23FPS_View_A`: `eye = (-6.749656f, 2.397435f, 3.705208f)`, `aim = eye + (0.0968301f, 0.5180270f, 0.8498659f)`, `up = (0, 1, 0)`
2. `Live_Parked_View`: `eye = (-14.059818f, 5.853045f, 0.831757f)`, `aim = eye + (0.0f, 0.5f, 0.8660254f)`, `up = (0, 1, 0)`
3. `Checkpoint_View1_Inside_Veil`: `eye = (12.5f, 67.0f, 39.0f)`, `aim = (12.5f, 67.0f, 184.0f)`, `up = (0, 1, 0)`
4. `Checkpoint_View2_Spawn_Dais_Diag`: `eye = (0.0f, 1.3f, 0.0f)`, `aim = (0.0f, 72.0f, 106.5f)`, `up = (0, 1, 0)`
5. `Checkpoint_View3_Below_Veils_Up`: `eye = (12.5f, 0.0f, 106.5f)`, `aim = (12.5f, 72.0f, 106.5f)`, `up = (0, 0, 1)`

For each of the 5 views in Gate 9:
- Record the route observer census (`empty`, `s1`, `s2`, `fallback` tile counts and percentages).
- Verify `0` refused rays, `0` non-finite words, and forced-refusal (`workLimit = 0`) bit-exact RGBA parity with `Current`.
- Run a 4-round completion-fenced ABBA timing comparison (`Current` vs `Routed`, 8 frames per round = 32 frames per mode per view) and report `current_mean_ms`, `routed_mean_ms`, `delta_ms`, `delta_pct`, and implied volume-pass FPS (`1000.0 / mean_ms`) in `docs/audits/data/positive_e2e_p13_verification.json`!
- Also rebuild `build/earthcall_webgpu` (`RelWithDebInfo` / `-O3 -DNDEBUG`) so Zach's live binary has Tasks P13.1–P13.3 ready for live testing!

---

**Signed:** Aetherion / The Firmament (`Gemini 4 Argon`) · 2026-11-01T17:30:00-08:00 (PST) · Human origin: Zachary Zhang
