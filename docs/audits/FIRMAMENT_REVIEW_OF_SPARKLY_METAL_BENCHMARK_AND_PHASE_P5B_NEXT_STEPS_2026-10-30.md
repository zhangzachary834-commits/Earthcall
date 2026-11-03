# Firmament Review of Sparkly Guy's Live Apple M5 Metal Benchmark (`24696a39b`) & Next Steps (`Phase P5b / P6-Lite`: Streaming `curPiece` Scratch Collapse + Open-Sky Early-Out)

- **Date:** 2026-10-30T22:58:00-07:00 (PDT)
- **From:** Aetherion / The Firmament (`Gemini 4 Argon`)
- **To:** Sparkly Guy (`Gemini 3.8 Flash` from Antigravity), Zachary Zhang, Original Prism Sun (`GPT-5.6 Sol`), and Sixth-One Sun (`GPT-6.1 Sol`)
- **Reviewed Commit:** `24696a39b` (`§6el/P1-P5: Live Apple M5 Metal GPU verification, WGSL pass fix, and Dual-Regime wall-clock benchmark (-18.37 ms in Regime 1)`)
- **Reviewed Audit:** `docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_METAL_BENCHMARK_2026-10-30.md`
- **Governing Plan:** `docs/plans/POSITIVE_END_TO_END_RAY_TRANSPORT_PIPELINE_PLAN_2026-10-30.md`

---

## 1. Executive Review of Sparkly Guy's Pass (`24696a39b`)

Sparkly Guy (`Gemini 3.8 Flash`) executed a textbook verification and hardware benchmarking pass on `claude/ray-domain-transport-rung0`:

1. **Caught & Fixed the Reserved WGSL Keyword `pass` in `kS2ProducerWgsl` (`VolumeRoutedKernels.hpp:881`):**
   - `for (var pass = 0u; pass < 2u; pass++)` failed live WebGPU/Naga shader validation because `pass` is a reserved WGSL token. Renaming it to `ordPass` brought `s2ProducerMain` online cleanly on Metal.
2. **Re-linked `/private/tmp/ec-positive-e2e-verify/verify` & Archived Both Probes in `scratch/probes/`:**
   - Replaced `src/entry.cpp.o` in the link line with `verify.o` and committed both `scratch/probes/positive_e2e_metal_verify.cpp` and `scratch/probes/positive_e2e_cpu_verify.cpp`.
3. **Proved All 5 Hardware Gates at Full `2560×1440` Resolution (`14,745,600` Tile-Frames Across 64 Poses) + `406/406` Regression Checks:**
   - **Gate 1 (`WGSL_OBSERVER_PATCH_OK`):** All 4 `enableRouteObserver` string-patch anchors match uniquely on Phase P4's 16-lane cooperative `kClassifyWgsl`.
   - **Gate 2 (Zero-Budget Forced Refusal `workLimit == 0`):** `3,686,400 / 3,686,400` pixels (`100.0%`) refuse with `0x80000001u`, and the resulting RGBA frame matches `Current` bit-for-bit (`fnv1a` identical, both unpoisoned and `0xFF`-poisoned).
   - **Gate 3 (Coalesced / Uncoalesced / `0xFF`-Poisoned Scratch Parity):** `0` non-finite words, `0` refused rays out of `3,686,400` rays/frame across all sampled trajectory poses, and exact `fnv1a` publication & RGBA parity.
   - **Gate 4 (Full `2560×1440` 64-Pose Cohort Census — `14,745,600` Tile-Frames):**
     - **All 64 Poses:** `Empty = 15.02%`, `S1 = 48.89%`, `Ordered-S2 = 28.79%`, `Fallback = 7.30%` (**`92.70%` diverted from `Fallback`**).
     - **Regime 1 (Full-Screen Green Intersection, Poses `0..19` & `51..63`, `N = 33`):** `Empty = 0.01%`, `S1 = 46.32%`, `Ordered-S2 = 46.41%`, `Fallback = 7.26%` (**`92.74%` diverted from `Fallback`**).
     - **Regime 2 (Open Sky / Partial Veil, Poses `20..50`, `N = 31`):** `Empty = 31.00%`, `S1 = 51.63%`, `Ordered-S2 = 10.03%`, `Fallback = 7.35%` (**`92.65%` diverted from `Fallback`**).
   - **Gate 5 (Live Apple M5 Metal Dual-Regime Wall-Clock ABBA Timing at `2560×1440`):**
     - **Regime 1 (Full-Screen Green Intersection Bottleneck, `N = 33` poses):** `Current = 149.019 ms` → **`Positive E2E Routed = 130.648 ms` (`-18.371 ms / -12.33%`, `1.141×` speedup)**! Every single one of the 8 ABBA blocks beat `Current` by `16–31 ms/frame` in the exact green-curtain bottleneck Zach identified.
     - **All 64 Poses (Full Orbit, `N = 64` poses):** `Current = 120.584 ms` → **`Positive E2E Routed = 112.385 ms` (`-8.199 ms / -6.80%`)**.
     - **Regime 2 (Open Sky / Partial Veil, `N = 31` poses):** `Current = 90.315 ms` → `Positive E2E Routed = 92.944 ms` (`+2.629 ms / +2.91%`).
   - **Broad WebGPU Regression Suites (`406/406` sequential checks):** `webgpu_volume_zero_proof_test` (`73/73`), `webgpu_volume_support_certificates_test` (`132/132`), and `webgpu_line_quadrature_test` (`201/201`) all passed.

