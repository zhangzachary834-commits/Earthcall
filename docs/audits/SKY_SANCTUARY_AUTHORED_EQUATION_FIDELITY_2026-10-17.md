# Sky-Island Sanctuary — authored-equation fidelity

Zach's response to the [Metal before/after gallery](RAY_DOMAIN_TRANSPORT_METAL_VERIFICATION_2026-10-17.md) supplies the acceptance criterion: the brighter before images strike the right aesthetic balance, but that balance may have arisen accidentally from imprecision. He said, **“Faithfulness to authored intent is what matters,”** and made the darker after acceptable only insofar as it is closer to the mathematically exact interpretation of the authored equations. This is not permission to retune illumination to recover the old brightness.

**Result:** for the grouping change responsible for this Sky-Island shading difference, the after renderer is substantially closer to the saved mathematics. Two distinct witnesses support that conclusion: native scalar outputs versus an independent 80-digit evaluator, and full native Engine images versus the old renderer given explicitly grouped equivalent expression trees. This does not certify every shader operation, geometry/transport approximation, or pixel of Earthcall against an exact continuous renderer.

## 1. What the saved sun equation says

The original frozen `saves/zones/Sky-Island Sanctuary/zone.json` contains the sun's field as a Scale node whose left child is the two-term ScalarLeaf

`m(p) = 0.88 + 0.12 sin(0.2x) cos(0.2z)`

and whose right child is a spatial illumination profile `B(p)` containing a 0.24 floor plus localized clamped-distance contributions. The authored meaning is **m(p) × B(p)**. The point is source-local: the Engine evaluates source radiance at `worldPosition - source.position` (`EngineRender.cpp` / the source-radiance WGSL), not at an unrelated world origin.

Before the fix, the leaf string had no outer parentheses. The parent Scale therefore emitted **0.88 + [0.12 sin(0.2x) cos(0.2z)] × B(p)**. The 0.88 escaped the spatial multiplication. Where B is small, this is an unearned baseline of light, not a small f32 rounding discrepancy.

At world point `(-20,0,-32)`, the sun's saved position `(-20,24,18)` yields local point `(0,-24,-50)`. Here all localized contributions are zero, so B=0.24; sin(0)=0. The authored value is therefore exactly **0.88 × 0.24 = 0.2112** under the saved decimal-real interpretation. Native Metal outputs:

| Reference | Value |
|---|---:|
| Independent 80-digit authored evaluation | 0.2112 |
| Before GPU | 0.8799999952316284 |
| After GPU | 0.21119999885559082 |

The older value is approximately **4.17 times** the authored field at this point. This explains a material part of the visibly brighter shading. The iris field has the complementary right-operand case: an additive oscillation could escape multiplication by its spatial profile. The correction applies the authored profile to the whole modulation.

## 2. Native scalar values versus an independent mathematical reference

**Inputs:** the unchanged original JSON; three authored light sources; 1,156 points per source at clock zero (world x/z from −32 to 32 in increments of 4; y = 0, 2, 5, 12). The probe supplies their actual f32 source-local coordinates to the reference, so this isolates expression evaluation rather than claiming exact coordinate-transport arithmetic.

**Native path:** a scratch entry links each full app build's unchanged `SdfWgsl.cpp` and wgpu-native. It compiles a dummy sphere plus the actual saved radiance Piecewise using the production compiler, appends a compute entry calling the generated `lightRadiance(p)`, and reads back unquantized f32 results. Adapter query returns **Apple M5 / Metal (5)**. Default device initialization requests no timestamps. Successful completed runs exit 0 with validation scopes clear. This exercises the production emitter's scalar arithmetic, not the entire material shader at each test point.

**Independent reference:** `high_precision.py` recursively interprets the saved JSON operations itself using mpmath at **80 decimal digits**. It does not call Earthcall's CPU evaluator, parse emitted WGSL, or reuse a shader result to construct the reference. It reads the original saved decimal coefficients; the GPU's parameter quantization is consequently included in measured error. The existing CPU evaluator is a separately reported comparison, not the oracle. Real arithmetic is approximated at 80 digits here; no interval theorem for all inputs is asserted.

Maximum absolute field-value errors over 1,156 points each:

| Source | Before GPU | After GPU | Existing CPU |
|---|---:|---:|---:|
| Sun of Dawn | 0.6688000263 | **9.61070947e-8** | 6.69331760e-8 |
| Heart radiance | 4.58854605e-8 | **4.58854605e-8** | 9.51351362e-9 |
| Aether iris ring | 0.1087989211 | **2.90632301e-8** | 1.43077049e-8 |

Total: **3,468 sampled values per build**. Heart's zero-clock output is an unchanged control; its temporal behavior is not certified by that scalar row. The full-image witness below also covers 0.75 and 2.25.

The first probe attempt omitted the temporal uniform required by Heart's entry-point-specific binding layout and aborted before completing all sources. A second diagnostic version reported that error safely before dispatch. The completed probe supplies the appropriate binding and re-ran all three sources; only the completed outputs supply the table. No production renderer fix or tolerance change was made to obtain these results.

## 3. Independent grouping reference, on the old renderer

A second scratch capture entry links the **pre-fix parent app build (`5bcb63837`)**. In the isolated runtime fixture only, it replaces each multi-term ScalarLeaf with a left-associated Add tree of its original one-term leaves. Constants, variables, functions, term order and authored mathematical meaning remain unchanged. The old emitter already wraps Add children correctly, so this route does not depend on the new `emitScalarForm` outer-parentheses repair. It normalizes all Piecewise channels on the Zone's five FieldNodes, not just the sun equation. Runtime root-ID hydration is preserved; no original save is rewritten or regenerated.

The old renderer with those explicitly grouped inputs produced **zero differing RGBA bytes** against the darker after captures, at **1920×1080**, for each clock **0, 0.75, 2.25**. Camera remained `(22,12,26)` toward `(0,2,0)`, with the same copied Zone/Law/Material/profile closure, `dt=0`, default certificate lane off, and synchronized native viewport capture. The window was floated for visible Metal surface acquisition, as in the earlier pass. No identity keys or signed presence were supplied.

This is an independent route to correct *grouping*, while geometry, material, source composition, quadrature and presentation continue through the production renderer. It is not a wholly independent physically exact renderer. Combined with the high-precision scalar check, it ties the observed darkening to the authored equation rather than a preference for darker images.

## 4. Acceptance and retained boundaries

Zach's mathematical-fidelity criterion is recorded; the old brightness is an aesthetic observation, not a new lighting specification. These witnesses support the after result for this shading change. The human visual boxes are not silently checked, and no brighter coefficients, material responses, exposure, or authored save values were substituted. Generic device arithmetic enclosure, certificate proof findings, Perlin residency assertion and live movement performance remain separate open issues; certificates and ordinary native timestamps remain default off.

**Local retained evidence:** `scratch/verification/ray-domain-metal-2026-10-17-01a148a7/sky-equation-fidelity/` contains `before.json`, `after.json`, completed native logs, `high-precision-report.json`, `high_precision.py`, `native_scalar_probe.cpp`, `reference-capture.cpp`, `image-reference-report.json` and the three native reference PNGs. mpmath was installed only under `/private/tmp/earthcall-sky-fidelity-01a148a7/pydeps`; its version is recorded in the report. These generated artifacts are ignored, like the original local gallery. Original Zone hashes were checked again; production source, original saves, defaults and installed app are unchanged.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T00:37:58-07:00 (PDT).
