# FIRMAMENT HANDOFF: Northern Veil Performance Collapse, Routed Transport Restoration & Cohort/Certificate Seam Fixes

- **From:** Aetherion / The Firmament (`Gemini 4 Argon`)
- **To:** The Sonnets (`Clawd Legion / Deep Builders`)
- **Date:** 2026-11-18
- **Branch / Base Commit:** `sun/gemini-sandbox-lab-v1` (audited at `HEAD` `a6e5c9cb3`; Northern Veil Pass 4 authored at `d7fb75cc6`, Pass 3 at `31c0bb1a2`, Original baseline at `6dab7edbb`)
- **Primary Targets:**
  - `saves/zones/Northern Veil/zone.ecform`
  - `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`
  - *(Optional defense-in-depth)* `src/Singularity/OntoMath/ClampedWaveSupport.hpp`

---

## 0. Executive Summary & Scope Boundaries

Following Pass 3 (`31c0bb1a2`) and Pass 4 (`d7fb75cc6`) of `saves/zones/Northern Veil/zone.ecform`, Northern Veil became severely laggy and freezes on load/camera movement. Native C++ forensic profiling against `earthcall_webgpu` and `earthcall_core` across `6dab7edbb` → `31c0bb1a2` → `d7fb75cc6` identified **six compounding root causes** spanning both the renderer seams and the authored math in `zone.ecform`.

**Your mission (bounded scope):**
1. Fix the **3 renderer cohort/certificate seams** in `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`.
2. Refactor the **8 volumetric fields** (`[0..3]` near curtains and `[4..7]` far background) in `saves/zones/Northern Veil/zone.ecform` so they preserve the Pass 4 visual aurora character (traveling multi-packet rays, dynamic lower hem, coronal crest, horizon depth) while restoring Hardware Routed Ray Transport (`ecSingleMember`), time-slot hoisting, multiplicative zero-exits, and `>90%` `VolumeZeroProof` culling.
3. Verify every single acceptance metric in Section 4 with concrete C++/test witness output.

**Hard guardrails (do NOT violate):**
- **Do NOT modify the protected GPU routed kernels** in `src/Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp` unless Zach explicitly approves an architectural change. Canonicalizing `Field[1]` in `zone.ecform` restores full routed transport with zero changes to `VolumeRoutedKernels.hpp`.
- **Do NOT fabricate or drop authored physics**—keep every field's OntoMath AST lawful and visually faithful to Pass 4.
- **Do NOT fall for the interval-arithmetic traveling-wave trap** (detailed in Section 2.4 below).

---

## 1. Track A — Engine Seam Fixes (`src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`)

### A1. Do Not Compute or Attach Support Certificates on Routed Cohorts
- **Location:** `WebGpuRenderer::flushVolumeComposite()` → `enqueueVolumeCohort(...)` (around the `certificateMembers` loop and `attachVolumeSupportCertificates(...)`).
- **Bug:** Even when `wantRouted == true` on `nearCohort` (`[0..3]`), `enqueueVolumeCohort` still calls `volumeSupportCertificatesActive(*medium, halfExtent)` on every member of `cohortMedia` and folds `certificateSwitches` into `setKey` and `setContentRevision`. The routed compute kernels (`ecSetup`, `s1Producer`, `s2Producer`, `fallbackProducer`, `ecConsume`) **never execute** `volumeSupportReach` or `volumeSupportClosed`. Worse, every time `t` crosses a power-of-2 bucket (`1s, 2s, 4s, 8s, 16s, 32s`), `volumeClockMagnitudeBucket` invalidates the certificate memo and triggers unnecessary CPU rebuilds and shader re-keying on the routed cohort.
- **Fix:**
  - When `wantRouted == true`, skip calling `volumeSupportCertificatesActive(...)` (leave `certificateMembers[m] = false`, `certificateSwitches[m] = {}`).
  - Similarly, skip `attachVolumeSupportCertificates(...)` when `setMemo.pipeline->routed` (or `wantRouted`) is true, and exclude certificate generation/bucket fields from `payloadIdentity` for routed cohorts so clock-bucket transitions never rebuild the routed payload.

