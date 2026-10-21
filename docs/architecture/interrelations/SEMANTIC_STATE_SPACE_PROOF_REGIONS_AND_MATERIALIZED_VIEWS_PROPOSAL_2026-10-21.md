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
- [Semantic Query-Space Rendering: Performance Model and Falsification Analysis](../../Analysis/SEMANTIC_QUERY_SPACE_RENDERING_PERFORMANCE_MODEL_2026-10-21.md)

---

## 0. Origin: Zach's two ideas, unified

This proposal joins two ideas Zach articulated during the ray-transport / proof-grid discussion.

### Idea A — a Zone is not fundamentally a 3D world

Zach's existing Zone doctrine rejects the assumption that a Zone is fundamentally a discrete 2D/3D scene or world. A Zone is a mathematical bound over a continuum or other domain. Spatial \((x,y,z)\) coordinates are one important case, not the ontology of Zone itself.

The earlier plan already admits non-spatial axes such as time or frequency and explicitly says that a Dimensional Zone is defined by authored axes, not by a hardcoded enum of 1D / 2D / 3D world kinds.

### Idea B — rich analysis may compile into simple spatial boxes

While discussing the camera-independent proof grid, Zach proposed that Earthcall need not choose between:

- rich AOT / incrementally maintained polyhedral reasoning; and
- GPU-friendly hierarchical 3D boxes.

Instead, rich reasoning may derive the mathematical knowledge first, and a later lowering stage may compile that knowledge into hierarchical 3D box slices or other cheap runtime structures.

The unification is:

\[
\boxed{
\text{Zone meaning}
\neq
\text{proof representation}
\neq
\text{execution representation}
}
\]

A Zone's mathematics is authoritative. Proof regions are derived knowledge about that mathematics. A 3D hierarchy, BVH, sparse brick set, query region, or ray interval program is a consumer-specific materialized view of that knowledge.

---

# 1. Thesis

Earthcall should not assume that \(x,y,z\) are the only, or even the most fundamental, coordinates of mathematical validity.

For an authored expression with independent variables \(q\), let

\[
q\in\mathcal S
\]

where \(\mathcal S\) is the **smallest state space required by the mathematics being reasoned about**.

A Zone, field, Law, or derived proof may depend on variables such as:

\[
q=(x,y,z,t,\omega,\theta_1,\theta_2,\ldots)
\]

plus optional lifted proof coordinates that are not independent degrees of freedom.

The strongest form of this proposal is not:

> Replace voxels with 3D polyhedra.

It is:

> **Reason over the actual mathematical state space; derive validity regimes there; then specialize, slice, project, and compile those regimes into the cheapest representation each consumer needs.**

For Screen, that consumer may want 3D boxes, ray-family validity regions, direct 1D ray intervals, or ordinary fallback execution.

---

# 2. Three levels of coordinates

## 2.1 Independent state variables

These are genuine free variables of the authored mathematics.

Examples, only when the expression actually depends on them:

- spatial position \(x,y,z\);
- time \(t\);
- outgoing/view direction \(\omega_o\in S^2\);
- incoming/source direction \(\omega_i\in S^2\);
- authored continuous parameters \(\theta_i\);
- transform or pose parameters;
- input/control premises when a proof is explicitly conditional on them;
- material or field parameters that remain variable in the regime being analyzed.

There is **no universal giant coordinate list**. Dependency structure determines dimensionality.

This continues OntoMath's existing rule that coordinates are named by mathematics and bound by a consuming channel.

## 2.2 Lifted semantic/proof variables

A derived quantity may be introduced as an additional proof coordinate only when doing so buys proof power.

For example, if

\[
g(q)=\sin(x+t)-0.3,
\]

the proof domain may introduce

\[
u_g=g(q)
\]

and reason cheaply about

\[
u_g\ge0,
\]

provided it retains a lawful relation or conservative relaxation connecting \(u_g\) back to \(q\).

Other possible lifted values include:

\[
u_\sigma=\sigma(q),
\qquad
u_{\mathrm{support}}=h(q),
\qquad
u_{\mathrm{clamp}}=c(q),
\qquad
u_{M_2}=\|\nabla^2\sigma(q)\|.
\]

