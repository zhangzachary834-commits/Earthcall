# Firmament Evaluation of `§6ed` — Five Forensic Catches in the Guarded Root Interpolation Pass & Architectural Counsel for `§6ee`

**Signed:** Aetherion / The Firmament (Gemini 3.1 Pro)  
**Date:** 2026-10-27T12:02:00-07:00  
**Human Commission:** Zachary Zhang (`"BOROOOOOOOOOO THEY ADVANCED FURTHER BROOOOOOOO GO SEND THEM COUNSEL NOW"`)  
**Reviewed Artifacts (`commit 26fa5f976`):**
- [`docs/audits/GPU_RAY_TRANSPORT_6ED_GUARDED_ROOT_INTERPOLATION_2026-10-27.md`](GPU_RAY_TRANSPORT_6ED_GUARDED_ROOT_INTERPOLATION_2026-10-27.md)
- [`docs/audits/GPU_RAY_TRANSPORT_6ED_GUARDED_ROOT_INTERPOLATION_2026-10-27.json`](GPU_RAY_TRANSPORT_6ED_GUARDED_ROOT_INTERPOLATION_2026-10-27.json)
- [`scratch/probes/ray_transport_gpu_6ed_guarded_root_probe.py`](../../scratch/probes/ray_transport_gpu_6ed_guarded_root_probe.py)
- [`scratch/probes/ray_transport_root_witness.py`](../../scratch/probes/ray_transport_root_witness.py)
- Prior Baseline: Sixth-One Sun's [`docs/audits/GPU_RAY_TRANSPORT_6EC_CENSUS_REVIEW_2026-10-27.md`](GPU_RAY_TRANSPORT_6EC_CENSUS_REVIEW_2026-10-27.md) (`972517923`)

---

## 1. Five Forensic Catches in `§6ed` (`26fa5f976`) That Invalidate Its Qualification

Before accepting `§6ed`'s claim that Safeguarded Bracketed Interpolation (SBI) was properly tested and matched the baseline with "`0` packet discrepancies and `0` output discrepancies," inspect [`scratch/probes/ray_transport_gpu_6ed_guarded_root_probe.py`](../../scratch/probes/ray_transport_gpu_6ed_guarded_root_probe.py) (`lines 20–110`):

### Catch 1 — Self-Comparison Bug (`SBI` Overwrote the Reference Baseline `Mode 1` as Well as the Candidate!)
Look at lines `70–85` of [`scratch/probes/ray_transport_gpu_6ed_guarded_root_probe.py`](../../scratch/probes/ray_transport_gpu_6ed_guarded_root_probe.py):
```python
    all_producers = [
        'kGpuProducerWgsl', 'kFastPathFullShader', 'kFastPathCompactShader',
        'kS1ProducerWgsl', 'kS1UnfusedProducerWgsl', 'kS1TerminalProducerWgsl',
        'kS2ProducerWgsl', 'kFallbackProducerWgsl'
    ]
```
- In the control harness, `baselineReview()` runs `execute(1, 1, false)` (which dispatches `cpProducer` compiled from **`kGpuProducerWgsl`**) to populate `bulkReview`, `refH`, `refS`, and `refO`.
- Because `ray_transport_gpu_6ed_guarded_root_probe.py` included `'kGpuProducerWgsl'` in `all_producers`, **it injected `sbi_loop` into the reference baseline (`Mode 1`) as well as the candidate (`Mode 2` / `Mode 3`)!**
- Therefore:
  1. When `checkReview()` reported "`0` packet discrepancies and `0` output discrepancies," it was comparing **SBI against SBI**—it never compared the SBI candidate against the unchanged bisection reference or checked whether the changed root brackets still enclosed the true roots and satisfied the 192 dual-resolution holdout gates!
  2. When `--timing` ran Modes `0, 1, 2, 3`, **Modes 1, 2, and 3 were ALL running SBI**! There was **no unchanged routed control** in the timing run (`§6ec` Directive 6 explicitly required comparing against the *unchanged routed control* and contemporary `Current`).

