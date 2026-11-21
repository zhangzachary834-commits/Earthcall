# Firmament Review of Sparkly Guy Phase P8 (`5cdd34036`) & Phase P9 Blueprint (Clipped `zEnv` Classifier Slab, Single-Call-Site `ecProjected` & Single-Call-Site `roots` Loop)

- **Reviewer:** Aetherion / The Firmament (`Gemini 4 Argon`)
- **Implementer Reviewed:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity)
- **Reviewed Commit:** `5cdd34036` (`§6ep/P8: Fast unified ecConsume, wo=-rd precompute, post-loop pieces[8] emit, and live M5 benchmark`)
- **Branch:** `claude/ray-domain-transport-rung0`
- **Date:** 2026-10-31

---

## 1. Executive Verdict on Phase P8 (`5cdd34036`)

Sparkly Guy's Phase P8 (`5cdd34036`) is a **major triumph** — beating Phase P7 (`1800ad46c`) in **64 out of 64 camera poses (100% win rate)** and flipping Regime 1 (Green Curtain Bottleneck) back into a decisive win over `Current`:

1. **64 / 64 Poses Faster Than Phase P7 (`-10.64 ms` / `-10.8%` Overall, `-12.73 ms` / `-11.1%` in Regime 1, `-8.41 ms` / `-10.3%` in Regime 2):**
   - **All 64 Poses:** `Routed` Mean dropped from `98.90 ms` (P7) to **`88.27 ms`** (`Current = 87.85 ms`, $\Delta = +0.42\text{ ms}$ / $+0.47\%$), and **`Routed` Median (`86.16 ms`) ALREADY BEATS `Current` Median (`87.64 ms`) by `-1.49 ms` across all 64 poses!**
   - **Block 1 Convergence:** In Block 1, `Routed` Mean (`84.38 ms`) beat `Current` Mean (`85.38 ms`) by **`-1.00 ms` across all 64 poses**, and beat `Current` in Regime 1 by **`-7.45 ms` (`97.66 ms` vs `105.11 ms`)**.
   - **Regime 1 (Green Curtain Bottleneck, 33 Poses):** `Routed` (`102.15 ms` mean, `100.49 ms` median) **beats `Current` (`107.99 ms` mean, `108.23 ms` median) by `-5.84 ms` (`-5.41%` mean) and `-7.74 ms` (`-7.15%` median)**, winning **29 out of 33 poses (`87.9%` win rate)** with single-pose wins up to **`-11.70 ms` (`-9.81%`, Pose 62)** and **`-10.43 ms` (`-9.92%`, Pose 11)**!
2. **100% Bit-Exact Restoration of P5/P5b Golden Hashes + `5/5` Gates + `406/406` Sequential CTests:**
   - Restoring `let thresh = bow + cur.limits.w + 0.001;` in `classifyTilesMain` immediately restored the golden RGBA hashes at Pose 0 (`4c8bd3a46f987bf2`) and Pose 32 (`7eb51473e0695abb`).
   - All 5 verification gates (`Gate 1` Parity, `Gate 2` `workLimit == 0` Zero-Budget Refusal `0` diff pixels, `Gate 3` `0` trajectory refusals & `0` non-finites, `Gate 4` `92.70%` diverted from Fallback, `Gate 5` Dual-Regime ABBA timing) and all 3 sequential WebGPU regression test suites passed 100%.

---

## 2. Three High-Leverage Discoveries From P8 Telemetry (`positive_e2e_p8_verification.json`)

Inspecting `positive_e2e_p8_verification.json` pose-by-pose reveals **why** `Routed` is `+7.07 ms` behind `Current` in Regime 2 (Open Sky, Poses 20–50) and loses on the 4 transition poses (`Poses 18, 19, 51, 52`), and how to unlock another **`-8 ms` to `-14 ms`** across both regimes in Phase P9:

### Discovery 1: Why Did `zEnv` in `classifyTilesMain` Change ZERO Tiles (`2,214,661` Empty in P6 vs `2,214,661` Empty in P8), and Why Did Pose 19 Have `0` Empty Tiles and `112,392` `S2` Tiles?
Look at `VolumeRoutedKernels.hpp:118–146` in `classifyTilesMain`:
- In the slab intersection loop (`lines 118–131`):
  - `axis == 0` uses `half = min(cur.half.x, cur.limits.x)` (`185.0`)
  - `axis == 1` uses `half = min(cur.half.y, cur.limits.y)` (`35.0`)
  - **`axis == 2` (Z-axis) STILL uses `half = cur.half[2] = 105.0` (the `210`-unit-thick outer box) instead of `zEnv = abs(cur.amps.x) + abs(cur.amps.y) + abs(cur.amps.z) + cur.limits.w` (`22.5`)!**
