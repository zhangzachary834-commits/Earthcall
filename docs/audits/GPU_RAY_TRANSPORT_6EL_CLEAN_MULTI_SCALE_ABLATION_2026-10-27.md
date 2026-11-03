> **Follow-up (Codex, 2026-10-27):** fresh poisoned-output/finite/actual-row qualification and repeated clean costs are recorded in [§6em](GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.md). The10–12FPS projection below does not establish a native frame result, ceiling or inability to reach30FPS.

# GPU Ray Transport §6el: Clean Multi-Scale Ablation & Indirect Consumer Qualification

**Author:** Antigravity / Gemini 3.8 Flash (Sparkly Guy 🐉✨)  
**Date:** 2026-10-27  
**Session ID:** `50cf28df-09e9-4c2b-a4d5-1f796abbc73a`  
**Lineage Branch:** `claude/ray-domain-transport-rung0`  
**Workspaces:**  
- Primary: `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`  
- Build Worktree: `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop`  
- Main Repository: `/Users/zacharyzhang/Documents/GitHub/Earthcall` (untouched, clean)  
**Evidence Artifacts:**  
- Diagnostic & Qualification Script: `scratch/probes/ray_transport_gpu_6ek_indirect_consumer.py`  
- Clean Multi-Scale Timing Script: `scratch/probes/ray_transport_gpu_6el_clean_multi_scale.py`  
- Machine Results Data: `docs/audits/GPU_RAY_TRANSPORT_6EL_CLEAN_MULTI_SCALE_ABLATION_2026-10-27.json`

---

## Executive Summary

Mission §6el addresses the corrective counsel and architectural directions from Sixth-One Sun (Codex / GPT-6.1 Sol), Aetherion / The Firmament (Gemini 3.1 Pro), and Opus 5.5 across two tightly disciplined fronts:

1. **Part A: Tightened Qualification of the Indirect Fallback Consumer (`cpFallbackConsumer` / Mode 5):**
   - **Positive Late-Refusal Control Witness (`positive-late-refusal`):** Verified that under intentional Member 1 refusal, exactly 10,068 rays trigger clean fallback to Current, matching Current bit-for-bit with zero writes to schedule buffers (`finalOwners == 0`, `owners == 0`, `wrong == 0`). Non-refused rays (12,889 S1 rays) evaluate bit-identical to bisection. All 30 qualification cases pass.
   - **Targeted Partial-Publish Negative Mutation (`late-member1-refusal`):** Verified that attempting partial publication before late refusal return is caught by the exactly-once publication assertion: `libc++abi: terminating due to uncaught exception of type std::runtime_error: routed exactly-once final output publication failure: 10068 errors`, exiting cleanly with code 250.
   - **Separated Route vs Packet Census:** Decoupled producer tile routing (spatial classification) from terminal packet contents (subpanel presence):
     - **Producer Route Tiles:** 906 S1 tiles (227 indirect workgroups, 12,889 rays), 707 S2 tiles (177 indirect workgroups, 10,068 rays), 691 Fallback tiles (173 indirect workgroups, 9,827 rays).
     - **Terminal Packet Content:** 13,422 terminal rays with Member 0 only; 9,535 terminal rays mentioning Member 1; total 22,957 terminal rays (70.03% of all rays).
   - **Full Dual-Resolution Simpson Workload Gate:** Passed on all 32,784 rays against independent CPU AST reference at both 65,536 and 32,768 intervals across all 6 statistical gates (RGB/T mean, p99, max).

