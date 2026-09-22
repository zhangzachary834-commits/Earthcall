# Formation Rete — Relevant-Change Incrementality, Dynamic Route Repair, and Self-Refining Event Topology

**Status:** Architecture / next-rung specification, 2026-09-21. The Law-Direct first executable rung is already built and measured; the mechanisms in §§3–9 below are the next architecture and are **not yet claimed as implemented** unless explicitly marked otherwise.

**Origin and attribution.** Zach originated the architecture in this addendum on 2026-09-21: incrementality must continue into Formation Rete's higher tiers; beings, Categories, Relations, Relations-of-Relations, similarity structures, and retained paths should be reconsidered only when a **relevant** change can affect their proof; shortest-path discovery should become incremental repair rather than repeated whole-graph search; Prophetic analysis should continually refine the EventBus boundaries that deliver those changes; stable Relation/Formation subsystems may be compiled through OntoMath to reduce interpretive event-pushing overhead; the bootstrap must remain grounded in the fundamental PropertyPath/opcode framework exposed through human-facing First Movers; and when several sound short paths remain, the cheapest proved path should stay first without globally re-ranking unchanged alternatives every frame.

**Recorded and formalized by:** GPT-5.6 Sol, 2026-09-21. Sol's contribution is the dependency-frontier model, conservative invalidation contract, complexity framing, dynamic-shortest-path mapping, bootstrap sequence, and implementation/test decomposition. The originating architectural direction above is Zach's.

**Companions:** `FORMATION_RETE_TIERED_RELEVANCE_LADDER.md`, `FORMATION_RETE_DIRECT_RELEVANCE_ADDENDUM.md`, `PROPHETIC_RETE.md`, `DERIVED_STATE_LEDGER.md`, `../../plans/ontological_rete_architecture.md`, `../../Analysis/LAW_DIRECT_TRAVERSAL_COMPLEXITY_AND_CI_RESULTS_2026-09-19.md`.

---

## 0. The new rule

> **Nothing stable should be recomputed merely because time passed. Computation should follow relevant change.**

Law-Direct proved this rule at one rung. A lower tier proved a category road once; the hot path stopped asking the Relation graph the same question on every Law application.

The next step is to apply the same rule to the machinery that maintains the ladder itself.

It is not enough to make terminal Law execution incremental while higher-tier maintenance still behaves like:

```text
something changed
    -> every Category may be stale
    -> every retained route may be stale
    -> every shortest path may need rediscovery
    -> every event subscription may need reconsideration
```

The target is:

```text
semantic delta
    -> conservative Prophetic dependency frontier
    -> only proofs that could have changed become uncertain
    -> repair only those proofs / paths / priorities
    -> preserve every still-current result
```

Unknown remains permission to widen, never permission to narrow.

---

## 1. Current executable floor: what is already real

As of 2026-09-21 the code already provides the substrate this addendum extends:

- the complete sweep remains the correctness floor;
- Vocabulary narrows by required Property names;
- the Slow Adapter retains `Related(kind, category)` roads and advances on its own wall-time clock;
- `Prophetic::Index` has branch-stable read/write relevance structure and retains known edges even when opacity makes the graph incomplete;
- `LawManager::_candidateRoutes` selects one current tier in O(1) steady-state currency checks;
- the first `CandidateTier::LawDirect` rung is built;
- Slow Adapter and Law-Direct are currently enabled by default in `LawManager`;
- Law-Direct retains concrete bearers plus a live residual condition and falls downward when its proof currency goes stale;
- the hostile witness measured 131,072 repeated Relation queries -> 0 and a 4.02x speedup while preserving 65,536 applications;
- real Chess moved 42 Laws to Direct and reduced frame p95 from about 20.07 ms to 11.16 ms in the recorded CI run.

The measured asymptotic change for the proved category component is:

```text
before Direct: Theta(P * L * M * D)
after Direct:  Theta(P * L * M)
```

where `D` is endpoint Relation degree.

The next architecture asks: can the *maintenance* cost also scale with the changed dependency cone rather than the whole world?

---