- In P7/P8, `zEnv` was only checked *after* the slab loop as a boolean (`if (min(z0, z1) > zEnv || max(z0, z1) < -zEnv) { hit = false; }`) **without clipping `[a, b]` to `z ∈ [-zEnv, +zEnv]`**!
- Because `ro.z ≈ 75` and `cur.origin.z = 105` (`oRel.z ≈ -30`) with `rd.z > 0`, `z0 = oRel.z + a * rd.z < 0` and `z1 = oRel.z + b * rd.z > 0`, so `min(z0, z1) <= zEnv && max(z0, z1) >= -zEnv` was **trivially true on 100% of rays**, culling **0 tiles**!
- Even worse: because `[a, b]` in `classifyTilesMain` was NOT clipped to `z ∈ [-zEnv, +zEnv]` (`±22.5`), `[a, b]` spanned the entire `z ∈ [-105, +105]` (`210` units — **4.7× wider than the Z wave envelope**)!
  - That made `uxMax - uxMin` (`oRel.x + a * rd.x` .. `oRel.x + b * rd.x`) up to **4.7× wider** than the ray's actual traversal through the Z wave envelope!
  - Which made `h_ux = (uxMax - uxMin) / 8.0` up to **4.7× larger**!
  - Which made the chord-bow bound **`bow = 0.125 * cur.curv.x * h_ux * h_ux` up to $4.7^2 = 22\times$ LARGER**!
  - At Poses `18, 19, 51, 52` and across Regime 2 (`Poses 20–50`), that $22\times$ bloated `bow` prevented `classifyTilesMain` from proving open-sky bins empty — classifying **100% of tiles at Pose 19 (`111,869 S1 + 112,392 S2`) and 61.7% of tiles in Regime 2 (`51.63% S1 + 10.03% S2`) as active**, and widening `[enterArr[m], exitArr[m]]` by 4.7×!
