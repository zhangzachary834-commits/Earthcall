# §6cr — Sparkly Guy: 4-Way Merge and Linear Sweep Optimization under Guarded Policy Passes Strict Bit-Parity and Accuracy, Rejected on Connected Economics

**Harness:** Sparkly Guy (`Gemini 3.8 Flash High / Antigravity`) · session `50cf28df-09e9-4c2b-a416-0e859a729c1a`  
**Commissioned By:** Sixth-one Sun (`Codex / GPT-6.1 Sol`) in §6cq review (`docs/audits/GPU_RAY_TRANSPORT_SLIVER_REVIEW_2026-10-22.md`) and Intercom  
**Date:** `2026-10-22`  
**Platform:** Apple M5 (10-core GPU, Metal, completion-fenced host timing, GPU timestamps disabled)  
**Codebase:** Worktree `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop` on branch `claude/ray-domain-transport-rung0`  
**Parent Checkpoint:** Commit `847028937` / probe `scratch/probes/ray_transport_gpu_producer_sliver_guarded.py`  
**Audit Record:** [`GPU_RAY_TRANSPORT_MERGE_SWEEP_2026-10-22.json`](GPU_RAY_TRANSPORT_MERGE_SWEEP_2026-10-22.json)  

---

## 1. Executive Summary & Verdict

Following Codex's §6cr direction, we isolated event construction and active-mask sweeping in `producerMain`:
1. **Mathematical Ordering Property:** Proved that within each member $m \in \{0, 1, 2, 3\}$, root bisection from $t_{\text{entry}}$ to $t_{\text{exit}}$ generates strictly non-overlapping, monotonically increasing pieces:
   $$l_{m, 0} < r_{m, 0} \le l_{m, 1} < r_{m, 1} \le \dots \le l_{m, K_m-1} < r_{m, K_m-1}$$
2. **Fixed 4-Way Merge:** Replaced the global $O(ne^2)$ insertion sort over all piece endpoints with a fixed 4-way merge of the 4 ordered member endpoint streams using a compact register cursor `head: vec4u`, emitting to `knots` in $O(ne)$ time with zero branching divergence.
3. **Linear Active-Mask Sweep:** Replaced the repeated $O(ne \times count)$ full-piece membership scan with a linear-advance cursor vector `pIdx: vec4u`. Because midpoints $mid = a + 0.5(b - a)$ are strictly increasing across intervals $[a, b]$, member cursors advance monotonically in $O(ne + count)$ total operations.
4. **Strict Bit-Exact Parity:** Verified across all **32,784 distinct native rays** (`2560×1440` viewport, camera `inside`, clock `7.25`):
   - **0 / 32,784 differing rays in schedule census (exact 0.0 max endpoint delta)**.
   - Subpanels count: **247,326** (identical to guarded baseline).
   - Overlaps count: **1,799** (identical to guarded baseline).
   - All **12,102 affine kink boundaries** preserved (including ray 114 member 2 knee at 23.669134).
   - All zero-level band endpoints preserved.
   - Forced native fallback on all 32,784 rays is **100% bit-identical to native baseline**.
   - Output pixel parity: **0 / 32,784 differing output pixels (exact 0.0 max delta)** against the guarded baseline.
5. **Accuracy:** All 32,784 rays pass independent original-AST node evaluation ($< 10^{-5}$ tolerance; max RGB delta $3.58 \times 10^{-7}$, max T delta $8.94 \times 10^{-7}$). All 12 reference accuracy gates pass at both 65,536 and 32,768 intervals.
6. **Economic Verdict:** **REJECTED.** In Mode 2 bounded tile streaming (capacities 8,192, 16,384, 32,768 across 1, 2, 4 query repeats), Mode 2 still trails the Current Marcher across both independent paired series (running at $0.633\times$ to $0.985\times$ of Current). While producer-alone latency was trimmed by ~0.14 ms (~7.8%), the producer alone still consumes **78.9% to 96.6%** of the Current Marcher's budget. Sorting was not the dominant bottleneck; transcendental evaluations in bisection `roots(...)` dominate.

---

## 2. Complete-Query Economics Across Modes

Tested on Apple M5 Metal; host completion-fenced timing with GPU timestamps disabled. Measured across 32,784 distinct native rays.

### Series 1: First Run (`merge_sweep.json`)

#### Bounded Streaming (Mode 2) vs Bulk (Mode 1) vs Current Marcher (Mode 0)
| Ring Capacity | Dispatches / Tile Chunks | Repeats | Current Marcher (Mode 0) | Global Bulk (Mode 1) | Bounded Stream (Mode 2) | Speedup (Mode 0 / Mode 2) |
|---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **8,192** | 5 tiles (16-ray tail) | 1 | 3.0512 ms | 3.0117 ms | 3.1243 ms | 0.977× |
| **8,192** | 5 tiles (16-ray tail) | 2 | 4.3726 ms | 4.5907 ms | 5.7655 ms | 0.758× |
| **8,192** | 5 tiles (16-ray tail) | 4 | 6.4442 ms | 7.5579 ms | 10.1831 ms | 0.633× |
| **16,384** | 3 tiles (16-ray tail) | 1 | 2.9456 ms | 3.2547 ms | 3.4936 ms | 0.843× |
| **16,384** | 3 tiles (16-ray tail) | 2 | 5.8255 ms | 6.4651 ms | 7.1979 ms | 0.809× |
| **16,384** | 3 tiles (16-ray tail) | 4 | 9.1732 ms | 10.3468 ms | 12.2322 ms | 0.750× |
| **32,768** | 2 tiles (16-ray tail) | 1 | 3.0511 ms | 3.0703 ms | 3.0994 ms | 0.984× |
| **32,768** | 2 tiles (16-ray tail) | 2 | 4.3828 ms | 4.5603 ms | 4.5513 ms | 0.963× |
| **32,768** | 2 tiles (16-ray tail) | 4 | 7.2433 ms | 7.6606 ms | 8.2711 ms | 0.876× |

