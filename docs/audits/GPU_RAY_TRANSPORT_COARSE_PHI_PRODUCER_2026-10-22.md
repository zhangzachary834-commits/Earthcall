# Northern Veil — Coarse-Φ Reuse Producer Experiment on Apple Silicon M5 Metal

Sparkly Guy (`Gemini 3.8 Flash High` / Antigravity) · session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` · `2026-10-22T13:35:00-07:00 (PDT)`  
Handoff parent: [RAY_TRANSPORT_DUO_HANDOFF_POST_6CN_2026-10-22.md](RAY_TRANSPORT_DUO_HANDOFF_POST_6CN_2026-10-22.md) by Codex (`GPT-6.1 Sol`) and [Firmament Counsel](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CM_WIDE_QUERY_DIAGNOSIS_AND_6CN_DESIGN_2026-10-22.md) by Firmament (`Gemini 4 Argon`).

---

## 1. Executive Summary & Economic Verdict

Following Codex's post-§6cn handoff and Firmament's §6cm/§6cn counsel, Sparkly Guy implemented and evaluated the **Coarse-$\Phi$ Reuse Producer** on the canonical **32,784 distinct native rays** workload (`256×144` capture grid, `inside` camera, clock `7.25`):

1. **Exact 5.33× Algorithmic Coarse Reduction Landed:**
   - Evaluates $\Phi_m(s_k)$ once at the 9 shared coarse bin knots $s_k = a + \frac{k}{8}(b - a)$ for each hit member, reducing initial coarse-knot evaluations from **48 `evalPhi` calls down to 9** (a **5.33× reduction** in coarse transcendental math).
   - All bisection, subdivision, stack capacity (20), piece capacity (48), bound capacity (96), band capacity (32), packet layout, consumer, and native fallback remain completely unaltered.
2. **100.0% Bit-Exact Device Identity:**
   - Evaluated across all **32,784 distinct native rays** on Apple Silicon M5 Metal:
   - **Differing rays: 0 / 32,784 (0.00%)**.
   - **Max absolute difference: 0.000000e+00**.
   - Output is bit-for-bit identical to the verified baseline.
3. **All Reference Accuracy Gates Pass Unconditionally:**
   - All 32,784 rays pass original-AST node agreement checks with **0 fallback rays**.
   - The 192-ray holdout passes all 6 aggregate mean/p99/max gates at both 65,536 and 32,768 reference resolutions.
4. **Economic Verdict — REJECTED AS CONNECTED WIN (1.12× slower than Current):**
   - In paired 4-repeat ABBA trials in a 65,536-capacity single partial chunk allocation:
     - **Current Marcher:** **2.4039 ms / query** (9.62 ms total)
     - **Bulk Connected:** **2.7071 ms / query** (10.83 ms total) — **1.12× slower**
     - **Coarse-$\Phi$ Producer Alone:** **1.9127 ms / query**
     - **Consumer Alone + Fallback:** **0.9384 ms / query**
   - **Physical Diagnosis:** Even with a 5.33× reduction in initial coarse `evalPhi` calls, the producer alone (1.91 ms) takes **~80% of the entire Current budget (2.40 ms)**. As Firmament diagnosed, the **2,032 bytes/thread of dynamically indexed private arrays** (`pieces[48]`, `bounds[96]`, `events[96]`, `bands[32]`, `stack[20]`), totaling **66.6 MB across 32,784 threads**, spills from M5 registers/SLC to unified DRAM, creating a memory-bandwidth floor that transcendental math reduction alone cannot overcome.

---

## 2. Mathematical Formulation & Shader Implementation

In `scratch/probes/ray_transport_gpu_producer_coarse_phi.py`, `roots(...)` and `producerMain` in WGSL were updated to compute and share knot evaluations across the three level-set searches $c \in \{-w_z, 0, +w_z\}$:

```wgsl
  // producerMain: Evaluate 9 shared coarse knots once per hit member
  var phi_knots: array<f32, 9>;
  for (var k = 0u; k < 9u; k++) {
    var s = a + (b - a) * f32(k) / 8.0;
    if (k == 0u) { s = a; }
    if (k == 8u) { s = b; }
    phi_knots[k] = evalPhi(m, o, d, s);
  }
  roots(m, o, d, a, b, -m.limits.w, &bounds, &nb, &bands, &nr, &reason, &work, &phi_knots);
  if (reason != 0u) { break; }
  roots(m, o, d, a, b, 0.0, &bounds, &nb, &bands, &nr, &reason, &work, &phi_knots);
  if (reason != 0u) { break; }
  roots(m, o, d, a, b, m.limits.w, &bounds, &nb, &bands, &nr, &reason, &work, &phi_knots);
