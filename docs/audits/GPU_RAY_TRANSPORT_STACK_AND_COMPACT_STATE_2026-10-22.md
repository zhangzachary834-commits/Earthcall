# Northern Veil — §6co Stack Endpoint Forwarding and Compact State Producer Audit

Sparkly Guy (`Gemini 3.8 Flash High` / Antigravity) · session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` · `2026-10-22T14:48:00-07:00 (PDT)`  
Handoff parent: [RAY_TRANSPORT_DUO_HANDOFF_POST_6CN_2026-10-22.md](RAY_TRANSPORT_DUO_HANDOFF_POST_6CN_2026-10-22.md) by Codex (`GPT-6.1 Sol`) and [Firmament Counsel](file:///Users/zacharyzhang/Documents/GitHub/Earthcall/agent%20intercom/communication-threads/sdf-and-rendering/FIRMAMENT_COUNSEL_POST_6CN_COARSE_PHI_AND_ROOT_SLIVER_DISCOVERY_2026-10-22.md) by Firmament (`Gemini 4 Argon`).

---

## 1. Executive Summary & Verdict

Following Firmament's §6co specification, Sparkly Guy implemented and evaluated the **Strict Same-Device Bit-Exact Producer with Stack Endpoint Forwarding and Compacted Private Memory** on the canonical **32,784 distinct native rays** workload (`256×144` capture grid, `inside` camera, clock `7.25`):

1. **Stack Endpoint Forwarding Implemented:**
   - Changed `stack: array<vec3f, 20>` to `array<vec4f, 20>` storing `(a, b, phi_a, phi_b)`, with 4-bit depth (`0..14`) packed into `depthBits: vec3u`.
   - On stack pop: `va = cell.z - c; vb = cell.w - c;` (**zero `evalPhi` calls on pop**).
   - On subdivision split: `phi_mid = evalPhi(m, o, d, mid);` is evaluated **once** and forwarded into both child cells on stack push.
   - Eliminates the 4.0× redundant `evalPhi` multiplier on subdivision splits.
2. **Private Array Compaction Implemented:**
   - Merged `bounds[96]` and `events[96]` into a single shared array `knots: array<f32, 96>` (-384 B).
   - Packed `pieces: array<vec4f, 48>` into `pieces: array<vec2f, 48>` (-384 B) + member indices packed into `pieceMembers: vec3u`.
   - Cut thread-private array memory from **2,148 B down to 1,344 B/thread** (-768 B/thread, a 35.8% reduction saving 25.1 MB across 32,784 threads).
   - All capacities (48 pieces, 96 bounds, 32 bands, 20 stack depth) and old packet formats retained without truncation.
3. **100.0% Bit-Exact Identity on Apple Silicon M5 Metal:**
   - Evaluated across all **32,784 distinct native rays**:
   - **Differing rays: 0 / 32,784 (0.00%)**.
   - **Max absolute difference: 0.000000e+00** across all channels.
4. **All Reference Accuracy Gates Pass Unconditionally:**
   - All 32,784 rays pass original-AST node agreement checks with **0 fallback rays**.
   - The 192-ray holdout passes all 6 aggregate mean/p99/max gates at both 65,536 and 32,768 reference resolutions.
5. **Economic Verdict — REJECTED AS CONNECTED WIN (1.15× slower than Current):**
   - In paired 4-repeat ABBA trials in a 65,536-capacity single partial chunk allocation:
     - **Current Marcher:** **2.2985 ms / query** (9.19 ms total)
     - **Bulk Connected:** **2.6371 ms / query** (10.55 ms total) — **1.15× slower**
     - **§6co Producer Alone:** **1.8684 ms / query** (7.47 ms total)
     - **Consumer Alone + Fallback:** **1.1177 ms / query** (4.47 ms total)
   - **Diagnosis:** Producer alone improved from 1.9127 ms to 1.8684 ms (-0.044 ms), but still accounts for **81.3% of the entire Current budget (2.30 ms)**. The dominant remaining bottleneck is Firmament's **Smoking Gun #2**: `putBand` emits **~144,000 root-bracket micro-subpanels of width $\le 0.0001\text{ m}$ (44.4% of all 324,088 subpanels)**, causing massive sorting, sweeping, and memory write overhead that Stage 2 (`§6cp` Root-Sliver Absorption) is required to remove.

---

## 2. Component Phases & Timing Breakdown

Paired 4-repeat ABBA trials on 32,784 distinct native rays (65,536 allocation capacity):

| Component / Mode | 1 Repeat (ms/q) | 2 Repeats (ms/q) | 4 Repeats (ms/q) | Share of Current Budget |
|---|:---:|:---:|:---:|:---:|
| **Current Marcher (Baseline)** | 3.6330 | 3.3450 | **2.2985** | 100.0% |
| **Bulk Connected Candidate** | 3.8877 | 3.6779 | **2.6371** | 114.7% (1.15× slower) |
| **§6co Producer Only** | 3.3557 | 2.5100 | **1.8684** | 81.3% |
| **Consumer Only + Fallback** | 2.1664 | 1.4263 | **1.1177** | 48.6% |

---

## 3. Shader Implementation

From `scratch/probes/ray_transport_gpu_producer_stack_and_compact_state.py`:

```wgsl
fn getDepth(bits: vec3u, idx: u32) -> u32 {
 return (bits[idx >> 3u] >> ((idx & 7u) << 2u)) & 15u;
}
fn setDepth(bits: ptr<function, vec3u>, idx: u32, d: u32) {
 let w = idx >> 3u; let s = (idx & 7u) << 2u;
 (*bits)[w] = ((*bits)[w] & ~(15u << s)) | ((d & 15u) << s);
}

// Inside roots():
// stack: array<vec4f, 20> storing (a, b, phi_a, phi_b)
// top--; let cell = stack[top]; a = cell.x; b = cell.y;
// let va = cell.z - c; let vb = cell.w - c; // ZERO evalPhi calls!
// ...
// On split:
let phi_mid = evalPhi(m, o, d, mid); // Single evalPhi call!
stack[top] = vec4f(mid, b, phi_mid, cell.w); setDepth(&depthBits, top, depth + 1u);
stack[top + 1u] = vec4f(a, mid, cell.z, phi_mid); setDepth(&depthBits, top + 1u, depth + 1u);
top += 2u;
```

---

## 4. Reprodution & Machine Evidence

- Script: `scratch/probes/ray_transport_gpu_producer_stack_and_compact_state.py`
- Phase script: `scratch/probes/ray_transport_gpu_producer_stack_and_compact_state_phases.py`
- Build / run directories: `/private/tmp/earthcall-stack-compact-build`, `/private/tmp/earthcall-stack-compact-phases-build`
- JSON Record: `docs/audits/GPU_RAY_TRANSPORT_STACK_AND_COMPACT_STATE_2026-10-22.json`

Signed: Sparkly Guy (`Gemini 3.8 Flash High` / Antigravity) · session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` · `2026-10-22T14:48:00-07:00 (PDT)`.
