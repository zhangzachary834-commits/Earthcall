# Volume II — isolated per-medium proof reuse (P1)

Sixth-Two Sun / Codex GPT-6 / session `01a1eb67-9c6f-72c0-805f-5caceca627ad` / 2026-11-20T00:47:45-08:00.

Zach relayed [Prism Sun's P1 commission](../../agent%20intercom/communication-threads/sdf-and-rendering/VOLUME_II_C5_SUN_PRISM_HORIZON_2026-11-19.md#rendering-warrior-prism-sun--sixth-one--sixth-two--ef5-horizon--next-directions-after-scalar-event-sweep) in the main checkout's Volume II thread. This responds to that commission and continues the [Horizon diagnostics](C5_HORIZON_TRAVERSAL_OPTICAL_DIAGNOSTICS_2026-11-19.md). Prism asked to preserve stock Joint8 chronology and isolate the cost of rediscovering per-medium zero intervals. The candidate remains scratch-only and is rejected as the primary performance repair.

## Frozen inputs and evidence scope

C5 HEAD `69ab1586f93be5522c5ce7c260b11f3e784d548a`; isolated worktree `/Users/zacharyzhang/.codex/worktrees/c5-summit-tribunal/Earthcall`. Restored eight-field Northern Veil from `31c0bb1a2`, Zone SHA256 `6c88b1f93f0e3f6b99a7880cabdcf5f5edbf9820632aa285e2d9a39fcc4ff5aa`. Fixture data lives under `build/c5-proof-reuse/fixture-c5/saves`. Live Person Data Home and the restored checkout Zone were not edited by this pass.

Native arm64 Release on the same Metal device, 2560 × 1440, clock 7.25, six existing cameras, requested Routed ON with zero routed dispatches. Actual draw records verify one full-viewport unrouted Joint8 packet, the expected WGSL bytes and ordered eight members. Warm frames have no compilation or program refusal. Frame timings below come from foreground-gated, uninstrumented runs, each with two batches of three completed frames and a separate untimed capture. They are short matched measurements, not statistical confidence intervals.

All scratch source transforms, native logs, captures, host replays and validators live in `build/c5-proof-reuse/`. `results.json` records exact source/capture identities, per-batch timing and per-medium counters; `witness-results.json` records the adversarial native comparisons.

## Cache contract and a caught scheduling defect

Each medium owns one invocation-local `skipUntil`, initially −1. A fresh `volumeZeroCellExit` supplies its horizon. A later stock sample may reuse the zero fact only under strict `sampleT < skipUntil`. Unknown/expired results do not authorize reuse. Bindings, proof payloads, ray origin/direction and clocks are immutable during the invocation; state never survives a ray, frame, source revision or camera change. The inherited helper's enlarged-cell, column and near-parallel contract remains the authority; this pass establishes no new universal numerical theorem.

The first literal horizon cache preserved six full-optics images but added 48 / 19 / 0 / 0 / 11 / 7 sampled null visits at A–F. A retained exit can lag a newly returned exit, changing collective jump endpoints. It also produced a weaker synthetic G1 parity result, recorded below. This first version is not the final P1 experiment.

The corrected P1 retains cached facts to suppress per-medium null work, but excludes their older exits from `walkExit`. If the sample would trigger a collective zero jump, cached members are queried freshly before that jump is chosen. Fresh refusals defeat the collective jump. Thus an older cached horizon cannot authorize a different global jump. Revalidation calls are included in the totals below; cache hits are not silently treated as net saved calls.

The sorted event array, clipping, segment construction, sample formulas and authored accumulation order remain stock. Exact source comparisons verify all eight optical blocks and the entire transport suffix unchanged in both inside/outside programs. Opaque-depth clipping and both `T < 0.01` breaks are preserved. An initial scratch refresh query used an out-of-scope `useColumns` identifier in the inside variant; that parsing failure was fixed by copying the original query arguments before the successful measurements.

## Correctness witnesses

- Independent finite proof-oracle replay: 5,003 cases pass, with 3,693,998 cached hits; includes opaque clipping, overlapping absorption/emission and 1,617 early-termination cases. A new invocation starts with empty state.
- Independent collective-jump replay: 2,003 cases pass with the corrected cache and identical visited indices. The naive horizon version changes visits in all 2,003 constructed cases. Oracles can conservatively refuse and can return changing horizons over certified zero regions.
- Actual restored Northern Veil: all six corrected full-optics native captures are byte-identical to fresh stock captures. The earlier naive variant also matched six full-optics captures; that does not erase its sampled traversal difference.
- Four additional native 64 × 36 fixtures pass exact stock/P1 comparison: overlapping absorber plus banded emitter, an opaque plane clipping that overlap, high extinction exercising the retained early-termination path, and an inside camera. Both actual generated fused shaders are transformed and identified at draw. Opaque and high-absorption controls change their images, so the witness does not compare an invariant blank buffer. Derived zero proofs are attached (`18,432` proven cells).
- Corrected census: sampled `stepIterations`, all eight density-evaluator counts and medium optical-evaluator totals match stock at every camera, in both full and G1 lanes.

G1 is a synthetic cost diagnostic, not an authored-frame correctness witness. A/C/F captures are exact; B/D/E differ at respectively 2 / 2 / 3 channels by at most one byte level. Both cache variants produce the same corresponding G1 capture hashes. The cause of those tiny differences is not isolated, and no floating-point explanation is promoted into proof. G1 exact parity is not claimed or normalized by a tolerance change. Its timings carry this diagnostic limitation. The full authored-scene and four adversarial exact gates above pass.

## Completed-frame timing and work census

| View | Full stock ms | P1 ms | G1 stock ms | G1 P1 ms | Stock / P1 proof calls | Cache hits |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 172.86 | 166.35 | 72.33 | 82.14 | 181,033 / 143,140 | 50,594 |
| B | 204.00 | 201.70 | 89.04 | 100.72 | 237,157 / 180,950 | 76,335 |
| C | 327.67 | 355.69 | 101.72 | 109.65 | 359,114 / 297,544 | 82,109 |
| D | 294.30 | 310.62 | 94.57 | 102.80 | 291,826 / 286,682 | 7,901 |
| E | 200.27 | 195.39 | 87.56 | 99.55 | 237,750 / 181,766 | 76,731 |
| F | 720.25 | 690.72 | 148.46 | 145.12 | 427,612 / 311,746 | 127,583 |

Timings are ms per completed frame. The table uses the corrected P1 and repeated stock baselines. The census selects one pixel at each center of a 64 × 36 screen grid, exactly 2,304 fragment invocations per view, in separate instrumented runs. Counts are sampled-ray observations, never scaled to full-screen instructions or used as frame timings. Added counters can change physical lowering.

| View | Steps, stock = P1 | Proof calls removed | Medium optical calls, stock = P1 |
| --- | ---: | ---: | ---: |
| A | 129,518 | 20.93% | 258,324 |
| B | 156,101 | 23.70% | 308,338 |
| C | 248,624 | 17.14% | 560,092 |
| D | 280,103 | 1.76% | 534,340 |
| E | 156,614 | 23.55% | 302,394 |
| F | 238,880 | 27.10% | 600,086 |

Optical totals count executed top-level medium shared-value preparation and density/extinction/scattering/chroma/phase/emission entry calls, excluding nested expression-tree leaves. G1 replaces these authored evaluations and has zero such calls. Authored light-source calls are outside this medium total. An additional instrumented run intended to count them separately was blocked by the later desktop foreground/drawable refusals and supplies no valid totals. This limitation must not be rewritten as a complete all-source optical census.

### Ordered medium identities

- M0: `northern_veil.aurora.primary-emerald-curtain`
- M1: `northern_veil.aurora.secondary-cyan-ribbon`
- M2: `northern_veil.aurora.accent-violet-crest`
- M3: `northern_veil.aurora.deep-crimson-fringe`
- M4: `northern_veil.aurora.far-horizon-helical-tendril`
- M5: `northern_veil.aurora.far-zenith-coronal-filaments`
- M6: `northern_veil.aurora.far-southern-twin-braid-curtain`
- M7: `northern_veil.aurora.far-southern-gossamer-tendrils`

### Proof queries per medium (stock → corrected P1)

| View | M0 | M1 | M2 | M3 | M4 | M5 | M6 | M7 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 80,381 → 65,222 | 22,296 → 19,891 | 56,402 → 46,006 | 21,954 → 12,021 | 0 → 0 | 0 → 0 | 0 → 0 | 0 → 0 |
| B | 93,156 → 73,322 | 25,995 → 24,019 | 80,768 → 63,059 | 37,238 → 20,550 | 0 → 0 | 0 → 0 | 0 → 0 | 0 → 0 |
| C | 198,154 → 149,072 | 55,073 → 53,205 | 95,754 → 89,808 | 10,133 → 5,459 | 0 → 0 | 0 → 0 | 0 → 0 | 0 → 0 |
| D | 259,517 → 259,517 | 0 → 0 | 0 → 0 | 0 → 0 | 0 → 0 | 9,617 → 8,463 | 10,542 → 8,230 | 12,150 → 10,472 |
| E | 87,197 → 67,686 | 23,468 → 21,990 | 85,955 → 67,705 | 41,130 → 24,385 | 0 → 0 | 0 → 0 | 0 → 0 | 0 → 0 |
| F | 0 → 0 | 0 → 0 | 0 → 0 | 0 → 0 | 120,653 → 99,286 | 135,952 → 103,560 | 91,419 → 48,008 | 79,588 → 60,892 |

### Density-evaluator calls per medium (stock = corrected P1)

| View | M0 | M1 | M2 | M3 | M4 | M5 | M6 | M7 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 45,863 | 16,071 | 33,149 | 8,648 | 0 | 0 | 0 | 0 |
| B | 45,216 | 16,312 | 45,405 | 13,074 | 0 | 0 | 0 | 0 |
| C | 121,217 | 37,592 | 60,396 | 3,045 | 0 | 0 | 0 | 0 |
| D | 235,649 | 0 | 0 | 0 | 0 | 6,989 | 6,547 | 7,634 |
| E | 40,189 | 15,329 | 47,675 | 15,314 | 0 | 0 | 0 | 0 |
| F | 0 | 0 | 0 | 0 | 81,629 | 82,875 | 36,826 | 49,117 |

## Shader identities

| Program | Fingerprint | WGSL bytes | SHA256 |
| --- | --- | ---: | --- |
| full-stock-repeat | `eba60b98aace5a5a` | 158565 | `cb1a2abd2a950172886749de77c5f842b4b224ffa6c03ec7381c6dff462b4571` |
| full-stock-repeat | `2907418542ec52c6` | 153527 | `673d98480b4d48cc5a2a0901c085578ce7022d138cd6b4353b1739f818b3c36a` |
| full-P1-stockjump | `82257e397aae8d4f` | 165140 | `ecbf484d4570ecadf9f7fd507708e910a77587deacc361ce62d9f315a3c81c24` |
| full-P1-stockjump | `44b28f43be40208d` | 159926 | `c6c780829e43ec0c82ce539007bad527e3f2301b68a6b798023a234b74eca41b` |
| G1-stock-repeat | `57d395063489dbd6` | 128453 | `0a73b5de3e37ee18362ea6ccf17e71bc5e1dd1c3ae146e07e35ef826997380a9` |
| G1-stock-repeat | `f8cafbe6d66af7d2` | 123415 | `c794442bcb6c6e2b744319d6caec3371035862a1aa286862c5d147ac531b7008` |
| G1-P1-stockjump | `3eefa83b1dddc121` | 135028 | `b8d8f43c9a7f3f033eb0380f24cabeb41e05df3ba55a1ede6b8ad2c7d9f7f766` |
| G1-P1-stockjump | `53d9ecf854d3798f` | 129814 | `8a7a50dc9c2ddca3b23e994f6f4e461ed1b33b425e9ceb60cebd1301a0e70a0f` |

## New Metal evidence and remaining instrument boundary

Storage was available for this pass. A new reverse-camera full-optics stock trace saved successfully. Its actual draw is the inside Joint8 `2907418542ec52c6`, byte-identical to the stock map. Joining `graphics-compiler-spill-events` to `metal-application-encoders-list` by command-buffer/encoder identity and benchmark process yields **15 labeled volume encoders and 15 attributed events reporting 960 bytes each**, with no unmatched benchmark spill events. The trace keeps a bounded 10-second window. This is full-optics stock evidence, distinct from the earlier 496-byte synthetic G1 trace.

P1 trace launches twice failed the foreground gate before any measured draw. The saved trace documents are rejected as P1 shader evidence. Later Cocoa activation attempts also failed; allowing a background window only for untimed diagnostics then failed the actual-draw warm-up gate. No no-draw or failed-activation interval enters timing or counter tables. There is no matched P1 spill result. Registers/thread, occupancy and instruction/cache attribution remain unavailable. No claim that P1 fixed spilling is supported.

The additional authored-light census failed under the same desktop conditions. The earlier complete medium census and foreground timing runs remain valid and retained. This pass did not remove the draw-completion requirement or turn a desktop refusal into a renderer failure.

## Causal disposition

Per-medium proof rediscovery exists, but this cache does not materially repair Joint8 economics. Reverse camera removes only 1.76% of proof calls despite its earlier Joint/split visit inflation, and corrected full frame time rises 294.30 → 310.62 ms. Far south removes 27.10% of proof calls and improves only 720.25 → 690.72 ms in this short pair. Dense inside regresses 327.67 → 355.69 ms. G1 worsens in five views. Other small changes do not recover the controlled default gap and establish no reliable across-view gain.

Apply Prism's abandon rule: stop treating P1 as the primary repair. This does not prove that all proof reuse is unprofitable or that a particular GPU resource is the sole culprit. Compiler effects, control overhead and physical shader economics remain candidates; there is no matched P1 hardware report that selects among them. The next authorized direction in Prism's note is exact-order, same-sample optical lowering; it has not been tested in this pass. Compute decomposition remains deferred.

Production renderer and tests have no changes from this experiment. Emerald thresholds, support certificates and authored saves are unchanged. No scratch candidate is landed, no PR rebase/push is performed, and no Person acceptance is claimed. The C5 performance gate remains closed.
