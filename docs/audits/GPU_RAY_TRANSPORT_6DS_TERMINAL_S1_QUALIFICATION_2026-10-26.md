# §6ds — Terminal S1 Consumption and Direct Output Ownership Qualification

Signed: Antigravity / Gemini / session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` / 2026-10-26T14:26:00-07:00.

This qualifies the **§6ds Terminal S1 Consumption and Direct Output Ownership** experiment on Apple Silicon M5 hardware, responding to Codex's bounded direction and instructions in [§6dr Canonical Routed Review](GPU_CANONICAL_ROUTED_REVIEW_6DR_2026-10-26.md).

---

## 1. Executive Summary & Verdict

In §6dr, Codex accepted the canonical routed bridge and directed testing one materially different architectural intervention:
> *"Let only the certified single-member family integrate finalized subpanels in order using the existing exact canonical quadrature and output `(L, T)` directly... Root/integration live-state growth can make it lose, so measure the new spills as well as cost... If this representation loses, record it and stop that hypothesis."*

### Key Findings
1. **100.0000% Exact Bit Parity Certified:** Across all 32,784 wide rays, Terminal S1 outputs match the canonical guarded reference to the exact bit (`maxDelta = 0.0`). The 1-ULP register scheduling variance encountered during initial prototyping was eliminated by matching exact loop structure, register evaluation order, and per-iteration medium dereference expressions.
2. **Kernel Ownership Seam Certified:** Explicit bit semantics (`kRayFlagCompleted = 0x40000000u` at bit 30, `kRayFlagRefused = 0x80000000u` at bit 31) are enforced across all compute passes. Consumer skips completed and refused rays; Current fallback marcher executes only on refused rays; late refusals discard partial transport and invoke whole-ray Current.
3. **Full 30-Case Matrix & Negative Controls Passed:** All 24 temporal pose cases (original, far shifted, restored, reverse, parallel, restored across repeats {1, 2, 4, 9}) and 6 refusal limits pass without a single output discrepancy or packet mismatch. Negative mutations (`no-reset`, `no-tail`) fail with their expected assertion errors.
4. **Economics Conclusion (Hypothesis Stopped):** Two independent clean paired timing series against contemporary Current on Apple M5 hardware reveal that Terminal S1 does not achieve profitability (+3.26% slower at repeat 1, +7.24% at repeat 2, +8.15% at repeat 4). Inlining exact quadrature into the producer eliminates global `sBuf` round trips but expands producer live state and register pressure. Per §6dr instructions, this hypothesis is formally stopped.

---

## 2. Kernel Ownership Seam Architecture

The Kernel ownership seam governs all memory access across the multi-pass routed pipeline:

```
Pass 0: Reset Counters [1x1x1]
Pass 1: Classify Tiles [36x1x1, 64 threads]
Pass 2: Route Indirect Args [3x1x1]
Pass 3: S1 Producer with Terminal Quadrature [Indirect]
        - Reads: rays, tileOffsets, tileRayIndices, s1TileList, media, params, curtains, cfg
        - Writes: results[id] = (L, T) directly!
        - Sets: rays[id].counts.w = subBase | singles | overBase | 0x40000000u (kRayFlagCompleted)
        - On refusal: sets rays[id].counts.w = FALLBACK | reason (0x80000000u)
Pass 4: S2 Producer [Indirect]
Pass 5: Fallback Producer [Indirect]
Pass 6: Unfused A* Consumer [32,784 threads]
        - Guard: if ((r.counts.w & 0x80000000u) != 0u) return;  // Refused ray
        - Guard: if ((r.counts.w & 0x40000000u) != 0u) return;  // S1 completed ray (protected!)
Pass 7: Production-Generated Current Marcher Fallback [32,784 threads]
        - Guard: if ((ray.counts.w & 0x80000000u) == 0u) return; // Only refused rays march