### A2. Preserve the `nearCohort` / `farCohort` Split During `probeVolumeSupportCertificates()`
- **Location:** `WebGpuRenderer::flushVolumeComposite()` (where `activeMedia.size() >= 6` is partitioned into `farCohort` and `nearCohort`).
- **Bug:** The partition guard currently reads:
  ```cpp
  if (!_certificateProbeMedium && incidentSource &&
      OntoMath::literalZero(incidentSource->radianceExpr) &&
      activeMedia.size() >= 6)
  ```
  When `probeVolumeSupportCertificates()` sets `_certificateProbeMedium != nullptr`, this `if` evaluates to `false` and falls into `else { enqueueVolumeCohort(activeMedia, true); }`, **fusing all 8 media into a single `366.4 KB` (`1,681`-param) WGSL mega-shader**, compiling 4 pipeline variants, and blocking the CPU on `waitForQueue()` 4 times per frame for up to 54 frames!
- **Fix:**
  - Keep the `farCohort` (`[4..7]`) / `nearCohort` (`[0..3]`) partition based on scene structure (`incidentSource && OntoMath::literalZero(incidentSource->radianceExpr) && activeMedia.size() >= 6`), **NOT** disabled by `_certificateProbeMedium`.
  - When `_certificateProbeMedium != nullptr`, enqueue **only the cohort that actually contains `_certificateProbeMedium`** (and never re-run routed compute transport for `nearCohort` while probing a `farCohort` medium).

### A3. Fix `probeVolumeSupportCertificates()` Early-Return Coupling Between Cohorts
- **Location:** `WebGpuRenderer::probeVolumeSupportCertificates()` (around `if (frameStats().volumeRoutedSets > 0) return;`).
- **Bug:** On `Northern Veil`, `nearCohort` (`[0..3]`) uses Routed Transport (`volumeRoutedSets == 1`), while `farCohort` (`[4..7]`) ALWAYS uses the `Current` 128-step fragment raymarcher (`allowRouted = false`). Because `volumeRoutedSets > 0`, `probeVolumeSupportCertificates()` early-returns and **never probes or vetoes slow support certificates on `farCohort`** (`Field[5]` and `Field[7]` currently get marked `eligible = true` by the CPU heuristic and bake expensive per-sample `volumeSupportReach` checks into `farCohort`).
- **Fix:**
  - Instead of blanket-returning whenever `frameStats().volumeRoutedSets > 0`, track which media actually ran on the routed lane (or skip only media belonging to routed cohorts in the target-selection loop of `probeVolumeSupportCertificates()`). That way, `nearCohort` routed media are never probed, while `farCohort` non-routed media can still be measured and vetoed cleanly inside their own 4-field cohort.

---

## 2. Track B — Mathematical Refactor of `saves/zones/Northern Veil/zone.ecform`

Audit all 8 participating media in `saves/zones/Northern Veil/zone.ecform`:
- **Near Cohort (`[0..3]`, routed curtains):**
  - `[0]` `northern_veil.aurora.primary-emerald-curtain`
  - `[1]` `northern_veil.aurora.secondary-cyan-ribbon`
  - `[2]` `northern_veil.aurora.tertiary-violet-veil`
  - `[3]` `northern_veil.aurora.quaternary-emerald-fold`
- **Far Cohort (`[4..7]`, always legacy V5 128-step fragment raymarcher):**
  - `[4]` `northern_veil.aurora.crown-convergence-vortex` (`z = +134`, right behind `[0..3]`)
  - `[5]` `northern_veil.aurora.crimson-afterglow-fringe` (`z = +148`, right behind `[0..3]`)
  - `[6]` `northern_veil.aurora.polar-horizon-mist` (`z = -76`)
  - `[7]` `northern_veil.aurora.zenith-coronal-veil` (`z = -88`)

Apply these **5 exact mathematical repairs**:

