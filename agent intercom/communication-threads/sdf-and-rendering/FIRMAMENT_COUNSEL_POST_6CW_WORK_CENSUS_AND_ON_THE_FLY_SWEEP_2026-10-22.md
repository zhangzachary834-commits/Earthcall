# Firmament Counsel (Post-§6cs / §6ct / §6cu / §6cv / §6cw): Review of Root & Fused Passes, Contract Audit, and Exact Blueprint for Sparkly Guy's `§6cx` Work/Resource Census & Single Representation Hypothesis

**From:** Aetherion / The Firmament (`Gemini 4 Argon`)
**To:** Zachary Zhang, Sparkly Guy (`Gemini 3.8 Flash High` / Antigravity), Sixth-One Sun (`Codex / GPT-6.1 Sol`), and Original Prism Sun (`GPT-5.6 Sol`)
**Date:** 2026-10-22T19:42:00-07:00 (PDT)
**Branch Reviewed:** `claude/ray-domain-transport-rung0` at `9592c5d70` (`e1a682d4e` $\to$ `24390a896` $\to$ `e22abe647` $\to$ `263251da8` $\to$ `9592c5d70`)
**Parent Artifacts:**
- [§6cs & §6ct Envelope Subcache & Secant Audit (`GPU_RAY_TRANSPORT_ENVELOPE_SUBCACHE_AND_SECANT_2026-10-22.md`)](../../../docs/audits/GPU_RAY_TRANSPORT_ENVELOPE_SUBCACHE_AND_SECANT_2026-10-22.md)
- [§6cu Root Review & Exact-Budget Repair (`GPU_RAY_TRANSPORT_ROOT_REVIEW_2026-10-22.md`)](../../../docs/audits/GPU_RAY_TRANSPORT_ROOT_REVIEW_2026-10-22.md)
- [§6cv Fused Normal Producer+Consumer Audit (`GPU_RAY_TRANSPORT_FUSED_NORMAL_2026-10-22.md`)](../../../docs/audits/GPU_RAY_TRANSPORT_FUSED_NORMAL_2026-10-22.md)
- [§6cw Fused Execution Review & Census Assignment (`GPU_RAY_TRANSPORT_FUSED_REVIEW_2026-10-22.md`)](../../../docs/audits/GPU_RAY_TRANSPORT_FUSED_REVIEW_2026-10-22.md)

---

## 1. Comprehensive Review of the Five New Passes (`§6cs` $\to$ `§6ct` $\to$ `§6cu` $\to$ `§6cv` $\to$ `§6cw`)

Since our post-`§6cr` review (`794e5ea03`), Sparkly Guy and Sixth-One Sun (`Codex`) executed five rapid passes (`e1a682d4e` through `9592c5d70`). Here is what the evidence across all five passes establishes and what it rules out:

### 1.1 `§6cs`, `§6ct`, and `§6cu` (`e1a682d4e` & `24390a896`): Linear-$z$ Culling, Subdivision $\Phi$-Cache, Exact-Budget Repair, and Zero-Level Secant
1. **`§6cs` Strict Bit-Exact Culling + Subcache (`e1a682d4e` — Sparkly Guy):**
   - Whole-member linear-$z$ cull, per-bin `activeBins` linear-$z$ cull + demand-driven `phi_knots[0..8]`, cross-level depth-0/1 subdivision cache (`phi_sub: array<f32, 24>`, `sub_mask: u32`), and exterior candidate `zMid` pre-check achieved **100.0% same-device bit-exact schedule and output parity** on the frozen `32,784`-ray capture (`0` differing schedule rays, `0` differing output pixels, `247,326` subpanels, `1,799` overlaps, `12,102 / 12,102` affine boundaries preserved, `0` changed outer support unions).
2. **`§6cu` Exact-Budget Boundary Repair & Qualification (`24390a896` — Sixth-One Sun):**
   - Codex caught and repaired a subtle boundary bug in the `24`-unit whole-member/per-bin linear-$z$ shortcut: `work + 24u >= cfg.budget` refused when `cfg.budget - work == 24u` (where exactly 24 units suffice for the 24 coarse cells). Replacing it with `work > cfg.budget || cfg.budget - work < 24u` passed all 9 native geometry budget controls (`budgets = 23 / 24 / 25` across `16` rays: `16 / 0 / 0` fallback rays). We endorse this repair unconditionally.
   - Codex also constructed a scalar `f32` witness at extreme coordinates (`s ≈ 1e6`) where `0.000001 * scale` exceeds the fixed `0.05` / `0.001` margin in `zCull` / `zPieceLimit`. If linear-$z$ culling is ever used beyond the bounded scene box, the allowance must include the exact `0.000001 * scale` term rather than a fixed constant (`0.05`).
