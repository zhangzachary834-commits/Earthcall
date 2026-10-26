# Northern Veil cell-local Perlin polynomial: near-current numerical output, no profitable native checkpoint

Zach commissioned the next pass after [Astra's architectural review](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#astra-architectural-review--compile-the-value-as-well-as-the-proof-2026-10-21). This continues [the coordinated existing thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#codex-next-pass--astra-cell-local-value-recipe-reuse-and-construction-screen). Zach originated the undiminished one-ray-program vision, OntoMath ownership boundary and current-or-better accuracy checkpoint; Astra proposed this fixed-cell value experiment and cost contract. Codex derived the executable generic recipe, built the scratch native consumer and performed the measurements below.

**Decision: reject this implementation on native cost.** The two inside-camera ABBA series give **+5.09% / −2.93%**, pooled **+0.85%** (92.621→93.409 ms/frame). There is no repeatable substantial gain. All eight runs/192 frames are retained. The finite raw-output comparison stays at current accuracy scale, with tiny mixed-sign changes relative to the independent reference, but does not establish global numerical admission. No production source, save, installed app, authored lighting, quality control, output budget, default certificate/timestamp setting or original integration schedule changes. No PR or push. Stop this candidate before broader qualification.

## What the experiment actually derives and executes

`scratch/probes/ray_transport_noise_polynomial_trial.py` contains a generic expression DAG for the real-arithmetic identity: inside a fixed noise cell, eight affine corner dots mixed through three quintic fades yield a univariate polynomial of degree at most 16. It follows the original z/y/x interpolation structure, folds literal zero/one operations and shares identical subexpressions. A separate direct corner-interpolation implementation checks **8,704 points** from 512 deterministic cases, including zero and signed direction components, against Horner evaluation. Maximum double discrepancy is **2.254e−14**. This is an algebra/numerical witness, not an f32 error bound or proof of all domains.

The coefficient DAG has **175 adds, 46 subtracts, 296 multiplies = 517 scalar operations**, plus original gradient/hash construction, 16 corner dot products, domain/key work and output stores. Horner evaluation takes 16 multiply-add stages. These are source arithmetic counts, not device instruction counts or timing predictions. The full polynomial has 17 coefficient scalars. Cache/key/domain state adds 13 logical scalar components (30 total); ray direction/current sample context adds 4 more. Backend register allocation and occupancy are not measured.

The scratch transform recognizes four verified affine noise calls in the frozen emitted shared evaluators. Each binds `frequency * (worldP - origin)` from its actual parameter buffer, with derivative `frequency * rd`. Call identity binds the immutable frequency/origin parameters; ray identity is the current invocation. Key=(call identity, actual f32 floor(P) bits). No state crosses rays, frames, clocks or devices. This is a complete affine-binding key within the stated invocation/fixture, not the older binding-independent gradient key. Structural hook mismatches refuse the experiment instead of silently guessing new affine forms.

On a miss, the original hash/gradient normalization defines the same cell gradients. The recipe builds coefficients using the current actual fractional noise position as local anchor and a dominant-direction normalization. Later evaluations use local u=(sampleT−anchorT)*rate. Every call checks its actual cell and its reconstructed f32 model position; both must remain in the prepared cell, with nonnegative local progression, or the slot is rebuilt. Large-cell/unsupported rate inputs use the original intrinsic. These are experimental numerical guards, **not** a certified enclosure of every rounded position. Unsupported ASTs, arbitrary transformations, domain-edge arithmetic, long/large rays and NaN cases are not broadly admitted by this finite trial.

The GPU slot is explicitly Kernel-only invocation scratch, not authored world state. The original sample nodes, zero/support decisions, box/event segmentation, density gate, projected six channels, source evaluation, extinction floor, shared attenuation and RGB update order remain. No CF4, new placement, noise lookup table, altered quality knob or extra proof grid is combined with the candidate. The reused builder's executable is still named `cf4`, but this transform applies **no CF4 integration**. It removes the parent's unrelated rule-witness metadata.

This is a mathematical recipe in experimental tooling, **not** a new production mathematical office hidden in Screen. Had native economics passed, generic affine/cell derivation and admission would belong in OntoMath with Screen lowering. Because economics fail, no production API/class/flag is added.

## Actual native reuse, then value and result comparisons

A distinct original-evaluator census uses 64×36 selected actual pixels of the 2560×1440 inside/time7.25 fixture, after all current support/channel gates. It observes:

- **67,184 noise calls**, **59,929 hits**, **7,255 preparations**, zero fallback calls and zero same-key model-domain resets.
- **89.2013% hit rate**, **9.2604 calls/preparation**. The full semantic key loses four hits compared with the previous gradient-only census; 89.21% was not assumed.
- All 7,255 reuse-run lengths are accounted for in the JSON histogram (length 31 bin includes 31+). The 517-op DAG alone amortizes to 55.83 scalar operations per original call before gradient, Horner, control and state costs. This justified a bounded native test, not a forecast of profit.

A separate untimed capture compares **all 67,184 actual noise evaluations** with the original intrinsic at the same input positions. Maximum absolute difference is **3.5793e−5**, on the ray through pixel (820,100). Rounded coordinate reconstruction, coefficient formation, Horner association and compiler lowering all contribute possible differences; this experiment does not isolate them. The polynomial is **not a bit-exact cache**. This comparison adds original evaluations only to the diagnostic executable, never to performance runs.

Paired raw T/premultiplied linear RGB captures use the original-evaluator control and polynomial candidate on all 2,304 actual rays. Control outputs exactly match the previously published native export; origins/directions/clip data match between the new captures, and all pixel labels match Opus's independent rows. The analyzer checks the frozen fixture SHA. The reference is Opus's prior CPU computation, not a new run claimed here.

| Absolute discrepancy against independent reference | Current T | Polynomial T | Current RGB | Polynomial RGB |
|---|---:|---:|---:|---:|
| Mean |0.0001335152281|0.0001335157590|0.00005223495505|0.00005223499264|
| p99 |0.001430285359|0.001430106545|0.000749102974|0.000749095523|
| Max |0.003024910894|0.003024910894|0.001958256956|0.001958256956|

Mean errors worsen by **5.309e−10 T /3.758e−11 RGB**, while p99 improves and maxima are unchanged. Those mean differences are below the reference's observed finest/half mean spreads (**8.396e−9 T /3.057e−9 RGB**). This supports a **near-current finite numerical screen**, not a demonstrated accuracy improvement, a rigorous reference-uncertainty interval, or author acceptance of arbitrary changed outputs. Reference refinement spread is not itself a certified error bound.

Candidate/control raw differences reach **2.384e−7 T /1.565e−7 RGB**. Every compared row is retained in the [candidate rows](NORTHERN_VEIL_CELL_POLYNOMIAL_PREFLIGHT_2026-10-21_ROWS.json); the evidence JSON includes twelve largest newly worsened rays for each output class, so aggregate averages do not hide new outliers. Instrumentation can perturb native arithmetic; it is not the full uninstrumented raw image.

The separate **uninstrumented full-resolution image** differs from its fresh original-evaluator control in **10 bytes on 10 pixels, each by one level**, out of 14,745,600 bytes. The fresh control matches the older frozen current image exactly. This is not byte parity, a new one-level permission, or Person verification. Broader raw outputs/views/clocks would still require qualification if economics were favorable.

## Complete native cost and stop decision

Original and candidate use the same scratch executable, with `EC_POLY_BASELINE=1` bypassing the entire transform for the control. Corrected authored incident-source projection is used; the fixture's radiance is literal zero. Camera (0,50,75) toward (40,62,110), clock 7.25, viewport 2560×1440, four media, original 96 samples/chord. Device timestamps remain disabled.

Two ABBA series, 24 timed frames/run after 4 warm, completion fences around the complete batch. All compilation, diagnostic captures and image comparisons completed before timing. Cold admission is recorded separately in each log and is not part of the warm means. No census, noise comparison, readback, profiler or other GPU workload was launched by this session during timing. Projection/preparation, submission, coefficient creation, guards, miss handling and shader execution are included. Final warm compiles/lowerings/parameter/proof uploads are zero on every run. Shared-desktop thermal/interference effects cannot be excluded; no hardware cause or statistical significance is asserted.

| Series | Original ms/frame | Polynomial ms/frame | Change |
|---|---:|---:|---:|
| 1 | 87.305 | 91.747 | +5.09% |
| 2 | 97.937 | 95.070 | -2.93% |

Pooled **92.621→93.409 ms/frame (+0.85%)**. The proposal's 20% inside gain screen is Astra's engineering suggestion, not a numeric requirement Zach imposed; this result fails any claim of a repeatable substantial gain regardless. No repeated/tuned series, additional cache form or changed approximation is used to rescue it. Offscreen volume-only timing is not inhabited FPS or p95 latency.

Two initial diagnostic launches stopped before experimental shader execution because the new wrapper changed whitespace at the parent's exact-text alpha hook. That scratch integration bug was fixed before successful census/candidate runs. No pre-fix output or timing enters these results, and no production source was touched.

## Reproduction, evidence and next work

Use fresh external output folders and the current configured app objects, from the ray worktree:

```sh
python3 scratch/probes/ray_transport_noise_polynomial_trial.py --root "$PWD" --witness-only
python3 scratch/probes/ray_transport_noise_polynomial_trial.py --root "$PWD" --output /private/tmp/veil-poly-hits --mode hits --capture
python3 scratch/probes/ray_transport_noise_polynomial_trial.py --root "$PWD" --output /private/tmp/veil-poly-capture --mode polynomial --capture
python3 scratch/probes/ray_transport_noise_polynomial_trial.py --root "$PWD" --output /private/tmp/veil-poly-values --mode polynomial --capture --compare-noise
python3 scratch/probes/ray_transport_noise_polynomial_trial.py --root "$PWD" --output /private/tmp/veil-poly-native --mode polynomial
```

For each capture run, set `EC_CAM=0,50,75,40,62,110` and `EC_CENSUS=<output.bin>`, then invoke its `cf4` binary with fixture `saves/zones/Northern Veil/zone.ecform` and arguments `0123 2560 1440 7.25 <output.rgba> 1`. For paired raw accuracy, save `baseline.bin` using `EC_POLY_BASELINE=1` and `polynomial.bin` with it unset under the same capture directory, then run `ray_transport_noise_polynomial_analyze.py --root "$PWD" --output <capture-directory>`. The analyzer requires the tracked ten-column published reference rows; Opus's source currently emits eight raw columns. To normalize a new raw reference, prepend the corresponding input pixel x/y to each eight-value row and label the ten columns explicitly, validating count/order/fixture before replacing any published dataset. No reference regeneration or published-row mutation was done here.

Capture records remain 64 u32 words/ray:0–15 retain parent metadata/raw outputs;20–23 are calls/hits/preparations/fallback;24–55 are the capped run histogram;56 is same-key domain reset count;57 holds max noise-value error bits only with `--compare-noise`. Timing must use the noncapture build, no dump/census environment, arguments ending `cost 24`, baseline/candidate/candidate/baseline twice. All timing logs and means are retained in [evidence JSON](NORTHERN_VEIL_CELL_POLYNOMIAL_PREFLIGHT_2026-10-21.json); source/fixture/reference hashes and every diagnostic scope are recorded there.

**Shipping account:** Astra's one proposed experiment is executed and closed for this implementation. It is a real value-side ray reduction, with current-scale observed outputs, but it does not earn promotion. No new shipped gain. A faithful profitable complete native consumer remains missing. This does not disprove all cell-value recipes or ray compilation. Before another representation change, isolate the actual occupied-program cost/parallelism and the total preparation/state cost; neither a polynomial identity nor high reuse predicts Metal speed. Concrete coefficients belong at ray/cell lifetime; reusable derivation can live at revision lifetime. A materially different design still needs the full cost/accuracy screen.

Production eligibility/fallback and edit/revision/device lifetime, other views/clocks/clipping/source/overlap behavior, actual inhabited timing/p95 and Person acceptance remain open. None is claimed qualified by this failed inside screen. No new visible behavior or Person action is introduced. The native lane is released after these completed runs. Python syntax and the 8,704-point algebra witness pass; no production suite is claimed rerun for rejected scratch-only work.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-21T13:32:16.064075-07:00`.
