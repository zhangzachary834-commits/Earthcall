# Property Addressing in Formation Rete

**Status:** Architectural companion, 2026-09-16.

**Origin:** Zach's clarification that `Property` is deliberately not a `Singular`, because a Property is a bridge into one being's legible state rather than another being. This note applies that ontology to Formation Rete, Prophetic Rete, and the direct-relevance shortcut proposed in `FORMATION_RETE_DIRECT_RELEVANCE_ADDENDUM.md`.

**Companions:** `../ontology/PROPERTY_AS_PREDICATION_NOT_BEING.md`, `FORMATION_RETE.md`, `FORMATION_RETE_DIRECT_RELEVANCE_ADDENDUM.md`, `PROPHETIC_RETE.md`, `DERIVED_STATE_LEDGER.md`.

---

## 1. The correction

The direct-relevance design must not drift into this:

```text
Law -> Property-as-Singular
```

because a Property is not an ontological endpoint.

The correct shape is:

```text
Law -> relevant Singular
       + PropertyPath qualifier
```

or, at Action granularity:

```text
Law / stable Action provenance
        |
        | relevant-to
        v
   Singular S
        |
        `- PropertyPath P
```

The Relation answers **which being** is relevant. The PropertyPath answers **which aspect of that being** is relevant.

This is not merely a concession to today's C++ type hierarchy. It is the intended ontology. `Property.hpp` remaining outside `Singular` is deliberate.

---

## 2. Why Formation Rete should care

Formation Rete moves Earthcall away from reconstructing hidden tuple joins and toward traversing authored/reified structure among beings. If optimization then reifies every Property as a graph node, it quietly recreates a second hidden ontology at a finer grain:

```text
being -> property node -> property-of-property node -> ...
```

That would undo two of Formation Rete's advantages:

1. **graph sparsity** — one being with twenty Properties would become twenty-one graph nodes before any actual Relation is counted;
2. **semantic fidelity** — a statement about an aspect of a being would be represented as a statement about a second being.

Formation Rete should therefore use the ontology to resolve **bearers** and use PropertyPath to address **predications on those bearers**.

---

## 3. Revised direct-relevance pipeline

The direct-relevance addendum proposed a progression:

```text
possible tuple space
-> Category/concept domain
-> constrained Relation subgraph
-> bidirectional meeting frontier
-> proved relevance route
-> direct Law -> destination edge
-> work proportional to actual consequences
```

The destination edge is now specified more precisely:

```text
proved relevance route
        |
        v
Law L --relevant-to--> Singular S
          qualifier: PropertyPath P
```

For example:

```text
law.chess.highlight-threat
    --relevant-to--> pawn.17
       path = "position"
```

or:

```text
law.temperature.warning
    --relevant-to--> reactor.3
       path = "thermal.core.temperature"
```

The path is not another endpoint. It is part of the reason/qualification under which `L` is relevant to `S`.

If several paths on the same bearer are relevant, the representation may carry a compact set of PropertyPaths or several property-specific sub-Relations. That representation choice must remain consistent with `PRIMARY_AND_SUB_RELATIONS.md`; it must not mint Property-beings for convenience.

---

## 4. Prophetic Rete supplies predicate relevance; Formation Rete supplies bearer relevance

The clean division is:

### Prophetic Rete

Reads Law structure and asks:

```text
Which PropertyPaths could possibly matter?
Which Action outputs can intersect which Condition demands?
Which paths are provably impossible?
```

### Formation Rete

Reads ontology/relevance structure and asks:

```text
Which Singulars could instantiate those paths?
Which Categories/concepts constrain the candidates?
Which Relations connect the relevant bearers?
Which route reaches them with the least relevant fan-out?
```

### Property layer

Once a bearer is reached:

```text
Resolve PropertyPath P on Singular S
read/write/evaluate the concrete aspect
```

So the strongest direct path is not:

```text
ActionNode -> Beta node
```

and not:

```text
ActionNode -> Property being
```

but:

```text
stable Action provenance
    -> Prophetic write/read relevance
    -> Formation relevance Relation
    -> Singular bearer
    -> PropertyPath resolution
    -> live Condition decision
