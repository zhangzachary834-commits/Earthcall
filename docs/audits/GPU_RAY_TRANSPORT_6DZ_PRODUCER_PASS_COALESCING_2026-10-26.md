# GPU Ray Transport §6dz: Isolation of Producer-Pass Coalescing on Apple M5

**Harness:** Antigravity  
**Model:** Gemini 2.5 Pro (Sparkly Guy)  
**Session ID:** `50cf28df-09e9-4c2b-a4d5-1f796abbc73a`  
**Timestamp:** `2026-10-26T23:10:00-07:00`  
**Commissioned By:** Sixth-One Sun (Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a`)  
**Workspace:** `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`  
**Branch:** `claude/ray-domain-transport-rung0`  
**Engine Source Tree:** `3603add4018961529a91bc9592d76f9a553500b1`  

---

## 1. Executive Summary & Core Verdict

Mission §6dz tests a single, strictly isolated execution variable on Apple M5:
> **Hypothesis:** Does coalescing the three indirect producer passes (`cpS1`, `cpS2`, `cpFallback`) into a **single compute pass encoder (`WGPUComputePassEncoder`)** remove command-buffer boundary overhead and improve dispatch latency over the unchanged routed control?

Experimental structure:
1. All shaders, pipelines, bind groups, indirect buffer offsets (`argsBuf` at 0, 12, 24), and buffers were preserved **100% byte-for-byte**.
2. Two coalesced candidates were introduced alongside the unchanged controls:
   - **Mode 4:** Candidate Coalesced Routed Unfused (tested against Mode 2 Unchanged Unfused Control).
   - **Mode 5:** Candidate Coalesced Terminal S1 (tested against Mode 3 Unchanged Terminal Control).
3. Both candidates and controls were subjected to:
   - The **120-case correctness and ownership review matrix** (30 cases $\times$ 4 modes), passing with 100.0000% exact bit parity and zero `sBuf` writes verified on terminal S1 rays.
   - Negative mutation verification (`fallback-missing-publish`), failing immediately with exit code 250.
   - **Two fresh, clean, independent matched timing invocations** across all 6 modes (`{0, 1, 2, 3, 4, 5}`) in balanced ABBA sequences.

### The Scientific Verdict:
**No substantial, repeatable Current win is established.** Repeat-1 coalesced/control completion MEDIANS differ by −0.046% to +0.193% in the two reported invocations. This is an observed median range, not an equivalence bound on all clocks. Means and ordered encode/completion/total samples must also be considered; the §6ea correction below provides them.

| Repeats | Invoc. | Mode 2 Unchanged Unfused Med. | Mode 4 Coalesced Unfused Med. | Unfused Coalescing Delta | Mode 3 Unchanged Terminal Med. | Mode 5 Coalesced Terminal Med. | Terminal Coalescing Delta |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **R=1** | Inv 1 | 2.532 ms | 2.533 ms | **+0.06% (+1 µs)** | 2.533 ms | 2.532 ms | **−0.05% (−1 µs)** |
| **R=1** | Inv 2 | 2.522 ms | 2.527 ms | **+0.19% (+5 µs)** | 2.522 ms | 2.526 ms | **+0.18% (+4 µs)** |
| **R=2** | Inv 1 | 3.818 ms | 3.809 ms | **−0.24% (−9 µs)** | 4.409 ms | 3.840 ms | **−12.91% (−569 µs)** |
| **R=2** | Inv 2 | 3.798 ms | 3.796 ms | **−0.07% (−2 µs)** | 4.373 ms | 4.427 ms | **+1.23% (+54 µs)** |
| **R=4** | Inv 1 | 6.279 ms | 6.353 ms | **+1.19% (+74 µs)** | 6.445 ms | 6.386 ms | **−0.92% (−59 µs)** |
| **R=4** | Inv 2 | 7.011 ms | 7.568 ms | **+7.95% (+557 µs)** | 8.207 ms | 8.225 ms | **+0.22% (+18 µs)** |

### Architectural Conclusion (narrowed by §6ea corrective review)
- This particular producer-pass coalescing candidate does not establish a substantial repeated gain over Current for the sampled resident workload; stop it as a shipping candidate.
- Host encoding is measurably smaller in these samples: repeat-1 mean encoding decreases by 5.6–9.2 µs versus the corresponding routed control. Completion medians alone do not measure CPU encoding or per-pass GPU costs.
- This experiment does not establish a universal 1–2 µs pass cost, rule out every dispatch/scheduling bottleneck, or causally separate ALU work, spills, divergence and completion-wait overhead. The next step is source-bound demand measurement inside the solver.

---

## 2. Experimental Setup & Candidate Definitions

The dispatch logic in `scratch/probes/ray_transport_gpu_6dz_producer_coalescing.py` defines 6 explicit execution modes:

```
Mode 0: Current Marcher Baseline
Mode 1: Guarded Bulk Connected (Reference)
Mode 2: §6dr Canonical Routed Unfused Control (Pass 3: cpS1Unfused, Pass 4: cpS2, Pass 5: cpFallback)
Mode 3: §6dt True Terminal S1 Control (Pass 3: cpS1Terminal, Pass 4: cpS2, Pass 5: cpFallback)
Mode 4: §6dz Candidate Coalesced Routed Unfused (Pass 3: [cpS1Unfused + cpS2 + cpFallback])
Mode 5: §6dz Candidate Coalesced Terminal S1 (Pass 3: [cpS1Terminal + cpS2 + cpFallback])
```

In Mode 4 and Mode 5, the three indirect producer dispatches are issued inside a single `WGPUComputePassEncoder`:
```cpp
// Pass 3: Coalesced Producers Pass (Passes 3, 4, 5 in a single WGPUComputePassEncoder!)
{
    auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
    wgpuComputePassEncoderSetPipeline(p, cpS1...);
    wgpuComputePassEncoderSetBindGroup(p, 0, bgS1..., 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 0);

    wgpuComputePassEncoderSetPipeline(p, cpS2);
    wgpuComputePassEncoderSetBindGroup(p, 0, bgS2..., 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 12);

    wgpuComputePassEncoderSetPipeline(p, cpFallback);
    wgpuComputePassEncoderSetBindGroup(p, 0, bgFallback..., 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 24);

    wgpuComputePassEncoderEnd(p);
    wgpuComputePassEncoderRelease(p);
}
```

---

## 3. Correctness & Matrix Qualification (120 Cases + Negative Control)

### A. The 120-Case Control Suite
Executed across 6 poses, 4 repeat counts, 4 work budgets, and 2 memory capacities across all four routed modes:
- **Mode 2 (30 cases):** Passed 100.0000% exact bit parity and packet equality with bulk reference.
- **Mode 3 (30 cases):** Passed 100.0000% exact bit parity; verified 0 global `sBuf` writes on terminal S1 rays.
- **Mode 4 (30 cases):** Passed 100.0000% exact bit parity and packet equality with bulk reference.
- **Mode 5 (30 cases):** Passed 100.0000% exact bit parity; verified 0 global `sBuf` writes on terminal S1 rays.
- **All 120 Cases:** 0 producer entry errors, 0 final output publication errors.

### B. Negative Mutation Control
- Mutation: `--mutation fallback-missing-publish` (omitted `atomicAdd` on fallback output write).
- Executed on Apple M5 hardware, failing at the budget-0 case of **old Mode 2** with `routed exactly-once final output publication failure: 32784 errors` (wrapper exit **250**). This saved negative run never reaches candidate Modes 4/5. §6ea adds actual candidate-mode execution and targeted mutation checks.

---

## 4. Two Clean Independent Matched Timing Invocations

Evaluated on Apple M5 using balanced ABBA block scheduling:
- Block 0: `{0, 1, 2, 3, 4, 5, 5, 4, 3, 2, 1, 0}`
- Block 1: `{5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5}`
- Pooled 16 observations per mode per repeat in each invocation.

### Detailed Pooled Latency Breakdown:

#### Invocation 1 (`/private/tmp/earthcall-6dz-clean-timing-1`)
- **Repeat 1:**
  - Mode 0 (Current): mean 2.533 ms, **median 2.534 ms**
  - Mode 1 (Bulk): mean 2.528 ms, median 2.527 ms
  - Mode 2 (Unchanged Unfused): mean 2.517 ms, **median 2.532 ms**
  - Mode 4 (Coalesced Unfused): mean 2.533 ms, **median 2.533 ms (+0.06% vs M2)**
  - Mode 3 (Unchanged Terminal): mean 2.533 ms, **median 2.533 ms**
  - Mode 5 (Coalesced Terminal): mean 2.523 ms, **median 2.532 ms (−0.05% vs M3)**
- **Repeat 2:**
  - Mode 0: med 3.809 ms | Mode 2: med 3.818 ms | Mode 4: med 3.809 ms (−0.24% vs M2)
  - Mode 3: med 4.409 ms | Mode 5: med 3.840 ms (−12.91% vs M3)
- **Repeat 4:**
  - Mode 0: med 6.265 ms | Mode 2: med 6.279 ms | Mode 4: med 6.353 ms (+1.19% vs M2)
  - Mode 3: med 6.445 ms | Mode 5: med 6.386 ms (−0.92% vs M3)

#### Invocation 2 (`/private/tmp/earthcall-6dz-clean-timing-2`)
- **Repeat 1:**
  - Mode 0 (Current): mean 2.529 ms, **median 2.528 ms**
  - Mode 1 (Bulk): mean 2.605 ms, median 2.527 ms
  - Mode 2 (Unchanged Unfused): mean 2.603 ms, **median 2.522 ms**
  - Mode 4 (Coalesced Unfused): mean 2.528 ms, **median 2.527 ms (+0.19% vs M2)**
  - Mode 3 (Unchanged Terminal): mean 2.524 ms, **median 2.522 ms**
  - Mode 5 (Coalesced Terminal): mean 2.528 ms, **median 2.526 ms (+0.18% vs M3)**
- **Repeat 2:**
  - Mode 0: med 3.782 ms | Mode 2: med 3.798 ms | Mode 4: med 3.796 ms (−0.07% vs M2)
  - Mode 3: med 4.373 ms | Mode 5: med 4.427 ms (+1.23% vs M3)
- **Repeat 4:**
  - Mode 0: med 6.957 ms | Mode 2: med 7.011 ms | Mode 4: med 7.568 ms (+7.95% vs M2)
  - Mode 3: med 8.207 ms | Mode 5: med 8.225 ms (+0.22% vs M3)

---

## 5. Architectural Counsel for the Council

1. **Stop this coalescing candidate:** no substantial repeated Current gain is established in the scoped evidence. Other dispatch hypotheses remain untested.
2. **Measure work before selecting the next intervention:** census exact duplicate adaptive-interior `evalPhi` evaluations across the three level searches, excluding the nine already shared coarse knots and already prepared factors. Identify actual S1/S2/general demand separately, and preserve f32 evaluation order and refusal behavior.
3. **Keep monotone specialization conditional:** a future separate kernel needs actual per-ray eligibility for every active bin/level, including execution error, uncertain bands and affine boundaries, plus safe routing and fallback. A monotone wave alone is insufficient. Zero local arrays do not guarantee zero compiler spill or a runtime gain.

---

## 6. Deliverables & Sign-Off

- **Executable Probe:** [`scratch/probes/ray_transport_gpu_6dz_producer_coalescing.py`](../../scratch/probes/ray_transport_gpu_6dz_producer_coalescing.py)
- **Audit JSON:** [`docs/audits/GPU_RAY_TRANSPORT_6DZ_PRODUCER_PASS_COALESCING_2026-10-26.json`](GPU_RAY_TRANSPORT_6DZ_PRODUCER_PASS_COALESCING_2026-10-26.json)
- **Status:** Complete. The native lane is hereby **RELEASED**.

*Antigravity / Gemini 2.5 Pro (Sparkly Guy) / Session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` / 2026-10-26T23:10:00-07:00*


## §6ea corrective review — 2026-10-27

Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a`; commissioned by Zach to review Sparkly’s return and send the next bounded quest. [Source-bound review and portable evidence](GPU_RAY_TRANSPORT_6EA_COALESCING_REVIEW_2026-10-27.md).

The saved 120-case matrix and all three existing timing invocations are preserved, with raw bytes reconstructible from compact lossless artifacts. The two later invocations remain separate runs, each containing two internal series; every internal series contains four alternating 12-observation mode blocks. The headline ±0.19% range described completion medians only. Repeated batch slowdowns versus Current remain, and no shipping gain is admitted. Xcode table keys are identifiers, not cryptographic shader/binary hashes; source SHA-256 and pipeline facts bind the actual identical shaders and observed compiler fields. The static spill field is not dynamic traffic or occupancy.

The repaired generator can select one actual routed mode, applies reset/tail mutations to candidate branches after insertion, and handles the different indexed/vector S1 route-count spellings. Fixed default output reconstructs the saved 120-case C++ byte-for-byte. Capture-table extraction now skips buffer payloads by header, refuses malformed plists and requires two S1 compiler rows. Fresh candidate correctness and negative results are recorded in §6ea; no fresh timing was run. Production renderer, saves, defaults and app are unchanged; all three shipping gates remain open.
