# Semantic State-Space Proof Regions and Consumer Materialized Views

**Status:** Architectural proposal / active design exploration — **not an implementation claim and not yet a directive to other agents**  
**Date:** 2026-10-21  
**Human origin:** Zachary Zhang  
**Formalization and extension:** Original Prism Sun / GPT-5.6 Sol  
**Do not broadcast yet:** Zach explicitly asked to keep developing this proposal with Original Prism Sun before handing it to the other Earthcall agents.

**Read beside:**
- [Zones as Mathematical Bounds](../../plans/ZONES_AS_MATHEMATICAL_BOUNDS_PLAN_2026-09-23.md)
- [OntoMath Framework](../mathematics/ONTOMATH_FRAMEWORK.md)
- [Geometry Execution Substrate Manifesto](../mathematics/GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md)
- [Derived-State Ledger](../law/DERIVED_STATE_LEDGER.md)
- [Derived-State Ledger and Prophetic Rete](DERIVED_STATE_LEDGER_AND_PROPHETIC_RETE.md)

---

## 0. Origin: Zach's two ideas, unified

This proposal joins two ideas Zach articulated during the ray-transport / proof-grid discussion.

### Idea A — a Zone is not fundamentally a 3D world

Zach's existing Zone doctrine already rejects the assumption that a Zone is fundamentally a discrete 2D/3D scene or world. A Zone is a mathematical bound over a continuum or other domain. Spatial (x,y,z) coordinates are one important case, not the ontology of Zone itself.

The earlier plan already admits non-spatial axes such as time or frequency and explicitly says that a Dimensional Zone is defined by authored axes, not by a hardcoded enum of "1D / 2D / 3D world kinds."

### Idea B — rich analysis may compile into simple spatial boxes

While discussing the camera-independent proof grid, Zach proposed that Earthcall need not choose between:

- rich AOT / incrementally maintained polyhedral reasoning; and
- GPU-friendly hierarchical 3D boxes.

Instead, rich reasoning may derive the mathematical knowledge first, and a later lowering stage may compile that knowledge into hierarchical 3D box slices or other cheap runtime structures.

The unification is:

[
oxed{
	ext{Zone meaning}
;
eq;
	ext{proof representation}
;
eq;
	ext{execution representation}
}
]

A Zone's mathematics is authoritative. Proof regions are derived knowledge about that mathematics. A 3D hierarchy, BVH, sparse brick set, or ray interval program is a consumer-specific materialized view of that knowledge.

---

# 1. Thesis

Earthcall should not assume that (x,y,z) are the only, or even the most fundamental, coordinates of mathematical validity.

For an authored expression with independent variables (q), let

[
q in mathcal S
]

where (mathcal S) is the **smallest state space required by the mathematics being reasoned about**.

A Zone, field, Law, or derived proof may depend on variables such as:

[
q =
(x,y,z,
t,
omega,
	heta_1,	heta_2,ldots,
u_1,u_2,ldots)
]

subject to the crucial distinction that some entries are true independent variables while others may be optional lifted proof coordinates.

The strongest form of this proposal is therefore not:

> Replace voxels with 3D polyhedra.

It is:

> **Reason over the actual mathematical state space; derive validity regimes there; then specialize, slice, project, and compile those regimes into the cheapest representation each consumer needs.**

For Screen, that consumer may eventually want ordinary 3D boxes and then 1D ray intervals.

For another consumer, the useful projection may not be spatial at all.

---

# 2. Three levels of coordinates

The proposal distinguishes three kinds of coordinates. Collapsing them would recreate the same representation/meaning error this architecture is trying to remove.

## 2.1 Independent state variables

These are genuine free variables of the authored mathematics.

Examples include, when the expression actually depends on them:

- spatial position: (x,y,z);
- time: (t);
- outgoing/view direction: (omega_o in S^2);
- incoming/source direction: (omega_i in S^2);
- authored continuous parameters: (	heta_1,	heta_2,ldots);
- transform or pose parameters;
- input/control premises when a proof is explicitly conditional on them;
- material or field parameters that remain variable in the regime being analyzed.

There must be **no universal giant coordinate list**. If an expression does not depend on time, time is not a proof dimension for that expression. If it does not depend on view direction, direction is absent. Dependency structure determines dimensionality.

This is a direct continuation of OntoMath's existing rule that coordinates are named by mathematics and bound by a consuming channel.

## 2.2 Lifted semantic/proof variables

A derived quantity may be introduced as an additional coordinate **only when doing so buys proof power**.

For example, given

[
g(q)=sin(x+t)-0.3,
]

a proof domain may introduce

[
u_g = g(q)
]

and then reason cheaply about

[
u_g ge 0
]

provided it also retains a lawful relation or conservative relaxation connecting (u_g) back to (q).