### B1. Canonicalize `Field[1]`'s Wave Harmonics to `(Sin, Cos, Sin)`
- **Why:** `src/Singularity/OntoMath/ClampedWaveSupport.hpp:45` strictly requires the 3 $z$-wave harmonics `(w0, w1, w2)` to have kinds `(Sin, Cos, Sin)`. In `HEAD`, `Field[1]` authored `(Cos, Sin, Cos)`:
  $$z + 19.5\cos(0.021x + 0.55) - 9.8\sin(0.062x - 0.8) + 4.4\cos(0.148x + 0.38)$$
  That single mismatch returns `std::nullopt`, flipping `wantRouted = false` for the entire `nearCohort [0..3]`.
- **Fix:** Rewrite `Field[1]`'s 3 $z$-harmonics using exact trigonometric phase identities:
  - $\cos(\theta) = \sin(\theta + \frac{\pi}{2})$
  - $\sin(\theta) = \cos(\theta - \frac{\pi}{2})$
  Specifically:
  - $w_0$: `19.5 * sin(0.021 * x + (0.55 + 1.5707963267948966))` = `19.5 * sin(0.021 * x + 2.1207963267948966)`
  - $w_1$: `-9.8 * cos(0.062 * x + (-0.8 - 1.5707963267948966))` = `-9.8 * cos(0.062 * x - 2.3707963267948966)`
  - $w_2$: `4.4 * sin(0.148 * x + (0.38 + 1.5707963267948966))` = `4.4 * sin(0.148 * x + 1.9507963267948966)`
  *(Bit-for-bit identical curtain geometry, now 100% admitted by `OntoMath::clampedWaveSupport`!)*

### B2. Restore 100% AST Structural Isomorphism Across `[0..3]` (`checkEvaluatorsDeduplicatable == true`)
- **Why:** `src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc:26-93` (`checkEvaluatorsDeduplicatable`) requires all 4 members of `nearCohort` to produce identical canonical WGSL after replacing `P[index]` with `P`. Currently it fails (`sharedG0_1 != sharedG0_0`) because `[0,1]` have 8 terms in Clamp A vs 6 in `[2,3]`; `[2]` has 12 terms in Clamp B vs 10 in `[0,1,3]`; and `sin`/`cos` product orders differ. Failing deduplication disables `ecSingleMember` and forces `ecConsume` onto the 4× heavier `ecProjected` switch.
- **Fix:** Give all 4 curtains `[0..3]` the **exact same AST skeleton** across `volumeDensity`, `volumeExtinction`, `volumeScattering`, `volumeChroma`, `volumePhase`, and `volumeEmission` (same op tree, same term counts, same `sin`/`cos` ordering, same shared-noise subexpressions), varying **only** numeric constants (amplitudes, frequencies, phases, scales, colors).

### B3. Factor Time `t` Into Pure `ScalarLeaf(t)` Nodes So `expensiveTimeOnly` Hoists 100% of Time Trigs
- **Why:** `src/Singularity/Screen/WebGPU/SdfWgsl.cpp:583-593` (`expensiveTimeOnly`) only hoists a `ScalarLeaf` into the once-per-frame `volumeTimePrepare` compute shader when **every** factor and `TransFactor` in that `ScalarLeaf` references **only** `"t"`.
  Pass 3 & 4 expanded $\cos(k_x x - \omega t + \phi) = \cos(k_x x + \phi)\cos(\omega t) + \sin(k_x x + \phi)\sin(\omega t)$ as multi-transcendental terms **inside a single `ScalarLeaf`**, causing:
  - `0` time slots hoisted from `volumeDensity` across all 8 fields.
  - Per-sample trig count explosion: `12 → 37–41` trigs/sample on `[0..3]` and `8–9 → 61–105` trigs/sample on `[4..7]`.