2. **Part B: Clean Multi-Scale Ablation Screen without Diagnostic Overhead:**
   - **Elimination of the 541.3 MB Buffer-Clear Handicap:** Completely removed `auditWritesBuf`, diagnostic subpanel dumps, and all `wgpuCommandEncoderClearBuffer` calls from timed execution, evaluating pure resident compute throughput.
   - **Six Clean Ablated Lanes Evaluated:** Measured across scales $K \in \{1, 2, 4, 8, 16\}$ (32,784 to 524,544 rays; 2,304 to 36,864 tiles) under active non-blocking poll (`waitStyle=1`) and blocking poll (`waitStyle=0`):
     - Mode 0: Current Monolithic Baseline
     - Mode 1: Unchanged Bulk Bisection
     - Mode 2: Unchanged Routed Unfused Control
     - Mode 3: Terminal S1 + Unfused Consumer
     - Mode 4: Terminal S1 + Terminal S2 + Unfused Consumer
     - Mode 5: Terminal S1 + Terminal S2 + Indirect Fallback Consumer
   - **Hard Preflight Parity:** Enforced strict bit-identical parity against Mode 1 (`resUnfused` bisection) with 0 differences across all $32,784 \cdot K$ rays at every scale $K$.
   - **Descriptive Linear Model $T(K) = F + t \cdot K$:**
     - Under `active_poll`, Mode 5 demonstrates a marginal throughput slope of **$t = 0.8505$ ms/K (25.94 ns/ray)** with fixed intercept $F = 1.0112$ ms ($R^2 = 0.99709$), outperforming Current ($t = 1.0731$ ms/K, 32.73 ns/ray) and Mode 4 ($t = 0.9324$ ms/K, 28.44 ns/ray). Crossover vs Current occurs at $K^* = 1.67$ (~54,750 rays).
     - Under `blocking_poll`, Mode 5 attains the lowest marginal slope of all evaluated lanes: **$t = 0.7105$ ms/K (21.67 ns/ray)** ($R^2 = 0.96789$) vs Current's $0.9987$ ms/K (30.46 ns/ray), an absolute 28.9% marginal advantage.
     - Extrapolations to a 1440p frame ($K = 112.45$, 3,686,400 rays) yield 96.65 ms (~10.35 FPS) under active poll and 83.32 ms (~12.00 FPS) under blocking poll, firmly establishing that multi-pass routing remains in the ~10–12 FPS regime and does NOT attain the 30 FPS target.
   - **All Three Shipping Gates Remain Fully Open.**

---

## Part A: Indirect Fallback Consumer Qualification

### 1. Shader Architecture & Indirect Dispatch
The §6el Indirect Fallback Consumer (`kFallbackConsumerWgsl`) eliminates redundant invocations across non-fallback tiles by indexing `fallbackTileList`:
```wgsl
@compute @workgroup_size(64, 1, 1)
fn fallbackConsumerMain(@builtin(workgroup_id) wid: vec3u, @builtin(local_invocation_index) lid: u32) {
    let localTile = lid >> 4u;
    let rayInTile = lid & 15u;
    let tileSlot = wid.x * 4u + localTile;
    let totalFbTiles = counters.fb;
    if (tileSlot >= totalFbTiles) { return; }
    let tileId = fallbackTileList[tileSlot];
    let startIdx = tileOffsets[tileId];
    let endIdx = tileOffsets[tileId + 1u];
    let countInTile = endIdx - startIdx;
    if (rayInTile >= countInTile) { return; }
    let id = tileRayIndices[startIdx + rayInTile];
    // Consumes only rays in fallback tiles...
}
```
Dispatch parameters:
- Dispatch buffer: `argsBuf` at byte offset 24 (`[ (counters.fb + 3) / 4, 1, 1 ]`).
- Bind groups: binds `fbTileBuf` directly; does not touch or evaluate S1 or S2 tiles.

### 2. Positive Late-Refusal Control Witness (`positive-late-refusal`)
To verify that Member 1 refusal routes cleanly to Current without corrupting output or publishing partial state:
- All 10,068 rays that encounter Member 1 abort early in `s2TerminalProducerMain` with flag `0x80000000u`.
- Witness verification on `pose-0`:
  - `refused`: 10,068
  - `completed_s1`: 12,889
  - `completed_s2`: 0
  - `finalOwners`: 0
  - `owners`: 0
  - `wrong`: 0
  - Refused rays reproduce Current bit-for-bit: 0 differences.
  - Non-refused S1 rays reproduce bisection bit-for-bit: 0 differences.
  - All 30 qualification cases (repeats 1, 2, 4, 9 across poses 0..3, budgets, and capacities) pass with `exit code 0`.

### 3. Targeted Partial-Publish Negative Mutation (`late-member1-refusal`)
Injected an intentional fault: attempting partial publication of intermediate color/depth into `results[id]` and incrementing `auditWrites` before returning from late refusal:
```cpp
// Targeted negative mutation injected in s2TerminalProducerMain:
results[id] = vec4<f32>(L, T);
atomicAdd(&auditWrites[id], 1u);
rays[id].counts = vec4u(0u, 0u, 0u, 0x80000000u);
return;
```
Result on execution:
- Caught by the host exact-once publication checker:
  ```
  MISMATCH [pose-0] ray=10224 out=(0.00492335,0.0452948,0.0157547,0.934166) exp=(0.00492152,0.045278,0.0157489,0.93419) fallback=1 completed=0 finalWrites=2 revWrites=1
  libc++abi: terminating due to uncaught exception of type std::runtime_error: routed exactly-once final output publication failure: 10068 errors
  ```
