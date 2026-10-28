# §6er — native ray/depth agreement exposes a stage arithmetic difference

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28T12:13:50.198456-07:00.

Zach requested the next pass after [§6eq's Screen publication witness](GPU_RAY_TRANSPORT_6EQ_SCREEN_PUBLICATION_2026-10-28.md). His criterion remains **faithfulness to authored equations**, with current-or-better accuracy and measured complete native cost required before the performance checkpoint. This pass executes the device-ray/opaque-depth comparison proposed in that handoff. Firmament's [in-flight counsel](FIRMAMENT_COUNSEL_ON_6EO_TO_6ER_STAGE_FMA_DISCOVERY_AND_6ES_CONNECTED_BRIDGE_2026-10-28.md) arrived during the work; its received bytes are preserved in the evidence archive, independently of his concurrently owned document.

**Outcome:** the original matrix expression produces different fragment and compute rays despite identical recorded matrix/NDC inputs. An experimental explicit nested-`fma` expression makes both stages match exactly in ten native cases, including **all 3,686,400 pixel positions at 2560×1440**, covered by four disjoint row-window jobs. However, its full rendered image differs from original Current by **1,164 RGBA8 bytes, maximum difference one level**. That is a measured change awaiting numerical qualification, not permission to accept it aesthetically. No engine source or numerical contract was changed; no routed integrator or performance gain is established.

## Actual native execution and boundaries

The isolated build uses the same source-matching engine tree as §6eq: `3603add4018961529a91bc9592d76f9a553500b1`. The control links the original `WebGpuRenderer.cpp.o`; the experimental executable substitutes only an external renderer copy, retaining the renderer object's original CMake compiler/optimization/FP/language flags. The diagnostic main is shared by each build's control and experimental executable. Three successive source-bound builds retain their distinct exact main sources and receipts. Shared engine objects and source trees were not grafted or edited.

The copy instruments the real emitted volume fragment WGSL and dispatches compute after the actual opaque pass, before the volume pass. Both stages bind the renderer's same frame globals buffer range, instances/header and completed opaque depth texture. The probe draws real opaque triangles through `drawSolid(..., Blend::Opaque, true)` across half the viewport. Compute reconstructs the same ray/box/depth expressions; its output is diagnostic facts only. Original Current transport still runs in the fragment shader. Authored Northern Veil expressions/source channel and frozen clock 7.25 follow §6eq. The actual clear color here is black `(0,0,0,1)`, as the archived generated main records; §6eq used a different colored diagnostic background. Default inside camera is `0,50,75,40,62,110`; spawn is `0,2,0,0,60,90`; partial footprint is `0,2,-200,300,2,0`.

Each stage has its own independently poisoned and GPU-written storage buffer. Every normal record is 64 bytes: `ro.xyz,t0`, `rd.xyz,t1`, ownership marker, clip status, actual sampled depth, reserved word, box bounds and actual pixel centre. Floats are compared by raw u32 representation. Status distinguishes box-empty, opaque-empty and valid. Trace variants replace the last four words with operands/intermediates and are excluded from ordinary-schema validation. These facts are **not** the existing 48-byte producer-ray ABI.

At full resolution, four separate frozen jobs cover rows `[0,360)`, `[360,720)`, `[720,1080)`, `[1080,1440)`. Global pixel coordinates are preserved; each stage buffer is 58,982,400 bytes, below the diagnostic 64MiB cap. This is full pixel-position coverage, **not** one production whole-frame dispatch or a frame-economics measurement. Each process renders three frozen frames; only final facts/image are retained. Ordinary GPU timestamps are disabled. The default native macOS WebGPU adapter's backend/vendor was not queried.

## The original expression fails the exact stage gate

At 334×190, all 63,460 original-path ray records differ between fragment and compute. Origin, pixel centres, matrix and NDC operands agree. The first observed divergence is the homogeneous product `u.invViewProj * farNdc`; clip endpoints also differ. The initial instrumented fragment image matches its independent original-renderer clear-view control in all RGBA8 bytes. That image equality does not prove uninstrumented f32 intermediates are unchanged.

For the first pixel, fragment direction is approximately `(0.850769699,0.508688152,-0.132012859)`, whereas compute is `(0.850772739,0.508689821,-0.131986201)`; `t1` is respectively 169.062316895 and 169.061767578. Separate matrix-column, viewport, NDC and homogeneous-intermediate trace variants preserve every stage's original ray/status/clip prefix over all 63,460 records. A divided-world-value trace **does perturb** those prefixes (14,383 fragment and 15,068 compute records); it is retained as observer-effect evidence and excluded from inference about the original normal rays.

A Decimal80 calculation independently evaluates the exact-real product of the **recorded f32 matrix and recorded f32 NDC operands**, then homogeneous division, eye subtraction and normalization. Across all 63,460 ordinary-path rays, compute's direction is closer on 63,459 and worse on one. Worst absolute direction-component errors are `5.025849717763128e-5` fragment and `1.2311988156780218e-7` compute. Homogeneous-component maxima `(x,y,z,w)` are:

| Stage | x | y | z | w |
|---|---:|---:|---:|---:|
| Fragment |1.62014e-7 |1.52479e-5 |6.08588e-5 |3.33929e-8 |
| Compute |1.23416e-7 |4.47024e-8 |1.18814e-7 |5.82065e-11 |

This reference excludes the authored camera's projection/inversion derivation and prior rounding. It is **not** a full camera or optical RGB/T reference and does not qualify the ordered candidate. Compiler contraction/reassociation is a plausible explanation; emitted MSL/ISA and a specific Naga/Tint/Metal or quad-SIMD cause were not inspected.

## Ordered-matrix experiment and actual opaque geometry

The candidate replaces far unprojection and `worldAtDepth` matrix multiplication in both stages with:

```wgsl
fn ecCanonicalMatVec(m:mat4x4<f32>,v:vec4<f32>)->vec4<f32>{
  return fma(m[0],vec4<f32>(v.x),
         fma(m[1],vec4<f32>(v.y),
         fma(m[2],vec4<f32>(v.z),m[3]*v.w)));
}
```

`canonical-*` is an experiment label, not an adopted project-wide canonical policy. The gate remains bit-exact; no tolerance was widened. WGSL permits floating-point reassociation, and its `fma` operation is not required to be fused: these captures establish the observed device/case result, **not a portable guarantee of evaluation order or cross-stage identity**. See the primary [WGSL floating-point rules](https://www.w3.org/TR/WGSL/#reassociation-and-fusion) and [fma builtin specification](https://www.w3.org/TR/WGSL/#fma-builtin).

| Native case | Ray/depth result | Original-Current image delta |
|---|---|---|
| Inside, clear, 334×190 |All 63,460 complete records identical |232 bytes, max 1 |
| Inside, front opaque geometry |All identical; 31,730 depth-clipped |15 bytes, max 1 |
| Spawn, front opaque geometry |All identical; 31,730 opaque-empty, 31,730 valid |2 bytes, max 1 |
| Depth just below branch threshold |All identical; 31,730 depth-branch, none shortened |161 bytes, max 1 |
| Depth just above branch threshold |All identical; 31,730 near-far values ignored |161 bytes, max 1 |
| Four full-native row windows, front geometry |921,600 identical records/window; 460,800 clipped/window |Each full captured image: 1,164 bytes, max 1 |
| Partial proxy footprint, 334×190 |9,509 visited/valid identical; compute classifies 53,951 box misses |No original-image qualification claimed |

All ten ordinary ordered cases have zero ray, clip, depth, status, ownership or finite-value gate failures. The partial case preserves poison where fragment rasterization never runs; it does not claim equality to nonexistent fragment rays. Fresh unmodified-renderer controls support the five small image deltas and full-native delta. The changes have **no authored-reference verdict yet**. Old ray references cannot simply be attached to these different directions/clips.

## Native negative controls and suite accounting

Four native mutations successfully execute (native exit 0) and are rejected by their intended independent earlier subgate (validator exit 2): pixel-corner addressing → pixel-centre mismatch; ignored opaque depth → depth-sample mismatch; removed compute writer → missing compute ownership; removed fragment writer → missing fragment ownership. These mutations use **legacy arithmetic**, whose full ray gate already fails. They test input/ownership subgates that pass in the normal legacy case; they are not four mutated otherwise-passing ordered programs.

The retained 33 native executions comprise ten ordered matches, six ordinary legacy exact-gate failures, four targeted subgate mutations, six trace-only variants and seven original-renderer controls. Native/shader/device failures do not count as successful negative controls. The full raw stage hashes are identical for the ordered fully visited cases; for partial coverage the validator independently checks visited records, compute classifications and unwritten poison.

## Durable evidence and CPU replay

[Machine record](GPU_RAY_TRANSPORT_6ER_DEVICE_RAY_DEPTH_2026-10-28.json), [compact evidence ZIP](GPU_RAY_TRANSPORT_6ER_DEVICE_RAY_DEPTH_2026-10-28.zip), [build/capture/validator](../../scratch/probes/ray_transport_6er_ray_setup.py), [independent matrix reference](../../scratch/probes/ray_transport_6er_matrix_reference.py), [lossless fact codec](../../scratch/probes/ray_transport_fact_codec.py), [replay](../../scratch/probes/ray_transport_6er_replay.py).

The approximately 32MB archive retains all final fact words in 35 deduplicated raw-SHA-addressed arrays, raw RGBA8 images, exact sources, compiler/link arguments, before/after binary/build/fixture fingerprints, native logs, shader dumps and comparisons. Fact packing uses word-column modulo-u32 second differences, zigzag and bounded zlib; it preserves raw bits, including signed zero/NaN encodings, rather than approximating float values. Random full-u32 round trips and malformed/header/trailing/extent refusals pass. It is a diagnostic archive codec, not an Earthcall save subsystem.

Shader dumps retain the last `volumePipeline` encounter per case, not every compiled variant or emitted ISA; the compute source prefix is asserted invariant across encounters. Numeric captures establish execution observations. Replay reconstructs the source derivation, checks build/control provenance and every member/decoded-array identity, repeats all exact gates and image deltas, checks four disjoint full-native windows, and recomputes all-ray Decimal80 results. It is **CPU replay of stored native evidence**, not another GPU run:

```sh
python3 -B scratch/probes/ray_transport_6er_replay.py  --record docs/audits/GPU_RAY_TRANSPORT_6ER_DEVICE_RAY_DEPTH_2026-10-28.json  --archive docs/audits/GPU_RAY_TRANSPORT_6ER_DEVICE_RAY_DEPTH_2026-10-28.zip  --output /tmp/6er-replay.json
```

Fresh reproduction starts with the script's `build --build-root <source-matching-checkout> --work <fresh-external-dir>`, followed by the source-bound receipt's capture dimensions/camera/scene/mutation. Native lane ownership must be checked before rerunning; external diagnostics have per-process timeouts. No raw executable/object or oversized single fact array enters Git.

## Reply to Firmament and bounded next work

Your separate production-channel consumer and stronger full-schedule poison controls are useful next preparation. The following tightenings preserve Zach's authored-faithfulness criterion:

1. **Numerical contract before promotion.** The ordered expression is an observed setup candidate, not WGSL-guaranteed order, proven Metal compiler bug, or automatically more accurate full camera. Decimal80 compared original compute/fragment arithmetic on rounded inputs; it did not establish the ordered candidate's optical quality. One original compute ray is worse. If a general matrix policy is needed, shared mathematical lowering owns that meaning and Screen reuses it; no new renderer mathematical office/API is adopted here.
2. **Input identity before reference reuse.** Old 32,784/1,094 ray references cannot qualify new ordered directions or opaque clips. The 230,400 frozen rays are stride-four samples of the **2560×1440 viewport**, not a 640×360 viewport. Preserve pixel centres, source clock, directions and clipping, or derive/bind new references explicitly.
3. **Do the preparatory run without lowering admission.** Full-population own-method parity, the correctly bound 1,094 pilot references and matched complete cost are useful on the unchanged old capture. They do not establish all-row max/p99 numerical quality for the other 229,306 rays. Prior holdout failures prohibit relabelling pilot acceptance as whole-population authored accuracy. The full dual-resolution gate remains open.
4. **Complete writes and exact ABI.** Translate the 64-byte facts into the existing 48-byte ray ABI deliberately; reset counts and map valid/empty/refused states explicitly. Zeroing empty rays alone does not prove W2. Every valid/refused/capacity/unknown/revision path must publish a whole answer or refusal; initialize pending slots safely. Necessary semantic initialization is legitimate paid work, even though diagnostic clears should be removed from timing.
5. **Production optics beyond D/X/E.** Reuse production lowering without losing source/scattering/chroma/phase/direction/clock preparation. A fixture-gated zero-source Northern Veil experiment may be narrower, but must state that restriction. Producer density-pool and production all-channel offsets still need translation. Consumption and final publication remain whole-ray commitments.
6. **Profile before assigning CPU causation.** Source inspection identifies inner binding maps, eager noise and repeated ASTs as candidates, not measured allocator contention, 23% heap traffic or a guaranteed 20-second job. Do not start another engine rewrite from unmeasured multipliers. Removing a dead allocation is footprint reduction until matched complete timing proves cost savings.

Next pass can prepare the unchanged-capture full-poison/parity/pilot/cost run, or qualify this changed ray setup with independently bound numerical references. Neither path should quietly merge its evidence into the other. A connected experiment must then execute setup → producers → production-faithful consumer → publication, with mixed refusal, opaque composition and clock/revision/resize/device controls.

## Shipping account

| Gate | State after this pass |
|---|---|
| Whole distinct-population authored accuracy and complete economics |OPEN; no reference rows added or clean GPU timing |
| Production Screen/validity bridge |OPEN; scratch publication and ordered ray/depth subset witnessed, actual routed consumer unconnected |
| Inhabited-frame performance and Person acceptance |OPEN; no installed or visible world change |

The full 230,400-ray reference remains unlaunched at the previous approximately eight-hour estimate; existing pilot coverage remains 1,094 old-input rays. Diagnostic poisoning, uploads, shader instrumentation/readback and four window jobs are not a shipping schedule. Native setup equality is progress, not a percentage of completed shipping performance. No engine/save/app/default/lighting/resolution edits, no new Person-visible feature or check, and no push by this pass. Existing Person checks remain open. All native captures are complete; **GPU lane RELEASED**.

Background-description correction recorded during [§6es](GPU_RAY_TRANSPORT_6ES_CAMERA_AND_OPTICAL_PILOT_2026-10-28.md#first-correct-6ers-background-description); captured data and numerical results unchanged. Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28T12:38:11.783456-07:00.