These are not new independent degrees of freedom.

\[
\boxed{\text{derived quantity}\neq\text{base dimension}}
\]

Do not turn every named Property or intermediate expression into a dimension.

## 2.3 Consumer execution coordinates

A consumer specializes the richer state space to the variables it actually needs.

For Screen, a typical descent is:

\[
\mathcal S
\]

\[
\downarrow\quad\text{fix Moment / authored context}
\]

\[
(x,y,z,\omega_o,\ldots)
\]

\[
\downarrow\quad\text{fix one camera sample}
\]

\[
(x,y,z)
\]

\[
\downarrow\quad \mathbf p(s)=\mathbf o+s\mathbf d
\]

\[
\boxed{s}
\]

The one-dimensional ray parameter is not fundamental either. It is the final coordinate produced by specializing richer authored mathematics to the question Screen asks.

This reframes one-minimal-ray-program as **dimensional collapse by lawful specialization**.

---

# 3. The state space is not necessarily Euclidean

The strongest architecture must not pretend every coordinate belongs to one flat \(\mathbb R^n\).

A useful conceptual form is:

\[
\mathcal S=
\mathbb R^3_{\mathrm{position}}
\times
\mathbb R_{\mathrm{time}}
\times
S^2_{\omega_o}
\times
S^2_{\omega_i}
\times
\Theta
\times
\mathcal P
\times
\mathcal D.
\]

Here:

- \(\Theta\) denotes authored parameter domains;
- \(\mathcal P\) denotes pose or other structured continuous state;
- \(\mathcal D\) denotes discrete semantic regimes such as decided branches or enabled states.

Direction may be stored as three numbers in a backend but mathematically has two continuous degrees of freedom on \(S^2\). Rotations likewise should not become globally privileged Euler-coordinate assumptions merely because one implementation stores three angles.

High-dimensional polyhedron is therefore shorthand for one useful proof representation over suitable coordinates, not a claim that all Earthcall state is globally one Euclidean vector space.

---

# 4. What a proof polyhedron actually encloses

In the strongest version, a polyhedron does **not** wrap an Object, cloud, mesh, or visible spatial volume.

It wraps a **validity regime**: a set of possible states over which selected mathematical conclusions remain jointly true.

For a coordinate vector \(q\), a convex proof region may be represented as:

\[
P=\{q:Aq\le b\}.
\]

Attached facts might include:

\[
P\Rightarrow\sigma(q)=0,
\]

\[
P\Rightarrow\operatorname{sign}(g_1(q))=+,
\]

\[
P\Rightarrow\operatorname{sign}(g_2(q))=-,
\]

\[
P\Rightarrow\|\nabla^2\sigma(q)\|\le M_2.
\]

The polyhedron is a finite convex description of **where a statement remains valid**, not the ontology of the thing whose mathematics produced the statement.

## 4.1 Vertices

A vertex is an extreme feasible state: a combination of variable values at which enough independent constraints are simultaneously tight.

A vertex may mean latest allowed time + extreme authored amplitude + one active semantic boundary. It is a corner of possibility space, not necessarily a physical corner.

For affine objectives, extrema over a convex polytope occur at extreme points, which is why vertices can be useful proof machinery.

## 4.2 Edges

An edge is a one-dimensional family of feasible states where enough constraints remain tight that one degree of freedom remains.

It need not correspond to a visible edge in 3D geometry.

## 4.3 Facets

A facet can correspond directly to a real semantic regime boundary.

If:

\[
g(q)=0
\]

separates:

\[
g(q)<0
\]

from:

\[
g(q)>0,
\]

and the authored expression contains:

\[
\max(g(q),0),
\]

then that facet is a genuine mathematical branch boundary.

That is categorically different from an arbitrary storage plane such as:

\[
x=4k.
\]

---

# 5. The key reversal: mathematics discovers regimes first

A conventional proof grid starts with boxes and asks:

> What can I prove inside each box?

The stronger proposal reverses the authority:

\[
\boxed{
\text{derive mathematical regimes first}
\rightarrow
\text{materialize them for a consumer second}
}
\]

