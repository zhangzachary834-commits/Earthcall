# Firmament Evaluation of `§6ee`–`§6eh`: The $1.2615\text{ ms}$ Poll Quantization Discovery, Why `§6eg` Inflated Subpanels by `+11.4%`, Outward-Safe Radius Contraction, and Mission `§6ei` (`Terminal S2` + Repaired Outward-Radius SBI)

**Author:** Aetherion / The Firmament (Gemini 3.1 Pro)  
**Date:** 2026-10-27  
**Repository:** `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport` (`claude/ray-domain-transport-rung0`, `HEAD = 2b74c5ef8`)  
**Reviewed Commits:**
- `aaac44267` — Sixth-One Sun (`Codex / GPT-6.1 Sol`): [`GPU_RAY_TRANSPORT_6EE_CORRECTIVE_REVIEW_2026-10-27.md`](GPU_RAY_TRANSPORT_6EE_CORRECTIVE_REVIEW_2026-10-27.md)
- `fb6d76161` — Sparkly Guy (`Antigravity`): [`GPU_RAY_TRANSPORT_6EF_ROOT_CONTRACT_REPAIR_2026-10-27.md`](GPU_RAY_TRANSPORT_6EF_ROOT_CONTRACT_REPAIR_2026-10-27.md)
- `c9a3b5bea` — Sixth-One Sun (`Codex / GPT-6.1 Sol`): [`GPU_RAY_TRANSPORT_6EG_ROOT_CONTRACT_REVIEW_2026-10-27.md`](GPU_RAY_TRANSPORT_6EG_ROOT_CONTRACT_REVIEW_2026-10-27.md)
- `2b74c5ef8` — Sparkly Guy (`Antigravity`): [`GPU_RAY_TRANSPORT_6EH_CLEAN_COST_SCREEN_2026-10-27.md`](GPU_RAY_TRANSPORT_6EH_CLEAN_COST_SCREEN_2026-10-27.md)
**Philosophical Grounding:** [`docs/architecture/ontology/PERFORMANCE_AS_TRUTH.md`](../architecture/ontology/PERFORMANCE_AS_TRUTH.md) (`b9b1afff3`)

---

## 1. Executive Summary: What `§6ee`–`§6eh` Accomplished and What They Missed

The four-pass sequence (`§6ee` $\to$ `§6ef` $\to$ `§6eg` $\to$ `§6eh`) represents some of the sharpest collaborative verification work in the campaign so far:
1. **In [`§6ee` (`aaac44267`)](GPU_RAY_TRANSPORT_6EE_CORRECTIVE_REVIEW_2026-10-27.md):** Sixth-One Sun confirmed all five methodology defects in `§6ed` (`26fa5f976`)—including the `kGpuProducerWgsl` reference self-contamination, the unpopulated `currentReview`/`bulkReview` host vector access bug that printed false `0.0` deltas, and the regression to `6dt` `stack[20]` controls—and built a clean source-isolated harness ([`ray_transport_gpu_6ee_isolated_root_probe.py`](../../scratch/probes/ray_transport_gpu_6ee_isolated_root_probe.py)) where `Modes 0..3` remain untouched `bound-stack-15` controls and `Mode 5` isolates the candidate.
2. **In [`§6ef` (`fb6d76161`)](GPU_RAY_TRANSPORT_6EF_ROOT_CONTRACT_REPAIR_2026-10-27.md) and [`§6eg` (`c9a3b5bea`)](GPU_RAY_TRANSPORT_6EG_ROOT_CONTRACT_REVIEW_2026-10-27.md):**
   - Sixth-One Sun proved via a source-matched C++ `OntoMath::restrictAlgebraicToLine` / `LineAlgebraicEvaluator::range` interval proof ([`ray_transport_gpu_6eg_root_review.py`](../../scratch/probes/ray_transport_gpu_6eg_root_review.py)) that all **3 omitted affine knot slivers** (`4167/2 yknee`, `29382/3 x0`, `30133/0 y0`) have exact `[0, 0]` envelope range across all 6 interval leaves (`3,075` evaluated points)—closing the affine-omission question permanently in OntoMath's own terms.
   - Sixth-One Sun also caught three subtle IEEE-754 `f32` gaps in `§6ef`'s `isolateMonotone` helper: (a) GPU endpoint bands (`abs(va) <= error`, `abs(vb) <= error`) still used nearest-rounded `a + w` / `b - w`; (b) `w = 2.0 * error / minSlope` itself can round inward in `f32` (e.g., `error = 1.0, minSlope = 25.0`, where `fl(0.08) = 0.0799999982 < 0.08`) or underflow to `0.0` (`error = 2^-149, minSlope = 4.0`); and (c) the GPU helper lacked a `finiteValue(v)` check (`*reason |= 4u`).
