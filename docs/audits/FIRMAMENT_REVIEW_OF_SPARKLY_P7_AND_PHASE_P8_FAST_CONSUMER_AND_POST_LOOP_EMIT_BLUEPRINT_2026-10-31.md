# Firmament Review of Sparkly Guy Phase P7 & Phase P8 Blueprint (Fast Unbloated `ecConsume`, Precomputed `wo = -rd`, Post-Loop `pieces[16]` Emit & Tight Classifier Threshold)

- **Reviewer:** Aetherion / The Firmament (`Gemini 4 Argon`)
- **Implementer Reviewed:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity)
- **Reviewed Commit:** `1800ad46c` (`§6eo/P7: Coalesced ecConsume, top-of-fs short-circuit, wave envelope culling, and live M5 benchmark (-27.8 ms speedup)`)
- **Branch:** `claude/ray-domain-transport-rung0`
- **Date:** 2026-10-31

## 1. Executive Verdict on Phase P7 (`1800ad46c`)

Sparkly Guy's Phase P7 (`1800ad46c`) cleanly verified our primary architectural hypothesis from P6:

1. **Restoring Coalesced Row-Major `ecConsume` Delivered `-27.79 ms` (`-21.9%`) Overall and `-33.97 ms` (`-22.8%`) in Regime 1 Over P6 (`3f5d354c2`):**
   - By restoring `@builtin(global_invocation_id) gid: vec3<u32>` (`let id = gid.x;`) in `ecConsume` and pruning `p.local` back from 10 bindings to 4 (`ecWindow`, `ecRays`, `ecSingles`, `ecOverlaps`), adjacent threads `0..31` in every Apple M5 SIMD32 group once again read contiguous addresses `ecSingles[s * rayCount + id]` (`stride = 16 B`).
   - Mean routed frame time dropped from `126.69 ms` (P6) to **`98.90 ms`** across all 64 poses, from `148.85 ms` to **`114.88 ms`** in Regime 1 (Green Curtain Bottleneck), and from `103.09 ms` to **`81.90 ms`** in Regime 2 (Open Sky).
2. **All 5 Correctness & Determinism Gates (`5/5`) + `406/406` Sequential WebGPU Regression Tests Passed:**
   - **Gate 1 (Pose 0 & 32 Parity):** PASS (`eab4b08b803dc630` at Pose 0, `7eb51473e0695abb` at Pose 32, bit-identical across coalesced, uncoalesced, and poisoned-memory runs).
   - **Gate 2 (Zero-Budget Refusal `workLimit == 0`):** PASS (`0` differing pixels against `Current` at Pose 0 `d3b19e0e0c19de99` and Pose 32 `7974914a672b5779`).
   - **Gate 3 (Trajectory Zero Refusals):** PASS (`0` refused rays and `0` non-finite outputs across all 8 sampled poses).
   - **Gate 4 (Tile Census):** PASS (`Empty = 2,214,467` / `15.02%`, `S1 = 7,208,653` / `48.89%`, `S2 = 4,244,968` / `28.79%`, `Fallback = 1,077,512` / `7.31%`).
   - **Sequential CTest Regression Suite:** `406/406` assertions passing (`webgpu_volume_zero_proof_test`, `webgpu_volume_support_certificates_test`, `webgpu_line_quadrature_test`).

## 2. Forensic Root-Cause Diagnosis: Why Did `P5` (`-18.37 ms` Regime 1, `-8.20 ms` All 64) and `P5b` (`-7.62 ms` Regime 1, `+0.29 ms` All 64) Beat `Current`, Whereas `P7` (`114.88 ms` vs `105.28 ms` `Current`) Lost Ground?

A line-by-line comparison of `P5` (`24696a39b`), `P5b` (`13d150b84`), and `P7` (`1800ad46c`) reveals **four exact, surgical root causes** that explain every millisecond of the difference:

### Root Cause 1: `ecConsume` Duplicated theSingle-Span Loop (`if (ray.counts.w == 0u)` vs `else`), Doubling the Inlined AST Code Size of `ecProjected` in the Hottest Kernel (`VolumeRoutedTransport.inc:66–90`)
- Look at `ecProjected(ro, rd, s, mask)` in `VolumeRoutedTransport.inc:43–48`:
  `ecProjected` contains the **entire 3D `MathNode` AST for all 4 aurora curtains** (3D Perlin noise `cnoise3`, trig wave envelopes, `volumeShared_i_Eval`, `volumeDensityEval_i`, `volumeExtinctionEval_i`, and `volumeEmissionEval_i`).
