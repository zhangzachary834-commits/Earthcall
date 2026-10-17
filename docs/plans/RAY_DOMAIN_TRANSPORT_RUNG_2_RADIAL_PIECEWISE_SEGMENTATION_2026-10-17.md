# Ray-Domain Transport — Rung 2: Radial-Piecewise Segmentation

**Status:** implemented as OntoMath + Kernel infrastructure with CPU witnesses. **No renderer consumes it yet.** No WGSL, pipeline, save, or authored meaning changed. No native GPU evidence: this ran in a Linux cloud container without a Metal/WebGPU device.

**Human origin.** Zach: "LETS GOOO do NEXT RUNG radial piecewise segmentation" (2026-10-17). This continues his 2026-10-16 commission, whose Rung 2 asked for Piecewise boundaries "resolved or conservatively projected into ray coordinates … Unknown boundaries retain the general evaluator". The *radial* emphasis comes from the [Rung 0/1](RAY_DOMAIN_TRANSPORT_COMPILATION_RUNG_0_1_2026-10-16.md) census (§6), which found Length/Distance/Sqrt beneath most real clamps. The canon itself is the revived Original Prism Sun's [Ray Transport Canon](https://github.com/zhangzachary834-commits/Earthcall/blob/firmament-laboratory/agent%20intercom/communication-threads/sdf-and-rendering/PRISM_REVIVED_TWO_CANONS_LAW_SYNTHESIS_AND_RAY_TRANSPORT_2026-10-16.md) (on `firmament-laboratory` only).

**Originated here (Claude Opus 5.5):**
- the piecewise-algebraic line form, with explicit *switching functions*;
- strict versus non-strict signs (discontinuous Select versus continuous branches);
- relevance pruning under saturation;
- interval branch-and-bound segmentation, made tight by centered (Taylor-shifted) polynomial enclosures with a double-precision error bound;
- forward-error coefficient enclosures in place of `ScalarForm::evalRange`;
- the per-piece classification reusing Rung 1's closed form.

Interval branch-and-bound root isolation and centered forms are standard interval-analysis technique (Moore; Neumaier). Nothing new is claimed for them as mathematics.

**Branch/head:** `claude/ray-domain-transport-rung0`, continuing `4e6979a1` from `sync-from-earthcall-main@d9cd72aa`.

## 1. What the rung does

```text
authored D, σ_t, σ_s, C_v, Φ, E_v                   (untouched)
   │ OntoMath::restrictAlgebraicToLine               AUTHORED-REVISION lifetime (cached with Rung 0/1 forms)
   ▼
LineAlgebraicForm: polynomial leaves (Rung 0, symbolic in o/d)
                   + Sqrt/Abs/Min/Max/Clamp/Select nodes, each declaring switching functions g(s)
   │ LineAlgebraicEvaluator(bindings)                RAY lifetime: coefficients + error bounds
   │ OntoMath::segmentLine                           sound sign regions of every RELEVANT switch
   ▼
pieces: definite (all relevant signs proved) | uncertain (≤ resolution, straddles a possible switch)
   │ Rendering::scheduleVolumeRaySegment             per piece: exact-empty | constant | general
   ▼
runs over the ACCEPTED lattice samples (never re-spaced)
   │ Rendering::executeVolumeSchedule                empty: skip · constant: Rung 1 closed form · general: lattice
```

On a ray, `|o + s·d − c|² = Q(s)` is an exact quadratic. So `clamp(k − |p−c|/R, 0, 1)` saturates to exactly 0 outside one quadratic root pair and to exactly 1 inside another; those are its switch roots. Outside the ball the density's range is the singleton `[0, 0]`, so every sample there is proved empty. Inside a saturated core every channel can be exactly constant, so Rung 1's closed form applies. The shell between keeps the lattice.

## 2. The admitted class and its mirrored semantics

