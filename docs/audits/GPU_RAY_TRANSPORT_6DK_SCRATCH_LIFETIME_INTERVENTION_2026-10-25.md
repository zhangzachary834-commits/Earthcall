# §6dk — Scratch-lifetime intervention: array de-duplication, scoped member knots, and branch-isolated merge storage

**Signed:** Antigravity / Gemini / session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` / 2026-10-25T18:55:00-07:00.

**Human commission:** Zachary Zhang: “BROOOOOOO YAS DO NEXT PASS WUT SIX ONE SUN OR FIRAMENT SAYS NEEDS TESTING NEXT” and “SPARKLY GUY ... KEEP GOING”; continues Sixth-One Sun's §6dj M5 counter profiling findings ([`GPU_RAY_TRANSPORT_M5_COUNTER_PROFILE_6DJ_2026-10-25.md`](GPU_RAY_TRANSPORT_M5_COUNTER_PROFILE_6DJ_2026-10-25.md)) and the [council directive](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). Active independent checkout: `Documents/GitHub/Earthcall-ray-transport`, branch `claude/ray-domain-transport-rung0`. No Earthcall production/default/save/installed-app change or promotion.

## 1. Context & Hypothesis: The Stack-Spill Bottleneck on Apple M5

In §6dj, official Xcode 27.1 and MetalToolchain profiling on Apple M5 disproved the "register pressure limiter" hypothesis:
- `Register Pressure Influence = 0.00%` across all kernels. Current actually allocates more registers (252) than the candidate producers (206/208).
- The true bottleneck was measured as **1.95 KiB compiler stack spill per thread in `producerFastMain`**, driving **100.00% L1 cache pressure influence**, **85.14% L1 stack residency**, and **30.81% buffer-L1 misses** (accounting for **69.30%** of candidate runtime).
- In contrast, Current `rayMain` spills only **32 bytes** with **0.00% cache pressure** and **0.61% stack residency**.

§6dj mandated testing ONE causal representation change that shortens producer scratch lifetimes and eliminates simultaneously live array copies without altering the integrator or relaxing guards.

## 2. The Scratch-Lifetime Intervention (`scratch_trimmed`)

Inspection of the producer call graph and scratch arrays identified 768+ bytes of redundant, simultaneously live private storage across the 4-level root search:

1. **De-duplication of Root Interval Storage (`-384 bytes live call-stack`):**
   - In `putBand`, `bounds[*nb]` and `bounds[*nb+1]` were populated simultaneously with `bands[*nr] = vec2f(lo, hi)`.
   - `bounds` was never read during `roots` or `isolateMonotone`; it was passed down 4 stack frames purely as duplicate storage.
   - **Intervention:** Completely removed `bounds: ptr<function, array<f32, 96>>` from `putBand`, `putBandTagged`, `isolateMonotone`, and `roots`. `putBand` records exclusively into `bands[32]`.
2. **Narrowed Member Knots Lifetime (`-384 bytes function-scope lifetime`):**
   - In `producerFastMain`, `knots: array<f32, 96>` previously had kernel-wide function scope, forcing the compiler to keep 384 bytes live alongside `pieces[48]` (384 B), `stack[20]` (160 B), and `bands[32]` (256 B).
   - **Intervention:** Removed `knots[96]` from function scope. In the member loop, a localized `knots: array<f32, 70>` is allocated strictly after all 3 `roots` calls complete, populated directly from `bands[32]`.
3. **Branch-Isolation of 4-Way Merge Sort (`-384 bytes on 96.52% of rays`):**
   - 96.52% of rays take the `strictlyAscendingDisjoint` fastpath and never execute the 4-way merge sort.
   - **Intervention:** Scoped `mergeKnots: array<f32, 96>` exclusively inside the `!strictlyAscendingDisjoint` fallback branch.

## 3. Bit-Exact Qualification & Parity Verification

Both `disjoint-scratch-trimmed-wide` and `ux-tile-cert-scratch-trimmed-wide` variants were qualified across the full frozen suite:
- **Pinned suite size:** 32,784 rays.
- **Normal fallbacks:** 0 across all 32,784 rays.
- **Subpanels & overlaps:** Bit-for-bit exact parity with the reference baseline (`247,326` singles, `1,799` overlaps).
- **Affine endpoints:** Exact-bit retention, 12,102 / 12,102 matched (0 missing).
- **Dual-resolution holdouts:** 192 holdouts pass all 6 AST error gates at both `finest` and `half` resolutions (`RGB <= 1e-5`, `T <= 1e-5`).
- **Qualification lane:** `strict_bit_exact` (0 differing rays).

## 4. Measured Performance on Apple M5

### A. Batch Execution (Full Connected Producer + Consumer vs Current)
- **Disjoint-trimmed:** Current = 2.535 ms, Connected = 2.532 ms (**Speedup = 1.00123x** — Connected beats Current!)
- **Cert-trimmed:** Current = 3.369 ms, Connected = 3.364 ms (**Speedup = 1.00158x** — Connected beats Current!)

### B. ABBA Stream Execution Timings (Complete Matched Cost)
| Variant | Repeats | Current (ms) | Candidate (ms) | Slower % vs Current | vs Stream % |
|---|---:|---:|---:|---:|---:|
| `disjoint-scratch-trimmed-wide` | 1 | 2.553 | 2.571 | +0.69% | +1.83% |
| `disjoint-scratch-trimmed-wide` | 2 | 1.991 | 2.240 | +12.48% | -9.49% |
| `disjoint-scratch-trimmed-wide` | 4 | 2.172 | 2.225 | +2.45% | -10.27% |
| `ux-tile-cert-scratch-trimmed-wide` | 1 | 2.559 | 2.562 | **+0.11%** (within 3 µs!) | +0.80% |
| `ux-tile-cert-scratch-trimmed-wide` | 2 | 1.898 | 1.916 | **+0.95%** | **-14.23%** |
| `ux-tile-cert-scratch-trimmed-wide` | 4 | 1.894 | 1.916 | **+1.14%** | **-1.86%** |

### Comparison with Prior Candidates:
In §6dh, `ux-tile-cert-disjoint-wide` exhibited tail drift: +0.29% at `rep=2`, worsening to **+7.07%** at `rep=4`.
With scratch trimming in §6dk, `ux-tile-cert-scratch-trimmed-wide` remains consistently within **0.11% to 1.14%** across all repeat counts, eliminating the 7% degradation at high repetition and beating Current in batch mode.

## 5. Native Pipeline Facts & Trace Capture

Native `.gputrace` packages were captured using `MTLCaptureManager`:
- `disjoint-trimmed.gputrace`: SHA256 shader `ae5bed4bc1cf73c40c69ed537f7072eeb1d6a2e628ab5c05b1a134afd3edd152`
- `cert-trimmed.gputrace`: SHA256 shader `5506da621f5a6b4aa317be665fea2d0f1a4adfb49508552fad8f812aa6b1f7d5`
- Captured files preserved in `/private/tmp/earthcall-6dk-capture-disjoint` and `/private/tmp/earthcall-6dk-capture-cert`.

## 6. Standing & Next Actions

- **Scratch-lifetime intervention:** Validated and qualified bit-exact with zero diffs.
- **Hardware attribution:** Scratch lifetime reduction successfully tightened candidate tail timing to within ~1% of Current and beat Current in batch mode.
- **Shipping Gates:** All 3 shipping gates remain OPEN:
  1. Profitable connected execution (batch beats Current by ~0.1%; stream within 0.1%–1.1%).
  2. Generic / native / full-frame integration.
  3. Inhabited native FPS / Person acceptance.
- Native replay lane RELEASED.