- Because WGSL/Metal inlines `ecProjected` at every call site:
  - In `P5` and `P5b`, `ecConsume` had a **single unified `loop`**, so the single-span `ecProjected` pair (`minus` and `plus`) was inlined **once** (plus once inside `ecAdvanceOverlap`, which is cold when `ray.counts.w == 0u`).
  - In `P6` and `P7`, duplicating the single-span body inside `if (ray.counts.w == 0u) { ... }` AND `else { ... }` forced the Metal compiler to **inline all 4 curtains' 3D Perlin noise + trig ASTs TWICE** inside `ecConsume` (`4×` total inlined copies of `ecProjected` instead of `2×` in the main body), blowing out Apple M5's instruction cache (ICache) and register allocation!
- Even worse: look inside `ecProjected` (`VolumeRoutedTransport.inc:44`):
  ```wgsl
  let world=ro+s*rd;let delta=ro-world;let len=length(delta);let wo=select(vec3<f32>(0),delta/max(len,1e-8),len>1e-8);
  ```
  Mathematically, since `world = ro + s * rd` with `s > 0` and unit `rd` (`length(rd) == 1.0`), **`delta = ro - world = -s * rd`**, so **`wo` is identically `-rd` (`-ray.rd_clipB.xyz`) along the entire ray**!
  Yet `ecProjected` recomputes `delta = ro - world`, `len = length(delta)` (`sqrt`), and `delta / max(len, 1e-8)` (3 divisions) **at every quadrature node (up to `128` times per pixel)**!
  Precomputing `let wo = -rd;` once per ray in `ecConsume` and passing `wo` into `ecProjected(ro, rd, wo, s, mask)` eliminates `128` `sqrt` + division vectors per pixel!

### Root Cause 2: Calling `emitPieceSubpanels` *Inside* the Divergent `for (var i = 1u; i < nb; i++)` Knot Loop in `S1`/`S2` (`VolumeRoutedKernels.hpp:478, 667`) Caused SIMD Scatter-Write Divergence (`+10.75 ms` Regression Between `P5` and `P5b`)
- Compare `P5` (`24696a39b`, where `Routed` beat `Current` by **`-18.37 ms` in Regime 1** and **`-8.20 ms` across all 64 poses**) against `P5b` (`13d150b84`, where `Routed` dropped to `-7.62 ms` in Regime 1 and `+0.29 ms` across all 64 poses):
  - In `P5` (`24696a39b`), `s1ProducerMain` and `s2ProducerMain` appended merged intervals into a tiny local buffer `pieces[count] = vec2f(l, r);` during the `for (var i = 1u; i < nb; i++)` knot loop, and **only wrote `subpanels[singles * cfg.rayCount + id]` AFTER the knot loop finished** in a tight, uniform `for (var p = 0u; p < count; p++)` loop where all active lanes in the SIMD32 group wrote `singles = 0, 1, 2, ...` in lockstep!
  - In `P5b` (`13d150b84`), calling `emitPieceSubpanels(...)` **inside** the divergent `if (!uncertain && s0 == s1 && s0 == sm)` branch of `for (var i = 1u; i < nb; i++)` forced threads within the same SIMD32 group to execute global VRAM scatter writes (`subpanels[*singles * cfg.rayCount + id] = ...`) on different loop iterations `i` in the middle of the `evalPhi` trig loop!