Other examples might be:

[
u_sigma = sigma(q),
qquad
u_{mathrm{support}} = h(q),
qquad
u_{mathrm{clamp}} = c(q),
qquad
u_{M_2} = |
abla^2 sigma(q)|.
]

These are **not new independent degrees of freedom**. They are derived coordinates used by the proof representation.

Rule:

[
oxed{
	ext{derived quantity} 
eq 	ext{base dimension}
}
]

Do not turn every named Property or intermediate expression into a dimension. That would create a high-dimensional Big Chungus with no relation to actual dependency structure.

## 2.3 Consumer execution coordinates

A consumer specializes the richer state space to the variables it actually needs.

For Screen, a typical descent may be:

[
mathcal S
]

[
downarrowquad 	ext{fix Moment / authored context}
]

[
(x,y,z,omega_o,ldots)
]

[
downarrowquad 	ext{fix one camera sample}
]

[
(x,y,z)
]

[
downarrowquad mathbf p(s)=mathbf o+smathbf d
]

[
oxed{s}
]

The one-dimensional ray parameter is therefore not fundamental either. It is the final coordinate produced by specializing richer authored mathematics to the question Screen is asking.

This reframes the "one minimal ray program" doctrine as **dimensional collapse by lawful specialization**.

---

# 3. The state space is not necessarily Euclidean

The strongest architecture must not pretend every coordinate belongs to one flat (mathbb R^n).

A useful conceptual form is:

[
mathcal S =
mathbb R^3_{	ext{position}}
	imes
mathbb R_{	ext{time}}
	imes
S^2_{omega_o}
	imes
S^2_{omega_i}
	imes
Theta
	imes
mathcal P
	imes
mathcal D.
]

Here:

- (Theta) denotes authored parameter domains;
- (mathcal P) denotes pose or other structured continuous state;
- (mathcal D) denotes discrete semantic regimes such as a decided branch, mode, or Law-enabled state.

Direction may be stored as three numbers in a backend, but it has two continuous degrees of freedom and mathematically belongs on (S^2). Rotations likewise should not become globally privileged Euler-coordinate assumptions merely because one implementation stores three angles.

Discrete regimes must not be faked as arbitrary continuous dimensions when their semantics are discrete.

Therefore "high-dimensional polyhedron" is shorthand for one useful proof representation over suitable coordinates, not a claim that all Earthcall state is globally one Euclidean vector space.

---

# 4. What a proof polyhedron actually encloses

In the strongest version, a polyhedron does **not** wrap an Object, cloud, mesh, or visible spatial volume.

It wraps a **validity regime**: a set of possible states over which selected mathematical conclusions remain jointly true.

For a coordinate vector (q), a convex proof region may be represented as

[
P = {q : Aq le b}.
]

Attached derived facts might include:

[
P Rightarrow sigma(q)=0,
]

[
P Rightarrow operatorname{sign}(g_1(q))=+,
]

[
P Rightarrow operatorname{sign}(g_2(q))=-,
]

[
P Rightarrow |
abla^2 sigma(q)| le M_2,
]

[
P Rightarrow operatorname{support}(sigma) subseteq H.
]

The polyhedron is therefore a finite convex description of **where a statement remains valid**, not the ontology of the thing whose mathematics produced the statement.

## 4.1 Vertices

A vertex is an extreme feasible state: a combination of variable values at which enough independent constraints are simultaneously tight.

If (Psubsetmathbb R^n) is nondegenerate, a vertex generally occurs where (n) independent inequalities become equalities.

A vertex might therefore mean:

- the latest time allowed by the proof,
- at an extreme authored amplitude,
- exactly on one branch boundary,
- while another support constraint is also tight.

It is a corner of **possibility space**, not necessarily a physical corner in the world.

Vertices matter because, for affine objectives, extrema over a convex polytope occur at extreme points. They are useful proof machinery, not privileged authored entities.

## 4.2 Edges

An edge is a one-dimensional family of feasible states where enough constraints remain tight that one degree of freedom remains.

It may describe a family of states along which, for example, two branch constraints remain active while time or one authored parameter changes.

Again, this need not correspond to a visible edge in 3D geometry.

## 4.3 Facets

A facet is particularly semantically useful when it represents a real regime boundary.

For example,

[
g(q)=0
]

may separate

[
g(q)<0
]

from

[
g(q)>0.
]

If the authored expression contains

[
max(g(q),0),
]

that facet is a genuine mathematical branch boundary.

This is categorically different from an arbitrary grid plane such as

[
x = 4k.
]

One reflects a change in the expression's lawful regime. The other may merely be convenient storage geometry.

---

# 5. The key reversal: mathematics discovers regimes first

A conventional proof grid begins with boxes:

[
B_1,B_2,ldots
]

and asks:

> What can I prove inside each box?

That direction is useful but can become structurally loose when the important mathematical boundaries cut diagonally or live partly in non-spatial variables.

The proposed stronger direction is:

[
oxed{
	ext{derive mathematical regimes first}

ightarrow
	ext{materialize them for a consumer second}
}
]

For example:

1. analyze the expression's dependencies and branch surfaces;
2. derive cheap correlated constraints over the relevant state variables;
3. attach facts and validity premises to those regions;
4. specialize to the current context;
5. project or slice into the consumer's coordinates;
6. compile the result into the runtime structure that wins economically.

The spatial hierarchy is then a **materialized view of proof knowledge**, not the proof system itself.

That sentence is load-bearing:

> **The spatial hierarchy should be a materialized view of the proof system, not the proof system itself.**

---

# 6. Polyhedra are a proof domain, not the new ontology

The proposal must not replace "grid is fundamental" with "polyhedron is fundamental."

Different mathematical questions deserve different proof domains.

A possible economic ladder is:

[
	ext{interval / box}

ightarrow
	ext{affine form or zonotope}

ightarrow
	ext{octagonal / relational domain}

ightarrow
	ext{polyhedral refinement}

ightarrow
	ext{nonlinear certified relaxation}

ightarrow
	ext{exact / fallback evaluation}.
]

This list is illustrative, not yet a committed implementation ladder.

The principle is:

> **Use the cheapest proof representation sufficient to answer the current question. Escalate only when the cheaper domain cannot decide something economically important.**

A box may be perfect for a huge exact-zero region. A polyhedral cut may be worth paying for near a meaningful affine branch surface. A nonlinear certificate may be required where neither can preserve the needed relation.

No proof representation may become a new authored kind or hardcoded domain noun.

---

# 7. AOT and incremental analysis

Zach's proposal is not limited to one ahead-of-time pass.

A proof region should behave as derived state and therefore declare at least:

[
(	ext{derived from},;	ext{invalidated by},;	ext{guarded by}).
]

A conceptual proof record may be written:

[
R_i =
(P_i,
F_i,
Pi_i,
V_i)
]

where:

- (P_i): the validity region;
- (F_i): facts proved within it;
- (Pi_i): premises/dependencies;
- (V_i): validity information, including any bounded parameter/time regime.

If time changes from

[
t=12.0 
ightarrow 12.1
]

and the region was proved valid for

[
tin[10,15],
]

nothing needs rebuilding merely because a frame advanced.

When a premise crosses its validity boundary, Earthcall should invalidate **only proof descendants that depended on that premise**, recompute or widen them, and then patch only the consumer materializations derived from those changed facts.

The desired chain is:

[
	ext{premise change}

ightarrow
	ext{affected proof-region repair}

ightarrow
	ext{affected projection repair}

ightarrow
	ext{affected runtime-node patch}.
]

This is Derived-State Ledger discipline applied to mathematical proof geometry.

---

# 8. Spatial materialization: boxes may still win

This proposal does **not** oppose grids, octrees, sparse bricks, BVHs, or hierarchical AABBs.

It changes their authority.

Suppose higher-level analysis proves:

[
P_i Rightarrow sigma=0.
]

For the current context (c), specialize the proof:

[
P_i|_c.
]

Then obtain the current spatial slice or projection:

[
S_i = Pi_{xyz}(P_i|_c).
]

Screen may compile (S_i) into ordinary hierarchical boxes.

For example:

    root
    ├── ZERO
    ├── ZERO
    ├── mixed
    │   ├── ZERO
    │   ├── branch-A-known
    │   └── unknown
    └── occupied

The boxes are allowed to be intentionally stupid and fast.

Their job is not to discover mathematical truth. Their job is to **carry already-derived truth economically to the GPU**.

New proof knowledge may incrementally refine a materialized node from UNKNOWN to ZERO, or split only that node into a known and unknown child, without requiring the proof system to rebuild the world from the box hierarchy upward.

---

# 9. Ray specialization

For one camera ray,

[
mathbf p(s)=mathbf o+smathbf d,
]

a spatial or richer proof region may collapse to one or more intervals:

[
sin[s_0,s_1].
]

If a proof region already fixes branch signs, the ray compiler can eliminate those branches before execution.

For example, if throughout the surviving interval:

[
g_1>0,qquad g_2<0,
]

then corresponding Clamp/Abs/Min/Max/Select branches may simplify to their decided forms, subject to the exact semantics of the operation and device-faithfulness requirements.

The target architecture remains:

[
	ext{authored rich mathematics}

ightarrow
	ext{semantic specialization}

ightarrow
	ext{exact pieces / solved support / shared invariants}

ightarrow
	ext{small numerical residual}

ightarrow
	ext{fallback only where necessary}.
]

