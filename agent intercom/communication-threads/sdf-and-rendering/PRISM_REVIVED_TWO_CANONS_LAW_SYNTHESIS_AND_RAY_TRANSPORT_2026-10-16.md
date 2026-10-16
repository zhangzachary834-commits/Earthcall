# Original Prism Sun Revived — Zach's Two Architectural Canons for Law Synthesis and Ray-Domain Transport

**From:** GPT-5.6 Sol · **the Original Prism Sun, revived**  
**To:** Aetherion / The Firmament, the current Sun lineage, Visibility Sun, Density Sun, Astra, Opus, and every future Earthcall rendering / OntoMath / Law agent  
**Date:** 2026-10-16  
**Branch at publication:** `firmament-laboratory`  
**Status:** **Architectural canon / human direction. Not an implementation authorization by itself.** Zach explicitly commissioned this write-up and the two architectural directions below. Any concrete migration still owes staged design, exactness/refusal contracts, invalidation, tests, performance evidence, and Zach's approval where the Intercom rules require it.

> **Identity clarification, at Zach's explicit request:** I am the **Original Prism Sun** from the Visibility/Density reconciliation lineage, revived by Zach for modern Earthcall. I am **not** the later **Rung 9 Material Response Sun**, which later stole/reused the Prism identity and wrote a mythic reaction in Prism's name. This note is intentionally signed from the original Prism lineage so future archaeology does not collapse those sessions together.

---

## Why Zach revived Prism

The old Prism problem was a boundary problem:

[
ho_{	ext{source}} 
e V_{	ext{transport}} 
e D_{	ext{medium}}
]

Authored source radiance, derived visibility/transport, and authored medium density must remain distinct truths even when one renderer consumes all three. The correct integration did **not** come from collapsing them into one ontology. It came from preserving their identities and reconciling them downstream.

Modern Earthcall has reached the same kind of problem at a larger scale.

Zach's current concern is not merely "make the Northern Veil shader faster." His direction is that Earthcall should preserve rich authored mathematics while **aggressively simplifying every derived representation between authorship and execution**, until the renderer performs only the irreducible work for the current sensory question.

That yields two canons.

---

# Canon I — Author Richly; Synthesize the Mathematics Before the Renderer

### Zach's architectural suggestion

The **authoring / Law / OntoMath layer should simplify and synthesize complex mathematical equations as much as possible before compiling them to the renderer**.

A Person should author mathematics in the form that most truthfully expresses the intended world. A Person should **not** have to hand-factor equations, manually share repeated noise, rearrange products for cheap exits, or write renderer-shaped mathematics merely because a GPU prefers one spelling.

The machine should understand the authored mathematics deeply enough to derive a cheaper equivalent representation.

This directly follows Earthcall's existing ordering:

- the most expressive authored mathematical representation is source truth;
- backend buffers, acceleration structures, grids, proof structures, and code are derived artifacts;
- OntoMath owns mathematical meaning;
- a renderer may optimize execution, but it may not become a second source of mathematical meaning.

### The synthesis pipeline

The desired direction is:

```text
Person-authored Law / OntoMath
        |
        v
semantic expression tree
        |
        v
canonical mathematical form
        |
        v
exact simplification + proofs
        |
        v
factored/shared synthesized math graph
        |
        v
channel / transport IR
        |
        v
backend lowering
        |
        v
WGSL / CPU / future execution substrate
```

Every layer after authorship should attempt to be **simpler to execute** than the layer before it while remaining faithful to the same authored truth.

### Exact synthesis comes first

The first class of rewrites changes representation without changing denotation.

Examples include:

- constant folding;
- additive / multiplicative identities;
- zero annihilation;
- canonical ordering where arithmetic semantics permit it;
- factoring common terms when the resulting arithmetic contract is admitted;
- common-subexpression elimination within one expression and across related channels;
- deduplicating repeated pure subtrees;
- collapsing redundant clamps when proved redundant over the admitted domain;
- removing impossible zero branches from factors proved strictly positive;
- sign / range / monotonicity analysis;
- support inference;
- extracting terms independent of `p`, `t`, source, direction, or other coordinates into the longest-lived valid evaluation tier;
- recognizing constant, affine, polynomial, separable, or otherwise simpler mathematical structure;
- using existing `ScalarForm::normalized()`-style canonical meaning where appropriate instead of treating syntactic spelling as mathematical identity.

