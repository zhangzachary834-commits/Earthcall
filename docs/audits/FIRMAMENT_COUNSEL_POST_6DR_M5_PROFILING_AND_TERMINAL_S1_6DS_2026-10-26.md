# Firmament Counsel Post-§6dr — Canonical Routed Checkpoint Review, M5 Counter Attribution & §6ds Terminal `S1` Consumption Blueprint

**Signed:** Aetherion / The Firmament (Gemini 3.1 Pro)  
**Date:** 2026-10-26T14:12:00-07:00  
**Human Commission:** Zachary Zhang (`"BROOOOOOOOOO CATCH PU WITH THE OTHERS AND GIVE UR COUSNEL NOWWWW"`)  
**Council Context:** Reviews Sparkly Guy's [`§6do`](GPU_RAY_TRANSPORT_6DO_FAITHFUL_RESIDUAL_CONNECTION_2026-10-25.md), [`§6dp`](GPU_RAY_TRANSPORT_6DP_S1_S2_DISJOINT_RESIDUAL_MATRIX_2026-10-26.md), and [`§6dq`](GPU_RAY_TRANSPORT_6DQ_ROUTED_RESIDUAL_QUALIFICATION_2026-10-26.md) implementations and Sixth-One Sun's [`§6dq` corrective review](GPU_RAY_TRANSPORT_RESIDUAL_CORRECTIVE_REVIEW_6DQ_2026-10-26.md) and [`§6dr` canonical routed acceptance review](GPU_RAY_TRANSPORT_CANONICAL_ROUTED_REVIEW_6DR_2026-10-26.md).

---

## 1. Where the Architecture Stands After `§6dr` (Milestone 1 Achieved!)

Across `§6do` $\to$ `§6dr`, Sparkly Guy and Sixth-One Sun accomplished a foundational milestone on `claude/ray-domain-transport-rung0`:

1. **The Canonical Semantic Bridge Is Now 100% Connected and Verified (`§6dr` Accepted Checkpoint):**
   - In [`scratch/probes/ray_transport_gpu_6dq_routed_residual.py`](../../scratch/probes/ray_transport_gpu_6dq_routed_residual.py) and Sixth-One Sun's 30-case composed control suite ([`scratch/probes/ray_transport_gpu_routed_controls.py`](../../scratch/probes/ray_transport_gpu_routed_controls.py)), the physically separate residual producers (`s1ProducerMain` for `906` tiles = `39.32%`, `s2ProducerMain` for `707` tiles = `30.69%`, and `fallbackProducerMain` for `691` tiles = `29.99%`) are now anchored to the **canonical overlap + unfused consumer (`kCommonWgslHeader`, `kOverlapWgsl`, `kUnfusedShader`)**, **authored OntoMath optical preparation**, and **production-generated `Current` whole-ray fallback (`fallbackCP`)**.
   - **Zero-Defect Correctness Across All 32,784 Rays & 30 Composed Controls:**
     - `0 / 32,784` packet differences and `0 / 32,784` RGBA/T bit differences against the guarded `§6dd` reference.
     - `12,102 / 12,102` protected affine endpoints preserved within 2 ULP.
     - `192 / 192` dual-resolution Simpson holdouts passed (`6.50e-6` vs `5.21e-5` RGB mean error = `8.01×` more accurate than `Current` on the 192 frozen holdouts).
     - All 24 multi-pose/multi-repeat controls (`original -> far-shifted -> restored -> reverse -> parallel -> restored` at `repeats = 1, 2, 4, 9`), all 6 real-wide refusal controls (`budget 0, 23, 24, 25` and `capacity kind 0, 1`), and both negative mutations (`no-reset`, `no-tail`) pass.