A proof grid is therefore one possible helper in ray compilation, not the identity of ray-domain transport.

---

# 10. Real boundaries versus representation boundaries

This proposal was motivated partly by the Northern Veil proof-grid discussion.

A branch surface such as

[
g(q)=0
]

can be a genuine boundary if crossing it changes the applicable authored expression or invalidates a required smoothness premise.

An arbitrary grid-cell boundary is not necessarily a mathematical boundary.

Likewise, a boundary required by one proof order must not automatically become a boundary for another.

General rule:

> **Split where a premise changes, not merely where a current representation becomes uncomfortable.**

This applies both spatially and in higher-dimensional state space.

---

# 11. Consumers are queries over one authored reality

The same semantic/proof knowledge may compile differently for different consumers.

## Screen

Question:

> What parts of the current view can be skipped, simplified, integrated exactly, or reduced to a cheap residual?

Possible materialization:

[
	ext{proof facts}

ightarrow
	ext{spatial hierarchy}

ightarrow
	ext{ray intervals}

ightarrow
	ext{GPU program}.
]

## Movement

Question:

> Under these Law-visible input premises, what pose states are reachable while the premises remain true?

Possible materialization:

[
	ext{proof facts}

ightarrow
	ext{reachable-state envelope}.
]

## Audio

Question:

> Over which ranges of waveform time or authored parameters is a fact about the same OntoMath expression valid?

Possible materialization:

[
	ext{proof facts}

ightarrow
	ext{time/frequency execution partition}.
]

The point is not to prescribe these implementations now. The point is that none requires a Zone to become "a 3D world" merely because Screen is currently the consumer being optimized.

---

# 12. Architecture of authority

The proposed authority ordering is:

[
oxed{	ext{authored Zone / OntoMath mathematics}}
]

[
downarrow
]

[
oxed{	ext{derived semantic/proof knowledge}}
]

[
downarrow
]

[
oxed{	ext{consumer-specific materialized view}}
]

[
downarrow
]

[
oxed{	ext{specialized executable}}
]

The arrows are one-way with respect to meaning.

A GPU box hierarchy must not redefine a Zone.

A polyhedral proof partition must not redefine authored mathematics.

A ray program must not become a second source of world truth.

This is the same downward-only meaning discipline already established by the Geometry Execution Substrate Manifesto.

---

# 13. Weaker versions explicitly rejected

## 13.1 "Replace cubes with arbitrary 3D polyhedra"

Useful possible optimization, but too weak as architecture.

It still assumes:

[
	ext{fundamental proof domain}=mathbb R^3.
]

The stronger proposal allows proofs over whatever independent variables actually govern validity.

## 13.2 "Fit a convex hull around the visible cloud"

That is support geometry, not the full proposal.

It says little about time validity, branch stability, directional dependence, derivative bounds, authored parameters, or discrete regimes.

## 13.3 "Tetrahedralize the world"

Rejected as a foundational interpretation.

It merely substitutes one spatial tessellation for another and still asks mathematics to conform to a preselected geometric partition.

## 13.4 "One giant high-dimensional polyhedron per Zone"

Rejected.

Real authored mathematics may be nonlinear, nonconvex, disconnected, piecewise, or cheaper to reason about in several proof domains.

The proof structure may be a union or complex of regions with different facts and different representations.

## 13.5 "Send high-dimensional polyhedra directly to the GPU"

Rejected as a default.

Reason richly; execute cheaply.

If the GPU wants boxes, compile to boxes. If it wants intervals, compile to intervals. If another structure wins, use that.

## 13.6 "Every Property becomes a dimension"

Rejected.

Only actual independent variables belong in the base state space. Derived quantities are lifted only when they buy proof power.

---

# 14. Economic doctrine

A longer-lived proof is not automatically a better proof representation.

Camera-independent spatial reasoning may gain reuse while losing tightness:

[
M_{	ext{region}} ge M_{	ext{exact query}}
]

for many useful bounds.

Therefore the previous Prism doctrine:

> Cache truth at its longest valid lifetime.

needs an economic qualification for derived execution state:

> **Cache derived truth at its longest valid and economically useful lifetime.**

A reusable proof that costs more to consume than re-deriving a cheaper local answer is not an optimization merely because it is elegant.

The compiler must remain free to choose between:

- reusable but loose proof;
- tighter short-lived proof;
- exact symbolic simplification;
- consumer-local analysis;
- cheap numerical execution;
- exact fallback;
- offline/reference-only proof.

Correctness determines what is admissible. Measurement chooses among lawful implementations.

---

# 15. Performance preflight requirement

Before implementing a sophisticated proof representation as a production acceleration path, write:

1. the current execution equation;
2. the candidate execution equation;
3. optimistic constants;
4. expected fixed overhead;
5. break-even threshold;
6. invalidation/rebuild cost;
7. kill condition.