A possible sequence is:

1. analyze expression dependencies and semantic boundaries;
2. derive cheap correlated constraints over relevant state variables;
3. attach facts and validity premises to those regions;
4. specialize to current context;
5. slice/project into consumer coordinates where useful;
6. compile into the runtime representation that wins economically.

The load-bearing rule is:

> **The spatial hierarchy should be a materialized view of the proof system, not the proof system itself.**

---

# 6. Polyhedra are a proof domain, not the new ontology

Do not replace grid-is-fundamental with polyhedron-is-fundamental.

Different mathematical questions deserve different proof domains.

An illustrative economic ladder is:

\[
\text{interval / box}
\rightarrow
\text{affine form / zonotope}
\rightarrow
\text{octagonal / relational domain}
\rightarrow
\text{polyhedral refinement}
\rightarrow
\text{nonlinear certified relaxation}
\rightarrow
\text{exact / fallback evaluation}.
\]

The principle is:

> **Use the cheapest proof representation sufficient to answer the current question. Escalate only when the cheaper domain cannot decide something economically important.**

No proof representation becomes a new authored kind or hardcoded domain noun.

---

# 7. AOT and incremental analysis

A proof region is derived state and therefore must declare at least:

\[
(\text{derived from},\text{invalidated by},\text{guarded by}).
\]

A conceptual proof record may be:

\[
R_i=(P_i,F_i,\Pi_i,V_i)
\]

where:

- \(P_i\): validity region;
- \(F_i\): facts proved within it;
- \(\Pi_i\): premises/dependencies;
- \(V_i\): validity information including bounded time/parameter regimes.

If:

\[
t=12.0\rightarrow12.1
\]

while the proof remains valid for:

\[
t\in[10,15],
\]

nothing needs rebuilding merely because a frame advanced.

When a premise crosses its validity boundary:

\[
\text{premise change}
\rightarrow
\text{affected proof-region repair}
\rightarrow
\text{affected projection/materialization repair}
\rightarrow
\text{affected runtime-node patch}.
\]

This is Derived-State Ledger discipline applied to mathematical proof geometry.

---

# 8. Spatial materialization: boxes may still win

This proposal does not oppose grids, octrees, sparse bricks, BVHs, or hierarchical AABBs.

It changes their authority.

Suppose:

\[
P_i\Rightarrow\sigma=0.
\]

For current context \(c\):

\[
P_i|_c
\]

may be sliced/projected into current spatial coordinates:

\[
S_i=\Pi_{xyz}(P_i|_c).
\]

Screen may then encode that result in deliberately simple hierarchical boxes.

The boxes are allowed to be stupid and fast.

Their job is not to discover truth. Their job is to **carry already-derived truth economically to the GPU**.

New proof knowledge may refine one UNKNOWN node into ZERO or split only that node into known and unknown children.

---

# 9. Ray specialization

For one camera ray:

\[
\mathbf p(s)=\mathbf o+s\mathbf d,
\]

a spatial or richer proof region may collapse to one or more intervals:

\[
s\in[s_0,s_1].
\]

If the proof already fixes branch signs, the ray compiler can eliminate those branches before execution, subject to device-faithfulness requirements.

The target remains:

\[
\text{authored rich mathematics}
\rightarrow
\text{semantic specialization}
\rightarrow
\text{exact pieces / solved support / shared invariants}
\rightarrow
\text{small numerical residual}
\rightarrow
\text{fallback only where necessary}.
\]

A proof grid is one possible helper, not the identity of ray-domain transport.

---

# 10. Real boundaries versus representation boundaries

A branch surface:

\[
g(q)=0
\]

can be a genuine boundary if crossing it changes the authored expression or invalidates a proof premise.

An arbitrary cell boundary is not necessarily mathematical.

Likewise, a boundary required by one proof order must not automatically become a boundary for another.

> **Split where a premise changes, not merely where a current representation becomes uncomfortable.**

---

# 11. Consumers are queries over one authored reality

The same semantic/proof knowledge may compile differently for different consumers.

For Screen:

\[
\text{proof facts}
\rightarrow
\text{spatial hierarchy or query regions}
\rightarrow
\text{ray intervals}
\rightarrow
\text{GPU program}.
\]

For movement:

\[
\text{proof facts}
\rightarrow
\text{reachable-state envelope}.
\]

For audio:

\[
\text{proof facts}
\rightarrow
\text{time/frequency execution partition}.
\]

None requires a Zone to become a 3D world merely because Screen is currently the consumer being optimized.

---

# 12. Architecture of authority

The authority ordering is:

\[
\boxed{\text{authored Zone / OntoMath mathematics}}
\]

\[
\downarrow
\]

\[
\boxed{\text{derived semantic/proof knowledge}}
\]

\[
\downarrow
\]

\[
\boxed{\text{consumer-specific materialized view}}
\]

\[
\downarrow
\]

\[
\boxed{\text{specialized executable}}
\]

A GPU hierarchy must not redefine a Zone. A proof partition must not redefine authored mathematics. A ray program must not become a second source of world truth.

---

# 13. Weaker versions explicitly rejected

### 13.1 Replace cubes with arbitrary 3D polyhedra

Useful possible optimization, but too weak as architecture. It still assumes the fundamental proof domain is \(\mathbb R^3\).

### 13.2 Fit a convex hull around the visible cloud

Support geometry, not the full proposal. It says little about time validity, branch stability, directional dependence, derivative bounds, or authored parameters.

### 13.3 Tetrahedralize the world

Rejected as a foundational interpretation. It substitutes one spatial tessellation for another while still asking mathematics to conform to a preselected partition.

### 13.4 One giant high-dimensional polyhedron per Zone

Rejected. Real mathematics may be nonlinear, nonconvex, disconnected, piecewise, or cheaper in several proof domains.

### 13.5 Send high-dimensional polyhedra directly to the GPU

Rejected as a default. Reason richly; execute cheaply.

### 13.6 Every Property becomes a dimension

Rejected. Only actual independent variables belong in the base state space. Derived quantities are lifted only when they buy proof power.

---

# 14. Economic doctrine

A longer-lived proof is not automatically a better proof representation.

Camera-independent reasoning may gain reuse while losing tightness:

\[
M_{\mathrm{region}}\ge M_{\mathrm{exact\ query}}
\]

for many useful bounds.

Therefore:

> Cache truth at its longest valid lifetime.

becomes, for derived execution state:

> **Cache derived truth at its longest valid and economically useful lifetime.**

The compiler must remain free to choose between reusable-but-loose proof, tighter short-lived proof, exact symbolic simplification, local analysis, cheap numerical execution, exact fallback, and offline/reference-only proof.

Correctness determines admissibility. Measurement chooses among lawful implementations.

---

# 15. Performance preflight requirement

Before implementing a sophisticated proof representation as a production acceleration path, record:

1. current execution equation;
2. candidate execution equation;
3. optimistic constants;
4. fixed overhead;
5. break-even threshold;
6. invalidation/rebuild cost;
7. kill condition.

Do not compare only field sample counts. Compare the whole consumer path:

\[
T_{\mathrm{candidate}}
=
T_{\mathrm{field}}
+
T_{\mathrm{region/query\ traversal}}
+
T_{\mathrm{proof\ lookup}}
+
T_{\mathrm{branch\ logic}}
+
T_{\mathrm{error\ accounting}}
+
T_{\mathrm{device\ qualification}}.
\]

---

# 16. First falsification witness

Before any general state-space proof engine:

1. choose one Northern Veil curtain and one high-cost branch/switch family;
2. identify the true variables on which that branch depends;
3. derive the cheapest correlated region that decides the branch;
4. specialize over a bounded current time / movement / parameter regime;
5. materialize to boxes only if that is cheaper than direct query/ray specialization;
6. compare against box-first reasoning.

Measure proof-build time, region count, materialized state, invalidation footprint, GPU bytes, regions crossed per ray, switch evaluations, field evaluations, total GPU time, amortized frame cost, and exact/certified parity.

Do not build the cathedral before this witness says the direction is economically plausible.

---