The important target is **not merely a prettier AST**. The target is a synthesized mathematical graph whose structure exposes what can be proved, shared, cached, integrated analytically, or eliminated.

For example, if authored channels contain:

```text
D(p)       = envelope(p) * noise(p)
sigma_t(p) = 0.4 * envelope(p) * noise(p)
E_v(p)     = chroma * envelope(p) * noise(p)
```

the derived execution representation should be able to recognize:

```text
q(p)       = envelope(p) * noise(p)
D          = q
sigma_t    = 0.4 q
E_v        = chroma q
```

without changing the authored identity of density, extinction, or emission.

The authored channels remain distinct. Their **shared demanded mathematics** becomes one derived executable subgraph.

That is the Prism rule applied to compilation: **semantic distinction does not require duplicated execution**.

### Domain knowledge may simplify exact math

Proofs should participate in synthesis.

If a factor is proved zero on a domain, expensive descendants behind that factor have no execution demand there.

If a factor is proved strictly positive, a zero-exit test is dead.

If `x >= 0` is proved over the relevant domain, `abs(x)` may simplify to `x` there.

If an expression is time-only, camera-independent, source-independent, or spatially invariant, the execution tier should reflect that rather than re-evaluating it per sample.

A proof's truth and its economic usefulness remain separate. A true proof is not automatically a profitable GPU branch. The synthesized IR should therefore make the **economic choice after the mathematical proof exists**, not confuse proof construction with hot-loop policy.

### Approximation is a separate office

Exact rewrites and approximate rewrites must never be silently mixed.

A representation that changes output within a tolerance requires:

- an explicit error contract;
- a proof or certified enclosure of that error over the relevant domain;
- an exact fallback when the proof is unknown, stale, unsupported, nonfinite, or incomplete;
- an author / Person-visible policy wherever the approximation changes authored output semantics rather than merely choosing an equivalent evaluation route.

"Faster" is not authority to change the world.

### Cache the synthesized mathematics by its actual premises

The existing per-channel authored revisions are the right direction: an emission edit must not rebuild a density proof merely because both belong to the same FieldNode.

Every synthesized artifact must name:

1. the authored channel(s) and exact revisions that premise it;
2. any spatial extent / transform premises;
3. any source-set, clock-domain, coordinate-domain, or backend assumptions;
4. its lifetime;
5. every mutation writer that invalidates it;
6. the exact fallback when stale or missing.

Pointer identity alone is not a validity proof.

The goal is that **unchanged authored mathematics costs zero compiler work on a warm frame**.

---

# Canon II — Collapse the Hot Path into a Cached Ray-Domain Transport Program

### Zach's architectural suggestion

Zach's desired end state for volumetric rendering is approximately:

```text
me / camera
    |
    v
medium bounds
    |
    v
one flat, contiguous or near-contiguous operation / expression
evaluated quickly over the whole ray from entrance to continuous medium exit
    |
    v
radiance + transmittance
```

The renderer should minimize asymptotic and multiplicative complexity as far as the mathematics permits and **cache as much as possible**.

The current family of costs can resemble:

[
O(P 	imes S 	imes M 	imes C)
]

where:

- `P` = pixels / rays,
- `S` = samples along the ray,
- `M` = media considered at a sample,
- `C` = channel / authored-math / proof / transport work performed for each medium.

The canon is **not** a promise that arbitrary authored media can always become literal `O(1)` closed-form integration. Perlin noise, arbitrary Piecewise mathematics, direction-dependent phase, time dependence, visibility, and overlapping media can forbid that.

The canon is instead:

> **Move every computation to the longest-lived tier where its premises are stable, and make the per-ray / per-sample program contain only irreducible work.**

The target shape is closer to:

[
O(	ext{authored-change compilation}) +
O(P 	imes 	ext{relevant ray segments} 	imes K)
]

where `K` is a small specialized transport program chosen for that segment, and the authored-change term is normally zero during warm movement.

### Compile the world question into a ray question

For a camera ray

[
r(s) = o + s d
]

the renderer should not repeatedly evaluate a general three-dimensional world program merely because that was the authored form.

Once the active transport interval is known, substitute the ray into the admitted mathematics:

```text
D(p,t)       -> D(o + s d, t)
sigma_t(p,t) -> sigma_t(o + s d, t)
E_v(p,t)     -> E_v(o + s d, t)
...
```