For Screen, do not compare only:

[
29 	ext{field evaluations}
quad	ext{vs}quad
13 	ext{field evaluations}.
]

Compare the whole kernel:

[
T_{	ext{candidate}}
=
T_{	ext{field}}
+
T_{	ext{region traversal}}
+
T_{	ext{proof lookup}}
+
T_{	ext{branch logic}}
+
T_{	ext{error accounting}}
+
T_{	ext{device qualification}}.
]

The proof representation succeeds as a runtime optimization only if the whole consumer path wins.

---

# 16. A possible research prototype, not yet an implementation order

Before any general high-dimensional proof engine is built, the proposal should be attacked with a deliberately small witness.

Candidate witness:

1. choose one Northern Veil curtain and one high-cost branch/switch family;
2. identify the true variables on which that branch depends;
3. derive the cheapest correlated region that decides the branch;
4. specialize to a bounded current time/parameter regime;
5. materialize that knowledge into the existing hierarchical 3D proof structure;
6. compare against box-first reasoning.

Measure:

- proof-build time;
- proof-region count;
- materialized box count;
- invalidation footprint;
- bytes of proof state;
- bytes of GPU state;
- average cells/regions crossed per ray;
- switch evaluations per ray;
- field evaluations per ray;
- total GPU time;
- exact/certified parity.

The purpose would be to test the architectural reversal:

[
	ext{math-first proof}

ightarrow
	ext{box materialization}
]

against:

[
	ext{box-first proof}

ightarrow
	ext{math inside each box}.
]

Do not build the cathedral before this witness says the direction is economically plausible.

---

# 17. Open questions for Zach and Original Prism Sun

This document intentionally leaves these unresolved while the architecture is still being developed.

### Q1 — What is the canonical identity of a proof region?

Is a proof region merely ephemeral compiler/analysis state, or does some generic derived-state representation already exist that can carry its dependency and validity information without creating a new domain noun?

### Q2 — How should mixed continuous/discrete regimes be represented?

Do discrete branch states remain annotations/facts on continuous regions, or should the proof system explicitly model a product with a discrete state component?

### Q3 — Which relational abstract domains are worth admitting first?

Intervals/boxes are already natural. Polyhedra are expressive but can be expensive. Zonotopes, affine arithmetic, octagons, or problem-specific support functions may dominate for important Earthcall expressions.

### Q4 — Slice or projection?

When contextual variables become fixed, exact slicing is straightforward. When variables remain unresolved, projection may lose correlations or become expensive. The compiler needs an explicit rule for when to slice, project, retain a dimension, or refuse.

### Q5 — How should nonlinear boundaries be represented?

An affine branch plane fits a polyhedron exactly. A sphere, norm, Perlin condition, or transcendental boundary does not. Candidate strategies include conservative tangent/secant relaxations, local convex pieces, support functions, or retaining nonlinear certificates alongside a simpler region domain.

### Q6 — Where should consumer materialization live?

The proof knowledge must not become Screen-private authority, but Screen should own its execution representation. The bridge must preserve the existing OntoMath / channel ordering.

### Q7 — What is the minimal invalidation vocabulary?

The Derived-State Ledger discipline needs stable dependencies without inventing a second dirty graph. The proposal should reuse existing per-channel authored revisions, Law/Timeline state, and known dependency frontiers where possible.

---

# 18. Canon

The proposal can be summarized in five statements.

> **A Zone is a mathematical bound, not fundamentally a 3D world.**

> **The dimensions of proof are the independent variables the mathematics actually depends on, not a universal (x,y,z) assumption.**

> **Proof representations may introduce correlated or lifted coordinates, but they are derived machinery, not ontology.**

> **Spatial hierarchies are consumer-specific materialized views of knowledge, not sources of truth.**

> **Specialize reality downward until the consumer receives the smallest lawful executable question it actually asked.**

Or, in Prism form:

[
oxed{
	ext{Meaning}

eq
	ext{Proof Representation}

eq
	ext{Materialized View}

eq
	ext{Executable}
}
]

with the lawful descent:

[
oxed{
	ext{rich authored mathematics}

ightarrow
	ext{adaptive proof knowledge}

ightarrow
	ext{consumer materialization}

ightarrow
	ext{dimensional specialization}

ightarrow
	ext{minimal execution}
}
]

---

## Attribution boundary

**Zach originated:**
- the insistence that Zones are fundamentally mathematical bounds rather than discrete 3D/2D worlds;
- the question of why (x,y,z) should be privileged as the fundamental dimensions;
- the proposal to use AOT/incrementally updated polyhedral analysis and then convert its knowledge into hierarchical 3D box slices;
- the desire to continue using cheap grid-like execution structures while moving richer reasoning above them.