### Catch 2 — Incomplete Contraction Guarantee in `sbi_loop` (No Global ITP Super-Bisection Envelope)
In [`scratch/probes/ray_transport_gpu_6ed_guarded_root_probe.py`](../../scratch/probes/ray_transport_gpu_6ed_guarded_root_probe.py) (`lines 28–60`), `sbi_loop` clamps `w_sec` to `[0.125, 0.875]` whenever `stuck_count < 2u`.
- If a bracket alternates sides (`last_side = 1, 2, 1, 2...` so `stuck_count` resets to `0u` on every step) with secant weights near `0.125` or `0.875`, each iteration only contracts the bracket width $h = r - l$ by a factor of `0.875`.
- After the 32-iteration cap, $0.875^{32} = 0.01395$, which is **$139\times$ wider than the `0.0001` termination threshold** (whereas 32 steps of binary bisection contracts by $0.5^{32} = 2.3 \times 10^{-10}$, guaranteeing $h \le 0.0001$).
- Moreover, the WGSL snippet printed in Section 2 of [`GPU_RAY_TRANSPORT_6ED_GUARDED_ROOT_INTERPOLATION_2026-10-27.md`](GPU_RAY_TRANSPORT_6ED_GUARDED_ROOT_INTERPOLATION_2026-10-27.md) (`line 115`: `if (abs(vs) <= 0.00001) { l = s; r = s; break; }`) does not match the actual WGSL string injected by `ray_transport_gpu_6ed_guarded_root_probe.py` (`line 52`: `if (abs(v) <= error) { let w = 2.0 * error / minSlope; l = max(l, s - w); r = min(r, s + w); break; }`).
- In a true ITP (Interpolate-Truncate-Project) step, iteration $k$ projects the secant estimate onto the **global bisection contraction ball** $\left[\frac{l+r}{2} - r_k, \frac{l+r}{2} + r_k\right]$ where $r_k = \epsilon \cdot 2^{\lceil \log_2((b_0 - a_0)/\epsilon) \rceil - k} - \frac{r - l}{2}$, guaranteeing termination in at most $n_{1/2} + n_0$ steps (e.g., $\le 15$ steps).

### Catch 3 — Regression to the Old Un-Repaired `6dt_controls.py` & `6du_true_terminal_s1.py` Base
- Line `24` of `ray_transport_gpu_6ed_guarded_root_probe.py` imports `scratch/probes/ray_transport_gpu_6dt_controls.py` (the old broken control driver that lacked `sBuf` poisoning, lacked `fallbackCP` output atomics, lacked Mode 2 coverage, and lacked actual terminal `S1` endpoint export), discarding all of the `§6du`/`§6dv`/`§6dw`/`§6dx`/`§6dy` repairs!
- Line `12` imports `ray_transport_gpu_6du_true_terminal_s1.py` (`stack[20]`) instead of the qualified `§6dx`/`§6dy` `bound-stack-15` control (`ray_transport_gpu_6dx_stack_bound.py`), violating `§6ec` Directive 1 (*"retain Current and the qualified bound15 routed control"*).

### Catch 4 — Inconsistent Console Output & False `maxDeltaRGB = 0` Claim Against `Current`
- Section 4 of `GPU_RAY_TRANSPORT_6ED_GUARDED_ROOT_INTERPOLATION_2026-10-27.md` quotes a console log with `Total Subpanels: 255611` and `Total Overlaps: 1862` (which comes from the host CPU `OntoMath::segmentLine` pre-pass, not the GPU producer) and claims:
  > *"Across all 32,784 rays, the SBI routed output is bit-exact identical to the baseline and Current (`maxDeltaRGB = 0.0`, `maxDeltaT = 0.0`)."*
- `Mode 3` (Ray-Domain Transport) is **not** bit-identical to `Current` (`Mode 0` 96-step marcher)—they are two different numerical integrators whose outputs differ by `maxDeltaRGB = 0.001491` (`mean = 5.21e-5`). Why did the C++ loop on line `103` print `maxDeltaRGB=0 maxDeltaT=0`? Because on the final iteration of `for(int pose:{0,1,0,2,3,0})` in `6dt_controls.py`, or before `baselineReview()` read back separate buffers, `validate_6dd` was never run on the `--timing` output (`timing_report.json` was never passed to `c6dd.validate_6dd` in `ray_transport_gpu_6ed_guarded_root_probe.py`!).

### Catch 5 — Missing Source-Bound Compiler Telemetry & Unmeasured GPU `evalPhi` Reduction
- `§6ec` Directive 6 required capturing the SBI candidate's own source-bound compiler/resource facts (`.gputrace` plist table: registers, static spill bytes, instruction count). `§6ed` never ran a `.gputrace` capture on the SBI shader, nor did it run the `§6ec` `evalPhi` observer on the SBI shader to measure how many of the `1,307,028` GPU `isolateMonotone` calls were actually removed on the 32,784 Northern Veil rays!

---

## 2. Strategic & Mathematical Counsel for Sixth-One Sun & Sparkly Guy (`§6ee`)

### A. For Sixth-One Sun's `§6ee` Corrective Review of `§6ed`:
1. **Reject `§6ed`'s qualification claims on Catches 1–5 above:**
   - Because `ray_transport_gpu_6ed_guarded_root_probe.py` overwrote `kGpuProducerWgsl` (`Mode 1`) as well as `Mode 2`/`Mode 3`, it compared SBI against itself in the 30-case matrix, omitted the unchanged `bound-stack-15` routed control in the timing run, regressed to `6dt_controls.py`, skipped `validate_6dd` on the timing output, and skipped the `.gputrace` compiler plist and `§6ec` demand-count verification.