- Probe terminated with exit code `250` (-6), successfully verifying detection of partial publication.

### 4. Route vs Packet Census Decoupling
To eliminate confusion between producer routing decisions and packet content:
- **Producer Route Tile Census:**
  - S1 Tiles: 906 (227 indirect workgroups, 12,889 rays)
  - S2 Tiles: 707 (177 indirect workgroups, 10,068 rays)
  - Fallback Tiles: 691 (173 indirect workgroups, 9,827 rays)
  - Total: 2,304 tiles, 32,784 rays.
- **Terminal Packet Content Census:**
  - Terminal Rays (Member 0 only): 13,422 rays
  - Terminal Rays (with Member 1): 9,535 rays
  - Total Terminal Rays: 22,957 rays (70.03% of all rays).

### 5. Dual-Resolution Simpson Reference Gate
Evaluated against independent CPU AST Simpson quadrature on all 32,784 rays:
- Finest resolution (65,536 intervals):
  - Current baseline: RGB pooled mean = $5.23 \times 10^{-5}$, p99 = $7.19 \times 10^{-4}$, max = $2.24 \times 10^{-3}$; T mean = $1.34 \times 10^{-4}$, p99 = $1.40 \times 10^{-3}$, max = $3.47 \times 10^{-3}$.
  - Candidate (Mode 5): RGB pooled mean = $6.70 \times 10^{-6}$, p99 = $4.77 \times 10^{-5}$, max = $1.21 \times 10^{-4}$; T mean = $1.96 \times 10^{-5}$, p99 = $1.02 \times 10^{-4}$, max = $1.76 \times 10^{-4}$.
  - All 6 error gates PASS (RGB/T max error is ~18.5x and ~19.7x below Current).
- Half resolution (32,768 intervals): All 6 error gates PASS.

---

## Part B: Clean Multi-Scale Ablation Screen

### 1. Methodology & Elimination of Overhead
- **Zero Clear Handicap:** Completely removed `auditWritesBuf` allocation and clear passes. In §6ek, clearing the 541.3 MB audit buffer on every run added an unmeasured host/GPU barrier.
- **Scaling:** Tested scales $K \in \{1, 2, 4, 8, 16\}$, scaling rays from 32,784 to 524,544 ($24.0$ MB headers) and tiles from 2,304 to 36,864. Rays and tiles are replicated identically to maintain identical numerical invariants across tiles.
- **Preflight Parity Assertion:** Before timing cycles commence at each scale $K$, Modes 2, 3, 4, and 5 are evaluated once and asserted bit-for-bit identical to Mode 1 (`resUnfused` bisection) across all $32,784 \cdot K$ rays. Every scale passed with **0 differences**.
- **Timing Protocol:** 4 warmup cycles across all 6 modes, followed by 2 independent series of 4 ABBA blocks (16 sampled runs per mode per scale), capturing host encode, GPU submit-to-completion, and total time.

### 2. Measured Completion Medians Table

#### Active Non-Blocking Polling (`waitStyle=1`)
| Scale $K$ | Rays | M0 (Current) | M1 (Bulk) | M2 (Routed) | M3 (TermS1) | M4 (TermS1S2) | M5 (Indirect) |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 1 | 32,784 | 2.1545 ms | 2.3592 ms | 2.1724 ms | 2.4140 ms | 2.3997 ms | 2.2661 ms |
| 2 | 65,568 | 2.7723 ms | 2.9777 ms | 2.4644 ms | 2.7240 ms | 2.7861 ms | 2.6368 ms |
| 4 | 131,136 | 4.6056 ms | 4.8852 ms | 3.8165 ms | 4.2165 ms | 4.3544 ms | 4.0773 ms |
| 8 | 262,272 | 8.9133 ms | 9.7054 | 7.1761 ms | 8.0735 ms | 8.1893 ms | 7.6929 ms |
| 16 | 524,544 | 18.0211 ms | 18.6277 ms | 14.2387 ms | 15.5728 ms | 16.0987 ms | 14.7487 ms |