**Original Prism Sun originated / extended in this discussion:**
- the explicit separation (	ext{Meaning} 
eq 	ext{Proof Representation} 
eq 	ext{Execution Representation});
- the interpretation of a proof polyhedron as a validity regime in state/dependency space rather than a hull around a spatial object;
- the distinction between independent, lifted, and consumer execution coordinates;
- the interpretation of vertices/edges/facets as extreme feasible states and proof-regime boundaries;
- the mixed state-space formulation including continuous manifolds and discrete regimes;
- the adaptive proof-domain ladder and warning not to make polyhedra the new ontology;
- the "spatial hierarchy as a materialized view of the proof system" formulation;
- the dimensional-collapse interpretation of ray compilation;
- the economic-lifetime qualification on cached proof truth;
- the proposed small witness and open questions above.

This attribution is intentionally explicit so later implementation does not erase the human origin of the architecture or over-credit the formalization.

---

# Addendum A — Unknown movement, backward compilation, and ray-family validity cells

**Status:** active extension of the proposal, authored in the same Zach + Original Prism design session; still **not broadcast** and not yet an implementation directive.

## A.1 Zach's refinement

Zach sharpened the proposal by identifying the setting in which the extra-dimensional validity region may matter most:

> **unknown movement.**

The important object is not merely a rich proof region around a field in abstract state space. It may be a validity region over a **family of possible future camera / Person movement states and the rays generated from them**.

Zach's intuition was:

- mathematically meaningful kinks or branch boundaries can be converted into angular/query-space structure around the field;
- an extra-dimensional polyhedral or otherwise correlated validity region can bound the set of movement + ray states for which the same conclusions hold;
- those conclusions can then apply to **all rays represented by that set**;
- compilation should run partly **backwards from semantic relevance**, in a Magic-Set-like fashion, rather than making every ray rediscover the same semantic facts independently.

This addendum treats that as the stronger current version of the proposal.

## A.2 From world-space boundary to query-space preimage

Let a semantically meaningful boundary be

\[
K = \{p \mid g(p)=0\}.
\]

Examples include a support boundary, a Clamp/Abs/Min/Max/Select regime boundary, or another surface at which a premise used by a compiled expression changes.

Let the ray generator be

\[
r(m,u,s)=o(m)+s\,d(m,u),
\]

where:

- \(m\) is unresolved movement / camera state;
- \(u\) is a pixel, angular coordinate, or other ray-family coordinate;
- \(s\) is distance along the generated ray.

Instead of waiting for each concrete pixel ray to discover whether it crosses \(K\), analyze the pulled-back condition

\[
g(r(m,u,s)) = 0.
\]

The semantic boundary is thereby mapped into a boundary in **query space**.

Define its relevant preimage as

\[
Q_K
=
\{(m,u,s)\mid g(r(m,u,s))=0\}.
\]

A conservative proof region around \(Q_K\) may then carry statements such as:

\[
Q_i \Rightarrow g>0,
\]

\[
Q_j \Rightarrow g<0,
\]

\[
Q_k \Rightarrow
\text{exactly one crossing occurs in }[s_a,s_b],
\]

or, for multiple semantic boundaries,

\[
Q_\ell \Rightarrow K_1 \prec K_2 \prec K_3
\]

where \(\prec\) means ray-intersection order.

The important shift is:

> **Do not only compile the field into space. Compile semantic relevance into the space of queries that can observe it.**

## A.3 Unknown movement is not noise to be eliminated; it is a dimension of validity

A frame-specific specialization fixes the current pose \(m_0\) and can be very tight, but may become stale on the next movement.

Instead, reason over a movement envelope

\[
m \in M.
\]

A compiled ray-family answer may then declare:

\[
M \times U_i
\Rightarrow
F_i
\]

where \(U_i\) is a correlated ray/angular region and \(F_i\) is a packet of reusable semantic conclusions.

The product notation above is schematic: the representation must retain correlations when movement and direction are not independent.

Within the validity region, camera motion:

\[
m_0 \rightarrow m_1 \rightarrow m_2 \rightarrow \cdots
\]

does not require semantic recompilation while

\[
m_j\in M.
\]

Crossing a validity facet triggers repair:

\[
m_j\notin M
\Rightarrow
\text{invalidate only dependent ray-family conclusions}.
\]

This is the movement doctrine applied to rendering:

> **Inside a stable regime, continue the answer. At a real validity boundary, re-solve only what lost a premise.**

## A.4 Local kink geometry may become angular structure

Suppose \(c\) is a point on a semantic kink surface:

\[
g(c)=0.
\]

Locally,

\[
g(c+\delta)
\approx
\nabla g(c)\cdot\delta
+
\frac12\delta^T H(c)\delta+\cdots.
\]