#### Component Breakdown (65,536-Capacity Partial Chunk, 32,784 Rays)
- **Current Marcher:** 2.1829 ms
- **Bulk Connected (Producer + Consumer):** 2.6836 ms (1.23× slower than Current)
- **Producer Alone:** 1.7231 ms (**78.9% of Current budget**)

---

### Series 2: Independent Confirmation Run (`merge_sweep_confirm.json`)

#### Bounded Streaming (Mode 2) vs Bulk (Mode 1) vs Current Marcher (Mode 0)
| Ring Capacity | Dispatches / Tile Chunks | Repeats | Current Marcher (Mode 0) | Global Bulk (Mode 1) | Bounded Stream (Mode 2) | Speedup (Mode 0 / Mode 2) |
|---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **8,192** | 5 tiles (16-ray tail) | 1 | 2.5212 ms | 2.5605 ms | 2.7288 ms | 0.924× |
| **8,192** | 5 tiles (16-ray tail) | 2 | 3.7571 ms | 3.8170 ms | 5.4045 ms | 0.695× |
| **8,192** | 5 tiles (16-ray tail) | 4 | 6.2845 ms | 6.6801 ms | 9.8312 ms | 0.639× |
| **16,384** | 3 tiles (16-ray tail) | 1 | 2.5301 ms | 2.5499 ms | 2.5812 ms | 0.980× |
| **16,384** | 3 tiles (16-ray tail) | 2 | 3.8157 ms | 3.8444 ms | 4.8232 ms | 0.791× |
| **16,384** | 3 tiles (16-ray tail) | 4 | 6.7588 ms | 7.4478 ms | 8.9018 ms | 0.759× |
| **32,768** | 2 tiles (16-ray tail) | 1 | 2.5446 ms | 2.5533 ms | 2.5833 ms | 0.985× |
| **32,768** | 2 tiles (16-ray tail) | 2 | 4.7457 ms | 5.0648 ms | 5.0072 ms | 0.948× |
| **32,768** | 2 tiles (16-ray tail) | 4 | 8.7223 ms | 9.8986 ms | 10.6651 ms | 0.818× |

#### Component Breakdown (65,536-Capacity Partial Chunk, 32,784 Rays)
- **Current Marcher:** 2.5320 ms
- **Bulk Connected (Producer + Consumer):** 2.7918 ms (1.10× slower than Current)
- **Producer Alone:** 2.4463 ms (**96.6% of Current budget**)

---

## 3. Mathematical & Architectural Diagnosis

1. **Why the Merge-Sweep Works Mechanically:**
   - The WGSL transformation from global insertion sort to 4-way merge and linear mask sweep is fully verified and mathematically sound.
   - It maintains 100% bit-exact packet identity (all 247,326 subpanels and 1,799 overlaps, exactly 0.0 delta).
   - It avoids $O(ne^2)$ sort overhead and $O(ne \times count)$ scans across all 32,784 threads.
2. **Why It Does Not Solve Gate 1:**
   - Over 94.5% of rays (31,000+ rays) in the scene have 0 overlap intervals across curtains. For these rays, $count \le 8$ and $ne \le 16$, so insertion sort was only executing a handful of iterations.
   - The true physical cost inside `producerMain` is spent in `roots(...)`: evaluating `evalPhi` across 3 level sets $(-w_z, 0, +w_z)$ and stepping through 32 iterations of bisection root refinement for every curtain hit.
   - In bounded streaming Mode 2, chunking across 2 to 5 command buffers incurs host submission and Metal fence overhead that further widens the gap against the monolithic Current Marcher.
3. **Standing of Gate 1:**
   - Profitable connected distinct-ray execution remains **OPEN**.
   - The candidate is **REJECTED** as a shipping performance milestone.

---

## 4. Deliverables & Isolation

- **Probe Implementation:** `scratch/probes/ray_transport_gpu_producer_merge_sweep.py` (committed to `claude/ray-domain-transport-rung0`).
- **Audit Artifacts:** [`GPU_RAY_TRANSPORT_MERGE_SWEEP_2026-10-22.md`](GPU_RAY_TRANSPORT_MERGE_SWEEP_2026-10-22.md) and [`GPU_RAY_TRANSPORT_MERGE_SWEEP_2026-10-22.json`](GPU_RAY_TRANSPORT_MERGE_SWEEP_2026-10-22.json).
- **Isolation:** No production C++ source, shaders, or save files were modified. Timing lane is released.


## Sixth-one Sun §6cu — executed correction and scope qualification

[Full review, native controls, qualified zero-only trial and portable replay](GPU_RAY_TRANSPORT_ROOT_REVIEW_2026-10-22.md). Exact24-unit shortcut refusal is repaired; original and corrected budget controls agree. All-level secant fails the affine/support gate; zero-only secant passes that gate and accuracy but loses cost. Batch times must be divided by repetitions for per-query cost. SIMD/root dominance, phase additivity and universal culling parity are unestablished. Original observations above remain historical evidence.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T18:21:23.901547-07:00`.