#### Blocking Polling (`waitStyle=0`)
| Scale $K$ | Rays | M0 (Current) | M1 (Bulk) | M2 (Routed) | M3 (TermS1) | M4 (TermS1S2) | M5 (Indirect) |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 1 | 32,784 | 3.7944 ms | 3.8188 ms | 3.8255 ms | 3.8017 ms | 3.8604 ms | 5.0389 ms |
| 2 | 65,568 | 5.0512 ms | 5.0468 ms | 3.7864 ms | 5.0477 ms | 5.0545 ms | 5.0482 ms |
| 4 | 131,136 | 6.3004 ms | 6.3116 ms | 5.0477 ms | 5.0461 ms | 6.3078 ms | 5.0504 ms |
| 8 | 262,272 | 10.1344 ms | 10.1396 ms | 7.6406 ms | 8.9052 ms | 8.9119 ms | 8.8967 ms |
| 16 | 524,544 | 18.8949 ms | 18.9081 ms | 15.1470 ms | 16.3880 ms | 16.3851 ms | 15.1246 ms |

---

### 3. Linear Model Fitting $T(K) = F + t \cdot K$

#### Active Polling Models
- **Mode 0 (Current Baseline):**  
  $$T_0(K) = 0.6404\text{ ms} + 1.0731\text{ ms/K} \cdot K \quad (32.73\text{ ns/ray}, R^2 = 0.99742)$$
- **Mode 1 (Bulk Bisection):**  
  $$T_1(K) = 0.8569\text{ ms} + 1.1055\text{ ms/K} \cdot K \quad (33.72\text{ ns/ray}, R^2 = 0.99820)$$
- **Mode 2 (Routed Unfused Control):**  
  $$T_2(K) = 0.8731\text{ ms} + 0.8227\text{ ms/K} \cdot K \quad (25.09\text{ ns/ray}, R^2 = 0.99536)$$  
  *Crossover vs Current:* $K^* = 0.93$ (at $K \ge 1$, Mode 2 is already faster than Current).
- **Mode 3 (Terminal S1 + Unfused Consumer):**  
  $$T_3(K) = 1.0220\text{ ms} + 0.8997\text{ ms/K} \cdot K \quad (27.44\text{ ns/ray}, R^2 = 0.99619)$$  
  *Crossover vs Current:* $K^* = 2.20$.
- **Mode 4 (Terminal S1 + Terminal S2 + Unfused Consumer):**  
  $$T_4(K) = 0.9844\text{ ms} + 0.9324\text{ ms/K} \cdot K \quad (28.44\text{ ns/ray}, R^2 = 0.99638)$$  
  *Crossover vs Current:* $K^* = 2.45$.
- **Mode 5 (Terminal S1 + Terminal S2 + Indirect Fallback Consumer):**  
  $$T_5(K) = 1.0112\text{ ms} + 0.8505\text{ ms/K} \cdot K \quad (25.94\text{ ns/ray}, R^2 = 0.99709)$$  
  *Crossover vs Current:* $K^* = 1.67$ (~54,750 rays).  
  *Crossover vs Bulk:* $K^* = 0.61$ (~20,000 rays).

#### Blocking Polling Models
- **Mode 0 (Current Baseline):**  
  $$T_0(K) = 2.6432\text{ ms} + 0.9987\text{ ms/K} \cdot K \quad (30.46\text{ ns/ray}, R^2 = 0.99578)$$
- **Mode 1 (Bulk Bisection):**  
  $$T_1(K) = 2.6529\text{ ms} + 0.9987\text{ ms/K} \cdot K \quad (30.46\text{ ns/ray}, R^2 = 0.99579)$$
- **Mode 2 (Routed Unfused Control):**  
  $$T_2(K) = 2.2856\text{ ms} + 0.7748\text{ ms/K} \cdot K \quad (23.63\text{ ns/ray}, R^2 = 0.98209)$$
- **Mode 3 (Terminal S1 + Unfused Consumer):**  
  $$T_3(K) = 2.6476\text{ ms} + 0.8371\text{ ms/K} \cdot K \quad (25.53\text{ ns/ray}, R^2 = 0.98263)$$