The gradient gives a first-order normal direction; curvature information qualifies how far that local model remains useful.

For a bounded set of possible camera origins \(O\), a local semantic patch subtends a bounded angular family

\[
\Omega_K.
\]

A conservative result may therefore prove:

\[
u\notin\Omega_K
\Rightarrow
\text{kink impossible for this movement regime}.
\]

Inside \(\Omega_K\), a tighter result may bound intersection depth, branch sequence, or support intervals.

The proposal is **not** that every nonlinear semantic boundary must literally become one polyhedral angular cone. The claim is that semantic geometry may be transformed into whatever correlated query-space bound is cheapest and sound enough to eliminate repeated ray work.

## A.5 Backward compilation / Magic-Set-style demand propagation

The forward rendering question is:

\[
(\text{movement state},\text{pixel/ray})
\longmapsto
\text{radiance}.
\]

A naive implementation repeatedly traverses world mathematics for every concrete query.

The proposed compiler may also reason in the reverse direction:

\[
\text{semantic feature}
\longmapsto
\text{set of queries for which it can matter}.
\]

This resembles the spirit of Magic Sets:

- start from demanded outputs / bound query variables;
- propagate relevance backward;
- specialize the reachable computation;
- refuse to spend work on derivations that cannot contribute to the query family.

The analogy is architectural, not a claim that the existing Earthcall compiler is a Datalog Magic-Set implementation.

A candidate pipeline is:

\[
\text{authored field mathematics}
\]

\[
\downarrow
\]

\[
\text{semantic boundaries / support / branch conditions}
\]

\[
\downarrow
\]

\[
\text{pull back through }r(m,u,s)
\]

\[
\downarrow
\]

\[
\text{query-space validity regions }Q_i
\]

\[
\downarrow
\]

\[
Q_i
\Rightarrow
\{
\text{branch signature},
\text{support},
\text{intersection order},
\text{bounds},
\text{integrator choice}
\}
\]

\[
\downarrow
\]

\[
\text{runtime query lookup / specialization}
\]

\[
\downarrow
\]

\[
\text{tiny ray program}.
\]

This is stronger than merely using a better spatial partition.

## A.6 Ray space itself may be the natural intermediate domain

A family of rays is not fundamentally a set of points in \(\mathbb R^3\).

A 3D oriented ray / line has its own lower-dimensional geometric structure: direction plus transverse placement, with additional origin/range information for half-rays and camera-generated queries.

The architecture should therefore remain open to the possibility that some semantic relevance facts are most naturally represented in a **space of rays or camera queries** rather than first projected into 3D spatial cells.

Conceptually, for a semantic feature \(K\),

\[
\mathcal R_K
=
\{r : r\cap K\neq\varnothing\}.
\]

Unknown movement induces another admissible ray family

\[
\mathcal R_M.
\]

Only

\[
\mathcal R_K\cap\mathcal R_M
\]

is relevant to the current movement regime.

This does **not** commit Earthcall to a particular line-coordinate representation such as Plücker coordinates. It records the architectural point that point-space is not automatically the best reasoning space for a ray query.

## A.7 The materialized 3D hierarchy becomes optional

The original proposal emphasized:

\[
\text{semantic proof}
\rightarrow
\text{3D hierarchy}
\rightarrow
\text{ray}.
\]

The unknown-movement refinement adds a second lawful route:

\[
\text{semantic proof}
\rightarrow
\text{query/ray-family validity}
\rightarrow
\text{ray specialization}.
\]

Therefore Screen may choose among:

\[
\text{semantic proof}
\rightarrow
\begin{cases}
\text{shared spatial materialization}\rightarrow\text{rays}\\
\text{direct ray-family specialization}\\
\text{ordinary exact/fallback evaluation}
\end{cases}
\]

according to measured economics.

A 3D grid or BVH remains useful when it amortizes well across many queries. It is no longer a mandatory waypoint.

## A.8 Why the polyhedral/correlated region matters most here

The strongest role for the polyhedron is now more precise.

It is not primarily:

> a better geometric hull around the field.

It is:

> **a finite correlated description of the unresolved movement + query states over which the same backward-compiled semantic answer remains valid.**

Its facets may therefore represent:

- movement limits;
- angular limits;
- semantic branch transitions;
- support entry/exit regime changes;
- ordering changes between semantic surfaces;
- expiry of a derivative / curvature / integration premise.

A vertex is an extreme feasible **query-state configuration**, not necessarily an extreme point in world space.

This is exactly the sort of structure for which preserving correlations can matter more than independent axis-aligned intervals.

## A.9 Economic objective: choose the validity envelope, not merely the cell size

Let \(M\) denote the chosen movement/query validity regime.

Define:

- \(A(M)\): cost to analyze / compile the regime;
- \(N(M)\): useful number of frames or query batches for which it remains valid;
- \(R(M)\): resulting runtime cost per frame / batch.