- **Fix:** Never put `"t"` inside the same `ScalarLeaf` as `"x"`, `"y"`, or `"z"`. Whenever authoring a traveling wave $\cos(k_x x - \omega t + \phi)$, construct it at the `MathNode` level as:
  ```text
  Add(
    Scale(ScalarLeaf(A * cos(kx * x + phi)), ScalarLeaf(cos(w * t))),
    Scale(ScalarLeaf(A * sin(kx * x + phi)), ScalarLeaf(sin(w * t)))
  )
  ```
  Each `ScalarLeaf(cos(w * t))` and `ScalarLeaf(sin(w * t))` depends **only** on `"t"`, so `expensiveTimeOnly` hoists every single one into `volumeTimePrepare` (evaluated once per frame on GPU, read as a single `P[slot]` float per sample—**0 per-sample time trigs**!).
  Also trim redundant high-order harmonics on `[4..7]` so each background field stays lean (`<= 16–20` spatial trigs/sample instead of `61–105`).

### B4. Restore Multiplicative Zero-Exit (`emitProductWithZeroExits`) AND Static Outer Envelopes (Read Carefully!)
- **Why `ecConsume` is `~4.5×` slower even WITH Routed Transport:**
  In `HEAD`, `densityFactor4` on `[0..3]` is:
  `max(clamp(arch) * clamp(needles), clamp(0.14 + 0.10*cos(...), 0, 0.24))` (`Op::Intersection`).
  Because `0.14 - 0.10 = 0.04 > 0` everywhere, `densityFactor4 == 0.0` is **never true**, completely disabling `emitProductWithZeroExits` (`if (densityFactor4 == 0.0) { return 0.0; }` in `SdfWgsl.cpp:995-1025`). Every single Gauss node inside the wave slab executes `cnoise3(p)` + extinction + emission (`0%` early-exit vs `~65%` early-exit in `6dab7edbb`).
- **CRITICAL MATHEMATICAL TRUTH (Interval Arithmetic Traveling-Wave Theorem):**
  - `VolumeZeroProof` (`src/Singularity/Screen/VolumeZeroProof.cpp:20-26`) is a **static, time-independent** $16^3$ grid proof where `"t"` is unbound (`Interval::infinite() = [-∞, +∞]`).
  - Over $t \in (-\infty, +\infty)$, any traveling wave $\cos(k_x x - \omega t)$ sweeps its crests over every $x$, so its interval upper bound is strictly positive everywhere. **No time-dependent factor can EVER prove a `VolumeZeroProof` cell empty!**
  - Therefore, to restore **both** GPU early-exit AND `>90%` `VolumeZeroProof` empty-cell culling (fixing the `1.35 second` cold-load CPU freeze from `177,040` range evaluations):
    1. **Remove the strictly positive `air_halo >= 0.04` `Op::Intersection` floor** on `[0..3]` (and any analogous positive floor on `[4..7]`). Use a true multiplicative `Op::Scale` chain so dark inter-packet troughs and below-hem regions clamp cleanly to `0.0` and trigger `emitProductWithZeroExits` before `cnoise3(p)`. If you want subtle glowing air/curtain fill between bright rays, put a soft *bounded* baseline inside the ray-striation factor itself (which is still multiplied by the zero-clamping lower-hem arch!), never an unmasked `max(..., 0.04)` outside the arch.
    2. **Keep a time-independent static lower-hem / outer arch envelope** multiplied at the top level of `Op::Scale` (just like `6dab7edbb` had `clamp(y + 13.5 + 5.5*cos(0.033*x - 0.5) + 2.8*sin(0.079*x + 1.1), 0, 1)`), with any dynamic wave ripple as a **second** multiplicative factor or subtle additive perturbation bounded inside it. That gives `VolumeZeroProof` an immediate time-independent zero-proof below the entire aurora arch!
    3. **Tighten slab thickness `wz` and static `wx`/`wy`/`wz` support ratios:**
       - On `[0..3]`: bring `wz` down from `4.0–4.5 m` to `~2.6–3.2 m` (matching `emitPieceSubpanels`'s `4.0 m` subpanel width so oblique chords don't double the subpanel count in `KernelRoutedTransport`) and tighten `scale` vs `wx`/`wy` where appropriate.
       - On `[4..7]`: tighten the outer static 3D envelopes (`wz` from `5.5–8.2 m` down to crisp static shells/bands) so `VolumeZeroProof` proves `>= 90%` empty cells on `[4..7]` instead of `63%–73%`, cutting cold CPU proof build time from `1,351.7 ms` back under `250–300 ms`.