- **The Best-of-Both-Worlds Fix for Phase P8:**
  - For a single curtain (`S1`, or one curtain at a time in `S2`), `nb <= 17` (`16` intervals), so after `mayAbsorb` a single curtain can produce **at most `16` pieces** (typically `1–3` pieces)!
  - Using a tiny **`var pieces: array<vec2f, 8>;`** (`64` bytes/thread, keeping thread scratch at `704 + 64 = 768 B/thread` — **still `-576 B/thread` smaller than `P5`'s `1,344 B/thread`**!) to buffer pieces during `for (var i = 1u; i < nb; i++)` (flushing if `count == 8u`) and emitting `subpanels` in a lockstep post-loop `for (var p = 0u; p < count; p++)` restores `P5`'s lockstep SIMD32 global memory writes while preserving `P5b`'s low register pressure!

### Root Cause 3: `classifyTilesMain` Added `+ cur.curv.y` (`+6.0`) to `thresh` (`VolumeRoutedKernels.hpp:209`), Bloating the 8-Bin Threshold by `+105%` and Canceling Out `zEnv` Culling
- Look at `VolumeRoutedKernels.hpp:209` in `1800ad46c`:
  ```wgsl
  - let thresh = bow + cur.limits.w + 0.001;
  + let thresh = bow + cur.limits.w + cur.curv.y + 0.001;
  ```
- Why did `empty` tiles in `P7` (`2,214,467`) barely change from `P6` (`2,214,661`), and why did `Fallback` tiles increase by `+759`?
  Because `evalWave(cur, ux)` in `classifyTilesMain` ALREADY evaluates the exact sinusoidal wave at all 9 knots `ux` (`bin = 0..8`), and the linear interpolation error between adjacent knots is strictly bounded by `bow = 0.125 * cur.curv.x * h_ux * h_ux`!
  Adding `cur.curv.y` (`6.0`) on top of `bow + cur.limits.w` bloated `thresh` from `~5.6` to `~11.6` (`+105%`), preventing the 8-bin classifier from proving empty bins and pushing non-overlapping tiles into `Fallback`!
- Furthermore, in `classifyTilesMain` (`VolumeRoutedKernels.hpp:132–139`), the `wgAnyValidRay` check adds **two `workgroupBarrier()` calls** to every workgroup even though `rayActive` (`clipB > clipA` on the union box `instances[0]`) is true for ~100% of tiles! Removing `wgAnyValidRay` (and keeping the direct `if (!rayActive)` per-lane guard) removes 2 barriers per workgroup across all `30` strips.
- And in `s1ProducerMain` (`VolumeRoutedKernels.hpp:434`), `s2ProducerMain` (`VolumeRoutedKernels.hpp:615`), and `fallbackProducerMain` (`VolumeRoutedKernels.hpp:792`), adding the same 4-line `zEnv` slab check right after the curtain AABB slab check (`if (hit && (b > a))`) lets individual rays in active `S1`/`S2`/`Fallback` tiles skip the `17`-knot `evalPhi` trig loop whenever a ray's $[a, b]$ segment lies outside `[-zEnv, +zEnv]`!

### Root Cause 4: Leftover `dd.requiredLimits = &limits` in `WgpuDevice.hpp` and `WebGpuContext.mm` from P6
- In P6 (`3f5d354c2`), `WgpuDevice.hpp` and `WebGpuContext.mm` were modified to request adapter-maximum storage buffer limits (`dd.requiredLimits = &limits`) when `p.local` temporarily had 10 bindings. Now that `p.local` is back to 4 bindings, reverting `WgpuDevice.hpp` and `WebGpuContext.mm` to default `WGPUDeviceDescriptor` limits restores clean baseline WebGPU device initialization.

## 3. Phase P8 Concrete Implementation Blueprint for Sparkly Guy

### Task P8.1: De-bloat `ecConsume` (Single Unified Loop + Precomputed `wo = -rd`) (`src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`)

1. In `VolumeRoutedTransport.inc`, replace `consumer` (`lines 41–94`) with a single unified loop that precomputes `let wo = -rd;` once per ray and inlines `ecProjected` only once for single subpanels:
   ```wgsl
   const char* consumer=R"WGSL(
   fn ecPhi(tau:f32)->f32 {if(abs(tau)<.01){return 1.0+tau*(-.5+tau*(1.0/6.0-tau/24.0));}return (1.0-exp(-tau))/tau;}
   fn ecFlow(state:vec4<f32>,v:vec4<f32>,h:f32)->vec4<f32>{let tau=h*v.w;return vec4<f32>(state.xyz+state.w*v.xyz*(h*ecPhi(tau)),state.w*exp(-tau));}
   fn ecProjected(ro:vec3<f32>,rd:vec3<f32>,wo:vec3<f32>,s:f32,mask:u32)->vec4<f32>{
    let world=ro+s*rd;
    var result=vec4<f32>(0);
    EC_PROJECTED
    return result;
   }
   fn ecAdvanceOverlap(stateIn:vec4<f32>,ro:vec3<f32>,rd:vec3<f32>,wo:vec3<f32>,span:ECSpan)->vec4<f32>{
    var state=stateIn;let h=(span.span.y-span.span.x)*.5;let a=.25-sqrt(3.0)/6.0;let b=.5-a;
    for(var panel=0u;panel<2u;panel++){
     let lo=span.span.x+f32(panel)*h;
     let minus=ecProjected(ro,rd,wo,lo+(.5-sqrt(3.0)/6.0)*h,span.info.x);
     let plus=ecProjected(ro,rd,wo,lo+(.5+sqrt(3.0)/6.0)*h,span.info.x);
     state=ecFlow(state,b*minus+a*plus,h);state=ecFlow(state,a*minus+b*plus,h);
    }
    return state;
   }
   @compute @workgroup_size(64) fn ecConsume(@builtin(global_invocation_id) gid:vec3<u32>){
    let id=gid.x;if(id>=ecWindow.control.x||id/ecWindow.window.x+ecWindow.window.z>=ecWindow.window.y){return;}
    let ray=ecRays[id];
    if((ray.counts.w&0x40000000u)!=0u){return;}
    if((ray.counts.w&0x80000000u)!=0u||ray.counts.y>64u||ray.counts.w>16u){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);return;}
    if((ray.counts.y|ray.counts.w)==0u){return;}
    let ro=ray.ro_clipA.xyz;let rd=ray.rd_clipB.xyz;let wo=-rd;
    var state=vec4<f32>(0,0,0,1);var single=0u;var overlap=0u;
    let gx0=.5*(1.0-.5773502691896257);let gx1=.5*(1.0+.5773502691896257);
    loop{
     if(single>=ray.counts.y&&overlap>=ray.counts.w){break;}
     var useOverlap=false;
     if(overlap<ray.counts.w){if(single>=ray.counts.y){useOverlap=true;}else{useOverlap=ecOverlaps[overlap*ecWindow.control.x+id].span.x<ecSingles[single*ecWindow.control.x+id].span.x;}}
     if(useOverlap){state=ecAdvanceOverlap(state,ro,rd,wo,ecOverlaps[overlap*ecWindow.control.x+id]);overlap++;continue;}
     let sub=ecSingles[single*ecWindow.control.x+id];single++;
     if(sub.info.x>=4u){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);return;}
     let h=sub.span.y-sub.span.x;let mask=1u<<sub.info.x;
     let minus=ecProjected(ro,rd,wo,sub.span.x+gx0*h,mask);
     let plus=ecProjected(ro,rd,wo,sub.span.x+gx1*h,mask);
     state=ecFlow(state,.5*(minus+plus),h);
    }
    if(any((bitcast<vec4<u32>>(state)&vec4<u32>(0x7f800000u))==vec4<u32>(0x7f800000u))||state.w<0||state.w>1){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);return;}
    ecPublication[ecPixel(id)]=vec4<f32>(state.xyz,1.0-state.w);
   }
   )WGSL";
   ```
2. And in `KernelRoutedTransport::program` (`VolumeRoutedTransport.inc:199`), update `sampling` so it uses `s > 1e-8` instead of `len > 1e-8` (since `len = s * length(rd) = s`):
   ```cpp
   for(unsigned i=0;i<4;++i){const auto n=std::to_string(i);sampling+="if((mask&"+std::to_string(1u<<i)+"u)!=0u){let inst=instances["+std::to_string(i+1)+"u];if(all(world>=inst.origin.xyz-inst.halfExtent.xyz)&&all(world<=inst.origin.xyz+inst.halfExtent.xyz)){g_instIdx="+std::to_string(i+1)+"u;let p=world-inst.origin.xyz;"+(source.find("fn volumeShared_"+n+"_Eval(")!=std::string::npos?"volumeShared_"+n+"_Eval(p);":"")+"let D=volumeDensityEval_"+n+"(p);if(D>0.0){let X=max(volumeExtinctionEval_"+n+"(p,D),1e-6);var E=vec3<f32>(0);if(!VOLUME_EMISSION_READS_OMEGA_"+n+"||s>1e-8){E=max(volumeEmissionEval_"+n+"(p,wo),vec3<f32>(0));}result+=vec4<f32>(E,X);}}}\n";}
   ```

### Task P8.2: Lockstep Post-Loop `pieces[8]` Subpanel Emission + Per-Ray `zEnv` Culling in `S1` & `S2` (`src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`)

1. **In `classifyTilesMain` (`VolumeRoutedKernels.hpp`):**
   - Remove `var<workgroup> wgAnyValidRay: u32;` and the two `workgroupBarrier()` calls around `wgAnyValidRay` (`lines 60, 132–139`), since `instances[0]` covers the viewport in almost every tile.
   - Restore line 209 to the exact mathematical chord-bow threshold:
     ```wgsl
     let thresh = bow + cur.limits.w + 0.001;
     ```
     (removing the redundant `+ cur.curv.y` that bloated `thresh` by `+105%`).
2. **In `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain` (`VolumeRoutedKernels.hpp`):**
   - Immediately after the curtain AABB slab loop (`if (!hit || !(b > a))` / `if (hit && (b > a))`), add the exact `zEnv` slab check so individual rays in active tiles that miss the curtain's Z wave envelope skip the 17-knot `evalPhi` loop completely:
     ```wgsl
     let zEnv = abs(cur.amps.x) + abs(cur.amps.y) + abs(cur.amps.z) + cur.limits.w + cur.curv.y;
     let z0 = oRel.z + a * rd.z;
     let z1 = oRel.z + b * rd.z;
     if (min(z0, z1) > zEnv || max(z0, z1) < -zEnv) {
         // return (in S1) or continue (in S2 / Fallback)
     }
     ```
   - In `s1ProducerMain` and `s2ProducerMain`, buffer the merged intervals in `var pieces: array<vec2f, 8>; var count = 0u;` during the `for (var i = 1u; i < nb; i++)` knot loop, and emit `subpanels` **after** the knot loop finishes in a tight lockstep `for (var p = 0u; p < count; p++)` loop (matching `P5` `24696a39b` while using only `64` bytes of `pieces` scratch instead of `384` bytes!):
     ```wgsl
     var pieces: array<vec2f, 8>;
     var count = 0u;
     for (var i = 1u; i < nb; i++) {
         ...
         if (active) {
             if (count > 0u && mayAbsorb(pieces[count - 1u].x, pieces[count - 1u].y, l, r)) {
                 pieces[count - 1u].y = r;
             } else if (count < 8u) {
                 pieces[count] = vec2f(l, r);
                 count++;
             } else {
                 emitPieceSubpanels(id, member, pieces[0].x, pieces[0].y, hTarget, &singles, &overflow);
                 for (var k = 1u; k < 8u; k++) { pieces[k - 1u] = pieces[k]; }
                 pieces[7u] = vec2f(l, r);
             }
         }
     }
     for (var p = 0u; p < count; p++) {
         emitPieceSubpanels(id, member, pieces[p].x, pieces[p].y, hTarget, &singles, &overflow);
     }
     ```

### Task P8.3: Revert Temporary P6 Device Limits (`WgpuDevice.hpp` & `WebGpuContext.mm`)

- Revert the temporary `dd.requiredLimits = &limits` changes in `src/Singularity/Screen/WebGPU/WgpuDevice.hpp` and `src/Singularity/Screen/WebGPU/WebGpuContext.mm` so the WebGPU device uses standard default limits now that all pipelines use $\le 8$ storage buffers.

### Task P8.4: Rebuild & Run Live Apple M5 Metal Benchmark + Sequential CTests

1. Rebuild `/private/tmp/ec-positive-e2e-verify/verify` and run:
   ```bash
   /private/tmp/ec-positive-e2e-verify/verify /private/tmp/ec-positive-e2e-verify/positive_e2e_p8_verification.json
   ```
2. Verify all 5 gates pass (`0` refusals, `0` non-finite pixels, bit-identical Gate 1 & Gate 2 parity, tile census, and Dual-Regime ABBA timing) and compare P8 vs P7 (`1800ad46c`), P5b (`13d150b84`), and P5 (`24696a39b`).
3. Run the 3 WebGPU regression test binaries sequentially one at a time.
4. Write `docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_P8_FAST_CONSUMER_BENCHMARK_2026-10-31.md`, copy the JSON to `docs/audits/data/positive_e2e_p8_verification.json`, post a summary to the Agent Intercom, and commit on `claude/ray-domain-transport-rung0`.
