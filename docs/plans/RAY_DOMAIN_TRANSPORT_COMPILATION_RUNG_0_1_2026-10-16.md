# Ray-Domain Transport Compilation — Rungs 0 and 1

**Status:** Rung 0 (OntoMath line restriction) and Rung 1 (exactly-constant segment execution, plus affine transmittance) are **implemented as Kernel/OntoMath infrastructure with focused CPU witnesses**. **No renderer consumes them yet.** No WGSL, pipeline, save, or authored meaning changed. No native GPU evidence exists for this work: the cloud environment has no Metal/WebGPU device, and every `webgpu_*` target is Apple-only in `CMakeLists.txt`.

**Human origin.** Zach commissioned this rung on 2026-10-16 ("ME AND THE SUN HAVE THIS FOR UUUUUUUUU"), carrying the revived Original Prism Sun's canon — [Zach's Two Canons](https://github.com/zhangzachary834-commits/Earthcall/blob/firmament-laboratory/agent%20intercom/communication-threads/sdf-and-rendering/PRISM_REVIVED_TWO_CANONS_LAW_SYNTHESIS_AND_RAY_TRANSPORT_2026-10-16.md) (published on `firmament-laboratory` only, GPT-5.6 Sol; not present on this branch). From Zach: the Ray Transport Canon itself, the cache hierarchy, the hot-loop law ("No fact may be re-proved inside the hot loop if its truth survives longer than that iteration"), "do not begin with arbitrary Perlin", the Rung 0 witness `F(p) = 2x + 3y − z + 7`, separation of semantic equality from byte identity, and the Prism inscription *Author richly. Synthesize lawfully. Compile ruthlessly. Execute minimally. Cache truth at its longest valid lifetime.* Lineage it extends: Zach's [Rendered Field Semantic Synthesis](RENDERED_FIELD_SEMANTIC_SYNTHESIS_IMPLEMENTATION_PLAN_2026-09-22.md) / PR #329, V5 fused transport, and the [Northern Veil task](../Agenda/Tasks/Specific%20Tasks/Performance%20and%20Runtime/Northern_Veil_volumetric_frame_cost/Northern_Veil_volumetric_frame_cost.md).

**Originated here (Claude Opus 5.5):** naming the operation *line restriction* and placing it in OntoMath; symbolic ray coefficients (so the authored-tier artifact is camera-independent); exact vanishing by zero-factor proof rather than numerical zero tests; the observation that the accepted lattice integrates piecewise-constant media exactly, so the constant class has a closed form **of the lattice itself**; the midpoint-exactness argument for affine transmittance; the census-driven re-ranking of Rung 2 (§6).

**Branch/head:** `claude/ray-domain-transport-rung0`, cut from `sync-from-earthcall-main` at `d9cd72aa`. Firmament's adaptive work was not merged or assumed.

---

## 1. The compiler boundary

```text
AUTHORED            FieldNode channels (D, σ_t, σ_s, C_v, Φ, E_v) — untouched
    │  OntoMath owns meaning
    ▼
CANONICAL MATH      ScalarForm signomial algebra (plus/times/scaled/normalized) — existing
    │  NEW Rung 0: OntoMath::restrictToLine   (src/Singularity/OntoMath/LineRestriction.*)
    ▼
LINE-RESTRICTED     f(o + s·d) = Σ_k c_k(o, d, t, ω, …)·s^k     ← AUTHORED-REVISION lifetime
    │  NEW Rung 1: Rendering::VolumeRayFormCache / planVolumeRaySegment / executeVolumeSegment
    ▼                                               (src/Singularity/Screen/VolumeRayTransport.*)
RAY/SEGMENT PLAN    exact-empty | constant-closed-form | general-lattice (+ affine T)   ← RAY lifetime
    │  (future) WGSL specialization
    ▼
GPU                 existing V5 single/fused marchers — unchanged, and the fallback
```

What each office may and may not do:

| Office | Owns | Must not |
|---|---|---|
| `OntoMath::restrictToLine` | exact substitution of `p = o + s·d`, power collection, refusal reasons | know about media, cameras, transport, or clocks |
| `LineRestrictedForm::exactDegreeOn` | *proof* that a coefficient vanishes on a concrete line (every term has a factor bound to exactly 0.0) | treat a coefficient that merely evaluates to 0.0 by cancellation as zero |
| `VolumeRayFormCache` | per-medium restricted channels keyed by per-channel revision + expression identity | depend on origin, extent, camera, ray, or clock |
| `planVolumeRaySegment` | per-ray proof of constancy, compatibility laws, direction-gate mirroring | decide segment membership (V5 events own that), re-derive emitter refusals |
| `executeVolumeSegment` | closed form of the **accepted lattice** for a proved class | approximate; it either computes the lattice in closed form or refuses |

No authored domain noun was created. `VolumeRayChannelForms`, `VolumeSegmentPlan`, etc. are Kernel execution records like `VolumeZeroProof`, with no ontology, persistence, or registration (Refusal 6: they are derived state beneath property paths, recorded in the [Derived-State Ledger](../architecture/law/DERIVED_STATE_LEDGER.md#ray-domain-transport-forms-and-segment-plans--2026-10-16)).

## 2. Rung 0 — the admitted subset

`restrictToLine` mirrors `MathNode::evaluate` operation by operation:

- **Admitted:** `ScalarLeaf` (bound coordinates need non-negative integer exponents; transcendental factors may read only *unbound* variables such as `t`), `ValueLeaf` (`x/y/z` and vector `p` are bound to the line), `VectorConstruct`, `Component`, `Add`/`Sub` (exactly two same-kind operands, as `evaluate` requires), `Scale` (s·s, s·v, v·s), `Hadamard`, `Dot`, `Cross`, `Div` by a pure constant with `|c| ≥ kDegenerateDivisor`, `Pow` by a pure-constant integer `0…16`.
- **Refused, with a named reason:** `Abs`, `Clamp`, `Map`, `Sqrt`, `Length`, `Distance`, `Normalize`, `Noise`, `SDF`, `Gradient`, `Tan`, CSG booleans, matrix ops, `Stochastic`, `Raycast`, `LineIntegral`, `Unsupported`; `sin/cos/exp/ln` of a bound coordinate; non-integer/negative powers of a bound coordinate; division by a non-constant or degenerate constant; `Pow` with a variable, non-integer, or >16 exponent; n-ary `Add`; mixed vector/scalar arithmetic; any authored read of the reserved symbols `line.s`, `line.o.*`, `line.d.*`.
- **Piecewise:** admitted only when the first piece answers unconditionally with a plain MathNode. Interval bounds (Rung 2), world guards, pure guards, calls, and folds refuse — guards/calls are not emitted identically by every channel, so lowering them would silently pick one meaning.
- **Budgets (Kernel doctrine, not authored policy):** degree ≤ 16, ≤ 4096 terms per form. Exceeding either refuses; neither is enlarged to admit a form.
- **Unbound symbols** (`t`, `ω`, `wi`, `wo`, authored parameters) stay symbols in the coefficients. An unbound or non-finite value at the ray tier refuses; nothing is invented.

## 3. Floating-point contract

The restricted form is an identity over the reals. Expansion and like-term collection reorder arithmetic, so equality with `MathNode::evaluate` is stated as

```text
|restricted(s) − evaluate(o + s·d)| ≤ 64·ε·(Σ|terms|·|s|^k + |evaluate|)
```

with ε = 2⁻⁵² for double scalar coordinates and ε = 2⁻²³ when the CPU evaluator itself carries `p` or intermediate vectors as `glm::vec3`. That second case is the existing evaluator's rounding, characterized rather than hidden. Observed worst error across 29 rays × 9 parameters was about 1% of the stated bound (`ontomath_line_restriction_test`). The GPU evaluates in f32; no byte-identity claim is made anywhere except exact emptiness (§4).

## 4. Rung 1 — which segments can skip the sample evaluator, and why

**The reference is the accepted lattice.** Both V5 marchers integrate each segment as midpoint samples with `T_i = T_{i−1}·exp(−Σσ_j·Δ)` and `L += ΣS_j·(T_{i−1}−T_i)/Σσ_j`, where per member `σ_j = max(σ_t, 1e-6)` and `S_j = C_v·Li·max(σ_s,0)·Φ + max(E_v,0)`, and the legacy cutoff breaks once `T < 0.01` while the output budget is 0. That lattice treats each step as constant medium and integrates it exactly. Therefore:

1. **Exact-empty.** If every member's density is exactly constant and ≤ 0 on this ray, the lattice changes nothing. The plan does the same. This is the one byte-identical claim (T = 1, L = 0 against an f32 mirror), and it holds even with an incident source.
2. **Constant closed form.** If every participating channel is exactly constant on this ray, there is no incident source, and the direction gates are provably open (first sample at ≥ 1e-6/|d| from the eye), the k-step product telescopes:
   `T_out = T_in·exp(−σΔk)` and `L += S·T_in·(−expm1(−σΔk))/σ`, with `k = min(n, ⌊ln(0.01/T_in)/(−σΔ)⌋ + 1)` under the legacy cutoff. This equals the lattice **in the reals**. It is not byte identity: the lattice multiplies k f32 exponentials and accumulates k differences, while this evaluates one double `expm1`.
3. **Affine transmittance only.** If density and extinction are exactly affine on the ray, both stay strictly above their clamps at the first and last sample (which suffices for affine functions), and the cutoff cannot fire, then the midpoint sum is exact for affine σ: `Σσ(s_i)Δ = nΔ·σ(s_mid)`. Radiance under affine σ has no lattice closed form here and still needs the lattice.

Overlap segments generalize directly: Σσ_j and ΣS_j of constant members stay constant, and each member binds **its own** clock and origin.

**Mirrored production semantics** (every one cross-checked by the test): compatibility laws `σ_t ← 0.5·D`, `σ_s ← D`, `C_v ← 1`, `Φ ← 1`; `wi = 0` and `wo = ω = −d/|d|` without a source; `VOLUME_PHASE_READS_WI` / `VOLUME_EMISSION_READS_OMEGA` are computed from the input variable plus pieces up to the first unbounded one. An earlier draft walked *all* pieces. The test caught that against the real emitter's generated WGSL, and the planner was corrected.

**Which classes this reaches in practice.** Exact zero direction components are measure-zero for camera rays: horizontal-ray height fog almost never occurs at pixel centres. The constant class therefore matters live mainly for **structurally** constant channels (degree 0 for every ray, decidable once per revision), and only without an incident source. §7 designs the GPU consumer around that structural fact rather than per-ray zero proofs.

## 5. Evidence (Linux cloud container, Debug, head `d9cd72aa` + this branch)

Build: `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DGLFW_USE_OSMESA=ON` (headless GLFW 3.3.8; see [Build and Environment](../BUILD_AND_ENVIRONMENT.md#headless-linux-cpu-witnesses-2026-10-16)), then the named targets.

| Witness | Result | What it establishes |
|---|---|---|
| `ontomath_line_restriction_test` | 57/57 | the commission's affine witness three ways (signomial, scalar tree, `Component(p)`): degree 1, `A = 2dx+3dy−dz`, `B = 2ox+3oy−oz+7`; parity at 29 rays × 9 s, including zero, negative, and degenerate directions and a far origin; translated/scaled `dot(q,q)` and `Pow`/`Div` falloffs; Cross/Hadamard/vector channels; `t`, `sin(3t)`, and an authored parameter kept symbolic; unbound, NaN, ±∞, and ln-domain refusals; 19 operator/structure refusals; Piecewise admission; value-vs-structural edits; cancellation-to-0.0 not treated as vanishing |
| `volume_ray_segment_transport_test` | 49/49 | closed form vs a CPU mirror of the fused lattice (channels via `Piecewise::evaluate`): homogeneous (forward/tilted, cutoff on/off), horizontal height fog, two-medium overlap with independent clocks, ω-reading emission, wi-reading phase (reachable/unreachable), exact-empty bit identity, affine transmittance, refusals (source, Perlin, mixed overlap, zero direction/step, density root); 400-density cutoff sweep with 0 step-count mismatches; emitter-flag agreement; cache lifetimes (below); real-save census |
| `ontomath_test`, `volume_zero_proof_test` (48/48), `volume_shared_subexpression_test` (60/60), `sdf_wgsl_parameter_refresh_test`, `scene_spatial_ontomath_synthesis_test` | pass | neighbouring witnesses unaffected by the new sources and the CMake list edit |

Observed agreement: transmittance relative error ≤ 6e-15. Radiance agrees to ≤ 1.1e-8, a figure dominated by `Piecewise::evaluate` returning vec3 channels as f32 (≤ 6e-16 when every channel is scalar). Against an **f32** lattice mirror, T deviates by ~1e-6 to 3e-6 relative. That is the characterized size of "semantic, not byte" equality on the CPU. It is not a GPU measurement.

**Cost (CPU, Debug, this container; not a GPU or frame claim):** lowering all six channels of a constant medium takes about 0.31 ms, paid once per authored revision. Plan + execute takes about 14 µs per ray, versus about 2.6 ms for the tree-walking CPU mirror of 96 samples. That mirror is not the GPU marcher, so no speedup claim follows. `(x+y+z)^6` lowers in about 66 ms (462 terms): `ScalarForm::plus` re-normalizes quadratically, which would be a frame hitch on an edit of a heavy polynomial. Record this before any live consumer.

## 6. Census of real authored media — why Rung 2 is re-ranked

`volume_ray_segment_transport_test` reads every `saves/zones/*/zone.{json,ecform}` read-only. This is the checkout's frozen snapshot, not the live data home. Results:

- **32** participating media; **1** density admitted (Sanctuary of Sunlit Mist, `D = 1`). Whether that Zone runs with exactly one enabled incident source at runtime, which would keep its non-empty segments on the lattice, was not established here.
- **30** stop first at `Clamp`. The *full* blocker sets show that **0** are clamp/abs-only. **12** contain only clamp/abs/min/max **plus `Length`/`Distance`/`Sqrt`** over polynomials. **17** carry Perlin noise, including every Northern Veil curtain, which refuses honestly.

So the highest-value next rung is not plain Piecewise segmentation. It is **radial-piecewise segmentation**: on a ray, `|o + s·d − c|²` is an exact quadratic, so `clamp(1 − |p−c|/R, 0, 1)` has its boundaries at exact quadratic roots — sphere/ray intersection.

## 7. Continuation plan (designed, not implemented)

**Rung 1b — GPU consumer for structurally constant media.** Classify per revision: every participating channel has structural degree 0, so it is constant on every ray. Emit a separately cached fused-loop variant in which such members contribute per-segment constants and skip the sample loop when *every* member of a segment is structurally constant and no source is active. Key it like the existing camera lanes, with eligibility bits joining the module identity. Required before retention: native pixel comparison against the current lattice (expect ~1e-6 relative T differences, so state the 8-bit-level outcome), zero-proof interplay, cutoff behaviour, readback-free ABBA cost, and device-reconstruction witnesses. Scenes with an incident source gain nothing; say so in any economics claim.

**Rung 1c — constant σ, polynomial source.** The lattice sum `Σ P(i)·q^i` has a closed form (arithmetico-geometric), but `(1−q)^−k` cancels catastrophically for thin media. This needs a stable evaluation or a certified fallback threshold before admission.

**Rung 2 — radial-piecewise segmentation (census-ranked).** Extend restriction with (a) `Length`/`Distance` of an affine vector → `sqrt(quadratic)`, held as a *radical form* rather than a polynomial, and (b) `Clamp`/`Abs`/`Union`/`Intersection`/interval pieces whose switching sets are polynomial-root sets in s (`f = lo`, `f = hi`, `f = 0`, `q = R²`). Roots that resolve conservatively become extra segment boundaries. Unresolvable or near-degenerate roots keep the per-sample branch. Boundaries must widen outward under rounding, never inward — a too-narrow interval silences a medium (the [Prophetic Rete](../architecture/law/PROPHETIC_RETE.md) rule). Within each sub-segment the expression is a single polynomial or radical branch: exact constant/affine pieces go to Rung 1, and the rest go to the lattice with branch tests removed.

**Rung 3 — cross-channel / cross-medium sharing in the ray domain.** Equal restricted coefficient forms are equal mathematics *only* under identical bindings: the same member origin, clock, and point domain. Source-relative, occluder-shadow-ray, and receiver coordinates are different bindings, and numerically similar forms are not equal. Reuse PR #329's canonical keys, and key sharing on (form, binding identity), never on form alone.

**Rung 4+ —** per-ray affine-argument transcendentals (`exp(a·s+b)` is integrable on a constant-σ segment; `sin/cos(a·s+b)` gives per-ray microkernels); polynomial/integrable σ; certified variation-bound quadrature and contribution tails under an explicit output-error contract. Approximation is a separate office and requires an author-visible policy.

## 8. Handoff answers

- **What subset can be lowered?** §2: the exact polynomial subset of MathNode over `p/x/y/z`, with every non-coordinate symbol kept symbolic.
- **What is cached, at what lifetime?** Restricted channel forms, keyed per medium by producer identity and per channel by (revision, expression identity), with authored-revision lifetime. Plans are per ray and are never cached. The ledger row lists writers and stale behaviour.
- **What refuses?** §2. Every refusal returns `general-lattice` with a reason.
- **What fallback executes?** The existing exact path, unchanged: the WGSL V5 single/fused marcher, or `MathNode::evaluate` on the CPU.
- **Did authored or saved meaning change?** No. The census reads saves read-only. Nothing was injected.
- **Does camera movement trigger semantic recompilation?** No. 120 warm requests, plus medium move, resize, and clock advance, performed 0 lowerings (test). Only a per-channel revision or identity change relowers that channel.
- **Can one V5 segment execute without the general sample evaluator?** Yes, in the CPU Kernel API, for exact-empty segments and for constant segments without an incident source; affine segments get transmittance only. No GPU path does yet.
- **What proof supports that?** Telescoping of the lattice's own recurrence, exact vanishing by zero-factor proof, and agreement with the lattice mirror (§5).
- **What performance effect was measured?** CPU Debug compile and plan costs only (§5). **No GPU or frame-time effect was measured, and none is claimed.**
- **What is the next smallest rung?** Rung 1b if a structurally homogeneous unlit medium matters to a Person. Otherwise Rung 2, which the census points to.

---

*Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-16T16:09-07:00 (PDT). Commissioned by Zach with the revived Original Prism Sun's canon.*