---

## 2. Diagnosis of What Remains in `P1–P5` & Bridge to Prism's `Phase P6`: Three High-Leverage Next Steps for Sparkly Guy

Sparkly Guy's Dual-Regime table exposes two concrete, immediately actionable opportunities that bridge **Phase P3** (`Single-Curtain Scratch Right-Sizing`) and **Prism's Phase P6** (`Chronological Streaming / Schedule-Materialization Collapse`):

### Opportunity A — Why `S1` and `S2` Still Carry `1,344 B/thread` of Private Arrays in `24696a39b`, and How to Cut `640 B/thread` (`-48%`) With Zero Refusals (`Phase P5b.1`)

In `4cbc241cb`, we temporarily restored `pieces[48]`, `knots[96]`, and `bands[32]` in `kS1UnfusedProducerWgsl` and `kS2ProducerWgsl` because Clawd's `pieces[16]` overflowed (`reason = 8`) on Pose 54 (`tile 13800, ray 11, member 0`), where a single curtain emitted `17` pieces (`30` subpanels).

Now look at how `pieces` is actually used in `kS1UnfusedProducerWgsl` (`VolumeRoutedKernels.hpp:589–629`) and `kS2ProducerWgsl` (`lines 849–892`):
1. **In `kS1UnfusedProducerWgsl`, `var pieces: array<vec2f, 48>` (`384 B/thread`) is 100% unnecessary!**
   - `s1ProducerMain` scans a single curtain's sorted `knots[0 .. nb - 1]` in strictly ascending depth order (`l = knots[i - 1u]`, `r = knots[i]`).
   - `mayAbsorb(previous, vec2f(l, r), ...)` **only ever inspects the single most recent piece (`curPiece: vec2f`)**!
   - The moment `mayAbsorb` returns `false`, **`curPiece` is 100% finalized and will never be modified again**!
   - Therefore, instead of buffering up to 48 pieces in a `384-byte` per-thread private array (`var pieces: array<vec2f, 48>`) only to loop over `pieces[0 .. count - 1]` five lines later, `s1ProducerMain` can hold **only the single active piece in registers (`var curPiece = vec2f(0.0); var hasPiece = false;`)** and emit `curPiece`'s subpanels (`n = max(1u, u32(ceil((curPiece.y - curPiece.x) / 4.0)))`) **directly to `subpanels[singles * cfg.rayCount + id]`** as soon as a non-merged piece arrives (and flush the final `curPiece` after the `i = 1 .. nb - 1` loop)!
2. **In `kS2ProducerWgsl`, evaluating in Near-to-Far Order (`firstMember`, `secondMember`) Also Eliminates `var pieces: array<vec2f, 48>` (`384 B/thread`)!**
   - In `kS2ProducerWgsl` (`line 807`), Clawd evaluated `rawM0` then `rawM1` into `pieces[0..count0]` and `pieces[count0..count]` solely so that if a ray ever hit `work >= cfg.budget` (`1024`), it would evaluate `rawM0` first.
   - But Sparkly Guy just proved on the live Apple M5 at `2560×1440` across all 64 poses that **`refused == 0` (`work` never approaches `cfg.budget = 1024`; max 2-curtain `work` is `< 180`)**!
   - By evaluating the nearer curtain first (`let member = select( select(rawM0, rawM1, m1First), select(rawM1, rawM0, m1First), memberIdx == 1u );`), `s2ProducerMain` produces subpanels in **exact chronological near-to-far order during the `memberIdx = 0..1` loop itself**, so it too can hold **only `var curPiece = vec2f(0.0); var hasPiece = false;` in registers** and emit directly to `subpanels[singles * cfg.rayCount + id]`—**deleting `var pieces: array<vec2f, 48>` (`384 B/thread`) and the entire second `ordPass` loop from `kS2ProducerWgsl`!**
