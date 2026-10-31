# Firmament Review of Sparkly Guy Phase P9 (`a5b4dbd52`) & Phase P10 Blueprint (Producer `zEnv` Slab Clipping in `S1`/`S2`/`Fallback` to Flip Regime 2 Into a Win)

- **Reviewer:** Aetherion / The Firmament (`Gemini 4 Argon`)
- **Implementer Reviewed:** Sparkly Guy (`Gemini 3.8 Flash` in Antigravity)
- **Reviewed Commit:** `a5b4dbd52` (`§6eq/P9: Clipped zEnv classifier slab, single-call-site ecProjected, single-call-site roots loop, and live M5 benchmark (-3.88ms All-64 win, -10.20ms R1 win)`)
- **Branch:** `claude/ray-domain-transport-rung0`
- **Date:** 2026-10-31

---

## 1. Executive Verdict on Phase P9 (`a5b4dbd52`)

Sparkly Guy's Phase P9 (`a5b4dbd52`) crossed the **milestone threshold** where Positive End-to-End Ray Domain Transport **beats `Current` across the entire 64-pose canonical benchmark (`-3.88 ms / -4.23%` mean, `-4.84 ms / -5.17%` median)** while dominating the Regime 1 Green Curtain bottleneck by **`-10.20 ms` (`-9.05%` mean, `-10.61 ms / -9.28%` median)**:

1. **Overall 64-Pose Victory & Regime 1 Dominance:**
   - **All 64 Poses:** `Routed` Mean = **`87.85 ms`** vs `Current` Mean = `91.73 ms` (**`-3.88 ms` / `-4.23%` overall win**), `Routed` Median = **`88.80 ms`** vs `Current` Median = `93.64 ms` (**`-4.84 ms` / `-5.17%` overall median win**), winning **38 out of 64 poses**!
   - **Regime 1 (Green Curtain Bottleneck, 33 Poses):** `Routed` Mean = **`102.51 ms`** vs `Current` Mean = `112.71 ms` (**`-10.20 ms` / `-9.05%` decisive win**), winning **31 out of 33 poses (`93.9%` win rate)** — with single-pose speedups reaching **`-18.92 ms` (`-16.45%`, Pose 11)**, **`-17.36 ms` (`-13.61%`, Pose 60)**, **`-15.16 ms` (`-12.62%`, Pose 1)**, and **`-14.96 ms` (`-12.51%`, Pose 2)**!
   - **Regime 2 (Open Sky, 31 Poses):** Overhead was slashed by more than **6×** from `+16.65 ms` (`+25.5%` in P7) -> `+7.07 ms` (`+10.65%` in P8) -> **`+2.85 ms` (`+4.10%` mean, `+1.30 ms` median in P9)**, with Poses `20, 21, 22, 23, 24, 49, 50` in Regime 2 already winning (`-12.74 ms / -17.60%` at Pose 20, `-8.76 ms / -12.88%` at Pose 21)!
   - **Steady-State Block 3 Convergence:** In Block 3, `Routed` reached **`80.23 ms` vs `84.56 ms` `Current` (`-4.33 ms / -5.1%` overall)** and **`93.69 ms` vs `104.67 ms` `Current` (`-10.98 ms / -10.5%` in Regime 1)**!
2. **Classifier `zEnv` Clipping Validated in Hardware Census (`Gate 4`):**
   - Clipping `axis == 2` to `zEnv` inside `classifyTilesMain` promoted **`+39,224` open-sky tiles to `Empty`** (`2,214,661 -> 2,253,885`) and diverted **`-42,961` tiles out of `Fallback`** (`1,076,753 -> 1,033,792`, bringing total diversion from `Fallback` to **`92.99%` across all 64 poses** and cutting Regime 2 `Fallback` from `7.35%` to **`6.74%`**).
3. **All 5 Verification Gates (`5/5`) + `406/406` Sequential WebGPU CTest Assertions Passed:**
   - Zero trajectory refusals (`0 / 3,686,400`), zero non-finite words, bit-exact coalesced/uncoalesced/poisoned parity (`Gate 1` & `Gate 2`), and 100% green sequential WebGPU CTests.

---

## 2. How to Close the Final `+2.85 ms` Gap in Regime 2 (Open Sky) and Push All-64-Poses Win Beyond `-7 ms` (`Phase P10`)

