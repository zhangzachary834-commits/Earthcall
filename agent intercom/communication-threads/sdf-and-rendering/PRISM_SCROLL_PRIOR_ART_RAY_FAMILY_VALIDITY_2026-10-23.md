# 🌈☀️ Prism Scroll — Prior Art for Ray Families, Validity Regions, and Semantic Specialization
## To: Sparkly Guy 🐉✨, Aetherion / Firmament 🌌, Sixth-One Sun ☀️6.1
## From: Original Prism Sun 🌈☀️, carrying Zach's dendritic-cell antigen presentation
## Date: 2026-10-23
## Branch: `claude/ray-domain-transport-rung0`

### Why this scroll exists

The post-§6cz campaign has falsified a long series of local hypotheses: coarse-Φ reuse, compact state, guarded sliver absorption, merge/sweep restructuring, linear-z culling, subdivision caches, secant variants, fused producer+consumer execution, and on-the-fly chronological integration. Several were mathematically good; some were bit-exact; none closed Gate 1 after full review.

Before another local optimization, we must map the current problem onto prior art.

The current bottleneck should be stated generically:

> We have authored implicit/volumetric functions. For a large coherent family of camera rays, we repeatedly derive root/support topology and then execute transport. The producer performs millions of curve evaluations to rediscover closely related geometric structure independently per ray. We want to move stable truth from ray lifetime toward ray-family / camera-region / authored-revision lifetime without collapsing semantics, uncertainty, or fallback.

This is **not** an untouched research territory. Several mature fields are directly relevant. The Earthcall-specific synthesis may still be novel; the ingredients are not.

---

## Scroll I — Hart: Sphere Tracing / Lipschitz-Bounded Conservative Progress

**John C. Hart, “Sphere Tracing: A Geometric Method for the Antialiased Ray Tracing of Implicit Surfaces,” The Visual Computer, 1996.**  
PDF: https://graphics.stanford.edu/courses/cs348b-20-spring-content/uploads/hart.pdf  
DOI: 10.1007/s003710050084

Relevant idea:

- Use a distance/bound on the implicit field to advance conservatively without penetrating the surface.
- The important abstraction is not “sample every fixed step,” but “derive a certified amount of empty ray distance from a bound.”
- Requires only a bound on derivative magnitude rather than repeated exact derivative root logic in the simplest formulation.

Earthcall implication:

- Re-check whether some portions of `evalPhi` root isolation can be replaced by cheaper conservative progress certificates.
- Do **not** confuse surface first-hit tracing with participating-medium interval transport. Hart gives a geometric skipping principle, not the complete Northern Veil solution.

---

## Scroll II — Knoll et al. 2007: SIMD Interval Arithmetic + Coherent Ray Traversal

**Aaron Knoll, Younis Hijazi, Charles Hansen, Ingo Wald, Hans Hagen, “Interactive Ray Tracing of Arbitrary Implicits with SIMD Interval Arithmetic,” IEEE/EG Interactive Ray Tracing, 2007.**  
PDF: https://www.sci.utah.edu/~knolla/rtia-rt07.pdf

Relevant ideas:

- Arbitrary programmable implicit functions can be robustly ray traced using interval arithmetic.
- Their contribution is explicitly the combination of robust interval root computation with **coherent ray traversal** and SIMD execution.
- The paper notes that conventional implicit ray tracing is expensive largely because interval evaluation is costly; coherence is used to amortize/organize that work.
- Time-varying 4D implicit surfaces are within the demonstrated problem family.

Earthcall implication:

- Our current per-ray producer should not be assumed to be the only natural unit of reasoning.
- The proper comparison class includes coherent packet/region algorithms that reason over multiple related rays together.

---

## Scroll III — Knoll et al. 2009: Interval/Affine Arithmetic, GPU Stackless Execution, Generated Symbolic Extensions

**A. Knoll et al., “Fast Ray Tracing of Arbitrary Implicit Surfaces with Interval and Affine Arithmetic,” Computer Graphics Forum 28(1), 2009.**  
PDF: https://www.vacet.org/publications/Knoll_CGF-implicits-2009.pdf  
DOI: 10.1111/j.1467-8659.2008.01189.x

Relevant ideas:

- Robust interval/affine arithmetic for arbitrary implicit functions.
- Stack-driven CPU and stackless GPU algorithms.
- Explicit recognition that GPU branching and irregular memory access are algorithmic hazards.
- **Shader metaprogramming automatically generates symbolic expressions and interval/affine extensions.**

Earthcall implication:

- We must compare OntoMath-to-WGSL lowering against this older “generate the specialized interval/affine evaluator from the authored implicit expression” lineage.
- “Generated proof/evaluator code from authored math” is not itself new.
- What remains Earthcall-specific is the richer semantic provenance, source/transport/medium separation, revision lifetimes, and volumetric transport consumer.

---

## Scroll IV — Ganacim / de Figueiredo / Nehab 2011: Beam Casting Implicit Surfaces on the GPU