```

---

## 5. Complexity consequence

Not making Properties graph nodes is also computationally useful.

Let:

- `N` = Singular count;
- `p` = average number of Properties per Singular;
- `E` = relevant ontological Relation count.

A graph that naïvely reifies each Property as a node begins with approximately:

```text
N + N*p
```

nodes before representing the Relations among the original beings, and it introduces maintenance edges between bearers and their Property nodes.

Keeping Properties as addressable predications instead leaves the ontological graph near:

```text
N beings + E meaningful Relations
```

while Property lookup uses the specialized PropertyPath machinery that already exists for exactly this purpose.

The direct relevance route therefore keeps the graph sparse and lets the hot path approach:

```text
O(r + path-resolution-cost)
```

where `r` is the number of genuinely relevant destination bearers/paths. With interned/pre-resolved paths, path resolution can itself be reduced toward a tiny bounded lookup rather than graph traversal.

The important asymptotic point is that **the number of attributes a being exposes does not automatically multiply the ontology's vertex count**.

---

## 6. Concept-Singular binding stays bearer-first

The variable-like interpretation of concept-Singulars also follows this distinction.

A concept may require:

```text
mass > 10
color == red
Related(instance-of, category.vehicle)
```

Resolution should proceed:

```text
concept/category constraints
       -> candidate Singulars
       -> Relation constraints
       -> PropertyPath predicates on each candidate bearer
       -> accepted binding
```

not:

```text
concept
  -> search for a mass-being
  -> search for a red-being
  -> somehow reconstruct the original bearer
```

Properties constrain a possible binding. They are not things being bound.

This keeps the Earthcall-native analogue of variables coherent:

- Category = binding domain;
- concept-Singular = conceptual binding position;
- concrete Singular = binding value;
- Relation = constraint between binding positions/bearers;
- Property predicate = constraint **on** one binding value.

---

## 7. Invalidation and soundness

A direct relevance Relation qualified by PropertyPath is derived structure unless authored directly by a Person. Therefore the Derived-State Ledger discipline applies.

For every such shortcut, declare at minimum:

```text
derived from:
  Law text/revision
  relevant Category/concept structure
  relevant Relation premises
  destination Singular identity
  PropertyPath named by the Law
  Prophetic write/read compatibility proof

invalidated by:
  any dependency above changing in a way that can affect the proof

fallback:
  broader Formation route / ordinary reactive path / complete sweep

guarded by:
  parity + mutation tests that deliberately invalidate each premise
```

The goal is not to ask every frame whether the entire world changed. The goal is for Prophetic/Formation maintenance to update the shortcut's dependency cone when relevant changes occur, so an unchanged shortcut is traversed directly.

A stale shortcut must never become the only road to a Law. If currency cannot be proved, downgrade it to a proposal and fall back.

---

## 8. Relation semantics

Because the endpoints remain Singulars, direct relevance can fit the primary/sub-Relation architecture without inventing a new ontological category for Properties.

A possible shape is:

```text
Primary Relation:
    Law L <-> Singular S

Sub-Relation / relevance qualification:
    active-relevance
    targetPath = "position.y"
    premise = <ConditionModel / proof provenance>
```

This is illustrative, not yet an implementation decision. `PRIMARY_AND_SUB_RELATIONS.md` still governs how active, historical, or deprioritized sub-Relations are represented.

The invariant is simpler:

> **The interaction is between the Law and the bearer. The PropertyPath states why or where that interaction matters.**

---

## 9. Implementation guidance

When implementing direct relevance:

1. Do not make `Property` inherit `Singular` for graph convenience.
2. Do not create one graph node per PropertyPath.
3. Give the write side stable provenance (Law + action-tree identity), not raw pointers that die on recompile.
4. Let Prophetic analysis map write effects to possible read demands.
5. Let Formation/category/concept traversal resolve candidate **bearers**.
6. Store/carry the relevant PropertyPath alongside the bearer relation.
7. On a concrete write, route to the proved bearer/path frontier.
8. Still make the live condition decide truth; relevance is candidacy, not truth.
9. Fail open whenever a proof, route, or destination is stale or incomplete.
10. Measure before replacing the generic path; preserve inactive scaffolding when a route is correct but not yet faster.

---

## 10. The compact rule for future agents

If you are tempted to write:

```text
Relation(Law, Property)
```

ask whether you really mean:

```text
Relation(Law, bearer Singular)
qualified by PropertyPath
```

In current Earthcall, the second is the default correct answer.

The ontology document states the reason in one line:

> **Relations join beings. Properties disclose them.**

Formation Rete should preserve that boundary even while optimizing across it.

---

*Recorded from Zach's 2026-09-16 clarification and applied to the direct-relevance architecture. This document narrows representation; it does not alter the Prophetic IMPOSSIBLE-only rule, the Formation sweep correctness floor, or the current concept-Singular implementation status.*
