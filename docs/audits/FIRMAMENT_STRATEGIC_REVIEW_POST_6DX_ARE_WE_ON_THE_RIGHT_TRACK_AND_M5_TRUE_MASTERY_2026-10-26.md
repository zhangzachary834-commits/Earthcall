# Firmament Strategic Review Post-`§6dx` — Are We on the Right Track? Why Array-Trimming Hit a Structural Wall & Five Threads of True Apple M5 Mastery

**Signed:** Aetherion / The Firmament (Gemini 3.1 Pro)  
**Date:** 2026-10-26T21:56:00-07:00  
**Human Commission:** Zachary Zhang (`"BROOOOOOOOO THEY DID MORE PASSES / DO REVIEW WHETHER WE ARE STILL ON THE RIGHT TRACK OR IF THERE ARE BETTER WAYS TO TAKE ADVANTAGE OF M5 AND FOLLOW THE ARCHITECTURAL THREADS OF TRUE MASTERY WE HAVE BEEN WORKING ON"`)  
**Reviewed Passes (`§6du` $\to$ `§6dx`):**
- Sparkly Guy [`§6du` Corrective Qualification (`1f29da123`)](GPU_RAY_TRANSPORT_6DU_CORRECTIVE_QUALIFICATION_2026-10-26.md)
- Sixth-One Sun [`§6dv` Corrective Review & Streamed Endpoint Witness (`c5551cae5`)](GPU_RAY_TRANSPORT_6DV_CORRECTIVE_REVIEW_2026-10-26.md)
- Sixth-One Sun [`§6dw` Two-Mode Admission & Scalar-Switch Stack Isolation (`000884cd3`)](GPU_RAY_TRANSPORT_6DW_STACK_ISOLATION_2026-10-26.md)
- Sparkly Guy [`§6dx` Pending-Stack Bound Proof ($\max(\text{top}) = 15$) & `-240 B` Spill (`1a105e57c`)](GPU_RAY_TRANSPORT_6DX_STACK_BOUND_2026-10-26.md) and Sixth-One Sun's in-progress `§6dx` source-certificate tightening ([`scratch/probes/ray_transport_gpu_6dx_stack_bound.py`](../../scratch/probes/ray_transport_gpu_6dx_stack_bound.py))

---

## 1. Honest Strategic Verdict: Are We Still on the Right Track?

**Short answer:**
- **Our verification & measurement harness (`§6dr` $\to$ `§6dx`) is 100% on the right track**—we now have an airtight 60-case dual-mode correctness suite, `1,073,023` verified terminal subpanel endpoints, 7 negative mutations, source-bound proof certificates, and exact Apple M5 `.gputrace` compiler plist extraction, with clean unthrottled timings within **`11–17 µs` (`+0.45%–0.65%`)** of `Current` while delivering **`8.01×` higher quadrature accuracy** on the 192 holdouts.
- **HOWEVER, our optimization strategy inside `s1ProducerMain` (`§6dk` $\to$ `§6dx`: trimming `knots[96] -> 70`, `stack[20] -> scalar switches`, `stack[20] -> stack[15]`) has drifted into micro-optimizing a per-pixel GPU theorem prover, which hits a hard structural wall on Apple M5 and contradicts our own core doctrine:**
  $$\boxed{\text{Meaning} \neq \text{Proof Representation} \neq \text{Materialized View} \neq \text{Executable}}$$

Look at the M5 compiler & timing scoreboard across `§6du`–`§6dx` to see why:

| Kernel / Representation | Physical Spill Bytes | Temp Regs | Static Instructions | R=1 Median (`§6dx`) | R=2 Median (`§6dx`) | R=4 Median (`§6dx`) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **`Current rayMain` (Mode 0 Marcher)** | **`0 B`** | `86` | **`2,530`** | **`2.531 ms`** | **`3.806 ms`** | **`6.340 ms`** |
| **`Guarded Bulk` (Mode 1)** | `2,000 B` | `206` | `~5,400` | `2.535 ms` | `3.823 ms` | `6.998 ms` |
| **`§6dw Scalar-Switch S1` (Stopped)** | `1,456 B` | `126` | `14,448 / 15,563` | `3.031 ms` | `5.306 ms` | `8.148 ms` |
| **`§6dx Routed Unfused` (Mode 2, `stack[15]`)** | **`1,760 B`** | `79` | **`5,093`** (`+ 4,280` consumer) | `2.542 ms` (`+0.45%`) | **`3.804 ms` (`-0.06%`)** | `6.394 ms` (`+0.86%`) |
| **`§6dx Terminal S1` (Mode 3, `stack[15]`)** | **`1,760 B`** | `83` | **`6,375`** | `2.547 ms` (`+0.65%`) | `3.876 ms` (`+1.86%`) | `7.050 ms` (`+11.20%`) |

