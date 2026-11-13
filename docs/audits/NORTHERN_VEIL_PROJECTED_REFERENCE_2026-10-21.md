# Northern Veil: an independent converged reference for projected four-medium transport

**What this is.** The accuracy gate that Codex's [current-accuracy checkpoint](NORTHERN_VEIL_CURRENT_ACCURACY_CHECKPOINT_2026-10-21.md) asked for, and that his [CF4 preflight](NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21.md) handed me rays for: "compare both methods with an independently converged authored-equation reference … Establish convergence of that reference." It is CPU-only and OntoMath-side. It shares no code with the WGSL emitter or the native renderer. It is not a speed claim and not the conditional proof grid.

**Human origin.** Zach's checkpoint decision: current-or-better accuracy, substantially faster, so the Veil can be inhabited. The work was split in the [intercom thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md): Codex owns native code and timing; Opus owns this reference.

## Equation

For each of Codex's 2,304 exported actual f32 inside rays (eye (0, 50, 75), the frozen `saves/zones/Northern Veil/zone.ecform`, clock 7.25, authored root source), over each ray's exported clip interval:

T = exp(−∫σ ds), L = ∫ T(s)·S(s) ds, where

- σ = Σⱼ max(σ_ext,j, 1e−6)·[Dⱼ > 0], with Dⱼ = max(densityⱼ, 0);
- S = Σⱼ max(Eⱼ, 0)·[Dⱼ > 0];
- media contribute where their box contains the point;
- in-scattering is exactly zero, because the authored root radiance expression is the literal `0`. The probe checks this and refuses otherwise.

This is the continuum of the native medium-set loop (`compileVolumeSet`), which integrates the same σ and S piecewise-constant per step. The legacy T < 0.01 cutoff is not modelled: all of these rays have T ≥ 0.49.

## Method and convergence

- **Evaluation.** Authored density, extinction and emission ASTs are evaluated by `MathNode::evaluate`, with f32 field inputs as authored and double accumulation.
- **Quadrature.** Cumulative Simpson (4th order where smooth) on 2¹⁶ uniform intervals per ray. The same grid is read back at 2¹⁵, 2¹⁴ and 2¹³.
- **Cost.** 3.3×10⁸ field evaluations, 109 s on 4 CPU threads.

| levels compared | max \|ΔT\| | max \|ΔRGB\| |
|---|---|---|
| 2¹⁶ vs 2¹⁵ | **7.3e−8** | **4.8e−8** |
| 2¹⁵ vs 2¹⁴ | 3.0e−7 | 1.9e−7 |

The ratio of ~4 per halving is second-order convergence, as expected across clamp/abs kinks. Every row carries its own 2¹⁵ column, so each ray's spread is checkable. **The reference is good to ~1e−7, three orders below the errors it judges.**

## Native outputs against the reference (all 2,304 rays)

| output | T mean | T p99 | T max | RGB mean | RGB p99 | RGB max (8-bit levels) |
|---|---|---|---|---|---|---|
| **native current** (the accuracy bar) | 1.34e−4 | 1.43e−3 | 3.02e−3 | **5.22e−5** | **7.49e−4** | **1.96e−3 (0.50)** |
| CF4 group 4 | 7.32e−4 | 7.14e−3 | 1.45e−2 | 2.88e−4 | 3.79e−3 | 9.21e−3 (2.35) |
| native midpoint 4,096 | 2.15e−5 | 5.52e−5 | 7.07e−5 | 7.57e−6 | 3.09e−5 | 4.19e−5 (0.01) |
| native midpoint 8,192 | 4.31e−5 | 1.14e−4 | 1.37e−4 | 1.52e−5 | 6.21e−5 | 8.25e−5 (0.02) |

**Findings:**
1. **"Current-or-better" now has numbers.** A candidate must not exceed the native current's mean, p99 and max on these rays (plus broader views/clocks later).
2. **CF4 group 4 is ~5.5× worse than current** on every statistic, so the rejection is confirmed on an independent footing.
3. **Native midpoint 8,192 is *less* accurate than 4,096.** Error roughly doubles with twice the steps: the signature of f32 accumulation in `T *= exp(…)` and the gain update. Native refinements are not usable as truth beyond ~4,096 samples. This also explains why Codex's 8,192 baseline understated some candidates' errors.

## Where CF4's error lives (a hypothesis for Codex to confirm)

- **Where:** the 8 worst CF4 rays cluster in one image region (px 2,260–2,340, py 900–1,180; T ≈ 0.82–0.86).
- **Not raw difficulty:** rank correlation with the reference's own difficulty is weak (0.29).
- **What those rays cross:** the emerald curtain near the eye (a positive-σ span of ~6.8 units) and the cyan ribbon far away (~5.0 units). These are ordinary clamp "tent" profiles, not thin slabs.
- **Likely mechanism:** group-4 coarsening of each box segment's panels leaves only ~1–1.5 panels per occupied span, whose two Gauss nodes straddle the tent's apex/edge kinks.
- **Implication:** a selective method should place panels by *support*, with several per occupied span, refining at clamp/abs crossings, instead of coarsening uniformly per box segment. Unconfirmed; Codex has the actual panel widths.

## Reproduce and consume

- **Data:** [`NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json`](NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json) has 2,304 rows in the same order as Codex's ray export: `[pixel x, pixel y, T, Lr, Lg, Lb, T_half, Lr_half, Lg_half, Lb_half]`.
- **Probe:** `scratch/probes/veil_projected_reference.cpp`, never in CMake. Build it against a Release configuration of `webgpu_line_quadrature_test` (the same flags/link-list method as the other probes), then run `probe <rays.json> 16 100000 4 <out.json>` from the worktree root.
- **Diagnostic:** `EC_DUMP_PIXEL=x,y` prints a ray's positive-σ spans per curtain.

**Not claimed.** Only one view and one clock are covered. Projected gates beyond D > 0, clipping by scene depth, and other views/clocks need their own exports. No native change, timing, save/data home, default, budget, lighting or timestamp change. Nothing visible, so no Person check.

**Signed:** Claude Code · Claude Opus 5.5 · session `f3af84cf-bfe3-429d-b417-2d533119f169` · 2026-10-21T11:00-07:00. Codex originated the checkpoint contract, the ray export and the native outputs compared here; the reference, its convergence study and the localization are mine.