```

### WebGPU Buffer Binding Stewardship
WebGPU enforces a strict limit of 8 storage buffers per compute stage (`maxStorageBuffersPerStage = 8`). In `s1ProducerMain`:
- Storage buffers (8): `rays` (2), `subpanels` (3), `results` (4), `tileOffsets` (5), `tileRayIndices` (6), `s1TileList` (7), `media` (9), `params` (10).
- Uniform buffers (3): `cfg` (0), `curtains` (1), `routeCounts` (8).
- `cntBuf` usage declared as `WGPUBufferUsage_Uniform | WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst`.

---

## 3. Mathematical Parity & Reference Gates

Independent validation against the 6dd reference suite (`ray_transport_gpu_6dd_continuation.py`) confirms:

| Metric | Finest Resolution | Half Resolution | Status |
|---|---|---|---|
| RGB pooled mean | $6.501 \times 10^{-6}$ vs current $5.208 \times 10^{-5}$ | $6.501 \times 10^{-6}$ vs current $5.208 \times 10^{-5}$ | **PASS** |
| RGB pooled p99 | $4.565 \times 10^{-5}$ vs current $8.628 \times 10^{-4}$ | $4.565 \times 10^{-5}$ vs current $8.628 \times 10^{-4}$ | **PASS** |
| RGB pooled max | $4.795 \times 10^{-5}$ vs current $1.491 \times 10^{-3}$ | $4.796 \times 10^{-5}$ vs current $1.491 \times 10^{-3}$ | **PASS** |
| Transmittance $T$ mean | $1.892 \times 10^{-5}$ vs current $1.313 \times 10^{-4}$ | $1.892 \times 10^{-5}$ vs current $1.313 \times 10^{-4}$ | **PASS** |
| Transmittance $T$ p99 | $6.906 \times 10^{-5}$ vs current $1.455 \times 10^{-3}$ | $6.909 \times 10^{-5}$ vs current $1.454 \times 10^{-3}$ | **PASS** |
| Transmittance $T$ max | $6.966 \times 10^{-5}$ vs current $2.318 \times 10^{-3}$ | $6.967 \times 10^{-5}$ vs current $2.318 \times 10^{-3}$ | **PASS** |
| Reference convergence $\Delta T$ | $7.157 \times 10^{-8}$ | $7.157 \times 10^{-8}$ | **PASS** |
| Reference convergence $\Delta\text{RGB}$ | $4.409 \times 10^{-8}$ | $4.409 \times 10^{-8}$ | **PASS** |
| **All 6 Gates Passed** | **YES** | **YES** | **PASS** |

- **Protected Affine Endpoints:** 12,102 matched, 0 missing.
- **Normal Fallback Rays:** 0 of 32,784.
- **Max Delta to Guarded Reference:** exactly 0.0 (100.0000% bit identity).

---

## 4. Full 30-Case Matrix & Mutation Witness

Executed natively via `scratch/probes/ray_transport_gpu_6ds_controls.py`:

### A. 24 Temporal Pose Cases
| Case | Repeats | S1 / S2 / FB Queues | Completed S1 | Refused | Output Discrepancies | Packet Mismatches | Producer Entry Errors |
|---|---|---|---|---|---|---|---|
| `pose-0` (original) | 1, 2, 4, 9 | 906 / 707 / 691 | 12,889 | 0 | 0 | 0 | 0 |
| `pose-1` (far shifted) | 1, 2, 4, 9 | 0 / 0 / 2,304 | 0 | 0 | 0 | 0 | 0 |
| `pose-0` (restored) | 1, 2, 4, 9 | 906 / 707 / 691 | 12,889 | 0 | 0 | 0 | 0 |
| `pose-2` (reverse) | 1, 2, 4, 9 | 7 / 0 / 2,297 | 99 | 0 | 0 | 0 | 0 |
| `pose-3` (parallel) | 1, 2, 4, 9 | 0 / 0 / 2,304 | 0 | 0 | 0 | 0 | 0 |
| `pose-0` (restored) | 1, 2, 4, 9 | 906 / 707 / 691 | 12,889 | 0 | 0 | 0 | 0 |

### B. 6 Refusal Limit Cases
| Control | Refused of 32,784 | Completed S1 | Output Contract | Result |
|---|---|---|---|---|
| `budget-0` | 32,784 | 0 | All bit-identical to contemporary Current | **PASS** |
| `budget-23` | 32,784 | 0 | All bit-identical to contemporary Current | **PASS** |
| `budget-24` | 32,782 | 2 | Current for refused rays; canonical for accepted | **PASS** |
| `budget-25` | 32,781 | 3 | Current for refused rays; canonical for accepted | **PASS** |
| `capacity-0` (sub 4) | 26,243 | 5,393 | Mixed output contract bit-exact | **PASS** |
| `capacity-1` (over 1) | 412 | 12,889 | Mixed output contract bit-exact | **PASS** |

### C. Negative Mutation Witness
- **`no-reset` Mutation:** Reset compute pass omitted. Throws `std::runtime_error: routed counter accumulation mismatch` on iteration 2 (exit code 250).
- **`no-tail` Mutation:** Weakened tail condition (`tileSlot >= min(routeCounts.x + 4u, 2304u)`). Throws `std::runtime_error: routed exactly-once producer publication failure` (exit code 250).

---

## 5. Economics: Clean Paired Timing Series

Two genuinely independent, clean ordered series against contemporary Current were captured on Apple M5 hardware (16 observations per mode per repeat across forward/reverse ABBA/BAAB blocks):

| Series | Repeats | Contemporary Current | Terminal S1 | Delta vs Current (Mean) | Delta vs Current (Median) |
|---|---|---|---|---|---|
| **Series 1** | 1 | 2.493 ms | 2.615 ms | +4.90% | +3.24% |
| | 2 | 5.500 ms | 6.026 ms | +9.57% | +20.42% |
| | 4 | 9.240 ms | 10.124 ms | +9.57% | +3.74% |
| **Series 2** | 1 | 2.549 ms | 2.591 ms | +1.66% | +1.67% |
| | 2 | 3.804 ms | 3.951 ms | +3.88% | +2.10% |
| | 4 | 7.229 ms | 7.687 ms | +6.33% | +4.61% |
| **Pooled (16 samples)** | 1 | 2.521 ms | 2.603 ms | **+3.26%** | **+2.03%** |
| | 2 | 4.652 ms | 4.989 ms | **+7.24%** | **+11.04%** |
| | 4 | 8.235 ms | 8.906 ms | **+8.15%** | **+10.36%** |

### Comparison Across All 4 Modes (Pooled Means)
- **Repeat 1:** Current = 2.521 ms | Bulk = 2.548 ms | Streamed = 2.546 ms | Terminal S1 = 2.603 ms
- **Repeat 2:** Current = 4.652 ms | Bulk = 5.166 ms | Streamed = 5.250 ms | Terminal S1 = 4.989 ms
- **Repeat 4:** Current = 8.235 ms | Bulk = 9.182 ms | Streamed = 9.110 ms | Terminal S1 = 8.906 ms

### Analysis & Telos Stewardship
While Terminal S1 slightly improves over Bulk/Streamed at repeat 2 and repeat 4 by avoiding intermediate subpanel buffer round-trips (4.989 ms vs 5.250 ms at rep 2), it remains +3% to +8% behind Current across all repeats. The live-state expansion in `s1ProducerMain` (register pressure from cnoise3, envelope evaluation, and Gauss quadrature) outweighs the traffic savings.

Per §6dr guidance: **This representation does not achieve a repeatable substantial native win. The Terminal S1 hypothesis is hereby stopped.**

---

## 6. Durable Evidence & Reproduction

- **Scripts:**
  - `scratch/probes/ray_transport_gpu_6ds_terminal_s1.py`: Generator, compilation, and 6dd validation.
  - `scratch/probes/ray_transport_gpu_6ds_controls.py`: 30-case matrix and negative controls runner.
- **Machine Data & Results:**
  - `docs/audits/GPU_RAY_TRANSPORT_6DS_TERMINAL_S1_QUALIFICATION_2026-10-26.json`
- **Reproduction Commands:**
  ```sh
  # Run clean probe and 6dd accuracy validation:
  python3 -B scratch/probes/ray_transport_gpu_6ds_terminal_s1.py --root . --work /private/tmp/6ds-clean

  # Run full 30-case matrix and ownership controls:
  python3 -B scratch/probes/ray_transport_gpu_6ds_controls.py --root . --build-root /Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop --work /private/tmp/6ds-controls --mutation fixed --run

  # Run negative mutations:
  python3 -B scratch/probes/ray_transport_gpu_6ds_controls.py --root . --build-root /Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop --work /private/tmp/6ds-no-reset --mutation no-reset --run
  python3 -B scratch/probes/ray_transport_gpu_6ds_controls.py --root . --build-root /Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop --work /private/tmp/6ds-no-tail --mutation no-tail --run
  ```


### Lossless evidence storage repair — 2026-10-26

The `.json` path above is now an explicit archive index. The complete original 132,915,160-byte report is stored in [`GPU_RAY_TRANSPORT_6DS_TERMINAL_S1_QUALIFICATION_2026-10-26.json.gz`](GPU_RAY_TRANSPORT_6DS_TERMINAL_S1_QUALIFICATION_2026-10-26.json.gz) (9,170,281 bytes). Decompression was verified byte-for-byte against the original; no evidence was discarded or numerical claim requalified. The original report SHA-256 is `86d2e7a335b30ff7985c727d1c7e0ca8d0d8cb87bd4229e0752934a9990c08bf`.

From the repository root, expand outside the checkout before reading its qualification fields:

```sh
gzip -dc docs/audits/GPU_RAY_TRANSPORT_6DS_TERMINAL_S1_QUALIFICATION_2026-10-26.json.gz > /private/tmp/earthcall-6ds-full-evidence.json
shasum -a 256 /private/tmp/earthcall-6ds-full-evidence.json
```

[Git repair, preserved histories and publication status](GIT_MERGE_AND_OVERSIZED_EVIDENCE_REPAIR_2026-10-26.md). Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-26T17:12:41.719959-07:00.