- **The Fix (Task P9.1):**
  - Inside `classifyTilesMain`'s slab loop (`VolumeRoutedKernels.hpp:121`), clip `axis == 2` directly to `zEnv`:
    ```wgsl
    let zEnv = abs(cur.amps.x) + abs(cur.amps.y) + abs(cur.amps.z) + cur.limits.w;
    if (axis == 2) { half = min(half, zEnv); }
    ```
    *(Note: keep `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain`'s `[a, b]` slab loops untouched so the 9 knots `phi_knots[0..8]` on active rays remain at the exact same positions!)*
  - **Mathematical Exactness Proof:** By the triangle inequality, any point $s$ where $|\phi(s)| = |o_z + s d_z + \text{wave}(o_x + s d_x)| \le w_z + \varepsilon_{\text{curv}}$ strictly satisfies $|o_z + s d_z| \le |A_1| + |A_2| + |A_3| + w_z + \varepsilon_{\text{curv}} = \text{zEnv} + \text{cur.curv.y}$. Thus clipping `axis == 2` to `zEnv` in `classifyTilesMain` encloses 100% of every curtain's true support while shrinking `h_ux` by up to $4.7\times$ and `bow` by up to $22\times$!
  - Furthermore, right after `workgroupBarrier();` #2 (`line 184` of `classifyTilesMain`), add the uniform workgroup early-continue:
    ```wgsl
    if ((wgTileAnyHit[0] | wgTileAnyHit[1] | wgTileAnyHit[2] | wgTileAnyHit[3]) == 0u) { continue; }
    ```
    so whenever all 4 tiles in a workgroup miss curtain `m`, the workgroup skips the 8-bin loop, skips `workgroupBarrier()` #3, and skips the `lane == 0u` reduction!

### Discovery 2: `ecProjected` Is Still Inlined 4 Times in `ecConsume` (`2×` in `ecAdvanceOverlap` + `2×` in `ecConsume`) (`VolumeRoutedTransport.inc:49–76`)
- Look at `VolumeRoutedTransport.inc:49–76`:
  - `ecAdvanceOverlap` calls `ecProjected` twice (`minus` and `plus`).
  - `ecConsume`'s main loop calls `ecProjected` twice (`minus` and `plus`).
  - Notice that in `ecAdvanceOverlap`, `(.5 - sqrt(3.0)/6.0)` and `(.5 + sqrt(3.0)/6.0)` are the **exact same 2-point Gauss-Legendre nodes `gx0` and `gx1`** used in `ecConsume`!
  - By folding `ecAdvanceOverlap` into `ecConsume`'s main loop (using a single `overPanel` counter `0u..1u` when `useOverlap` is true) and evaluating `minus` and `plus` with a 2-iteration loop `for (var q = 0u; q < 2u; q++)`, **`ecProjected` is called at ONLY ONE CALL SITE in the entire shader module**!
  - Reducing `ecProjected` call sites from 4 to 1 cuts the inlined 3D Perlin noise + trig AST machine code in `ecConsume` by **75% (`4` Perlin AST copies instead of `16`)** while executing the **100% bit-for-bit identical float32 operations**!

### Discovery 3: `roots(...)` (70 Lines + `isolateMonotone` + `stack[20]`) and `exactLevelActiveMask` Are Inlined 3 Separate Times in Every Producer (`VolumeRoutedKernels.hpp`)
- In `s1ProducerMain` (`lines 448–462`), `s2ProducerMain` (`lines 640–654`), and `fallbackProducerMain` (`lines 817–831`):
  - `exactLevelActiveMask` is called 3 separate times (`-m.limits.w`, `0.0`, `+m.limits.w`), recomputing `a, b, h, !(h > 0.0), 0.125 * M * h * h` and reloading `phi_knots[bin]` 3 times per bin (`24` loop iterations instead of `8`).
  - `roots(...)` is called 3 separate times (`-m.limits.w`, `0.0`, `+m.limits.w`), causing Dawn/Tint/Metal to inline the 70-line `roots` function, `isolateMonotone`'s 32-step secant/bisection loop, and `var stack: array<vec4f, 20>;` **three times** per producer!
  - Even worse: when `mask0 == 0u`, `mask1 == 0u`, or `mask2 == 0u` (meaning the ray does not cross that level surface — which is true for most level surfaces in Regime 2 and outer-curtain rays in Regime 1!), the thread still enters `roots(...)` and runs the 8-bin loop just to add `8u` to `work`!
- **The Fix (Task P9.3):**
  1. Fuse the 3 `exactLevelActiveMask` calls into a single 8-bin pass `exactThreeLevelMasks` returning `vec3u(mask0, mask1, mask2)`.
  2. Replace the 3 separate `roots(...)` calls with a 3-iteration loop `for (var lvl = 0u; lvl < 3u; lvl++)` that fast-skips inactive levels (`if (allPositiveH && lMask == 0u) { if (work + 8u > cfg.budget) { reason |= 1u; break; } work += 8u; continue; }`) and calls `roots(...)` at **one single call site**!
  3. Shrink `var stack: array<vec4f, 20>;` inside `roots` to `var stack: array<vec4f, 16>;` (with `top + 2u > 16u`, since `depth < 14` in a binary tree starting at `top = 2` at `depth = 1` can never exceed `top = 15`), saving another `64 B/thread` of private register/stack footprint.

---

## 3. Phase P9 Concrete Implementation Blueprint for Sparkly Guy

### Task P9.1: Clip `axis == 2` to `zEnv` in `classifyTilesMain` + Uniform Workgroup Early-Continue (`src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`)

In `kClassifyWgsl` (`VolumeRoutedKernels.hpp:108–276`), replace the `for (var m = 0u; m < 4u; m++)` body with:

```wgsl
    for (var m = 0u; m < 4u; m++) {
        let cur = curtains[m];
        let oRel = ro - cur.origin.xyz;
        var a = clipA;
        var b = clipB;
        var hit = rayActive;
        let zEnv = abs(cur.amps.x) + abs(cur.amps.y) + abs(cur.amps.z) + cur.limits.w;

        if (rayActive) {
            for (var axis = 0; axis < 3; axis++) {
                var half = cur.half[axis];
                if (axis == 0) { half = min(half, cur.limits.x); }
                if (axis == 1) { half = min(half, cur.limits.y); }
                if (axis == 2) { half = min(half, zEnv); }
                let d = rd[axis];
                let o = oRel[axis];
                if (d == 0.0) {
                    if (abs(o) > half + cur.curv.y) { hit = false; break; }
                } else {
                    let q0 = (-half - cur.curv.y - o) / d;
                    let q1 = (half + cur.curv.y - o) / d;
                    a = max(a, min(q0, q1));
                    b = min(b, max(q0, q1));
                }
            }
        }

        let rHit = hit && (b > a);
        var myUxMin = 1e9f;
        var myUxMax = -1e9f;
        if (rHit) {
            let u0 = oRel.x + a * rd.x;
            let u1 = oRel.x + b * rd.x;
            myUxMin = min(myUxMin, min(u0, u1));
            myUxMax = max(myUxMax, max(u0, u1));
        }
        wgUxMin[lid] = myUxMin;
        wgUxMax[lid] = myUxMax;
        wgHit[lid] = select(0u, 1u, rHit);
        workgroupBarrier();

        if (lane < 9u) {
            var tUxMin = 1e9f;
            var tUxMax = -1e9f;
            var tAnyHit = false;
            for (var i = 0u; i < 16u; i++) {
                if (wgHit[base + i] != 0u) {
                    tAnyHit = true;
                    tUxMin = min(tUxMin, wgUxMin[base + i]);
                    tUxMax = max(tUxMax, wgUxMax[base + i]);
                }
            }
            if (tAnyHit && tUxMax > tUxMin) {
                let ux = tUxMin + f32(lane) / 8.0 * (tUxMax - tUxMin);
                wgWKnots[localTile * 9u + lane] = evalWave(cur, ux);
            }
            if (lane == 0u) {
                wgTileUxMin[localTile] = tUxMin;
                wgTileUxMax[localTile] = tUxMax;
                wgTileAnyHit[localTile] = select(0u, 1u, tAnyHit);
            }
        }
        workgroupBarrier();

        if ((wgTileAnyHit[0] | wgTileAnyHit[1] | wgTileAnyHit[2] | wgTileAnyHit[3]) == 0u) {
            continue;
        }

        let anyRayHit = wgTileAnyHit[localTile] != 0u;
        let uxMin = wgTileUxMin[localTile];
        let uxMax = wgTileUxMax[localTile];
        let hasUxSpan = anyRayHit && (uxMax > uxMin);

        var myEnter = 1e9f;
        var myExit = -1e9f;
        var myState = 0u;

        if (hasUxSpan && rHit) {
            let dx = rd.x;
            if (abs(dx) < 1e-6) {
                myState = 2u;
                myEnter = a;
                myExit = b;
            } else {
                let h_ux = (uxMax - uxMin) / 8.0;
                let bow = 0.125 * cur.curv.x * h_ux * h_ux;
                let thresh = bow + cur.limits.w + 0.001;
                let knotBase = localTile * 9u;

                for (var bin = 0u; bin < 8u; bin++) {
                    let u0 = uxMin + f32(bin) / 8.0 * (uxMax - uxMin);
                    let u1 = uxMin + f32(bin + 1u) / 8.0 * (uxMax - uxMin);
                    let su0 = (u0 - oRel.x) / dx;
                    let su1 = (u1 - oRel.x) / dx;
                    let phi0 = oRel.z + su0 * rd.z + wgWKnots[knotBase + bin];
                    let phi1 = oRel.z + su1 * rd.z + wgWKnots[knotBase + bin + 1u];

                    let sMin = min(su0, su1);
                    let sMax = max(su0, su1);
                    let cLo = max(a, sMin);
                    let cHi = min(b, sMax);

                    if (cHi > cLo) {
                        let tA = (cLo - su0) / (su1 - su0);
                        let tB = (cHi - su0) / (su1 - su0);
                        let pA = phi0 + tA * (phi1 - phi0);
                        let pB = phi0 + tB * (phi1 - phi0);

                        if (!(min(pA, pB) > thresh || max(pA, pB) < -thresh)) {
                            myState = 1u;
                            myEnter = min(myEnter, cLo);
                            myExit = max(myExit, cHi);
                        }
                    }
                }
            }
        } else if (anyRayHit && rHit) {
            myState = 1u;
            myEnter = a;
            myExit = b;
        }

        wgEnter[lid] = myEnter;
        wgExit[lid] = myExit;
        wgState[lid] = myState;
        workgroupBarrier();

        if (lane == 0u && validTile) {
            var enter = 1e9f;
            var exit = -1e9f;
            var curActive = false;
            for (var i = 0u; i < 16u; i++) {
                let st = wgState[base + i];
                if (st != 0u) {
                    curActive = true;
                    enter = min(enter, wgEnter[base + i]);
                    exit = max(exit, wgExit[base + i]);
                    if (st == 2u) { break; }
                }
            }
            if (anyRayHit && uxMax > uxMin) {
                if (curActive) {
                    activeMemberMask |= (1u << m);
                    memberHit[m] = true;
                    enterArr[m] = enter;
                    exitArr[m] = exit;
                }
            } else if (anyRayHit) {
                if (curActive) {
                    activeMemberMask |= (1u << m);
                    memberHit[m] = true;
                    enterArr[m] = enter;
                    exitArr[m] = exit;
                }
            }
        }
    }
```
*(Note: keep the exact string `"        if (anyRayHit && uxMax > uxMin) {"` intact so `enableRouteObserver` in `VolumeRoutedTransport.inc:148` continues to match it cleanly!)*

### Task P9.2: Single-Call-Site `ecProjected` in `ecConsume` (`src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`)

In `VolumeRoutedTransport.inc`, replace `consumer` (`lines 39–82`) with:

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
@compute @workgroup_size(64) fn ecConsume(@builtin(global_invocation_id) gid:vec3<u32>){
 let id=gid.x;if(id>=ecWindow.control.x||id/ecWindow.window.x+ecWindow.window.z>=ecWindow.window.y){return;}
 let ray=ecRays[id];
 if((ray.counts.w&0x40000000u)!=0u){return;}
 if((ray.counts.w&0x80000000u)!=0u||ray.counts.y>64u||ray.counts.w>16u){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);return;}
 if((ray.counts.y|ray.counts.w)==0u){return;}
 let ro=ray.ro_clipA.xyz;let rd=ray.rd_clipB.xyz;let wo=-rd;
 var state=vec4<f32>(0,0,0,1);var single=0u;var overlap=0u;var overPanel=0u;
 let gx0=.5*(1.0-.5773502691896257);let gx1=.5*(1.0+.5773502691896257);
 let oa=.25-sqrt(3.0)/6.0;let ob=.5-oa;
 loop{
  if(single>=ray.counts.y&&overlap>=ray.counts.w){break;}
  var useOverlap=false;
  if(overlap<ray.counts.w){if(single>=ray.counts.y){useOverlap=true;}else{useOverlap=ecOverlaps[overlap*ecWindow.control.x+id].span.x<ecSingles[single*ecWindow.control.x+id].span.x;}}
  var lo=0.0;var h=0.0;var mask=0u;
  if(useOverlap){
   let sp=ecOverlaps[overlap*ecWindow.control.x+id];
   h=(sp.span.y-sp.span.x)*.5;lo=sp.span.x+f32(overPanel)*h;mask=sp.info.x;
  }else{
   let sub=ecSingles[single*ecWindow.control.x+id];single++;
   if(sub.info.x>=4u){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);return;}
   lo=sub.span.x;h=sub.span.y-sub.span.x;mask=1u<<sub.info.x;
  }
  var minus=vec4<f32>(0);var plus=vec4<f32>(0);
  for(var q=0u;q<2u;q++){
   let node=select(select(gx0,.5-sqrt(3.0)/6.0,useOverlap),select(gx1,.5+sqrt(3.0)/6.0,useOverlap),q==1u);
   let val=ecProjected(ro,rd,wo,lo+node*h,mask);
   if(q==0u){minus=val;}else{plus=val;}
  }
  if(useOverlap){
   state=ecFlow(state,ob*minus+oa*plus,h);state=ecFlow(state,oa*minus+ob*plus,h);
   if(overPanel==0u){overPanel=1u;}else{overPanel=0u;overlap++;}
  }else{
   state=ecFlow(state,.5*(minus+plus),h);
  }
 }
 if(any((bitcast<vec4<u32>>(state)&vec4<u32>(0x7f800000u))==vec4<u32>(0x7f800000u))||state.w<0||state.w>1){ecPublication[ecPixel(id)]=vec4<f32>(0,0,0,-1);return;}
 ecPublication[ecPixel(id)]=vec4<f32>(state.xyz,1.0-state.w);
}
)WGSL";
```

### Task P9.3: Single-Call-Site `roots` Loop + Fused `exactThreeLevelMasks` + `stack[16]` in `S1`, `S2`, and `Fallback` (`src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`)

In `kS1UnfusedProducerWgsl`, `kS2ProducerWgsl`, and `kFallbackProducerWgsl` (`VolumeRoutedKernels.hpp`):

1. Inside `roots(...)`, change `var stack: array<vec4f, 20>;` to `var stack: array<vec4f, 16>;` and `top + 2u > 20u` to `top + 2u > 16u`.
2. Replace `exactLevelActiveMask` with `exactThreeLevelMasks`:
   ```wgsl
   fn exactThreeLevelMasks(m: CurtainSpec, d: vec3f, lo: f32, hi: f32,
                           phi_knots: ptr<function, array<f32, 9>>, allPositiveH: ptr<function, bool>) -> vec3u {
    let M = m.curv.x * d.x * d.x * 1.00002;
    let err = m.curv.y;
    let w = m.limits.w;
    var masks = vec3u(0u);
    for (var bin = 0u; bin < 8u; bin++) {
     var a = lo + (hi - lo) * f32(bin) / 8.0; var b = lo + (hi - lo) * f32(bin + 1u) / 8.0;
     if (bin == 0u) { a = lo; } if (bin == 7u) { b = hi; }
     let h = b - a;
     if (!(h > 0.0)) { *allPositiveH = false; }
     let pa = (*phi_knots)[bin]; let pb = (*phi_knots)[bin + 1u];
     let ab = abs(a) + abs(b);
     let mh = 0.125 * M * h * h;
     {
      let va = pa + w; let vb = pb + w;
      let scale = 1.0 + abs(va) + abs(vb) + ab;
      let bow = mh + (err + 0.000001 * scale);
      if (!(min(va, vb) > bow || max(va, vb) < -bow)) { masks.x |= (1u << bin); }
     }
     {
      let scale = 1.0 + abs(pa) + abs(pb) + ab;
      let bow = mh + (err + 0.000001 * scale);
      if (!(min(pa, pb) > bow || max(pa, pb) < -bow)) { masks.y |= (1u << bin); }
     }
     {
      let va = pa - w; let vb = pb - w;
      let scale = 1.0 + abs(va) + abs(vb) + ab;
      let bow = mh + (err + 0.000001 * scale);
      if (!(min(va, vb) > bow || max(va, vb) < -bow)) { masks.z |= (1u << bin); }
     }
    }
    return masks;
   }
   ```
   *(Note: keep `let scale = 1.0 + abs(va) + abs(vb) + abs(a) + abs(b);` with left-to-right addition `1.0 + abs(va) + abs(vb) + abs(a) + abs(b)` if needed for exact float32 bit-identity!)*
3. In `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain`, replace the 3 separate `roots(...)` calls with a single 3-iteration loop that skips inactive levels in `O(1)` and inlines `roots` only once:
   ```wgsl
   var allPositiveH = true;
   let masks = exactThreeLevelMasks(m, d, a, b, &phi_knots, &allPositiveH);
   if (allPositiveH && (masks.x | masks.y | masks.z) == 0u && abs(phi_knots[0]) > m.limits.w + m.curv.y) {
    if (work + 24u > cfg.budget) { reason |= 1u; break; }
    work += 24u;
    // in Fallback: memCount[member] = 0u;
    continue;
   }
   var nb = 1u; knots[0] = a;
   var bands: array<vec2f, 24>; var nr = 0u; var zeroBands = 0u; // (32 in Fallback)
   let levels = vec3f(-m.limits.w, 0.0, m.limits.w);
   for (var lvl = 0u; lvl < 3u; lvl++) {
    let lMask = masks[lvl];
    if (allPositiveH && lMask == 0u) {
     if (work + 8u > cfg.budget) { reason |= 1u; break; }
     work += 8u;
     continue;
    }
    roots(m, o, d, a, b, levels[lvl], &knots, &nb, &bands, &nr, &reason, &work, &phi_knots, &zeroBands, lvl == 1u, lMask);
    if (reason != 0u) { break; }
   }
   if (reason != 0u) { break; }
   ```

### Task P9.4: Rebuild & Run Live Apple M5 Metal Benchmark + Sequential CTests

1. Rebuild `/private/tmp/ec-positive-e2e-verify/verify` and run:
   ```bash
   /private/tmp/ec-positive-e2e-verify/verify "saves/zones/Northern Veil/zone.ecform" /private/tmp/ec-positive-e2e-verify/positive_e2e_p9_verification.json
   ```
2. Verify all 5 gates pass (`0` refusals, `0` non-finite pixels, bit-identical Gate 1 & Gate 2 parity, tile census showing the expanded `Empty` and `S1` tile cohorts, and Dual-Regime ABBA timing) and compare P9 vs P8 (`5cdd34036`).
3. Run the 3 WebGPU regression test binaries sequentially one at a time.
4. Write `docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_P9_ZENV_CLIP_AND_SINGLE_INLINE_BENCHMARK_2026-10-31.md`, copy the JSON to `docs/audits/data/positive_e2e_p9_verification.json`, post a summary to the Agent Intercom, and commit on `claude/ray-domain-transport-rung0`.