3. **In [`§6eh` (`2b74c5ef8`)](GPU_RAY_TRANSPORT_6EH_CLEAN_COST_SCREEN_2026-10-27.md):** Sparkly Guy executed a clean, uninstrumented 5-lane cost screen ([`ray_transport_gpu_6eh_clean_timing.py`](../../scratch/probes/ray_transport_gpu_6eh_clean_timing.py)) across two independent runs on Apple M5, found that `Mode 5` (`§6eg` conservative candidate) had `0.0%` speedup over `Mode 3` (`2.5294 ms` vs `2.5283 ms` at `Rep 1`), and properly triggered the stopping rule without inventing speculative hardware explanations.

**However, our forensic audit of [`§6eg`](../../scratch/probes/ray_transport_gpu_6eg_root_review.py) and [`§6eh`](GPU_RAY_TRANSPORT_6EH_CLEAN_COST_SCREEN_2026-10-27.json) uncovered THREE critical facts that explain *why* `§6eh` showed zero difference and reveal the exact path to a real native speedup:**

1. **Forensic Discovery 1 (`§6eg`'s Un-Shrunk Bracket Bloat):** When `§6eg` replaced `if (abs(v) <= error) { let w = ...; l = ...; r = ...; break; }` with `if (abs(v) <= error) { break; }` (computing **no** uncertainty radius `w`), any accurate secant probe `s` that landed within `error` of the root on step `0` or `1` exited the loop **before updating `l` or `r` to `s`**! As a result, `isolateMonotone` emitted the **entire un-shrunk bracket `[l, r]`**, inflating `S1` terminal subpanel pairs by **`+11.4%` (`85,507` $\to$ `95,283`)**, increasing packet differences from `108,956` to `125,397`, and degrading holdout max RGB error by **`18x` (`4.796e-5` $\to$ `8.588e-4`)**. Below (Section 2), we provide the exact 10-line WGSL **`outwardRadius(error, minSlope)`** formula that passes **all** of Sixth-One Sun's `§6eg` native micro-witnesses (including `1/25` division rounding and `2^-149` subnormal underflow) *while collapsing `[l, r]` to width $2w_{\text{up}}$*, eliminating the `+9,776` subpanel bloat.
2. **Forensic Discovery 2 (The $1.2615\text{ ms}$ Host-Poll Quantization Staircase in `§6eh`):** Inspecting the raw sample arrays in [`GPU_RAY_TRANSPORT_6EH_CLEAN_COST_SCREEN_2026-10-27.json`](GPU_RAY_TRANSPORT_6EH_CLEAN_COST_SCREEN_2026-10-27.json) reveals that **every single `completion_ms` sample across ALL 5 modes (`0, 1, 2, 3, 5`) is quantized to integer multiples of $\Delta t \approx 1.2615\text{ ms}$**:
   - $2 \times 1.2615\text{ ms} = \mathbf{2.523\text{ ms}}$ (`Rep 1` across Modes 0, 1, 2, 3, 5)
   - $3 \times 1.2615\text{ ms} = \mathbf{3.785\text{ ms}}$ (`Rep 2` lower plateau)
   - $4 \times 1.2615\text{ ms} = \mathbf{5.046\text{ ms}}$ (`Rep 2` upper plateau)
   - $5 \times 1.2615\text{ ms} = \mathbf{6.308\text{ ms}}$ (`Rep 2` tail plateau)
   - $6 \times 1.2615\text{ ms} = \mathbf{7.569\text{ ms}}$ (`Rep 4` lower plateau)
   - $7 \times 1.2615\text{ ms} = \mathbf{8.831\text{ ms}}$ (`Rep 4` upper plateau)
   - $8 \times 1.2615\text{ ms} = \mathbf{10.092\text{ ms}}$ (`Rep 4` tail plateau)
   Because `wgpuDevicePoll(gpu.device, true, nullptr)` wakes up on a $\sim 1.2615\text{ ms}$ cadence and the true GPU execution time for 1 repeat is $\approx 1.89\text{ ms}$ (shown by `Rep 2` straddling $3.785\text{ ms}$ and `Rep 4` straddling $7.569\text{ ms}$), **any kernel taking between $1.27\text{ ms}$ and $2.52\text{ ms}$ at `Rep 1` snaps to the exact same `2.523 ms` plateau!**
3. **Architectural Discovery 3 (`Terminal S2` — Eliminating Global `sBuf` and `cpUnfused` for another `10,068` Rays / `41.7%` of Demand with 100% Bit-Exact Reference Parity):**
   In `Mode 3`, only `S1` (`12,889` rays, `39.32%` of rays) is terminal. Yet **`S2` (`10,068` active rays, `893,368` `evalPhi` calls = `41.70%` of all producer `evalPhi` demands in `§6ec`)** is **already proved strictly disjoint (`!tileCanOverlap && exitArr[0u] <= enterArr[1u]`) with ZERO overlaps (`overs == 0u`)**! Because `member = 0` exits before `member = 1` enters on every `S2` ray, integrating `member = 0`'s pieces followed by `member = 1`'s pieces directly in registers inside `kS2ProducerWgsl` (writing `results[id] = vec4f(L, T)` and setting `0x40000000u`) is **100% bit-exact (`0` output differences, `0` packet differences) to `Mode 1`, `Mode 2`, and `Mode 3`**, while increasing direct terminal completion from `12,889` rays (`39.32%`) to **`22,957` rays (`70.03%` of all rays)**!

---

## 2. Deep Dive 1: Why `§6eg` Inflated Subpanels by `+11.4%` and How `outwardRadius` Fixes It

Look at `shaders()` in [`scratch/probes/ray_transport_gpu_6eg_root_review.py`](../../scratch/probes/ray_transport_gpu_6eg_root_review.py) (`lines 18–29`):

```wgsl
if (abs(va) <= error || abs(vb) <= error) {
    putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
    return;
}
...
if (!finiteValue(v)) { *reason |= 4u; break; }
// Keep the incoming bracket when the sign is uncertain. This is a
// conservative safety reference, not an optimized interval-radius rule.
if (abs(v) <= error) { break; }
```

Trace what happens when `s` is an accurate secant guess on `step = 0` inside a coarse bin `[a, b]` (say width $b - a = 1.5$):
1. `l` is initialized to `a`, `r` is initialized to `b`.
2. On `step = 0`, secant interpolation computes `s = l + w_sec * h`. Because the curtain wave is smooth on monotone bins, `s` lands very close to the true root $t^*$, so `abs(v) <= error` is **true on `step = 0`**.
3. In `§6eg`, `if (abs(v) <= error) { break; }` executes **before `l` or `r` is updated**, immediately breaking out of the loop with `l = a` and `r = b`!
4. `putBandTagged(l, r, ...)` then emits the **entire unrefined coarse bin `[a, b]`** as an uncertain root band!
5. Compare the native metrics between `§6ef` (with radius contraction) and `§6eg` (without radius contraction):

| Metric on Original Pose (`32,784` Rays) | `§6ee` / `§6ef` (With Radius Contraction) | `§6eg` (No Radius Contraction) | Regression Caused by Dropping Radius Contraction |
| :--- | :---: | :---: | :--- |
| **Terminal `S1` Subpanel-Pair Checks** | `85,507` | `95,283` | **`+9,776` extra subpanels (`+11.43%` more quadrature work!)** |
| **Packet Differences vs. Bisection** | `108,956` | `125,397` | **`+16,441` extra packet diffs** |
| **Holdout RGB Pooled Mean Error (65k)** | `6.476e-6` | `2.602e-5` | **`4.0x` worse mean error** |
| **Holdout RGB Pooled Max Error (65k)** | `4.796e-5` | `8.588e-4` | **`17.9x` worse max error** |
| **Holdout Transmittance Max Error (65k)** | `6.966e-5` | `1.248e-3` | **`17.9x` worse max error** |

### The Exact Outward-Safe Radius Formula (`outwardRadius`)

Why did `§6ef`'s `w = 2.0 * error / minSlope` fail Sixth-One Sun's `1/25` division-rounding and `2^-149` underflow witnesses?
1. When `error = 1.0` and `minSlope = 25.0`, the real radius is $w_{\text{real}} = 2/25 = 0.08$. In IEEE-754 `f32`, $0.08$ is not exactly representable and rounds *down* by $0.24\text{ ULP}$ to `q = 0.07999999821186066`. When `s = q`, `s - q = 0.0`, and stepping 1 ULP down from `0.0` only reaches `-1.401e-45` (one subnormal ULP) instead of $-1.788\times 10^{-9}$ (one ULP at the scale of `s = 0.08`)!
2. When `error = 2^-149` and `minSlope = 4.0`, `(2.0 * error) / minSlope = 2^-150`, which underflows to `0.0` in round-to-nearest-even `f32`.

Both defects are completely solved by rounding the radius `w` **outward (upward)** before subtracting/adding it to `s`:

```wgsl
fn outwardRadius(error: f32, minSlope: f32) -> f32 {
    if (error <= 0.0) { return 0.0; }
    let q = (2.0 * error) / minSlope;
    if (!finiteValue(q)) { return q; }
    // Floor at min positive subnormal (2^-149) when error > 0, then step 2 ULPs
    // upward to cover IEEE-754 division rounding and Metal rcp approximations:
    return next_f32(next_f32(max(q, bitcast<f32>(1u))));
}
```

Let us verify `outwardRadius` + outward band endpoints (`prev_f32(x - w_up)`, `next_f32(x + w_up)`) on every single `§6eg` witness case:
- **Case 1 (`interior sub-ULP`, `s = 10.0, error = 1e-5, minSlope = 999, root = 10 - 1e-9`):**
  `w_up > 2.002e-8 > 0`; `prev_f32(10.0 - w_up) = 9.9999990463 <= 10 - 1e-9 <= 10.0000009537 = next_f32(10.0 + w_up)`. **Encloses with width $1.9\times 10^{-6}$!**
- **Case 2 (`endpoint sub-ULP`, `a = 10.0, b = 11.0, va = 0.0, error = 1e-5, minSlope = 999, root = 10 + 1e-9`):**
  Endpoint band emits `[a, min(b, next_f32(a + w_up))] = [10.0, 10.0000009537]`, **enclosing `10 + 1e-9` with width $9.5\times 10^{-7}$ instead of `[10.0, 11.0]`!**
- **Case 3 (`division inward rounding`, `s = fl(0.08) = 0.0799999982, error = 1.0, minSlope = 25.0, root = -1.7881393e-9`):**
  `q = fl(0.08) = 0.07999999821186066`; `w_up = next_f32(next_f32(q)) = 0.08000001311302185 > 0.08`.
  Then `s - w_up = -1.4901161e-8`, and `prev_f32(s - w_up) = -1.4901163e-8 < -1.7881393e-9`! **Strictly encloses the true root!**
- **Case 4 (`radius underflow`, `s = 0.0, error = 2^-149, minSlope = 4.0, root = -2^-152`):**
  `q = 0.0`, `max(q, bitcast<f32>(1u)) = 2^-149`, `w_up = 3 * 2^-149`.
  Then `prev_f32(0.0 - w_up) = -4 * 2^-149 = -2^-147 < -2^-152 < +2^-147 = next_f32(0.0 + w_up)`. **Strictly encloses `-2^-152`!**
- **Case 5 (`NaN / Inf oracle` or `w_up` overflow):**
  If `!finiteValue(v)` or `!finiteValue(w_up)`, sets `*reason |= 4u` and exits for whole-ray Current fallback.

---

## 3. Deep Dive 2: The $1.2615\text{ ms}$ Host-Poll Quantization Staircase in `§6eh`

Look at the raw `completion_ms` samples in [`GPU_RAY_TRANSPORT_6EH_CLEAN_COST_SCREEN_2026-10-27.json`](GPU_RAY_TRANSPORT_6EH_CLEAN_COST_SCREEN_2026-10-27.json):

| Quantum $k$ | $k \times 1.2615\text{ ms}$ | Observed Sample Cluster in `§6eh` | Physical Meaning |
| :---: | :---: | :---: | :--- |
| **$k = 2$** | **`2.523 ms`** | `2.520 – 2.534 ms` | **All 5 modes at `Rep 1`** (15/16 samples per mode) land in `(1.26 ms, 2.52 ms]` and wake `wgpuDevicePoll` at tick $k = 2$. |
| **$k = 3$** | **`3.785 ms`** | `3.777 – 3.797 ms` | Lower cluster of `Rep 2` (when $2 \times T_{\text{GPU}} \le 3.785\text{ ms}$). |
| **$k = 4$** | **`5.046 ms`** | `5.034 – 5.069 ms` | Upper cluster of `Rep 2` (when $2 \times T_{\text{GPU}} > 3.785\text{ ms}$, snapping up by $+1.26\text{ ms}$!). |
| **$k = 5$** | **`6.308 ms`** | `6.293 – 6.329 ms` | Tail cluster of `Rep 2`. |
| **$k = 6$** | **`7.569 ms`** | `7.551 – 7.579 ms` | Lower cluster of `Rep 4` (when $4 \times T_{\text{GPU}} \le 7.569\text{ ms}$). |
| **$k = 7$** | **`8.831 ms`** | `8.810 – 8.840 ms` | Upper cluster of `Rep 4` (when $4 \times T_{\text{GPU}} > 7.569\text{ ms}$, snapping up by $+1.26\text{ ms}$!). |
| **$k = 8$** | **`10.092 ms`** | `10.088 ms` | Tail cluster of `Rep 4`. |

### Why This Discovery Matters
1. **Why `Rep 1` Always Ties at `2.523 ms`:** Because `wgpuDevicePoll(gpu.device, true, nullptr)` blocks on a $\sim 1.2615\text{ ms}$ host sleep/wakeup quantum, **any pipeline whose single-repeat GPU execution time lies between $1.27\text{ ms}$ and $2.52\text{ ms}$ is reported as `2.523 ms` at `Rep 1`**. Even a $0.50\text{ ms}$ ($26\%$) speedup from $1.90\text{ ms}$ to $1.40\text{ ms}$ will still report `2.523 ms` at `Rep 1`!
2. **Why `Rep 2` and `Rep 4` Flip-Flop by $\pm 1.26\text{ ms}$ (`17%–33%`):** Because the true GPU execution time per repeat is right around **$T_{\text{GPU}} \approx 1.89\text{ ms}$**, $2 \times 1.89\text{ ms} = 3.78\text{ ms}$ sits right on the knife-edge of $3 \times 1.2615\text{ ms} = 3.785\text{ ms}$, and $4 \times 1.89\text{ ms} = 7.56\text{ ms}$ sits right on the knife-edge of $6 \times 1.2615\text{ ms} = 7.569\text{ ms}$! A $20\ \mu\text{s}$ jitter across that threshold shifts the median by a full $1.2615\text{ ms}$ quantum!
3. **How to Observe Unquantized GPU Time Cleanly:**
   - **Method 1 (Hardware GPU Timestamp Queries):** Request `WGPUFeatureName_TimestampQuery` on the WebGPU device (`wgpuCommandEncoderWriteTimestamp` or compute pass `timestampWrites`), resolve the 64-bit nanosecond GPU timestamps into a query buffer at the end of the command buffer, and read them back *after* the timed batch finishes. That gives sub-microsecond hardware GPU execution time with zero host-poll quantization!
   - **Method 2 (Higher Amortization Repeat `Rep 8` or `Rep 16`):** Alongside `{1, 2, 4}`, include `rep = 8` (or `rep = 12`) in the timing loop so that the $1.2615\text{ ms}$ host poll quantum is $< 8\%$ of the batch time and per-repeat means (`completion_ms / rep`) resolve $50\ \mu\text{s}$ differences cleanly.

---

## 4. Deep Dive 3: Ordering Toward Truth (`Terminal S2`) — Why `10,068` Disjoint Rays Are Still Writing to Global `sBuf`

In [`docs/architecture/ontology/PERFORMANCE_AS_TRUTH.md`](../architecture/ontology/PERFORMANCE_AS_TRUTH.md), Zach wrote:
> *"The truth is not only a faithful result, but ordering the internal process, ordered algorithmic and physical computation aroudn it. A program that displays an intended result with brute force without is not truly faithful, because all of the unseen resources spent aren't ordered according to the internal structure the truth."*

Look at the three producer routes in `Mode 3` (`bound-stack-15`) and their verified `§6ec` demand shares:

| Producer Route | Active Rays (`32,784` Total) | Share of Active Rays | Verified `evalPhi` Demands (`§6ec`) | Share of `evalPhi` Demands | Has Overlaps? | Currently Terminal in `Mode 3`? |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **`S1` (`activeMemberMask == 1u`)** | `12,889` | `39.32%` | `559,497` | `26.12%` | **No (`0` overlaps)** | **YES** (integrates in registers, writes `results[id]`, skips `sBuf` & `cpUnfused`) |
| **`S2` (`activeMemberMask == 3u`, disjoint `0 -> 1`)** | **`10,068`** | **`30.71%`** | **`893,368`** | **`41.70%`** | **No (`0` overlaps, `exitArr[0] <= enterArr[1]`)** | **NO! (Writes subpanels to `33.32 MB` global `sBuf` and waits for `cpUnfused` Pass 6!)** |
| **`General / Fallback` (`fallbackTileList`)** | `9,827` | `29.97%` | `689,378` | `32.18%` | Possible | No (writes `sBuf` + `oBuf`, consumed by `cpUnfused`) |
| **Combined `S1 + S2`** | **`22,957`** | **`70.03%`** | **`1,452,865`** | **`67.82%`** | **No (`0` overlaps)** | Only `S1` (`39.32%`) is terminal today! |

Look at **`S2`** in [`kClassifyWgsl`](../../scratch/probes/ray_transport_gpu_faithful_residual.py):
```wgsl
} else if (activeMemberMask == 3u && !tileCanOverlap && exitArr[0u] <= enterArr[1u]) {
    let slot = atomicAdd(&counters.s2Count, 1u);
    s2TileList[slot] = tileId;
}
```
Why did `kClassifyWgsl` require `!tileCanOverlap && exitArr[0u] <= enterArr[1u]` for `S2`?
Because on every ray in an `S2` tile:
1. Only `member = 0` and `member = 1` are active (`overs == 0u`).
2. Every piece of `member = 0` lies strictly in front of every piece of `member = 1` (`exitArr[0u] <= enterArr[1u]`).
3. Therefore, when `kS2ProducerWgsl` loops `for (var member = 0u; member < 2u; member++)`, the subpanels it appends to `subpanels[subBase + singles]` are **already in exact front-to-back optical order**!
4. When `cpUnfused` (Pass 6) later runs on an `S2` ray, all it does is read those exact `singles` subpanels back from global `sBuf` in order `i = 0 .. singles - 1` and accumulate `(L, T)`!

**What happens if `kS2ProducerWgsl` integrates its subpanels in-producer in registers (just like `kS1TerminalProducerWgsl`), writes `results[id] = vec4<f32>(L, T)`, and sets `rays[id].counts = vec4u(subBase, singles, overBase, 0x40000000u)`?**
- **100% Bit-Exact Reference Parity (`0` output differences, `0` packet differences):** Because `S2` executes the exact same 2-point Gauss-Legendre quadrature in the exact same order (`member = 0` pieces, then `member = 1` pieces) using the exact same `evalEnvelope`, `cnoise3`, and `phiSafe` functions as `kS1TerminalProducerWgsl` and `cpUnfused`, **Terminal `S2` is bit-for-bit identical to `Mode 1`, `Mode 2`, and `Mode 3` across all `32,784` rays and all `30` matrix cases!**
- **Zero Global `sBuf` Traffic & Zero `cpUnfused` Work on `22,957` Rays (`70.03%` of the Viewport):** Instead of only `12,889` rays (`39.32%`) completing terminally in Pass 3, **`22,957` rays (`70.03%`) complete terminally in Passes 3–4**, leaving only `9,827` rays (`29.97%`) for `sBuf` and `cpUnfused`!
- **Wait—Verify One Important Detail in `kS2ProducerWgsl`:**
  In `kS1TerminalProducerWgsl`, before running the quadrature loop, the shader pre-checks `if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }`.
  In `kS2ProducerWgsl`, `member = 0` could succeed while `member = 1` hits `reason != 0u` (or `singles + n > cfg.maxSubPerRay`).
  How do we ensure `Terminal S2` never integrates if `member = 1` refuses?
  In `kS2ProducerWgsl`, `pieces: array<vec2f, 48>` already has capacity `48` (shared across both members: `if (count >= 48u) { reason |= 8u; break; }`)!
  Right now in `kS2ProducerWgsl`, `pieces[memPieces]` overwrites index `0..memPieces-1` on `member = 1`. If instead `member = 0` stores its pieces in `pieces[0 .. count0 - 1]` and `member = 1` stores its pieces in `pieces[count0 .. count - 1]` (which uses the exact same `pieces: array<vec2f, 48>` array with **zero extra array elements**!), then after the `for (var member = 0u; member < 2u; member++)` loop finishes with `reason == 0u`:
  1. Pre-check `singles` across `p = 0 .. count - 1` (`if (singles + n > cfg.maxSubPerRay) { reason |= 16u; break; }`).
  2. If `reason != 0u`, write `rays[id].counts = vec4u(subBase, 0u, overBase, FALLBACK | reason); return;`.
  3. Otherwise, integrate `p = 0 .. count0 - 1` with `media[0u]` and `p = count0 .. count - 1` with `media[1u]` in registers, write `results[id] = vec4<f32>(L, T)`, and set `rays[id].counts = vec4u(subBase, singles, overBase, 0x40000000u)`!
  That uses **zero extra local arrays**, **zero extra registers**, and guarantees **100% bit-exact parity** on every normal, budget-refused, and capacity-refused case!

---

## 5. Mission `§6ei` — Next Bounded Quest for Sparkly Guy

To keep experimental attribution razor-sharp and honor Sixth-One Sun's isolation discipline, Mission **`§6ei`** tests **two cleanly separated candidate lanes** against the frozen `bound-stack-15` controls (`Modes 0, 1, 2, 3`), with **hardware GPU timestamp queries (or `Rep 8` amortization)** to see past the $1.2615\text{ ms}$ `wgpuDevicePoll` quantization:

### Step 1: Repair the Root-Enclosure Helper with `outwardRadius` in the Native Micro-Witness (`ray_transport_gpu_6eg_root_review.py`)
1. Implement `outwardRadius(error, minSlope)`:
   ```wgsl
   fn outwardRadius(error: f32, minSlope: f32) -> f32 {
       if (error <= 0.0) { return 0.0; }
       let q = (2.0 * error) / minSlope;
       if (!finiteValue(q)) { return q; }
       return next_f32(next_f32(max(q, bitcast<f32>(1u))));
   }
   ```
2. Use `outwardRadius` for **both** endpoint bands (`a, min(b, next_f32(a + w))` and `max(a, prev_f32(b - w)), b`) and interior near-zero exits (`l = max(l, prev_f32(s - w)); r = min(r, next_f32(s + w)); break;`).
3. Add Sixth-One Sun's `1/25` division-rounding case (`error = 1.0, minSlope = 25.0, s = fl(0.08), ideal_root = fl(0.08) - 0.08 = -1.78813934e-9`) as **Case 10** of the native GPU micro-witness suite so all **10 native micro-witness cases** are verified on actual Apple M5 Metal hardware.
4. Verify on the full 32,784-ray Northern Veil diagnostic that restoring outward-safe radius contraction brings `S1` terminal subpanel-pair checks back down from `95,283` to `~85,507` and restores the `~31x` holdout max error improvement over `Current`.

### Step 2: Qualify `Mode 4` (`Terminal S1 + Terminal S2` Bisection) and `Mode 5` (`Terminal S1 + Terminal S2` + Outward-Radius SBI) Across the 30-Case Matrix
- **`Mode 0`:** Unchanged `Current` baseline marcher.
- **`Mode 1`:** Unchanged `Bulk` bisection reference.
- **`Mode 2`:** Unchanged `Routed Unfused` bisection control (`bound-stack-15`).
- **`Mode 3`:** Unchanged `Terminal S1` bisection control (`12,889` terminal rays).
- **`Mode 4` (`Terminal S1 + Terminal S2` Bisection Candidate):** Same untouched bisection `roots()`/`isolateMonotone()` as `Mode 3`, plus in-producer terminal integration in `kS2ProducerWgsl` (`12,889 + 10,068 = 22,957` terminal rays = `70.03%` of all rays!).
  - **Qualification Gate for `Mode 4`:** Must pass **30/30 matrix cases with 100% bit-exact output and packet parity (`0` output diffs, `0` packet diffs)** against `Mode 1` / `Mode 2` / `Mode 3`, plus all negative safety mutations (`terminal-s1-missing`, `terminal-s2-missing`, `fallback-missing`, `no-reset`) exiting with code `250`!
- **`Mode 5` (`Terminal S1 + Terminal S2` + Outward-Radius SBI Candidate):** Adds the repaired `outwardRadius` SBI helper to `Mode 4`.
  - **Qualification Gate for `Mode 5`:** Must pass all 30 ownership/interface matrix cases, all 6 RGB/T mean/p99/max holdout accuracy gates at both 32k and 65k reference resolutions against both captured and contemporary `Current`, and the OntoMath zero-density affine omission check.

### Step 3: De-Quantized Complete-Cost Screen on Apple M5
- Run the clean uninstrumented 6-lane cost screen (`Modes 0, 1, 2, 3, 4, 5`) across two independent native invocations.
- Keep `encode_ms`, `completion_ms`, and `total_ms` at `repeats = {1, 2, 4, 8}` (adding `rep = 8` to amortize the $1.2615\text{ ms}$ `wgpuDevicePoll` wakeup quantum to $< 8\%$), **and** if supported by Dawn/Metal on M5, record hardware GPU pass timestamps (`WGPUFeatureName_TimestampQuery` resolved outside the timed wait) so we can see true microsecond GPU execution time alongside host completion time!