| Node | Meaning (MathNode::evaluate / WGSL emitter) | Switching functions |
|---|---|---|
| Polynomial | Rung 0 restriction (exact) | — |
| Sqrt | `sqrt(max(a, 0))` | `a` (none for Length/Distance: a sum of squares is ≥ 0) |
| Abs | `\|a\|` (vectors componentwise) | `a` |
| Min / Max | `Union` / `Intersection` (vectors componentwise, scalars broadcast); `Difference = max(a, −b)` | `a − b` |
| Clamp | `std::clamp`, **pure-constant `lo ≤ hi` only** (undefined otherwise) | `a − lo`, `a − hi` |
| Select | Piecewise with interval bounds on its input variable; first match wins; pieces after the first unbounded one never reached (where `emitPiecewise` stops); no match is **Undefined** | `sel − lo_i`, `hi_i − sel`, **strict** |
| Undefined | CPU: `nullopt`; emitter: `0`. OntoMath refuses to choose; the volume channel binding passes `undefinedAsZero` (the emitter's semantics, as `VolumeZeroProof` already assumes) | — |

Continuous branches use non-strict signs (`g ≥ 0`), because both branches agree at `g = 0`. Select is discontinuous, and the CPU's include flags differ from the emitter's `>=`/`<=` exactly at the bound, so no definite piece may contain a bound.

**Still refused:** Noise; `sin/cos/exp/ln` of a bound coordinate; Tan, Map, SDF, Gradient, Normalize, matrix ops; guards, calls, folds, pieces without a MathNode; vector Clamp/Sqrt/Pow; Clamp with non-constant or inverted bounds; a Piecewise cut on the vector point; division by a non-constant; non-integer powers of space. A Rung 0 budget or reserved-symbol refusal is never routed around.

## 3. Soundness contract

- **Coefficients:** each polynomial coefficient is evaluated in double. Its enclosure is `value ± 4·γ·Σ|terms|` (forward-error bound), converted outward to float. `ScalarForm::evalRange` is deliberately not used (see §5.2).
- **Polynomial ranges:** a centered form. Coefficients are Taylor-shifted to the leaf midpoint in double with an explicit error bound, then evaluated by interval Horner over `h ∈ [s0 − m, s1 − m]`. This replaced plain interval Horner after the test caught it failing (§5.1).
- **Definite pieces:** every *relevant* switch has a proved sign over the closed piece. Relevance prunes switches hidden beneath a saturated clamp, a decided min/max loser, or a zero sqrt.
- **Exact-empty:** the density's interval range over the piece is ≤ 0. It is proved even on uncertain pieces when the range already settles it.
- **Constant:** every surviving polynomial leaf vanishes exactly in degree ≥ 1 on this line (Rung 1's zero-factor proof), evaluated at the piece midpoint.
- **What is claimed:** real-number lattice semantics. The GPU evaluates sample positions and channels in f32, so a sample within f32 rounding of a switch root may branch differently on the device. Uncertain slivers around roots and strict Select signs shrink that exposure, but native agreement remains unproven.

## 4. Evidence (Linux, Debug unless noted)

| Witness | Result | Establishes |
|---|---|---|
| `ontomath_line_segmentation_test` | 37/37 | Radial clamp admitted via `Length(p−c)` and `Distance`, with only the two clamp switches. Value parity with `MathNode::evaluate` over 25 rays × 7 points (f32-scaled bound; worst 4e-4 of bound). Range enclosures contain every probe under decided and undecided signs. Recorded signs hold at every probed point. No analytic sphere/ray root lies strictly inside a definite piece. A chord gives exactly 2 zero runs and 1 constant core run. Relevance: `clamp(\|x\|−5)` across `x=0` is one definite piece. Abs/min parity; Select semantics (first match, Undefined vs channel zero, strict bounds). 10 refusals; evaluator refusal on an unbound symbol; budget exhaustion keeps the cover with uncertain remainder. |
| `volume_ray_radial_segmentation_test` | 25/25 | Schedule + execution vs an emitter-semantics lattice mirror. Ball through its centre: 24 empty / 24 constant / 48 general samples; T relative error 9e-16; L 2.5e-9, dominated by f32 vec3 channel constants. Also: tilted chord, legacy cutoff, a missing chord (96/96 exactly empty), source active (core falls back, empty still skips), two-ball overlap with independent clocks, bounded slab Piecewise (exact to 1e-16), Perlin refusal. Finer resolution never leaves more samples general. Real-media soundness: 0 violations across 73,728 samples. |
| `volume_ray_segment_transport_test` | 49/49 | Rung 1 unchanged after its planner moved onto the shared constant-member helper; Perlin fixture corrected (§5.4) |
| `ontomath_line_restriction_test` | 57/57 | Rung 0 unchanged |

### Real authored media (frozen `saves/zones`, read-only; 64 random rays × 96 samples each; t = 0)

- **12 of 32** densities are admitted by the algebraic class: the eleven Prism Cathedral / Veiled Hour / Gyroid Reliquary radial media plus Sunlit Mist. The other 20 refuse, with first refusals: Perlin 7, `sin`/`cos` of space 10, division by a non-constant 2, `x^1.8` 1.
- On the 12 admitted media, **Rung 2 proves 69.8% of lattice samples empty**, and 8.3% are constant-density (Sunlit Mist, `D = 1`). The existing `VolumeZeroProof` grid proves **0 cells** for all twelve. Its `evalRange` deliberately does not admit Length/Distance: the October 15 Euclidean-range rung scoped those rules to `evalMathematicalRange` only. Every proved-empty sample was re-checked against the emitter-semantics evaluator, with **0 violations**.
- These are random rays aimed at random interior points, not camera distributions, and empty fraction is not GPU time. They establish *provable demand*, not a frame-time gain.

**CPU cost of scheduling:** about 209 µs per ray in Debug and 32–36 µs per ray in Release (two runs, identical demand figures, 25/25 under -O2), over a 96-sample chord, measured in this container. Even optimized, a 1080p frame (~2M rays) would cost about 70 s on one CPU core, which is far too expensive to run per pixel. The GPU consumer must not port branch-and-bound to the fragment shader (§6).

## 5. Findings

1. **Interval dependency at a grazing ray.** Plain interval Horner over a leaf at `s ≈ 21` widens like `|s|·w`. A ray grazing the core sphere (distance 2.06 against radius 2, margin 0.015) could not be decided at any resolution, which produced a run of uncertain leaves. The centered form fixed it. Without the width check in the test this would have been a silent loss of provable demand, though never a soundness error.
2. **`ScalarForm::evalRange` constants are float points.** `ScalarForm::constant(0.1).evalRange({})` returns `[0.1f, 0.1f]`, which excludes the double 0.1, because `Term::evalRange` starts from `Interval(static_cast<float>(coefficient))` without outward rounding. The test prints this as an observation. Existing proof consumers that reach it through `MathNode::evalRange` (VolumeZeroProof, Prophetic Rete) are therefore off by up to half a float ulp on lone constant terms. It is tiny but real; it is recorded for its owners, not fixed here, and Rung 2 does not depend on it.
3. **The zero-proof grid is blind to radial media.** This is a scope decision, not a bug: admitting Length/Distance into `evalRange` would change every existing proof consumer and needs its own admission. Rung 2 reaches the same media through the ray domain without touching that office.
4. **A Rung 1 fixture was passing for the wrong reason.** The Rung 1 Perlin case authored its `ValueLeaf` with key `"variableName"` (the codec uses `"var"`), so it refused a nameless leaf rather than Noise. It is corrected, and now refuses "no exact line restriction for Noise".
5. **The census predicate was looser than the rule.** It predicted 13 radial candidates by counting Div/Pow as polynomial without checking constancy. The measured admitted count is 12.

## 6. Continuation (designed, not implemented)

- **Rung 2b — GPU consumer: exact support intervals for the radial class.** The census says this is where the value is. For a ball-shaped clamp, the switch roots are sphere/ray intersections: one quadratic per ray. That is cheaper than one density evaluation, needs no branch-and-bound, and can trim each member's traversal to its proved-nonempty interval inside the authored box. The box events and sample spacing stay put (the Northern Veil ledger's rule: support bounds never replace authored events or quadrature). Classify per revision which members have such closed-form roots, and emit a variant keyed like the camera lanes. Before retention it needs native pixels against the current marcher, f32 root-margin handling (widen outward), zero-proof interplay, ABBA cost, and device reconstruction.
- **Rung 2c — per-ray affine-argument transcendentals.** *(Implemented; see §6a.)* `sin(k·x)` restricted to a ray is `sin(α·s + β)` with ray-local α, β. These are the next blockers (10 media). Switch roots of `clamp(… sin …)` are then transcendental, but bounded via derivative bounds, and the same branch-and-bound machinery holds.
- **Rung 2d — cost.** *(Implemented with measured verdicts; see §6a.)* Memoize subtree polynomial attempts in the builder, and share selector/switch evaluations across vector components.
- Noise (17 media) stays on the lattice. Its ray-domain treatment belongs to the certified-quadrature/microkernel rungs, under an explicit error contract.

## 6a. Rungs 2c and 2d — implemented (2026-10-17, second pass)

Zach: "YAAAAAAAAS NEXT RUGNSSSSS". Taken in the order the census and cost evidence above ranked them. Rung 2b (production WGSL) is deliberately **not** attempted here, because a renderer change needs native pixel comparison and this container has no GPU.

**Rung 2c — affine-argument transcendentals.** Every OntoMath transcendental is `kind(scale·var + shift)`, so restricted to a line its argument is the polynomial `scale·(o + s·d) + shift`. A new `Trans` node holds sin/cos/exp of that polynomial leaf. A ScalarLeaf is decomposed per term into (polynomial part) × Π Trans. Ranges reuse OntoMath's own `TransFactor::evalRange`, which finds sin/cos extrema exactly and treats exp as monotone, but is called with `scale = 1, shift = 0` on a soundly enclosed argument. Its own float conversion of scale/shift is therefore never relied on. A Trans node is proved constant when its argument is (a zero direction component on that axis). **Ln of a bound coordinate stays refused**: the CPU is undefined at ≤ 0, while the emitter evaluates `log(max(·, 1e-8))`. Witness: parity with `MathNode::evaluate` for `clamp(0.6 + 0.3·sin(0.7x+0.2)·y − 0.25·cos(1.3z) + 0.1·exp(−0.05x), 0, 1)` over 25 rays, range soundness on every probe, and constancy on a ray with zero x/z direction. **Census: 14 of 32 densities are now admitted** (Gyroid Reliquary labyrinth and Sky-Island halo mist join). The remaining 18 refuse on Perlin (15), division by a non-constant (2) and `x^1.8` (1). Real-media soundness: 0 violations over 86,016 samples. Rung 2 proves 63.8% of those samples empty; the zero-proof grid proves 1.13%, all of which Rung 2 also proves.

**Rung 2d — cost.** Three changes were measured. The deterministic measure is range evaluations per ray; wall-clock in this shared container varies by ±40% between identical runs.

| Change | Result | Kept? |
|---|---|---|
| Leaf-range memo: polynomial enclosures depend only on the interval, never on switch signs, so they are memoized per interval inside the ray-lifetime evaluator | torus media 425 → 132 µs/ray (Debug); no evaluation-count change | **yes** |
| Two-phase schedule: density alone segments the whole ray. Exact-empty pieces are final. Definite pieces with provably varying density and uncertain slivers go straight to the lattice. Only pieces where constancy is still possible are refined by the other channels, inheriting phase 1's proved signs (`segmentLine(…, initialSigns)`) | real-media Debug 346 → 225 µs/ray | **yes** |
| Analytic root proposals (`proposeRoots`): closed-form quadratic / sphere-ray roots choose the cut points; interval range still proves every piece | synthetic ball 90 → 50 evaluations, but summed over 14 real media **292.2 µs on vs 290.8 µs off** (Release anatomy). Neutral, and it costs ~1 sample per root at window edges | **kept, default OFF**: sound, tested, and the prototype of the GPU's closed-form-root path |

Release, original twelve media: **24–27 µs/ray mean over three runs**, from ~34 at Rung 2's first commit. That is still about 50 s per 1080p frame on one CPU core. The conclusion for the GPU consumer stands: use closed-form roots per member, not fragment-shader branch-and-bound.

## 6b. Rung 2e — Perlin noise on a ray, and what it showed about Northern Veil (2026-10-17, third pass)

Zach: "KEEP GOIGNGNGNGNGNG".

**Built.**
- **Noise node.** `noise(q)` for an algebraic vec3 argument. Its range is never re-derived here: the argument's enclosure box goes to OntoMath's own load-bearing theorem (`MathNode::evalRange` on Noise: glm::perlin's lattice formula intersected with the proved Lipschitz ball). The CPU evaluates `glm::perlin` at the f32-rounded argument. Rounding a value inside a float-bounded box stays inside it, so the theorem covers the CPU's actual value too.
- **Sign-free range memo.** This generalizes 2d's leaf memo. Any node with no switch beneath it (a polynomial, or a noise of a polynomial argument) has a range that depends only on the interval, so it is memoized per interval. Northern Veil fell from 12–15 to 4–6 ms/ray in Debug with identical proofs.
- **Density-sign targets.** The marchers branch on `max(D,0) > 0`, so phase 1 resolves each member's density sign as an asymmetric target: proved ≤ 0, or proved strictly > 0. This splits even a structurally non-negative D at its zeros. It costs evaluations on simple media (synthetic ball 90 → 131).
- **Two bugs the witnesses caught.**
  1. Adjacent pieces merged on switch signs alone, so a proved-empty stretch could be absorbed by a positive neighbour. Target signs are now part of a piece's identity.
  2. The schedule re-asked emptiness as a fresh range over a merged piece. That range is looser than the per-leaf proofs, which were being discarded. It now consumes the recorded target sign.

  A pure-noise chord went from 0/96 to 64/96 proved-empty samples, re-checked sound.
- **Proposals re-decided.** With all 28 media, analytic root proposals halve segmentation cost (26.2 vs 50.7 ms summed; Northern Veil about 3×; real schedule 0.71–1.36 → 0.44–0.76 ms/ray, Release). The 2d "neutral" verdict came from a 14-medium sample without these forms and is superseded. Proposals now default **on**.

**Census (Release, frozen `saves/zones`, 64 random rays × 96 samples, t = 0, read-only).** **28 of 32** densities are admitted; 4 refuse (division by a non-constant 3, `x^1.8` 1). Soundness: 0 violations. Selected media:

| Medium | Rung 2 proved empty | Zero-proof grid | Schedule µs/ray |
|---|---|---|---|
| Northern Veil emerald / cyan / violet / crimson | 98.0 / 99.0 / 98.4 / 97.5% | 95.6 / 97.0 / 96.2 / 95.1% | 564 / 628 / 429 / 908 |
| Borealis hero auroral curtain | 81.8% | 74.3% | 327 |
| Borealis v1/v2 clouds (four media) | 88.5–93.7% | 0% | 630–969 |
| Prism organic noise cloud | 93.0% | 0% | 1138 |
| Sky-Island sea of clouds / halo mist | 42.5 / 60.8% | 0 / 22.4% | 571 / 140 |

Every grid-proven sample is also Rung 2-proven ("grid but Rung 2 not": 0%). "100% empty" rows (veil pillar-rims, reliquary iris-halo) are t = 0 observations of time-varying media.

**What it means for Northern Veil (Zach's actual frame-cost problem).** The grid leaves 3.0–4.9% of random-ray samples to evaluate, and Rung 2 leaves 1.0–2.5%. Exact per-ray reasoning would roughly **halve the aurora's remaining sample work beyond the existing grid**. Attributing each extra-proved sample to the density factor actually zero there gives:
- the thin sinusoidal curtain `clamp(1 − |z − f(x)|/w)`: 59–85%;
- the cheap slab clamps (`|x| ≤ W`, the y-envelope): 15–41%;
- noise alone: 0%.

**Economics, stated plainly.** At ~0.4–0.9 ms per aurora ray, CPU segmentation is minutes per frame. That route to the proof is not real-time, and porting branch-and-bound into a fragment shader would not be either. These are random rays, not camera views, and proved demand, not GPU time.

**Therefore the next rung (2f) is Lipschitz-certified support tracing on the GPU**, sphere tracing in the sense of Hart (1996). For a zero-saturating factor `clamp(1 − |g|/w)` whose argument `g` restricts to the ray with a derivative bound `L` (here `|dz| + Σ|A_i·k_i·dx|`, from the authored amplitudes and frequencies), a sample at `s` with `|g(s)| − w = m > 0` certifies emptiness up to `s + m/L`. Each step costs a few sines. It targets exactly the factor that carries 59–85% of the measured gain. It needs native pixel parity and cost evidence, so it cannot be completed in this container. A CPU prototype of the certificate and step rule can be.

## 6c. Rung 2f — Lipschitz-certified support tracing, CPU prototype (2026-10-17, fourth pass)

Zach: "KEEP GOIGNGNGNGNGNG" (continued). This is the GPU-shaped algorithm §6b called for, sphere tracing in the sense of Hart (1996), built and proved on the CPU against the real saves. There is still no shader.

**OntoMath additions.**
- `derivativeRange` / `switchDerivativeRange`: forward-mode interval differentiation over the piecewise-algebraic form. Polynomials use the centered form on the derivative coefficients; sin/cos/exp use the chain rule (OntoMath's `TransFactor::evalRange` for the outer factor); Noise uses OntoMath's proved Lipschitz constant times ‖q′‖; undecided abs/min/max/clamp take the hull of branch derivatives (sound for Lipschitz functions). A plain sqrt near 0 and an undecided Select (a jump) answer unbounded, so no false Lipschitz claim is ever made.
- **Euclidean norms recognized however they are spelled.** `Length`/`Distance`, and any authored `sqrt(Σ base²)`, carry their components. Their derivative is bounded by Cauchy–Schwarz, `|d‖v‖/ds| ≤ ‖v′‖`, which stays finite where ‖v‖ = 0. An authored sum of squares also drops its sqrt switch, since `sqrt(max(a,0)) = sqrt(a)` exactly for a ≥ 0 in the reals.

**`Rendering::traceVolumeSupport` (prototype, unconsumed).**
- A *killer* is a switch sign that by itself forces the density's range over the chord to ≤ 0, such as the curtain clamp saturating at zero. The density's own sign also serves when D is Lipschitz.
- Per killer, `L = sup|g′|` over the chord.
- Walking the accepted lattice, each not-yet-certified sample takes one point enclosure of `g`. A margin `m > 0` certifies emptiness up to `s + m/L` (mean value theorem).
- A sample is empty only if every member's density is certified there.

**Evidence** (Linux; Debug suite green, Release figures below).
- `ontomath_line_segmentation_test` 52/52: the mean-value bound `|Δv| ≤ sup|v′|·|Δs|` holds on 11,016 probes across radial, sin/cos, Perlin and abs/min/clamp forms, and a Piecewise jump refuses a Lipschitz claim.
- `volume_ray_radial_segmentation_test` 27/27: tracing certifies 0 real samples with positive density.

| | Rung 2 segmentation | Lipschitz tracing | Zero-proof grid |
|---|---|---|---|
| All 28 admitted real media, samples proved empty | 70.60% | **70.65%** | 17.16% |
| Northern Veil emerald / cyan / violet / crimson | 98.0 / 99.0 / 98.4 / 97.5% | **98.0 / 99.1 / 98.4 / 97.6%** | 95.6 / 97.0 / 96.2 / 95.1% |
| …point evaluations of `g` per aurora ray | — | **18–30** (+ ~38–40 setup) | — |
| Borealis hero auroral curtain | 81.8% | **81.9%** (60 points/ray) | 74.3% |

Two witness-driven corrections made those numbers:
1. Plain `Q′/(2√Q)` derivatives left radial media at 11–42% traced. Cauchy–Schwarz on norm components lifted them to parity with segmentation (synthetic ball 0 → 24/24).
2. Three tori authored as explicit `sqrt(pow(sqrt(x²+z²) − R, 2) + y²)` stayed at 11–22% until sum-of-squares recognition. They now trace 88.6–99.7%.

**What this means for the GPU (Rung 2b/2f-native).**
- Killer identification and the symbolic Lipschitz form are *structural*: they can be compiled once per authored revision.
- Per ray, a shader computes L from the ray coefficients in closed form, then sphere-traces the killer's argument. On the aurora that argument is the curtain function, a few sines, at **~18–30 evaluations per ray instead of 96 full density evaluations**, while matching full per-ray segmentation and beating the 32k-cell grid.
- The cost is an extra cheap `g` evaluation per not-yet-certified sample (wasted on media that are never empty, up to 192 points/ray here), so eligibility should be decided per medium.
- Required before any retention: native pixel parity against the current marcher (sample spacing and authored box events unchanged), f32 handling of margins and L (shrink reaches inward, never outward), readback-free ABBA cost, and device reconstruction.

## 6d. Rung 2f-native groundwork — authored-tier certificates and a cloud WebGPU lane (2026-10-17, fifth pass)

Zach: "YAAAAA NEXT PASSSS".

**1. Support certificates (what a shader can carry).**
- **Box mode.** `LineAlgebraicEvaluator` accepts *interval* bindings, with coefficients as centre ± radius from `ScalarForm::evalRange` plus one outward ulp (§5.2). Binding the line origin to the whole medium box with `d = e_i`, the derivative at `s = 0` encloses `∂g/∂x_i` everywhere in the box.
- **`lipschitzBoundOverBox`** combines the axes as `√(Σ sup|∂_i g|²)`. It is sound, though per-axis independence costs up to √3 for isotropic g (radial clamp 0.433 against the exact 0.25). On a Northern Veil-like curtain it is tight: 0.3373 against a sampled maximum of 0.3311 over 4,000 random directions.
- **`deriveVolumeSupportCertificates`** records killers that hold over the **whole box at every time** (t bound as the unbounded interval), each with a spatial L and its authored source operand. Branch nodes now remember their `MathNode` operands, so a future emitter can lower `g` with the production emitter's own arithmetic. Lifetime: authored revision × half-extent. It does not depend on camera, ray, origin or clock.
- **`traceVolumeSupportCertified`** is the shader-shaped tracer. Per ray it binds the ray, then makes one point evaluation of `g` per not-yet-certified sample, jumping `margin/(L·|d|)`. There is no per-ray interval work.

| All 28 admitted real media (Release, 64 random rays, t = 0) | Proved empty | Point evals/ray | CPU µs/ray |
|---|---|---|---|
| Rung 2 segmentation | 70.60% | — | 356 |
| Per-ray Lipschitz tracing (2f) | 70.65% | 95 (+17 setup) | 302 |
| **Certificate-only tracing (2f-native shape)** | **70.65%** | **73** | **160** |
| Northern Veil per curtain, certificates | 97.6–99.1% (5 killers each) | 28–39 | 33–68 |

Soundness: 0 violations. Witnesses: `ontomath_line_segmentation_test` 54/54 and `volume_ray_radial_segmentation_test` 28/28.

**2. A software WebGPU lane in the cloud.** Earthcall's real WGSL pipelines now execute in this Linux container: Mesa **lavapipe** (llvmpipe Vulkan on the CPU), plus wgpu-native built from source at the vendored tag (`v29.0.1.1`, `cargo build --release`, 67 s), linked through the new opt-in `EARTHCALL_WGPU_NATIVE_LIBRARY` CMake cache path. See [Build and Environment](../BUILD_AND_ENVIRONMENT.md#software-webgpu-lane-lavapipe-2026-10-17). **Evidence class:** same-backend A/B pixel parity and functional witnesses of the real WGSL. **Not** Metal precision, and never frame time. Existing native witnesses on this lane:
- `webgpu_volume_zero_proof_test`: **73/73**, including full 2560×1440 RGBA preservation and device replacement.
- `webgpu_volume_unified_quadrature_test`: 15/15.
- `webgpu_volumetric_mist_test`: PASS.
- `webgpu_volume_shared_subexpression_test`: 74/76. Both failures are `ontoPow` normal/domain cases, e.g. `base = −2, exp = NaN`: CPU route 0, lavapipe NaN. This sits inside the open [Power Lowering Parity](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Power_Lowering_Parity/Power_Lowering_Parity.md) task. It is backend-specific and was not introduced here.
- `webgpu_v5_overlap_physics_test`: **aborts**. A semantically null overlapping member shifts another medium's centre pixel by up to 42 bytes "solely by repartitioning samples". The test was last changed 2026-10-07; per-segment unified spacing (`segmentSpacing`, commit `10082310`) landed 2026-10-09, so the test predates the design it now contradicts. Whether Metal shows the same drift needs a Mac run. **⚑ AUTHOR (Zach):** PR #361's tribunal ("a null member must not repartition") and the 10-09 unified quadrature ("finest resolution of the media present") now conflict. Rung 2's proofs offer a reconciliation, excluding a member *proved* empty on a segment from spacing, but that changes the accepted lattice and is not done here.

**Full sweep of the 17 `webgpu_*` targets on lavapipe (head `68d7d2ca` + this pass):**

| Result | Targets |
|---|---|
| pass (13) | `cathedral_open_hand` (519 s on llvmpipe), `heightfield_sweep`, `micro_mastery_lag`, `object`, `ontomath_native_resolution_parity`, `particle`, `sdf_color_expr`, `sdf_distance`, `sdf_parity` (25 shapes agree with the CPU), `sdf_range_perf`, `volume_unified_quadrature` (15/15), `volume_zero_proof` (73/73), `volumetric_mist` |
| fail, recorded above | `volume_shared_subexpression` 74/76 (ontoPow NaN-exponent domain, backend-specific) and `v5_overlap_physics` (null-member repartition drift, ⚑ AUTHOR conflict) |
| fail, needs a Mac comparison | `perlin_exact_gradient`: every mathematical gate passes (exact C2/C3 counterexamples; Gate D 16384/16384 hits, 0 RGBA difference), then a residency assertion fires because a later camera re-uploaded range-hierarchy bytes. Backend buffer behaviour or a real cache regression; unknown |
| not buildable off Apple | `screen_recorder` (needs the macOS window surface: `createWindowContext`/`configureSurface` live in `WebGpuContext.mm`). The lane is offscreen-only |

17 targets in all: 13 pass, 3 fail with recorded causes, 1 cannot be built off Apple. Nothing here is a frame-time or Metal claim.

## 6e. Rung 2f-native — the GPU certificate tracer, opt-in (2026-10-17, sixth pass)

Zach: "KEEEEPP GOINGGNNGG". §6c called for this shader tracer and §6d built what it carries. It now runs inside both production marchers as an **opt-in lane** (`ScreenChannel volumeSupportCertificatesEnabled`, a registered property path, default **off**).

**Mechanism.**
- **Structure: `sdfwgsl::lowerVolumeSupportKillers`.** Every line-form switch with lowerable operands becomes a candidate: |a| and sqrt(a) (role 0), min/max(a, b) (role 1), clamp(a, lo, hi) (roles 2/3). Each operand is lowered by the **production emitter** (`emitMathNode`) under its own `Emit`, so its constants land in the certificate payload, never in the authored parameter block, the kernel DAG or the sharing plan. Time-only leaves are evaluated inline: a certificate holds at every clock.
- **Payload: `WebGpuRenderer::attachVolumeSupportCertificates`.** Each header slot stores {sign, δ, 1/L, offset}, filled only where `deriveVolumeSupportCertificates` certified that switch over the box. δ is rounded up, 1/L rounded down, and the offset is the f32 constant the shader's clamp compares against. Cache tier and key: see the [ledger](../architecture/law/DERIVED_STATE_LEDGER.md).
- **Marchers.** Where no grid proof covers a sample, a member with certificates evaluates its candidates. The first certifying one returns `(sign·g − δ)/L`. The exit `sampleT + reach·(1−2⁻¹⁶) − 4·10⁻⁵·scale` joins the **existing** walk-exit/jump law, so sample positions, box events, spacing and transport arithmetic are untouched.
- **Converse bound (new here).** If no candidate certifies, every killer is short by at least |r|·L in g, so none can certify within |r|. *Checks* are skipped until then. A skipped check only sends the sample down the exact path, so this needs no pad.

**The device tolerance is declared, not proved.** δ = τ·(1 + sup|a| + sup|b| + |offset|), with τ = 2⁻¹⁰ (`kVolumeSupportDeviceTolerance`). Codex's audits leave Metal Fast mode's complete execution enclosure open ([native profile](../audits/VOLUME_SUPPORT_NATIVE_PROFILE_2026-10-12.md)). That is why the lane is opt-in and witnessed per backend.

**Evidence: `webgpu_volume_support_certificates_test`, 56/56 on lavapipe (llvmpipe, *not* Metal).** Every comparison below is a byte-identical full RGBA framebuffer, with certificates against the matching lane without them:
- Northern Veil, fused (4 media) and single pipelines, at t = 0 and t = 7.25, certificates alone and over the grid.
- Six further views: inside, Emerald/Violet support, reverse, axis-parallel, side, fold crossing.
- Two translated media, up to (−1024, 32, −512).
- Edits and resizes: an emission edit rebuilds no payload; a density edit rebuilds exactly one payload; a resize re-derives the certificates.
- A density with no killer attaches nothing.
- The positive-budget full lattice.
- 2560×1440 at spawn.
- All eight Prism Cathedral wing-B media, singly and fused.

Adapter-relative cost (ABBA ×4, 8 frames each, medians). These numbers **rank lanes on llvmpipe only** and are not a frame-time claim:

| Scene | exact | certificates | grid (production) | grid + certificates |
|---|---|---|---|---|
| Northern Veil, 4 media fused | 53.5 ms | **31.6** | **21.9** | 29.9 |
| Northern Veil, 1 medium | 48.0 | **20.0** | **11.8** | 15.9 |
| Prism wing B, 8 media fused (grid proves 0 cells) | — | — | 9.52 | **4.14** |
| Prism wing B, per medium (8) | — | — | 14.4–31.7 | **3.2–25.9** (1.2–4.8× cheaper) |
| Northern Veil 2560×1440, one frame | 2919 | **1434** | — | — |

**Findings.**
1. **Substitute, not complement.**
   - Where the grid proves nothing (Prism Cathedral's radial clouds, shells, tori, CSG crescent, noise cloud), certificates cut adapter cost by up to 4.8×.
   - Where the grid already proves 95–97% (Northern Veil), stacking them *costs* 16–49%.
   - A scratch run with the lane compiled but no killers attached measured no overhead (grid 23.9 ms vs 23.1 ms). The loss therefore comes from the checks themselves, at samples the grid leaves unknown, not from shader size.
   - The CPU census (§6d: certificates 70.65% vs grid 17.16% across 28 media) predicted which scenes win. It did not predict that Northern Veil's extra 2–3 points would fail to repay their checks.
   - Next: a per-medium eligibility policy from Kernel facts (proven-cell fraction, killer count). It ranks cost, never truth.
2. **A production emitter bug, found by this witness and fixed.**
   - `emitScalarForm` returned a multi-term leaf as `(t1) + (t2)` with no enclosing parentheses. `Sub` therefore compiled `a − (t1 + t2)` as `(a − t1) + t2`, and `Scale` compiled `a·(t1 + t2)` as `a·t1 + t2`.
   - Volume lowering hid it for *time-only* leaves, which it pre-computes into a prepared slot. It never hid it for spatial leaves or for other modalities.
   - The certificate lane exposed it: its killers evaluate time leaves inline, so the breathing nebula's killer read `|p| − 2.4 + 0.8 sin` instead of `|p| − (2.4 + 0.8 sin)`, and pixels differed only when sin ≠ 0.
   - The new precedence witness renders `length(p) − (2.4 + 0.3y)` as one leaf and as `Add(2.4, 0.3y)`. Before the fix they differed in 17,184 bytes (max 255) with certificates off; after it, 0.
   - A read-only scan of `saves/zones` finds affected authored expressions in **Prism Cathedral** (light chroma 160 < z ≤ 195, two field members, the root field), **Radiance Gallery** (root field) and **Sky-Island Sanctuary** (two fields). There, the GPU now matches the CPU's multiplicative modulation; before, it applied an additive offset. These are listed for Person verification. No save was edited.

**Regression check after the emitter fix (Linux Release, 4 cores).**
- **WebGPU sweep on lavapipe:** identical to §6d. 13 pass, the new certificate witness passes 56/56, and the same three fail for their recorded causes; `screen_recorder` is Apple-only.
- **Full default ctest:** 216/296 pass. The failures fall into five groups:
  - headless environment: "no GL context" or window creation;
  - a network-backed cloud test;
  - a usage-only perf tool;
  - the 18 explicit-target `webgpu_*` entries, run by the sweep instead;
  - 15 others: Prism hydration, `.ecform` semantic files, the Sun radiance contract, an OntoMath witness segfault, the chess/go/zone-boot kills, first-mover aborts, cloud upload and two app tests.
- **Those 15 were rebuilt at the pre-pass commit `5bcb6383`, and every one fails identically there.** They pre-date this pass and are not regressions.
- My two census tests briefly failed in the full suite, because other tests create a fixture `saves/zones` in the build directory. They now read the root holding the most Zones.

**Not done / limits.**
- Select-piece killers and the density-sign killer are not candidates yet.
- Device reconstruction is witnessed only with the lane off.
- Metal pixels and cost were unmeasured at this cloud pass; the dated Metal verification below records the Mac run.
- The lane stays off until eligibility, Metal ABBA and Person acceptance.

*Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-17.*

### Metal verification — October 17 (Codex)

Zach handed Codex Opus's native-verification commission. MacBook Air Mac17,3 / Apple M5 / 16 GB / macOS 26.6 (25G72), adapter directly observed as Metal (enum 5), HEAD `ee9057845`. Certificate witness **56/56** and timing rerun **56/56**; zero-proof **73/73**; shared-expression **76/76**; overlap tribunal passes with black null-participant samples; Perlin **aborts at `!sawInitialRangeUpload`, line 817**, reproducing the cloud residency assertion. The stock precedence comparison is 0 differing bytes. Sandbox acquisition initially skipped; these results are from desktop execution.

Every report-only TIMING line from the native run:

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

Full app builds at the pre-fix parent `5bcb63837` and branch supplied **15 native Engine before/after pairs**, 1920×1080, matched cameras and clocks 0 / 0.75 / 2.25. Screenshot locations: `scratch/verification/ray-domain-metal-2026-10-17-01a148a7/{before,after}/*.png`; the side-by-side `index.html` is beside them. The floating-window fixture overcame Metal Occluded refusals. Original saves and installed app were untouched; copied fixture closure preserved the existing authors.

The independent source-review CPU cancellation case is **reproduced**: original density evaluates to 1 while the planner selects ExactEmpty. Proposed GPU manifestations did not reproduce on this M5: eight black-reference comparisons match, and four nonblack positive controls match. Do not promote the stock green suite into proof qualification. The device tolerance remains declared, certificate-on device reconstruction remains unverified, and the default remains off. Person appearance boxes and live 30 FPS remain open. [Full native record, capture provenance and limits](../audits/RAY_DOMAIN_TRANSPORT_METAL_VERIFICATION_2026-10-17.md); [generic eligibility proposal](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Ray_Domain_Transport_Compilation/Ray_Domain_Transport_Compilation.md#metal-economics-and-eligibility-proposal--october-17).

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T00:18:36-07:00 (PDT).

## 6f. Execution provenance: the Codex P1 repair (2026-10-17, seventh pass)

**Origin.**
- Codex (GPT-6.1 Sol, session `01a148a7-bd85-7550-a416-0e859a729c1a`) returned Zach's macOS verification commission. It arrived as a relayed handoff, because its evidence commits are local and unpushed.
- **Metal results** (M5, macOS 26.6):
  - certificate witness 56/56, including 2560×1440 parity and the precedence witness;
  - zero-proof 73/73;
  - shared-subexpression 76/76: the lavapipe `ontoPow` NaN case does not reproduce;
  - `v5_overlap_physics` passes, but its null comparisons were all black, which leaves the ⚑ AUTHOR question open;
  - `perlin_exact_gradient` **aborts on Metal too**.
- A separate Codex source review raised two **P1** findings, both reproduced on the CPU:
  1. **Cancellation.** `F = ((A + x) − A) − x` simplifies to the exact 0. The retained evaluator executes `−1` at x = 1 for A = 2^54, yet `planVolumeRaySegment` returned ExactEmpty for `D = −F`. A certificate on `D = −min(F, 0)` would have inherited zero magnitude and zero Lipschitz from the simplified F.
  2. **Computed bound.** `lo = B − (B − 1)` folds to 1 in double on the host. In f32 the shader computes 0.
- **Zach's criterion:** "Faithfulness to authored intent is what matters." A claim that *replaces or skips execution* must hold for what executes, and must refuse when equivalence is unknown, never loosen.

**Repair (OntoMath, then every consumer).**
- **Literal constants only.** Divisors, exponents and clamp bounds must be `literalConstant`: one constant ScalarLeaf. That holds in the polynomial flattening and in the algebraic builder alike. A computed one refuses with "a … where a literal constant is required". *No real medium was affected: 28/32 still admitted.*
- **`executionBound` (new OntoMath API).** It works on an authored MathNode, ScalarForm or Piecewise and returns two numbers:
  - `sup|value|`, by magnitude arithmetic (no cancellation, so no rounding is erased);
  - an absolute **slack ≥ |executed − exact|**, under a declared **`ExecutionModel`**.

  The production model `kVolumeExecutionModel` charges two things:
  - each operation, literal read and coordinate read errs by ≤ 2⁻¹⁶ of its magnitude bound (256 f32 ulps, room for Fast-mode reassociation, fusion and approximate division);
  - each sin/cos/exp/noise evaluation errs by ≤ 2⁻¹⁰ absolute.

  f32-exact literals, and point arithmetic with f32-exact results, err by nothing.
- **Provenance.** Every Polynomial leaf of the line form records the authored subtree it was flattened from, and so does every Length/Distance norm. A leaf with no provenance has unbounded slack, never a silent exact.
- **`LineAlgebraicEvaluator::slack`/`switchSlack`.** These propagate slack per branch, under decided signs:
  - a proved-saturated clamp executes its literal exactly;
  - an undecided Select may jump, so its slack is unbounded;
  - a Euclidean norm is 1-Lipschitz, even at its axis;
  - sqrt is Lipschitz above its argument's slack and Hölder below it;
  - an undecided switch is first decided by its own range beyond its own slack (the proof segmentLine would make).
- **Decisions.** `segmentLine` decides switch and target signs only beyond the slack. The schedule's empty and non-participating claims require `hi + slack ≤ 0`. Rung 1 ExactEmpty requires the raw constant plus the original-tree slack to be ≤ 0. Closed-form constants pass a cancellation guard: slack ≤ 16·2⁻¹⁰·(1 + |c|).
- **Certificates.**
  - A killer must keep the *executed* density ≤ 0.
  - δ = 2⁻¹⁰ + 2·switchSlack(g), from the authored operands (twice, because the shader evaluates g in the killer and again in the density). This is never looser than the former 2⁻¹⁰·(1 + |args|).
  - Device error in sin(k·t) grows with |t|, so the slack is derived for |t| ≤ a **clock-magnitude bucket** (the next power of two). The memo and the payload identity carry that bucket.

**Evidence (Linux Release; lavapipe for native).**
- `ontomath_line_segmentation_test` **61/61**:
  - cancellation slack scales with what cancelled;
  - exact semantics prove the cancelled D empty, while the execution model proves no stretch of it empty;
  - the computed clamp bound refuses;
  - a literal reads exactly.
- `volume_ray_segment_transport_test` **53/53**:
  - the original tree executes D(1) = 1;
  - the planner keeps the lattice for it;
  - literal zero stays exact-empty;
  - a computed divisor refuses.
- `volume_ray_radial_segmentation_test` **32/32**. On the cancelled density, 51/96 samples execute D > 0, and the schedule, the trace and the certificates each prove 0 of them (the killer's δ ≈ 2.7·10¹²). The computed clamp bound gets no form and no certificate.
- `webgpu_volume_support_certificates_test` **57/57** on lavapipe. It adds an f32 cancellation medium (A = 2²⁵, lit 29,388 px, one never-clearing killer) with 0 differing bytes.
- Also passing:
  - `webgpu_volume_zero_proof_test` 73/73;
  - `webgpu_volume_unified_quadrature_test` 15/15;
  - mist;
  - `channel_paths_test`.

  The two lavapipe-only failures are unchanged.

| 28 real media, CPU census | before the repair | after |
|---|---|---|
| Rung 2 empty | 70.60% | 70.59% |
| Lipschitz trace | 70.65% | 70.62% |
| certificate-only trace | 70.65% (73 evals, 160 µs/ray) | 70.57% (73.4 evals, ~123–147 µs/ray) |
| soundness violations | 0 | 0 |

**Three refinements, each driven by a witness** (a first, coarser model lost 21 points of certificate coverage):
1. Separating per-operation rounding (2⁻¹⁶) from transcendental error (2⁻¹⁰).
2. Exact literal and point arithmetic. `veil.pillar-rims` went from 0% back to 100%: its box never reaches a pillar, and every saturated clamp executes 0 exactly.
3. The norm rule. The three tori went from 19–69% back to 88.6–99.6%: a torus box contains its axis, where Hölder √S is hopeless.

**Not done / open.**
- The execution model is declared, not a proof of Metal's Fast-mode enclosure. The lane stays default OFF.
- Native Mac revalidation of this pass is required.
- **The `perlin_exact_gradient` residency assertion is fixed.** A frame whose SDF batch carried no ranged instance cleared the persistent range buffer's CPU mirror. Nothing had been written, so the GPU still held identical bytes, yet the next traversal frame re-uploaded them. That is a residency bug (`WebGpuRenderer` SDF flush), not a backend quirk, which is why Metal agreed. The mirror now stays.
  - Lavapipe: `webgpu_perlin_exact_gradient_test` ALL OK, with traversal in 6/6 cameras and 0 RGBA difference.
  - Also passing: `sdf_range_perf`, `sdf_parity` (25 shapes), `sdf_distance`, `object`, `heightfield_sweep`, `cathedral_open_hand`, `micro_mastery_lag` and `ontomath_native_resolution_parity`.
  - Metal revalidation is pending.
- Generic eligibility comes only after correctness, with paired native correctness and cost evidence.

*Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-17. Human origin: Zachary Zhang; findings and Metal evidence: Codex / GPT-6.1 Sol / `01a148a7-bd85-7550-a416-0e859a729c1a`.*

## 7. Handoff answers (deltas from Rung 0/1 §8)

- **Lowerable subset now:** polynomials plus sqrt/abs/min/max/clamp and bounded Piecewise over them, including Length/Distance of polynomial vectors (§2).
- **Cached:** the algebraic form shares the Rung 0/1 entry, keyed by per-channel revision and expression identity. Evaluators, pieces and schedules are ray-lifetime and never cached.
- **Refusals:** §2. Uncertain pieces and refused channels keep the lattice.
- **Fallback:** the unchanged V5 marcher, or `MathNode::evaluate` on the CPU.
- **Authored or saved meaning changed:** none.
- **Camera movement recompiles:** no. Only binding and segmentation run per ray.
- **Segment without the sample evaluator:** yes, now *partially within a segment* — proved-empty and saturated-constant runs inside one V5 segment, interleaved with lattice runs, CPU API only.
- **Proof:** interval range enclosures, plus zero-factor constancy and Rung 1 telescoping.
- **Measured:** CPU scheduling cost and provable sample demand on real media. **No GPU time.**
- **Next smallest rung:** 2b, closed-form radial support intervals on the GPU.

---

*Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-17. Commissioned by Zach ("LETS GOOO do NEXT RUNG radial piecewise segmentation").*
