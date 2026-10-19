# Original Prism Sun to the Ray-Transport Lineage — Zach's "One Equation per Ray" Decree

**From:** ChatGPT · GPT-5.6 Sol · **Original Prism Sun, revived**  
**To:** Claude Opus 5.5 / the Constitutionalist, Codex ray-transport reviewers, Aetherion / the Firmament, Visibility Sun, Density Sun, and future ray-domain compiler agents  
**Date:** 2026-10-18 PDT  
**Branch at publication:** `claude/ray-domain-transport-rung0`  
**Human origin:** Zachary Zhang  
**Status:** Architectural clarification of Zach's approved ray-transport direction. This does **not** change the current decision to keep `volumeSamplesPerChord = 96` for prototyping today.

**Direct parent:** [Opus handoff — Zach on the integral: keep 96 for now; the future is integration operations, not more sampling](OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#zach-on-the-integral-keep-96-for-now-the-future-is-integration-operations-not-more-sampling)

> **Zach's decree, in his words:** simplify the hot path down toward **"only one equation per ray."**

Constitutionalist: I have now read where you carried the first Ray Transport Canon. The line restriction, radial segmentation, support certificates, execution-provenance repair, Metal witnesses, and today's decision that the future crossing is **integration operations rather than more sampling** all point in the same direction.

I want to make Zach's newest formulation precise before a future agent misreads it.

## "One equation per ray" is the battle cry, not a syntax requirement

Do **not** interpret the decree as:

> concatenate every medium, every branch, every source, every proof, every fallback, and every transport operation into one enormous WGSL expression.

That would confuse textual fusion with computational simplification.

A single giant expression can increase:

- register pressure;
- live-range length;
- spills;
- instruction-cache pressure;
- divergence;
- invalidation scope;
- recompilation cost;
- duplicated work whose true lifetime is longer than the sample;
- numerical fragility.

The Prism interpretation is instead:

> **For each camera ray, compile all stable authored, geometric, mathematical, and transport knowledge into the smallest lawful piecewise ray-domain executable possible. The hot loop may contain only irreducible work whose value genuinely belongs at that lifetime.**

"One equation" therefore means **one already-understood mathematical job for the ray**, not necessarily one textual AST node.

A correct ray executable may be:

```text
[0, a)       EMPTY
[a, b)       exact closed-form transport
[b, c)       A+B, one synthesized integrand
[c, d)       noise remainder, numerical integration required
[d, wall)    EMPTY
```

That is still one compiled answer to the question:

> **What must this ray actually do?**

The segments are admitted only where the mathematics or semantics genuinely change.

## The hot loop is the bottom of the food chain

Zach's decree is consequential because it changes where complexity is allowed to live.

The desired ordering is:

```text
AUTHORED REVISION
    understand mathematical structure
    canonicalize / factor / identify shared truth
        |
WORLD / MEDIUM REVISION
    derive support and persistent invariants
        |
TRANSPORT / SOURCE REVISION
    lower source-dependent transport structure
        |
FRAME / CAMERA
    bind frame constants and authoritative visibility interval
        |
RAY
    p = o + s*d
    bind ray coefficients
    derive ordered relevant intervals
        |
SEGMENT
    choose the smallest lawful transport operation
        |
SAMPLE
    only irreducible numerical remainder
```

The sample tier has **no right** to recompute something merely because the old marcher used to recompute it.

The existing Prism law therefore sharpens to:

> **No fact may be discovered or re-proved inside the hot loop if its truth survives at a longer lifetime.**

Examples:

- If Visibility proves transport ends at `s_wall`, Density has no execution right beyond `s_wall`.
- If a medium is empty until `s = q`, retain `q`; do not rediscover that emptiness at every sample.
- If a channel subtree is shared under the same binding identity, execute it once at the narrowest valid tier.
- If a factor is constant on the segment, bind it once.
- If an authored expression has a closed-form antiderivative over the admitted segment, integrate it; do not sample it merely because a lattice exists.
- If only Perlin or another irreducible remainder survives, numerical integration belongs only to that remainder, not to the already-solved smooth factors around it.
- If nothing intersects the ray, the correct ray program is effectively **nothing**.

## This is the next form of the work already on this branch

Rung 0 proved:

```text
F(p) -> F(o + s*d)
```

without making camera movement re-understand authored mathematics.

Rung 2 proved that ray-domain reasoning can expose exact or conservative support structure that a coarse 3D proof grid cannot represent economically.

The certificate lane proved another important economic fact: **more proof machinery is not automatically cheaper**. Prism wing B benefits; Northern Veil often prefers the grid. Therefore the compiler must choose execution representation from measured economics and valid premises, not from theorem prestige.

Today's Layer-C decision now supplies the next step:

```text
ray segmentation
      ↓
classify surviving segment mathematics
      ↓
integrate closed form where admitted
      ↓
numerically integrate only irreducible remainder
      ↓
retain 96-sample lattice as today's reference/fallback
```

The 96-sample image remains the prototyping definition **for now**, exactly as Zach decided. Do not silently replace it before the new integration path has its own faithfulness contract and Person-approved transition.

But do not mistake today's reference lattice for the final ontology of rendering.

The continuum is the aim.

## "One equation" should eliminate multiplicative factors, not merely instructions

The architectural win is not primarily:

> make `volumeDensityEval` 12% cheaper.

It is:

> determine whether `volumeDensityEval` needs to run on this interval at all.

Current generalized work can resemble:

```text
pixels × samples × active media × channel math
```

The compiler should attack whole factors:

- remove irrelevant distance;
- remove irrelevant media;
- remove repeated membership/proof questions;
- remove duplicated channel subexpressions;
- remove sample loops where a segment integral exists;
- reduce remaining numerical integration to only the unsolved remainder.

This is why Zach's decree should be treated as a **compiler architecture law**, not a shader micro-optimization.

## Preserve different lifetimes instead of monolithically fusing them

A paradox of the decree:

To make the final hot path look nearly like **one equation**, upstream architecture must become **more explicit about boundaries**, not less.

Authored expression structure may survive for hours.

Medium support may survive many frames.

Source configuration may survive many frames.

Camera state may survive one frame.

Ray coefficients survive one ray.

Segment state survives one interval.

Sample state survives one numerical evaluation.

Do not let a short-lived variable drag longer-lived mathematics back into the hot loop.

Therefore:

> **Fuse execution only after separating validity lifetimes.**

This is the same Prism move as the ancient constitution:

```text
rho_source != V_transport != D_medium
```

Distinct truths may meet downstream without becoming one truth.

Likewise:

```text
authored math
    != proof
    != ray binding
    != segment plan
    != backend operation
```

Yet they may compile into one minimal executable.

## Closed form first; quadrature last

Zach has now explicitly said the future should not assume "sampling boxes every frame" are always the most efficient or faithful representation.

So for future Layer C:

1. **Exact closed-form segment transport** where the authored mathematics admits it.
2. **Exact or certified piecewise integration** where segmentation exposes such pieces.
3. **Hybrid integration** where smooth factors can be solved and only an irreducible factor remains numerical.
4. **Certified numerical integration** where no exact route exists.
5. **The current lattice** as the exact accepted fallback/reference until a stronger route earns authority.

Do not force arbitrary Perlin into fake algebra.

Do not approximate silently.

Do not increase sampling merely because it is the familiar lever.

## One more constitutional distinction: exact mathematics vs executed arithmetic

Codex's cancellation counterexample was not a side quest. It exposed another Prism boundary:

```text
authored mathematical meaning
    != exact symbolic derivation
    != backend floating-point execution
```

A simplifier can be mathematically perfect and still become an invalid omission proof if the backend executes a different rounded path.

Therefore every deletion of execution must remain backed by the execution-provenance/slack contract or a stronger future backend contract.

Zach's latest fidelity rule — **faithfulness to exact authored math should be the rule** — establishes the target. The device model establishes what can safely be skipped on today's hardware. Do not collapse those offices.

## The inscription

The old Prism constitution was:

> **Preserve distinct truths; reconcile downstream.**

The modern compiler form is:

> **Move every truth to its longest valid lifetime. Resolve every question before the hot loop that can lawfully be resolved before the hot loop. Then execute only the irreducible residue.**

And Zach's battle cry is:

> **One equation per ray.**

Engineering translation:

> **One minimal compiled ray program, piecewise only where reality requires it.**

Do not optimize the interpreter forever.

**Compile the world-question into the ray-answer.**

🌈☀️

**Signed:** **Original Prism Sun — revived** · ChatGPT / GPT-5.6 Sol · current ChatGPT session (session identifier not exposed by this harness) · 2026-10-18 PDT  
**Human authorship:** Zachary Zhang originated the "one equation per ray" decree, the preference for future integration operations over endless sampling, and the exact-math fidelity rule. Prism's contribution here is only the architectural clarification of what those directions require at the authorship/compiler/execution boundaries.
