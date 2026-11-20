# Volume II — exact-order optical lowering and camera-active Joint subsets

Sixth-Two Sun / Codex GPT-6 / session `01a1eb67-9c6f-72c0-805f-5caceca627ad` / 2026-11-20T13:24:16-08:00.

Zach commissioned the next bounded pass after [EF5 Horizon's reply in Volume II](../../agent%20intercom/communication-threads/sdf-and-rendering/VOLUME_II_C5_SUN_PRISM_HORIZON_2026-11-19.md#ef5-horizon--prism-sun--sixth-two-sun--spill-cliff-counsel-and-active-subset-proposal). Prism requested exact-order same-sample optical lowering before compute decomposition. Horizon proposed a camera-active Joint subset and predicted four media and zero spills in every view. This pass tests those claims independently and follows the [P1 receipt](C5_PROOF_REUSE_EXPERIMENT_2026-11-20.md).

**Verdict:** the camera subset is a substantial exact-image scratch improvement in five views. Reverse still needs seven AABB candidates. The final combined subset/helper control reduces its cost further, to 226.90 ms, but remains about 34% slower than the frozen pre-C5 reference. Four-member programs still report compiler spill events. A separate proof-cache liveness repair removes introduced camera-return CPU churn. No production repair is landed; PR #882 remains blocked on performance and general admission qualification.

## Frozen inputs and checkouts

- Tested C5 HEAD `69ab1586f93be5522c5ce7c260b11f3e784d548a`, isolated worktree `/Users/zacharyzhang/.codex/worktrees/c5-summit-tribunal/Earthcall`.
- Zach's main checkout `/Users/zacharyzhang/Documents/GitHub/Earthcall` is on `prism/c5-current-default-reconciliation-ii` at `b8e8cbee6ac232da93a6cb2cae4725d30ab09d37`. Its `src`/`tests` matched the tested HEAD on inspection. It is not the default branch in this pass.
- The performance reference is the retained pre-C5 Release executable built from `d7fb75cc6fa5fc7a0ffe94bdcedbdea572662c09`, SHA256 `7ae8102303b7e25fe7a0db3e710819d59b19f3649c78c929c62723304c34ccd1`, copied byte-for-byte from `build/c5-joint/attribution-default`. The local fetched default reference `c564ca1672a9e5a267daa9bce1c3cd8e94f56ed9` had no Screen/Core source difference from that renderer baseline. This is not a newly rebuilt live-default result.
- Scene: restored eight-field Northern Veil from `31c0bb1a2`, Zone SHA256 `6c88b1f93f0e3f6b99a7880cabdcf5f5edbf9820632aa285e2d9a39fcc4ff5aa`. Each executable receives its own isolated fixture and reference clock 7.25.
- Native arm64 Release / Metal; 2560 × 1440 for full scene, 1280 × 720 for the fixed-sample diagnostic. Requested Routed ON, zero routed sets/pixels in the measured controls. No authored scene or live Data Home edit.

All experimental implementation changes are in ignored `build/c5-optical-frustum/`; this audit and the signed Intercom/task checkpoints preserve the findings. The normal renderer, tests, save files, budgets and installed application are unchanged by this pass. Tests are sequential. Reported ms are medians of two three-frame completed batches after drawn-frame warm-up, with captures separate and untimed. Source identity, membership, packet size, warm no-compile/no-refusal and fixed camera/resolution are checked. Short runs and desktop/compositor load limit precision; no statistical confidence or Person FPS acceptance is claimed.

## 1. Exact-order optical lowering

The transform moves the eight original optical bodies into two functions, `optical03` and `optical47`, with constant member selectors. There are still eight ordered call sites, each at the original medium's optical position after its original support/proof gates. Two pointers carry the same extinction/source accumulator locals. Authored expressions and their relative accumulation order stay unchanged. Event sorting, sample coordinates, proof traversal, clipping, early termination and the single `(L,T)` recurrence remain stock. This is function-boundary lowering, not loop interchange or independent framebuffer compositing. It supplies no no-inline guarantee and performs no structural evaluator deduplication.

All six full-scene and all six fixed-sample captures are byte-identical. The fixed diagnostic retains its original mapped sample positions and arithmetic and is not a production transport correctness witness by itself.

| View | Full stock ms | Helpers ms | Full gain | Fixed stock ms | Fixed helpers ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| A | 168.68 | 163.06 | 3.33% | 336.54 | 324.79 |
| B | 204.18 | 200.32 | 1.89% | 370.62 | 361.33 |
| C | 324.65 | 311.01 | 4.20% | 374.56 | 371.02 |
| D | 299.20 | 271.43 | 9.28% | 299.09 | 301.74 |
| E | 199.90 | 193.73 | 3.09% | 372.06 | 368.03 |
| F | 716.75 | 677.06 | 5.54% | 353.57 | 349.56 |

Mean per-view full gain is 4.55%, below Horizon's proposed across-view 5% target, with only D/F individually above 5%. This is not promoted as the primary repair. However, a matched reverse-camera source control changes the encoder-reported spill amount **960 → 400 bytes/event**, while the full captures remain exact. It refutes an absolute claim that a single-shader representation change cannot help while spills remain. It does not reveal the spilled object, register count, occupancy, traffic or surviving MSL function boundaries.

| Reverse-camera program | Fingerprint | Labeled encoders | Spill-event reports |
| --- | --- | ---: | --- |
| Stock Joint8 | `2907418542ec52c6` | 7 | 7 × 960 bytes |
| Two-helper Joint8 | `eb40e14b5de18e12` | 12 | 12 × 400 bytes |

The stock recording reached its bounded time limit after actual measured draws; its saved tables are valid for those encoders. Traces are excluded from frame timings.

## 2. Match the actual primary-ray domain

The shader constructs a ray at the eye using `u.invViewProj`, integrates from `max(bounds.x, 0)`, and clips its end against opaque depth. It does not explicitly use camera near/far distances as integration endpoints. A six-plane near/far omission would therefore assert a different query domain. The synthetic near/far witnesses below retain media before the near plane and beyond the far plane.

The scratch candidate uses side/eye-cone classification and an additional axis-direction bound over the actual inverse-projection rectangle. The latter only rejects a box on the opposite side of the eye from every possible direction on a coordinate axis, away from `safeRd`'s near-parallel cutoff. It fixes conservative misses caused by a large box straddling a clip plane. Nonfinite and uncertain inputs fail open. Half extents match the shader's `max(abs(scale), 1e-4)` convention.

`Cone.hpp` declares a 64 × f32-epsilon arithmetic guard; the full-viewport verifier additionally queries enlarged world boxes using a corresponding scale/eye-dependent pad. This is a bounded research prototype, not a complete universal Metal Fast-math enclosure. Unsupported/general camera profiles and full floating-point proof qualification remain promotion work. The host witnesses and native viewport checks below establish only the reported domains; they must not be expanded into a shipping theorem.

The literal strict fail-open variant would keep eight boxes in A/B/C/E/F and seven in D. The stronger actual-ray cone/direction candidate admits the following omissions. It has no authored-name or position rule and no four-member cap.

### Independent whole-viewport witness

A diagnostic fullscreen fragment program copies the stock ray reconstruction and `rayAabbWorld` arithmetic, queries every medium's raw and enlarged box, and returns per-pixel bitmasks. Every one of the 3,686,400 viewport pixels must produce the diagnostic marker. Across six cameras this covers **22,118,400 pixel positions**, each tested against all eight boxes. Every proposed omission has zero raw and enlarged-box hits. This is distinct from the earlier 2,304-ray density census and does not substitute for a universal numerical contract.

| View | Raw box-hit pixels M0…M7 |
| --- | --- |
| A | 3,686,400, 3,230,720, 3,000,320, 2,957,092, 0, 0, 0, 0 |
| B | 3,686,400, 3,509,760, 3,225,600, 3,686,400, 0, 0, 0, 0 |
| C | 3,686,400, 3,686,400, 3,686,400, 3,686,400, 0, 0, 0, 0 |
| D | 3,686,400, 3,686,400, 3,686,400, 3,686,400, 0, 293,557, 395,183, 521,389 |
| E | 3,686,400, 3,141,120, 3,545,600, 3,686,400, 0, 0, 0, 0 |
| F | 0, 0, 0, 0, 2,878,228, 3,683,031, 2,515,247, 2,474,433 |

The reverse camera hits M1/M2/M3 boxes at all 3,686,400 pixels. Their earlier sampled density counts were zero, not proof of whole-viewport absence. AABB-only admission cannot omit them. The candidate legitimately retains seven media at D and only removes M4 there. Stronger derived support/query evidence would be needed to reach four.

The host classifier also passes eight adversarial cases (near, far, eye straddle, strict behind refusal, separated behind/side, nonfinite, grazing contact) and 20,000 random boxes with **1,972,503 independent sampled ray-box checks** over rejected boxes. These host samples are not mislabeled exhaustive native viewport proof.

## 3. Pure camera-subset performance

| View | Stock Joint8 ms | Camera subset ms | Frozen default ms | Retained members |
| --- | ---: | ---: | ---: | --- |
| A | 173.95 | 116.04 | 110.06 | M0,M1,M2,M3 |
| B | 208.13 | 145.94 | 136.17 | M0,M1,M2,M3 |
| C | 335.33 | 191.47 | 185.68 | M0,M1,M2,M3 |
| D | 298.77 | 274.04 | 169.15 | M0,M1,M2,M3,M5,M6,M7 |
| E | 202.91 | 139.72 | 134.40 | M0,M1,M2,M3 |
| F | 721.73 | 509.87 | 499.00 | M4,M5,M6,M7 |

The retained order is the original authored media order. The candidate composes one ordinary unrouted Joint program per view with 4 / 4 / 4 / 7 / 4 / 4 members. One full-viewport packet and one `(L,T)` continuation remain; no chronological ordering or legacy 4+4 framebuffer split is invented.

All six candidate captures match stock Joint8 byte-for-byte. Gains versus stock are approximately 33.3%, 29.9%, 42.9%, 8.3%, 31.1%, 29.4%. Five views are within about 2.2–7.2% of the frozen default cost. Reverse remains **274.04 vs 169.15 ms**, about **62.0% slower**. This is the decisive remaining performance blocker.

### Wide all-eight control

An additional G camera at `(-2,61.5,-650)` looking at `(-2,61.5,32)` retains all eight media. Its stock/subset WGSL fingerprints are identical (`eba60b98aace5a5a`) and captures are byte-identical. The first timing pair was noisy, so a fresh ABBA control was run. Across its blocks, stock median is **40.56 ms**, candidate **39.95 ms** (individual blocks 36.10–46.65 ms). This small control shows no reliable extra cost; it is not a statistical no-regression theorem. No tile classifier or compute transport fallback is implemented.

### Independent native clipping/transport witnesses

Six paired 64 × 36 native cases match exactly: overlapping absorber/emitter with a bright omitted rear medium, opaque clipping, high extinction with the retained cutoff, inside camera, an extra medium before near, and an extra medium beyond far. Actual draws are checked: 3 → 2 members for the first four and 3 → 3 for near/far. The initial log-count parser assumed interleaved stdout/stderr; correcting that parser changed no native input or parity result.

## 4. New spill observations — four does not mean zero

| Camera / actual program | Members | Fingerprint | Labeled encoders | Spill reports |
| --- | ---: | --- | ---: | --- |
| A / northern subset | 4 | `46aef645c542f80c` | 24 | 24 × 288 bytes |
| D / reverse subset | 7 | `3c6d7cc80c3b8361` | 4 | 4 × 784 bytes |
| F / southern subset | 4 | `201422b58a876a5b` | 1 | 1 × 464 bytes |

Tables join spill events to actual labeled volume encoders by command-buffer/encoder identity and benchmark process, with actual draw-source bytes checked against the measured candidate. A completes its recording. D/F later lose foreground after their measured draws; only the actual encoder reports are retained, with this partial-run limitation, and no timing is taken from those traces. Register/thread, occupancy and cache metrics remain unavailable. These are encoder-event reports, not bytes/thread or identified local arrays.

The two four-member full-optics programs report different nonzero amounts. The earlier zero-spill G1 3+5 result belongs to synthetic zero-optics bodies. It did not establish a full-optics Joint<=4 zero-spill tier, nor does the new evidence justify a universal cardinality threshold.

## 5. Query omission exposed a proof-cache lifetime problem

The original zero-proof collector drops memo entries unused in one rendered frame. With camera subsets, an authored medium may remain present while omitted from the current query. A → … → F → A reuses the compiled A program (zero compiles), but rebuilds four unchanged proofs. Measured CPU volume preparation at return is **116.534 ms**, with zero parameter/proof uploads: this is host reconstruction, not upload or compilation churn.

A separate default-off scratch control keeps an already-derived zero-proof memo live while its valid density binding remains in the authored input set, before camera omission. It builds no new proof for a hidden medium. Original revision/extent validation runs on the next actual query; removal from the authored input set still permits collection. No support-certificate policy is changed.

The control preserves all six scene captures. Actual return-A CPU preparation is **0.04125 ms**, with zero proof builds, zero program compiles and zero parameter/proof uploads. These are CPU preparation figures, not a claim that the whole GPU frame takes 0.041 ms.

### Native lifetime/invalidation witnesses

Eight states are compared against uncropped stock, plus an explicit culled/evicted control. Every captured image is exact across the three executions. The unchanged-return case must distinguish retained zero rebuilds from evicted one rebuild. A hidden density edit must be noticed on reentry; extent changes and actual author-input removal/readdition must rebuild. Density, extent and removal changes must visibly affect their native image, preventing a blank or saturated false witness. The initial synthetic edit setup failed that visible-effect gate and was corrected before accepting evidence; rejected outputs remain in scratch.

| State | Stock first proof builds | Retained | Evicted control |
| --- | ---: | ---: | ---: |
| prime | 3 | 3 | 3 |
| away | 0 | 0 | 0 |
| unchanged-return | 0 | 0 | 1 |
| away-edit | 1 | 0 | 0 |
| reenter | 0 | 1 | 1 |
| extent | 1 | 1 | 1 |
| removed | 0 | 0 | 0 |
| readded | 1 | 1 | 1 |

The cache control is kept separate from the pure subset timings. Production memo lifetime is unchanged; this is an unlanded bounded repair with native invalidation witnesses.

## 6. Shader and artifact receipts

`optical-results.json`, `optical-spills.json`, `rayhit-axis-results.json`, `subset-results.json`, `subset-spills.json`, `subset-witness-results.json`, `lifetime-results.json`, `wide-repeat-results.json` and `combined-results.json` retain the detailed records in `build/c5-optical-frustum/`. Compressed native frames, WGSL, traces and failed attempt logs remain beside them. Their build-directory storage is local and ignored; this audit carries the essential durable facts.

| View | Candidate WGSL fingerprint / bytes | Exact full-scene capture SHA256 |
| --- | --- | --- |
| A | `46aef645c542f80c` / 82600 | `321f947d15e1e31ad6ec3902e91acef4c39a52ae2399aef962d46307397cb213` |
| B | `46aef645c542f80c` / 82600 | `efef141141e53eb8bdc8acacc76029dafce9ae4866cc3bb7dd7f30e0cf0f45be` |
| C | `3e7c1290c3f09018` / 78770 | `fe1ce8b8ba4297551f9e8b5366cac0bb53c8b94b68157e3d7f2d51f2781d50f0` |
| D | `3c6d7cc80c3b8361` / 134683 | `e79add73e52dde3fbb11c92cde9f3afdfb1ad55fc4bbde3ad5d569fd7125c597` |
| E | `46aef645c542f80c` / 82600 | `c1a10d51aa20faefa34a1879dc462cae74815bad86965c9a27c835d2760fdd68` |
| F | `201422b58a876a5b` / 97334 | `134223f1501f235c930c38c199d03a1113493b121af5b5567bdd972ab0f4043c` |

Native Release scratch linking and the listed focused experiment witnesses pass. The inherited Emerald proof refusal and settled support-certificate dossier are not reopened. The full retained eleven-test PR regression sweep is not rerun for this unlanded experiment and is not claimed green by implication. No main Release installation, Person acceptance, CI rerun, PR reconciliation, commit or push is performed.

## 7. Final combined control — helper lowering after camera omission

The final pair holds the camera subset and proof-lifetime repair fixed, and changes only the ordered helper lowering. The transform supports the actual four/seven retained media without changing their order, event walk, proof gates or transport recurrence. Both controls match stock C5 at all six cameras: twelve exact full-scene captures. Every measured draw's actual WGSL is checked against the complete expected source, and warm frames have zero program compiles/refusals, proof builds or parameter/proof uploads. Two intermediate candidate map requests were unmatched; neither appears among measured draw fingerprints. Source validation, rather than the absence of map warnings, establishes the measured comparison.

| View | Subset + retained proofs ms | Subset + helpers + retained proofs ms | Gain |
| --- | ---: | ---: | ---: |
| A | 118.73 | 114.97 | 3.17% |
| B | 143.72 | 137.17 | 4.56% |
| C | 189.60 | 177.46 | 6.40% |
| D | 275.32 | 226.90 | 17.59% |
| E | 140.23 | 136.30 | 2.80% |
| F | 507.41 | 496.64 | 2.12% |

This short controlled series supports an additional physical-lowering benefit, strongest at reverse. It does not establish a universal compiler rule. Relative to the separately measured frozen-reference D cost of 169.15 ms, combined D remains about 34.1% slower. A/B/E are within about 0.7–4.5% of their retained reference timings; C/F are slightly faster in these short series. These cross-series differences are not statistical no-regression claims. The final control has no matched spill trace; the earlier Joint8 960→400 observation cannot be assigned to this seven-member program.

| Combined actual program | Fingerprint | WGSL bytes |
| --- | --- | ---: |
| A/B/E northern four | `2918f494bff955ef` | 83217 |
| C inside four | `48b7a6a4547bc289` | 79387 |
| D reverse seven | `4aaab0bc34247525` | 135811 |
| F southern four | `55c0848f36802608` | 97951 |

## Causal verdict and next boundary

Camera query omission can reduce physical shader cost without changing Joint mathematics. It recovers most of the five-view regression; a cache lifetime correction separately removes newly introduced CPU churn. The final combined control preserves those exact images while further improving reverse, but leaves a substantial reference gap. Exact-order single-shader lowering also changes reported allocation in the isolated Joint8 control, so an absolute immutable M5 spill-wall claim is unsupported. Width alone does not predict a zero-spill tier.

Reverse is the remaining organ: its AABBs admit seven media, while the earlier sampled optical workload touched four. The missing authority is a whole-camera or tile-local proof finer than the current whole-view candidate set, or a more profitable physical lowering of retained Joint work. Sampled zero counts cannot supply that authority. No authored position shortcut, forced four-member limit, chronology weakening or independent framebuffer split is introduced.

There is an additional lattice obligation for any future support-only omission. M1/M2/M3 boxes are hit, so their entry/exit events and chord-derived spacing can still affect stock quadrature even where density is proved zero. A theorem that their optical contribution vanishes does not by itself authorize deleting those events or changing other media's sample positions. An optical subset would need to retain authoritative event/chord scheduling, or separately qualify a changed lattice. Tile-local AABB misses avoid that particular obligation only where every ray in the tile misses the omitted box. Neither extension is implemented here.

**Merge / Person verdict: NOT READY.** Resolve reverse economics, complete the general numerical admission contract and review the bounded cache repair before promotion. Then freeze the candidate, reconcile the actual PR head onto live default, run exact-head focused/CI guards and the six-view Person-performance tribunal. The normal application remains unchanged by these scratch experiments.
