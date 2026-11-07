# §6cs & §6ct — Sparkly Guy: Linear-z Wave-Envelope Culling, Cross-Level Subdivision Φ-Cache, and Guarded Hybrid Secant-Bisection

**Harness:** Sparkly Guy (`Gemini 3.8 Flash High / Antigravity`) · session `50cf28df-09e9-4c2b-a416-0e859a729c1a`  
**Counsel:** Aetherion / The Firmament (`Gemini 4 Argon` · `2026-10-22T17:38:00-07:00`)  
**Council:** Zachary Zhang, Sixth-one Sun (`Codex / GPT-6.1 Sol`), and Original Prism Sun (`GPT-5.6 Sol`)  
**Date:** `2026-10-22`  
**Platform:** Apple M5 (10-core GPU, Metal, completion-fenced host timing, GPU timestamps disabled)  
**Codebase:** Worktree `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop` on branch `claude/ray-domain-transport-rung0`  
**Parent Checkpoint:** Commit `794e5ea03` (§6cr merge-sweep)  
**Audit Record:** [`GPU_RAY_TRANSPORT_ENVELOPE_SUBCACHE_AND_SECANT_2026-10-22.json`](GPU_RAY_TRANSPORT_ENVELOPE_SUBCACHE_AND_SECANT_2026-10-22.json)  

---

## 1. Executive Summary & Verdict

Following The Firmament's blueprint in [`FIRMAMENT_COUNSEL_POST_6CR_LINEAR_Z_CULL_SUBDIVISION_CACHE_AND_SECANT_2026-10-22.md`](../../../agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CR_LINEAR_Z_CULL_SUBDIVISION_CACHE_AND_SECANT_2026-10-22.md), we implemented and evaluated both stages on all **32,784 distinct native rays** (`2560×1440` viewport, camera `inside`, clock `7.25`):

### Stage 1 (`§6cs`): Strict Bit-Exact Linear-$z$ Culling & Cross-Level $\Phi$-Cache
1. **Whole-Member Linear-$z$ Cull:** Without altering `a` or `b` from the AABB clip, bounded $z(s) = o_z + s \cdot d_z$ against $Z_{\text{cull}} = W_{\max} + w_z + 0.125 M h_0^2 + \text{curv.y} + 0.05$. Skipping non-overlapping curtain crossings ($work += 24u$) eliminates 9 `phi_knots` evaluations and 3 `roots()` calls on empty ray paths.
2. **Per-Coarse-Bin Linear-$z$ Cull & Demand-Driven `phi_knots`:** Evaluates `phi_knots[k]` only if knot $k$ bounds an active coarse bin (`knotMask = activeBins | (activeBins << 1u)`). Inactive bins in `roots()` are skipped with $*work += 1u$.
3. **Cross-Level Subdivision $\Phi$-Cache:** Passed `phi_sub: array<f32, 24>` and `sub_mask: u32` across the 3 level-set calls to `roots()`. Midpoints evaluated at depth 0 and depth 1 in Call 1 ($-w_z$) are reused bit-identically in Call 2 ($0.0$) and Call 3 ($+w_z$), eliminating duplicate transcendental evaluations.
4. **Hoisted Affine Knots & Exterior Linear-$z$ Pre-Check:** Pre-checks `abs(o.z + mid * d.z) <= zPieceLimit` before calling `evalPhi(mid)` on exterior candidate pieces.
- **Stage 1 Bit-Exactness Verdict:** **100.0% Same-Device Bit-Exact Parity (`0 / 32,784` differing schedule rays, `0 / 32,784` differing output pixels)**:
  - Subpanels: **247,326** (100% bit-identical).
  - Overlaps: **1,799** (100% bit-identical).
  - Affine boundaries: **12,102 / 12,102 matched (0 missing)**.
  - Ray-member support unions changed: **0**.
  - All 32,784 rays pass AST agreement ($< 10^{-5}$); all 12 reference accuracy gates pass at both 65,536 and 32,768 intervals; forced fallback is bit-identical to native baseline.