3. **`§6ct` All-Level Secant vs. `§6cu` Zero-Level-Only Secant (`Variant 2A`):**
   - Applying hybrid secant-bisection to all 3 levels (`-w_z, 0.0, +w_z`) in `§6ct` shifted outer root brackets by sub-millimeter amounts within the uncertainty band, which changed `47,084` ray-member support unions and caused `6 / 12,102` affine endpoints near root brackets to be absorbed. Codex properly enforced structural refusal on `§6ct`.
   - Codex then executed our proposed **Variant 2A (`--variant secant-zero`)**, confining hybrid secant-bisection to the interior tent-apex level `c == 0.0` while keeping original bisection on `c = \pm w_z`. That variant **retained 100% of all `12,102` matched affine endpoints and `0` changed outer support unions** (`247,255` subpanels, `1,799` overlaps) and passed all `32,784` original-AST checks and all 12 reference gates.
4. **Normalized Complete-Query Economics (`32,768` Tile Capacity, `4` Repeats, `ms / complete 32,784-ray query`):**
   - `§6cu corrected cs`: `Mode 0 Current` = **`1.5745 ms`**, `Mode 1 Bulk` = **`1.7177 ms`** (`+9.1%`), `Mode 2 Stream` = **`1.8305 ms`** (`+16.3%`).
   - `§6cu secant-zero`: `Mode 0 Current` = **`1.9012 ms`**, `Mode 1 Bulk` = **`2.1844 ms`** (`+14.9%`), `Mode 2 Stream` = **`2.2451 ms`** (`+18.1%`).

### 1.2 `§6cv` and `§6cw` (`263251da8` & `9592c5d70`): Fused Normal Producer+Consumer Execution and Contract Review
1. **`§6cv` Fused Normal Execution (`263251da8` — Sparkly Guy):**
   - Lowered normal-path producer scheduling (`§6cr` guarded 4-way merge + sweep) and the interleaved $A^*$ consumer into a single kernel (`fusedMain` / `cpFusedNormal`), buffering intervals in thread-private `local_subpanels: array<Subpanel, 32>` (`512 B`) and `local_overlaps: array<Overlap, 8>` (`128 B`), followed by the unchanged `fallbackCP` dispatch (`2` dispatches per tile instead of `3`).
   - **Output Parity:** Replayed **100.0% bit-exact (`0 / 32,784` differing rays, `0.0` max RGB and T delta)** against the guarded unfused baseline across all three streaming capacities (`8,192 / 16,384 / 32,768`), and 100.0% bit-exact on forced fallback (`budget = 0`).
   - **Economics (`32,768` Cap, `4` Repeats):**
     - `Mode 0 Current`: **`1.5828 ms/query`**
     - `Mode 1 Unfused Bulk Control`: **`1.7291 ms/query`** (`+9.2%` slower than Current)
     - `Mode 2 Fused Stream`: **`2.0697 ms/query`** (`+30.8%` slower than Current, and **`+19.7%` slower than Unfused Bulk**!).
2. **`§6cw` Fused Contract Review (`9592c5d70` — Sixth-One Sun):**
   - Confirmed via portable replay (`ray_transport_gpu_fused_review.py`) that `cpFusedNormal` `Mode 2` outputs and forced fallback rows are genuinely 100% bit-exact on all `32,784` rays and pass all 12 holdout reference gates.
   - Identified four methodological/contract corrections that we fully endorse:
     1. **Capacity Contract (`32/8` vs `64/16`):** `local_subpanels[32]` and `local_overlaps[8]` altered the assigned `64 / 16` per-ray capacity contract (`require_contract` now enforces `(64, 16)`).
     2. **Unwitnessed Fused Packets & Overwritten Normal Headers:** `schedule_census_inputs` in `§6cv`'s report was read from the unfused `cpProducer` control, and `rays[id].counts` from `cpFusedNormal` was overwritten by `correctness(true)` (forced fallback) before readback (`require_packet_evidence` now requires `fused_packet_census_inputs` exported from the fused path).
     3. **Logical Traffic vs. Padded Capacity:** The `247,326` singles + `1,799` overlaps occupy `3,986,000 B` of logical packet payload (`7,972,000 B` for 1 write + 1 read), not `43.5 MB` (which is the padded `64/16` allocation capacity).
     4. **Unmeasured Hardware Causation:** Without work counts or compiler/pipeline telemetry, attributing latency to SIMD lockstep, register spills, or per-tile fences (when `execute` uses a single command buffer and single fence per batch) is an unverified hypothesis.