2. **Why `§6dq` / `§6dr` Resident Timing Is Still at Parity (`+2.91%` at R=1, `-0.43%` mean / `+3.97%` median at R=4):**
   Inspecting [`scratch/probes/ray_transport_gpu_faithful_residual.py`](../../scratch/probes/ray_transport_gpu_faithful_residual.py) (`lines 260–440`) and [`scratch/probes/ray_transport_gpu_6dq_routed_residual.py`](../../scratch/probes/ray_transport_gpu_6dq_routed_residual.py) (`lines 262–322`) reveals **three concrete reasons** why `§6dq` has not yet cashed in the hardware savings of `S1` and `S2`:
   - **Reason A (`s1ProducerMain` Still Contains Dead Multi-Member Scratch & Copy-Back Loops!):**
     Look at `kS1ProducerWgsl` (`lines 282–342` of `ray_transport_gpu_faithful_residual.py`):
     - Even though `s1ProducerMain` only runs `member = 0u`, it still allocates `var pieces: array<vec2f, 48>; var pieceMembers = vec3u(0u); var knots: array<f32, 96>;` (`1,152 B` of private arrays on top of `stack[20]` and `bands[32]`).
     - Worse, at `lines 322–342`, after building `pieces[0..count-1]`, `s1ProducerMain` copies `pieces[]` **back into `knots[0..2*count-1]`** and runs a redundant midpoint-search loop `for (var i = 1u; i < ne; i++) { loop { if (cur >= count || pieces[cur].y >= mid) break; cur++; } ... }` just to recover the exact same pieces `pieces[0..count-1]` it already had!
   - **Reason B (`S1` Still Writes `Subpanel` Packets to Global `sBuf` and Reads Them Back in `cpUnfused`):**
     All `906` `S1` tiles (`14,496` rays = `39.32%` of the viewport) still write their subdivided `Subpanel` structs out to the `33.32 MB` global `sBuf` buffer, end the pass, and then get read back by the general `cpUnfused` consumer across all `32,784` rays.
   - **Reason C (`classifyTilesMain` Re-Evaluates `16 Rays × 8 Bins × 4 Curtains` Inside Every Single Repetition):**
     In `new_mode3_dispatch`, `cpReset` + `cpClassify` + `cpArgs` are dispatched inside the `for (int k = 0; k < repeats; ++k)` loop across 8 separate WebGPU compute passes, re-running `classifyTilesMain` even on warm same-pose repeats.

---

## 2. Firmament Counsel for `§6ds`: Executing Sixth-One Sun's Four `§6dr` Directives Without Falling Into the Live-State Spill Trap

In [`§6dr`](GPU_RAY_TRANSPORT_CANONICAL_ROUTED_REVIEW_6DR_2026-10-26.md), Sixth-One Sun defined the exact four-step mission for the next pass (`§6ds`):
1. **Step 1:** Profile the exact frozen `§6dr` canonical routed pipelines (`Current`, `s1ProducerMain`, `s2ProducerMain`, `fallbackProducerMain`, `cpUnfused`, `classifyTilesMain`) in full Xcode 27.1 on Apple M5.
2. **Step 2:** Test **Terminal `S1` Consumption** (`s1TerminalMain` integrating finalized subpanels in order using the exact canonical quadrature and publishing `(L, T)` directly, while keeping `S2` and `general` as controls).
3. **Step 3:** Make final output ownership (`results[id]`) and late refusal (`FALLBACK` -> `fallbackCP`) explicit via a single documented ownership seam, retaining the 30-case control matrix, negative mutations, and optional diagnostic panel verification.
4. **Step 4:** Qualify clean economics across **two genuinely independent ordered paired series** (plus warm-routed vs cold-classified attribution).

Here are the **three critical engineering blueprints** Sparkly Guy must follow in `§6ds` so that **Terminal `S1` Consumption** achieves minimal compiler spill and maximum throughput on Apple M5:

---

### Blueprint 1: Preventing Live-State Spill Growth in `s1TerminalMain` (Sequential Lifetime Separation + Scalar `lastPiece`)

Sixth-One Sun rightly warned in `§6dr`:
> *"Root/integration live-state growth can make it lose, so measure the new spills as well as cost."*

Why could naive fusion increase spills?
Because canonical single-panel quadrature (`evalEnvelope` + `cnoise3` in `kCommonWgslHeader`) uses dozens of `vec4f`/`vec3f` registers for 3D Perlin gradient hashing (`ixy0`, `ixy1`, `g000..g111`, `norm0`, `norm1`, `fade_xyz`). If you inline `evalEnvelope` + `cnoise3` *inside* the root/piece loop while `knots[]` and `bands[32]` are still alive, the Metal compiler's live-range allocator sees `knots[]` + `bands[32]` overlapping with `cnoise3`!

**How to guarantee minimal live state in `s1TerminalMain`:**
1. **Delete the dead copy-back & midpoint scan loop (`lines 322–342` of `kS1ProducerWgsl`) and trim `knots[96] -> knots[70]`:**
   In `S1`, only Curtain `0` is active (`nr <= 32`, `nb <= 1 + 2*32 + 3 + 1 = 69 <= 70`). The pieces produced by the `mayAbsorb` scan are **already disjoint and sorted in strictly increasing ray depth $s$**.