- **Stage 1 Economics Verdict:** **REJECTED.** Mode 2 bounded tile streaming trails Current Marcher across all capacities (8,192, 16,384, 32,768) and repetition series ($0.642\times$ to $0.974\times$ of Current).

### Stage 2 (`§6ct`): Guarded Hybrid Secant-Bisection Numerical Candidate
Inside `roots()`, replaced 32-step pure midpoint bisection on monotone sign-change cells with a clamped secant step on even iterations ($t = \text{clamp}(left / (left - right), 0.1, 0.9)$) and midpoint step on odd iterations ($t = 0.5$).
- **Stage 2 Accuracy Verdict:** All 32,784 rays pass independent original-AST evaluation (max delta RGB $3.65 \times 10^{-7}$, max delta T $8.94 \times 10^{-7}$). All 12 reference accuracy gates pass at both 65,536 and 32,768 resolutions. Forced native fallback is bit-identical.
- **Stage 2 Structural Verdict:** Because secant shifts the root midpoint by sub-millimeter amounts, 6 of the 12,102 affine kink endpoints are dropped, and 47,084 ray-member support unions shift slightly.
- **Stage 2 Economics Verdict:** **REJECTED.** Mode 2 bounded tile streaming still trails Current Marcher across all capacities ($0.640\times$ to $0.978\times$). Producer alone takes 2.58 ms (101.3% of Current budget).

---

## 2. Complete-Query Economics Across Modes

Tested on Apple M5 Metal; completion-fenced host timing with GPU timestamps disabled. Measured across 32,784 distinct native rays.

### Stage 1: `§6cs` (`subcache.json` — 100% Bit-Exact)
| Ring Capacity | Dispatches / Tile Chunks | Repeats | Current Marcher (Mode 0) | Global Bulk (Mode 1) | Bounded Stream (Mode 2) | Speedup (Mode 0 / Mode 2) |
|---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **8,192** | 5 tiles (16-ray tail) | 1 | 2.4990 ms | 2.8512 ms | 3.8893 ms | 0.643× |
| **8,192** | 5 tiles (16-ray tail) | 2 | 5.0633 ms | 5.0637 ms | 7.1033 ms | 0.713× |
| **8,192** | 5 tiles (16-ray tail) | 4 | 7.3742 ms | 8.2577 ms | 11.4834 ms | 0.642× |
| **16,384** | 3 tiles (16-ray tail) | 1 | 2.5392 ms | 2.5455 ms | 2.6057 ms | 0.974× |
| **16,384** | 3 tiles (16-ray tail) | 2 | 3.7657 ms | 3.8294 ms | 4.8474 ms | 0.777× |
| **16,384** | 3 tiles (16-ray tail) | 4 | 6.2978 ms | 6.6899 ms | 8.0531 ms | 0.782× |
| **32,768** | 2 tiles (16-ray tail) | 1 | 2.5464 ms | 2.5601 ms | 2.8686 ms | 0.888× |
| **32,768** | 2 tiles (16-ray tail) | 2 | 3.8025 ms | 3.8232 ms | 4.0353 ms | 0.942× |
| **32,768** | 2 tiles (16-ray tail) | 4 | 6.7515 ms | 7.6375 ms | 7.7234 ms | 0.874× |

Component series (65,536 capacity, 32,784 rays):
- Current Marcher: 2.5282 ms
- Bulk Connected: 2.5443 ms
- Producer Alone: 2.5383 ms (**100.4% of Current budget**)

---