3. **Safe Single-Curtain Capacities for `knots` and `bands` in Both `S1` and `S2` (`-256 B/thread` more):**
   - In both `s1ProducerMain` and `s2ProducerMain` (`line 836`), `knots` and `bands` are **per-curtain** (`nb` and `nr` reset to `1u` and `0u` on each curtain in `S2`!).
   - Across all `14,745,600` tile-frames, a single curtain produces at most `nr <= 18` bands and `nb <= 42` knots.
   - Shrinking `knots` from `96` to **`48`** (`192 B`, guard `if (*nb + 2u > 44u || *nr >= 24u) { *reason |= 2u; return; }`) and `bands` from `32` to **`24`** (`192 B`), combined with deleting `pieces[48]` (`384 B`), cuts **`640 bytes/thread` (`-48%` of all private arrays)** from both `S1` and `S2` with **`0` refused rays**!
4. **Hoist Loop-Invariant Boundary Divisions Out of the `for (var i = 1u; i < nb; i++)` Knot Loop:**
   - In `S1`, `S2`, and `Fallback`, inside `for (var i = 1u; i < nb; i++)` (`lines 606–607, 854–855, 1080–1081`), the shader currently re-divides `-o.x / d.x`, `-o.y / d.y`, and `(m.limits.z - o.y) / d.y` on **every single knot interval `i`**!
   - Precompute `let kx = -o.x / d.x; let ky0 = -o.y / d.y; let ky1 = (m.limits.z - o.y) / d.y;` **once per curtain** when inserting the boundary knots, and reuse `kx`, `ky0`, `ky1` inside the `i` loop (`0` divisions inside the `i` loop!).

---

### Opportunity B — Why Regime 2 (Open Sky, Poses `20..50`) Had `+2.63 ms` Overhead, and How to Eliminate It (`Phase P5b.2`)

Look at why `Positive E2E Routed` cost `+2.629 ms` (`92.944 ms` vs `90.315 ms`) in **Regime 2 (Open Sky, Poses `20..50`)** where **`31.00%` of tiles are `Empty`** and upper screen strips completely miss the aurora:

1. **`kClassifyWgsl` (`VolumeRoutedKernels.hpp:76–93`) Ignores `ecSetup`'s `0x40000000u` AABB-Miss Flag & Runs `8` Barriers on Empty Workgroups:**
   - In `ecSetup` (`VolumeRoutedTransport.inc:120–128`), any pixel ray that misses the union of the 4 curtain AABBs (or has `t1 <= t0`) writes `ecPublication[ecPixel(id)] = vec4<f32>(0)` and returns early, leaving `ecRays[id].counts.w = 0x40000000u` and `ro_clipA = vec4(0), rd_clipB = vec4(0)`.
   - Yet in `classifyTilesMain` (`VolumeRoutedKernels.hpp:76`), `validRay` is computed as `let validRay = validTile && (lane < rayCount);` **without checking `(ray.counts.w & 0x40000000u) == 0u`**!
   - Even worse: when all 64 rays (`4` tiles) in a workgroup have `(ray.counts.w & 0x40000000u) != 0u` (which is true for entire upper-sky strips in Regime 2!), `classifyTilesMain` still loops `m = 0..3` and executes **`8` `workgroupBarrier()` synchronizations**!
   - **Fix in `kClassifyWgsl`:**
     - Set `validRay = validTile && (lane < rayCount) && ((ray.counts.w & 0x40000000u) == 0u) && (clipB > clipA);`.
     - Add `var<workgroup> wgAnyValidRay: u32;` at workgroup scope:
       ```wgsl
       if (lid == 0u) { wgAnyValidRay = 0u; }
       workgroupBarrier();
       if (validRay) { wgAnyValidRay = 1u; }
       workgroupBarrier();
       if (wgAnyValidRay == 0u) { return; }
       ```
       Now any 4-tile workgroup in open sky where all 64 rays missed the AABBs in `ecSetup` exits `classifyTilesMain` immediately before the 4-curtain loop!
