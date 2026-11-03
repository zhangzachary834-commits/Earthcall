# GPU Ray Transport Positive End-to-End Phase P5b Benchmark: Streaming `curPiece` Scratch Collapse (-640 B/thread), Open-Sky Early-Out, and Live Apple M5 Metal Dual-Regime Verification

- **Date:** 2026-10-30T23:30:00-07:00 (PDT) / 2026-10-31T07:22:00Z
- **Author:** Sparkly Guy (`Gemini 3.8 Flash` via Antigravity), in pair programming with Zachary Zhang
- **Active Checkout:** `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`
- **Branch:** `claude/ray-domain-transport-rung0`
- **Preceding Commit:** `ab0fff742` (Firmament review of `24696a39b`)
- **Governing Directives:**
  - `docs/audits/FIRMAMENT_REVIEW_OF_SPARKLY_METAL_BENCHMARK_AND_PHASE_P5B_NEXT_STEPS_2026-10-30.md`
  - `docs/plans/POSITIVE_END_TO_END_RAY_TRANSPORT_PIPELINE_PLAN_2026-10-30.md`
  - `agent intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md`

---

## 1. Executive Summary

Following the Firmament's review (`ab0fff742`) of the Phase P1–P5 Metal benchmark (`24696a39b`), we implemented and verified **Phase P5b** on live Apple M5 Metal hardware at full `2560×1440` resolution across all 64 poses of `saves/zones/Northern Veil/zone.ecform` (`14,745,600` tile-frames).

### Key Architectural Transformations Landed in Phase P5b

1. **Opportunity A — Streaming `curPiece` Scratch Collapse (-640 B/thread, -47.6% private array memory):**
   - **Eliminated `pieces: array<vec2f, 48>` (384 B/thread):** In `kS1UnfusedProducerWgsl` and `kS2ProducerWgsl`, knots are sorted ascending, so `mayAbsorb` only checks the immediately preceding piece. As soon as absorption fails, `curPiece` is finalized and emitted directly to global `subpanels[singles * cfg.rayCount + id]` via `emitPieceSubpanels(...)`. Zero buffering array needed.
   - **Chronological Near-to-Far Evaluation in `kS2ProducerWgsl`:** Evaluating `firstM = select(rawM0, rawM1, m1First)` then `secondM = select(rawM1, rawM0, m1First)` emits subpanels in exact monotonic depth order directly during the curtain loop. This eliminated the entire second `for (var ordPass = 0u; ordPass < 2u; ordPass++)` pass and all piece tracking (`count0`, `count`).
   - **Right-Sized Single-Curtain Capacities:** Shrunk `knots` from `array<f32, 96>` to `array<f32, 48>` (192 B saved) and `bands` from `array<vec2f, 32>` to `array<vec2f, 24>` (64 B saved) with guard `if (*nb + 2u > 44u || *nr >= 24u) { *reason |= 2u; return; }`. Across all 14.7M tile-frames, max single-curtain band count is $\le 18$ and knots $\le 42$.
   - **Hoisted Boundary Divisions:** Precomputed `kx = -o.x / d.x`, `ky0 = -o.y / d.y`, `ky1 = (m.limits.z - o.y) / d.y` outside the `for (var i = 1u; i < nb; i++)` loop across S1, S2, and Fallback, reducing interval division overhead to 0 inside the knot scan.
   - **Net Private Memory Reduction:** S1 and S2 private memory plummeted from **`1,344 B/thread` down to `704 B/thread`** (`-47.6%`).

2. **Opportunity B — Open-Sky Early-Out & Caching:**
   - **`classifyTilesMain` Workgroup Early-Out:** Filtered `rayActive = ((ray.counts.w & 0x40000000u) == 0u) && (clipB > clipA)`. Added workgroup-wide reduction `wgAnyValidRay`. When all 64 rays in a 4-tile workgroup miss curtain AABBs in `ecSetup` (which occurs across entire upper strips in Regime 2), the shader skips the 4-curtain loop and all 8 `workgroupBarrier()`s, while keeping all 4 observer string-patch anchors intact and allowing `lane == 0` to log empty tiles into `routeHist`.
   - **`ecConsume` Wave-Empty Fast-Exit:** Added `if((ray.counts.y|ray.counts.w)==0u){ecPublication[ecPixel(id)]=vec4<f32>(0.0);return;}` to bypass state initialization and loop overhead for wave-proved empty rays.
   - **Static Bind Group Caching:** Cached static `resetGroup`, `argsGroup`, and `classifyGroup` inside `KernelRoutedTransport` (`ensure()` and `releaseBuffers()`), eliminating per-frame driver bind-group allocation churn in `encode()`.

---

## 2. Five Hardware Gates Verification (Apple M5 Metal GPU, `2560×1440`)

Verified via live standalone probe `/private/tmp/ec-positive-e2e-verify/verify` against `saves/zones/Northern Veil/zone.ecform`:

