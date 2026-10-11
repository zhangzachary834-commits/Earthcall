# Northern Veil: saved equations, actual overlap and executed work

**Commission:** Zach asked to model these particular aurora sheets and their overlaps from the rendering compiler and the saved equations, after asking whether the remaining roughly 200 ms was necessary mathematics or avoidable work. This is analysis, not a renderer change or a new performance result. Zach originated mathematical sharing, compile-once/incremental repair, native-resolution fidelity and minimum necessary evaluation. The existing task attributes the Northern Veil authorship to Gemini with Zach. Codex extends that human direction with this support/transport/compiler model.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T00:47:14-07:00.

**Parent:** [current Northern Veil task](../Agenda/Tasks/Specific%20Tasks/Performance%20and%20Runtime/Northern_Veil_volumetric_frame_cost/Northern_Veil_volumetric_frame_cost.md) and [Sixth-one Sun's retained preparation record](../../agent%20intercom/communication-threads/Poltergeist_II_Gemini_And_Opus_Share_A_Checkout_2026-10-09.md#sixth-one-sun-independent-gpu-clock-preparation-retained).

## Evidence and scope

Inspected working tree at `ffe0681f668d91ce361bed8fb3f7de9bc21f89c6`; uncommitted renderer changes are the previously retained continuation. Other agents' Storage/test changes were preserved. Read the current `saves/zones/Northern Veil/zone.ecform` without modifying it: SHA256 `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`. Its MessagePack wrapper's `MigrationRoot` equals the existing decoded benchmark fixture exactly. This diagnostic read is not a save-codec change or per-frame serialization.

A scratch CPU executable linked against current app objects decoded that save through FieldNode, lowered each medium through the production compiler, composed both camera variants, built current density proofs and emitted inspection WGSL/metadata. No GPU device, application window or performance run was started. A scratch Python model extracted the actual AST coefficients and verified each shape against the generator helper's mathematical structure after equivalent omitted-zero defaults and f32 normalization. It never called `build_zone()` or wrote a save.

Geometry below models real-valued support envelopes. Sampled curve separation is supplemented by a derivative bound; this is an analytical argument with ordinary numerical evaluation, not an outward-rounded GPU certificate. The interactive diagram omits turbulence, optical transport, opaque depth, proof jumps and opacity termination. Its ray dots reproduce the real-valued construction of the accepted lattice before those exits; they are not GPU instrumentation or exact f32/pixel witnesses. Existing native fidelity evidence retains its earlier date/scope.

Scratch evidence: `/private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/aurora_model_dump.cpp`, `build_aurora_model.py`, `aurora_geometry_model.py`, `aurora-compiler-model.json`, `aurora-equation-model.json`, `aurora-density-witnesses.json`, four member evaluator WGSL files, `aurora-inside.wgsl`, `aurora-outside.wgsl` and `aurora-time-prepare.wgsl`. The inline diagram's controls were exercised in browser preview, with no warning/error logs, and its layout checked at a 320px viewport. No engine/source/save edits, commits or pushes.

## One sheet, factored by its actual dependencies

Let `q = worldP - origin_j`; its x/y/z components are world-unit displacements, not normalized by scale. Define `clamp01(v)=clamp(v,0,1)`. Every curtain has the same structural form, with different constants:

```
X(x) = clamp01(1 - |x|/W)
B(y) = clamp01(0.25*(y+H))
Y(y) = B(y) * clamp01(1 - (y+H)/(2H))
f(x) = A1*sin(k1*x) + A2*cos(k2*x+0.5) + A3*sin(k3*x+1.2)
Z(x,z) = clamp01(1 - |z-f(x)|/w)
F(x) = 0.68 + 0.20*cos(r*x) + 0.12*cos(2.2*r*x+0.8)
N(q) = clamp01(0.70 + 0.30*cnoise3(n*q))
S(q) = (X(x) * (Y(y) * Z(x,z))) * (F(x) * N(q))
u_j(t_j) = 0.85 + 0.15*cos(rate_j*t_j)
```

This notation preserves the authored product hierarchy in S. It explains meaning; it does not authorize reassociation of the compiled f32 arithmetic. Shape/density, folds, fluting and noise have no `t`. Only emission's pulse reads `t`. Phase reads directions, which change with camera/source placement. Thus static shape does not mean the final image is static.

| Sheet | Origin | W / H / w | A1 / k1 | A2 / k2 | A3 / k3 | Fluting r | Noise n | Pulse rate |
|---|---|---|---|---|---|---:|---:|---:|
| Emerald | (0,48,75) | 75 / 16 / 3.2 | 15 / .026 | 5.5 / .068 | 2 / .16 | .30 | .08 | .32 |
| Cyan | (15,56,140) | 70 / 17 / 2.8 | -14 / .028 | 5 / .075 | -2 / .18 | .35 | .09 | .40 |
| Violet | (-5,78,82) | 75 / 16 / 3.8 | 13 / .024 | 5 / .060 | 1.5 / .14 | .24 | .07 | .24 |
| Crimson | (22,72,68) | 32 / 30 / 3 | 9 / .038 | 3.5 / .095 | 1.5 / .18 | .28 | .10 | .36 |

The one shape feeds independently authored channels:

```
D_j(q)       = d_j*S_j(q)
sigma_t_j(q) = a_j*S_j(q)
sigma_s_j(q) = b_j*S_j(q)
E_j(q,t_j)   = S_j(q) * e_j * u_j(t_j)
Phi_j        = max(1 + 3*g_j*dot(wi,wo), 0)   [Violet: identity 1]
```

| Sheet | d / a / b | Medium chroma C | Emission vector e | Phase g |
|---|---|---|---|---:|
| Emerald | .75 / .10 / .04 | (.10,.98,.45) | (.010,.092,.032) | .40 |
| Cyan | .65 / .09 / .03 | (.08,.88,.98) | (.005,.070,.082) | .25 |
| Violet | .55 / .08 / .02 | (.88,.20,.95) | (.066,.010,.068) | identity |
| Crimson | .50 / .08 / .02 | (.98,.18,.32) | (.068,.012,.022) | .30 |

D is the positive-density admission test; the explicit sigma_t is not multiplied by D again. The marcher clamps active extinction to at least `1e-6`, scattering/emission to nonnegative values, and retains channel independence. Sharing does not make an edit to one channel edit the other authored channels.

## Three different meanings of overlap

**Admission-box overlap:** `VolumeDensityBinding` treats FieldNode.scale as half-extents. The generator's apparent full-span comments do not override that implemented contract. World bounds are `origin +/- abs(scale)`:

| Sheet | Box X | Box Y | Box Z | Positive-support Y | Conservative support Z |
|---|---|---|---|---|---|
| Emerald | [-160,160] | [12,84] | [29,121] | (32,64) | [49.3,100.7] |
| Cyan | [-135,165] | [18,94] | [96,184] | (39,73) | [116.2,163.8] |
| Violet | [-165,155] | [42,114] | [36,128] | (62,94) | [58.7,105.3] |
| Crimson | [-48,92] | [8,136] | [32,104] | (42,102) | [51,85] |

All six box pairs overlap; all four boxes share a region. These conservative boxes contain considerable empty space. A geometric ribbon occupies at most `(2W)*(2H)*(2w)` volume, even though its center bends in z. Relative to its actual admission box, this is Emerald 1.449%, Cyan 1.329%, Violet 1.721%, Crimson 1.786%. Noise may suppress support further and cannot enlarge it. These are volume fractions, not pixel/sample/time fractions and not a promised removable fraction of current work.

**Actual geometric sheet intersection:** at a world x shared by two ribbons, their possible z intervals intersect only if `|center_i(x)-center_j(x)| < w_i+w_j`, and their positive y intervals must intersect too. Cyan is separated from every other ribbon by the conservative z bounds alone. Violet/Crimson have overlapping axis-aligned support bounds, but their curved bands remain separated.

For the curved test, a 20,001-point x sweep covers the common x interval. A derivative bound `L = sum |A*k|` over both curves makes the band-gap function L-Lipschitz. Subtracting `L*gridSpacing/2` from the minimum observed gap supplies the real-valued separation estimate. Violet/Crimson's estimated minimum gap is 6.26823 units; the derivative allowance leaves 6.26541 units. Before using this in the GPU exact stage, implement outward rounding/numerical uncertainty, dependencies and native parity. This diagnostic does not install a proof.

Emerald/Violet's geometric bands intersect, restricted vertically to Y=62..64. Emerald/Crimson intersect narrowly; the inspected minimum band gap is about -0.14242 at world X=24.2272. The production CPU density evaluator, including noise, confirms positive intersection witnesses: world (-74.9,63,62.72732) gives Emerald/Violet densities about 0.000008899/0.003278; world (24.2272,50,75.83955) gives Emerald/Crimson about 0.002424/0.004946. Other two densities are zero at each witness. These point witnesses establish actual density intersections, not native pixel parity. No triple or quadruple geometric intersection occurs in this model because Cyan is separate and Violet/Crimson are separate.

**Camera-ray overlap:** separate spatial sheets can contribute to the same pixel at different depths. The rear Cyan ribbon can still be attenuated by a nearer curtain. Spatial nonintersection therefore does not permit dropping a medium from a pixel, merging colours without transport, or removing authored admission-box event positions. In exact mode, those events remain part of the retained quadrature layout even if nearby density vanishes.

## The compiler's actual retained sharing

Current production lowering emits one shared S evaluator per sheet. D, sigma_t, sigma_s and E read that result. Each member evaluator has one `cnoise3` call site, two spatial sine and three spatial cosine call sites; cheap zero factors can return before later work/noise. Those are emitted operations, not instruction timings or a guarantee about driver optimization.

The other three sheets cannot borrow its numeric value: local origins, fold constants, frequency, scale and noise inputs differ. A shared implementation of cnoise3 is not a shared cnoise3 result. Equality must include values and coordinate/clock domains.

The separate preparation module contains three eligible cosine recipes per sheet, corresponding to the three authored emission components. They have equivalent current pulse values but distinct slots. Thus the present stage reduces these from per-sample operations to three scalar transcendental recipes per medium per frame; it has not reduced them to one recipe per distinct pulse. Twelve once-per-frame recipes are not a credible explanation of the remaining roughly 200 ms.

Parameter layouts contain 64/64/63/64 floats respectively, including the derived output slots. Current 32k-target proofs have grids 80x18x23, 77x20x23, 80x18x23, 42x38x22 and prove 96.38%, 96.67%, 96.11%, 96.46% of cells empty. GPU residency avoids repeated packing/uploads; shaders still read the resident data while rendering.

Demand-first nested sharing remains a generic open item. For this save there is one selected shared shape per sheet and density itself consumes it. Do not project the generic eager-helper limitation into a claim that many unused shared groups dominate Northern Veil.

## The fused ray integral

For each covered pixel, the current fused shader intersects the union admission box, clips against opaque scene depth, intersects each of the four independent boxes, and sorts their eight entry/exit events. Every occupied segment chooses spacing from the finest active clipped box chord divided by 96:

```
spacing = min(chord_j/96 over admitted boxes present in segment)
count = clamp(ceil(segmentLength/spacing),1,96)
sample_i = segmentStart + (i+0.5)*segmentLength/count
```

At each visited sample, grid walking asks whether every box holding it has a proven-empty cell and jumps only when that is true. Otherwise it tests each holding medium's proof/density independently. Positive media contribute to one summed extinction and one summed source:

```
sigma = sum(max(a_j*S_j,1e-6) over density-positive media)
J = sum(E_j + C_j * Li_j * max(b_j*S_j,0) * Phi_j)
T_new = T_old * exp(-sigma*stepLength)
L_new = L_old + J * ((T_old-T_new)/sigma)
C_out = L_final + T_final*C_scene
```

The zero-sigma branch retains `L += T_old*J*stepLength`. One attenuation/integration update occurs per accepted combined sample; it is not four independent full integrators. Multiple active shape values may still be required. Emission/extinction ratios are spatially constant for an isolated sheet away from the extinction floor, but this does not solve its optical-depth integral or direction-dependent scattering; overlapping differently coloured sheets do not have a constant combined ratio. Analytic integration remains research, not an implemented shortcut.

No curtain has an authored occluder SDF in this save. The compiled visibility evaluator is identity 1: the generic 24-step visibility marcher is **not** Northern Veil's current inner workload. The saved source has no radiance/chroma/angular AST. Current single/fused marchers use identity defaults for those absent ASTs; they conditionally call lightChromaEval only when an authored chroma AST exists. Consequently the saved legacy black `light.color` alone does not make this compiled volume source term zero: its fallback evaluator reading incidentColor is bypassed by that condition. This source-inspection discrepancy is recorded, not silently repaired; changing it would alter the retained image and requires separate semantic/fidelity review. The model follows current execution: with the admitted source and no occluder, Li is identity and scattering/phase remain demanded at positive-density samples.

## Why the inside camera matters

The benchmark inside camera is `(0,50,75)` looking toward `(40,62,110)`. It is inside Emerald/Violet/Crimson admission boxes but outside all four geometric sheet envelopes at its starting point. For Emerald, local z=0 while the fold center is about 6.69, outside w=3.2; Violet's local y=-28 is below its lower boundary; Crimson is outside its folded band; Cyan is distant. The production CPU evaluator reports all four densities exactly zero at this eye position.

Current renderer selection disables support trimming whenever the eye is inside **any** admitted authored box. The inside variant still uses zero proofs/grid walking but omits the outside variant's support-interval prepass and prefix/tail clipping. This conservative choice preserved native parity after an earlier always-on helper experiment failed; it is not an ontological rule that all empty air inside a box must remain expensive. Improving proof-based ray admission/prefix handling inside boxes while preserving event/lattice positions is a concrete hypothesis worth testing.

The interactive inside central-ray model produces 208 accepted lattice positions before skips and 29 positions intersecting a geometric sheet envelope. Those counts apply to one ideal central ray with no depth/proof/cutoff, not the frame, GPU counters or a new cost measurement. The central ray is a direction through the camera target; a finite even-sized framebuffer does not have a pixel exactly on that direction.

## Computational model and next hypotheses

```
Frame cost ~ native/driver/command overhead
           + coveredPixels * (ray/event setup
             + sum(visited samples: box/proof traversal
                   + unproven media's support/shape evaluation
                   + positive media's channel/direction arithmetic
                   + one combined attenuation update))
```

There is no measured coefficient assigning these terms milliseconds. The earlier inside ABBA 219.3225->207.075 ms result is completed offscreen batch cost, not separated GPU execution. This model does not prove a lower bound or promise FPS.

Most useful scene-informed generic candidates:

1. **Support-aware inside-ray traversal:** preserve box events and accepted lattice; reduce visits/medium checks in the large conservative admission overlap using proved per-medium support. Do not simply shrink authored boxes or enlarge grids. The rejected helper/larger-grid experiments already show why measured economics and parity are mandatory.
2. **Reuse noise-cell intermediates:** cnoise3's corner hash/gradient construction depends on the integer noise cell, not the changing fractional point within it. The four cell widths in world-unit local coordinates are approximately 12.5, 11.111, 14.286 and 10. A ray can make many samples in one such cell. A generic Noise lowering could retain those identical intermediates per cell/domain and evaluate the same interpolation arithmetic at each sample. Preserve f32 order and all value dependencies; avoid cross-medium aliasing and benchmark register pressure/cache cost. No implementation or gain is claimed.
3. **Separable expression support/value preparation:** x-only folds/fluting and y-only tapers offer tighter retained dependency structure. Being static does not permit evaluating a full spatial field once per frame: changing sample coordinates still demand changing values. Any grid/texture interpolation needs certified error handling; it is not exact by default.
4. **Certified contribution tails:** remain useful only where the final-output proof actually closes. Thin, weakly attenuating rays may have little tail opportunity. No blanket T threshold or claim that most inside cost is hidden light is justified.

The model suggests investigating traversal and noise-cell arithmetic ahead of expecting a large gain from generic demand-first sharing in this particular save. Every production mechanism must derive from OntoMath structure/dependencies, not the Northern Veil identifiers or this parameter table.