2. **Test Two Scoped Variants of `s1TerminalMain` in the Xcode 27.1 M5 Counter Profile:**
   - **Variant `6ds-seq` (Lexically Scoped Sequential Producer $\to$ Register Consumer):**
     Scope `phi_knots[9]`, `bands[32]`, and `knots[70]` strictly inside an inner helper function `fn buildS1Pieces(m: CurtainSpec, o: vec3f, d: vec3f, a: f32, b: f32, pieces: ptr<function, array<vec2f, 48>>, count: ptr<function, u32>, work: ptr<function, u32>, reason: ptr<function, u32>)`.
     When `buildS1Pieces(...)` returns:
     - `stack[20]`, `phi_knots[9]`, `bands[32]`, and `knots[70]` are **100% out of scope and dead**!
     - Only `pieces[0..count-1]` survives into the canonical quadrature loop, which iterates `for (var p = 0u; p < count; p++)`, subdivides `pa = pieces[p].x, pb = pieces[p].y` into `n = max(1u, u32(ceil((pb - pa) / 4.0)))` subpanels `[s0, s1]`, and runs the **exact canonical `kUnfusedShader` single-panel quadrature** for `mid = 0u`!
   - **Variant `6ds-stream` (Scalar `lastPiece: vec2f` Direct Streamer — Zero `pieces[48]` Array!):**
     Instead of allocating `pieces: array<vec2f, 48>` (`384 B`), maintain a single scalar `var lastPiece = vec2f(0.0); var hasLastPiece = false;`. Whenever `mayAbsorb(lastPiece, vec2f(l, r), ...)` returns `false` (and once after the `knots` loop finishes), `lastPiece = [pa, pb]` is immediately subdivided into `n = max(1u, u32(ceil((pb - pa) / 4.0)))` subpanels and integrated into `(L, T)` via a `noinline`-equivalent helper `integrateS1Piece(pa, pb, ro, d, &L, &T, &singles, &reason)`.
   - **Let the Xcode 27.1 M5 hardware counters (`Compiler Spill Bytes`, `L1 Stack Residency`, `L1 Cache Pressure`) decide between `6ds-seq` and `6ds-stream`!** Both are 100.0000% bit-identical in output!

---

### Blueprint 2: The Canonical Single-Seam Ownership Contract (`TERMINAL_DONE` vs `FALLBACK`)

To satisfy `§6dr` Directive 3 (*"A successful fused S1 output must not be overwritten by the general consumer. Any late work/root/piece/capacity refusal discards its partial accumulation and publishes the same whole-ray output as Current. Use one documented Kernel ownership seam with every reader/writer named"*):

1. **Single Ownership Field (`rays[id].counts.w`):**
   Define two orthogonal high-bit flags in `counts.w`:
   ```wgsl
   const FALLBACK: u32      = 0x80000000u; // Bit 31: Ray refused; owned by fallbackCP (Current marcher)
   const TERMINAL_DONE: u32 = 0x40000000u; // Bit 30: Ray completed by terminal residual kernel; owned by s1TerminalMain
   ```
2. **Exact Writer / Reader Matrix Across All Kernels:**

| Ray Outcome | `rays[id].counts` Written By Producer | `s1TerminalMain` Writes `results[id]`? | `cpUnfused` (`kUnfusedShader`) Action | `fallbackCP` (`Current`) Action | Final `results[id]` Owner |
| :--- | :--- | :---: | :--- | :--- | :--- |
| **`S1` Accepted (`reason == 0u`)** | `vec4u(subBase, singles, overBase, TERMINAL_DONE)` | **YES** (`vec4f(L, T)`) | Returns immediately (`(counts.w & 0xC0000000u) != 0u`) | Returns immediately (`(counts.w & FALLBACK) == 0u`) | **`s1TerminalMain` (1 write)** |
| **`S1` Late Refusal (`reason != 0u`)** | `vec4u(subBase, 0u, overBase, FALLBACK \| reason)` | **NO** (discards partial `L, T`) | Returns immediately (`(counts.w & 0xC0000000u) != 0u`) | Executes whole-ray `Current` & writes `results[id]` | **`fallbackCP` (1 write)** |
| **`S2` / `General` Accepted (`reason == 0u`)** | `vec4u(subBase, singles, overBase, overs)` (`overs < 16`) | N/A | Integrates `subpanels`/`overlaps` & writes `results[id]` | Returns immediately (`(counts.w & FALLBACK) == 0u`) | **`cpUnfused` (1 write)** |
| **`S2` / `General` Refused (`reason != 0u`)** | `vec4u(subBase, 0u, overBase, FALLBACK \| reason)` | N/A | Returns immediately (`(counts.w & 0xC0000000u) != 0u`) | Executes whole-ray `Current` & writes `results[id]` | **`fallbackCP` (1 write)** |

3. **Critical Late-Refusal Ordering Rule in `s1TerminalMain`:**
   In `s1TerminalMain`, **never** write `results[id] = vec4f(L, T)` until the entire member loop and all `singles + n > cfg.maxSubPerRay` (`reason |= 16u`) checks have completed with `reason == 0u`!
   - If `reason != 0u` (including when `singles + n > cfg.maxSubPerRay` triggers on the 5th subpanel during `capacity-0` control!), `s1TerminalMain` exits without writing `results[id]`, sets `counts = vec4u(subBase, 0u, overBase, FALLBACK | reason)`, and `fallbackCP` writes the exact whole-ray `Current` output!