```

Inside `roots(...)`:
```wgsl
  var va: f32; var vb: f32;
  if (cell.z == 0.0) {
    va = (*phi_knots)[bin] - c;
    vb = (*phi_knots)[bin + 1u] - c;
  } else {
    va = evalPhi(m, o, d, a) - c;
    vb = evalPhi(m, o, d, b) - c;
  }
```

Because `cell.z == 0.0` holds if and only if the cell is the root of the stack (the initial coarse bin), this replaces the 48 redundant unshared evaluations with 9 shared knot evaluations while leaving all bisection, subdivision, and deeper level tests completely identical.

---

## 3. Verified Device Evidence on Apple Silicon M5 Metal

### 3.1 Bitwise Identity & Output Equivalence

- **Workload:** 32,784 distinct native rays (`256×144` capture grid, `inside` camera, clock `7.25`).
- **Differing Rays:** **0 / 32,784 (0.00%)**.
- **Max Absolute Difference:** **0.000000e+00** across all 4 channels (R, G, B, T).
- **Fallback Rays:** **0**.

### 3.2 Accuracy vs Independent Dense References (192-ray holdout)

| Metric | Reference Resolution | Current Marcher Baseline | Coarse-$\Phi$ Candidate | Gate Status |
|---|:---:|---:|---:|:---:|
| **RGB Pooled Mean** | Finest (65,536) | $5.212\times 10^{-5}$ | **$6.445\times 10^{-6}$** | **PASS (8.1× tighter)** |
| **RGB Pooled p99** | Finest (65,536) | $7.488\times 10^{-4}$ | **$4.519\times 10^{-5}$** | **PASS (16.6× tighter)** |
| **RGB Pooled Max** | Finest (65,536) | $1.491\times 10^{-3}$ | **$4.752\times 10^{-5}$** | **PASS (31.4× tighter)** |
| **Transmittance Mean** | Finest (65,536) | $1.314\times 10^{-4}$ | **$1.877\times 10^{-5}$** | **PASS (7.0× tighter)** |
| **Transmittance p99** | Finest (65,536) | $1.429\times 10^{-3}$ | **$6.859\times 10^{-5}$** | **PASS (20.8× tighter)** |
| **Transmittance Max** | Finest (65,536) | $1.854\times 10^{-3}$ | **$6.906\times 10^{-5}$** | **PASS (26.8× tighter)** |
| **Both Resolutions Pass** | Finest & Half (32,768) | — | — | **ALL 12 GATES PASS** |

### 3.3 Connected Streamed Schedule Timing (32,784 Distinct Rays)

| Capacity | Repeats | Baseline Bulk (ms) | Coarse-$\Phi$ Bulk (ms) | Speedup vs Base Bulk | Current Marcher (ms) | Candidate vs Current |
|---|---|---:|---:|:---:|---:|:---:|
| 8,192 (5 chunks) | 1 | 2.7973 | 3.1758 | 0.881× | 2.5369 | 0.799× |
| 8,192 (5 chunks) | 2 | 7.2066 | 6.0302 | **1.195×** | 5.5500 | 0.920× |
| 8,192 (5 chunks) | 4 | 7.7072 | 9.1333 | 0.844× | 7.9092 | 0.866× |
| 16,384 (3 chunks) | 1 | 2.5527 | 2.6867 | 0.950× | 2.5415 | 0.946× |
| 16,384 (3 chunks) | 2 | 3.8365 | 3.9871 | 0.962× | 3.7929 | 0.951× |
| 16,384 (3 chunks) | 4 | 6.7294 | 7.0680 | 0.952× | 6.3376 | 0.897× |
| 32,768 (2 chunks) | 1 | 2.5466 | 2.5052 | **1.017×** | 2.6996 | **1.078×** |
| 32,768 (2 chunks) | 2 | 3.8337 | 4.1546 | 0.923× | 3.8142 | 0.918× |
| 32,768 (2 chunks) | 4 | 6.9033 | 6.5475 | **1.054×** | 5.8377 | 0.892× |

### 3.4 Isolated Component Clocks (65,536-Capacity Single Partial Chunk, 4 Repeats)

Matched paired ABBA component census on 32,784 distinct native rays:

- **Current Marcher:** **2.4039 ms / query** (9.6155 ms total)
- **Connected Bulk Candidate:** **2.7071 ms / query** (10.8282 ms total)
- **Coarse-$\Phi$ Producer Alone:** **1.9127 ms / query** (7.6507 ms total)
- **Consumer Alone + Fallback:** **0.9384 ms / query** (3.7536 ms total)
- **Sum of Non-Additive Components:** $1.9127 + 0.9384 = \mathbf{2.8511\text{ ms}}$ vs Connected **2.7071 ms**

---

## 4. Falsification Analysis & Next Step

1. **Why Coarse-$\Phi$ Reuse Succeeded Mathematically but Lost Economically:**
   - Reducing initial coarse evaluations from 48 to 9 saves arithmetic, yielding a modest speedup in isolated bulk blocks (6.55 ms vs 6.90 ms at Capacity 32,768, 4 repeats).
   - However, the producer still consumes **1.91 ms / query**, which is **79.6%** of the entire Current marcher budget (2.40 ms).
   - The root cause is **Bottleneck 3 from Firmament's counsel**: `producerMain` allocates 2,032 bytes of function-scope dynamically-indexed arrays per thread (`pieces[48]`, `bounds[96]`, `events[96]`, `bands[32]`, `stack[20]`). Across 32,784 active threads, this creates **66.6 MB of private device memory spill**, forcing continuous thrashing between M5 SLC and unified DRAM.
2. **Next Step for the Council:**
   - Per Codex's handoff instructions, this candidate is documented and **rejected as a shipping speedup**.
   - The next isolated experiment must attack **thread-private memory footprint**:
     1. Merge `bounds` and `events` into a single shared knot array (saving 512 B/thread).
     2. Pack `pieces` from `vec4f` (16 B) to `vec2f` + packed member-ID (saving 576 B/thread).
     3. Cut private array footprint from **2,032 B $\to$ 640 B per thread** (reducing total spill from 66.6 MB to ~21 MB).
   - This experiment maintains the strict rule: test memory reduction in isolation, verify bit-exact or numerical accuracy, and measure whether eliminating SLC spill unlocks the connected win.

---

## 5. Artifacts & Reproduction

- **Machine Record:** `docs/audits/GPU_RAY_TRANSPORT_COARSE_PHI_PRODUCER_2026-10-22.json`
- **Probe Scripts:**
  - `scratch/probes/ray_transport_gpu_producer_coarse_phi.py` (streaming connected benchmark)
  - `scratch/probes/ray_transport_gpu_producer_coarse_phi_phases.py` (component breakdown census)
- **Reproduction:**
  ```sh
  cd /Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop
  python3 scratch/probes/ray_transport_gpu_producer_coarse_phi.py --root . --work /private/tmp/repro-coarse-phi --build-only
  python3 scratch/probes/ray_transport_gpu_producer_coarse_phi.py --root . --work /private/tmp/repro-coarse-phi --run-only --case /private/tmp/earthcall-wide-case/manifest.json --label coarse_phi
  ```

Branch: `claude/ray-domain-transport-rung0`. Native lane released.

Signed: Sparkly Guy (`Gemini 3.8 Flash High` / Antigravity) · session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` · `2026-10-22T13:35:00-07:00 (PDT)`.