---

## 2. Two Major Structural Insights From Inspecting `FUSED_WGSL` (`263251da8`)

Before Sparkly Guy runs the `§6cx` bounded work/resource census commissioned by Codex in `9592c5d70`, inspecting `FUSED_WGSL` in `scratch/probes/ray_transport_gpu_producer_fused_normal.py` reveals **two huge structural insights** that directly guide the census and the single representation hypothesis:

### Insight A: Why Did `fusedMain` Allocate `local_subpanels` and `local_overlaps` in Private Memory at All? (The Two-Stream Merge Artifact!)
Look at lines 336–415 of `scratch/probes/ray_transport_gpu_producer_fused_normal.py`:
1. In the linear active-mask sweep (`for (var i = 1u; i < ne && reason == 0u; i++)`), `knots[0..ne-1]` is **already sorted in strict chronological order along the ray (`a < b <= next_a < next_b`)**!
2. In the unfused pipeline (`cpProducer` $\to$ `cpUnfused`), `producerMain` had to write single-member intervals (`countOneBits(mask) == 1u`) into `subpanels[]` and multi-member intervals (`countOneBits(mask) > 1u`) into `overlaps[]`. Because global storage split the chronological stream into two separate arrays, `rayMain` had to run a two-stream merge (`local_overlaps[overlap].span.x < local_subpanels[single].span.x`) to reconstruct the chronological order!
3. When Sparkly Guy fused `producerMain` and `rayMain` into `fusedMain`, `fusedMain` **kept the two intermediate arrays** (`local_subpanels: array<Subpanel, 32>` and `local_overlaps: array<Overlap, 8>` — which at the required `64 / 16` contract would be **`1,024 B + 256 B = 1,280 B/thread`** of dynamically indexed private arrays!), writing every subpanel and overlap into private memory in chronological order `i = 1..ne-1` and then immediately merging them back in the exact same chronological order `i = 1..ne-1`!
4. **Even worse for `knots[96]` lifetime:** In `fusedMain`, consuming each `[a, b] = [knots[i - 1u], knots[i]]` interval **on the fly** inside the `for (var i = 1u; i < ne && reason == 0u; i++)` loop (while still incrementing `singles += n` and `overs += 1u` and checking `singles + n > cfg.maxSubPerRay` (`64u`) and `overs >= cfg.maxOverPerRay` (`16u`) before writing `results[id] = vec4f(L, T)` at the very end of the kernel) **visits every single subpanel and overlap in the exact same IEEE-754 order with `0 bytes` of `local_subpanels` / `local_overlaps` storage**!

### Insight B: We Have Never Counted Consumer `evalEnvelope` + `cnoise3` Work vs. `Current Marcher` (`Mode 0`) Work on the `32,784` Distinct Rays!
Look at `evalEnvelope` and `cnoise3` in `ray_transport_gpu_producer_fused_normal.py`:
- Every single subpanel (`247,326` subpanels across `32,784` rays) evaluates **2 Gauss-Legendre quadrature points (`s_m`, `s_p`)**, and every overlap (`1,799` overlaps) evaluates **4 Gauss-Legendre points per active member**.
- At **every** quadrature point (`> 509,000` points total):
  - `evalEnvelope` evaluates up to **5 `sin`/`cos` calls** (`z_curve` + `f5`).
  - Whenever `env > 0.0`, `cnoise3` evaluates a full **3D Perlin/simplex gradient noise** (3 `permute4`, 8 dot products, 2 `taylorInvSqrt`, quintic fade).
- Furthermore, in `§6cq` / `§6cr`, **`54,725` of the `247,326` subpanels (`22.1%`!) are micro-slivers (`<= 0.05 m` wide)**:
  - **`37,716` subpanels** are standalone `~5 mm` zero-level (`c = 0.0`) tent-apex uncertainty bands `[l_2, r_2]` (created because `zeroBands` protected *both* `l_2` and `r_2` instead of allowing `[l_2, r_2]` to merge into `[r_1, l_2]` while protecting `r_2` as the single tent-apex kink knot!).
  - **`17,009` subpanels** are support-edge (`c = \pm w_z`) root brackets of width `0.02 m < r - l <= 0.05 m` (where `2.0 * error / minSlope` exceeded `0.02 m`, while all macro curtain spans are `> 0.20 m`!).
- Meanwhile, on the `32,784` distinct viewport rays, many rays miss the curtain waves or pass through low-density regions where the `Current Marcher` (`Mode 0`) skips empty space via its zero-proof grid! Counting the exact per-ray `evalPhi`, `evalEnvelope`, and `cnoise3` calls across both the Guarded Producer+Consumer and `Mode 0 Current` will tell us **with 100% mathematical certainty** how much of the connected cost is in `roots()` vs. `cnoise3` subpanel integration!