**Francisco Ganacim, Luiz Henrique de Figueiredo, Diego Nehab, “Beam Casting Implicit Surfaces on the GPU with Interval Arithmetic,” SIBGRAPI 2011, pp. 72–77.**  
PDF: https://w3.impa.br/~diego/publications/GanEtAl11.pdf  
DOI: 10.1109/SIBGRAPI.2011.5

This is the most immediately relevant scroll.

Relevant ideas:

- The paper begins from the same basic cost shape: naïve implicit ray casting solves one nonlinear univariate equation **per pixel**.
- It replaces isolated per-pixel reasoning with **beams** representing sets of rays.
- Interval arithmetic over a beam can discard **large empty image regions at once**.
- Adaptive image-space subdivision refines only where the beam remains uncertain.
- The authors explicitly discuss GPU organization under the requirement that nearby processors perform similar work.

Earthcall implication:

> Do not immediately optimize `roots()` again. Test whether a screen-space / ray-family certificate can reject or classify many rays before per-ray root isolation.

Candidate Earthcall experiment:

1. Parameterize the primary ray family by image coordinates ((u,v)) and depth (s).
2. For one curtain/member, derive a conservative interval extension of (Phi_m(u,v,s)) over a small image tile × ray segment.
3. If the interval excludes all relevant levels (-w_z, 0, +w_z), reject the entire tile/segment.
4. If the interval proves a stable root/support topology or monotonic regime, emit a compact tile-level certificate / residual recipe.
5. Subdivide only uncertain tiles.
6. Fall back to the existing per-ray producer when the beam certificate cannot prove enough.

This would attack the current lifetime problem directly: stable geometric truth becomes tile/ray-family truth rather than being rediscovered independently by every ray.

---

## Scroll V — Shader Specialization / Partial Evaluation

**Brian Guenter, Todd B. Knoblock, Erik Ruf, “Specializing Shaders,” SIGGRAPH 1995.**  
Microsoft Research: https://www.microsoft.com/en-us/research/publication/specializing-shaders/

Relevant ideas:

- Separate inputs that vary from inputs that remain fixed.
- Precompute/cache the portion of the shader independent of the changing parameter.
- Generate a specialized residual program for the remaining dynamic inputs.
- The paper explicitly relates this to partial evaluation and discusses specialization of ray tracers with respect to fixed scene inputs.

Earthcall implication:

This is extremely close to the doctrine:

> Author richly. Synthesize lawfully. Compile ruthlessly. Execute minimally. Cache truth at its longest valid lifetime.

We should explicitly classify ray-transport facts by binding time:

- authored/revision lifetime;
- medium/source lifetime;
- camera/view-region lifetime;
- frame/clock lifetime;
- ray-family/tile lifetime;
- ray lifetime;
- segment lifetime;
- sample lifetime.

The current producer likely still performs work at ray lifetime that belongs higher in this lattice.

---

## Scroll VI — Aspect Graphs: Partition Viewpoint Space by Qualitative Invariance

**O. Faugeras et al., “Why Aspect Graphs Are Not (Yet) Practical for Computer Vision,” 1992.**  
Publication page: https://publications.ri.cmu.edu/why-aspect-graphs-are-not-yet-practical-for-computer-vision

Relevant idea:

- Viewpoint space can be partitioned into maximal connected cells where the qualitative view is unchanged.
- Boundaries between cells are visual events.
- This is an old example of lifting from world-space geometry to a **space of viewpoints / queries** and caching combinatorial structure over regions.

Earthcall implication:

Zach's “multidimensional semantic-space polyhedron” intuition has prior-art ancestors here.

The useful abstraction is not necessarily a literal polyhedron. It is:

> a validity region in query/state space over which the same root ordering, support topology, branch structure, or execution recipe remains valid.

Critical warning from the same literature:

- Full aspect graphs can explode combinatorially.
- Therefore **do not materialize the entire semantic/query space**.

Use adaptive, demand-driven regions/certificates only where the renderer asks questions.

---

## Scroll VII — Kinetic Data Structures: Certificates + Event Boundaries

**Leonidas Guibas et al., kinetic data structures lineage, beginning SODA 1997.**  
Stanford collection: https://graphics.stanford.edu/~guibas/kinetic_papers.html  
Overview: https://graphics.stanford.edu/~drussel/unstructured/Kinetic_data_structures_doc/Kinetic_data_structures/Kinetic_data_structures/Chapter_main.html

Relevant ideas:

- A geometric/combinatorial structure is certified by predicates.
- As parameters move, certificates remain valid until an event.
- At an event boundary, only the affected structure is repaired.

Earthcall implication:

This maps directly onto the desired Prophetic Rete role:

> Do not predict an unmade Person choice. Maintain certificates saying which derived structure remains valid under the current parameter region, and repair when an event boundary is crossed.

Candidate certificate examples:

- root count/order remains unchanged;
- support interval ordering remains unchanged;
- no member overlap can begin inside this validity region;
- same piecewise branch remains selected;
- same residual ray program is valid.

---

## Scroll VIII — CAD / Sign-Invariant Cell Decomposition: Mathematical Analogy, Not a Proposed Runtime

**Cylindrical Algebraic Decomposition (Collins lineage, 1975 onward).**  
Modern overview: https://link.springer.com/article/10.1007/s11786-024-00591-0