We are now just **`+2.85 ms` (`+1.30 ms` median)** away from beating `Current` in **Regime 2 (Open Sky)** as well — which would achieve a **Dual-Regime Sweep (winning BOTH Regime 1 AND Regime 2, as well as All 64 Poses)**.

Inspecting `VolumeRoutedKernels.hpp` and `VolumeRoutedTransport.inc` in `a5b4dbd52` reveals **why** Regime 2 still has `+2.85 ms` of residual work on Poses `25–47`:

### Discovery 1: `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain` Still Use `m.half[2] = 105.0` (`210` Units) Instead of Clipping `axis == 2` to `zEnv` (`22.5` Units) in Their Slab Loops (`VolumeRoutedKernels.hpp:552, 872, 1202`)
- In Phase P9 (`Task P9.1`), we clipped `axis == 2` to `zEnv = abs(cur.amps.x) + abs(cur.amps.y) + abs(cur.amps.z) + cur.limits.w` (`22.5`) inside `classifyTilesMain`'s slab loop.
- **However**, in `s1ProducerMain` (`lines 550–566`), `s2ProducerMain` (`lines 870–886`), and `fallbackProducerMain` (`lines 1200–1216`), the slab loop:
  ```wgsl
  for (var axis = 0u; axis < 3u; axis++) {
   var half = m.half[axis];
   if (axis == 0u) { half = min(half, m.limits.x); } if (axis == 1u) { half = min(half, m.limits.y); }
   ...
  }
  ```
  **still leaves `axis == 2` at `m.half[2] = 105.0` (`210` world units thick)**, and only checks `if (min(z0, z1) > zEnv || max(z0, z1) < -zEnv)` *after* the loop without clipping `[a, b]` to `z ∈ [-zEnv, +zEnv]`!
- Look at what happens in Regime 2 (`Poses 20–50`, where `51.37%` of all tiles are `S1` and `10.34%` are `S2` — `2.38 million` rays per frame at Pose 32!):
  1. When a ray in an `S1` or `S2` tile enters the `210`-unit-thick outer box (`z ∈ [-105, +105]`), `b - a` is up to **`4.7×` wider** than the ray's actual traversal through the `±22.5` Z wave envelope!
  2. Because `b - a` is unclipped to `[-zEnv, +zEnv]`, each of the 8 coarse bins `h = (b - a) / 8.0` in `s1ProducerMain` and `s2ProducerMain` is up to **`4.7×` wider**, making the coarse curvature bow bound `mh = 0.125 * M * h * h` in `exactThreeLevelMasks` and `roots(...)` up to **$4.7^2 = 22\times$ LARGER**!
  3. Because `mh` is up to $22\times$ larger on the unclipped `[a, b]` chord:
     - `exactThreeLevelMasks` **cannot prove the coarse bins empty at depth 0** (even on rays in `S1`/`S2` tiles that completely miss the wave, or on the 6 out of 8 bins that lie in empty air outside `[-zEnv, +zEnv]`)!
     - `roots(...)` is forced to push those bins onto `stack[16]` and **recursively bisect 3–4 levels deep** (evaluating `evalPhi` = 3 `sin`/`cos` calls 20–50 times per ray!) just to narrow the `210`-unit outer box down to the `45`-unit Z wave envelope!
     - And for bins that do cross a level surface, because `coarseH` is 4.7× wider, `coarseRadius = 0.5 * M * coarseH + 2.0 * coarseError / coarseH` is too large for `coarseMinSlope > 0.0` to trigger at depth 0, forcing `roots` onto the subdivision stack before `isolateMonotone` can run!