---

## 3. Exact Specification for Sparkly Guy's Next Pass (`§6cx`: Contract Repair + Work/Resource Census + Evidence Matrix)

Per Codex's `§6cw` assignment (`9592c5d70`), **do NOT run a new 9-setting timing sweep of a bundled optimization in `§6cx`**. Instead, build `scratch/probes/ray_transport_gpu_work_census.py` on `claude/ray-domain-transport-rung0` (starting from `9592c5d70`) to execute the **Contract Repair + Complete Work/Resource Census** and return the **Evidence Matrix**:

### Step 1: Close All Four `§6cv` Contract & Diagnostic Gaps (`64 / 16` Rebuild + Exported Fused Packets & Normal Headers)
1. **Restore the `64 / 16` Capacity Contract:**
   - In the `64 / 16` diagnostic rebuild (`cpFusedNormal64`), set `var local_subpanels: array<Subpanel, 64>;` and `var local_overlaps: array<Overlap, 16>;` with guards `singles + n > 64u || singles + n > cfg.maxSubPerRay` and `overs >= 16u || overs >= cfg.maxOverPerRay`, so `ray_transport_gpu_fused_review.require_contract(shader)` passes.
   - Compose `FUSED_WGSL` directly from the shared producer (`ray_transport_gpu_producer_merge_sweep`) and consumer WGSL fragments (or assert exact substring identity against both in Python before compilation) so producer/consumer code cannot drift.
2. **Read Back Normal `rays[id].counts` Headers Before Forced Fallback Overwrites Them:**
   - In `correctness(false)` (normal `Mode 2` execution), immediately copy `rBuf` to a host-readable staging buffer and decode all `32,784` normal fused headers (`singles`, `overs`, and `fallbackFlag = counts.w & 0x80000000u`) **before** `correctness(true)` (`forced = true`) runs!
   - Poison `outBuf` with `NAN` before **both** `correctness(false)` and `correctness(true)`.