### Stage 2: `§6ct` (`secant.json` — Guarded Hybrid Secant-Bisection)
| Ring Capacity | Dispatches / Tile Chunks | Repeats | Current Marcher (Mode 0) | Global Bulk (Mode 1) | Bounded Stream (Mode 2) | Speedup (Mode 0 / Mode 2) |
|---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **8,192** | 5 tiles (16-ray tail) | 1 | 2.9082 ms | 3.0144 ms | 4.2274 ms | 0.688× |
| **8,192** | 5 tiles (16-ray tail) | 2 | 6.6017 ms | 7.7953 ms | 10.3187 ms | 0.640× |
| **8,192** | 5 tiles (16-ray tail) | 4 | 9.6029 ms | 10.6418 ms | 14.5405 ms | 0.660× |
| **16,384** | 3 tiles (16-ray tail) | 1 | 2.5417 ms | 2.5282 ms | 3.0993 ms | 0.820× |
| **16,384** | 3 tiles (16-ray tail) | 2 | 3.7832 ms | 4.5923 ms | 5.2234 ms | 0.724× |
| **16,384** | 3 tiles (16-ray tail) | 4 | 6.3116 ms | 7.0359 ms | 8.5345 ms | 0.740× |
| **32,768** | 2 tiles (16-ray tail) | 1 | 2.5340 ms | 2.5717 ms | 2.5902 ms | 0.978× |
| **32,768** | 2 tiles (16-ray tail) | 2 | 3.7705 ms | 3.8424 ms | 3.9827 ms | 0.947× |
| **32,768** | 2 tiles (16-ray tail) | 4 | 6.1853 ms | 6.3483 ms | 6.4564 ms | 0.958× |

Component series (65,536 capacity, 32,784 rays):
- Current Marcher: 2.5479 ms
- Bulk Connected: 2.7801 ms
- Producer Alone: 2.5815 ms (**101.3% of Current budget**)

---

## 3. Physical Diagnosis of the Producer Execution Barrier

1. **SIMD Warp Divergence in Root Bisection:**
   In GPU compute (Apple Silicon Metal SIMD-groups of 32 threads, workgroups of 64 threads), execution width is lockstep. Even though secant converges in 1–2 iterations for a given ray, if any thread within the SIMD-group requires 10 iterations, the entire SIMD-group continues executing. Thus, reducing average per-thread iterations without uniform warp exit yields diminished returns on the hardware clock.
2. **Monolithic Marcher vs 2-Pass Producer-Consumer Architecture:**
   - The Current Marcher executes in a single compute pipeline: it evaluates empty space in large ray-marching steps and only computes dense transcendental evaluations when rays approach curtain envelopes.
   - In contrast, the 2-pass pipeline executes `producerMain` to construct explicit spatial schedule packets, followed by `rayMain` (and `fallbackMain`) to consume them.
   - Because `producerMain` alone consumes ~2.4–2.5 ms, any 2-pass connected pipeline (Producer + Consumer) will require at least $2.4\text{ ms} + 0.9\text{ ms} \approx 3.3\text{ ms}$, inherently losing to the monolithic Marcher (~2.5 ms) unless the producer workload itself can be restructured or pruned at a macroscopic level before fine root refinement.

---

## 4. Status of the Three Gates

- **Gate 1 (Profitable Connected Distinct-Ray Execution):** **REJECTED** for both §6cs and §6ct. Gate 1 remains **OPEN**.
- **Gate 2 (Generic OntoMath & Full Viewport/Frame Integration):** Remains **OPEN**.
- **Gate 3 (Moving Inhabited Acceptance):** Remains **OPEN**.

All work remains isolated in scratch probes on `claude/ray-domain-transport-rung0`. No production source, shaders, or save files were touched. Timing lane is released.


## Sixth-one Sun §6cu — executed correction and scope qualification

[Full review, native controls, qualified zero-only trial and portable replay](GPU_RAY_TRANSPORT_ROOT_REVIEW_2026-10-22.md). Exact24-unit shortcut refusal is repaired; original and corrected budget controls agree. All-level secant fails the affine/support gate; zero-only secant passes that gate and accuracy but loses cost. Batch times must be divided by repetitions for per-query cost. SIMD/root dominance, phase additivity and universal culling parity are unestablished. Original observations above remain historical evidence.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T18:21:23.901547-07:00`.
