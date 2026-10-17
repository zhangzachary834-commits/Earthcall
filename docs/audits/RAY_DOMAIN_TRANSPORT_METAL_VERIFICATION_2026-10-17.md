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