## 2. Semantic deltas, not global revision panic

A revision counter is an excellent correctness witness and a coarse invalidation instrument. It is not the final relevance model.

Today a route may conservatively key on a world structural revision or Relation generation. That is sound, but a write to an unrelated Relation can make a proof look stale even when no premise of that proof changed.

The higher-tier target is a semantic delta:

```text
Delta {
    bearer / Relation / Formation identity,
    changed PropertyPath or Relation role,
    old abstract value if available,
    new abstract value if available,
    authored branch provenance,
    temporal / Zone context
}
```

Prophetic analysis consumes that delta and asks:

```text
Can this change possibly alter proof P?
```

Three answers are permitted:

1. **proved irrelevant** -> P stays current;
2. **possibly relevant** -> P becomes dirty / enters repair;
3. **unknown because analysis is incomplete** -> widen to the existing safe lower tier.

There is no fourth answer called "probably irrelevant, skip it."

---

## 3. Every derived route needs a dependency frontier

A derived result should carry not merely a currency number but the semantic frontier that could invalidate it.

Representative examples:

```text
CategoryMembershipProof
    depends on:
      PropertyPaths read by the Category predicate
      Relation kinds consulted by membership
      concept / Formation structure used by the proof

DirectRouteProof
    depends on:
      carrying Relation(s)
      bearer existence / admission
      required-property presence
      Law condition branch identity
      adapter-road provenance

ShortestPathProof
    depends on:
      path edges
      edge-cost expressions
      admissible route families / constraints
      any abstract bounds used to prove alternatives more expensive

EventSubscriptionProof
    depends on:
      downstream read demands
      upstream write effects
      branch provenance
      opaque / unknown sources
```

This frontier is conservative. Extra dependencies waste work; missing dependencies can make a Law deaf and are forbidden.

The `DERIVED_STATE_LEDGER.md` rule therefore grows from:

```text
derived from -> invalidated by -> guarded by test
```

into the more granular implementation target:

```text
derived from
    -> semantic dependency frontier
    -> delta kinds that can invalidate
    -> highest surviving repair source
    -> fallback when frontier completeness is unknown
    -> adversarial invalidation test
```

---

## 4. Incremental Category and Relation-Formation maintenance

A changed Singular should not be re-tested against every Category merely because it changed.

For a Category predicate `C` reading paths `{p1, p2, ...}`:

```text
Delta(S.q)
    -> if q cannot affect C: no work
    -> if abstract interpretation proves C's truth cannot cross its boundary: no work
    -> otherwise: re-evaluate membership of S in C
```

The target cost therefore moves from a global shape such as:

```text
O(B * C)
```

for `B` beings and `C` Categories toward:

```text
O(sum over changed beings of dependent Category predicates)
```

and may become zero for a change whose abstract range cannot affect membership.

The same rule applies recursively to Relations-between-Relations and Formations. A Relation delta should propagate through the dependency cone of the higher structures that actually mention its kind, endpoints, cost, telos, or membership — not through every relevance Formation in the world.

---

## 5. Incremental shortest-path repair

`Relevance::breadthFirstRoutes` is the current bounded shortest-in-hops primitive. The architecture already permits a later Dijkstra-like cost function.

The next requirement is stronger: **do not rerun BFS/Dijkstra over the entire reachable graph when only a small part of the retained proof changed.**

The target is a dynamic shortest-path family: Lifelong Planning A*, D* Lite, dynamic SSSP, or another algorithm chosen for Earthcall's actual route/cost semantics. This document does not mandate one algorithm prematurely.

Conceptually:

```text
retained shortest-path structure
          |
       relevant Delta
          |
          v
identify settled distances / predecessor proofs
that depend on the changed edge or cost expression
          |
          v
re-open only the affected frontier
          |
          v
repair until old/new bounds are consistent
```

Worst case may still touch the whole graph. A mutation can genuinely change every shortest path.

The desired ordinary-case complexity is proportional to the **affected subgraph**, not automatically to all `V + E`.

This is the same rule Law-Direct already proved:

> Do not rediscover what stayed true. Repair what stopped being proved.

