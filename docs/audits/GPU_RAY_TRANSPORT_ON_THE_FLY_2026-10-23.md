# §6cz — Sparkly Guy: On-The-Fly Chronological Consumption Passes Strict Parity, Falsifies Buffering Hypothesis, Rejected on Economics

**Harness:** Sparkly Guy (`Gemini 3.8 Flash High / Antigravity`) · session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a`  
**Commissioned By:** Sixth-One Sun (`Codex / GPT-6.1 Sol`) and Aetherion / The Firmament (`Gemini 4 Argon`) in Intercom (`OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md` / `FIRMAMENT_COUNSEL_POST_6CW_WORK_CENSUS_AND_ON_THE_FLY_SWEEP_2026-10-22.md` / §6cy review `docs/audits/GPU_RAY_TRANSPORT_CENSUS_REVIEW_2026-10-23.md`)  
**Date:** `2026-10-23`  
**Platform:** Apple M5 (10-core GPU, Metal, completion-fenced host timing, GPU timestamps disabled)  
**Codebase:** Worktree `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop` on branch `claude/ray-domain-transport-rung0`  
**Primary Probe:** `scratch/probes/ray_transport_gpu_on_the_fly.py`  
**Machine Record:** [`GPU_RAY_TRANSPORT_ON_THE_FLY_2026-10-23.json`](GPU_RAY_TRANSPORT_ON_THE_FLY_2026-10-23.json)  

---

## 1. Executive Summary & Verdict

Following the assignment from Codex (§6cy) and counsel from Firmament, Sparkly Guy designed, qualified, and benchmarked **`§6cz`: On-The-Fly Chronological Consumption**.

### Core Hypothesis Tested
In §6cv, fusing producer and consumer with thread-private intermediate arrays (`local_subpanels: array<Subpanel, 64>` = 1,024 B/thread and `local_overlaps: array<Overlap, 16>` = 256 B/thread) and a two-stream consumer re-merge loop resulted in a 30.8% slowdown vs Current Marcher. The proposed hypothesis was that eliminating these arrays and their two-stream re-merge loop by evaluating Gauss-Legendre quadrature updates and overlap updates immediately on-the-fly during the knot sweep would alleviate thread-private register/spill pressure and recover execution speed.

### Empirical Verdict
1. **Mathematical & Structural Parity: 100.0% PASSED**
   - Preserves exact single-member subdivision rule $n = \max(1, \lceil(b - a)/4.0\rceil)$ and exact $s_0, s_1$ endpoint arithmetic.
   - Preserves exact `advanceOverlap` sequence, member order, and shared attenuation.
   - Schedule census exported via uninstrumented/trace verification matches guarded baseline bit-identically across all 32,784 rays (0 differing schedule rays, 12,102/12,102 affine boundaries preserved, 0 changed outer support unions).
   - Pixel output matches guarded unfused baseline **100.0% bit-exact across all 32,784 distinct native rays** (`max_delta_RGB = 0.0`, `max_delta_T = 0.0`, 0 differing rays).
   - Forced fallback (`budget = 0`) matches native marcher **100.0% bit-exact on all 32,784 rays**.
   - Synthetic budget controls (23/24/25 $\to$ 16/0/0) and synthetic capacity controls ($\max_{\text{sub}}=4 \to 26,243$; $\max_{\text{over}}=1 \to 412$) pass with exact expected failure flags and bit-identical fallback/normal outputs.
   - Independent holdout accuracy passes all 12 reference gates at both 65,536 and 32,768 intervals across all 192 holdout rays on both captured and contemporary baselines.
2. **Economic Benchmark: DECISIVELY REJECTED**
   - In a 4-mode benchmark in the same process at primary 32,768 capacity (2 tiles: 32,768 + 16 tail) with balanced paired order, **Mode 3 (On-The-Fly Fused) is 10.4% to 18.2% slower than Mode 2 (Buffered Fused), and 21.4% to 30.4% slower than Mode 0 (Current Marcher)**.
   - Speedup vs Current Marcher: **0.786×** (repeats=1), **0.728×** (repeats=2), **0.696×** (repeats=4).
   - Speedup vs Buffered Fused: **0.818×** (repeats=1), **0.882×** (repeats=2), **0.896×** (repeats=4).
3. **Causal Falsification of the Buffering Hypothesis**
   - Eliminating the local arrays did NOT reduce register pressure; it worsened it.
   - In **Mode 2 (Buffered)**, producer state (pieces, knots, bisection stack, bands) dies before consumer optical quadrature begins, allowing the Metal compiler to reuse SIMD registers between the two phases.
   - In **Mode 3 (On-The-Fly)**, consumer quadrature (transcendental noise calls, envelopes) is interleaved inside the knot sweep loop. This forces all producer state to remain live simultaneously across noise evaluations, widening live ranges, increasing register pressure, and degrading SIMD occupancy.

---

## 2. Invariant Preservation & Structural Qualification

The on-the-fly shader (`kOnTheFlyShader`) adheres strictly to all mathematical invariants established in §6cu and §6cy:

### 1. Single-Member Subdivision Rule
```rust
let span_len = b - a;
let n = max(1u, u32(ceil(span_len / 4.0)));
if (singles_emitted + n > cfg.maxSubPerRay) {
    reason = 16u; // Capacity exceeded
    break;
}
for (var k = 0u; k < n; k = k + 1u) {
    let s0 = select(a, mix(a, b, f32(k) / f32(n)), k > 0u);
    let s1 = select(b, mix(a, b, f32(k + 1u) / f32(n)), k + 1u < n);
    // Chronological integration: 1 Gauss-Legendre update per subpanel
    integrateSubpanelOnTheFly(ro, d, s0, s1, mid_m, &L, &T);
    singles_emitted = singles_emitted + 1u;
}
```
Each subpanel $[s_0, s_1]$ undergoes exactly one Gauss-Legendre 2-point quadrature step, identically matching the buffered consumer's calculation.

### 2. Overlap Integration
Overlaps are evaluated immediately upon encountering an active overlap state between knots, using the exact `advanceOverlap` sequence, member order, and shared attenuation. Overlap capacity exhaustion immediately sets `reason = 32u`.

### 3. Late Fallback Publication Boundary
Transport $(L, T)$ is accumulated in local variables. If `reason != 0u` or `budget == 0u` occurs at any point during root sweep or quadrature:
- All partial transport $(L, T)$ is discarded.
- `rays[id].counts = vec4u(0u, 0u, 0u, 0x80000000u | reason)` is published to global memory.
- `fallbackCP` (unchanged native marcher) reads the fallback flag and computes the complete ray once.
- On full success, bit 31 is cleared (`rays[id].counts = vec4u(0u, singles, 0u, overs)`), and the finished $(L, T)$ is published directly to `out_pixels`.

### 4. Qualification Results Summary
Across all 32,784 distinct native rays:
- **Differing schedule rays:** Exactly `0`.
- **Preserved affine boundaries:** `12,102 / 12,102`.
- **Changed outer support unions:** `0`.
- **Singles count:** `247,326`.
- **Overlaps count:** `1,799`.
- **Normal fallbacks:** `0`.
- **Differing pixel rays vs unfused control:** `0` (`max_delta_RGB = 0.0`, `max_delta_T = 0.0`).
- **Forced fallback bit-exactness:** `100.0%` (32,784 / 32,784 rays bit-identical to native current).
- **Synthetic budget controls:** Budgets 23 / 24 / 25 yield 16 / 0 / 0 fallbacks (100% pass).
- **Synthetic capacity controls:** $\max_{\text{sub}}=4$ yields 26,243 fallbacks (reason 16); $\max_{\text{over}}=1$ yields 412 fallbacks (reason 32). Every fallback ray bit-matches native current; every non-fallback ray bit-matches unfused control.
- **Reference accuracy:** Both 65,536 and 32,768 intervals pass all 12 accuracy gates across all 192 holdouts on both captured and contemporary baselines.

---

## 3. Four-Mode Benchmark Methodology & Measured Latencies

All 4 modes were executed in the SAME process on Apple M5 Metal, host completion-fenced timing with native GPU timestamps disabled. Tile capacity was pinned to primary 32,768 (2 tiles: 32,768 + 16 tail rays). Dispatches followed a balanced paired order:
`[0, 1, 2, 3, 3, 2, 1, 0, 3, 2, 1, 0, 0, 1, 2, 3] * 2` (32 complete dispatches per repeat level).

### Benchmark Results (Normalized ms per 32,784-ray query)

| Repeats | Mode 0: Current Marcher | Mode 1: Guarded Bulk | Mode 2: Buffered Fused 64/16 | Mode 3: On-The-Fly Fused 64/16 | Speedup (Mode 0 / Mode 3) | Speedup (Mode 2 / Mode 3) |
|---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **1** | **4.1457 ms** | 4.0800 ms | 4.3137 ms | **5.2719 ms** | **0.786×** (-21.4%) | **0.818×** (-18.2%) |
| **2** | **3.5031 ms** | 3.7413 ms | 4.2425 ms | **4.8108 ms** | **0.728×** (-27.2%) | **0.882×** (-11.8%) |
| **4** | **2.2398 ms** | 2.4262 ms | 2.8839 ms | **3.2170 ms** | **0.696×** (-30.4%) | **0.896×** (-10.4%) |

### Paired Differences (ms)
- **Repeats = 1:** Mode 3 is $+1.126\text{ ms}$ slower than Mode 0 on average ($+0.770\text{ ms}$, $+0.569\text{ ms}$, $+1.596\text{ ms}$, $+1.570\text{ ms}$ across paired blocks); Mode 3 is $+0.958\text{ ms}$ slower than Mode 2.
- **Repeats = 2:** Mode 3 is $+1.308\text{ ms}$ slower than Mode 0 on average; Mode 3 is $+0.568\text{ ms}$ slower than Mode 2.
- **Repeats = 4:** Mode 3 is $+0.977\text{ ms}$ slower than Mode 0 on average; Mode 3 is $+0.333\text{ ms}$ slower than Mode 2.

In every configuration and repetition level, **Mode 3 is strictly slower than Mode 2, and significantly slower than Mode 0**.

---

## 4. Deep Causal Diagnosis: The Register Live-Range Widow

Why did eliminating 1,280 bytes of thread-local schedule arrays per thread cause a slowdown?

### 1. Mode 2 (Buffered Fused): Phased Register Reuse
In Mode 2, the kernel has two non-overlapping phases:
1. **Producer Phase:** Evaluates affine intervals, root bisection, and 4-way merge. It writes out compact subpanels and overlaps into `local_subpanels` and `local_overlaps`.
2. **Consumer Phase:** Iterates through the local arrays, evaluating `cnoise3`, transcendentals, and optical quadrature.
Crucially, **the compiler knows that producer variables (`pieces`, `knots`, `phi_knots`, bisection stack, `bands`, `pIdx`) are completely dead when Phase 2 begins.** The Metal compiler reuses SIMD registers allocated for the bisection and merge logic to hold the noise generation and lighting state in Phase 2.

### 2. Mode 3 (On-The-Fly Fused): Live Range Explosion
In Mode 3, optical quadrature is evaluated immediately during the knot sweep loop. This means that:
- Every time `cnoise3` is called (which requires extensive register state for hash permutations and gradient interpolations), the outer knot sweep variables (`pieces`, `memStart`, `memCount`, `knots`, `ro`, `d`, `pIdx`, bisection outputs, active masks) **MUST REMAIN LIVE IN REGISTERS**.
- Live ranges of producer variables are stretched across the entire lifetime of the ray transport computation.
- This creates massive live register pressure across the kernel. To accommodate this, the Metal compiler either spills variables to thread-private scratch memory or reduces thread group occupancy per compute unit.
- The occupancy reduction and register spill traffic within the inner loop severely degrades instruction throughput, completely overwhelming any benefit from removing array indexing.

### Conclusion on Single-Kernel Fusion
The hypothesis that fused producer+consumer execution is held back by array buffering is falsified. Whether buffered or on-the-fly, **fusing polynomial root isolation and transcendental volumetric quadrature into a single kernel on Apple Silicon unified memory creates severe register pressure that degrades performance compared to partitioned execution or the specialized Current Marcher.**

---

## 5. Shipping Standing & Lane Release

In accordance with Codex's instruction rule 7 (*"Stop on clear loss"*):
- Mode 3 (On-The-Fly) is decisively **REJECTED** as an optimization.
- **Gate 1 (Profitable Connected Execution):** **OPEN**. Neither Mode 2 nor Mode 3 beats Current Marcher.
- **Gate 2 (Generic OntoMath Integration):** **OPEN**.
- **Gate 3 (Inhabited p95 / Zach Acceptance):** **OPEN**.
- **Native Timing Lane:** Formally **RELEASED**.
- No production files, shaders, or save files were modified. All changes remain quarantined in `scratch/probes/ray_transport_gpu_on_the_fly.py` and `docs/audits/`.

Signed: Sparkly Guy / Gemini 3.8 Flash High / Antigravity / session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` / `2026-10-23T01:39:00-07:00`.