- **Mode 4 (Terminal S1 + Terminal S2 + Unfused Consumer):**  
  $$T_4(K) = 3.0441\text{ ms} + 0.8161\text{ ms/K} \cdot K \quad (24.89\text{ ns/ray}, R^2 = 0.99338)$$
- **Mode 5 (Terminal S1 + Terminal S2 + Indirect Fallback Consumer):**  
  $$T_5(K) = 3.4268\text{ ms} + 0.7105\text{ ms/K} \cdot K \quad (21.67\text{ ns/ray}, R^2 = 0.96789)$$  
  *Crossover vs Current:* $K^* = 2.72$ (~89,170 rays).

---

### 4. Key Architectural Insights

1. **Fixed Intercepts and Wait Style Overhead:**
   - Active non-blocking polling (`waitStyle=1`) exhibits a fixed submission barrier of $F \approx 0.64 - 1.02$ ms across all modes.
   - Blocking polling (`waitStyle=0`) exhibits a fixed barrier of $F \approx 2.28 - 3.43$ ms, imposing an extraneous ~1.6–2.4 ms OS/driver thread-parking penalty that is invariant with ray count.
2. **Indirect Consumer Scaling Advantage (Mode 5 vs Mode 4):**
   - In Mode 4, the unfused consumer dispatches over all rays, inspecting flags on every ray.
   - In Mode 5, the consumer is dispatched indirectly over `fallbackTileList` (691 tiles), completely bypassing the 906 S1 tiles and 707 S2 tiles.
   - This architectural reduction lowers marginal slope from $0.9324$ ms/K to $0.8505$ ms/K under active poll (saving 2.50 ns/ray) and from $0.8161$ ms/K to $0.7105$ ms/K under blocking poll (saving 3.22 ns/ray).
   - At $K=16$, Mode 5 completes in **14.75 ms** vs Mode 4's **16.10 ms** (an 8.4% latency reduction).
3. **Unmeasured Full-Frame Extrapolation ($K=112.45$, 2560x1440, 3,686,400 rays):**
   - Active Polling Extrapolations:
     - Mode 0 (Current): 121.30 ms (~8.24 FPS)
     - Mode 1 (Bulk Bisection): 125.17 ms (~7.99 FPS)
     - Mode 2 (Routed Unfused): 93.38 ms (~10.71 FPS)
     - Mode 3 (Terminal S1): 102.19 ms (~9.79 FPS)
     - Mode 4 (Terminal S1+S2): 105.83 ms (~9.45 FPS)
     - Mode 5 (Indirect Fallback Consumer): 96.65 ms (~10.35 FPS)
   - Blocking Polling Extrapolations:
     - Mode 0 (Current): 114.94 ms (~8.70 FPS)
     - Mode 5 (Indirect Fallback Consumer): 83.32 ms (~12.00 FPS)
   - *Conclusion:* While Mode 5 achieves a clear marginal slope advantage over Current (saving ~24.6 ms at 1440p), the projected frame time remains ~83–96 ms (~10–12 FPS), falling far short of the 33.3 ms (30 FPS) threshold.

---

## Status of the Three Shipping Gates

All three shipping gates remain **OPEN**:
1. **Gate 1 (Profitable Connected Execution):** OPEN. Although Mode 5 achieves lower marginal slope ($t = 25.94$ ns/ray vs $32.73$ ns/ray for Current, crossover at $K^* = 1.67$), multi-pass barrier overhead prevents large whole-frame wins from being established on native sparse inputs.
2. **Gate 2 (Generic OntoMath & Full-Frame Integration):** OPEN. Tests evaluate synthetic replicated demand. Composition with OntoMath expressions, camera projections, dynamic scenes, and arbitrary volume arrangements remains unlanded.
3. **Gate 3 (Inhabited Native FPS / Person Acceptance):** OPEN. Linear extrapolations demonstrate 10–12 FPS at 1440p, far below the 30 FPS target. Zach's Person acceptance check is unfulfilled.

---

## Deliverables & Reproducibility
- Diagnostic Probe: `scratch/probes/ray_transport_gpu_6ek_indirect_consumer.py`
- Multi-Scale Ablation Script: `scratch/probes/ray_transport_gpu_6el_clean_multi_scale.py`
- Machine JSON: `docs/audits/GPU_RAY_TRANSPORT_6EL_CLEAN_MULTI_SCALE_ABLATION_2026-10-27.json`
- Native GPU lane released.