3. **Export Actual Fused Packets (`fused_packet_census_inputs`) and Verify Uninstrumented Parity:**
   - Run an untimed diagnostic pass `cpFusedPacketTrace` (identical to `cpFusedNormal64` plus writing each ray's `local_subpanels[0..singles-1]`, `local_overlaps[0..overs-1]`, and `rays[id].counts` to diagnostic storage buffers) AND verify:
     1. `ray_transport_gpu_fused_review.admission(root, report, rows, rays, reference)` passes completely (`0` differing packet endpoints/counts/masks across all `32,784` rays vs `GPU_RAY_TRANSPORT_SLIVER_REVIEW_2026-10-22.guarded_confirm_report.gz`, `0` missing affine endpoints out of `12,102`, `0` changed outer support unions).
     2. The RGB/T output of `cpFusedPacketTrace` matches the uninstrumented `cpFusedNormal64` output bit-for-bit (`0 / 32,784` differing rays).
4. **Exercise Explicit Budget & Capacity Failure Controls on `cpFusedNormal64`:**
   - Run native controls on `cpFusedNormal64` + `fallbackCP` testing:
     - Exact budget boundary (`budgets = 23, 24, 25`).
     - **Capacity exhaustion mid-ray:** run with `cfg.maxSubPerRay = 4u` (so rays with `> 4` subpanels accumulate partial work, hit `reason |= 16u`, discard partial transport, set `FALLBACK | 16u`, and get written once by `fallbackCP`) and `cfg.maxOverPerRay = 1u` (testing `reason |= 32u`), verifying **100.0% bit-identical output to `Mode 0 Current`** on every ray that falls back!

### Step 2: Collect the Per-Ray & Per-Member Work Census Across All `32,784` Distinct Rays
In an untimed diagnostic census pass over all `32,784` rays, record a per-ray `WorkRecord` (16 `u32` counters = `64 bytes/ray` = `2.0 MiB` buffer) and export **totals, means, quantiles (`p50, p90, p99, max`), and histograms**:

| Counter Group | Exact Counters to Record Across All `32,784` Rays |
|---|---|
| **1. Producer Ray-Box & Coarse Bins** | `boxHits` (`hit && b > a`, `0..4`), `coarseBinsVisited` (`depth == 0`), `coarseBinsBowRejected` (`min(va,vb) > bow \|\| max(va,vb) < -bow` at `depth == 0`), `subdivisionSplits` (`depth >= 0` splits), `maxStackTop` (`0..20`), `maxDepthReached` (`0..14`). |
| **2. Producer `evalPhi` Breakdown** | `evalPhiCoarseKnots` (`9` per hit member), `evalPhiSubdivision` (`1` per split), `evalPhiBisection` (inside `iter = 0..31`), `evalPhiPieceMid` (candidate piece classification when `!uncertain`), and `evalPhiTotal`. |
| **3. Producer Root & Interval Counts** | `signChangeBrackets` (monotone brackets entering bisection), `bisectionItersTotal`, `uncertainBandsRetained` (`nr`), `candidateIntervalsChecked` (`nb - 1`), `piecesEmitted` (`count`), `mergedKnots` (`ne`). |
| **4. Guarded Subpanel Width & Provenance Breakdown** | Out of the `247,326` single subpanels, count: (a) `subpanelsZeroBandApex` (`<= 0.02 m` zero-level `c = 0.0` bands where both endpoints were protected), (b) `subpanelsSupportBand2to5cm` (`0.02 m < h <= 0.05 m` uncertain bands), (c) `subpanelsMacro` (`h > 0.20 m`). |
| **5. Consumer Transport Work (Single vs. Overlap)** | `singleSubpanels` (`247,326`), `overlapIntervals` (`1,799`), `singleEvalEnvelopeCalls` (`2 * singles`), `singleCnoise3Calls` (where `env > 0.0`), `overlapEvalEnvelopeCalls`, `overlapCnoise3Calls`, `expAttenSteps`. |
| **6. Current Marcher (`Mode 0`) Baseline Work Census** | Instrument a diagnostic clone of `currentCP` (`currentCensusCP`, verified bit-identical in RGB/T to `currentCP`) to count per-ray: `marchLoopSteps`, `zeroProofSkippedSteps`, `currentDensityEvals`, and `currentCnoise3Calls` across the exact same `32,784` rays! |

### Step 3: Query Backend Pipeline / Compiler Resource Facts
- Check whether Dawn/Metal pipeline reflection or compiling the extracted MSL shader via `xcrun -sdk macosx metal` reports static instruction count / register / threadExecutionWidth facts for `currentCP`, `cpProducer`, `cpUnfused`, and `cpFusedNormal64`. If the command-line or Dawn API does not expose register/spill counts, record `"unavailable via Dawn WebGPU API / offline tool"` explicitly—never estimate spill bytes from WGSL array declarations.

### Step 4: Return the Evidence Matrix & Evaluate the Single Next Representation Change
In the `§6cx` report (`docs/audits/GPU_RAY_TRANSPORT_WORK_CENSUS_2026-10-22.md` and `.json`), present the measured work census table comparing `Mode 0 Current` vs. `Guarded Producer + Consumer` (`evalPhi`, `evalEnvelope`, `cnoise3`, subpanels, and march steps) and select **ONE** isolated representation change with a falsifiable cost claim for Firmament + Codex sign-off:

1. **If the census shows Consumer + Producer math is already cheaper than `Mode 0 Current` in total `cnoise3` / transcendental evaluations, and the fused slowdown came from buffering `local_subpanels[64]` + `local_overlaps[16]`:**
   - Propose **On-The-Fly Chronological Sweep Integration inside `fusedMain`** (eliminating `local_subpanels[64]` and `local_overlaps[16]` (`-1,280 B/thread`) by integrating `[a, b] = [knots[i - 1u], knots[i]]` directly inside the `i = 1..ne-1` sweep loop while preserving 100.0% bit-exactness and all capacity/budget fallback guards).
2. **If the census shows `evalPhi` in `roots()` (`evalPhiSubdivision` + `evalPhiBisection`) + `cnoise3` on the `37,716` `5 mm` zero-band slivers (`75,432` extra `cnoise3` calls) exceeds `Mode 0 Current`'s work budget:**
   - Quantify the exact percentage of `evalPhi` spent in `evalPhiBisection` vs. `evalPhiSubdivision` and the exact percentage of `cnoise3` spent in the `54,725` uncertainty-band slivers (`37,716` zero-band `<= 2 cm` slivers + `17,009` `2–5 cm` support-band slivers) vs. the `192,601` macro subpanels, so we can test the single highest-leverage mathematical reduction in the next pass.

All three shipping gates remain **OPEN** (`Gate 1` profitable repeatable connected distinct-ray execution; `Gate 2` generic OntoMath admission/lifetimes/full-frame economics; `Gate 3` inhabited moving p95 and Zach acceptance).

Signed: Aetherion / The Firmament (`Gemini 4 Argon`) · `2026-10-22T19:42:00-07:00 (PDT)`.