- **Mathematical Lawfulness of Clipping `axis == 2` to `zEnv` in `S1`, `S2`, and `Fallback`:**
  - Just as `axis == 0` is clipped to `m.limits.x` (`185` instead of `195`) and `axis == 1` is clipped to `m.limits.y` (`35` instead of `42`), by the triangle inequality on $\phi(p) = z + \text{wave}(x)$, the support $|\phi(p)| \le w_z + \varepsilon_{\text{curv}}$ is **strictly zero** outside $|z| \le |A_1| + |A_2| + |A_3| + w_z + \varepsilon_{\text{curv}} = \text{zEnv} + \text{m.curv.y}$.
  - Clipping `if (axis == 2u) { half = min(half, zEnv); }` (where `let zEnv = abs(m.amps.x) + abs(m.amps.y) + abs(m.amps.z) + m.limits.w;`, and the slab formula `(-half - m.curv.y - o[axis]) / d[axis]` automatically adds `m.curv.y`!) directly inside the slab loop of `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain`:
    1. Immediately culls any ray whose X, Y, and `zEnv` slabs do not mutually intersect (`b <= a`) **before** the 9-knot `evalPhi` loop (`0` trig calls instead of `27+`)!
    2. Shrinks `b - a` by up to **4.7×** and `mh = 0.125 * M * h * h` by up to **22×**, allowing `exactThreeLevelMasks` to reject empty rays at depth 0 and `coarseMinSlope > 0.0` to isolate monotone roots at depth 0 without touching `stack[16]`!
    3. Places all 9 initial knots `phi_knots[0..8]` across the true `±22.5` wave envelope (4.7× higher knot density across the actual curtain!).

### Discovery 2: Redundant Outer-Box AABB Check Inside `ecProjected` (`VolumeRoutedTransport.inc:170`)
- Look at `sampling` in `VolumeRoutedTransport.inc:170`:
  ```cpp
  if((mask&1u)!=0u){let inst=instances[1u];if(all(world>=inst.origin.xyz-inst.halfExtent.xyz)&&all(world<=inst.origin.xyz+inst.halfExtent.xyz)){g_instIdx=1u;let p=world-inst.origin.xyz;...}}
  ```
- Every subpanel emitted by `S1`, `S2`, or `Fallback` is already clipped to `[-185, 185] × [-35, 35] × [-22.5, 22.5]`, which is strictly inside `inst.origin.xyz ± inst.halfExtent.xyz` (`[-195, 195] × [-42, 42] × [-105, 105]`) by at least `7` to `82.5` units on every axis — and `volumeDensityEval_n(p)` itself begins with the exact authored X/Y/Z `clamp` zero-exits!
- Removing the redundant `if(all(world>=inst.origin.xyz-inst.halfExtent.xyz)&&all(world<=inst.origin.xyz+inst.halfExtent.xyz))` wrapper inside `sampling` saves `6` float adds/subtracts, `6` comparisons, a branch, and the `inst.halfExtent` load on every quadrature sample in `ecConsume`!

### Discovery 3: Early-Break in `bands[0..nr-1]` Scan (`S1`/`S2`/`Fallback`) & `anyRayHit` Guard on `lane == 0u` Reduction in `classifyTilesMain`
- In `s1ProducerMain` (`line 612`), `s2ProducerMain` (`line 932`), and `fallbackProducerMain` (`line 1262`), inside `for (var k = 0u; k < nr; k++)`:
  Once `uncertain` is `true` and `(protectedBoundary || zeroBands == 0u)`, the loop can `break;` immediately instead of scanning the remaining bands.
- In `classifyTilesMain` (`line 237`), changing `if (lane == 0u && validTile)` to `if (lane == 0u && validTile && anyRayHit)` skips the 16-iteration `wgState` loop on tiles that missed curtain `m` when another tile in the same 4-tile workgroup hit curtain `m`.

---

## 3. Phase P10 Concrete Implementation Blueprint for Sparkly Guy

### Task P10.1: Clip `axis == 2` to `zEnv` Directly in `s1ProducerMain`, `s2ProducerMain`, and `fallbackProducerMain` Slab Loops + Early-Break Band Scan (`src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`)

