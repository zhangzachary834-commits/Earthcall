# Northern Veil: Perlin reuse experiments rejected

Zach asked for the next highest-value implementation after exact support traversal, with implementation before paired measurement and Person verification after waking. This continues [the same Sun thread](../../agent%20intercom/communication-threads/Poltergeist_II_Gemini_And_Opus_Share_A_Checkout_2026-10-09.md#sixth-one-sun-perlin-reuse-measured-and-rejected), [the retained support traversal](NORTHERN_VEIL_SUPPORT_TRAVERSAL_2026-10-11.md), and his minimum-necessary-mathematics direction. Six generic Perlin candidates were implemented. None passed both exact-image and economics gates. All candidate production code, dependent test changes and GPU resources were removed; the ordinary app was rebuilt with the previously retained implementation.

## Three exact ray caches, all slower

The full-corner cache stored the eight normalized gradients for the actual `floor(P)` cell. The smaller full-cell cache stored only the two four-component final hash vectors. The last version stored one four-component XY hash, reusing it while `floor(P.xy)` stayed equal and rebuilding Z hashes at every call. These used approximately 28, 12 and 7 scalar slots per medium respectively, including validity/coordinate state. Each fragment invocation started invalid; separate medium slots avoided interleaved medium replacement. Source/occluder and shifted/Gradient points could replace a slot, but no final noise output was reused across points. Fractional coordinates, dot products, fade and interpolation remained per call. Large/nonfinite cell coordinates bypassed reuse.

All three matched eight 2560×1440 RGBA captures at spawn, inside, reverse and side, t=0/7.25. Each comparison checks all 14,745,600 bytes. The first two native noise witnesses passed 768 original-versus-cache f32-bit comparisons; the XY witness passed 33,536 calls, including slow movement, cell transitions, revisits, negative cells and large-coordinate bypass. Existing sharing/proof/refresh native checks passed for the first two versions. The final XY candidate was rejected on performance rather than expanding its test suite.

Each candidate was measured against a frozen current-head retained baseline in two sequential A–B–B–A series per view, 24 completion-fenced offscreen frames/run after warmup, fixed t=7.25, 2560×1440. Native timestamp features remained disabled. No image readback or simultaneous GPU correctness runs occurred during performance batches. Frozen and candidate constructor layouts matched their renderer binaries. Baseline binaries were preserved before changing renderer layouts for later experiments.

These are means of four run means per version, **not minima or floor timings**. Positive percentages below mean slower. Each experiment has its own paired baseline; absolute values across experiments or the earlier support pass must not be compared as causal gains.

| Candidate | View | Baseline mean ms | Candidate mean ms | Cost regression | Separate series regressions |
|---|---|---:|---:|---:|---|
| Full normalized corners | spawn | 55.05 | 61.03 | +10.88% | +7.64% / +13.96% |
| Full normalized corners | inside | 137.69 | 145.47 | +5.66% | +5.49% / +5.81% |
| Full normalized corners | reverse | 122.93 | 142.93 | +16.27% | +14.14% / +18.44% |
| Full normalized corners | side | 83.43 | 90.91 | +8.97% | +8.36% / +9.58% |
| Full-cell hashes | spawn | 59.43 | 62.15 | +4.57% | +5.30% / +3.90% |
| Full-cell hashes | inside | 145.91 | 158.92 | +8.92% | +8.84% / +9.00% |
| Full-cell hashes | reverse | 126.11 | 144.46 | +14.56% | +15.70% / +13.42% |
| Full-cell hashes | side | 85.98 | 93.29 | +8.50% | +9.82% / +7.21% |
| XY-only hash | spawn | 59.36 | 61.13 | +2.99% | +2.66% / +3.31% |
| XY-only hash | inside | 144.80 | 151.78 | +4.82% | +4.60% / +5.04% |
| XY-only hash | reverse | 126.15 | 135.62 | +7.51% | +7.66% / +7.36% |
| XY-only hash | side | 84.53 | 88.53 | +4.74% | +5.37% / +4.12% |

All three fail the 5% targeted improvement / 3% repeatable other-view regression gate. The XY inside, reverse and side regressions independently recur in both series. Reducing evaluated mathematics did not reduce hardware cost here. Branches, retained private state and occupancy/register effects are plausible costs, but **occupancy was not measured**, so no specific hardware cause is established. Do not convert this result into a universal claim that Perlin reuse can never help.

Across all 96 warm performance runs, lowering, shader compilation and CPU parameter/proof upload bytes remained zero. Existing resident storage is already operating; this pass did not find recurring parameter/proof transfer as the measured fixed-view bottleneck. Separated volume GPU execution time remains unavailable. Completion-fenced offscreen costs do not measure native surface acquisition, live movement FPS or route p95.

## Three GPU lookup variants, all fail exact image acceptance

An immutable GPU table was prepared once with the original WGSL arithmetic, with command-order synchronization, no CPU readback/wait and device/reload teardown. The variants stored (1) normalized gradients for 289 hash values, (2) unnormalized gradients while leaving normalization at the sample, and (3) the original permutation values for 577 integer inputs while leaving all gradient mathematics inline. Unproved table indices fell back to the original float intrinsic. These were generic Kernel intrinsic data, never Zone rules or authored/save state.

Each passed 33,536 native compute noise comparisons at exact f32 bits. Each nevertheless failed the first inside-view t=0 full-image check by one 8-bit level, after matching spawn. The comparison stopped immediately at that failure; no complete eight-view parity or performance result is claimed for these variants. Baseline inside SHA256 was `4495141ce86ebb671304681bd2313826a63e7f45e93de7f9948f508f31a21a20`; each lookup produced `00c265e4d4a53a8508e4b101b7b35747ac5a3cba91cea62f69e0bb2ff53b7c25`.

A scratch control kept the new resource machinery but restored the original inline evaluator. Its inside image matched the baseline SHA exactly. The discrepancy is therefore isolated to the changed evaluator path in this witness, rather than justified as an unrelated scene change. Its precise compiler/rounding cause remains unresolved. Returning normalization to the fragment did not fix it. Compute equality cannot substitute for complete rendered-pixel equality.

Observed one-level differences are also insufficient to admit these into bounded mode: there is no conservative final-output proof over all inputs. The budget is not permission to accept shortcuts based only on a handful of captures. All three lookup implementations and their resources were removed.

## Restored implementation and verification limits

`earthcall_webgpu` rebuilt successfully. The original sharing/parameter/proof/quadrature tests and production compiler/renderer sources were restored byte-for-byte from this pass's frozen originals. Seven focused targets pass with no skips, including the existing Perlin `EARTHCALL_PERLIN_TEST_MODE=ABC` selector. Eight fresh restored-versus-baseline full-resolution captures match every byte. No new optimization, cache resource or test-only feature remains in production.

The default full Perlin test's terrain Gate D independently fails `!sawInitialRangeUpload` with its original source and original compiler linked against the same current-head objects. This was reproduced before the cache/lookup retention decision; baseline log is `noise-cell-perlin-baseline.log`. The assertion remains intact. ABC deliberately excludes that terrain gate, so the seven-target result is not a passing full default Perlin test or full-suite claim. Revisit the terrain upload witness separately; do not weaken it to land rendering work.

Current base HEAD: `416bbcb99ed49b66e51480863d8b8c1b43de5ec5`. Restored `SdfWgsl.cpp` SHA256: `6387935592c6a1aba395ef9158d88bb854e90f557be1d7137eea5dcdfc44c65b`. Northern Veil save SHA256 remains `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`. Saves, the original read-only `nv_ref.cpp`/probe, resolution, sample counts, codecs and authority were untouched; no commits, pushes, stashes or deletion. An intermediate restore preserved old source timestamps and initially left a stale renderer object; the link refused it, restored source timestamps were refreshed, and the final complete rebuild/verification uses matching original sources.

## Handoff

The retained compiler/residency, GPU time preparation and support traversal remain active. Zach should assess that implementation through the existing [Person movement and appearance checks](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md#northern-veil-exact-support-traversal--october-11); there is no new Perlin speedup to expect. Thirty FPS at his usual framebuffer and ≤33.3 ms movement p95 remain unverified.

Do not repeat the 28/12/7-slot ray caches or these three immutable-table variants without a new mechanism that addresses the observed failure. Further work can revisit exact demand-aware retained-DAG sharing, direct parameter recipes, or certified contribution tails. Avoid assuming that higher proof grids, always-on support helpers, lower sample counts or broader private caches are wins. Existing unknown-proof fail-open behavior and the independent source-chroma semantic discrepancy remain authoritative boundaries.

Scratch evidence under `/private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/`:

- `noise-full-gradient-*`, `noise-full-hash-*`, `noise-xy-*`: cost/exact records, source/binary provenance and preserved candidates; `noise-cell-full-gradient.cpp` and `noise-cell-full-hash.cpp` preserve first compiler variants.
- `normalized-lookup-*`, `unnormalized-lookup-*`, `permutation-lookup-*`: rejected source/image records; `lookup-original-evaluator-control.*` and `lookup-control-inside.rgba` isolate the changed evaluator path.
- `noise-restored-build.log`, `noise-restored-focused.log`, `noise-restored-exact-results.json`, `noise-restored-provenance.json`: final ordinary app restoration and verification.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T10:29:00-07:00