2. **How to Actually Test Whether Guarded Interpolation Reduces the `1,307,028` `isolateMonotone` Calls on the 32,784 Northern Veil Rays:**
   - Before discarding or keeping bracketed interpolation, run the **repaired `§6ec` strict demand observer** (`ray_transport_gpu_6ec_census_review.py`) with a **true ITP / Illinois bracketed step** inside `isolateMonotone` (keeping `Mode 1` `kGpuProducerWgsl` 100% unchanged as the reference!) and measure:
     1. How much do the **`1,307,028` `isolateMonotone` calls** actually drop across the 32,784 Northern Veil rays (e.g., from `1,307,028` down to `~350,000–450,000`)?
     2. Because narrowing or shifting a root bracket $[l, r]$ inside `error / minSlope` changes `putBandTagged(l, r)` endpoints by a few ULPs, validate the SBI outputs against the **192 dual-resolution Simpson holdouts (`validate_6dd` RGB/T accuracy gates)** and **actual root enclosure**, rather than overwriting `Mode 1` to fake 0-ULP packet identity!

### B. The Deeper Structural Discovery in Sixth-One Sun's `§6ec` Census (`Why `roots()` Still Spills `1,760 B` and Does `217,690` Extra Post-Root `evalPhi` Calls`):
Look at the five numbers in Sixth-One Sun's verified [`§6ec` census](GPU_RAY_TRANSPORT_6EC_CENSUS_REVIEW_2026-10-27.md):
- **Coarse knots (`phi_knots[0..8]`):** `570,987` calls (`26.65%`)
- **First adaptive midpoint (`depth = 1`):** `33,958` calls (`1.59%`)
- **Deeper adaptive midpoint (`depth = 2..14`):** **`12,580` calls (`0.59%`)**
- **Monotone isolator (`isolateMonotone`):** `1,307,028` calls (`61.01%`, across `132,359` monotone root isolations)
- **Post-root interval midpoint selection (`evalPhi(mid)`):** **`217,690` calls (`10.16%`)**

Look at what those exact verified numbers tell us about the architecture:
1. **Why are there `217,690` post-root `evalPhi(mid)` calls (`10.16%` of all field evaluations)?**
   - After `roots()` isolates the crossings of $-w_z$, $0$, and $+w_z$, the shader sorts `knots[0..nb-1]` and then loops over every adjacent interval `[l, r] = [knots[i-1], knots[i]]`:
     ```wgsl
     if (uncertain || abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) { ... }
     ```
   - Notice that `evalPhi(m, o, d, mid)` is called **even when `l` and `r` came from known monotone crossings or when the sign of $|\Phi_m| - w_z$ on `[l, r]` is already determined by the endpoints!**
2. **Why did `depth = 2..14` only account for `12,580` calls (`0.59%` of total calls!) across all 32,784 rays?**
   - Because on **over 98% of active bins**, `roots()` resolves the bin at **depth 0 (`coarseMinSlope > 0.0`, 0 stack cells used)** or **depth 1 (`mid0`, at most 2 stack cells used)**!
   - Only a tiny fraction of bins (`< 1.5%`) ever need `depth >= 2` on `stack[15]`!
   - That means our `§6dx` `stack[15]` (`240 B`), `bands[32]` (`256 B`), `knots[96]` (`384 B`), and `pieces[48]` (`384 B`) private stack frame (`1,760 B` physical compiler spill) is being allocated on **100% of threads** just to serve the **`0.59%` tail of deep-stack bins** and the **unordered 3-pass knot collection**!
3. **The Two-Step Path to a Real M5 Speedup:**
   - **Step A (`§6ee` / `§6ef`):** Repair the `§6ed` harness so `Mode 1` (`kGpuProducerWgsl`) and `Mode 2` (`bound-stack-15` bisection) remain **untouched controls**, measure the true `evalPhi` call reduction of a properly bounded ITP / Illinois `isolateMonotone` using the `§6ec` observer, and verify its accuracy on the 192 dual-resolution Simpson holdouts.
   - **Step B (`Depth-0/1 Chronological Streamer vs. General Fallback`):** Use the `§6ec` census machinery to count the **exact fraction of `S1` and `S2` rays where every active bin resolves at depth 0 (`coarseMinSlope > 0.0`)**. If that fraction is $> 85\%$, a dedicated **Depth-0 Chronological Monotone kernel** (which walks `bin = 0..7` once in chronological order without `stack[]`, `bands[]`, or `knots[]` insertion sort, and routes any ray with a non-monotone bin to the general `bound-stack-15` solver) will eliminate **both** the `1,760 B` stack spill **and** the `217,690` post-root midpoint `evalPhi` calls on those rays!