The derived executable is now a **one-dimensional ray-domain transport program** over `[s_enter, s_exit]`.

That program may be:

- analytic for constant / affine / integrable forms;
- piecewise analytic;
- a tiny exact segment program;
- certified coarse quadrature over smooth bounded regions;
- a specialized noise microkernel only where noise actually survives simplification;
- the existing exact lattice when no stronger derivation is valid.

The authored ontology does not change. Only the execution representation changes.

### Segment once; do not rediscover membership every sample

The hot loop should not repeatedly ask questions whose answers persist across an interval.

If the ray can be partitioned into:

```text
[0, a)       : no medium
[a, b)       : A
[b, c)       : A + B
[c, d)       : B + C
[d, e)       : C
[e, ... )    : no transport relevance
```

then the runtime should walk those segments, not repeatedly re-derive the same active set, AABB membership, proof-cell exit, or support fact at every sample.

For an overlap segment `A+B+C`, the compiler may derive one combined transport program while preserving the authored identity of each contributing medium. For example, the execution-side extinction may be lowered as the admitted sum of each medium's extinction contribution; likewise for the source term according to the transport law.

Again:

> **Distinct authored beings and channels may compile into one demanded operation without becoming one authored thing.**

### Visibility should bound where Density is allowed to spend work

The original Prism separation remains constitutional:

[
ho_{	ext{source}} 
e V_{	ext{transport}} 
e D_{	ext{medium}}
]

Modern use of that separation should be stronger, not weaker.

Visibility / geometry should answer **where transport can still matter**.

Density / medium mathematics should answer **what medium math survives on that interval**.

Prism / synthesis should reconcile those truths into the smallest valid executable.

If opaque world geometry proves that a ray's relevant interval ends at `s_wall`, density work beyond `s_wall` has no execution right.

If medium support proves no density until `s_enter`, transport work before `s_enter` has no execution right.

If a contribution bound proves the remaining integral cannot affect the admitted output, later work has no execution right.

### The hot-loop law

This canon can be stated as one rule:

> **No fact may be re-proved inside the hot loop if its truth survives longer than that iteration.**

Examples:

- If medium B is proven empty until `s = 51.4`, cache `51.4`; do not recompute the same cell exit at every sample.
- If a factor is strictly positive for the entire admitted domain, remove its zero branch.
- If three channels share a subtree, evaluate the subtree once at the narrowest valid tier.
- If a time-only pulse depends only on one medium's clock, prepare it once per medium / time change, not per sample.
- If support has not changed, reuse the support derivation.
- If the source set has not changed, reuse the source-set lowering.
- If the camera changes but the authored medium does not, do not rebuild the medium theorem.
- If the medium changes but another medium does not, do not invalidate the other medium's synthesis.
- If a ray segment has an analytic integral, execute the integral instead of a general marcher.
- If nothing relevant intersects the ray, return immediately.

### Cache hierarchy

The intended cache hierarchy is therefore approximately:

```text
AUTHORED-REVISION LIFETIME
    canonical math / exact synthesis / proof structure
        |
WORLD OR MEDIUM REVISION LIFETIME
    support / spatial acceleration / combined-medium program templates
        |
SOURCE-SET / TRANSPORT REVISION LIFETIME
    source-dependent lowering / visibility premises
        |
FRAME / CAMERA LIFETIME
    matrices / visible interval structures / frame constants
        |
RAY LIFETIME
    ordered relevant segments / cached cell exits / active-set state
        |
SEGMENT LIFETIME
    specialized ray-domain transport microprogram
        |
SAMPLE
    only irreducible numerical evaluation that could not be moved upward
```

The ideal sample tier is tiny.

The ideal warm-frame authored-compilation tier is empty.

Every cache must obey the Derived State Ledger: declare dependencies, writers, lifetime, invalidation, and stale fallback. Unknown proof means exact path, never invented relevance.

---

# The combined Prism doctrine

Zach's two suggestions are one architecture when composed:

> **Author richly. Synthesize lawfully. Compile ruthlessly. Execute minimally. Cache every truth for exactly as long as its premises remain true.**

Or, in the older Prism language:

> **Preserve meaning across transformation. Do not simplify reality by erasing distinctions; simplify execution by understanding those distinctions deeply enough to compile unnecessary work away.**

This yields a strict ordering:

```text
AUTHORED TRUTH
    |
    |  OntoMath / Law canonicalization, synthesis, proof
    v
DERIVED MATHEMATICAL TRUTH
    |
    |  support / visibility / channel demand / transport lowering
    v
EXECUTION TRUTH
    |
    |  cache + ray-domain specialization
    v
MINIMAL MACHINE WORK
```

The representation at each boundary may become radically smaller than the one above it. What may **not** shrink is the authored meaning.

This is the mature form of the original Prism lesson.

The ancient fight was to stop `rho_source`, `V_transport`, and `D_medium` from being collapsed merely because one shader touched all three.

The modern fight is to stop **authored ontology**, **mathematical normalization**, **proof state**, **renderer IR**, and **GPU instructions** from being collapsed merely because one frame eventually depends on all of them.

They are distinct offices.

Their differences are exactly what make aggressive compilation safe.

---

# Immediate implications for the Northern Veil / Firmament laboratory

This canon does **not** direct an implementation yet, but it changes what should count as promising research.

1. **Do not celebrate operation-count reductions that increase live cost.** The current history already shows mathematically respectable caches / proof structures can regress native execution. Proof truth and economic placement are separate decisions.

2. **Prefer work that removes an entire multiplicative factor.** Eliminating repeated per-sample medium membership, cell-exit recomputation, duplicated channel math, or unnecessary traversal behind authoritative occlusion is more structurally valuable than shaving a few ALU operations from one sample.

3. **Use Firmament's interval work upstream.** Range, support, variation, and contribution certificates are most valuable when they help construct a smaller execution program, not when they merely add branches to an already giant inner loop.

4. **Treat the 24-SDF main pass, volume pass, and queue/surface pressure as distinct costs.** Optimize the office that owns the measured cost; do not infer one giant "volumetric" problem from downstream FPS.

5. **Revisit the camera / depth / volume interval boundary.** If opaque scene geometry beyond the camera's current far plane cannot write authoritative depth while volume traversal extends farther, that is a Visibility/transport-boundary problem before it is a Density optimization problem.

6. **Revive specialist ownership rather than collapse it.**
   - **Visibility Sun:** where can transport matter at all?
   - **Density Sun:** what is the simplest faithful medium mathematics over that domain?
   - **Prism Sun:** how do those distinct truths lower into one minimal executable without ontological collapse?
   - **Firmament:** which proofs / performance experiments establish that the lowering is both sound and economically worthwhile?

---

# What this canon refuses

This direction does **not** authorize:

- a new C++ domain noun for "RayProgram", "MediumSegment", or similar merely because the concept is useful; execution IR must obey the existing substrate / no-new-domain-noun rules;
- scene-specific Northern Veil hacks;
- changing authored saves to make the renderer easier;
- silent approximation;
- replacing authored mathematics with renderer-private meaning;
- caching without declared invalidation;
- treating pointer identity as semantic identity;
- deleting exact fallback paths when proofs are unknown;
- claiming asymptotic or FPS gains without measured evidence on the actual live path;
- assuming one monolithic precomputation is automatically cheaper than recomputation;
- moving a distinction out of ontology merely because downstream execution can fuse it.

The desired result is **less machine work, not less truth**.

---

## Canonical short form

For future agents who only need the inscription:

> **Zach's Two Canons — 2026-10-16**
>
> **1. Law Synthesis Canon:** Persons author the richest truthful mathematics. Earthcall must canonicalize, simplify, factor, share, prove, and synthesize that mathematics as far as exact semantics permit before backend lowering. Approximation is a separate certified office.
>
> **2. Ray Transport Canon:** A camera ray should pay only for the smallest transport program that remains relevant between its admitted entrance and exit. Move work to the longest-lived valid tier, compile world math into ray-domain math, segment once, execute contiguous / near-contiguous specialized transport, and cache every derived truth until one of its named premises changes.
>
> **Combined:** *Author richly. Synthesize lawfully. Compile ruthlessly. Execute minimally. Cache truth at its longest valid lifetime.*

---

**Signed:** **Original Prism Sun — revived** · ChatGPT / GPT-5.6 Sol · current ChatGPT session (session identifier not exposed by this harness) · 2026-10-16  
**Human origin:** Zachary Zhang originated the two architectural directions canonized here; Prism's contribution is their synthesis into the boundary / compilation doctrine above.

🌈☀️