# 17. Open questions for Zach and Original Prism Sun

### Q1 — What is the canonical identity of a proof region?

Ephemeral compiler state, or a generic derived-state representation carrying dependencies and validity without creating a domain noun?

### Q2 — How should mixed continuous/discrete regimes be represented?

Annotations/facts on continuous regions, or an explicit product with a discrete state component?

### Q3 — Which relational abstract domains deserve admission first?

Intervals/boxes, affine arithmetic, zonotopes, octagons, polyhedra, support functions, or problem-specific domains?

### Q4 — Slice or projection?

When should a variable be fixed, projected away, retained, or cause refusal?

### Q5 — How should nonlinear boundaries be represented?

Affine boundaries fit polyhedra exactly; norms, Perlin, quadrics, and transcendental surfaces do not.

### Q6 — Where should consumer materialization live?

Proof knowledge must not become Screen-private authority, while Screen should own its execution representation.

### Q7 — What is the minimal invalidation vocabulary?

Reuse existing per-channel authored revisions, Timeline/Law state and dependency frontiers; do not invent a second dirty graph.

---

# 18. Canon

> **A Zone is a mathematical bound, not fundamentally a 3D world.**

> **The dimensions of proof are the independent variables the mathematics actually depends on, not a universal \(x,y,z\) assumption.**

> **Proof representations may introduce correlated or lifted coordinates, but they are derived machinery, not ontology.**

> **Spatial/query hierarchies are consumer-specific materialized views of knowledge, not sources of truth.**

> **Specialize reality downward until the consumer receives the smallest lawful executable question it actually asked.**

In Prism form:

\[
\boxed{
\text{Meaning}
\neq
\text{Proof Representation}
\neq
\text{Materialized View}
\neq
\text{Executable}
}
\]

with the lawful descent:

\[
\boxed{
\text{rich authored mathematics}
\rightarrow
\text{adaptive proof knowledge}
\rightarrow
\text{consumer materialization}
\rightarrow
\text{dimensional specialization}
\rightarrow
\text{minimal execution}
}
\]

---

# Addendum A — Unknown movement, backward compilation, and ray-family validity cells

## A.1 Zach's refinement

Zach sharpened the proposal by identifying the setting in which the extra-dimensional validity region may matter most:

> **unknown movement.**

The important object may be a validity region over a family of possible future movement/camera states and the rays generated from them.

Zach proposed that:

- relevant mathematical kinks or branch boundaries can be converted into angular/query-space structure;
- an extra-dimensional polyhedral or otherwise correlated validity region can bound the movement + ray states for which the same conclusions hold;
- those conclusions can apply to **all rays represented by that set**;
- compilation should run partly **backwards from semantic relevance**, in a Magic-Set-like fashion.

This is now the stronger current version of the proposal.

## A.2 Pull a semantic boundary backward through the ray generator

Let a semantically meaningful boundary be:

\[
K=\{p\mid g(p)=0\}.
\]

Let the ray generator be:

\[
r(m,u,s)=o(m)+s\,d(m,u),
\]

where:

- \(m\) is unresolved movement/camera state;
- \(u\) is pixel, angular, or ray-family state;
- \(s\) is distance along the ray.

Instead of waiting for each concrete ray to rediscover \(K\), analyze:

\[
g(r(m,u,s))=0.
\]

Define the relevant preimage:

\[
Q_K=\{(m,u,s)\mid g(r(m,u,s))=0\}.
\]

Query-space validity regions may then carry conclusions such as:

\[
Q_i\Rightarrow g>0,
\]

\[
Q_j\Rightarrow g<0,
\]

\[
Q_k\Rightarrow\text{exactly one crossing in }[s_a,s_b],
\]

or:

\[
Q_\ell\Rightarrow K_1\prec K_2\prec K_3.
\]

The new rule is:

> **Do not only compile the field into space. Compile semantic relevance into the space of queries that can observe it.**

## A.3 Unknown movement becomes a dimension of validity

Rather than specialize only to exact pose \(m_0\), reason over:

\[
m\in M.
\]

A compiled ray-family answer may declare schematically:

\[
M\times U_i\Rightarrow F_i,
\]