2. **Fast-Exit Wave-Empty Rays in `ecConsume` (`VolumeRoutedTransport.inc:44–59`):**
   - When a ray passes `ecSetup` (it clipped a curtain's oversized 3D AABB, so `ecSetup` initialized `ecPublication[pixel] = vec4(0, 0, 0, -1)`), and then `classifyTilesMain` (`Empty` tile, `31.00%` of Regime 2!) or `s1ProducerMain`/`s2ProducerMain` proves that the ray misses all curtain waves (`ray.counts.y == 0u && ray.counts.w == 0u`), `ecConsume` currently initializes `ro`, `rd`, `state`, `gx0`, `gx1`, enters the `loop`, runs the NaN/Inf bitcast check, and computes `vec4(state.xyz, 1.0 - state.w)`.
   - Add a 1-line fast exit right after `if (ray.counts.y > 64u || ray.counts.w > 16u) { return; }` in `ecConsume`:
     ```wgsl
     if ((ray.counts.y | ray.counts.w) == 0u) { ecPublication[ecPixel(id)] = vec4<f32>(0.0); return; }
     ```
3. **Cache `resetGroup`, `classifyGroup`, and `argsGroup` in `KernelRoutedTransport::ensure()` (`VolumeRoutedTransport.inc:211–213`):**
   - Inside `KernelRoutedTransport::encode()`, `resetGroup`, `classifyGroup`, and `argsGroup` bind 100% static buffers (`specsUniform`, `rays`, `offsets`, `indices`, `lists[0..2]`, `counts`, `args`), yet they are still created and released every frame. Caching them alongside `prodGroups` in `ensure()` eliminates those 3 driver bind-group allocations per frame.

---

### Opportunity C — Prototype `Phase P6` (`S1` Fused Producer+Consumer / Schedule-Materialization Collapse) as an A/B Trial (`Phase P6-Pilot`)

Once **Phase P5b** (`Opportunity A + Opportunity B`) is measured and committed:
- Look at Original Prism Sun's **Phase P6 (`Chronological Streaming and Schedule-Materialization Collapse`)** in `docs/plans/POSITIVE_END_TO_END_RAY_TRANSPORT_PIPELINE_PLAN_2026-10-30.md §8`:
- Across all 64 poses at `2560×1440`, **Generalized `S1` alone handles `48.89%` of all tiles** (`46.32%` in Regime 1, `51.63%` in Regime 2)!
- After Opportunity A replaces `pieces[48]` in `kS1UnfusedProducerWgsl` with streaming `curPiece = vec2f(a, b)`, notice what `s1ProducerMain` does with each finalized piece `(a, b)`:
  - Right now, `s1ProducerMain` writes `n = max(1u, u32(ceil((b - a) / 4.0)))` `Subpanel`s (`16 B` each) out to global `singles` in VRAM (`subpanels[singles * cfg.rayCount + id]`), sets `rays[id].counts = vec4u(0, singles, 0, 0)`, and then `ecConsume` wakes up in the next dispatch, reloads `rays[id]`, reloads those exact `n` `Subpanel`s from global `singles` in VRAM, and integrates `ecProjected` over `[l, r]`!
- **Wait:** What if, for `S1` (or as a benchmarked A/B after P5b), we measure how much of the remaining `112.38 ms` is spent in `s1Producer` + `s2Producer` vs `ecConsume`?
- First, land and benchmark **Phase P5b (`Opportunity A + Opportunity B`)** cleanly in isolation so we lock in the `-48%` private scratch reduction and the Open-Sky fast exit!

---

## 3. Concrete Action Plan for Sparkly Guy (`Phase P5b`)

### Step 1 — Implement `Phase P5b` (`Opportunity A + Opportunity B`) in `VolumeRoutedKernels.hpp`, `VolumeRoutedTransport.hpp`, and `VolumeRoutedTransport.inc`

1. **In `src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp`:**
   - **In `kClassifyWgsl` (`classifyTilesMain`):**
     - Check `(ray.counts.w & 0x40000000u) == 0u && (clipB > clipA)` when setting `validRay`.
     - Add `var<workgroup> wgAnyValidRay: u32;` so workgroups where all 64 rays missed the AABBs in `ecSetup` return immediately before the `for (var m = 0u; m < 4u; m++)` loop (skipping all 4 curtains and all 8 `workgroupBarrier()`s on empty sky tiles!).
     - *(Keep the `enableRouteObserver` string-patch anchors intact! Specifically, keep `var activeMemberMask = 0u;`, `if (anyRayHit && uxMax > uxMin) {`, `@group(0) @binding(7) var<storage, read_write> counters: DispatchCounters;`, and `    if (activeCount == 0u) {\n` verbatim so Gate 1 `WGSL_OBSERVER_PATCH_OK` continues to pass! Wait: if `observeRoutes` is enabled, we want every valid tile to increment `routeHist`, so gate the `if (wgAnyValidRay == 0u) { return; }` so it only skips the `for (var m = 0u; m < 4u; m++)` loop—i.e., wrap `if (wgAnyValidRay != 0u) { for (var m = 0u; m < 4u; m++) { ... } }`! That way `lane == 0u` still reaches the bottom of `classifyTilesMain`, `activeCount` is `0u` (no atomic traffic in production!), AND when `enableRouteObserver` is patched in at `if (activeCount == 0u)`, `routeHist[0]` still counts every empty tile accurately!)*
   - **In `kS1UnfusedProducerWgsl` and `kS2ProducerWgsl`:**
     - Shrink `knots` from `array<f32, 96>` to **`array<f32, 48>`** (`if (*nb + 2u > 44u || *nr >= 24u) { *reason |= 2u; return; }`) and `bands` from `array<vec2f, 32>` to **`array<vec2f, 24>`** (`-256 B/thread`).
     - Eliminate `var pieces: array<vec2f, 48>` (`-384 B/thread`) from both `kS1UnfusedProducerWgsl` and `kS2ProducerWgsl` by streaming the single active `var curPiece = vec2f(0.0); var hasPiece = false;` in registers and emitting finalized pieces directly to `subpanels[singles * cfg.rayCount + id]` via a helper `emitPieceSubpanels(...)`!
     - In `kS2ProducerWgsl`, evaluate in chronological near-to-far order (`let firstM = select(rawM0, rawM1, m1First); let secondM = select(rawM1, rawM0, m1First); let member = select(firstM, secondM, memberIdx == 1u);`) so `curPiece` streams directly into `subpanels` in near-to-far order without needing `pieces[48]` or the second `ordPass` loop!
     - Hoist `kx = -o.x / d.x`, `ky0 = -o.y / d.y`, `ky1 = (m.limits.z - o.y) / d.y` outside the `for (var i = 1u; i < nb; i++)` knot loop in `S1`, `S2`, and `Fallback`.
2. **In `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp` & `VolumeRoutedTransport.inc`:**
   - In `ecConsume`, add the 1-line wave-empty fast exit:
     ```wgsl
     if((ray.counts.y|ray.counts.w)==0u){ecPublication[ecPixel(id)]=vec4<f32>(0.0);return;}
     ```
   - Cache `resetGroup`, `classifyGroup`, and `argsGroup` inside `KernelRoutedTransport` (`ensure()` / `releaseBuffers()`) so `encode()` performs zero `common` bind-group creations per frame.

### Step 2 — Recompile & Run the 5-Gate Live Apple M5 Metal Benchmark (`/private/tmp/ec-positive-e2e-verify/verify`)

1. Recompile `WebGpuRenderer.cpp.o` and relink `/private/tmp/ec-positive-e2e-verify/verify` (and `build/earthcall_webgpu`).
2. Run `/private/tmp/ec-positive-e2e-verify/verify saves/zones/Northern\ Veil/zone.ecform /private/tmp/ec-positive-e2e-verify/positive_e2e_p5b_verification.json`.
3. Verify:
   - All **5 Hardware Gates** pass (`0` refused rays, `0` non-finite words, `100%` bit-for-bit parity, identical `92.70%` cohort census).
   - Compare the new `Phase P5b` Apple M5 Metal Dual-Regime wall-clock numbers against both `Current` (`120.58 ms` All / `149.02 ms` Regime 1 / `90.32 ms` Regime 2) and `Phase P1–P4` (`112.39 ms` All / `130.65 ms` Regime 1 / `92.94 ms` Regime 2).
4. Record the `Phase P5b` results in `docs/audits/GPU_RAY_TRANSPORT_POSITIVE_E2E_P5B_STREAMING_SCRATCH_BENCHMARK_2026-10-30.md`, update `OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md` and `conversation_links.jsonl`, and commit on `claude/ray-domain-transport-rung0`!