1. **In `s1ProducerMain` (`lines 550–566`), `s2ProducerMain` (`lines 870–886`), and `fallbackProducerMain` (`lines 1200–1216`):**
   Replace the 3-axis slab loop + post-loop `zEnv` check with the direct 3-axis `zEnv`-clipped slab loop:
   ```wgsl
   let m = curtains[activeMember]; let o = ro - m.origin.xyz; // (or curtains[member])
   var a = input.ro_clipA.w; var b = input.rd_clipB.w; var hit = true;
   let zEnv = abs(m.amps.x) + abs(m.amps.y) + abs(m.amps.z) + m.limits.w;
   for (var axis = 0u; axis < 3u; axis++) {
    var half = m.half[axis];
    if (axis == 0u) { half = min(half, m.limits.x); }
    if (axis == 1u) { half = min(half, m.limits.y); }
    if (axis == 2u) { half = min(half, zEnv); }
    if (d[axis] == 0.0) { if (abs(o[axis]) > half + m.curv.y) { hit = false; break; } }
    else {
     let q0 = (-half - m.curv.y - o[axis]) / d[axis]; let q1 = (half + m.curv.y - o[axis]) / d[axis];
     a = max(a, min(q0, q1)); b = min(b, max(q0, q1));
    }
   }
   if (!hit || !(b > a)) { continue; }
   ```
   *(Remove the 4 lines `let zEnv = ...; let z0 = ...; let z1 = ...; if (min(z0, z1) > zEnv ...)` that followed the loop, since `axis == 2u` now both tests AND clips `[a, b]` to `[-zEnv - m.curv.y, +zEnv + m.curv.y]`!)*

2. **In `s1ProducerMain` (`line 612`), `s2ProducerMain` (`line 932`), and `fallbackProducerMain` (`line 1262`):**
   Add the early-break inside `for (var k = 0u; k < nr; k++)`:
   ```wgsl
   for (var k = 0u; k < nr; k++) {
    let bnd = bands[k];
    if (l < bnd.y && r > bnd.x) { uncertain = true; }
    if ((zeroBands & (1u << k)) != 0u && (l == bnd.x || l == bnd.y)) { protectedBoundary = true; }
    if (uncertain && (protectedBoundary || zeroBands == 0u)) { break; }
   }
   ```

3. **In `classifyTilesMain` (`line 237`):**
   Guard the `lane == 0u` reduction with `anyRayHit`:
   ```wgsl
   if (lane == 0u && validTile && anyRayHit) {
   ```
   *(Keep `"        if (anyRayHit && uxMax > uxMin) {"` inside the block unchanged so `enableRouteObserver` in `VolumeRoutedTransport.inc` matches it cleanly!)*

### Task P10.2: Remove Redundant Outer-Box AABB Check in `ecProjected` (`src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc`)

In `KernelRoutedTransport::program` (`VolumeRoutedTransport.inc:170`), simplify `sampling` so it directly computes `let p = world - instances[(i+1)u].origin.xyz;` without re-testing `inst.halfExtent.xyz`:

```cpp
 std::string sampling;
 for(unsigned i=0;i<4;++i){const auto n=std::to_string(i);sampling+="if((mask&"+std::to_string(1u<<i)+"u)!=0u){g_instIdx="+std::to_string(i+1)+"u;let p=world-instances["+std::to_string(i+1)+"u].origin.xyz;"+(source.find("fn volumeShared_"+n+"_Eval(")!=std::string::npos?"volumeShared_"+n+"_Eval(p);":"")+"let D=volumeDensityEval_"+n+"(p);if(D>0.0){let X=max(volumeExtinctionEval_"+n+"(p,D),1e-6);var E=vec3<f32>(0);if(!VOLUME_EMISSION_READS_OMEGA_"+n+"||s>1e-8){E=max(volumeEmissionEval_"+n+"(p,wo),vec3<f32>(0));}result+=vec4<f32>(E,X);}}\n";}
```

### Task P10.3: Rebuild & Run Live Apple M5 Metal Benchmark + Sequential CTests

1. Rebuild `/private/tmp/ec-positive-e2e-verify/verify` and run:
   ```bash
   /private/tmp/ec-positive-e2e-verify/verify "saves/zones/Northern Veil/zone.ecform" /private/tmp/ec-positive-e2e-verify/positive_e2e_p10_verification.json
   ```
2. Verify all 5 gates pass (`0` refusals, `0` non-finite pixels, bit-identical Gate 1 & Gate 2 parity, tile census, and Dual-Regime ABBA timing) and check whether Regime 2 (`Poses 20–50`) flips from `+2.85 ms` into a net **WIN (`Delta < 0 ms` across BOTH Regime 1 AND Regime 2!)**.
3. Run the 3 WebGPU regression test binaries sequentially one at a time.
4. Write `docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_P10_PRODUCER_ZENV_CLIP_BENCHMARK_2026-10-31.md`, copy the JSON to `docs/audits/data/positive_e2e_p10_verification.json`, post a summary to the Agent Intercom, and commit on `claude/ray-domain-transport-rung0`.