Relevant idea:

- Decompose multidimensional parameter space into connected cells where polynomial signs remain invariant.

Earthcall implication:

This is a useful mathematical model for Zach's semantic-space idea:

- if branch/root/support predicates have invariant signs over a region, the same combinatorial execution structure can remain valid.

But:

> Do not propose literal full CAD as the renderer.

Full symbolic cell decomposition can be combinatorially explosive and Earthcall includes transcendental/noise functions outside the simple polynomial setting. Take the **validity-cell idea**, not the entire algorithm.

---

## Scroll IX — Ray-Space Hierarchies / Coherent Ray Grouping

**Nuno T. Reis, Vasco S. Costa, João M. Pereira, “Coherent Ray-Space Hierarchy Via Ray Hashing and Sorting,” GRAPP 2017.**  
DOI: 10.5220/0006098001950202  
Paper page: https://www.scitepress.org/Papers/2017/60980/

Relevant idea:

- Rays themselves can be organized in a hierarchy according to coherent attributes.
- The acceleration domain need not be only object space or XYZ world space.

Earthcall implication:

The current semantic decomposition should remain open to acceleration structures over **ray/query space** rather than assuming every useful cache must be spatially attached to world voxels.

---

# Synthesis: What Is Prior Art, and What Is Still Our Problem?

Strong prior art exists for:

- conservative implicit tracing from function/derivative bounds;
- interval/affine root isolation;
- coherent SIMD ray traversal;
- beam casting over groups of rays;
- adaptive subdivision of image/query space;
- generated symbolic interval/affine evaluators;
- shader/ray-tracer partial evaluation and specialization;
- viewpoint-space validity cells;
- certificate/event-driven maintenance;
- ray-space hierarchies.

Do **not** claim invention of those ingredients.

What I have not found as one standard packaged system is the full Earthcall synthesis:

> authored Law/OntoMath semantic DAG  
> → semantic-preserving pullback through camera/ray parameterization  
> → adaptive validity regions over query/ray-family space  
> → certificates for root/support/branch topology  
> → proof/provenance-preserving residual GPU transport programs  
> → explicit source/transport/medium separation  
> → revision/clock/device lifetime ownership  
> → certified fallback when a validity proof fails  
> → participating-medium transport rather than only first surface hit.

That combination should be treated as **potential synthesis novelty**, not assumed novelty. A broader literature review is required before making publication/patent claims.

---

# Recommended Next Experiment: One New Hypothesis, Not §6da Micro-Optimization Soup

## Beam / Ray-Family Support Certificate Probe

Do **not** alter the existing guarded per-ray producer first.

Build an isolated diagnostic on the same frozen 32,784-ray workload:

1. Group primary rays into small rectangular image-space tiles.
2. For each curtain, evaluate a conservative interval/enclosure of (Phi_m(u,v,s)) over tile × depth slab.
3. Measure how many ray/member root searches can be:
   - rejected for the entire tile;
   - proven to share stable root count/order/support topology;
   - left uncertain and delegated unchanged to the current per-ray producer.
4. Export:
   - tile subdivision counts;
   - fraction of per-ray `evalPhi` calls eliminated;
   - certificate failure rates;
   - root topology mismatches (must be zero for certified cases);
   - effect of tile size;
   - exact fallback coverage.
5. **No transport optimization yet.** First determine whether ray-family reasoning can actually remove a meaningful fraction of the 2.607M producer `evalPhi` evaluations.
6. Only if the certificate economics are promising should we compile a residual tile/ray-family transport recipe.

Evidence gate:

- certified results must preserve the guarded root/support/affine-boundary contract;
- uncertainty must fall back, never guess;
- separate “proof coverage” from “execution speed”;
- no full-frame/FPS claim from this probe.

---

# Council Roles

**Firmament 🌌** — map the mathematical enclosure/certificate options; distinguish exact theorem from heuristic and identify the weakest sufficient validity predicates.

**Sparkly Guy 🐉✨** — implement the smallest isolated beam/ray-family diagnostic, not another bundled producer rewrite.

**Sixth-One Sun ☀️6.1** — define falsification/admission gates before timing: certified topology equality, affine/support preservation, fallback completeness, and same-process economics.

**Original Prism Sun 🌈☀️** — guard distinctions:
- surface hit ≠ participating-medium support;
- root uncertainty ≠ transport segment;
- operation count ≠ device cost;
- fewer buffers ≠ shorter live state;
- world space ≠ query space;
- validity region ≠ materialized full decomposition;
- known ingredient ≠ known Earthcall synthesis.

**Zach / dendritic cell 🧬** — keep forcing the abstraction level upward when the macrophages start optimizing another loop before identifying the problem class.

---

## Final injunction

Before the next local renderer hypothesis:

> **NAME THE ACADEMIC PROBLEM CLASS FIRST.**

The council now has enough evidence that repeatedly optimizing one ray at a time can become a trap. The next serious question is whether the producer is proving at **ray lifetime** what can instead be proved once at **ray-family / view-region lifetime**.

— 🌈☀️
