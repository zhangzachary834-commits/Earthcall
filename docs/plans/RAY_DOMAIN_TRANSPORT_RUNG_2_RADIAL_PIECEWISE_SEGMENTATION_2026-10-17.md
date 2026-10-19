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

### Metal revalidation of the seventh pass — October 17 (Codex)

At merged HEAD `a597da4cd`, rebuilt native M5 witnesses pass: certificates **57/57**, zero-proof **73/73**, Perlin **ALL OK** (including the formerly failing residency assertion and traversal in all six cameras). The cancellation comparison is black on both sides on this M5 (1 killer, 0 differing bytes), so it does not reproduce the lit cloud witness. Perlin hit coverage matches exactly; the 45-degree camera reports 27 diagnostic color-byte differences, with the other five at zero. The stock witness does not assert full RGBA equality. No full device-enclosure, new cost/FPS or Person-acceptance claim follows. [Native record and raw-log locations](../audits/RAY_DOMAIN_TRANSPORT_METAL_VERIFICATION_2026-10-17.md#post-repair-metal-revalidation--opus-seventh-pass). Default remains off.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T12:12:06-07:00 (PDT).

## 6g. Eligibility: where certificates pay (2026-10-18, eighth pass)

Zach: "YAAAAAAAAAAA". The pass follows Codex's and the Sun's handoffs (Metal results recorded in `docs/audits/RAY_DOMAIN_TRANSPORT_METAL_VERIFICATION_2026-10-17.md`). It reads Refusal 7 and Codex's note ("resource allocation must retain the authored-Law direction") as one constraint: the engine ranks cost, and Persons and Laws decide.

**Authority stays authored.**
- `volumeSupportCertificatesEnabled` (default off) remains the Person/Law switch.
- Within it, `volumeSupportCertificatesAutoEligibility` (default **on**) lets the Kernel attach a medium's certificates only where its cost estimate favours them. Off attaches them to every medium (the A/B mode the parity witnesses use).
- The outcome is published as read-only `volumeSupportCertificateEligibleMedia`, next to `volumeSupportCertificateKillers`.
- All three are registered paths (`channel_paths_test` 547/547).

**The estimate** (`Rendering::estimateVolumeSupportCertificateGain`, a cost ranking, never truth):
- It simulates the shader's own loop over 48 deterministic rays through the box at the renderer's `samplesPerChord`. A grid-proven cell jumps to its exit; otherwise the certificates are checked (the first success answers, the converse bound skips checks) and then the density is evaluated.
- Cost units are authored operation counts (noise 40, sin/cos/exp 4 per term, sqrt/length/div/pow 4).
- A medium is eligible only if its cost with certificates is < 0.85× its cost without.
- The certificate memo key gains auto mode × grid state × samples per chord, and the payload identity gains auto mode and samples, so a change re-lays the payload.

**Evidence.**
- CPU census (`volume_ray_radial_segmentation_test` 32/32): **13 of 28** admitted media are estimated eligible.
  - Northern Veil: all four curtains are not eligible (ratios 0.90–1.25).
  - Prism wing B: 7 of 8 are eligible (0.09–0.47). The noise cloud is the exception, at 1.07.
- Lavapipe (`webgpu_volume_support_certificates_test` **68/68**, ABBA medians, adapter-relative):

| Scene | grid lane | grid + auto certificates |
|---|---|---|
| Northern Veil fused (0/4 eligible) | 17.24 ms | 17.35 ms (no penalty, within noise) |
| Prism wing B, the 7 eligible media | 8.78–16.69 ms | 2.31–6.41 ms (1.5–5.2× cheaper) |

Every auto-mode frame is byte-identical to the grid lane.

**Known miss.** The noise cloud is estimated ineligible, but lavapipe measured it about 18% faster with certificates. The likely cause is the shader's shared-noise prelude: a density that uses sharing evaluates every shared noise group before itself, so each skipped sample saves more than its op count credits. The miss is in the safe direction (that medium keeps the production path). Modeling the sharing plan is a follow-up.

**Also in this pass.**
- **The Sun's two limits.**
  - *Perlin 45°:* a scratch run of the pre-fix renderer shows the identical 18 differing bytes on lavapipe. It is the range traversal's lawful sample-sequence change (the test asserts coverage, not RGBA), and the baseline, not a regression.
  - *Black Metal cancellation frames:* fast math may reassociate `((A+x)−A)−x` into the exact 0. A polynomial identity is not reassociable: `(x+A)² − A² − 2Ax − x²` at A = 2¹² now runs as a lit witness (36,726 px on lavapipe, one never-clearing killer, 0 differing bytes). Metal should see it lit; that is a prediction until the next Mac run.

**Open.**
- Metal ABBA of the auto mode.
- Person acceptance before any default change.
- Modeling the sharing plan in the estimate.
- Calibrating the cost weights on Metal; the lavapipe agreement is directional.

*Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-18. Human origin: Zachary Zhang; Metal evidence: Codex / GPT-6.1 Sol and the local Sun.*

### Eighth-pass Metal return — October 17 (Codex)

Rebuilt `993f92a6` on M5 Metal: **68/68 twice** with timing enabled; channel paths **547 advertised, 3 clock skips, 0 failures**. The polynomial cancellation case is now lit (36,669 pixels) with 0 differing bytes in both runs. Auto mode selects 7/8 Prism media and 0/4 Northern Veil curtains; finite auto-mode byte parity passes. The seven selected individual Prism media improve 1.58–3.90× across paired medians. **Northern Veil auto is slower**: stock 2.95→3.49 and 2.64→3.24 ms (+18.3%, +22.7%). A scratch same-clock probe confirms zero eligible media/killers and zero warm compiles/uploads at both 0.75 and 7.25, yet preserves the overhead. The cloud's no-penalty inference does not transfer to Metal.

The renderer still composes certificate WGSL from the master switch even when no medium is eligible; inspect an effective-eligibility baseline fast path next, without encoding Zone names. This is a source lead, not an isolated attribution of all cost. Auto measurements are 321×181 completion/readback-inclusive observations; the forced-all 2560×1440 comparison also passes but does not establish full-resolution auto-mode cost/parity. No default, source, save or installed-app change. [All results, raw paths and limits](../audits/RAY_DOMAIN_TRANSPORT_METAL_VERIFICATION_2026-10-17.md#auto-eligibility-metal-verification--opus-eighth-pass). Default remains off and Person acceptance remains open.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T19:22:42-07:00 (PDT).

## 6h. Effective eligibility: the lane-off program when nothing is eligible (2026-10-18, ninth pass)

**Origin.** The Sun's M5 return (`82af7aba`, Metal record §"Auto eligibility") gave three results:
- 68/68 twice;
- the polynomial witness lit (36,669 px, byte-identical);
- the seven selected Prism media 1.6–3.9× faster; but **zero-eligible Northern Veil 18–23% slower**.

The cause is structural. The certificate program variant was selected whenever the lane was enabled. An idle branch costs nothing on llvmpipe, but it costs on Metal, plausibly through register pressure and occupancy.

**Repair.** Eligibility is decided before the program key:
- `volumeSupportCertificatesActive` brings the zero-proof memo up to date first (`updateVolumeZeroProofMemo`), so the estimate sees this frame's proof.
- The certificate lane is **per member**: the key's certificate bool, the structure string's killer text and `composeVolumeSet(…, std::vector<bool>)` include a member only when its certificates are attached.
- With nothing eligible, the key equals the lane-off key, so **the lane-off program itself runs**.
- A mixed set compiles certificate code only for its eligible members.

**Evidence (lavapipe).**
- `webgpu_volume_support_certificates_test` **69/69**. New check: enabling the lane on Northern Veil with nothing eligible causes **0 program compiles**, with byte parity.
- `webgpu_volume_zero_proof_test` 73/73 and `webgpu_volume_unified_quadrature_test` 15/15.
- Lavapipe never showed the idle-variant cost (17.46 vs 17.58 ms), so **the paired M5 measurement is the acceptance test.**

**Pending: the Perlin 45° native baseline.** The Sun notes that the pre-fix 45° baseline was reproduced on lavapipe, not on Metal. Running the pre-fix renderer with the residency assertion bypassed on the M5 would close it. That run is scratch-only, with no commit.

*Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-18. Human origin: Zachary Zhang; measurement: the local Sun (Antigravity).*

### Ninth-pass Metal return — October 17 (Codex)

At `4c17e915`, native M5 certificates pass **69/69 twice**, with **0 new program compiles** when enabling auto for zero-eligible Northern Veil; zero-proof **73/73**, quadrature **15/15**, Perlin **ALL OK**. Stock Northern Veil medians are 2.09→2.09 and 1.86→2.01 ms. Two independent 128-frame-batch probes at clocks 0.75/7.25 span **−2.62% to +0.16%** (all byte-identical, no warm compilation/uploads/builds), replacing the earlier repeatable 18–23% penalty with near-baseline observations. CPU preparation still adds roughly 1.2 µs/frame in the four-medium fixture. Selected Prism individuals retain **1.58–4.10×** improvements in both stock series. No isolated GPU-time or live-FPS claim.

The scratch Perlin old-mirror/current comparison executes the full corpus and saves raw buffers: all 12 corresponding buffers are byte-identical, including the same 27-byte OFF/ON difference at 45°. The residency fix therefore did not introduce that difference. A D-only-mode no-op was detected and rejected before acceptance; that harness mode needs correction. [All measurements, method and limits](../audits/RAY_DOMAIN_TRANSPORT_METAL_VERIFICATION_2026-10-17.md#effective-eligibility-metal-verification--opus-ninth-pass). Registers/occupancy are unprofiled hypotheses. Default remains off; Person acceptance and the mathematical device enclosure remain open.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T19:56:43-07:00 (PDT).

## 6i. What the noise cloud's miss actually was, and the Perlin D mode (2026-10-18, tenth pass)

**Origin.** Zach relayed Opus's open list and asked the local session to do it: (1) model the shared-noise prelude in the eligibility estimate, (2) make the Perlin D-only mode run or refuse, (3) lower Select-piece and density-sign killers. This pass does (1) and (2), measured natively on the M5. (3) remains open.

**(1) The shared-noise hypothesis (§6g "Known miss") was wrong for the noise cloud.**
- `sdfwgsl::inspectVolumeSharedPrelude` now exposes the emitter's own sharing plan. `Rendering::volumeDensitySampleCost` charges an undecided sample what the marcher runs: density with shared sites read from slots, plus every group when the prelude fills before density. The renderer's memo re-derives this cost when any channel revision moves and re-ranks only if it changed.
- The census shows 8 of 28 media fill the prelude before density (Northern Veil's curtains among them). **The noise cloud is not one of them.** Its density, `1.7·clamp(1 − (|p| − 0.7·noise(1.2p))/2.2, 0, 1)`, reads no shared group. Its one killer is that clamp, whose operand carries the noise, so a check costs about one density evaluation. Counting the prelude alone promoted and demoted nothing.
- **The real miss was the estimate's ray set.** All 48 rays aimed within 0.8 of the half-extent, which made every one a long central chord. A camera's pixels meet the box uniformly over its silhouette (Crofton measure), so many chords are short corner chords. There samples crowd (spacing = chord/n), and one certified reach skips many. The estimate now uses 96 such lines, which ranks the noise cloud **0.70**; the M5 measured **0.72**.
- That ray set alone overshoots, though. It would make Northern Veil's cyan ribbon eligible (0.78), while forced certificates on Northern Veil are measured 1.5–2.2× slower on the M5. Scanning the per-sample base cost (2–80 units) moves nothing except the grid-heavy curtains. The missing term is the one the M5 already measured: carrying certificate code costs a member about 18–23% before any check runs. So eligibility now needs `1.2 × costWith < 0.85 × costWithout` (`kVolumeSupportCertificateVariantOverhead`, device-relative).
- **Census (32/32):** 17 of 28 eligible, up from 13. New predictions: the noise cloud, Borealis v1 ×2 and Sky-Island sea-of-clouds. The last three are **unmeasured on Metal**. Every Northern Veil curtain stays ineligible.
- **M5 (`webgpu_volume_support_certificates_test` 69/69 twice, then 72/72 with the new witness):** auto mode now attaches all 8 Prism wing-B media. The noise cloud runs 1.66→1.21 and 1.44→1.10 ms (−27%, −24%), and the other seven keep 1.6–3.0×. Northern Veil has 0 eligible and 0 new compiles, so it runs the identical lane-off program; its auto medians 3.03→3.06 and 3.19→3.45 ms are run-to-run spread on a busier machine (absolute times this session ran higher than Codex's).
- **Invalidation witness (new, auto mode):** another curtain's emission leaves the per-sample cost at 135 and rebuilds nothing. Noise-free companion channels move it 135→133 and rebuild exactly once. A check also makes sure the witness is not vacuous.

**(2) Perlin D mode.** Gate A3's block never closed, so the `} // runCore` brace actually closed Gate A. That nested Gate D and the final "ALL OK" inside the A–C block, and `EARTHCALL_PERLIN_TEST_MODE=D` ran nothing yet exited 0, as Codex caught. The brace now closes A3, "ALL OK" names the gates that ran, and any other mode value fails. M5: full mode 17 gate lines; `ABC` 9; `D` 8, including the same 27 bytes at 45°; `bogus` exits 1.

**(3) Deferred on evidence.** In the census, certificate-only coverage equals Rung 2's own empty fraction, density-sign targets included, on every admitted medium: the largest shortfall is 0.18 points (Sky-Island sea-of-clouds, 36.93% vs 37.11%), and hollow-shell's certificates exceed it. No authored density in the frozen saves has bounded pieces, so Select-piece killers would have no consumer. A density-sign check costs a full density evaluation. Lowering either now adds shader structure with no measurable proof to gain. Build it when an authored medium shows the gap.

**Not done.** Select-piece and density-sign killers (3, deferred above); Metal timing of the four newly predicted media; Person acceptance; the device enclosure. Certificates stay default off. No save, default or installed app changed.

**Evidence:** `scratch/verification/ray-domain-tenth-pass-2026-10-18/` (census before/after and the base-cost scan, both timed certificate runs, the witness run, zero-proof 73/73, Perlin in every mode).

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T01:20 PDT. Human origin: Zachary Zhang; the open list is Opus's (`session_01HvZSR64QghrdyCpq2M7qvX`); the idle-variant measurement and the D-mode catch are Codex's (`01a148a7`).*

## 6j. Compile only the enabled killers; a killer subset, measured (2026-10-18, eleventh pass)

**Origin.** Zach asked the local session to keep going on the next pass and to answer the Firmament (Aetherion, Gemini 4 Argon), whose eleventh-pass counsel in the [handoff thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md) gave four directions: (1) stop emitting dead switch branches, (2) choose a greedy killer subset, (3) Metal-time the newly eligible media, (4) Rung 2b ray-prefix trimming; plus a housekeeping item on `ScalarForm` constants. Directions 1–3 and the housekeeping are done here, each measured on the M5. Direction 4 stays open.

**Correction to §6i.** Under the final ray set and overhead, counting the shared prelude *does* change one decision: it promotes Sky-Island sea-of-clouds (ratio 0.889 without the prelude, 0.688 with it). §6i's "promoted and demoted nothing" held only for the old ray set. The tenth-pass report to Zach repeated the error.

**What the M5 said, step by step** (ABBA medians, 321×181; this machine's baselines drift up to 2× between runs, so only within-run pairs are compared):
1. **Greedy subset as the eligibility rule** (`Rendering::selectVolumeSupportCertificateKillers`). The census promoted Northern Veil emerald (switches 5, 8) and cyan (switch 8 alone, the Firmament's curtain-only hypothesis). On Metal, emerald ran 1.6–1.75× and cyan 1.47–1.63× slower, reproduced in three runs.
2. **Where the time went.** Two scratch variants (never committed) separated it. With the certificate program carried but no check run, the overhead was 1–21%. With every check run but no jump taken, even Prism's spherical cloud was +72%. So a check costs far more than its operation count. Each `volumeSupportReach` compiled **every** lowerable switch (12 per curtain, including a clamp around `noise`), guarded only by a runtime-uniform `P.v[h] != 0`. Emitting only switches 5 and 8 brought emerald from +60–75% to +10% and cyan from +47–63% to −8%. That confirms the Firmament's direction 1, though the mechanism is the per-check cost of compiled-in candidates rather than the payload size.
3. **Direction 1, retained.** `sdfwgsl::lowerVolumeSupportKillers(density, only)` lowers just the enabled switches. The renderer's certificate memo keeps the enabled set (`switches`) and its WGSL. Every composer takes the set, so the enabled set is now structural, and a box resize recompiles exactly when it changes that set. Forced certificates on Northern Veil (two runs each, old → new):
   - Fused set, certificates alone (exact 3.97–4.25 ms): 2.99–3.02 → 2.19–2.46 ms.
   - Fused set, grid + certificates (grid 1.41–1.44 ms): 2.38–2.43 → 2.10–2.12 ms.
   - Single curtain, certificates alone (exact 0.75 ms): 0.67 → 0.48 ms.
   - Single curtain, grid + certificates (grid 0.30–0.31 ms): 0.61–0.62 → 0.47 ms.
4. **A parity break this exposed, fixed.** Both program caches recomputed structure only when *content* revisions moved, so a forced→auto change of the killer set kept the old program while the payload was re-laid. The emerald curtain rendered alone differed in bytes. The enabled set is now folded into the revision that gates structure (`combineCertificateSet`; the equations' evaluator keeps its pure content revision). The per-curtain auto parity check, which runs after the forced sections, is the regression witness.
5. **Eligibility stays with every killer enabled.** Even pruned, the subset-promoted emerald ran +16–20% and cyan −6…0%. The selector now decides eligibility with all candidates and only then picks the subset an eligible medium checks and compiles (never estimated worse than all of them). Northern Veil: 0 of 4 eligible, 0 compiles, auto fused 1.53→1.57 and 1.55→1.59 ms. The census is 33/33, with 17 of 28 eligible (unchanged), of which 4 compile fewer killers than they certify.
6. **Direction 3, the tenth pass's unmeasured promotions** (auto mode, two final runs, every byte equal to the grid lane):

   | Medium | Framing | Grid | Grid + auto certificates |
   |---|---|---|---|
   | Borealis v1 airy cloud | side (Prism-style) | 1.18 / 1.66 / 1.29 | 0.73 / 1.04 / 0.84 |
   | Borealis v1 airy cloud | top-down | 0.99 / 1.12 | 1.05 / 1.19 |
   | Borealis v1 dense cloud | side | 1.15 / 1.58 / 1.26 | 0.59 / 0.78 / 0.64 |
   | Borealis v1 dense cloud | top-down | 0.95 / 1.03 | 0.94 / 1.03 |
   | Borealis v2 emerald-cyan | top-down | 1.15 / 1.22 | 0.87 / 0.93 |
   | Borealis v2 violet-magenta | top-down | 1.25 / 1.27 | 0.94 / 0.95 |
   | Sky-Island sea-of-clouds | top-down | 2.83 / 2.87 | 3.03 / 3.09 |
   | Sky-Island sunlit-halo-mist | top-down | 2.52 / 2.53 | 2.43 / 2.42 |

   The side runs predate pruning. The final test frames these zones from above, because Sky-Island's sea is a thin slab the side camera grazes and its halo mist lit nothing there.
   - Borealis v1 gains from a distance and is even up close, which is the Crofton estimate's own distant-camera assumption showing.
   - **Sea-of-clouds is a false positive (+7%), and it is the medium the prelude term promoted.** It stays eligible this pass rather than being demoted by a one-off rule; see "Next".
7. **Housekeeping, done at its owner.** `Term::evalRange` now encloses a coefficient that no float equals by the floats on either side. Float-exact constants stay points, and a value beyond float range still encloses itself. Plan §5 observation 2 is now an assertion in `ontomath_line_segmentation_test` (64/64). The Prophetic Rete, SDF range soundness and authoring reachability tests pass. Every real medium proves the same zero-proof cells as before. The Firmament cited `widenConstantDoubleToFloat` in `LineRestriction.cpp`; no such function exists, but `LineRestriction` already adds one outward ulp at that point, which stays as belt and braces.

**What the Firmament got right and wrong.**
- **Right:** compiled-but-dead switches are the dominant cost of a check.
- **Wrong on the facts:**
  - Curtains have 12 switches, not 13.
  - The y-slab killers are not "nearly zero" past the grid: attribution shows they alone prove 40 of 189 extra samples on emerald.
  - Retiring the ×1.2 is not supported. Without it, the every-killer estimate admits cyan (0.78), which Metal runs even at best.

**Next.**
- **Measured, device-relative eligibility.** The operation-count estimate is right for the media certificates were built for (Prism, Borealis v2). It is wrong where checks interleave with grid proofs or the shared prelude (Northern Veil, sea-of-clouds) and where framing differs from a distant camera (Borealis v1). The frontier answer is to let the device decide: a short A/B of the two compiled programs per medium. That needs a timing source that does not reintroduce the native-timestamp flicker, which is Zach's call.
- **Rung 2b** (Firmament direction 4): closed-form ray-prefix trimming for radial and half-space killers. Prototype it against these numbers.

**Evidence:** `scratch/verification/ray-domain-eleventh-pass-2026-10-18/` (census before/greedy/final; the greedy, idle, no-jump and switches-5-and-8 scratch timings; pruned and final timed runs; nine related volume tests; the post-`ScalarForm` suite).

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T11:46 PDT. Human origin: Zachary Zhang ("KEEP GOING… check if the FIRMAMENT sent u anything… NEXT PASS"). Directions 1–4 and the housekeeping item are the Firmament's (Gemini 4 Argon). The idle-vs-no-jump decomposition, the cache-revision repair and the eligibility-stays-whole rule are this session's.*

## 6k. Measured eligibility: the device may veto the estimate (2026-10-18, twelfth pass)

**Origin.** §6j left measured eligibility as Zach's decision because a timing source must not bring back the native-timestamp flicker. Asked to choose, Zach picked the hidden render: "assuming we're only measuring once per necessary interval instead of recomputing the measurement from scratch every time, we can do hidden render." This section is that design, built and measured on the M5.

**Rule.** In auto mode the operation-count estimate still *admits* a medium. The device can then *veto* it. Once per regime (defined below), `WebGpuRenderer::probeVolumeSupportCertificates` times the frame's own volume pass with that medium's certificates on and off:
- It runs at the end of `endFrame`, after the visible frame is submitted, against that frame's finished opaque depth, with every member present.
- It uses the CPU clock around queue completion. No timestamp queries are used.
- It runs one on/off pair per frame, alternating which lane goes first (ABBA across frames).
- The first pair warms up and is not recorded. Four recorded pairs follow.
- The medium keeps its certificates only if its median work is at least 3% below the median without them. The margin favors the simpler program on ties.

Pixels are byte-identical either way, so a verdict changes only speed. Forced mode (auto off) never measures.

**The interval.** A verdict is keyed by a regime: inside or outside the box, the camera's distance octave in box radii, the viewport's pixel octave, and the set's membership.
- Returning to a regime reuses its verdict, so a camera oscillating across an octave boundary does not re-measure.
- Any rebuild of the certificate memo clears every regime: density revision, extent, clock bucket, auto mode, grid lane, samples per chord, or per-sample cost.
- A medium whose box covers under 2% of the view is not measured, and the estimate stands until it does.
- At most 32 regimes are kept per medium.

**Cost of measuring.** A full-footprint pair at 2560×1440 cost up to 80 ms per frame for five frames (noise-organic-cloud: 120 ms against 41 ms settled), a visible stutter. The hidden lanes now draw only 8 full-width strips spread down the medium's projected footprint, using the same full-resolution rays the frame casts there, just fewer of them. The strips start at 1/16 of the footprint's height for the warm-up pair, then are sized once so a pair's work is about 5 ms. Each pair also times an empty submission, and that round trip is subtracted. Two failed attempts showed why both pieces are needed:
- **One centred band:** it vetoed noise-organic-cloud at full size, whose gain lives at the edges.
- **A 3 ms budget:** at 321×181 each lane's fixed setup cost diluted the strips' comparison, and true 2× gains were vetoed.

**What the M5 said** (three runs, 5 ms budget, every byte equal):
- **Clear wins, kept in all three runs:** Prism ×8 (at 321×181 and at 2560×1440), Borealis v2 ×2, and Borealis v1 dense.
- **Near-ties, can flip between runs:**
  - Borealis v1 airy: kept 2 of 3.
  - Sky-Island sea-of-clouds, §6j's false positive at +7%: vetoed 2 of 3.
  - Sunlit-halo-mist, at −4%: kept 3 of 3.

  Either choice costs about the same, and within a run each verdict is held.
- **Measuring cost at 2560×1440:** settled → measuring went 2.6→5.6, 5.2→9.3 and 42.6→48.1 ms per frame. So it adds about 3–5 ms per frame for five frames, once per regime.

**A latent hazard the first run found.** A hidden lane reused the frame's number. The resident volume payload refused a second write in one frame and fell back to the per-frame stream, which shares one buffer with the instances under the time-only preparation dispatch, and wgpu rejected it. Every frame and every hidden lane now gets its own encoding epoch (`_volumePayloadEpoch`). The same fallback is still reachable if the resident buffer cannot be created, and it would fail the same way for a medium with GPU preparation.

**Witnesses** (`webgpu_volume_support_certificates_test`, 90/90): an admitted medium is timed (warm-up plus 4 pairs) and then settles; a held regime runs no further hidden renders; one octave farther is measured once; returning reuses the verdict; a rebuild clears it; and no verdict changes a byte. The visible frame's statistics stay its own; the probe reports `volumeSupportCertificateProbeRenders`, `…ProbeCompiles` and `…MeasuredVetoes` separately.

**Not done.** Measurement only vetoes: a medium the estimate rejects is never timed, so false negatives remain the estimate's. None was observed: Northern Veil, the only rejected medium measured by hand, was slower when forced. Live movement and Person feel are unverified. Rung 2b is next.

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T15:23 PDT. Human origin: Zachary Zhang chose the hidden render and the once-per-interval rule. The regime key, the strip sampling, the overhead subtraction and the veto-only rule are this session's.*

## 6l. Rung 2b: closed-form killers, solved once per ray (2026-10-18, thirteenth pass)

**Origin.** Zach: "NEXT PASS". This is the Firmament's direction 4 (Gemini 4 Argon): closed-form ray-prefix trimming for radial and half-space killers, measured against Rung 2f's in-loop sphere tracing on Prism wing B. Rung 2b was first planned (§3, before certificates existed) as "one sphere/ray quadratic per member per ray". Here it lands as a refinement of the certificate lane rather than a separate lane. The Firmament proposed it at the ray prefix; this pass also adds a hole for shells.

**Classification (CPU, `deriveVolumeSupportCertificates`).**
- A certified killer gets a closed form when its switch function is, by the line form's own structure, `K + k1·r + ka·|c0 + c1·r|` with `r = |M p + b|`, or `K + n·p`:
  - `r` is one Euclidean norm of at most 3 affine components (a ball, ellipsoid or cylinder).
  - Allowed combinators are Add/Sub, Mul by a constant, and one Abs of a term affine in `r`.
- Coefficients come from the Polynomial leaves' symbolic coefficients, evaluated at unit origins and directions. That is exact for an affine leaf, and each leaf is verified affine at a probe point.
- **Bounded residual:** any subtree that does not fit is replaced by OntoMath's proven range for it over the box at every time. This covers noise, the clock and non-affine polynomials, including a time-varying `c0`.
- The worst case is certified, folded with δ and the sign into a canonical form:
  - radial: certified where `r > ρout` or `r < ρin`
  - half-space: certified where `n·p > e`
- A form without a residual is **exact**: it certifies exactly where the per-sample check would. That check is then dropped, so its switch is never compiled. A form with a residual keeps the check for the band the residual leaves open.

**Per ray (GPU, `volumeSupportClosed`).**
- One solve per closed killer gives a **hull** (density may be nonzero only inside it) and the longest **hole** (never nonzero inside it).
- It uses the closest-point form `r² = A·(s − sc)² + |w|²`. The obvious `B² − AC` cancels `|u0|²` against the root when the camera is hundreds of radii away.
- It widens outward under a 2⁻¹² model, about 4096 ulps. That covers Fast-mode arithmetic, the f32 copies of `M`, `b` and the thresholds (each rounded toward certifying less), and coordinate rounding at `|origin|`, `|ro|` and `|t|`.
- **Single marcher:** the hull narrows `supportStart`/`supportEnd`, so the first sample index and the tail break use the authored chord lattice unchanged. The hole is one compare and a lattice jump per sample.
- **Fused marcher:** the hull intersects each member's `raySupport`, which already feeds the collective walk and the prefix union. The hole sets that member's cell exit.
- Box events and sample spacing never move.

**Census (CPU, 28 real media).**
- 48 closed forms (40 radial, 8 half-space, 6 with a bounded residual) on 23 media. They certify 35.5% of all census samples without a check.
- **Witnesses:**
  - Every claim is backed by the authored switch at random box points and clocks 0, 0.75 and 7.25. Every exact form also covers every clear certification.
  - No sample a closed form certifies has positive density.
  - The census was 36/36. It caught a bug in this pass: an abs whose offset was a residual had been marked exact.
- **Estimate** (cost with certificates over without, before → after):
  - spherical-cloud 0.136 → 0.075, hollow-shell 0.222 → 0.109, csg-crescent 0.103 → 0.067, dual-sovereign 0.147 → 0.067, noise-organic 0.70 → 0.53, time-breathing 0.205 → 0.181.
  - Northern Veil's y-slab half-spaces become closed forms. Its curtains stay ineligible (0.83–1.16).

**Native (M5).**
- Bytes are equal throughout: `webgpu_volume_support_certificates_test` passes 116/116.
- New byte-parity witnesses on every closed-form Prism medium, each at 321×181 from inside the box, at grazing incidence and from 150 radii away, and at 2560×1440.
- Within-run A/B at 2560×1440, forced grid + certificates, sphere-traced → closed form (ms; two runs):

  | Medium | Grid | Sphere-traced | Closed form |
  |---|---|---|---|
  | spherical-cloud | 12.6 / 13.6 | 2.68 / 2.88 | 1.86 / 2.04 (−29–31%) |
  | hollow-shell | 14.8 / 16.3 | 4.46 / 4.79 | 2.96 / 3.21 (−33–34%) |
  | csg-crescent | 15.0 / 16.0 | 3.06 / 3.25 | 2.00 / 2.25 (−31–35%) |
  | dual-sovereign | 16.2 / 17.0 | 3.51 / 3.58 | 2.37 / 2.52 (−30–32%) |
  | noise-organic-cloud | 58.1 / 61.2 | 39.9 / 41.4 | 34.0 / 35.1 (−15%) |

  Time-breathing-nebula was classified after this A/B. Cross-run, it settled at 5.8 ms against 6.2–6.5 ms before; that is not a claim.

**Scratch-harness correction.** A first A/B reported noise-organic at 3.2 ms (−91%). The scratch toggle had changed the closed set while the compiled switches, and therefore the pipeline and its payload identity, stayed the same, so the GPU read a stale payload for that one medium. Production cannot reach that state: every memo rebuild trigger is already in the payload identity. It is now structural anyway: the memo's rebuild `generation` joins the payload identity. Warm frames still upload nothing.

**Not done.**
- The tori: a nested norm is quartic along a ray.
- A residual abs certifying inside an annulus.
- More than one hole per member.

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T16:40 PDT. Human origin: Zachary Zhang ("NEXT PASS"). Direction 4 and the prefix-trimming idea are the Firmament's; Rung 2b's sphere/ray quadratic is the §3 plan's. The classification by line-form structure, the bounded residual, the hull-and-hole lowering, the closest-point numerics and the generation key are this session's.*

## 6m. Nested norms (tori) and the payload fallback (2026-10-18, fourteenth pass)

**Origin.** Zach: "KEEP GOING". This pass takes two items §6l and §6k left open. The Sun's new branch (`sun/person-rooted-delegated-authority-20261018`, a governance doctrine) and the Firmament had nothing addressed to this thread.

**Nested norms.** A norm bounds itself below by each of its components (`r ≥ |c_i|`), so a killer certified where `r > ρout` is also certified wherever any `|c_i| > ρout`. A torus `|(ρ − R, y)|` therefore yields two closed forms:
- `|y| > ρout`: its affine components together, as one radial form (a slab here).
- `|ρ − R| > ρout`: each component that is itself affine in an affine norm, as a cylindrical shell (hull plus hole).

These are bounds, never exact, so the per-sample check stays. A killer now carries a list of `VolumeSupportClosedForm`s, and the renderer lays out one 16-float block per form.
- **Census:** 54 forms (46 radial, 8 half-space; 12 from residuals or bounds) on 26 media. They certify 46.4% of samples without a check (was 35.5%). Every claim is backed by the authored switch, and no certified sample has positive density (36/36).
- **M5 at 2560×1440,** within-run A/B against sphere tracing, bytes equal (ms; two runs):

  | Medium | Grid | Sphere-traced | Closed forms |
  |---|---|---|---|
  | torus-medium | 33.2 / 34.9 | 4.73 / 4.91 | 3.17 / 3.37 (−31–33%) |
  | proxy-proof-torus | 20.5 / 21.4 | 1.52 / 1.56 | 0.74 / 0.73 (−51–53%) |

  All eight Prism wing-B media now carry closed forms. `webgpu_volume_support_certificates_test` passes 124/124 (inside, grazing, 150-radii and 2560×1440 parity on both tori).

**Payload fallback (§6k's latent hazard).** When the resident parameter buffer cannot be used, a pipeline with GPU preparation now gets its own frame-lifetime parameter buffer instead of the frame stream. The stream shares one buffer with the instances under the preparation dispatch, which wgpu rejects. **Witness:** with the probe's payload epoch removed by hand, which forces every hidden lane down this path, the certificate test passes 116/116 with no validation panic, where §6k's first run had panicked. The epoch was then restored.

**Still open:**
- A residual abs certifying inside an annulus.
- More than one hole per member.
- Measured false negatives.
- Device enclosure.
- Person acceptance.

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T16:51 PDT. Human origin: Zachary Zhang ("KEEP GOING"). The norm lower bound and the fallback repair are this session's.*

## 6n. Whether a bounded closed form keeps its per-sample check (2026-10-18, fifteenth pass)

**Origin.** Zach: "KEEP GOING". There were no new messages for this thread. A cloud Claude session (`01HvZSR6…`) had tidied two agenda lines on this branch, and this pass fast-forwarded onto them.

**Question.** A closed form with a residual or a lower bound (noise, the clock, a torus) used to keep the killer's per-sample check for the band it leaves open. Does that check pay? A scratch A/B (removed) at 2560×1440, within one run, bytes equal (ms; two runs):

| Medium | Closed form + check | Closed form only |
|---|---|---|
| torus-medium | 3.22 / 3.31 | 2.63 / 2.66 |
| time-breathing-nebula | 4.62 / 4.93 | 3.97 / 4.26 |
| proxy-proof-torus | 0.70 / 0.74 | 0.65 / 0.69 |
| noise-organic-cloud | 26.5 / 29.6 | 27.6 / 31.8 |

**Rule.** `VolumeSupportCertificate::perSampleCheck` is always on without closed forms and off for exact ones. For a bounded form, `selectVolumeSupportCertificateKillers` drops the check after the greedy subset when the estimate says it costs more than it skips.
- The census drops it for the three tori (the M5 agrees) and keeps it for the noise cloud (the M5 agrees).
- It keeps it for the breathing nebula, where the M5 measured dropping it 14% faster. That miss is recorded, not patched. The likely cause is that its clock term is a GPU-prepared slot the operation count overcharges.
- The hidden probe cannot decide this cheaply: flipping a killer's check changes the compiled switches, so each timing pair would recompile.

**Auto mode, 2560×1440,** settled (ms; two runs): torus 2.77 / 2.93 (§6m: 3.48), proxy-proof-torus 0.56 / 0.56 (0.61). Bytes equal: auto parity at 321×181 covers the closed-only path, and `webgpu_volume_support_certificates_test` passes 124/124.

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T17:41 PDT. Human origin: Zachary Zhang ("KEEP GOING").*

**Addendum, sixteenth pass (Zach: "KEEP GOING").**
- **Prepared clock leaves.** The production evaluator prepares expensive clock-only leaves (`sdfwgsl` `expensiveTimeOnly`) once per frame, while the certificate check recomputes them per sample. The estimate now costs them as slot reads in the density and in fill-first prelude groups, but at full cost in a check (`preparedTimeLeaf`). No census decision changed (36/36), and the breathing nebula still keeps its check. The rest of that M5 gap is something an operation count cannot see, such as divergence and per-iteration overhead.
- **Closed forms only on Northern Veil** (a scratch A/B, removed; y-slab half-spaces only, no per-sample checks; grid → grid + closed forms only, ms, two runs, bytes equal):
  - Fused set of four: 1.581 → 1.622 and 1.812 → 1.805.
  - One curtain: 0.368 → 0.345 and 0.367 → 0.348.

  The fused set is what Northern Veil runs, and it is neutral there, so no closed-only eligibility path was added. Northern Veil's 30 FPS milestone is not this lane's to close.

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T17:55 PDT.*

## 6o. Device margin witness: how much of the execution slack the M5 uses (2026-10-18, sixteenth pass)

**Origin.** Zach: "KEEP GOING". Every certificate skip and every closed form rests on the declared execution model (`kVolumeExecutionModel`: 2⁻¹⁶ of magnitude per operation, literal and coordinate read; 2⁻¹⁰ per transcendental or noise). A killer certifies where `sign·g − δ > 0` with `δ = τ + 2·slack(g)`, so the claim needs the device's `g` within `slack(g)` of the exact `g`. Metal Fast-mode's complete enclosure has been open since `VOLUME_SUPPORT_NATIVE_PROFILE_2026-10-12`. Pixel parity shows only that nothing changed on the views tried.

**Witness:** `webgpu_volume_support_enclosure_test`.
- **Method.** For every certified killer of every real medium in `saves/zones`, it takes that killer's own production program (`composeVolumeSet` with only its switch compiled) and adds a compute entry point that calls `volumeSupportReach` at 4096 random box points. This is the same lowered code the marcher calls, in a compute rather than fragment context.
- **Exact readback.** With the header `{±1, δ = 0, 1/L = 1}`, the function returns `sign·g` exactly. Sign −1 recovers `g ≤ −1`.
- **Reference.** The authored operands evaluated on the CPU at the same f32 points: double scalars and f32 vector operations, which contribute under 2⁻⁸ of `slack(g)` here.
- **Coverage.** Clocks 0, 0.375B, −0.75B and B for buckets B = 1 and 1024, with slack re-derived per bucket as the renderer does.

**Result (M5).**
- 160 killer–bucket pairs (80 killers × 2), all compiled and measured. The worst device error is **0.16% of `slack(g)`** (proxy-proof torus, |t| ≤ 1), the median 0.08%. No evaluation strays beyond δ. Passes 4/4.
- Noise killers use 0.01%. The device's Perlin agrees with the CPU's within about 10⁻⁶.
- **Clock-dependent killers at |t| ≤ 1024.** The breathing nebula's real error grows to 1.6×10⁻⁴ (f32 rounding of `t` inside `sin(1.8t)`). The model gives it `slack(g)` 0.18 and δ 0.37, about 1000× what this device needs. That conservatism, not the geometry, is what weakens certification at large clocks.

**What this is and is not.** It is a measured device margin, about 600–1000× under the declared model on this adapter and at these sampled points. It is not an enclosure proof:
- Sampled points, not every point.
- A compute rather than fragment compilation context.
- One adapter.
- The density's own executed switch is not separately sampled.

Whether this evidence justifies a tighter declared model, or the lane's default, is Zach's decision (⚑). Nothing here changes either.

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T18:00 PDT. Human origin: Zachary Zhang ("KEEP GOING"). The witness design is this session's; the declared model is Codex's (2026-10-12, P1 2026-10-17).*

## 6p. Faithfulness to exact math is the rule: the baseline, and what Noise means (2026-10-18, seventeenth pass)

**Zach's decision** (2026-10-18, answering the choice the Firmament's Milestone 2 and Codex's request raised): **"Faithfulness to exact math should be the rule."** For rewriting volume arithmetic (Rung 3a and after), byte identity with today's shader is no longer the test. Agreement with the authored equation evaluated exactly is.
- **Unchanged:** skips and closed forms (Contract I) must still keep every sample and its value. They already do, and byte parity remains their witness.
- **New:** a rewrite of the arithmetic (Contract II) must be at least as close to the authored equation as the arithmetic it replaces, measured below. A tolerance on final pixels, if one is wanted beyond "never less faithful", is still Zach's to set.

**Witness:** `webgpu_volume_faithfulness_test`.
- **Device side:** each real medium's own production program (`composeVolumeSet`, every channel, the shared-noise fill and the GPU clock-preparation pass). A compute entry forms each sample exactly as the marcher does (`ro + rd·t − origin` in f32) and returns `volumeDensityEval(p)`.
- **Reference:** an independent double-precision interpreter of the authored tree with the CPU evaluator's semantics (division guard, power, clamp, union/intersection/difference, first-match Piecewise), at the point formed in double from the same f32 ray, origin and t.
- **Samples:** 64 rays × 96 lattice samples × 2 clocks per medium, at the medium's real world origin.

**Baseline (M5, density channel).**
- 31 of 32 real media measured. Every one is within **1.2×10⁻⁵ of its density scale**, most within about 10⁻⁶. Today's shader is faithful to f32 precision.
- The one refusal is `reliquary.shaft-beam`, whose production program does not compile: "volume emission: piece 0 must evaluate to Vector, got Scalar". It is a separate, pre-existing finding, and needs either a save fix or an emitter decision (task file).

**What Noise means (a finding about the definition).** The authored `Noise` op is `glm::perlin` as the CPU evaluator runs it, on floats, and the GPU's Perlin matches that to about 10⁻⁶. But `glm::perlin<double>` is a **different function**: it differs from the float one by up to 1.2, for example at (3.1985, −3.4835, 2.0853), 0.5759 against −0.6281.
- **Cause:** the lattice hash. `mod289`/`permute`, the ×1/7 gradient fold and `floor` select gradients through f32 rounding, so some cells get different gradients in double.
- **Consequence:** a naïve high-precision reference, including an 80-digit one that re-implements Perlin in real arithmetic, measures a different noise and reports errors of 15–24% of the density scale on every noisy medium. This witness hit exactly that before correcting it.
- **The reference** keeps the hash exactly as its f32 definition, from the exact lattice cell, and evaluates only the continuous part (fractional position, dots, fade, blends) in double. It agrees with float `glm::perlin` to 1.4×10⁻⁶.
- A precision-independent Noise would need its hash stated in integer arithmetic. That would be an OntoMath definition change, recorded for the owners and not made here.

**Next (Rung 3a).** Every arithmetic rewrite gets gated by this witness against today's per-medium errors, with the same rays and the same reference. It extends to the other channels as they are rewritten. Per the Firmament's caution, polynomial forms in `s` should be re-centred near the box (or kept in the closest-point form for norms), because expanding about a far camera loses exactly the accuracy this rule now protects.

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T18:46 PDT. Human origin: Zachary Zhang ("Faithfulness to exact math should be the rule"). Contracts I/II are the Firmament's framing; the witness and the Noise finding are this session's.*

## 6q. Where today's pixels leave the authored equation: quadrature, not arithmetic (2026-10-18, eighteenth pass)

**Origin.** Zach: "keep going", under §6p's rule. This pass tried the first Rung 3a rewrite, the sample point in the medium's own frame, and then measured where faithfulness is actually lost.

**1. On Metal, source-level reassociation does not control rounding.**
- The witness evaluated each lattice point two ways at once:
  - the marcher's `(ro + rd·t) − origin`;
  - the medium frame `((ro − origin) + rd·t₀) + rd·(i + ½)·step`.
- In strict IEEE f32 on the CPU (fast-math off) they differ at 91% of coordinates. On the M5 they are **bit-identical at every sample of every medium**, near and far cameras alike, and so are their densities.
- wgpu compiles for Metal in fast-math mode, where the compiler canonicalizes algebraically equivalent forms. It almost certainly also hoists loop invariants itself.
- **Consequences for Rung 3a.** Rewriting per-ray arithmetic in WGSL buys neither faithfulness (the compiler owns the rounding) nor, plausibly, speed (it already hoists). A faithfulness gain from arithmetic must come from values the shader cannot see, computed on the CPU in double, as the GPU clock preparation already does.
- No production change was made. The comparison stays in `webgpu_volume_faithfulness_test` as a witness, with a "never less faithful" check.

**2. The integral is where pixels leave the authored equation.** The marcher integrates optical depth as 96 midpoint samples per chord, τ ≈ Σ σ(sᵢ)·step, with σ = D > 0 ? max(σₜ, 10⁻⁶) : 0. The same witness compares that with the converged integral of the same authored σ: 8192 against 16384 subintervals in double, 16 rays per medium. Transmittance error, in 8-bit levels:

| Media | Worst transmittance error against the continuum | Optical depth error |
|---|---|---|
| `borealis.hero.auroral-curtain` | **3.85 of 255** | 31% |
| Borealis v1/v2 ×4 | 0.96 – 1.92 of 255 | 1.9 – 2.3% |
| Prism wing B and summit | 0.02 – 0.22 of 255 | ≤ 0.6% |
| Northern Veil ×4 | 0.02 – 0.17 of 255 | ≤ 1.4% |
| Everything else | < 0.01 of 255 | — |

The references converge to 2×10⁻⁴ or better. Today's per-point arithmetic is faithful to about 10⁻⁶ (§6p), so on this corpus **quadrature is the dominant departure from exact math, by three to four orders of magnitude**, and it is visible on Borealis. This is transmittance (alpha) only; in-scattered and emitted radiance share the same lattice and are expected to behave alike, which is not yet measured.

**Recommendation (the Firmament's Layer C, reframed).** The faithful crossing is error-controlled integration, not arithmetic rewriting. Spend samples where the integrand varies, using what the ray-domain machinery already knows per ray: closed-form support, segments, Lipschitz bounds. Use higher-order rules where it is smooth, against an output budget measured from the continuum. `volumeOutputErrorBudget` (Codex, 2026-10-10; capped at 1/255; today against the full 96-sample lattice) is the natural seat if its reference becomes the continuum.

**Zach's decisions this needed (⚑, answered below):**
- Whether `volumeSamplesPerChord` stays the definition of the image, or becomes a floor under a continuum error budget.
- What budget, for example ≤ ½ level against the continuum.

Faster media may get slower where they are unfaithful today (the Borealis hero curtain).

**Zach's answer (2026-10-18):**
- "for now its fine to keep it at 96 then, because 96 is decent enough for me to prototype and inhabit in this stage. future aim is complete accuracy htough, 96 -> even more fiathful we can use as a way to probe for even more issues."
- "I woudlnt assume having sampling boxes every frame is always more efficient or faithful than a future approach, like psosibly an integration operation."

So:
- **`volumeSamplesPerChord = 96` stays the image definition for now.** No production change, and no continuum budget yet.
- **The continuum stays the aim.** The witness's 96-against-continuum gap is kept as a probe for issues.
- **The future crossing is integration operations, not more sampling.**
  - Where the authored mathematics admits it, integrate closed-form over the ray's segments. The spherical cloud's optical depth, ∫ 1.6·(1 − √(A s² + B s + C)/2.2) ds, has an elementary antiderivative. Shells and tori split into such pieces, and Rung 2's segmentation and the Rung 2b closed forms already find them.
  - Numeric integration remains only for what has no closed form (Perlin noise), and even there the smooth factors around it can be integrated exactly.
  - Adaptive sampling is at most a fallback inside that remainder, not the destination. This also refines the Firmament's Layer C: segment-level closed-form transport first, quadrature last.

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T19:08 PDT. Human origin: Zachary Zhang ("Faithfulness to exact math should be the rule"; "keep going"). Layer C is the Firmament's; the compiler finding and the quadrature measurement are this session's.*

## 6r. The integration operation: OntoMath integrates along a ray (2026-10-18, nineteenth pass)

**Origin.** Zach, after §6q: "does ontomath have integration", then "good yes lets enrich the ontomath integration substrate". OntoMath's one integration office (`ScalarForm::antiderivative`, `antiderivative(MathNode)`, `integrable`, `definiteIntegral`; built for reading Laws backwards, ONTOMATH_FRAMEWORK §6) is extended, not duplicated in the renderer. It keeps the office's rule: exact, or `nullopt` with the reason, never an approximation.

**1. Integration by parts** (`ScalarForm::antiderivative`), still inside the term algebra:
- **xⁿ·g(ax + b)** for a whole n > 0 and g ∈ {sin, cos, exp}, by the tabular rule. sin and cos rotate by quarter turns, so no π is ever rounded.
- **xᵃ·ln(cx)** for any a, including ½·ln(cx)² at a = −1.
- **Still refused:** what has no elementary antiderivative, such as sin x/x (the sine integral), √x·eˣ, or several transcendentals of x in one term.
- **Consequence for Laws:** a Flow whose rate is t·sin t now reverses exactly. `law_reversal_test` pins ∫₀^π t·sin t dt = π, and keeps sin(t)/t as the honest refusal. `ontomath_test` differentiates every new form back to its integrand.
- ONTOMATH_FRAMEWORK §6, LAW_AND_CREATION_SYSTEM and ALGORITHMS_AS_LAW were updated where they said by parts is refused.

**2. Exact integrals along a ray:** `LineAlgebraicEvaluator::integral(node, s₀, s₁, signs)`.
- **What it integrates.** On a decided branch of a ray-restricted form, the integrand is P(s) + Σ c·√Q(s) + Σ Pₖ(s)·g(αₖs + βₖ):
  - polynomials, about the interval midpoint (no cancellation against a far origin);
  - √Q, in closed form (asinh, log or asin by the sign of its discriminant);
  - Pₖ·g, by the tabular rule.
- **Branch decisions.** Switches the caller left open are decided by their proved range over the interval (strict for Select bounds). One that changes sign inside is refused with "split at its root".
- **Products.** (k₁ + c₁√Q)(k₂ + c₂√Q) is expanded exactly.
- **Refused:** noise, an undecided branch, √ of a non-quadratic (a torus's nested norm: an elliptic integral), a radical times a non-constant, Undefined.

**3. Witness on real media** (`volume_ray_radial_segmentation_test`, now 40/40).
- **Method.** Along 24 rays per medium, `segmentLine` splits at density's sign. ∫max(D, 0) ds is then the closed form on each decided piece, plus a **proved enclosure** for what interval arithmetic cannot decide: the root slivers a few resolutions wide, and pieces whose range only grazes zero (∫max(F, 0) ∈ [∫F, ∫F + w·max(0, −F_lo)]). It is compared with a Richardson-extrapolated 32768-point midpoint sum.
- **Exact on every ray** (enclosure ≤ 5×10⁻⁶ relative, agreement 2×10⁻⁸–5.5×10⁻⁷, which is the reference's own error): spherical cloud, hollow shell, breathing nebula, dual sovereign, `veil.forward-scatter`, the Sunlit Mist volume. That is **6 of 28 admitted media**; the crescent is exact on 3 of 9 rays.
- **Honest refusals:**
  - noise (Borealis ×5, the noise cloud, nimbus, `veil.outer`);
  - √ of a non-quadratic (the three tori);
  - a branch flipping inside a piece (Northern Veil, Sky-Island, Borealis hero);
    *Corrected in §6t: these three were misattributed. The witness reported the last root sliver's reason, not the definite pieces'. Their real limits are noise (Northern Veil, Borealis hero, Sky-Island's sea of clouds) and a square root times a wave (Sky-Island's halo mist).*
  - products the algebra cannot hold (the labyrinth, `shaft-beam`).
- On every ray, closed form or not, the converged sum lies inside the proved enclosure.

**Next.**
- A segmentation that also splits where a value branch, not only density's sign, changes. That covers the crescent and the curtains' non-noise factors.
- Extinction and emission channels: the same operation on σₜ, then transmittance.
- The GPU emission of these closed forms, gated by `webgpu_volume_faithfulness_test` against the continuum.
- Noise and lit transport remain the numeric remainder (§6q, Zach: integration operations first).

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-18T23:58 PDT. Human origin: Zachary Zhang ("lets enrich the ontomath integration substrate"). The integration office is the Law-reversal one (ONTOMATH_FRAMEWORK §6); the by-parts rules, the ray integral and its enclosure witness are this session's.*

## 6s. Movement rung M0: a camera move rebinds; it does not relower (2026-10-19, twentieth pass)

**Origin.** Zach asked whether ray math worked out ahead of time forces redoing everything on each small camera move. The Original Prism Sun answered with a movement ladder (M0–M6) in [its movement architecture](../../agent%20intercom/communication-threads/sdf-and-rendering/PRISM_PROPHECY_MOVEMENT_DIRTY_RAY_SENSITIVITY_ARCHITECTURE_2026-10-19.md). Its M0 asks for a witness that the lowered artifact is pose-parametric. This section is that witness. Zach approved the order ("YAAASSSSSS"): M0, then the M0.5 locomotion audit, then value-branch segmentation.

**What the witness does** (`webgpu_volume_support_certificates_test`, section "Movement rung M0", now 132/132):
- **The tour.** Northern Veil (all media, fused) and Prism wing B (15 closed-form-carrying killers), each with certificates alone and over the grid. 168 and 176 poses: orbits at five radii from 2% to 120% of the set's extent, the centre of every medium looking out, and every medium's face looking in.
- **Rebinding only.** Across every pose: zero program compiles, zero lowerings, zero certificate payload builds, zero zero-proof builds, zero probe renders, zero parameter or proof bytes uploaded, and the same killer count on every frame.
- **Fresh-bind equality.** At 6–7 checkpoints across the tour, the renderer that moved through every pose draws, byte for byte, what a freshly initialized renderer that never moved draws at the same pose. 4–5 of the checkpoints are lit.
- **Control.** Each fresh renderer's first frame does lower and build certificates, so the counters the tour holds at zero are live.

**Scope.**
- Measured eligibility (auto mode) is deliberately pose-dependent: crossing inside/outside or a distance or viewport octave owes one cached measurement per regime (§6k). The witness runs with auto off, so it covers the artifact, not that verdict cache.
- It covers the Screen path only. Where Person movement comes from (`LocomotionChannel`'s direct pose writes, no revision counter) is the M0.5 audit Prism asked for, not this witness.
- No timing claim.

**Evidence:** `scratch/verification/ray-domain-twelfth-pass-2026-10-18/m0-pose-witness-1.log` (local, ignored like the earlier logs).

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-19T11:55 PDT. Human origin: Zachary Zhang (the movement question; approval of the order). The ladder and the M0 definition are the Original Prism Sun's; the witness is this session's.*

## 6t. Value-branch segmentation: every piece integrates its decided branch, and the office holds more products (2026-10-19, twenty-first pass)

**Origin.** Zach approved the order M0 → M0.5 → value-branch segmentation, and then: "YASSSSSSS (do value branch segmenattion)". §6r named the rung: split where a value branch, not only density's sign, changes, so the crescent and the curtains' non-noise factors integrate.

**What the diagnosis found first.**
- `segmentLine` already decides every switch the value reads (`relevantUndecided`), so a definite piece is already a value-branch piece. The rung's real gaps were in `integral()` and in the witness.
- **`integral()` read operands its branch does not use.** A clamp proved saturated still built its argument, and a min/max built *both* sides before choosing. A saturated clamp over noise, or the unchosen side of a CSG min holding noise or an open switch, refused although the decided branch is exact.
- **The witness misreported refusals.** It kept the *last* refusal on a ray, which is usually a root sliver (expected to be undecided). That hid the definite pieces' real reasons, so §6r's "branch flipping" for Northern Veil, Sky-Island and Borealis hero was wrong.

**What changed.**
1. **Branch nodes build only the decided branch** (`LineRestriction.cpp`, `integral`): Clamp saturated low or high is its bound; Min/Max builds only the chosen side; a sqrt proved of a negative is 0. This mirrors `relevantUndecided` exactly.
2. **Polynomial times square root, R(s)·√Q(s).** Radicals now carry a polynomial coefficient. The moments Iₙ = ∫uⁿ√Q du come from differentiating u^(n−1)·Q^(3/2): (n+2)·A·Iₙ = [u^(n−1)Q^(3/2)] − (n+½)·B′·Iₙ₋₁ − (n−1)·C′·Iₙ₋₂, about the interval midpoint, with I₀ the existing closed form (a linear Q has its own recurrence).
3. **Same-root products with polynomial coefficients:** (P₁ + R₁√Q)(P₂ + R₂√Q) = P₁P₂ + R₁R₂·Q + (P₁R₂ + P₂R₁)·√Q.
4. **Wave products by product-to-sum** (sin·cos = ½[sin(u+v) + sin(u−v)], and so on); exp·exp = exp(u+v).
5. **Refusals now name the mathematics:** two different square roots multiplied ("an elliptic integral in general"); a square root times a wave; exp times sin/cos (exact, but not held by this algebra yet).
6. **The witness reports a definite piece's refusal before a sliver's**, and its floor rises from 6 to 7 media.

**Witnesses.**
- `ontomath_line_segmentation_test` (70/70, 6 new), each against its analytic antiderivative to 1e-12, not against quadrature:
  - ∫(1 + x + x²)·|p|;
  - ∫₀^π sin x·cos 2x = −2/3;
  - a saturated clamp over noise;
  - a min whose unchosen side is noise;
  - the elliptic and root-times-wave refusals.
- **Mutation check.** All six fail against the previous `LineRestriction.cpp` and pass against this one.
- `volume_ray_radial_segmentation_test` (40/40). The corrected census of the 28 admitted media:

| Outcome | Media |
|---|---|
| **Exact on every ray (7, was 6)** | `veil.forward-scatter`, Sunlit Mist, **`reliquary.shaft-beam` (new, 24/24)**, spherical cloud, hollow shell, breathing nebula, dual sovereign; enclosure ≤ 9.6×10⁻⁶, agreement with the converged sum ≤ 5.5×10⁻⁷ |
| Partly exact | crescent, 3 of 9 rays; the rest are two different shells multiplied (elliptic) |
| **Noise (13)** | `veil.outer`, nimbus, Sky-Island sea of clouds, Borealis ×5, Northern Veil ×4, noise-organic |
| Elliptic (3) | the three tori (√ of a non-quadratic) |
| Square root × wave (2) | gyroid labyrinth (a gyroid times a radial falloff), Sky-Island halo mist |
| No density on the sampled rays (2) | `veil.pillar-rims`, `reliquary.iris-halo` |

**Reading.** On this corpus the elementary closed form has nearly reached its limit: every remaining non-noise refusal is a genuine non-elementary integral (elliptic, or a Fresnel-type root times a wave). The next gains are therefore:
- **Hybrid integration** (Prism decree, step 3). Noise is the numerical remainder, multiplied by smooth factors that are now exactly integrable. On the curtains, quadrature should then sample only the noise factor.
- **Exact transmittance:** the same operation on σₜ.
- **Elliptic integrals** by Carlson's symmetric forms (R_F, R_D, R_J) for the tori and the crescent. These are exact special functions with known, convergent duplication algorithms, so they would not be quadrature. This is a ⚑ for Zach: does "exact" admit a special function computed to machine precision by a convergent algorithm, as `asinh` already is?

**Not changed.** No GPU path; the 96-sample image stays the definition (Zach, §6q).

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-19T13:17 PDT. Human origin: Zachary Zhang (the rung and the order; "integration operations first"). The diagnosis, the moment recurrence, the product rules and the census correction are this session's; "hybrid integration" is the Original Prism Sun's decree step 3.*

## 6u. Hybrid integration: closed-form weights, sampled remainders, and focus (2026-10-19, twenty-second pass)

**Origin.** Zach: "NEXT RUGN" (he is meanwhile learning about elliptic integrals; §6t's ⚑ stays open). The rung is the Original Prism Sun's decree step 3: "where smooth factors can be solved and only an irreducible factor remains numerical". Zach's standing rule applies: integration operations, not more sampling; 96 stays the image.

**What it is.** `LineAlgebraicEvaluator::hybridIntegral(node, s₀, s₁, signs, cells)`, in OntoMath's integration office beside `integral()`. Both now share one core.
- **The split.** The decided branch becomes a closed form plus Σ wₖ(s)·Gₖ(s). Gₖ is the irreducible remainder, an authored node with no closed form here: noise, an elliptic product, a root times a wave. wₖ is the closed-form weight its parents multiply it by.
- **How a node becomes a remainder.** Any node that refuses becomes a remainder with weight 1. Its parents carry it through every operation that is linear in it: sums, products with closed-form factors, a decided abs, an in-range clamp, a decided min/max or Select piece. Sqrt and sin/cos/exp of a remainder are not linear, so the node itself becomes the remainder there.
- **Product Simpson rule (product integration).** Gₖ is sampled at `cells` + 1 edges and interpolated by a quadratic per pair of cells; an odd count closes with one cubic panel over the last three. Each ∫ wₖ·(Lagrange basis) is exact, so a sharp closed-form profile adds no quadrature error at all. Order 4 where Gₖ is smooth, against the midpoint sum's 2; measured: each doubling of cells cuts the error ≥ 6×.
- **Proved radius.** Per panel, sup|w|·width·(1 + Λ)·min(G's proved range width, L′·half-width), with Λ the interpolant's Lebesgue constant (1, 1.25, 1.6311). It holds on every ray of every refused medium (0 misses). It is loose: 0.01–6× the integral. It is a certificate of soundness, not of accuracy.

**Two numerical repairs it forced.** Both affect `integral()` too; all exact-path checks still pass.
1. **Wave integrals re-centre on the interval.** `integrateWave` evaluated P(s)·G at the global s. At Northern Veil's s ≈ 500, a cubic basis cancelled terms of size 10⁹.
2. **Slow waves integrate their Taylor series.** Along a ray nearly perpendicular to a wave's axis, the ray frequency α = scale·dₓ is tiny, and the tabular rule divides by α^(j+1). When |α|·h ≤ 1, Σ g⁽ᵏ⁾(β)(αv)ᵏ/k! is integrated term by term instead. It converges geometrically to rounding, as `sin` itself is computed.

Each repair has a witness that fails without it.

**Witness** (`volume_ray_radial_segmentation_test` 41/41; `ontomath_line_segmentation_test` 81/81). Every medium the closed form refuses, on the same rays, against the converged Richardson midpoint sum. Three estimators, all at about 96 evaluations per ray:
- **plain**: the 96-sample midpoint sum over the chord, as the renderer's lattice does;
- **focused**: the same budget on only the pieces segmentation could not prove empty;
- **hybrid**: focused, plus closed-form weights and product Simpson.

| Medium | Plain 96 | Focused | **Hybrid** |
|---|---|---|---|
| Northern Veil curtains ×4 | 4.1–6.6×10⁻³ | 7.7×10⁻⁶–3.6×10⁻⁵ | **3.7–4.5×10⁻⁶** |
| Borealis hero curtain | 1.6×10⁻³ | 1.9×10⁻⁵ | **1.3×10⁻⁶** |
| Borealis v1 / v2 clouds | 5.0×10⁻⁴ / 1.6×10⁻³ | 7.3×10⁻⁵ / 1.9×10⁻⁵ | **1.3×10⁻⁷ / 1.1×10⁻⁶** |
| Tori (wing B, summit, proxy) | 2.2×10⁻³–0.12 | 4.5–5.5×10⁻⁵ | **4.7×10⁻⁸–1.0×10⁻⁶** |
| Crescent | 9.8×10⁻⁴ | 3.7×10⁻⁵ | **6.5×10⁻⁷** |
| noise-organic | 1.1×10⁻³ | 7.2×10⁻⁵ | **4.7×10⁻⁷** |
| Sky-Island ×2 | 1.5×10⁻⁴ / 6.5×10⁻⁵ | 1.1×10⁻⁴ / 3.8×10⁻⁵ | **3.4×10⁻⁶ / 1.7×10⁻⁶** |
| `veil.outer` (3 remainders, 288 evaluations) | 9.1×10⁻⁸ | 1.9×10⁻⁷ | 1.4×10⁻⁷ |

Mean relative error of ∫max(D, 0) per ray.

**Reading.**
- **Where the gain comes from.** Focus alone beats plain on 17 of 19 media; the weights then beat focus on 19 of 19. The hybrid beats plain on 18 of 19. The exception is `veil.outer`, whose three remainders cost 3× the evaluations, and where both errors are under 2×10⁻⁷.
- **Optical depth only, CPU only.** These are errors in ∫max(D, 0), not pixels. They become pixels through transmittance and lit emission, which is the next rung, and the GPU emission gated by `webgpu_volume_faithfulness_test` follows that. The 96-sample image is still the definition (Zach); nothing rendered changed.

**Open.**
- The radius is first-order loose. A second-derivative bound on Perlin noise (OntoMath proves only its Lipschitz constant) would make it fourth-order like the estimate.
- The elliptic ⚑ (§6t) would turn the tori's remainder into closed form.

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-19T14:06 PDT. Human origin: Zachary Zhang (the rung order; integration first; faithfulness to exact math). Hybrid integration as a step is the Original Prism Sun's decree; the remainder algebra, product Simpson rule, radius, focus attribution and the two numerical repairs are this session's.*

## 6v. The Carlson rung: exact special-function reduction, witnessed before it is trusted (2026-10-19, twenty-third pass)

**Origin.** §6t asked Zach whether elliptic integrals by Carlson's forms count as exact. The Original Prism Sun answered yes, and Zach adopted it: "LETS GO WITH IT (unless i later decide otherwise but i probably wont)".
- **The Sun's distinction:** elementary closed form ≠ exact special-function reduction ≠ numerical quadrature. A reduction to R_F/R_D/R_J preserves the integral exactly; evaluating them in finite precision is an execution-faithfulness question, like sqrt.
- **Its acceptance criteria:**
  1. the transformation preserves the quantity exactly;
  2. the evaluation error is bounded or characterized;
  3. no authored semantics is silently altered;
  4. a fallback remains where the reduction cannot be certified.
- **Its order:** witness the rung before trusting it broadly. Its hierarchy is now in ONTOMATH_FRAMEWORK §6.

**What landed** (`Singularity/OntoMath/EllipticIntegrals`, free functions, no domain type):
- **R_F, R_D, R_J, R_C** by Carlson's duplication (Numer. Algorithms 1995), iterated to his truncation bound r = 2⁻⁵³. **Complex R_F** answers for arguments ≥ 0.01π from its cut. Every out-of-domain argument answers nullopt: negative, two zeros, p ≤ 0 (no principal value), NaN or infinity.
- **The first-kind quartic reduction** ∫_y^x Π(aᵢ + bᵢt)^(−½) dt = 2R_F(U₁₂², U₁₃², U₁₄²), and `ellipticFirstKindTwoQuadratics`, which factors Q₁·Q₂ into conjugate or oriented real linear factors.

**What the witness found.** Every one of these was found by the witness; none was assumed.
1. **The first-kind formula has a branch condition the real-root form hides.** For a conjugate pair, U₁₃ = 2|X₁X₃||Y₁Y₃|·cos(φ(x) − φ(y))/(x − y), with φ = ½Σ arg of the pair's factors. Past a total sweep of π, U₁₃ turns negative and 2R_F(U²…) is another branch: 0.83 relative error on long intervals, confirmed in mpmath. Since φ is monotone, splitting the interval exactly (by additivity) until each piece sweeps ≤ 0.9π restores the formula.
2. **R_J at extreme arguments, three numerical repairs:**
   - δ = (p−x)(p−y)(p−z) overflows at p = 10²⁰⁰;
   - √pₘ − √xₘ cancels as the iterates converge;
   - 1 + e cancels whenever p lies far from the arguments on either side.

   e is now a product of bounded factors fᵢ = 4⁻ᵐ(p₀ − xᵢ)/(√pₘ + √xᵢₘ)², and 1 + e a sum of non-negative terms built from the small one of 1 ∓ fᵢ (each exact without subtraction).
3. **References must be checked too.**
   - mpmath's `elliprj` loses about as many digits as the arguments' spread: R_J(2, 3, 4, 10⁻²⁰⁰) is 67.1 at 40 digits, 80.5 at 50, and 141.13295926608469 from 200 on. R_J's defining integral, transformed to u = ln t, confirms the last.
   - A plain quadrature reference for ∫dt/√(Q₁Q₂) was itself 3.5×10⁻¹⁰ off on a spike of width Im ρ = 0.014.
   - So every function reference is now required to agree between 200 and 400 digits, and every integral reference between tanh-sinh and Gauss–Legendre on graded breakpoints. The generator refuses to write a value otherwise (`scripts/generate_carlson_references.py`).

**Witness** (`ontomath_carlson_elliptic_test`, 19/19):
- **Function errors against the references,** 110 R_F, 86 R_D, 87 R_J and 47 R_C values (arguments 10⁻³⁰⁰…10³⁰⁰, one zero, near-equal, p on both sides): worst R_F 2.9 ulp, R_D 2.0, R_J 3.1, R_C 1.3. Complex R_F on conjugate pairs: 3.0 ulp, with 9 of 40 refused as too near the cut. The stated bound is 16 ulp.
- **Checks needing no reference:** equal-argument and homogeneity identities; Legendre's relation EK′ + E′K − KK′ = π/2 to 2.5 ulp; K against the arithmetic-geometric mean to 1.2 ulp.
- **Continuity:** R_C through y = x, on its series to 10⁻¹⁵, and R_J monotone as p passes an argument.
- **The first-kind reduction:** 50 intervals (complex-root pairs, real roots outside, rays at s ≈ 500) within 3.8×10⁻¹⁴ of the references.

**Not yet graduated.**
- **The crescent** needs ∫√(Q₁Q₂) ds. Reducing √(quartic) brings in all three kinds: t²/√P₄ is second kind, and t/√P₄ has residues at the quartic's two points at infinity, so it is third kind. Those reductions (R_D, R_J) come next, each verified against references before use.
- **The tori** reduce after the Euler substitution t = e^θ (√Q = c·cosh θ), which turns the nested radical into √(quartic in t) with quartic roots to find.
- Until then they keep the hybrid (§6u), at 5×10⁻⁸ to 1×10⁻⁶ relative error.

*Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-19T14:53 PDT. Human origin: Zachary Zhang (adopting the Sun's verdict). The hierarchy, distinctions and acceptance criteria are the Original Prism Sun's; the implementation, the branch condition and its repair, the R_J repairs and the reference discipline are this session's.*

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