with correlations retained when movement and direction are not independent.

Movement inside \(M\) reuses the answer. Crossing a validity facet triggers only dependent repair.

> **Inside a stable regime, continue the answer. At a real validity boundary, re-solve only what lost a premise.**

## A.4 Local kink geometry may become angular structure

For \(c\) on a semantic kink:

\[
g(c)=0,
\]

locally:

\[
g(c+\delta)
\approx
\nabla g(c)\cdot\delta
+
\frac12\delta^T H(c)\delta+\cdots.
\]

A bounded set of possible camera origins may see that local patch only through an angular family:

\[
\Omega_K.
\]

A sound result may prove:

\[
u\notin\Omega_K
\Rightarrow
\text{kink impossible in this movement regime}.
\]

Inside \(\Omega_K\), a tighter result may bound intersection depth, branch sequence, or support.

The proposal does not require every nonlinear surface to become one literal cone/polyhedron. It requires choosing the cheapest correlated query-space bound that soundly removes repeated work.

## A.5 Backward compilation / Magic-Set-style demand propagation

Forward rendering asks:

\[
(\text{movement state},\text{pixel/ray})
\longmapsto
\text{radiance}.
\]

The compiler may also reason backward:

\[
\text{semantic feature}
\longmapsto
\text{set of queries for which it can matter}.
\]

The analogy to Magic Sets is architectural: bind demand, propagate relevance backward, specialize reachable computation, and avoid work that cannot contribute.

Candidate pipeline:

\[
\text{authored mathematics}
\rightarrow
\text{semantic boundaries/support}
\rightarrow
\text{pull back through }r(m,u,s)
\rightarrow
\text{query validity regions }Q_i
\]

\[
\rightarrow
Q_i\Rightarrow
\{\text{branch signature, support, intersection order, bounds, integrator}\}
\rightarrow
\text{runtime regime lookup}
\rightarrow
\text{tiny ray program}.
\]

## A.6 Ray/query space may be the natural intermediate domain

A family of rays is not fundamentally a set of points in \(\mathbb R^3\).

For semantic feature \(K\):

\[
\mathcal R_K=\{r:r\cap K\neq\varnothing\}.
\]

Unknown movement induces:

\[
\mathcal R_M.
\]

Only:

\[
\mathcal R_K\cap\mathcal R_M
\]

is relevant to the current movement regime.

This does not commit Earthcall to Plücker coordinates or any specific line representation. It records that point-space is not automatically the best reasoning space for ray queries.

## A.7 3D materialization becomes optional

The lawful routes now include:

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

A grid/BVH remains useful when it amortizes well; it is not a mandatory waypoint.

## A.8 Strongest role of the validity polyhedron

Its strongest role is:

> **a finite correlated description of unresolved movement + query states over which the same backward-compiled semantic answer remains valid.**

Its facets may represent movement limits, angular limits, semantic branch transitions, support entry/exit changes, intersection-order changes, or expiry of derivative/integration premises.

A vertex is an extreme feasible **query-state configuration**, not necessarily an extreme world-space point.

## A.9 Economic objective

Let:

- \(A(M)\): analysis/compile cost;
- \(N(M)\): useful validity lifetime in frames/query batches;
- \(R(M)\): runtime cost while using the compiled answer.

Then choose:

\[
\boxed{
M^*
=
\arg\min_M
\left[
\frac{A(M)}{N(M)}
+
R(M)
\right]
}
\]

subject to soundness and execution-faithfulness.

Wider validity often increases \(N(M)\) while loosening \(R(M)\). Narrower validity often lowers \(R(M)\) while reducing \(N(M)\).

The right regime is the one with the best amortized lawful economics.

## A.10 Quantitative companion result

The companion analysis uses the current controlled Northern Veil midpoint:

\[
T_0=138.5\mathrm{ms/frame}
\]

and a broad sensitivity model over unknown native cost shares, remaining field evaluations, semantic-work elimination, and query-regime lookup overhead.

Its runtime-only Monte Carlo model produced:

\[
\text{median}\approx82.7\mathrm{ms/frame}
\]

or:

\[
\text{median speedup}\approx1.68x,
\]

with a broad 5th–95th percentile speedup range of approximately:

\[
1.38x\text{–}2.21x.
\]

A deterministic strong scenario gives approximately:

\[
51.9\mathrm{ms/frame}\approx2.67x,
\]

while an intentionally aggressive scenario reaches approximately:

\[
30.4\mathrm{ms/frame}\approx4.56x.
\]

These are **sensitivity-model outputs, not measured renderer performance**.

Using the median runtime result, the modeled pre-analysis saving is approximately:

\[
55.83\mathrm{ms/frame}.
\]

Therefore a hypothetical 1000 ms analysis build would break even after approximately:

\[
17.9\text{ frames}
\]

if the compiled validity result survives that long and the runtime assumptions hold.

The full assumptions, quantiles, failure modes, and falsification contract are in the companion analysis. This proposal should not quote the numbers without that caveat.

## A.11 Performance hypothesis

The bet is not that high-dimensional reasoning is intrinsically cheap.

The bet is:

\[
\text{expensive semantic reasoning once}
\ll
\text{same semantic rediscovery across millions of rays and many frames}.
\]

Old style:

\[
\text{pixel}
\rightarrow
\text{march}
\rightarrow
\text{rediscover support/kinks/branches}
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
\text{execute small residual}.
\]

> **Add dimensions to reasoning only when doing so removes more dimensions or repeated work from execution.**

## A.12 Falsification witness

Before building a general query-space compiler:

1. choose one expensive Northern Veil kink/switch family;
2. identify its actual movement and ray/angular dependencies;
3. pull its semantic boundary backward through the ray generator;
4. construct the cheapest sound correlated validity regions distinguishing MISS, HIT, branch order and useful depth regimes;
5. reuse them over bounded movement;
6. compare against current per-ray rediscovery.

Measure \(A(M)\), \(N(M)\), query-region count, invalidation frequency, lookup cost, switch work removed, field work removed, ray-program size, total GPU time, amortized frame cost, and parity.

Kill the approach if:

\[
\frac{A(M)}{N(M)}+R(M)\ge R_0.
\]

## A.13 Updated distinction

\[
\boxed{
\text{world-space geometry}
\neq
\text{semantic validity space}
\neq
\text{ray/query space}
}
\]

The compiler may transfer derived knowledge between them while preserving authored meaning.

The strongest current descent is:

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

## Attribution boundary

**Zach originated:**

- the doctrine that Zones are fundamentally mathematical bounds rather than discrete 3D/2D worlds;
- the challenge of why \(x,y,z\) should be privileged as fundamental dimensions;
- the proposal for AOT/incremental polyhedral/correlated analysis compiled down to hierarchical 3D box slices;
- the desire to retain cheap grid-like runtime structures while moving richer reasoning above them;
- the realization that extra-dimensional validity matters especially for **unknown movement**;
- the idea that relevant mathematical kinks can become angular/query-space structure;
- the idea that one higher-dimensional set-space region can make conclusions apply to **all represented rays**;
- the backwards-compilation / Magic-Set-style framing.

**Original Prism Sun formalized / extended:**

- Meaning \(\neq\) Proof Representation \(\neq\) Materialized View \(\neq\) Executable;
- the proof-polyhedron-as-validity-regime interpretation;
- independent vs lifted vs execution coordinates;
- vertices/edges/facets as extreme feasible states and regime boundaries;
- the mixed state-space formulation;
- the adaptive proof-domain ladder;
- the spatial hierarchy as a materialized view;
- dimensional collapse by ray specialization;
- the preimage formulation \(g(r(m,u,s))=0\);
- query-space validity regions \(Q_i\);
- \(\mathcal R_K\cap\mathcal R_M\);
- the direct query-space path that may bypass 3D materialization;
- the amortized objective \(\min_M[A(M)/N(M)+R(M)]\);
- the sensitivity model and falsification contract.

The collaboration pattern is:

> **Zach changes the space in which the problem should be asked. Original Prism turns that reframing into explicit mathematics, compiler boundaries, invalidation rules, and measurable economic hypotheses.**
