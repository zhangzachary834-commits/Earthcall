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