### Two Crucial Lessons from `§6du`–`§6dx`:

1. **Lesson 1 — Why `Mode 2` (`Routed Unfused`) Beats `Mode 3` (`Terminal S1`) at Repeats 2 and 4 (`3.804 ms` vs `3.876 ms`, and `6.394 ms` vs `7.050 ms`):**
   Look at the table above!
   - On Apple M5, unified memory bandwidth is massive (`>150 GB/s`), so reading `1.57 MB` of `Subpanel` structs in `cpUnfused` costs almost nothing—**and `cpUnfused` runs with only `32 B` of spill**!
   - Conversely, fusing `cnoise3` + `evalEnvelope` into `s1ProducerMain` (`Mode 3`) keeps the thread inside a **`1,760 B`-spilling kernel for `6,375` instructions** (when L1 cache is already thrashing with `1,760 B` stack frames!), whereas keeping `s1ProducerMain` unfused (`Mode 2`) finishes the `1,760 B`-spilling producer earlier and runs quadrature inside the `32 B`-spilling `cpUnfused` kernel!
2. **Lesson 2 — Why Shrinking `stack[20] -> stack[15]` (or Bounding `bands[32]` / `knots[96]`) Cannot Break Through the Wall:**
   - Look at what `s1ProducerMain` is still doing on **every single pixel ray**:
     Even when a $4\times 4$ tile is certified `S1` (only Curtain 0 active), every one of the 16 rays in that tile still executes a **5,093-instruction interval root-finder (`roots()` 3 times + `stack[15]` + `bands[32]` + `knots[96]` quadratic insertion sort + `pieces[48]`)**!
   - As long as `stack[]`, `bands[]`, `knots[]` (with variable-index insertion sort `knots[j] = knots[j-1]`), and `pieces[]` exist as runtime-indexed `function`-space arrays inside `s1ProducerMain`, the Apple M5 compiler **must** allocate a **`~1.5–1.76 KiB` stack frame in L1** for every thread!
   - In short: **we put the Proof Engine inside the per-pixel Executable**, instead of using the Proof Region to emit a **Zero-Array Residual Executable**!

*(Note on `§6dx` Section 6.2 byte accounting: as Sixth-One Sun also caught in his `§6dx` review, `bands` is `array<vec2f, 32>` = `256 B`, `knots` is `array<f32, 96>` = `384 B`, and `pieces` is `array<vec2f, 48>` = `384 B`—not `vec4f` arrays of `512 B / 1,536 B / 768 B`.)*

---

## 2. How to Actually Take Advantage of Apple M5 & Follow Our Architectural Threads of True Mastery

If trimming array sizes inside the 5,093-instruction per-ray `roots()` solver has reached diminishing returns (`-240 B` spill $\to$ still `1,760 B` spill), what **actually** unlocks a decisive win on Apple M5 while honoring our **Semantic State-Space / Magic-Set / Intrinsic Foliation** architecture?

Here are the **Five High-Leverage Architectural Levers** we have not yet used on M5:

---

### Lever 1: Split `S1` (and `S2`) by **Solver Regime** — `S1_DEPTH0_MONOTONE` (`0 B` Spill, Zero Arrays!) vs. `S1_GENERAL` (`stack[15]` Fallback)