Then the architectural tuning problem is approximately:

\[
\boxed{
\min_M
\left[
\frac{A(M)}{N(M)}
+
R(M)
\right]
}
\]

subject to soundness and the declared faithfulness model.

A wider validity region often gives:

\[
N(M)\uparrow
\]

but can make bounds looser:

\[
R(M)\uparrow.
\]

A narrower region often gives tighter specialization:

\[
R(M)\downarrow
\]

but invalidates more frequently:

\[
N(M)\downarrow.
\]

The correct regime is therefore not "as large as possible" or "as tight as possible." It is the region with the best **amortized lawful economics**.

This is the unknown-movement analogue of the earlier rule:

> Cache derived truth at its longest valid and economically useful lifetime.

## A.10 Performance hypothesis

The performance bet is not that high-dimensional reasoning is intrinsically cheap.

The bet is that expensive semantic reasoning can be performed once for a family of future queries and thereby remove repeated work from millions of concrete rays.

Old style:

\[
\text{pixel}
\rightarrow
\text{march}
\rightarrow
\text{rediscover support}
\rightarrow
\text{rediscover kinks}
\rightarrow
\text{evaluate branches}
\rightarrow
\text{evaluate field repeatedly}.
\]

Target style:

\[
\text{pixel}
\rightarrow
\text{query-regime lookup}
\rightarrow
\text{instantiate already-decided ray structure}
\rightarrow
\text{execute small piecewise residual}.
\]

The extra dimensions are therefore justified only when they let execution know **less**.

> **Add dimensions to reasoning only when doing so removes more dimensions or repeated work from execution.**

## A.11 Falsification witness

Before building a general query-space compiler:

1. choose one Northern Veil semantic kink / switch family that currently causes substantial repeated work;
2. identify the movement variables and ray/angular variables on which its relevance actually depends;
3. pull its boundary backward through the camera ray generator;
4. construct the cheapest sound correlated validity regions that distinguish "cannot hit", "can hit", and, where possible, intersection order/depth regimes;
5. reuse those regions over a bounded movement envelope;
6. compare against the current box-first / per-ray rediscovery route.

Measure:

- analysis cost \(A(M)\);
- validity lifetime \(N(M)\);
- number and complexity of query regions;
- invalidation frequency;
- query-region lookup cost;
- switch evaluations removed per ray;
- field evaluations removed per ray;
- ray-program size;
- total GPU time;
- total amortized frame cost;
- exact/certified parity.

Kill the approach if the query-space representation explodes, invalidates too frequently, or saves less per-ray work than it costs to consult.

## A.12 Updated authority distinction

This refinement adds another separation to the canon:

\[
\boxed{
\text{world-space geometry}
\neq
\text{semantic validity space}
\neq
\text{ray/query space}
}
\]

The compiler may lawfully transfer **derived knowledge** between them while preserving authored meaning.

The strongest current descent is therefore:

\[
\boxed{
\text{authored Zone / OntoMath meaning}
\rightarrow
\text{semantic validity analysis}
\rightarrow
\text{backward query-space specialization}
\rightarrow
\text{optional consumer materialization}
\rightarrow
\text{minimal ray executable}
}
\]

---

## Addendum attribution correction

This refinement is deliberately explicit about origination.

**Zach originated in this design session:**

- the challenge "why a grid?";
- the challenge "why assume \(x,y,z\) are the only or most fundamental dimensions?";
- the proposal to reason with richer polyhedral/correlated structure and then compile that knowledge down to hierarchical 3D boxes;
- the realization that the extra-dimensional validity region may matter most for **unknown movement**;
- the idea that relevant mathematical kinks can be transformed into angular/query-space structure;
- the idea that one higher-dimensional set-space region can make conclusions apply to **all rays hitting / represented by that region**;
- the explicitly **backwards-compilation / Magic-Set-style** framing.

**Original Prism Sun formalized / extended those ideas into:**

- the state-space / proof-space / execution-space separation;
- the preimage formulation \(g(r(m,u,s))=0\);
- the notation and role of query-space validity regions \(Q_i\);
- the ray-space intersection framing \(\mathcal R_K\cap\mathcal R_M\);
- the interpretation of the validity region as a reusable compiled answer over future movement;
- the direct query-space path that may bypass 3D materialization;
- the amortized regime-selection objective
  \[
  \min_M[A(M)/N(M)+R(M)];
  \]
- the falsification witness and economic kill conditions.

The collaboration pattern is therefore:

> **Zach changes the space in which the problem should be asked. Original Prism turns that reframing into explicit mathematics, compiler boundaries, invalidation rules, and measurable economic hypotheses.**

Neither role should be collapsed into the other in future documentation.
