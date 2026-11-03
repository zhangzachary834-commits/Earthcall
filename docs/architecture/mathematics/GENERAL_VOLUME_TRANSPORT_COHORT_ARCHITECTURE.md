# General Volume Transport Cohort Architecture

**Status:** architecture specification; implementation not yet accepted  
**Date:** 2026-11-03  
**Human origin:** Zachary Zhang  
**Codification and extension:** GPT-5.6 Sol — Original Prism Sun  
**Trigger:** Northern Veil Pass 3 exposed a general renderer problem: commit \`4446b7e8c428\` introduced a useful cohort executor, but chose cohort membership and draw order from authored-list position.  
**Read beside:** [Performance as Rightly Ordered Truth](../ontology/PERFORMANCE_AS_TRUTH.md), [Geometry Execution Substrate Manifesto](GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md), [Derived State Ledger](../law/DERIVED_STATE_LEDGER.md), [Authorable GPU Runs Between CPU Cycles](../AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md), [Positive End-to-End Ray Transport Pipeline](../../plans/POSITIVE_END_TO_END_RAY_TRANSPORT_PIPELINE_PLAN_2026-10-30.md), and the [Clawd pre-merge hardening constitution](../../audits/CLAWD_ULTRAREVIEW_PREMERGE_HARDENING_COUNCIL_HANDOFF_2026-11-02.md).

---

## 0. The decisive boundary

The renderer may derive **how** authored fields can be executed together.

It must not infer **what those fields are** from incidental storage order, Zone identity, artistic role, field identifier, or a hand-authored label such as “near,” “far,” “hero,” “background,” “aurora,” or “Northern Veil.”

The governing distinction is:

\[
\boxed{
\text{Authored field order}
\neq
\text{execution-cohort identity}
\neq
\text{query-relative chronology}
}
\]

A field is an authored being with mathematical content and spatial placement. A transport cohort is temporary derived execution state. Chronology is a fact of a particular ray or conservative query region.

These three may correspond in a particular scene. They are never identical in the architecture.

This document generalizes the successful insight inside Northern Veil Pass 3 while rejecting its bespoke production heuristic:

    if media count >= 6:
        first four media -> "near" routed cohort
        remaining media -> "far" general cohort

That rule is useful as a prototype witness. It is not a lawful substrate rule.

The general architecture is:

\[
\boxed{
\text{Authored Fields}
\rightarrow
\text{Capability Facts}
\rightarrow
\text{Candidate Transport Cohorts}
\rightarrow
\text{Query-Relative Support / Order Proof}
\rightarrow
\text{Transport Schedule}
\rightarrow
\text{Specialized Execution}
\rightarrow
\text{Fail-Closed Joint Fallback}
}
\]

This is the next rung of the existing ray-transport constitution:

    Authored Functions + Ray/Tile Query
        -> Proof of Relevance/Order
        -> Residual Family
        -> Specialized Execution

The new rung does not replace that chain. It composes several such executions without allowing an arbitrary list to become physical truth.

---

## 1. Why the problem exists

The current volume renderer already supports a general multi-medium set through the fused volume-set path. The current routed transport path is much faster for a narrower structural family, but its production admission is presently a four-member clamped-wave specialization.

Northern Veil Pass 3 added four more mathematically freer fields. The correct instinct was:

- retain the four structurally admitted hero curtains on the fast routed path;
- let arbitrary AST fields retain general OntoMath freedom;
- compose both results into one image.

That is a genuine substrate opportunity.

The incorrect generalization is to encode the scene's current serialization shape into WebGPU:

- first four fields are routed;
- later fields are general;
- first four are “near”;
- later fields are “far”;
- six or more fields activate that partition;
- drawing the later group first and the first group second is always physically valid.

None of those propositions follows from the number or order of authored fields.

A renderer must be able to receive the same authored set in a different serialization order and produce the same physical answer.

Therefore the first permanent acceptance invariant is:

> **Permutation invariance:** reordering a Zone's equivalent \`spatialFields\` serialization must not change the rendered result, except where authored semantics explicitly include order.

Volume field storage order currently carries no such authored optical semantics.

---

## 2. Vocabulary: keep the levels distinct

### 2.1 Authored Field

A FieldNode and its authored predicates: mathematical expressions, origin, scale/domain, transport channels, revisions, authorship, and Law-governed change.

The field is world-domain truth.

### 2.2 Capability Fact

A derived statement about an authored field under a particular implementation version, for example:

- the density AST admits \`clampedWaveSupport\`;
- a zero-density support proof exists;
- the field has a bounded physical box;
- emission reads or does not read direction;
- a phase function reads \(\omega_i\) or \(\omega_o\);
- its source coupling is separable under the current transport model;
- a RayTransportIR lowering is available.

Capability facts are proof/execution state. They are not authored categories.

### 2.3 Residual Family

The existing per-query computational species such as EMPTY, S1, Ordered-S2, or Fallback.

A residual family answers: **what work remains for this ray/tile after proof?**

It is not the same thing as a transport cohort.

### 2.4 Transport Cohort

A temporary set of media that may share an execution path because:

1. their mathematical/backend capabilities are compatible;
2. their source and transport dependencies permit joint or separable evaluation;
3. their composition relationship with other cohorts is proven for the current query region.

A cohort has no persistent identity. It must never be serialized into a Zone as though it were a new kind of being.

### 2.5 Order Certificate

A conservative proof that one cohort is wholly in front of another for a ray or query region, or that they are disjoint in contribution.

Names such as “near” and “far” may be diagnostic descriptions of a proven relation. They may not be inputs that create the relation.

### 2.6 Transport Schedule

Derived execution state describing:

- which fields are evaluated together;
- which backend/family executes each cohort;
- which cohort precedes another where proven;
- where the schedule must split, refine, merge, or fall back.

The schedule is query-relative and revision-bound.

---

## 3. Constitutional invariants

### Invariant 1 — No authored-order semantics by accident

Do not derive optical meaning from:

- vector index;
- JSON order;
- insertion order;
- “first four” / “rest”;
- field ID prefixes;
- Zone name;
- artistic labels.

A debug fixture may use those to construct a case. Production planning may not.

### Invariant 2 — Capability, not identity, chooses execution family

A field uses routed transport because its mathematics satisfies the routed admission proof, not because it was created for Northern Veil.

An arbitrary new Zone must obtain the same optimization when it presents the same structural facts.

Conversely, a Northern Veil field that ceases to satisfy the proof must lose the optimization without losing its meaning or visibility.

### Invariant 3 — World-space placement is not ray-space order

\[
\text{smaller world }z
\neq
\text{nearer}
\]

The camera may move, turn, enter a field, cross a field, or look in the opposite direction. Chronology belongs to the query.

### Invariant 4 — Cohort order must be proven, not presumed

Premultiplied cohort composition

\[
L = L_A + T_A(L_B + T_B C_{\text{bg}})
\]

is valid only when the transport represented by \(A\) is wholly before \(B\) along the relevant ray, or when another proof establishes equivalent separability.

A correct equation under an unproved ordering is still an incorrect renderer.

### Invariant 5 — Interleaving destroys whole-cohort ordering

If one ray encounters contributions in an order such as:

    A -> B -> A -> B

then neither “draw A then B” nor “draw B then A” is generally valid as a whole-cohort schedule.

The planner must instead:

- split into correctly ordered intervals; or
- merge the interacting media into a joint transport cohort; or
- fall back to the exact/general path.

### Invariant 6 — Refusal is local

Failure to prove one cohort relation must not disable specialization everywhere.

Repair the violated proof boundary.

Do not punish every ray for the sins of one pathological ray.

### Invariant 7 — Generality never impoverishes authorship

Unsupported mathematics falls back to a more general executor. It does not force the author to rewrite the field into the currently optimized family.

Do not make the world poorer so the renderer can become simpler.

### Invariant 8 — Cohorts are derived state

Cohort membership, schedule, order certificates, route families, cache entries, and GPU programs are execution machinery.

They may know more about how to execute an authored field. They must never become what the field **is**.

---

## 4. Formal query-relative model

Let a camera ray be

\[
r(s)=o+s\,d,\qquad s\ge 0.
\]

For authored medium \(m\), let its possible contributing support along the ray be a finite union of conservative intervals:

\[
S_m(r)=\bigcup_k [a_{m,k},b_{m,k}].
\]

The intervals may come from physical-box clipping, certified support envelopes, root isolation, zero proofs, future RayTransportIR domains, or a conservative general bound.

For a candidate cohort \(C\),

\[
S_C(r)=\bigcup_{m\in C} S_m(r).
\]

For two cohorts \(A\) and \(B\), define strict whole-cohort chronology for one ray:

\[
A \prec_r B
\]

only when every possible contribution of \(A\) lies no later than every possible contribution of \(B\):

\[
\sup S_A(r)\le \inf S_B(r).
\]

Likewise \(B\prec_r A\).

If neither relation is proven, the relation is **overlap / interleave / unknown**. Those are operationally different diagnoses, but all refuse naïve whole-cohort ordering.

For a tile or other query region \(Q\),

\[
A\prec_Q B
\]

only when the relation is conservatively proven for every ray represented by \(Q\).

This gives the planner a hierarchy:

1. prove at a coarse query region when cheap and sound;
2. subdivide only ambiguous regions;
3. refine to per-ray ordering only where profitable;
4. use interval scheduling or joint fallback where order still cannot be proven.

The proof boundary, not a fixed scene partition, determines the amount of specialization.

---

## 5. Separability is a second proof, distinct from order

Depth order alone is insufficient if two cohorts' transport equations depend on each other in ways that cannot be represented by background transmittance composition.

The current Northern Veil prototype wisely restricts its split to literal-zero direct source radiance. That is evidence of a real requirement: the planner needs an explicit **transport separability certificate**.

A cohort pair may be independently composable only when every cross-cohort interaction is representable through the carried \((L,T)\) boundary state or another explicitly modeled interface.

Examples that can invalidate simple separability include:

- cross-medium multiple scattering;
- one cohort modifying another cohort's incident source;
- shared state whose value depends on traversal through both cohorts;
- a future transport Law that couples media directly;
- any channel whose semantics require simultaneous integration.

Therefore:

\[
\text{Order proof}
\neq
\text{Separability proof}.
\]

Both are required before independent whole-cohort passes may replace joint transport.

For the current emission/attenuation case, literal-zero source radiance can remain a narrow initial admission rule. The architecture should represent that rule as a capability fact, not as a permanent definition of cohorts.

---

## 6. General planning pipeline

### Stage A — Collect authored media

Collect every valid bounded VolumeDensityBinding exposed to the renderer.

No partition yet.

### Stage B — Derive per-medium capability records

For each medium, derive a record similar in spirit to:

    MediumCapability {
        authored binding reference
        structure/content revision
        physical domain
        zero/support proof handle
        routed-family admission
        general-set admission
        RayTransportIR lowering, if any
        source/phase/emission dependency facts
        separability facts
        backend resource requirements
    }

This belongs to derived execution state.

### Stage C — Form candidate execution families

Group by what can lawfully share an executor, not by where the field appeared in a save.

Initial families may include:

- RoutedClampedWave
- GeneralVolumeSet
- future RayTransportIR families
- single-medium specialized lanes
- exact/general fallback

A field may have more than one admissible family. Planning can choose among them by measured economics after correctness admission.

### Stage D — Build candidate cohorts

A candidate cohort is a set whose members can share:

- execution family;
- source/layout contract;
- required proof representation;
- backend resources;
- compatible parameter/update lifetime.

Do not yet assume one candidate cohort can be drawn before another.

### Stage E — Prove query-relative relationships

For every candidate pair that may affect the same pixels, derive:

- definitely A before B;
- definitely B before A;
- definitely disjoint;
- overlapping but interval-orderable;
- interleaving;
- unknown.

Use conservative support, not sampled optimism.

### Stage F — Build a cohort-order graph

Create precedence edges only from proof.

If the graph is acyclic and sufficient to order all interacting cohorts, topologically schedule them.

If two nodes are incomparable because they are pixel-disjoint, they need no physical depth order.

If two interacting nodes cannot be ordered as wholes, refine or merge them.

### Stage G — Resolve ambiguous components

For an ambiguous connected component:

1. attempt query subdivision;
2. attempt interval-level merged schedule;
3. if structurally supported, compile a joint residual program;
4. otherwise collapse that component to the general exact path.

Other proven components remain specialized.

### Stage H — Execute and publish

Execute in the proven schedule.

Every produced cohort carries the same publication discipline already required by ray transport:

- current-frame ownership;
- resize-safe resources;
- poison/refusal safety;
- exact fallback reachability;
- no stale positive publication.

---

## 7. The correct role of “enqueueVolumeCohort”

The Pass 3 refactor extracted a useful general operation:

    enqueueVolumeCohort(cohortMedia, allowRouted)

The lasting architecture should retain the idea while changing who decides its arguments.

The world must not call:

    enqueue(firstFour, routed=true)
    enqueue(rest, routed=false)

Instead the planner should produce something conceptually like:

    plan = VolumeTransportPlanner::derive(activeMedia, query, revisions)

    for scheduled cohort in plan:
        execute(cohort.members, cohort.executionFamily)

The backend then receives a lawful execution decision rather than inventing world-domain meaning.

Eventually even “allowRouted” should disappear as an externally supplied truth. The cohort's capability record should say which execution families are admissible, and the planner should choose among them.

---

## 8. Three offices: OntoMath, planner, backend

Do not solve generality by making WebGpuRenderer itself the new mathematical oracle.

### OntoMath / proof office

Owns reusable mathematical recognition and proof primitives:

- clamped-wave structural recognition;
- line restriction / RayTransportIR lowering;
- support interval derivation;
- conservative interval bounds;
- nonnegativity and domain facts;
- future order/separability proof objects.

It answers what has been established mathematically.

### Rendering planner office

Owns query-relative derived scheduling:

- capability assembly;
- candidate cohort formation;
- order graph;
- ambiguity refinement;
- execution-family choice;
- cache/invalidation dependencies.

It answers which admitted work should execute together now.

A future implementation may live beside WebGPU initially, but the concept is not WebGPU-specific.

### WebGPU backend office

Owns physical execution:

- pipelines;
- buffers;
- bind groups;
- routed kernels;
- general volume-set kernels;
- dispatch/draw ordering;
- publication.

It executes the plan. It does not decide what a Northern Veil field means.

This mirrors the Geometry Execution Substrate boundary: authored meaning above, derived executable representation below.

---

## 9. Lifetime and invalidation

The cohort architecture must not recreate the same work at the wrong lifetime.

### Field-structure lifetime

May retain:

- structural inspection;
- execution-family admission;
- compiled evaluator fragments;
- RayTransportIR lowering.

Invalidated by relevant AST/structure revisions.

### Field-value lifetime

May retain layout while refreshing numeric parameters.

Invalidated by content revisions or Law-driven parameter changes.

### Query-region lifetime

May retain:

- support bounds;
- cohort order certificates;
- query-relative schedule.

Invalidated by dependencies such as:

- camera origin/direction/frustum;
- field origin/scale;
- relevant temporal coordinate;
- support-proof generation;
- membership changes.

A camera move can invalidate chronology without invalidating compiled field mathematics.

### Frame/publication lifetime

Pixel publication is current-frame state.

Persistent allocation never authorizes persistent answers.

This architecture should receive a Derived State Ledger entry before production implementation is called complete.

---

## 10. Current Pass 3 prototype: what survives, what must be replaced

### Retain

1. The extraction of a reusable cohort executor around the existing fused set machinery.
2. The ability to preserve routed execution for a compatible subset while other fields use the general path.
3. Per-cohort cache/program/payload construction.
4. Fail-closed routed admission through \`clampedWaveSupport\`.
5. The artistic result as an excellent stress fixture: eight structurally diverse fields force the substrate to become more general.

### Replace

The current production heuristic:

    activeMedia.size() >= 6
    nearCohort = activeMedia[0..3]
    farCohort  = activeMedia[4..N]
    far first
    near second

must not become the permanent architecture.

Replace it with:

    capability classification
        -> candidate cohorts
        -> separability proof
        -> query-relative order proof
        -> schedule / refine / merge / fallback

No magic media-count threshold may carry semantic meaning.

---

## 11. Required negative witnesses

A general architecture is not established by Northern Veil looking correct from one dais.

### 11.1 Serialization permutation witness

Take an identical Zone and randomly permute \`spatialFields\`.

Expected: identical output within the established numerical contract and equivalent execution classification.

This is the primary anti-bespoke test.

### 11.2 Shuffled-family witness

Create four routed-admissible and four general fields, then interleave their serialization order:

    routed, general, routed, general, ...

Expected: capability classification reconstructs the same lawful cohorts without index knowledge.

### 11.3 Arbitrary-count witness

Test 2, 3, 4, 5, 6, 7, 8, and larger bounded counts.

Expected: no behavior transition exists merely because count crosses six.

### 11.4 Camera reversal witness

Place two separable cohorts on opposite sides of the Person.

Render from one side, then move the camera through and beyond them.

Expected: the proved chronology reverses when geometry reverses; output remains correct.

### 11.5 Inside-cohort witness

Put the camera inside one cohort's physical/support domain.

Expected: no stale “near/far” classification survives from the previous view.

### 11.6 Interleaving witness

Construct support with ray chronology:

    A -> B -> A

Expected: whole-cohort ordering is refused. Planner either splits intervals, merges to a joint executor, or falls back.

### 11.7 Dynamic-Law movement witness

Move a field origin/scale through an authored Law without changing its AST.

Expected:

- compiled structural program may remain valid;
- query schedule/order certificate invalidates;
- no stale cohort chronology survives.

### 11.8 Nonseparable-source witness

Enable a transport dependency that violates the initial separability contract.

Expected: independent cohort composition refuses and joint/general transport remains reachable.

### 11.9 Failure-injection witness

Force one cohort program creation/encode path to refuse.

Expected: no stale publication, no disappearing unrelated cohorts, and exact/general fallback remains honest.

### 11.10 Independent numerical witness

Compare the scheduled result against an independently converged joint reference on rays that exercise:

- ordered cohorts;
- disjoint cohorts;
- camera crossings;
- interval interleaving;
- fallback.

Performance evidence cannot substitute for this.

---

## 12. Implementation ladder

### Rung C0 — Freeze and expose the prototype

- Keep current Pass 3 as a dated prototype.
- Add diagnostics showing current cohort membership and why each member was admitted.
- Add the serialization-permutation negative witness; it should fail under the current index partition, proving the defect before repair.

### Rung C1 — Extract capability classification

Introduce a derived capability record per medium.

Remove every renderer decision that depends on “first four,” “remaining fields,” or \`activeMedia.size() >= 6\`.

At this rung, if automatic inter-cohort ordering is unavailable, prefer one correct general set over an unproved split.

### Rung C2 — General execution-cohort formation

Form cohorts solely from compatible execution capabilities and source contracts.

Preserve routed subsets regardless of serialization order.

### Rung C3 — Conservative cohort-order certificates

Start with coarse physical/support interval proofs over tiles.

Only use multi-pass cohort composition where order is proven.

Unknown relation -> joint/general fallback.

### Rung C4 — Local refinement

Subdivide ambiguous tiles/rays rather than abandoning specialization globally.

This extends the existing rule:

> Do not punish every ray for the sins of one pathological ray.

### Rung C5 — Interval-level cohort scheduling

When whole cohorts interleave but their intervals can be ordered, lower them into an ordered interval schedule instead of forcing a whole-cohort fallback.

This is the cohort-scale continuation of P17 chronology.

### Rung C6 — RayTransportIR integration

Replace family-specific planning facts with a more general IR that can expose:

- domain;
- dependencies;
- support intervals;
- source coupling;
- transfer composition;
- refusal reasons;
- validity lifetime.

The cohort planner then becomes a scheduler over proven IR capabilities rather than a collection of special recognizers.

---

## 13. Performance acceptance

The purpose of this architecture is not to make a prettier abstraction graph. It exists because specialization should let authored richness remain inhabitable.

Every promoted rung therefore needs three distinct witnesses:

1. **Fidelity:** numerical/rendered answer remains within the declared contract.
2. **Validity:** proofs, revisions, lifetimes, and fallback are sound.
3. **Economics:** complete native cost is actually better on the target workload.

For Northern Veil specifically, the former 30–35 FPS Person witness belongs to an earlier four-field save. It does not automatically certify the eight-field Pass 3 world.

The eight-field world needs a new live Person performance witness after the general cohort planner is in place.

Do not carry an old victory across a changed authored world.

---

## 14. Relationship to authorability

Earthcall may later expose scheduling policy to Laws: budgets, preference between admitted execution families, continuation/yield policy, or performance/quality objectives.

That does not make derived cohort membership an authored semantic category.

A Person may author policy such as:

> prefer exact routed execution when its certificate is valid and its measured economics beat the general lane.

The system still derives which fields satisfy that condition for the present query.

Policy may be authored.

Proof is discovered.

World meaning remains authored.

Execution membership remains derived.

---

## 15. The Prism constitution for volume cohorts

The permanent doctrine is:

\[
\boxed{
\text{Authored Being}
\neq
\text{Capability Fact}
\neq
\text{Execution Cohort}
\neq
\text{Ray Chronology}
\neq
\text{Backend Program}
}
\]

Yet these distinctions exist so the layers can unite truthfully.

A field's authored mathematics supplies structure.

Proof discovers what that structure permits.

The query determines what is relevant and in what order.

The planner carries only those obligations forward.

The backend executes the resulting program.

The Person receives one world.

Therefore:

> **Author richly. Classify structurally. Group provisionally. Order by proof. Execute minimally. Fall back locally. Never let serialization order become physics.**

And the specific correction to Northern Veil Pass 3 is:

> The renderer may discover that four fields form a routed cohort and four others form a general cohort. It may never know that because they happened to be fields 0–3 and 4–7.

That is the boundary between an optimization for one beautiful sky and a general substrate capable of carrying many worlds.