---

## 6. Multiple sound paths: incremental priority, not per-frame re-ranking

Suppose a Law has several current routes:

```text
A cost 7
B cost 11
C cost 18
```

If C changes, A and B do not need exact recomputation merely to prove A remains first.

Each candidate route should eventually carry a certificate resembling:

```text
RouteCertificate {
    path / provenance,
    dependency frontier,
    exact cost or conservative cost interval,
    currency,
    proof status
}
```

A priority structure orders current candidates.

When one certificate changes, repair that certificate and only the comparisons whose ordering could change.

Prophetic / OntoMath abstract bounds can sometimes avoid even exact recomputation:

```text
A in [7, 8]
B in [12, 15]
```

If a delta changes A internally but still proves:

```text
A.upper < B.lower
```

then A remains cheaper. The priority proof survives without calculating a new exact scalar.

This preserves Zach's earlier requirement that priorities ultimately become authorable/legible rather than a permanent hidden comparator. The incremental data structure is an execution mechanism; it is not the ontology of value.

---

## 7. The EventBus becomes compiled derived state

Prophetic Rete already asks which writes can possibly matter to which reads.

The next step is to use that knowledge to refine the event topology itself.

Bootstrap form:

```text
broad property / Relation / admission events
        -> conservative Prophetic analysis
        -> downstream work
```

Refined form:

```text
broad complete feed
        -> prove dependency frontier
        -> compile narrow subscription channels
        -> push deltas only to dependent proof frontiers
```

Then the system may recompile those boundaries as authored Laws, Categories, Relations, Formations, or unknown sources change.

This creates a feedback loop:

```text
events feed Prophetic compilation
        ->
Prophetic compilation refines which events need to feed what
        ->
narrower event boundaries reduce future interpretive work
        ->
new semantic structure may refine them again
```

Safety rule:

> A subscription may omit an event only when the analysis proves that event irrelevant. Incomplete analysis restores the broader feed.

The broad complete feed is therefore not deleted. It is the bootstrap/fallback witness.

---

## 8. OntoMath synthesis as delta compilation

A stable Relation/Formation subsystem may have a repeatedly interpreted change path:

```text
Delta input
  -> Relation predicates
  -> Formation membership
  -> route validity
  -> Law relevance
  -> downstream delta
```

Where the structure is sufficiently legible, OntoMath synthesis may compile that subsystem into an equivalent bounded transfer:

```text
Delta input -> Delta relevant outputs
```

The compiled artifact must carry:

- the authored/derived structure it represents;
- its dependency frontier;
- proof/currency witnesses;
- its input/output PropertyPath and Relation domains;
- an interpreter/fallback path;
- tests proving parity under invalidation.

This is a JIT-like optimization of stable relational semantics, **not** a new place to hard-code authored meaning into opaque C++.

---

## 9. Bootstrap: the complete floor must precede the self-refining optimizer

There is a real circularity:

- Prophetic analysis needs events in order to learn/refine event boundaries;
- incremental path repair needs dependency proofs whose production may itself use paths;
- compiled delta opcodes need observability whose delivery may later be compiled.

Earthcall therefore needs a complete, slower bootstrap floor.

Zach's intended anchor is the fundamental opcode framework wrapped around `PropertyPath` and exposed through human-facing First Movers.

A representative boot sequence is:

```text
BOOT 0
fundamental First-Mover opcodes
PropertyPath read/write
Relation form/dissolve/query
Formation membership
complete broad event publication
complete fallback traversal

BOOT 1
Prophetic observes authored reads/writes and dependencies

BOOT 2
build conservative dependency graph/frontiers

BOOT 3
compile narrower event subscriptions and incremental Category maintenance

BOOT 4
incremental route / shortest-path repair

BOOT 5
Law-Direct + PropertyPath-qualified crystallization

BOOT 6
OntoMath-compile stable Relation/Formation delta subsystems
```

BOOT 0 remains available as the epistemic floor. Higher tiers replace its repeated cost only while their proofs remain current.

This is how the architecture self-hosts without trusting an optimizer before the optimizer has evidence.