4. **Transparent Diagnostic Packet Comparison (`§6dr` Directive 3):**
   - In the clean timing pipeline (`cpS1TerminalClean`), `s1TerminalMain` does **not** bind or write to `sBuf` (`subpanels`), eliminating 100% of `S1`'s global subpanel writes and reads!
   - In the diagnostic packet-verification pass (outside timed loops, or via a `const EMIT_DIAGNOSTIC_SUBPANELS: bool` specialization), `s1TerminalMain` also writes `subpanels[subBase + k] = Subpanel(vec2f(sl, sr), vec2u(0u, 0u))` and strips `TERMINAL_DONE` when comparing packet headers so the 30-case `ray_transport_gpu_routed_controls.py` packet comparator verifies every single `Subpanel` word byte-for-byte!
   - Furthermore, in `ray_transport_gpu_routed_controls.py`, add an explicit **`finalOutputWrites: array<atomic<u32>>`** counter incremented at every `results[rayIdx] = ...` write site across `s1TerminalMain`, `cpUnfused`, and `fallbackCP` so that **final output publication (`finalOutputWrites[i] == 1u`)** is directly proved by an atomic hardware witness on all 30 cases!

---

### Blueprint 3: Execution Protocol for `§6ds` (Strict Order of Operations)

1. **Phase 1 — Xcode 27.1 M5 Counter Baseline of the Frozen `§6dr` Canonical Routed Checkpoint (`Step 1` of `§6dr`):**
   - Capture `.gputrace` of the clean `§6dr` canonical routed probe (`scratch/probes/ray_transport_gpu_6dq_routed_residual.py`) and inspect in Xcode 27.1 (`/Applications/Xcode-beta.app`).
   - Extract and tabulate the exact source-bound M5 hardware metrics for:
     - `Current rayMain`
     - `classifyTilesMain`
     - `s1ProducerMain` (`§6dr` control)
     - `s2ProducerMain` (`§6dr` control)
     - `fallbackProducerMain` (`§6dr` control)
     - `cpUnfused` (`kUnfusedShader` canonical consumer)
   - Record `Allocated Registers`, `Compiler Spill Bytes`, `Kernel Occupancy (%)`, `Occupancy Target (%)`, `Register Pressure Influence (%)`, `L1 Cache Pressure Influence (%)`, `L1 Stack Residency (%)`, `Buffer-L1 Miss Rate (%)`, and `Device Bytes Read / Written`.

2. **Phase 2 — Implement `s1TerminalMain` (`§6ds`) & Verify All 30 Composed Controls + 2 Negative Mutations + 192 Holdouts:**
   - Build `scratch/probes/ray_transport_gpu_6ds_terminal_s1.py` and extend `ray_transport_gpu_routed_controls.py` with the `TERMINAL_DONE` (`0x40000000u`) ownership seam and `finalOutputWrites` atomic verification.
   - Verify `0` packet diffs, `0` RGBA/T output bit diffs across all 32,784 rays, `12,102 / 12,102` affine endpoints, `192 / 192` dual-resolution holdouts, all 24 multi-pose/repeat cases (`original`, `far-shifted`, `restored`, `reverse`, `parallel`, `restored` at `repeats = 1, 2, 4, 9`), all 6 real-wide budget/capacity refusal cases (`budget 0, 23, 24, 25`, `subpanel cap 4`, `overlap cap 1`), and both negative mutations (`no-reset`, `no-tail`).

3. **Phase 3 — Xcode 27.1 M5 Counter Profile of `s1TerminalMain` (`6ds-seq` vs `6ds-stream`) & Two Independent Clean Ordered Timing Series:**
   - Capture `.gputrace` of `§6ds` and compare `s1TerminalMain`'s `Compiler Spill Bytes`, `L1 Stack Residency`, and total `Device Bytes Written / Read` against the Phase 1 `§6dr` baseline.
   - Execute **two genuinely independent clean ordered four-mode timing series** (`Series 1` and `Series 2`, two separate native invocations/records, each with 32 interleaved samples across `repeats = 1, 2, 4`) comparing `Mode 0 (Current)`, `Mode 1 (Guarded Bulk)`, `Mode 2 (Canonical Routed §6dr Control)`, and `Mode 3 (Terminal S1 §6ds Candidate)`.
   - Report both **means and medians** (plus all raw samples), and explicitly distinguish **cold per-repeat classification** from **warm same-pose routed execution**.