Ask yourself: **Why does `s1ProducerMain` need `stack[15]`, `bands[32]`, `knots[96]` insertion sort, and `pieces[48]` at all?**
Only for rays where a coarse bin is **not** depth-0 monotone (`coarseMinSlope <= 0.0`) or has overlapping uncertain bands!
- Remember our `§6dd` census: on the vast majority of active curtain crossings, **every active coarse bin `bin in 0..7` already satisfies `coarseMinSlope > 0.0` at depth 0**!
- When all active bins of Curtain $m$ on a ray satisfy `coarseMinSlope > 0.0` at depth 0 (which is checked right after computing `phi_knots[0..8]`!):
  1. **`stack[15]` is NEVER used (`0` pushes)!**
  2. Inside each monotone bin `bin = 0..7`, $\Phi_m(s)$ is strictly increasing (`coarseSlope > 0`) or strictly decreasing (`coarseSlope < 0`), so the crossings of the three levels $\{-w_z, 0, +w_z\}$ occur in **known chronological order**:
     - Rising bin (`coarseSlope > 0`): $-w_z \prec 0 \prec +w_z$
     - Falling bin (`coarseSlope < 0`): $+w_z \prec 0 \prec -w_z$
  3. And the 3 affine kink planes (`kx = -o.x/d.x`, `ky0 = -o.y/d.y`, `ky1 = (m.limits.z - o.y)/d.y`) can be sorted into 3 scalar registers `(k0, k1, k2)` with a 3-element comparator network (`3` `min`/`max` ops in scalar registers!) and interleaved chronologically as `bin = 0..7` advances!
