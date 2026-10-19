# Ray-domain transport — Metal verification

Zach handed Codex Claude Opus 5.5's Mac-verification commission. This continues [Opus's sixth-pass handoff](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#continuation--the-gpu-tracer-and-a-bug-it-caught-2026-10-17-sixth-pass) and tests the separate Sun's [local source-review counterexamples (pre-existing, uncommitted)](RAY_DOMAIN_TRANSPORT_SOURCE_REVIEW_2026-10-16.md). Opus authored the implementation and stock witnesses; Codex ran the Mac verification, built the isolated capture entry and supplemental counterexample witness, and prepared the comparisons. No production implementation was changed.

**Checkout:** `claude/ray-domain-transport-rung0`, fetched and verified at `ee905784589ed42626c61c0c2e6fa67c39657ec3`. **Before:** `5bcb63837`, parent of the emitter/certificate implementation commit. No merge, PR, push, or installation. Pre-existing uncommitted source-review notes were preserved.

**Machine:** MacBook Air `Mac17,3`, Apple M5, 16 GB; macOS 26.6, build `25G72`. The supplemental witness directly queried adapter `Apple M5`, backend enum 5 (`WGPUBackendType_Metal`). Ordinary timestamp features remained disabled. The initial sandbox certificate invocation skipped device acquisition; the desktop rerun supplied the results below.

## Executed stock witnesses

Configured the existing Debug tree with the documented CMake policy and vendored OpenSSL paths, then explicitly built all five targets below and `earthcall_webgpu`. Tests ran from `build/` against the frozen checkout fixtures, with desktop GPU access. They ran sequentially; ABBA completed before starting the app build. These are focused results, not a complete-suite verdict.

| Witness | Metal result | Limit |
|---|---|---|
| `webgpu_volume_support_certificates_test` | **56/56**, exit 0 | Includes single/fused, clocks, views, edits, extent changes, full lattice, 2560×1440, and Prism wing B. Stock coverage does not settle the source-review proof gaps. |
| Same, `EARTHCALL_CERTIFICATE_TIMING=1` | **56/56**, exit 0 | Report-only completion/readback-inclusive adapter costs below; no GPU timestamps or movement FPS. |
| `webgpu_volume_zero_proof_test` | **73/73**, exit 0 | Separate proof lane witness. |
| `webgpu_v5_overlap_physics_test` | **PASS**, exit 0 | Closed-form and mixed-source output `(37,0,56)` matched expectation. All five null-participant comparisons were black on both sides, drift 0; they do not settle the authorial quadrature conflict. |
| `webgpu_perlin_exact_gradient_test` | **ABORT**, exit 134 | `Assertion failed: (!sawInitialRangeUpload)`, line 817, after Gate D's downward-camera image comparison. The residency assertion reproduces on Metal, not just lavapipe; root cause was not repaired here. |
| `webgpu_volume_shared_subexpression_test` | **76/76**, exit 0 | The lavapipe `ontoPow(-2, NaN)` failure did not reproduce on this Metal device. |

The stock precedence witness reported **0 bytes differ, max 0**, with 6,592 lit pixels. No check or tolerance was loosened.

## Every report-only TIMING line

```text
TIMING all 4 media (fused set pipeline): exact median 5.03 ms vs certificates median 4.13 ms (ABBA x4, 8 frames each)
TIMING all 4 media (fused set pipeline): grid median 1.93 ms vs grid+certificates median 2.87 ms (ABBA x4, 8 frames each)
TIMING one medium (single pipeline): exact median 0.88 ms vs certificates median 0.79 ms (ABBA x4, 8 frames each)
TIMING one medium (single pipeline): grid median 0.50 ms vs grid+certificates median 0.85 ms (ABBA x4, 8 frames each)
TIMING prism.wingB.spherical-cloud: grid median 0.74 ms vs grid+certificates median 0.35 ms
TIMING prism.wingB.hollow-shell: grid median 0.80 ms vs grid+certificates median 0.46 ms
TIMING prism.wingB.torus-medium: grid median 0.90 ms vs grid+certificates median 0.40 ms
TIMING prism.wingB.csg-crescent-medium: grid median 0.84 ms vs grid+certificates median 0.45 ms
TIMING prism.wingB.noise-organic-cloud: grid median 1.27 ms vs grid+certificates median 0.92 ms
TIMING prism.wingB.time-breathing-nebula: grid median 0.80 ms vs grid+certificates median 0.50 ms
TIMING prism.wingB.proxy-proof-torus: grid median 0.75 ms vs grid+certificates median 0.18 ms
TIMING prism.wingB.dual-sovereign-being: grid median 0.82 ms vs grid+certificates median 0.42 ms
TIMING Prism wing B fused (8 media): grid median 2.86 ms vs grid+certificates median 1.25 ms
```

The separate single-frame 2560×1440 exact/certificate observations were 198.9/98.6 ms in the first witness and 227.4/105.0 ms in the timing witness; these are not TIMING medians. The production comparison is **grid versus grid + certificates**: fused Northern Veil became about 49% dearer, its selected single medium about 70% dearer; Prism wing B became about 56% cheaper. This supports researching generic per-medium eligibility after correctness qualification, not enabling the lane globally or claiming a Northern Veil movement improvement. Proposed policy work is in the [task](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Ray_Domain_Transport_Compilation/Ray_Domain_Transport_Compilation.md#metal-economics-and-eligibility-proposal--october-17).

## Supplemental source-review witness

The local `adversarial-witness.cpp` reuses the stock witness's real renderer, texture readback and byte comparison, but supplies the review's original Add/Sub/Scale/min/clamp MathNode trees. It does not precombine them into a ScalarLeaf. It runs single/fused pipelines with the grid off/on and includes a distinct null participant and a positive constant-density emitting control.

**CPU defect reproduced:** for `D=-(((2^54+x)-2^54)-x)`, the retained Piecewise evaluator returned **D(1)=1**, while `planVolumeRaySegment` returned **exact-empty**, reason `every member's density is exactly constant and non-positive on this ray`. This is an executed counterexample to the planner's admission premise. This planner still has no production renderer consumer.

**Proposed GPU manifestations not reproduced on this M5:** the canceled-min wrapper and computed clamp bound produced black reference frames and byte-identical certificate frames in all eight combinations. Their headers nevertheless admitted the questionable premises: canceled min had mathematical L=0/magnitude=0; clamp used CPU-folded offset 1. The positive control had 24,649 lit pixels and matching images in all four combinations. Thus **12/12 native comparisons** passed, including four nonblack controls; the actual reason the two candidate fields rendered black was not independently characterized. These observations do not prove equivalence for other backend arithmetic, do not discharge the source deductions, and do not turn the declared device tolerance into a theorem.

## Native Engine captures for Zach

Built the full app at both heads. The parent tree used an exact `git archive` of its `src/` and CMake file, unchanged shared foreign dependencies, Debug with debug symbols omitted for disk economy. Each capture executable replaces only `entry.cpp` with the same scratch entry and links its own build's production Engine, renderers and recorder. These are actual native Engine viewport snapshots, not mockups, reconstructed scenes, or only an offscreen volume fixture.

Both runs used byte-copied frozen Zone files, Sky-Island's five Law roots and eleven Material roots, and the existing legacy `saves/persons/Zach.ecform` profile. No identity keys or signed presence were supplied. Authorship metadata stayed as recorded in the copies; no agent-authored beings were injected into the original world. The Engine's incidental writes were confined to `/private/tmp/earthcall-metal-01a148a7-captures/{before,after}/saves`. The original three Zone files were hash-checked afterward. Neither checkout saves nor live data-home saves were written.

The first fixture attempts lacked Sky-Island's referenced closure and were correctly refused; those failed attempts are not captures of Sky-Island. The successful captures use the complete closure. Direct and frame-synchronized reads initially refused unavailable frames; a scratch-only renderer diagnostic observed native **Occluded = 0x30001**. Floating the temporary window and requesting snapshots on candidate frames solved acquisition. Final captures link the unchanged production renderer, without that diagnostic. The computer-use foreground attempt timed out; no image was fabricated from its failure.

**30 unaltered PNGs, 1920×1080: 15 matched pairs**, at world clocks 0, 0.75, 2.25 with `dt=0` and certificates off. Camera positions and targets:

| View | Eye → target |
|---|---|
| Prism light region | `(0,4,166)` → `(0,5,190)` |
| Prism breathing-nebula diagnostic | `(99,8,298)` → `(95,4,285)` |
| Radiance near station | `(-15,4,3)` → `(10,4,0)` |
| Radiance far station | `(-15,4,171)` → `(10,4,175)` |
| Sky-Island | `(22,12,26)` → `(0,2,0)` |

The light region, far Gallery and Sky-Island exhibit image differences. The near Gallery is heavily saturated and masks most of the change. The selected nebula view is byte-identical at all three clocks, so it does not witness a changed breathing silhouette. It also does not cover every other breathing field. The compiler now groups the authored proportional modulation; whether these appearances fulfill Zach's intention remains **Person verification**, as does temporal appearance between the sampled clocks.

**Local evidence directory:** `scratch/verification/ray-domain-metal-2026-10-17-01a148a7/`. [Side-by-side gallery](../../scratch/verification/ray-domain-metal-2026-10-17-01a148a7/index.html); PNGs under `before/` and `after/`; exact counts in `image-comparison.json`; stock logs, adapter/counterexample logs, `capture.cpp`, `capture-build.py`, `adversarial-witness.cpp`, compile/link records and source manifest alongside. This generated evidence is retained locally and ignored by Git; the counts, findings and timing text in this audit are committed. The gallery is also served locally at `http://127.0.0.1:8843/index.html` while its server runs. Its side-by-side presentation was inspected in the in-app browser.

## Remaining gates

Keep the certificate lane **default off**. No optional live-property trial was requested or performed. Correct the execution-zero admission/provenance issue and qualify computed bounds/device error before promotion; investigate the reproduced Perlin cache assertion separately. A generic cost policy needs its own measurements and authored allocation decision. Zach alone checks the three Zone appearance boxes. Ordinary native timestamps stay off; live 30 FPS, motion smoothness, unshown breathing members, device reconstruction with certificates enabled, and a complete device arithmetic enclosure remain open.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T00:18:36-07:00 (PDT).


## Zach's fidelity criterion and Sky-Island follow-up

Zach prefers the before lighting aesthetically but accepts the after change only if it follows the authored equations more closely. [Independent 80-digit/native field comparison and old-renderer grouping reference](SKY_SANCTUARY_AUTHORED_EQUATION_FIDELITY_2026-10-17.md) support the darker Sky-Island result: sun field error falls from 0.6688 to 9.61e-8, and the old renderer with explicit Add trees matches all three after captures byte-for-byte. This is bounded evidence for the grouping/shading change, not a complete exact-renderer certification. No aesthetic retuning or silent Person checkbox acceptance. **Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T00:37:58-07:00 (PDT).


## Post-repair Metal revalidation — Opus seventh pass

Zach relayed Opus's return and requested continuation after the merge. Rebuilt all three requested native targets from merged HEAD `a597da4cd` (including `09143a5df` execution provenance and `94554ae93` range residency), then ran them sequentially with desktop GPU access on the same Apple M5 Mac. Existing Debug configuration; default test modes, no timing mode or timestamp requests. No production source, test, save, lighting coefficient, installed app or default was changed.

| Witness | Result | Observed scope |
|---|---|---|
| `webgpu_volume_support_certificates_test` | **57/57, exit 0** | Single/fused, edits/extents, warm residency, full lattice, Prism and 2560×1440 comparisons pass. Precedence witness: 6,592 lit pixels, 0 differing bytes. |
| `webgpu_volume_zero_proof_test` | **73/73, exit 0** | Includes density/emission edits, mixed membership, full lattice, device replacement and 2560×1440. |
| `webgpu_perlin_exact_gradient_test` | **ALL OK, exit 0** | Gates A–D execute; residency assertion passes; traversal active 6/6 cameras, identical OFF/ON hit coverage and sampled CPU root agreement, 0 reported GPU validation errors. |

**Two important limits.** The added cancellation case reports **0 lit pixels**, 1 killer and 0 differing bytes on this M5. The witness asserts RGBA equality but does not require the cancellation frame to be nonblack. Thus it passes here without reproducing lavapipe's 29,388-lit-pixel cancellation manifestation; do not cite it as native positive-density cancellation coverage.

Perlin's “45 degrees down” camera reports **27 differing RGBA bytes** despite identical hit coverage; all other five cameras report 0. The stock witness asserts coverage and root agreement, while color-byte differences are diagnostic only. The prior pre-repair run aborted before this camera, so these results alone cannot classify the color difference as a regression. The residency fix is verified within this witness; full RGBA parity is not established. No assertions or tolerances were changed.

Raw logs and exit/duration metadata: `scratch/verification/ray-domain-metal-2026-10-17-01a148a7/repair-revalidation/{webgpu_*_test.log,results.json}`. Elapsed test durations were 40.180 / 25.588 / 6.432 seconds; these are test execution durations, not renderer performance measurements. No ABBA rerun or CPU-census rerun was performed.

Certificates and ordinary timestamps stay default off. The declared execution-error model is still not a complete Metal Fast-mode enclosure. Generic authored eligibility, device proof, the null-member AUTHOR decision and existing Person appearance/movement checks remain open. This pass verifies the three requested native witnesses, not the entire renderer or a new FPS result.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T12:12:06-07:00 (PDT).


## Auto eligibility Metal verification — Opus eighth pass

Zach relayed Opus's `993f92a6` handoff, asking for continuation of the native commission. The main checkout had moved to `sync-from-earthcall-main`, so Codex used the existing clean ray-domain worktree at `993f92a6d5619359c59b9043f0a52a10671b62bd`. Refreshed that worktree's stale Debug CMake inventory using the documented policy/OpenSSL flags; rebuilt `webgpu_volume_support_certificates_test` and `channel_paths_test` with assertions enabled (`-UNDEBUG`). No source, test, default, authored coefficient, original save or installed app changed. The main checkout stayed untouched. The scratch probe directly reports **backend 5 (Metal), Apple M5**. Ordinary timestamps remained disabled.

**Executed:** channel paths reports **547 advertised paths resolved, 3 skipped (clock), 0 failed; ALL OK**. Two independent, sequential process invocations of `EARTHCALL_CERTIFICATE_TIMING=1 ./webgpu_volume_support_certificates_test` each pass **68/68**, exits 0, in 52.512 and 46.598 seconds. These are focused witnesses, not a full-suite verdict. Native auto-mode parity/timings use **321×181**; the stock **2560×1440** stress comparison passes with forced-all certificates, not auto mode. No ABBA run overlaps another test/build. Timing batches warm first, alternate grid/auto then auto/grid over four rounds, eight submitted frames per batch, and report median completion/readback-inclusive cost per frame. They are not isolated GPU timestamps, live FPS, or usual-framebuffer movement measurements.

**Cancellation:** the simple reassociable case remains black. The new polynomial identity at A=4096 produces **36,669 lit pixels**, 1 killer, and **0 differing bytes** in both Metal runs. This supplies the previously missing nonblack native case for that authored expression. It does not prove any compiler universally preserves the expression or establish a complete device error enclosure.

**Auto selection:** Northern Veil reports **0/4** at clock 7.25; Prism wing B **7/8** at clock 0.75, excluding the noise cloud. Every stock auto-mode comparison matches the grid lane byte for byte. Both frozen fixture hashes (Northern Veil ecform, Prism JSON) are unchanged.

| Auto-mode median cost, ms | Series 1 grid → auto | Series 2 grid → auto |
|---|---|---|
| Northern Veil fused | 2.95 → 3.49 | 2.64 → 3.24 |
| Prism sphere | 0.58 → 0.30 | 0.55 → 0.29 |
| Prism shell | 0.55 → 0.34 | 0.82 → 0.50 |
| Prism torus | 2.85 → 1.14 | 1.02 → 0.49 |
| Prism crescent | 0.92 → 0.50 | 0.84 → 0.48 |
| Prism breathing nebula | 0.66 → 0.37 | 0.84 → 0.53 |
| Prism proxy torus | 0.82 → 0.21 | 0.78 → 0.21 |
| Prism dual medium | 0.79 → 0.44 | 1.15 → 0.68 |

The seven admitted individual Prism media improve **1.58–3.90×** across these paired medians. Absolute costs drift across series, particularly the torus, so do not pool unrelated low/high observations. Northern Veil instead costs **18.3% and 22.7% more** with auto mode. The cloud's “no penalty” conclusion is not established on this Metal device. Forced-all (not auto) Prism fused comparisons also improve, 2.31→0.96 and 1.72→0.69 ms, but no auto-mode fused-Prism timing is supplied by this stock witness.

**Northern Veil diagnostic:** the stock test asserts eligibility at clock 7.25 but times it at 0.75. A scratch-only entry retained its exact renderer/readback helper, linked the same freshly built objects, and checked both clocks. Both are lit, byte-identical off/on, with **0 eligible media and 0 killers**. Every timed batch reports **0 warm compiles and 0 parameter uploads**. Separate per-batch ABBA medians are 2.3012945→2.918323 ms at 0.75 (+26.8%), and 2.3306825→2.7031175 ms at 7.25 (+16.0%). Thus the clock mismatch does not explain away the overhead. These instrumented batches corroborate the stock finding rather than replace its medians.

**Concrete source lead, not a proved cost attribution:** `WebGpuRenderer::flushVolumeComposite` still keys/composes the certificate shader using the master `volumeSupportCertificatesEnabled()` value (including `composeVolumeSet`) even when auto filtering clears every certificate payload. A zero eligible count therefore does not select the original no-certificate shader. Next investigate a generic effective-eligibility program identity / baseline fast path for an empty eligible set, with coherent memo/payload invalidation and renewed paired pixels/costs; no Zone names or authored-equation changes. No such optimization was implemented in this verification pass.

**Other bounds:** Opus's pre-fix Perlin comparison was on lavapipe (18 diagnostic bytes). It does not independently establish that the M5's 27-byte difference was present before the residency fix; native baseline attribution remains open. The fixed Metal residency assertion and coverage results from the prior pass remain valid.

Certificates stay **default off**. Person live-trial/visual/movement acceptance, the null-member decision, shared-noise estimate modeling, full device enclosure, and zero-eligible Metal overhead remain open. The new positive-density cancellation witness is complete on M5; the performance gate for neutral Northern Veil auto mode is not.

**Raw evidence:** `scratch/verification/ray-domain-metal-2026-10-17-01a148a7/auto-eligibility/` retains build log, two full stock logs, all 42 TIMING lines, channel log, exact source HEAD, commands, fixture hashes and result metadata. `northern-veil-clock-probe.cpp`, its compile/link argv, build log and full per-batch log retain the scratch diagnostic. Original copy also remains at `/private/tmp/earthcall-auto-metal-01a148a7-20261017/`.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T19:22:42-07:00 (PDT).


## Effective eligibility Metal verification — Opus ninth pass

Zach relayed Opus's per-member shader-selection return. Fetched and fast-forwarded only the clean ray-domain worktree to `4c17e91502b4034d7a8f8531d0087f0273d415e5`; the main checkout and its unrelated ongoing work remained untouched. Rebuilt four focused native targets in the existing Debug tree with assertions enabled. No production/test source, saves, lighting, defaults or installed app changed during verification. Scratch controls query **Metal backend 5 / Apple M5**. Ordinary timestamps remain disabled.

### Stock witnesses and paired cost

Two sequential timing-enabled certificate runs pass **69/69**, exits 0 (42.998 and 42.014 seconds). Both report **0 new program compiles** after enabling auto mode for zero-eligible Northern Veil, **36,669 lit pixels** for the polynomial cancellation witness and **0 differing bytes** there. All finite parity/edit/full-lattice checks pass, including the stock 2560×1440 forced-all comparison. Separate native witnesses pass: **zero-proof 73/73**, **unified quadrature 15/15**, and **Perlin ALL OK** with its residency assertion intact and traversal active in 6/6 cameras. These are focused results, not the full suite.

Stock auto timing is 321×181, eight submitted frames per warmed batch, four alternating grid/auto and auto/grid rounds, median completion/readback-inclusive cost per frame. Runs/builds were sequential; no native timestamps, usual-framebuffer movement or live FPS was measured.

| Median cost, ms | Series 1 grid → auto | Series 2 grid → auto |
|---|---|---|
| Northern Veil fused | 2.09 → 2.09 | 1.86 → 2.01 |
| spherical-cloud | 0.72 → 0.36 | 0.74 → 0.36 |
| hollow-shell | 0.83 → 0.51 | 0.85 → 0.47 |
| torus-medium | 1.83 → 0.78 | 0.94 → 0.43 |
| csg-crescent-medium | 0.89 → 0.47 | 0.85 → 0.48 |
| time-breathing-nebula | 0.85 → 0.51 | 0.82 → 0.52 |
| proxy-proof-torus | 0.80 → 0.21 | 0.82 → 0.20 |
| dual-sovereign-being | 0.74 → 0.42 | 1.17 → 0.65 |

The seven selected Prism individual media remain cheaper in both series, **1.58–4.10×** across the paired medians. The noise cloud remains excluded. Northern Veil's first short series is neutral; its second shows +8.1%, so short medians alone do not establish literally zero overhead. They also must not be compared by absolute cost against yesterday's/different-process lows.

### Longer Northern Veil batches

To investigate that short-run residual, a scratch-only entry retained the same stock renderer/readback helper, used 128 submitted frames per batch, and reported CPU volume-preparation averages and warm counters. Two independent process runs each measured clocks 0.75 and 7.25 in alternating AB/BA order. Both clocks remain lit and byte-identical off/on, with **0 eligible media / killers**. Every warm batch reports **0 compiles, uploads, certificate builds and zero-proof builds**.

| Probe / clock | Grid median ms | Auto median ms | Relative change |
|---|---:|---:|---:|
| 1 / 0.75 | 1.3842185 | 1.3863645 | +0.16% |
| 1 / 7.25 | 1.4177710 | 1.3889410 | −2.03% |
| 2 / 0.75 | 1.5002395 | 1.4645215 | −2.38% |
| 2 / 7.25 | 1.5106975 | 1.4711615 | −2.62% |

The earlier repeatable 18–23% penalty is not reproduced in these longer comparisons; the measured changes span −2.62% to +0.16%. Median CPU volume-preparation time rises from about 0.0072–0.0073 ms to 0.0084–0.0086 ms, roughly **1.2 microseconds per frame** in this four-medium fixture. That supports small remaining host bookkeeping, not a claim that every cost is mathematically zero. The 128-frame medians amortize readback differently and are not interchangeable with the stock eight-frame medians.

Source inspection confirms the effective certificate boolean enters each member's program key/structure/composition before payload attachment. With no eligible members the baseline variant is reused. Together with measured neutral longer-batch costs, this verifies the intended removal on this M5. **Register pressure/occupancy remains an explanation hypothesis:** no GPU disassembly or hardware register/occupancy counter was measured, so the exact driver mechanism is not established.

### Perlin mirror-fix baseline, native raw bytes

Built two throwaway controls from the current source and same freshly compiled objects. Both use the same copied stock test plus raw-readback capture and bypass **only** `assert(!sawInitialRangeUpload)`. One uses the current renderer; the other restores exactly the historical `else if (rangeChanged) persistentRange.mirror.clear()` behavior from before `94554ae93` in a copied renderer. Every other assertion remains enabled; the unchanged stock Perlin test separately passes with the residency assertion intact. This isolates the mirror fix, not every unrelated change since the historical commit.

The initial `EARTHCALL_PERLIN_TEST_MODE=D` controls exited 0 but executed no camera corpus and produced no readbacks. The wrapper rejected them for missing captures; they are explicitly marked **invalid evidence**. Repeating in full mode executes Gates A–D and yields the required 12 raw buffers per control: OFF and ON images for six cameras, 128×128 RGBA8.

**All 12 corresponding current/old-mirror buffers match byte for byte (0 cross-version differing bytes; SHA-256s recorded).** Within each control, the 45-degree OFF/ON comparison has the same 27 differing color bytes; the other five have zero, and hit coverage agrees in all six. Thus those 27 bytes predate the residency-behavior correction on this Metal device; the correction did not introduce them. This does not establish full OFF/ON color parity or prove that the remaining traversal difference is mathematically exact.

**Remaining:** fix or explicitly refuse the no-op Perlin D-only mode; full device arithmetic enclosure, shared-noise cost modeling, the null-member AUTHOR decision and existing Person visual/movement acceptance remain open. Certificates remain **default off**. No live trial, installation or save mutation occurred; the two frozen fixture hashes are unchanged.

**Evidence:** `scratch/verification/ray-domain-metal-2026-10-17-01a148a7/effective-eligibility/` retains all stock/probe logs, all TIMING lines, source HEAD, fixture hashes, raw Perlin buffers, source diffs, and exact scratch compile/link argv. Large throwaway executables/objects remain only in the original `/private/tmp/earthcall-member-lane-metal-01a148a7-20261017/`.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T19:56:43-07:00 (PDT).


## Eligibility ray set, shared prelude and Perlin mode — tenth pass (M5)

Zach asked the local session to do the items Opus listed as open. Built in the clean ray-domain worktree, on a fresh Debug tree from `a67643b03` plus this pass's changes. Native Apple M5 Metal throughout, with ordinary timestamps disabled. No save, default or installed app changed.

| Witness | Result |
|---|---|
| `webgpu_volume_support_certificates_test` (timing on) | **69/69** twice; then **72/72** with the new auto-mode invalidation witness |
| `webgpu_volume_zero_proof_test` | **73/73** |
| `webgpu_perlin_exact_gradient_test` | full mode ALL OK (Gates A–D); `ABC` ALL OK (A–C); `D` ALL OK (Gate D only, 6 cameras, 45° still 27 bytes); `bogus` exits 1 |
| `volume_ray_radial_segmentation_test` (CPU) | **32/32**; 17 of 28 media eligible (was 13) |

**Auto-mode costs (321×181, 8 frames, ABBA ×4 medians, completion/readback-inclusive; grid → grid+auto, ms):**

| Medium | Run 1 | Run 2 |
|---|---|---|
| Northern Veil fused (0 eligible, 0 compiles) | 3.03 → 3.06 | 3.19 → 3.45 |
| spherical-cloud | 0.92 → 0.46 | 0.76 → 0.40 |
| hollow-shell | 0.94 → 0.53 | 0.90 → 0.54 |
| torus-medium | 1.03 → 0.49 | 0.99 → 0.49 |
| csg-crescent-medium | 0.66 → 0.39 | 1.03 → 0.58 |
| **noise-organic-cloud (newly eligible)** | **1.66 → 1.21** | **1.44 → 1.10** |
| time-breathing-nebula | 1.13 → 0.77 | 0.94 → 0.60 |
| proxy-proof-torus | 0.89 → 0.30 | 0.72 → 0.29 |
| dual-sovereign-being | 1.11 → 0.63 | 1.08 → 0.54 |

All 8 Prism wing-B media now get certificates in auto mode, and all keep byte parity. Northern Veil runs the identical lane-off program, so its spread is run-to-run noise. Absolute times this session ran above Codex's (another process load; run 1's forced "Prism wing B fused" grid median of 22.61 ms is an outlier). Every TIMING line from both runs is in `cert-timing-1.log` and `cert-timing-2.log`.

**Findings.**
- The shared-noise prelude did not cause the noise cloud's miss: its density reads no shared group. The estimate's box-centred ray set did. Lines uniform over the box silhouette rank it 0.70, against 0.72 measured.
- The measured idle-variant cost (×1.2) keeps Northern Veil ineligible under the new ray set.
- Borealis v1 ×2 and Sky-Island sea-of-clouds are newly predicted eligible and **not yet measured** on Metal.

**Evidence:** `scratch/verification/ray-domain-tenth-pass-2026-10-18/`. Method and design: [Rung 2 plan §6i](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6i-what-the-noise-clouds-miss-actually-was-and-the-perlin-d-mode-2026-10-18-tenth-pass). No live FPS, movement or Person-acceptance claim.

**Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T01:20 PDT.

## Enabled-killer compilation, killer subsets and the unmeasured promotions — eleventh pass (M5)

Zach asked the local session to keep going on the next pass and to answer the Firmament's eleventh-pass counsel. Built in the `ourverse_navigator_prototype_loop` worktree, which holds `claude/ray-domain-transport-rung0`, in a fresh Release tree (`build-ray`) from `9967cb2cf` plus this pass. Native Apple M5 Metal throughout, with ordinary timestamps disabled. WindowServer held about 40% CPU and grid baselines drifted up to 2× between runs, so only within-run ABBA pairs are compared. No save, default or installed app changed.

| Witness (final tree) | Result |
|---|---|
| `webgpu_volume_support_certificates_test` (timing on) | **83/83** in each of two final runs (new: per-curtain Northern Veil auto parity, Borealis and Sky-Island auto parity and timing) |
| `volume_ray_radial_segmentation_test` (CPU) | **33/33**; 17 of 28 eligible (unchanged); 4 compile fewer killers than they certify |
| `webgpu_volume_zero_proof_test` / `webgpu_volume_shared_subexpression_test` / `webgpu_volume_unified_quadrature_test` / `webgpu_volumetric_mist_test` | 73/73 / 76/76 / 15/15 / PASS |
| `webgpu_perlin_exact_gradient_test` | ALL OK (Gates A–D) |
| `volume_ray_segment_transport_test` / `volume_shared_subexpression_test` / `volume_zero_proof_test` / `sdf_wgsl_parameter_refresh_test` | 53/53 / 60/60 / 48/48 / PASS |
| After the `ScalarForm` fix: `ontomath_line_segmentation_test` / `prophetic_rete_test` / `sdf_range_soundness_test` / `ontomath_authoring_reachability_test` | 64/64 / ALL OK / PASS / PASS; zero-proof cells proven identical on all 28 real media |

**Northern Veil, forced certificates (ms; old = every switch compiled, runs 2–3; new = only certified killers compiled, final runs 1–2):**

| Configuration | Baseline | Old | New |
|---|---|---|---|
| Fused, certificates alone | exact 3.97–4.25 | 2.99 / 3.02 | 2.46 / 2.19 |
| Fused, grid + certificates | grid 1.41–1.44 | 2.38 / 2.43 | 2.10 / 2.12 |
| Single curtain, certificates alone | exact 0.75 | 0.67 / 0.67 | 0.48 / 0.48 |
| Single curtain, grid + certificates | grid 0.30–0.31 | 0.61 / 0.62 | 0.47 / 0.47 |

**The decomposition** (scratch builds that were never committed; grid → variant, ms, one run each):
- *Idle:* the certificate program is attached with count 0. It cost 1–21%: emerald 0.32→0.34, cyan 0.16→0.18, spherical-cloud 0.29→0.35.
- *No jump:* every check runs but no skip is taken. Spherical-cloud went 0.29→0.50, noise cloud 1.15→2.27, emerald 0.32→0.60.
- *Only switches 5 and 8 compiled:* emerald 0.60→0.66, cyan 0.53→0.49. The same subset with all 12 switches compiled had run 1.6–1.75× and 1.47–1.63×.

**Subset-promoted curtains** (rejected as an eligibility rule): with only the subset compiled, emerald still ran 0.31→0.36 and 0.30→0.36, and cyan 0.16→0.16 and 0.17→0.16. In the final tree, Northern Veil auto has 0 eligible and 0 compiles, and runs fused 1.53→1.57 and 1.55→1.59.

**The tenth pass's unmeasured promotions** (auto mode, every byte equal to the grid lane):

| Medium | Side framing, before pruning (3 runs) | Top-down framing, final (2 runs) |
|---|---|---|
| Borealis v1 airy cloud | 1.18→0.73, 1.66→1.04, 1.29→0.84 | 0.99→1.05, 1.12→1.19 |
| Borealis v1 dense cloud | 1.15→0.59, 1.58→0.78, 1.26→0.64 | 0.95→0.94, 1.03→1.03 |
| Borealis v2 emerald-cyan | 1.23→0.55 (run 1) | 1.15→0.87, 1.22→0.93 |
| Borealis v2 violet-magenta | 1.19→0.55 (run 1) | 1.25→0.94, 1.27→0.95 |
| Sky-Island sea-of-clouds | 0.93→1.17, 1.78→2.38, 1.68→2.29 | 2.83→3.03, 2.87→3.09 |
| Sky-Island sunlit-halo-mist | lit nothing (side camera) | 2.52→2.43, 2.53→2.42 |

**Findings.**
- Compiling only the enabled killers is a reproducible win for every certificate configuration, and is retained.
- The program cache gated structure on content revisions only. A forced→auto change of the killer set therefore kept the old program under a re-laid payload: emerald's bytes differed once, and the repair is witnessed.
- Killer subsets do not make Northern Veil pay.
- Sea-of-clouds is a measured false positive (+7% top-down, +26–36% side).
- Borealis v1 gains only from a distance.

**Evidence:** `scratch/verification/ray-domain-eleventh-pass-2026-10-18/`. Method and design: [Rung 2 plan §6j](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6j-compile-only-the-enabled-killers-a-killer-subset-measured-2026-10-18-eleventh-pass). No live FPS, movement or Person-acceptance claim.

**Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T11:46 PDT.

## Measured eligibility (hidden render) — twelfth pass (M5)

Zach chose the hidden render for measured eligibility, "measuring once per necessary interval." Built in the same worktree and `build-ray` tree from `4a0b591c5` plus this pass. Native Apple M5 Metal, with ordinary timestamps disabled (the probe uses none). WindowServer held about 40% CPU. No save, default or installed app changed.

| Witness (final tree) | Result |
|---|---|
| `webgpu_volume_support_certificates_test` (timing on) | **90/90** in each of three final runs (new: measured, held, new-octave, return and rebuild regime witnesses; per-medium verdicts; measuring cost at 321×181 and 2560×1440) |
| `webgpu_volume_zero_proof_test` / `webgpu_volume_shared_subexpression_test` / `webgpu_volume_unified_quadrature_test` / `webgpu_volumetric_mist_test` / `webgpu_perlin_exact_gradient_test` | 73/73 / 76/76 / 15/15 / PASS / ALL OK (A–D) |
| `volume_zero_proof_test` / `volume_shared_subexpression_test` / `volume_ray_segment_transport_test` / `volume_ray_radial_segmentation_test` | 48/48 / 60/60 / 53/53 / 33/33 |
| `sdf_range_soundness_test` / `ontomath_authoring_reachability_test` / `prophetic_rete_test` | PASS / PASS / ALL OK |

**Verdicts, three runs** (5 ms pair budget; K = kept, V = vetoed):

| Medium | Verdicts | Hand ABBA (§6j) |
|---|---|---|
| Prism wing B ×8 | K K K (also K at 2560×1440) | 2–4× faster |
| Borealis v2 emerald-cyan, violet-magenta | K K K | −24–29% |
| Borealis v1 dense | K K K | even top-down |
| Borealis v1 airy | K V K | even top-down |
| Sky-Island sea-of-clouds | K V V | +7% |
| Sky-Island sunlit-halo-mist | K K K | −4% |

**Cost while measuring, 2560×1440, per frame for five frames** (settled → measuring, ms; three runs):
- spherical-cloud 2.6→5.6, 2.7→6.0, 2.8→5.6
- torus 5.2→9.3, 6.1→10.7, 6.0→10.0
- noise-organic 42.6→48.1, 44.6→49.8, 44.1→48.4
- proxy-proof-torus 1.7→5.4, 1.8→5.7, 1.9→5.8

**Rejected along the way:**
- Full-footprint pairs: up to +80 ms per frame for five frames.
- One centred band: vetoed noise-organic-cloud, which is kept in settled timing at 41 against 61 ms.
- A 3 ms budget: vetoed csg-crescent and violet-magenta at 321×181.

**Found:** a hidden lane sharing the frame's number made the resident payload fall back to the frame stream. That shares one buffer with the instances under the preparation dispatch, and wgpu raised a validation panic. Fixed with a per-submission payload epoch. The fallback remains reachable on a failed buffer creation.

**Evidence:** `scratch/verification/ray-domain-twelfth-pass-2026-10-18/`. Design: [Rung 2 plan §6k](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6k-measured-eligibility-the-device-may-veto-the-estimate-2026-10-18-twelfth-pass). No live FPS, movement or Person-acceptance claim.

**Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T15:23 PDT.

## Rung 2b closed-form killers — thirteenth pass (M5)

Zach: "NEXT PASS". This is the Firmament's direction 4. Built in the same worktree and `build-ray` tree from `319628733` plus this pass. Native Apple M5 Metal, with ordinary timestamps disabled. No save, default or installed app changed.

| Witness (final tree) | Result |
|---|---|
| `webgpu_volume_support_certificates_test` (timing on) | **116/116** (new: inside, grazing and 150-radii byte parity on 6 closed-form Prism media; 2560×1440 parity on each) |
| `volume_ray_radial_segmentation_test` | **36/36**; 48 closed forms on 23 media; 0 claims unbacked by the authored switch; 0 certified samples with positive density |
| `webgpu_volume_zero_proof_test` / `webgpu_volume_shared_subexpression_test` / `webgpu_volume_unified_quadrature_test` / `webgpu_volumetric_mist_test` / `webgpu_v5_overlap_physics_test` | 73/73 / 76/76 / 15/15 / PASS / PASS |
| `webgpu_perlin_exact_gradient_test` / `sdf_wgsl_parameter_refresh_test` / `channel_paths_test` | ALL OK (A–D) / PASS / ALL OK |
| `volume_zero_proof_test` / `volume_shared_subexpression_test` / `volume_ray_segment_transport_test` / `ontomath_line_segmentation_test` | 48/48 / 60/60 / 53/53 / 64/64 |
| `sdf_range_soundness_test` / `ontomath_authoring_reachability_test` / `prophetic_rete_test` | PASS / PASS / ALL OK |

**Closed form against sphere tracing**, 2560×1440, forced grid + certificates, ABBA within one run, bytes equal across lanes (ms; two runs):
- spherical-cloud: 2.68 → 1.86, 2.88 → 2.04
- hollow-shell: 4.46 → 2.96, 4.79 → 3.21
- csg-crescent: 3.06 → 2.00, 3.25 → 2.25
- dual-sovereign: 3.51 → 2.37, 3.58 → 2.52
- noise-organic-cloud: 39.9 → 34.0, 41.4 → 35.1

**Auto mode with the measured veto** (2560×1440, settled ms, against grid): spherical 1.77 / 13.1, hollow 3.00 / 15.7, csg 1.89 / 15.7, noise-organic 34.6 / 60.4, time-breathing 5.77 / 15.4, dual 2.33 / 16.7. Every Prism medium was kept. Sea-of-clouds was vetoed. Northern Veil has 0 eligible.

**Corrected:**
- A scratch A/B's −91% on noise-organic came from a stale payload under a toggle production cannot make. The memo `generation` now joins the payload identity regardless.
- The census caught a residual abs marked exact. Fixed before any native run used it.

**Evidence:** `scratch/verification/ray-domain-twelfth-pass-2026-10-18/` (`census-closed-*`, `closed-ab-fixed-*`, `2b-*`). Design: [Rung 2 plan §6l](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6l-rung-2b-closed-form-killers-solved-once-per-ray-2026-10-18-thirteenth-pass). No live FPS, movement or Person-acceptance claim.

**Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T16:40 PDT.

## Nested-norm closed forms and the payload fallback — fourteenth pass (M5)

Zach: "KEEP GOING". Built from `999c7fe1e` plus this pass, on native Apple M5 Metal with ordinary timestamps disabled. No save, default or installed app changed.

| Witness (final tree) | Result |
|---|---|
| `webgpu_volume_support_certificates_test` (timing on) | **124/124** (new: both tori's inside, grazing, 150-radii and 2560×1440 parity) |
| `volume_ray_radial_segmentation_test` | **36/36**; 54 closed forms on 26 media, certifying 46.4% of samples; 0 unbacked claims; 0 positive certified samples |
| The 15 other suites of §6l's sweep | all pass, unchanged |
| Forced payload fallback (probe epoch removed by hand) | 116/116, no wgpu validation panic (previously a panic) |

**Tori,** 2560×1440 within-run A/B, bytes equal (ms; two runs):
- torus-medium: 4.73 → 3.17, 4.91 → 3.37
- proxy-proof-torus: 1.52 → 0.74, 1.56 → 0.73

**Auto mode** (settled ms, against grid): torus 3.48 / 37.5, proxy-proof-torus 0.61 / 23.3, spherical 1.65 / 12.4, hollow 2.79 / 14.4, csg 1.68 / 14.8, noise 32.3 / 57.9, breathing 4.98 / 14.3, dual 2.07 / 14.7. Every Prism medium was kept. Sea-of-clouds was kept this run, a near-tie as in §6k.

**Evidence:** `scratch/verification/ray-domain-twelfth-pass-2026-10-18/` (`census-forms-1`, `forms-*`, `torus-ab-*`, `fallback-forced`). Design: [Rung 2 plan §6m](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6m-nested-norms-tori-and-the-payload-fallback-2026-10-18-fourteenth-pass). No live FPS, movement or Person-acceptance claim.

**Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T16:51 PDT.

## Per-killer check choice — fifteenth pass (M5)

Built from `6da67683e` plus this pass, on native M5 Metal. No save, default or installed app changed.

| Witness | Result |
|---|---|
| `webgpu_volume_support_certificates_test` (timing on, two runs) | **124/124** each |
| `volume_ray_radial_segmentation_test` | **36/36**; the selection drops the check of 3 bounded killers (the tori) |
| The 15 other suites of §6l's sweep | all pass |

**Scratch A/B,** closed form + check → closed form only (2560×1440, ms; two runs, bytes equal):
- torus: 3.22 → 2.63, 3.31 → 2.66
- breathing: 4.62 → 3.97, 4.93 → 4.26
- proxy-proof-torus: 0.70 → 0.65, 0.74 → 0.69
- noise-organic: 26.5 → 27.6, 29.6 → 31.8 (the check pays)

The estimate's choices match the M5 on the tori and the noise cloud, and miss on the breathing nebula, where it keeps the check. Auto settled: torus 2.77 / 2.93, proxy-proof-torus 0.56 / 0.56.

**Evidence:** `scratch/verification/ray-domain-twelfth-pass-2026-10-18/` (`closedonly-ab-*`, `census-checkdrop`, `drop-*`). Design: [§6n](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6n-whether-a-bounded-closed-form-keeps-its-per-sample-check-2026-10-18-fifteenth-pass). **Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T17:41 PDT.

## Device margin witness — sixteenth pass (M5)

Built from `42e4195c8` plus this pass, on native M5 Metal. A new test, `webgpu_volume_support_enclosure_test`: every certified killer of the corpus, run by its own lowered production code at 4096 random box points × 4 clocks, in buckets |t| ≤ 1 and ≤ 1024, against CPU evaluation of the authored operands.

| Result | Value |
|---|---|
| Killer–bucket pairs measured | 160 of 160 |
| Worst device error / `slack(g)` | **0.0016** (proxy-proof torus, |t| ≤ 1) |
| Median | 0.0008 |
| Noise killers | about 0.0001 (device Perlin within about 10⁻⁶ of the CPU) |
| Breathing nebula, |t| ≤ 1024 | error 1.6×10⁻⁴ against `slack(g)` 0.18 and δ 0.37 |
| Checks | 4/4 (no evaluation beyond δ; every killer within its slack) |

**Reading.** On this adapter the declared model is about 600–1000× generous. At large clocks its conservatism, not the geometry, limits what certificates can prove. This is a sampled margin, not an enclosure proof: compute context, one device, and the density's executed switch not sampled separately. Whether to tighten the declared model, or change the lane's default, is Zach's decision (⚑).

**Evidence:** `scratch/verification/ray-domain-twelfth-pass-2026-10-18/enclosure-2.log`. Design: [§6o](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6o-device-margin-witness-how-much-of-the-execution-slack-the-m5-uses-2026-10-18-sixteenth-pass). **Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T18:00 PDT.

## Faithfulness baseline — seventeenth pass (M5)

Zach: "Faithfulness to exact math should be the rule." A new witness, `webgpu_volume_faithfulness_test`, measures each real medium's production density as the M5 executes it (its own composed program, shared fill and clock preparation) against a double-precision authored reference, at 64 rays × 96 lattice samples × 2 clocks.

| Result | Value |
|---|---|
| Media measured | 31 of 32 (`reliquary.shaft-beam`: production program refused, emission authored as a scalar) |
| Worst error / density scale | **1.2×10⁻⁵** (`veil.dust`, absolute 1.8×10⁻⁹); most about 10⁻⁶ |
| Noisy media (Borealis ×5, noise cloud, Northern Veil ×4, sea-of-clouds, `veil.reflection`) | 5.6×10⁻⁷ – 6.7×10⁻⁶ of scale |
| Reference noise vs float `glm::perlin` | 1.4×10⁻⁶ (100 000 points) |
| Checks | 3/3 |

**Finding:** `glm::perlin<double>` is not the authored noise. Its lattice hash selects different gradients in some cells, by up to 1.2. A naïve double (or 80-digit) Perlin reference therefore reports 15–24% errors on every noisy medium, as this witness did before the reference kept the hash's f32 definition.

**Evidence:** `scratch/verification/ray-domain-twelfth-pass-2026-10-18/faithfulness-1.log` (naïve reference), `faithfulness-2.log` (corrected). Design: [§6p](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6p-faithfulness-to-exact-math-is-the-rule-the-baseline-and-what-noise-means-2026-10-18-seventeenth-pass). **Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T18:46 PDT.

## Frame arithmetic and quadrature against the continuum — eighteenth pass (M5)

`webgpu_volume_faithfulness_test` (4/4), extended.

- **World frame against medium frame:** bit-identical points and densities at every sample of all 31 media, near (3 radii) and far (300 units) cameras. The same two formulas differ at 91% of coordinates in strict IEEE f32 on the CPU, so Metal fast-math canonicalizes them. No production change.
- **Quadrature:** 96-sample midpoint transmittance against the converged continuum (CPU double, 16 rays per medium). Worst errors: `borealis.hero.auroral-curtain` **3.85/255** (optical depth 31%); Borealis v1/v2 0.96–1.92/255; Prism ≤ 0.22/255; Northern Veil ≤ 0.17/255; the rest < 0.01/255. References converged to ≤ 2×10⁻⁴.

**Reading:** on this corpus the integral, not the arithmetic, is where pixels leave the authored equation. **Evidence:** `scratch/verification/ray-domain-twelfth-pass-2026-10-18/faithfulness-frames-*.log`, `faithfulness-quadrature-1.log`. Design: [§6q](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6q-where-todays-pixels-leave-the-authored-equation-quadrature-not-arithmetic-2026-10-18-eighteenth-pass). **Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T19:08 PDT.

## Movement M0 pose witness — twentieth pass (M5)

- **Rebinding only:** Northern Veil 168 poses and Prism wing B 176 poses, certificates alone and grid+certificates. 0 compiles, 0 lowerings, 0 certificate or proof builds, 0 probes, 0 bytes uploaded, constant killer count (20 and 15).
- **Fresh-bind equality:** 26 of 26 checkpoint images byte-equal to a fresh renderer's at the same pose; every fresh first frame lowered and built (live-counter control).

**Reading:** on the M5, the Screen's ray artifact does not depend on the camera. Auto-mode measured verdicts are pose-keyed by design and not covered. **Evidence:** `scratch/verification/ray-domain-twelfth-pass-2026-10-18/m0-pose-witness-1.log`. Design: [§6s](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6s-movement-rung-m0-a-camera-move-rebinds-it-does-not-relower-2026-10-19-twentieth-pass). **Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-19T11:55 PDT.

## Value-branch segmentation — twenty-first pass (CPU only)

- **Exact optical depth on every ray:** 7 of 28 admitted media (was 6). `reliquary.shaft-beam` becomes exact through polynomial × √Q.
- **Corrected attribution:** Northern Veil, Borealis hero and Sky-Island refuse on noise or on root × wave, not on "branch flipping" as the eighteenth-pass entry implied.
- `ontomath_line_segmentation_test` 70/70: six new analytic checks, all failing against the previous code.

**Not run:** `ontomath_two_direction_witness_test` segfaults at `main` (null active Zone from the booted harness), independent of this diff. `sky_island_sanctuary_test` fails to link (missing WebGPU objects in its target). Neither touches the line algebra. Design: [§6t](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6t-value-branch-segmentation-every-piece-integrates-its-decided-branch-and-the-office-holds-more-products-2026-10-19-twenty-first-pass). **Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-19T13:17 PDT.

## Hybrid integration — twenty-second pass (CPU only)

- **Mean ∫max(D, 0) error per ray at ~96 evaluations,** against the converged sum:
  - Northern Veil curtains 3.7–4.5×10⁻⁶ (plain 96-sample midpoint 4.1–6.6×10⁻³);
  - Borealis hero 1.3×10⁻⁶ (plain 1.6×10⁻³);
  - tori 4.7×10⁻⁸–1.0×10⁻⁶ (plain up to 0.12).
- **Proved radius** holds on every ray of all 19 refused media.
- **Two numerical repairs,** each with a failing-without witness: wave integrals re-centred on their interval, and slow waves (|α|h ≤ 1) integrated by Taylor series.

**Not run:** the same two pre-existing failures as the twenty-first pass. Evidence: `scratch/verification/ray-domain-twelfth-pass-2026-10-18/hybrid-integration-1.log`. Design: [§6u](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6u-hybrid-integration-closed-form-weights-sampled-remainders-and-focus-2026-10-19-twenty-second-pass). **Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-19T14:06 PDT.

## Carlson special functions — twenty-third pass (CPU only)

- **Worst error against precision-stable references:** R_F 2.9 ulp, R_D 2.0, R_J 3.1, R_C 1.3, complex R_F 3.0. Legendre's relation holds to 2.5 ulp.
- **The first-kind two-quadratic reduction:** within 3.8×10⁻¹⁴ on 50 intervals.
- **Found and repaired:** the conjugate-pair branch condition (sweep < π); three R_J cancellation and overflow paths; and two wrong reference sources (mpmath's `elliprj` at 40 digits, and a plain quadrature on a spike).
- No medium is wired to it yet.

**Evidence:** `scratch/verification/ray-domain-twelfth-pass-2026-10-18/carlson-witness-1.log`. Design: [§6v](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6v-the-carlson-rung-exact-special-function-reduction-witnessed-before-it-is-trusted-2026-10-19-twenty-third-pass). **Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-19T14:53 PDT.