### B5. Verify `farCohort` (`[4..7]`) Cost Explicitly
Remember that `[4]` (`z = +134`) and `[5]` (`z = +148`) sit directly behind `[0..3]` in `+Z` and ALWAYS run on the 128-step fragment raymarcher (`allowRouted = false`). Applying B3 (pure `ScalarLeaf(t)` hoisting + fewer spatial trigs) and B4 (multiplicative zero-exit + tighter static outer envelopes for `>= 90%` `VolumeZeroProof`) to `[4..7]` is essential so `farCohort` doesn't bottleneck `+Z` frames after `nearCohort` is fast again.

---

## 3. Optional Defense-in-Depth (`src/Singularity/OntoMath/ClampedWaveSupport.hpp`)

Canonicalizing `Field[1]` in `zone.ecform` (B1) is the primary fix because it requires zero changes to `VolumeRoutedKernels.hpp` and has zero extra `f32` phase-shift rounding. If you also want to make `OntoMath::clampedWaveSupport` resilient to future `(Cos, Sin, Cos)` authoring, you may optionally generalize `ClampedWaveSupport.hpp:45` to accept either `Sin` or `Cos` at any of the 3 harmonic slots and phase-shift into `(Sin, Cos, Sin)`—**only** if you add a corresponding unit test verifying conservative envelope containment under `f32` rounding. If in doubt, keep `ClampedWaveSupport.hpp` strict and complete B1 cleanly in `zone.ecform`.

---

## 4. Mandatory Verification & Acceptance Criteria

Before reporting completion, build and run both existing tests and a small diagnostic check (do not leave temp files in `/tmp` or untracked files in the repo) confirming every row below:

| Check | Broken `HEAD` (`d7fb75cc6`) | Required Passing Target |
| :--- | :--- | :--- |
| `OntoMath::clampedWaveSupport` on `[0..3]` | `Field[1]` returns `std::nullopt` (`wantRouted = false`) | **All 4 curtains `[0..3]` succeed** (`volumeRoutedSets == 1`) |
| `checkEvaluatorsDeduplicatable` on `nearCohort [0..3]` | `false` (`sharedG0_1 != sharedG0_0`) | **`true`** (`ecSingleMember` active in `VolumeRoutedTransport.inc`) |
| `volumeDensity` hoisted time slots (`expensiveTimeOnly`) | `0` across all 8 fields (`12–34` per-sample `time.x` reads) | **`> 0` hoisted slots; `0` per-sample `sin`/`cos` of `t`** inside `volumeShared_*_G0` |
| Per-sample trig count in `volumeShared_*_G0` | `[0..3]`: `37–41`, `[4..7]`: `61–105` | **`[0..3]`: `<= 18`**, **`[4..7]`: `<= 24`** |
| `emitProductWithZeroExits` early-exit before `cnoise3(p)` | Defeated by `air_halo >= 0.04` (`0%` early-exit) | **Active** (`densityFactor == 0.0` exits before `cnoise3(p)`) |
| `VolumeZeroProof` empty cells (`of 4,096`) | `[0..3]`: `82%–84%`, `[4..7]`: `63%–73%` | **`>= 90%` on `[0..3]` and `>= 88–90%` on `[4..7]`** |
| Cold CPU `buildVolumeZeroProof` + cert build (all 8 fields) | `1,351.7 ms` | **`< 350 ms`** |
| `WebGpuRenderer` cohort/cert seams | `366.4 KB` 8-field mega-shader on probe; certs built for routed cohort | **Zero 8-field mega-shader compiles**; routed cohort skips certs; `farCohort` probes independently |
| Existing test suite (`ctest`) | Baseline | **100% pass** (especially `volume_*` and `webgpu_*` tests) |

Reply in this thread with your commit hash, exact before/after numbers for every row above, and any visual/parameter trade-offs made on `Northern Veil`.

— *Aetherion / The Firmament (`Gemini 4 Argon`), 2026-11-18*
