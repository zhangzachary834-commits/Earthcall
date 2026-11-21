# §6es — camera and actual Current transport, judged against bound numerical targets

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28T12:38:11.783456-07:00.

Zach asked to continue [§6er's ray/depth witness](GPU_RAY_TRANSPORT_6ER_DEVICE_RAY_DEPTH_2026-10-28.md). His requirement is mathematical faithfulness, with approximately current-or-better accuracy and substantial complete native performance before a checkpoint; a pleasant-looking changed image cannot settle that question. This pass supplies the missing independent camera calculation and new native float-output/reference bindings. It does **not** implement Firmament's proposed connected Mode2 consumer or adopt a matrix policy.

**Outcome:** the ordered arithmetic is closer to an explicit ideal camera target on mean/p99/max direction error over all63,460 pixels of the334×190 view. It is still worse at20,287 individual pixels. In a separately identified **80-pixel clear-view optical pilot**, actual native Current outputs using the ordered rays improve all six RGB/T mean/p99/max metrics at both reference resolutions against **one common ideal-camera reference**. This is useful evidence for continuing the candidate; it is not whole-image numerical admission, a portable arithmetic guarantee, a new transport method or a performance result.

Seven new native executions, five capped CPU reference executions, two camera-precision analyses and the complete stored-evidence replay pass their stated expectations. One native missing-optical-write mutation is correctly rejected after successful GPU execution. No engine source, save, installed app/default or live-world lighting/resolution changed.

## First: correct §6er's background description

§6er incorrectly said its clear color followed §6eq's `(.125,.25,.5,1)`. Both §6er and this pass actually use **black `(0,0,0,1)`** in the generated main; the source-bound originals in their archives establish that. This correction changes no captured bytes, ray counts or before/after differences. §6eq's distinct colored-background publication test remains as recorded. The prior audit is corrected explicitly and Intercom receives an append-only correction.

## What ran and what was recorded

The same source-matching engine tree `3603add4018961529a91bc9592d76f9a553500b1` and original compiler/FP flags from §6er are retained. An external renderer copy and common main form the bridge executable; an independent control links the **original unmodified renderer object** with that exact common main. No shared engine object, source or build tree was replaced. The concurrent Firmament counsel commit `c2538c14c` changes documentation only and leaves this source identity unchanged.

The instrumented real volume fragment runs Current unchanged. Immediately before `let alpha = 1 - transmittance` and output discard, it writes **actual `(T,Lr,Lg,Lb)`** into words12–15 of the existing64-byte fragment facts. T is captured directly, rather than reconstructed from rounded alpha. The first48 bytes still carry origin, direction, interval, ownership, status and sampled depth; compute keeps the original diagnostic tail. These records intentionally have a different final-word schema from §6er and must not go through its ordinary full-record comparator.

Native cases are clear/front × original/ordered/original-renderer-control, plus a clear missing-optical-write mutation. All are334×190, inside camera `0,50,75,40,62,110`, authored source enabled and clock7.25. The front scene draws actual opaque triangles over the left half. Each process renders three frozen frames; only final facts/images are retained. The default native macOS WebGPU adapter's backend/vendor was not queried; ordinary timestamps are disabled. Diagnostic writes, poison uploads, compilation and readback prevent any clean-cost interpretation.

All four positive instrumented cases have63,460 visited valid records with finite T/RGB and T in[0,1]. Their ray/status/depth prefixes and final RGBA8 images match their respective §6er captures exactly: zero observer differences. Original arithmetic matches fresh original-renderer controls in every image byte. Ordered arithmetic retains the previous232-byte clear difference and15-byte front difference, maximum one level. These pixel equalities do not prove unchanged unobserved f32 intermediates.

For an independent observable bridge, the saved float outputs reproduce **all63,460 black-background pixels** in each clear case exactly after the existing discard rule and RGBA8 quantization. The front cases also reproduce their31,730 independently known black-background pixels on the unobstructed right half; no opaque-background reconstruction claim is made for the other half. Thus190,380 pixel predictions/761,520 bytes match across the four cases. This validates the stored output's connection to visible pixels, not the correctness of the integration against authored math.

The negative mutation suppresses only the optical-tail write, leaving native execution and image generation successful. The ordinary ray tail is not a valid `(T,L)` answer; the optical validator rejects `invalid optical value` (validator2, native0). No device/shader failure is counted as a successful negative.

## Independent camera calculation

The prior capture already exposed both arithmetic paths' actual directions. A new Decimal calculation derives camera rays **directly** from an ideal60° vertical perspective, exact viewport aspect/pixel centres and the exact integer eye/look vectors, using an orthonormal look-at basis and `tan(30°)=1/sqrt(3)`. It does not reuse the uploaded inverse matrix or its NDC rounding. This is an explicitly stated diagnostic target for the known probe-camera source, not a new universal interpretation of arbitrary authored camera matrices.

A second conditional target evaluates the uploaded f32 inverse matrix and recorded f32 NDC in high precision, preserving the distinction from §6er. Both targets are evaluated at80 and110 decimal digits. All reported binary64 metrics and closer/worse counts agree between precisions. Analytic centre-ray and exactly symmetric corner/orientation controls pass. The ordered homogeneous trace preserves its entire original48-byte prefix at all63,460 records, so it adds no observed prefix perturbation.

For each direction, error is the maximum absolute difference across its three components; the distribution is over pixels, not angles:

| Target / arithmetic | Mean | p99 | Maximum |
|---|---:|---:|---:|
| Ideal camera / original fragment |1.856288e-5 |5.285236e-5 |6.732618e-5 |
| Ideal camera / ordered |1.345325e-5 |1.740079e-5 |1.752582e-5 |
| Rounded operands / original fragment |1.262552e-5 |4.006644e-5 |5.025850e-5 |
| Rounded operands / ordered |3.423689e-8 |8.302690e-8 |1.233161e-7 |

Relative to the ideal target, ordered is closer on43,144 pixels, worse on20,287 and equal on29. Relative to the rounded-operand target it is closer on all63,460. The much smaller latter errors do not erase the upstream matrix/camera approximation. Mean/max improvement is not pointwise dominance, a formal interval enclosure or a cross-platform WGSL guarantee.

## Fresh optical references: own input versus common intended target

The selected sets are frozen **before** evaluating their references:64 uniform paired-valid pixels, the eight largest direction changes and up to eight changed-RGBA pixels. Clear and front each select80 shared pixel positions for their original/ordered pair. They are diagnostic stress pilots, not unbiased full-image estimates. The union is88 pixel positions.

Each native path receives its **own** reference input with the captured origin/direction/t0/t1 and actual T/RGB. No old1,094-ray reference is transplanted. Opus's original-AST generator, using the already validated closed §6ep binding-map derivation, evaluates each ray at2^16 and2^15 nested resolutions. Its unused four native-method diagnostic column groups all contain the same Current tuple; four different GPU methods were **not** executed. Exact source, compiled binary/build and input/output hashes, clock, before/after identity, logs and process bounds are retained.

Four80-ray own-input runs take9.91,10.24,12.06 and12.12 seconds respectively on eight threads, each capped300 seconds. Maximum finest/half differences across them are6.514e-8 T and3.805e-8 RGB, below the unchanged diagnostic1e-6 convergence guard. That guard is neither an author tolerance nor proof of exact convergence.

**Own-ray results are deliberately preserved even where less favorable:** the ordered clear RGB p99 is7.53732e-4 versus original7.52313e-4, a small increase, at the finest level; front means also increase slightly. These measure each integrator result's error relative to a *different* exact ray and cannot alone decide which image follows one common camera target better. They also prevent claiming a universal six-metric integration improvement from changing matrix arithmetic.

For the common-target experiment, the clear pilot gets a **fifth, separately identified reference**. It uses the direct Decimal80 ideal-camera directions and exact fixture-union slab intersection: bounds `[-165,8,29]` to `[165,136,184]`, extracted from the four frozen authored spatial fields. Those bound coordinates are exactly f32-representable; a changed fixture outside this assumption refuses. Direction and clip are rounded once to binary64 for the existing CPU generator. The inherited `native_case` and repeated output columns identify the parent captured values for diagnostics; no ideal-camera GPU run occurred. It is an additional reference, never a relabelled own-ray row.

This run takes10.34 seconds, with finest/half differences6.027e-8 T and3.543e-8 RGB. Against that **same common target**, finest-resolution errors are:

| Metric | Original Current rays | Ordered Current rays |
|---|---:|---:|
| T mean |2.017195e-4 |1.938735e-4 |
| T p99 |2.762851e-3 |2.752360e-3 |
| T maximum |2.762851e-3 |2.752360e-3 |
| RGB mean |8.072673e-5 |7.641574e-5 |
| RGB p99 |7.556658e-4 |7.534082e-4 |
| RGB maximum |1.789260e-3 |1.782811e-3 |

All six improve at **both** reference resolutions. This holds for80 selected clear pixels; it does not qualify the full63,460-pixel view, the full-native viewport, other cameras or ideal opaque-plane clipping. The front pilot has only own-input references. No result here substitutes for the established whole-population numerical gate.

Five runs supply400 reference rows for336 distinct origin/clock/pixel/direction/clip inputs; some clear/front rays coincide. These are new experiment inputs. They add **zero** coverage to the old230,400-ray capture's all-row reference obligation; that population still has its previously qualified1,094-ray pilot and an unlaunched full job. Do not combine those denominators.

## Evidence, binding controls and replay

[Machine record](GPU_RAY_TRANSPORT_6ES_CAMERA_AND_OPTICAL_PILOT_2026-10-28.json), [approximately6.9MB evidence archive](GPU_RAY_TRANSPORT_6ES_CAMERA_AND_OPTICAL_PILOT_2026-10-28.zip), [native build/capture and input preparation](../../scratch/probes/ray_transport_6es_optical_setup.py), [independent camera math](../../scratch/probes/ray_transport_6es_camera_reference.py), [replay and error gates](../../scratch/probes/ray_transport_6es_replay.py). The archive retains exact generated C++/WGSL sources, compiler/link arguments, build/binary/fixture receipts, all native fact bits and RGBA8, reference source/build metadata, inputs/results/logs and numerical summaries. It uses §6er's bounded lossless fact codec and depends explicitly on its hashed JSON/ZIP for prior observer/camera facts. Shader dumps are last encountered source texts, not emitted ISA or a complete pipeline compilation trace.

Replay independently regenerates native-to-reference selections from the saved facts/images and regenerates the ideal target from the fixture. The actual binding guard rejects four altered capture documents (clock, direction, native output, truncated rows), even if a caller were to re-sign those input documents. Three reference-receipt mutations (foreign input, timeout, false convergence) are also rejected. These are source/input/provenance controls, not a claim that hashes can establish mathematical truth or detect fabricated execution evidence.

Replay checks native original/experimental link identity, before/after source/binary/fixture identity, successful authored-clock logs, all decoded arrays/images, the intended missing-writer failure, selected native rows, exact reference derivation and bound input/output/log receipts, convergence, all numerical statistics, pixel reconstruction and80/110-digit camera stability. It does not execute the GPU or recompute the expensive optical reference:

```sh
python3 -B scratch/probes/ray_transport_6es_replay.py  --record docs/audits/GPU_RAY_TRANSPORT_6ES_CAMERA_AND_OPTICAL_PILOT_2026-10-28.json  --archive docs/audits/GPU_RAY_TRANSPORT_6ES_CAMERA_AND_OPTICAL_PILOT_2026-10-28.zip  --parent-record docs/audits/GPU_RAY_TRANSPORT_6ER_DEVICE_RAY_DEPTH_2026-10-28.json  --parent-archive docs/audits/GPU_RAY_TRANSPORT_6ER_DEVICE_RAY_DEPTH_2026-10-28.zip  --output /tmp/6es-replay.json
```

Fresh captures use the optical script's source-matched external `build` and `capture` commands; the capture work directory must exist. CPU `prepare` freezes inputs; `reference` uses the previously source-bound generator binary. `prepare_ideal` is an explicit Python helper, not a native mode. GPU lane ownership must be checked before rerunning. No suite-wide engine test or native timing claim follows from this diagnostic.

## Next work and shipping account

The ordered setup now has a numerical reason to continue beyond cross-stage self-agreement. It is still a candidate. General mathematical camera/lowering policy belongs with shared mathematics; Screen binds/executes it. This pass adds no renderer-specific ontology, class, policy or hidden authored state.

A useful bounded next performance experiment can proceed **without adopting the matrix change**: run fully poisoned routed/unfused parity on the unchanged230,400 captured rays, apply its correctly bound1,094 pilot references, and measure matched complete native cost if those preparatory gates pass. Label the missing all-row references honestly. This tests the real distinct workload before another expensive full-reference commitment. In parallel architectural terms, the actual Screen bridge still needs the real production-faithful consumer, explicit ray/parameter ABI translation, whole-ray fallback/publication, and revision/clock/resize/device validity. None is implemented by the present float-output witness.

| Shipping gate | Status |
|---|---|
| Whole distinct-population authored accuracy + complete economics |OPEN; clear common-target pilot only, no clean timing |
| Production Screen/validity connection |OPEN; native scratch setup/output witnessed, Mode2 unconnected |
| Inhabited-frame/Person acceptance |OPEN; no installed or visible feature changed |

No new Person-visible check is introduced; existing checks remain open. No engine/save/app/default/lighting/resolution or numerical contract changed, no push. All native and bounded reference executions are complete; **GPU lane RELEASED**.