---

## 10. Complexity target

Let:

- `Delta` = number of semantic changes in one maintenance interval;
- `F_delta` = dependency frontier reached by those changes;
- `V_a, E_a` = vertices/edges in the affected route-repair subgraph;
- `K_a` = candidate paths whose ordering could actually change;
- `R` = actual downstream consequences.

A coarse global maintenance design tends toward work shaped like:

```text
O(all Categories + all retained routes + all path candidates + all subscriptions)
```

per broad invalidation.

The target is closer to:

```text
O(Delta + F_delta + dynamicRepair(V_a, E_a) + K_a + R)
```

with complete fallback when `F_delta` cannot be proved complete.

The critical asymptotic variable is therefore no longer only world size. It is the size of the **relevant changed dependency cone**.

---

## 11. Implementation sequence

### A. Semantic delta vocabulary and frontier ledger
Define the minimal delta shapes already present in EventBus / PropertyPath / Relation generation and map each current derived Formation-Rete structure to the exact delta families that can invalidate it.

### B. Per-proof dependency frontiers
Start with Law-Direct and Slow Adapter roads. Replace coarse "any Relation changed" invalidation only where a narrower frontier is proved complete; keep coarse generation as a fail-open witness.

### C. Incremental Category membership
For authored Category predicates that expose their reads, subscribe only to those paths/Relation kinds and re-evaluate only changed bearers.

### D. Incremental retained-route repair
Preserve route provenance and repair affected road segments rather than discarding an entire current road on unrelated graph changes.

### E. Dynamic shortest-path experiment
Add an adversarial graph test comparing full BFS/Dijkstra recomputation against affected-frontier repair under sparse edge/cost changes. Choose the dynamic algorithm from measurements and authored cost semantics, not fashion.

### F. Incremental route priority
Retain multiple sound candidates, cost certificates/intervals, and a priority structure; update only paths/comparisons touched by relevant deltas.

### G. Self-refining EventBus subscriptions
Compile narrow dependency channels from Prophetic proofs while retaining the broad complete feed as fallback. Adversarially inject an opaque/unknown source and prove the system widens immediately.

### H. OntoMath delta synthesis
Compile one stable Relation/Formation subsystem end-to-end and prove byte/observable parity with the interpreter under relevant and irrelevant changes.

### I. First-Mover bootstrap integration
Make the fundamental opcode/PropertyPath framework the explicit author-facing substrate from which these optimizations derive; do not create a parallel hidden command ontology.

---

## 12. Required adversarial tests

Every implementation rung must include at least these classes of witness:

1. **irrelevant mutation:** unrelated Property/Relation change causes zero derived repair;
2. **relevant mutation:** exactly the dependent proof frontier becomes dirty;
3. **unknown source:** opacity/incompleteness widens to the broad feed/lower tier;
4. **remove/re-form:** a route can become stale and valid again without losing provenance;
5. **Zone/provider swap:** no retained proof survives a world-context change without valid currency;
6. **priority crossing:** path B becomes cheaper than A and ordering flips exactly once;
7. **priority non-crossing:** a changed path's bounds move but cannot overtake the winner, so no global re-rank occurs;
8. **dynamic-path locality:** sparse edge change repairs the affected region and matches full recomputation;
9. **event recompilation parity:** narrow subscription and broad subscription produce identical lawful observables;
10. **bootstrap fallback:** disabling every optimizer still reaches the same lawful result through the complete floor.

No optimization may claim success from timing alone. Count the work that disappeared.

---

## 13. Compact handoff

Formation Rete should now be read as two nested compilers:

```text
FIRST COMPILER: relevance
broad search
  -> semantic roads
  -> retained provenance
  -> Law-Direct bearer/path bindings

SECOND COMPILER: maintenance of relevance
broad change feed
  -> semantic dependency frontiers
  -> incremental proof/path repair
  -> refined event subscriptions
  -> compiled delta-transfer subsystems
```

Both obey one law:

> **Prove before narrowing; preserve what stayed true; repair only what relevant change made uncertain; widen immediately when proof completeness is lost.**