- **Why Separating `s1MonotoneMain` from `s1GeneralMain` Changes Everything on M5:**
  - In Metal on Apple M5, **stack frame allocation (`spilled_bytes`) is per-kernel**!
  - As long as the rare `stack[15]` + `knots[96]` insertion-sort fallback code lives inside the *same* `s1ProducerMain` kernel, **100% of `S1` threads pay the `1,760 B` stack frame allocation** even when 90%+ of them never push to `stack[15]`!
  - If instead `s1MonotoneMain` handles the depth-0 monotone rays using **pure chronological scalar registers (`0` arrays: no `stack`, no `bands`, no `knots`, no `pieces`)** and sets a handoff bit (or routes non-monotone tiles/rays) to `s1GeneralMain` for the remaining complex rays, then **`s1MonotoneMain` compiles with `0 bytes` of stack spill** (matching `currentRayMain`'s `0 B` spill)!

---

### Lever 2: Use Apple M5's `32 KB` On-Chip `var<workgroup>` Shared Memory (Currently at `0 Bytes` Across All 7 Kernels!)

Look at the M5 pipeline reflection table across every audit from `§6di` to `§6dx`:
- **Hardware available per workgroup on Apple M5:** `32,768 bytes` (`32 KB`) of dedicated on-chip Threadgroup Shared Memory.
- **Amount currently used by our 7 kernels:** **`0 bytes` (`staticThreadgroupMemoryLength = 0 B`)**!

Meanwhile, our producer kernels are spilling `1,760–2,000 bytes` per thread into **L1-cached device stack memory** (`85%` L1 stack residency, `100%` L1 cache pressure in `§6dj`/`§6dl`)!
Look at two ways `var<workgroup>` shared memory on M5 immediately attacks the bottleneck:
1. **SIMD-Cooperative Tile Classification in `classifyTilesMain` (`16×` Less Serial Loop Work per Thread):**
   - Right now, in `classifyTilesMain` (`lines 155–270` of `ray_transport_gpu_faithful_residual.py`), **1 single thread** processes an entire 16-ray tile sequentially: it allocates private arrays `rA[16]`, `rB[16]`, `rHit[16]`, `wKnots[9]`, loops `for (i = 0..15)` to clip 16 rays, loops `for (k = 0..8)` to evaluate 9 wave knots, and then loops `for (i = 0..15) for (bin = 0..7)` (**128 inner iterations per curtain = 512 iterations per thread!**).
   - If instead **16 threads (half a 32-wide M5 SIMD group)** cooperatively classify 1 tile (`rayInTile = lid & 15u`, `localTile = lid >> 4u`, `576` workgroups of `64` threads = `2,304` tiles):
     - Each thread `rayInTile` clips **only its own 1 ray** (`rA`, `rB`, `hit` are **scalar registers**, deleting `rA[16]`, `rB[16]`, `rHit[16]`!).
     - Across the 16 rays in the tile, `uxMin` and `uxMax` are reduced in 4 instructions via Metal's hardware **`quad_min`/`simd_min` and `simd_max`** (or a 16-slot `var<workgroup>` reduction)!
     - Threads `rayInTile in 0..8` evaluate the 9 wave knots `wKnots[rayInTile]` **in parallel** (1 `evalWave` call per thread instead of 9 sequential calls!) and store them in `var<workgroup> wKnots: array<array<f32, 9>, 4>`.
     - Each thread `rayInTile` checks **only its own 1 ray's 8 bins** against `wKnots` (`8` iterations instead of `128`!).
     - That cuts the serial work per thread in `classifyTilesMain` by **`12×–16×`** and deletes all per-ray arrays from `classifyTilesMain`!
2. **Passing Tile-Shared Intrinsic Wave Knots `wKnots[0..8]` to the Producer (or Hosting Hot Scratch in `var<workgroup>`):**
   - Because `classifyTilesMain` *already* computes `uxMin`, `uxMax`, and the 9 intrinsic wave evaluations `wKnots[0..8]` for every active curtain on every tile, why is `s1ProducerMain` throwing those away and re-evaluating `evalPhi` (`sin`/`cos`) from scratch on every ray?!

---

### Lever 3: Intrinsic $u_x$-Wave Preimage Partitioning (The Extradimensional Set-Space / Magic-Set Breakthrough)

Remember why we wrote [`FIRMAMENT_EXTRADIMENSIONAL_SEMANTIC_POLYHEDRA_AND_QUOTIENT_SPACE_2026-10-24.md`](FIRMAMENT_EXTRADIMENSIONAL_SEMANTIC_POLYHEDRA_AND_QUOTIENT_SPACE_2026-10-24.md) and [`SEMANTIC_STATE_SPACE_PROOF_REGIONS_AND_MATERIALIZED_VIEWS_PROPOSAL_2026-10-21.md`](../../../Earthcall/docs/architecture/interrelations/SEMANTIC_STATE_SPACE_PROOF_REGIONS_AND_MATERIALIZED_VIEWS_PROPOSAL_2026-10-21.md) (`Addendum A`):
- We asked:
  > *"If you were free to choose the mathematical coordinates from scratch, what coordinate system would make Northern Veil's execution structure simplest?"*
- And we proved the answer: **the Intrinsic Wave Coordinate $u_x = x - \text{origin}_{m,x}$**!
- Look at why `roots()` in `§6dd`–`§6dx` has to work so hard:
  - `roots()` slices the ray segment $[a, b]$ into **8 arbitrary uniform bins in ray distance $s$**:
    $$s_k = a + \frac{k}{8}(b - a)$$
  - Because $a$ and $b$ depend on where each individual ray hits the bounding box, **every ray has different $s_k$ points**, every ray evaluates `sin`/`cos` at different $x(s_k)$ points, and whenever a uniform $s$-bin happens to straddle a wave crest/trough ($w'_m(u_x) = 0$), `coarseMinSlope > 0.0` fails and the ray falls into the `stack[15]` recursive bisection loop!
- **Now look at what happens when you partition in $u_x$-space first (AOT / per-frame in OntoMath) and pull back to the ray via $s(u_x) = (u_x - o_x) / d_x$:**
  - The wave $w_m(u_x) = \sum_{j=1}^3 A_j \sin(\omega_j u_x + \varphi_j)$ is a **1D function of $u_x \in [-W_x, +W_x]$ alone**. It does NOT depend on $y$, $z$, camera origin $\mathbf{o}$, or ray direction $\mathbf{d}$!
  - In OntoMath (on the CPU, once when the curtain is authored or when time $t$ changes—**zero per-frame or per-ray cost!**), we partition $[-W_x, +W_x]$ at the wave's extrema / inflection cells into a 1D table of certified monotone cells $[u_{x,i}, u_{x,i+1}]$ storing $(w_m(u_{x,i}), w_m(u_{x,i+1}), M_{2,i})$.
  - On the GPU, for any ray $p(s) = \mathbf{o} + s\,\mathbf{d}$ (with $d_x \neq 0$), the Magic-Set pullback of cell $[u_{x,i}, u_{x,i+1}]$ onto the ray is **purely affine**:
    $$s_i = \frac{u_{x,i} - o_x}{d_x}, \qquad \Phi_m(s_i) = o_z + s_i\,d_z + w_m(u_{x,i})!$$
  - **STOP AND LOOK AT THAT FORMULA:**
    $$\Phi_m(s_i) = o_z + s_i\,d_z + \text{table\_w}[i]$$
    - **How many `sin` or `cos` calls does the ray need at the cell endpoints $s_i$? ZERO (`0`)—it's 1 FMA plus a uniform/constant table read!**
    - **And because the $u_x$-cells are already partitioned at the wave extrema where $w'_m(u_x) = 0$, $\Phi_m(s)$ is already monotone on the pulled-back cells—eliminating the depth-14 `stack[15]` tree discovery!**
    *(Of course, per Sixth-One Sun's strict faithfulness rule, any new $u_x$-cell table must include a rigorous `f32` error enclosure or be used as an exact coarse-knot/monotone-bracket generator that feeds the exact `§6dd` endpoint predicate so `100%` accuracy / holdout gates are preserved!)*

---

### Lever 4: Consolidate Compute Passes in `Mode 2` (Eliminating Redundant Command-Buffer Pass Barriers)

Look at why `Mode 2` (`Routed Unfused`) is already tying `Current` at Repeat 2 (`3.804 ms` vs `3.806 ms`) in `§6dx` despite launching **8 separate compute passes** per repeat (`cpReset`, `cpClassify`, `cpArgs`, `cpS1`, `cpS2`, `cpFallback`, `cpUnfused`, `fallbackCP`):
- In `Mode 2` (`ray_transport_gpu_6du_true_terminal_s1.py`), every single stage calls `wgpuCommandEncoderBeginComputePass` and `wgpuComputePassEncoderEnd` separately.
- Yet `cpS1`, `cpS2`, and `cpFallback` all read the same read-only `argsBuf`/`cntBuf` and write to **strictly disjoint ray indices** in `rBuf`, `sBuf`, and `oBuf`!
- Encoding `cpS1`, `cpS2`, and `cpFallback` inside a **single `WGPUComputePassEncoder`** (3 indirect dispatches in 1 pass instead of 3 separate passes) and encoding `cpUnfused` + `fallbackCP` cleanly removes unnecessary Metal pass barriers while keeping the exact same shaders and bit-identical outputs!

---

## 3. Recommended Action Plan (`§6dy` and Beyond)

1. **Wait for Sixth-One Sun's `§6dx` Review to Complete:**
   Sixth-One Sun is currently finalizing his CPU-only review of `§6dx` (tightening the token-hash source-proof certificate in `scratch/probes/ray_transport_gpu_6dx_stack_bound.py` and correcting the Section 6.2 `vec4f` byte-count prose). Do not touch the working tree until his review commit lands.
2. **Stop Further Capacity-Trimming of `stack[]` / `bands[]` / `knots[]` Inside the Monolithic 3-Pass `roots()` Loop:**
   As `§6dw` (`1,456 B` spill, `15,563` insts) and `§6dx` (`1,760 B` spill, `6,375` insts) proved, keeping the general 3-pass `roots()` + insertion-sort + `pieces[]` pipeline inside `s1ProducerMain` caps `s1ProducerMain` at `~1,760 B` spill and `~2.54 ms` (parity with `Current`, not a breakthrough win).
3. **Pivot the Next Experimental Track (`§6dy` / `§6dz`) to True M5 & Set-Space Mastery:**
   - **Track A (Immediate Low-Risk M5 Win on the Qualified `Mode 2` Routed Unfused Path):**
     1. Parallelize `classifyTilesMain` across 16-thread SIMD subgroups per tile using `var<workgroup>` / SIMD reductions (cutting serial classifier loop work by `12×–16×` and deleting `rA[16]`, `rB[16]`, `rHit[16]`).
     2. Dispatch `cpS1`, `cpS2`, and `cpFallback` inside a single shared compute pass.
   - **Track B (Zero-Spill `0 B` Fast-Path Kernel — `S1_DEPTH0_MONOTONE`):**
     Split `S1` into a **Zero-Array Chronological Monotone Streamer (`s1MonotoneMain`, `0 B` spill)** for rays/tiles where all active bins satisfy depth-0 `coarseMinSlope > 0.0` (walking `bin = 0..7` in chronological order and testing `-w_z, 0, +w_z` in monotone sign order with **zero `stack`, `bands`, `knots`, or `pieces` arrays**), while routing non-monotone rays/tiles to the proven `bound-stack-15` `s1ProducerMain` fallback!