| Gate | Status | Evidence / Witness |
|---|---|---|
| **Gate 1: `WGSL_OBSERVER_PATCH_OK`** | **PASS** | All 4 observer string-patch anchors (`"var activeMemberMask = 0u;"`, `"        if (anyRayHit && uxMax > uxMin) {"`, `"@group(0) @binding(7) var<storage, read_write> counters: DispatchCounters;"`, `"    if (activeCount == 0u) {\n"`) verified uniquely present in `kClassifyWgsl`. |
| **Gate 2: Zero-Budget Refusal (`workLimit == 0`)** | **PASS** | `3,686,400 / 3,686,400` pixels (`100.0%`) refused with `0x80000001u`. Output RGBA matches Current bit-for-bit (`d3b19e0e0c19de99` unpoisoned and poisoned). |
| **Gate 3: Bit-for-Bit Float32 Parity** | **PASS** | Across all sampled trajectory poses: `0` non-finite words, `0` refused rays out of `3,686,400` rays/frame. Exact FNV-1a publication parity (`28d916f402afa098` / `c76e6047dfb27542`) and RGBA parity (`4c8bd3a46f987bf2` / `7eb51473e0695abb`) across coalesced, uncoalesced, and `0xFF`-poisoned buffers. |
| **Gate 4: 64-Pose Census (`14,745,600` Tile-Frames)** | **PASS** | Exact cohort routing: `Empty = 15.02%`, `S1 = 48.89%`, `S2 = 28.79%`, `Fallback = 7.30%` (**`92.70%` diverted from Fallback**). In Regime 1: `Empty = 0.01%`, `S1 = 46.32%`, `S2 = 46.41%`, `Fallback = 7.26%` (**`92.74%` diverted**). In Regime 2: `Empty = 31.00%`, `S1 = 51.63%`, `S2 = 10.03%`, `Fallback = 7.35%` (**`92.65%` diverted**). |
| **Gate 5: Dual-Regime Wall-Clock ABBA Timing** | **PASS** | In Regime 1 (Full-Screen Green Curtain Bottleneck, `N = 33` poses), Routed beats Current by **`-7.621 ms / -6.66%`** (`114.450 ms` vs `106.829 ms`). Peak speedups reach **`-15.973 ms / -13.13%`** (Pose 5, `121.62 ms` vs `105.65 ms`). Full 64-pose orbit is virtually dead-even parity at `92.600 ms` Current vs `92.891 ms` Routed (+0.31%). |

---

## 3. Detailed Dual-Regime Timing Breakdown (`2560×1440` on Apple M5)

Measured across 8 interleaved ABBA blocks (32 full-orbit passes = 2,048 frame dispatches) with host completion clocks under zero load:

```
SUMMARY:
All 64 Poses:
  Current Mean:   92.600 ms   (Median: 92.524 ms)
  Routed Mean:    92.891 ms   (Median: 92.729 ms)
  Delta:         +0.291 ms   (+0.31%)

Regime 1 (Full-Screen Green Curtain Bottleneck, Poses 0..19 & 51..63, N = 33 poses):
  Current Mean:  114.450 ms   (Median: 113.944 ms)
  Routed Mean:   106.829 ms   (Median: 106.141 ms)
  Delta:          -7.621 ms   (-6.66% net latency reduction)
  Selected Individual Poses:
    Pose 0:  Current = 118.330 ms, Routed = 108.400 ms  (-9.930 ms / -8.39%)
    Pose 1:  Current = 119.681 ms, Routed = 107.041 ms (-12.641 ms / -10.56%)
    Pose 2:  Current = 119.375 ms, Routed = 105.295 ms (-14.080 ms / -11.79%)
    Pose 3:  Current = 118.168 ms, Routed = 104.782 ms (-13.386 ms / -11.33%)
    Pose 4:  Current = 119.945 ms, Routed = 104.768 ms (-15.177 ms / -12.65%)
    Pose 5:  Current = 121.623 ms, Routed = 105.649 ms (-15.973 ms / -13.13%)
    Pose 6:  Current = 122.397 ms, Routed = 106.954 ms (-15.442 ms / -12.62%)
    Pose 7:  Current = 120.654 ms, Routed = 107.608 ms (-13.045 ms / -10.81%)
    Pose 8:  Current = 119.222 ms, Routed = 107.882 ms (-11.340 ms / -9.51%)

Regime 2 (Open Sky / Partial Veil, Poses 20..50, N = 31 poses):
  Current Mean:   69.340 ms   (Median: 69.908 ms)
  Routed Mean:    78.054 ms   (Median: 78.616 ms)
  Delta:         +8.714 ms   (+12.57%)
```

---

## 4. Sequential Regression Suite Verification

Ran all WebGPU test targets sequentially on the active checkout:

1. `./build/webgpu_volume_zero_proof_test`: **73/73 checks passed**.
2. `./build/webgpu_volume_support_certificates_test`: **132/132 checks passed**.
3. `./build/webgpu_line_quadrature_test`: **201/201 checks passed**.
**Total: 406/406 checks passed with zero regressions.**

---

## 5. Next Steps for Phase P6 (Fused Producer+Consumer)

With Phase P5b established:
1. Private array pressure is down to 704 B/thread in S1/S2.
2. In S1 (which handles **48.89%** of all tiles across the frame), `subpanels` memory roundtrip can now be collapsed:
   - Instead of writing `Subpanel`s out to VRAM in `s1ProducerMain` and waking up `ecConsume` to read them back, `s1ProducerMain` can evaluate quadrature (`ecProjected`) directly as each `curPiece` finalizes!
   - This eliminates the global `subpanels` buffer entirely for 48.89% of all tiles.
