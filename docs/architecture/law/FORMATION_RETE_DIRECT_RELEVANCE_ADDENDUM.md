# Formation Rete Addendum — Concept-Singular Binding, Bidirectional Goal Traversal, and Direct Relevance Crystallization

**Status:** Architectural addendum, 2026-09-16. This document extends `FORMATION_RETE.md` §§1–4, §6, §8 and §9. It does not replace the rung ladder. It records a refinement Zach articulated after the 2026-09-16 Formation Rete work: the variable-like role of Categories and concept-Singulars, the complexity consequences of bidirectional / Magic-Set-style traversal, and the final step in which a proved relevance route may crystallize into a direct Relation from a Law to the Singular/property address it can actually affect.

**Origin and attribution.** The core ideas in this addendum are Zach's: (1) a Category or concept-Singular can play the role ordinary rule engines give to variables; (2) search should be constrained from both ends so the goals meet in the middle rather than fan out from one side; (3) once category/Relation structure has mapped a Law to the specific Singular or property it can affect, a **direct relevance Relation** should be made only when that shortcut is **provably sound**; and (4) the shortcut-validation phase belongs after Prophetic Rete has incorporated the changes relevant to the present Moment, so the hot path does not rediscover whether the route changed.

**Recorded and formalized by:** **GPT-5.6 Sol (OpenAI)**, in conversation with Zach on 2026-09-16. Zach originated the architectural refinements above; Sol compared them against indexed Beta complexity, formalized the meet-in-the-middle and amortized-direct-edge consequences, and connected them to the existing Prophetic filter, Slow Adapter, Magic Sets framing, and derived-state invalidation discipline.

**Companions:** `FORMATION_RETE.md`, `PROPHETIC_RETE.md` §2, `B-time Rete.md`, `DERIVED_STATE_LEDGER.md`, `AUTHORED_CATEGORIES.md`, `PRIMARY_AND_SUB_RELATIONS.md`, `PROPERTY_ADDRESSING_IN_FORMATION_RETE.md`, `../ontology/PROPERTY_AS_PREDICATION_NOT_BEING.md`.

---

## 1. Correction to the phrase "Earthcall has no free variables"

`FORMATION_RETE.md` §1.1 correctly observes that the **current condition syntax** does not contain a classical anonymous free-variable language. A qualified root resolves to a named being, `@event.subject`, `@event.object`, or `@world`; the old `ForAnyPair` / `ForAllPair` kinds were retired in favour of Relations.

That implementation fact should not be mistaken for the stronger architectural claim that Earthcall has no native representation capable of doing the work variables do.

Earthcall already has richer ontological pieces:

- a **Category** is the domain of possible bindings: the set/Formation of beings that could occupy a conceptual role;
- a **concept-Singular** is a variable-like or pattern-like position that anticipates a Singular with specifically shaped properties/Relations;
- a **Relation** constrains how two conceptual positions, or a conceptual position and a concrete Singular, may coexist;
- a **Formation** can embody the joined relational pattern formed by several such constraints;
- a concrete **Singular** satisfying the concept is the binding/substitution.

In conventional rule notation one might write:

```text
x : ChessPiece
y : Square
occupies(x, y)
```

The Earthcall-native form need not introduce an invisible interpreter-local `x` and `y`. It can instead represent conceptual positions and their constraints as beings/Relations, then resolve those positions to concrete Singulars.

This is not merely a change of notation. A classical Beta memory stores opaque tuples such as `(x = pawn.17, y = square.e4)`. Earthcall can potentially represent the same semantic achievement as a legible Formation/Relation structure whose conceptual positions and concrete fulfillments remain addressable in-world.

**Current limitation:** concept-Singular resolution is not yet the finished runtime mechanism. `FORMATION_RETE.md` §3.4 already records that the Formation half of rung 2 is blocked on the concept-Singular bridge. This addendum specifies the computational destination without pretending that bridge has landed.

---

## 2. Complexity model: Beta joins versus Category / concept-Singular resolution

Let:

- `N` = total Singulars in the relevant world;
- `A`, `B` = candidate counts in two Categories;
- `E_R` = number of extant relevant Relations of kind `R`;
- `M` = number of final satisfying bindings/results;
- `d` = graph distance between constrained endpoints;
- `b_rel` = effective branching factor **after** Category, Relation-kind, Property, Prophetic and other relevance restrictions.

Consider the conceptual pattern:

```text
Concept X ∈ Category A
Concept Y ∈ Category B
R(X, Y)
```

### 2.1 Naive tuple-space join

A naive join tests every candidate pair:

```text
O(A * B)
```

With `k` unconstrained variables each ranging over `N` beings, the raw possibility space can reach:

```text
O(N^k)
```

This is the combinatorial space Formation Rete is explicitly trying not to recreate as hidden machinery.

### 2.2 Proper indexed Beta is not the naive cross-product

A competent Rete engine does not normally rescan the full Cartesian product on every change. Alpha memories, Beta memories and hash/indexed shared variables can make an incremental update approximately proportional to the matching partial bindings reached by the changed fact.

For a simple indexed binary join, the useful comparison is therefore not "Formation O(E), Beta O(AB)" as a universal law. A good indexed Beta can often approach an output-sensitive form such as:

```text
O(relevant indexed input + M)
```

and a single well-indexed update may be close to:

```text
O(1 + k)
```

where `k` is the number of compatible partial tuples reached by the lookup.

Formation Rete should be compared with that strong version, not a straw man.

### 2.3 Formation traversal changes the natural parameter

When the semantic relation already exists as an authored `Relation`, Earthcall does not need to rediscover that relation by testing all possible pairs. It traverses the edges that actually exist.

A category-constrained relational query becomes roughly:

```text
O(V_relevant + E_relevant + M)
```

and, once one endpoint is bound:

```text
O(deg_R(bound endpoint) + M)
```

rather than `O(A * B)`.

For example, if 1,000 Persons and 2,000 Objects admit only 300 actual `owns` Relations, the semantic graph contains 300 relevant edges, not two million meaningful pairs. Traversal can be proportional to those 300 edges rather than manufacturing a two-million-pair question merely to rediscover which pairs are connected.

The advantage is greatest in the regime Earthcall is designed to inhabit:

```text
E_relevant << N^2
```

— sparse authored relation structure with semantically meaningful edges.

### 2.4 Categories make resolution output-sensitive earlier

If a concept's domain is already indexed by Category, resolving the first conceptual position need not scan `N` beings. The lookup is approximately:

```text
O(1 + |Category|)
```

for ordinary indexed access plus enumeration of the members that could actually bind.

Subsequent concept resolution can then proceed relationally. If `X` is bound and `Y` must satisfy `R(X,Y)`, the next step is proportional to `deg_R(X)`, not to the size of every possible `Y` in the world.

The pattern therefore changes from:

```text
|X-domain| * |Y-domain| * |Z-domain| * ...
```

toward:

```text
Category lookup
→ adjacency traversal
→ adjacency traversal
→ actual results
```

when the world has already reified the relevant constraints.

### 2.5 Worst case is still worst case

Formation Rete does **not** abolish combinatorics. If every member of `A` relates to every member of `B`, then:

```text
E_R = A * B
```

and traversing all answers is itself `O(A * B)`. Likewise, if a query genuinely has `N^k` satisfying bindings, no engine can enumerate them in `o(N^k)` time.

The gain is not "graph traversal beats mathematics." The gain is:

> **do not pay for combinations the ontology already says do not exist.**

---

## 3. Bidirectional Magic-Set traversal: both goals constrain the road

`FORMATION_RETE.md` §4C already identifies Magic Sets as the frontier technique that combines backward goal restriction with forward incremental evaluation. Zach's refinement is stronger and more operational: **when both ends of a relevance problem are constrained, search from both ends and let the constrained frontiers meet.**

Suppose a Law knows:

- the Category / concept shape it can govern;
- the Relation kinds its condition admits;
- the property name/path it ultimately reads or writes;
- Prophetic ranges proving which value domains can matter.

And the candidate side knows:

- which Categories the Singular inhabits;
- which Relations touch it;
- which properties it actually exposes;
- which paths/ranges can satisfy the Law.

Then relevance discovery is not a one-sided BFS from the Law through every available branch. The Law-side goal and the Singular/property-side goal can both push bindings inward.

For a roughly tree-like search with effective branching factor `b_rel` and distance `d`, a one-sided search has the familiar bad shape:

```text
O(b_rel^d)
```

while ideal bidirectional meet-in-the-middle exploration is approximately:

```text
O(2 * b_rel^(d/2)) = O(b_rel^(d/2))
```

This is not a blanket bound for arbitrary cyclic graphs; it is the standard intuition for why two constrained frontiers can be exponentially smaller in depth than one unconstrained frontier. In Earthcall the important multiplier is that `b_rel` is not the raw graph degree: Category membership, Relation kinds, Property requirements, Prophetic impossibility proofs and previously retained relevance Relations all shrink the effective frontier first.

The operational doctrine is therefore:

```text
Law goal / concept constraints
        ↓
restricted forward frontier
        ↓
        meet
        ↑
restricted reverse frontier
        ↑
Singular / Property goal
```

The search should use the most selective known constraints from **both** sides before expanding a broad Relation family.

### 3.1 Multi-way joins should intersect, not rebuild pairwise products

Where several conceptual branches meet, the engine should prefer worst-case-optimal / trie-style intersection over a chain of pairwise Cartesian joins. This is the computational counterpart to `FORMATION_RETE.md` §4D: if three Relation families already expose the candidate identities, intersect those identity sets directly instead of materializing intermediate pair tuples that exist only to be joined again.

This remains compatible with Beta as a local implementation technique. The architectural rule is simply that Beta tuple materialization is not privileged when the ontology already exposes a more selective graph/intersection route.

---

## 4. From route discovery to route crystallization

Bidirectional traversal is still search. Zach's second refinement is that a repeatedly rediscovered, **proved** relevance route should not remain a route forever.

Suppose the slow / proof path establishes:

```text
Law L
  → Category C
  → Relation-family R
  → Singular S
  → property path P
```

If the engine can prove that `S.P` is a sound downstream relevance destination for `L` under the current declared premises, it may crystallize the route into a direct relevance Relation conceptually equivalent to:

```text
Law L ── relevant-to ──> Singular S
                     + PropertyPath P
```

The next hot-path write no longer needs to traverse `L → C → R → S → P`. It follows the proved shortcut to the bearer, with the PropertyPath identifying the relevant predication on that bearer.

### 4.1 What exists literally today — and why that boundary is deliberate

A `Law` is a `Singular`, so it can stand in an ordinary first-class Relation.

A `Property` deliberately **does not inherit `Singular`**. This is not an implementation gap awaiting future reification. Per `../ontology/PROPERTY_AS_PREDICATION_NOT_BEING.md`, a Property is a legible predication of one bearer and a bridge into machine-level state, not another being. Reifying every Property would both confuse attribute with substance and reopen an infinite regress of Properties-of-Properties.

The ontologically correct representation is therefore:

```text
Law L ── relevant-to ──> Singular S
                     + authored/derived PropertyPath P as the address within S
```

or an equivalent first-class in-world structure that preserves the Singular endpoint and the property-path qualification without inventing a Property-being. `PROPERTY_ADDRESSING_IN_FORMATION_RETE.md` records the execution rule compactly: **Relations join beings; Properties disclose them.**

### 4.2 A direct relevance edge is a derived proof object, not a guess

The shortcut may be created only when its relevance is **provably sound** under Prophetic Rete's one rule:

> an optimization may be too generous; it may never be too narrow.

A false-positive relevance edge merely causes extra evaluation. A missing edge can make a Law deaf.

Therefore direct relevance crystallization must either:

1. be complete for the dependency slice it claims to replace; or
2. remain a **proposal** with the current complete mechanism behind it.

No heuristic similarity edge, stale Category membership, approximate HNSW neighbour, or partially explored path earns the right to become the sole route by which a Law can hear.

---

## 5. Ordering with Prophetic Rete: prove first, then trust the shortcut

Zach's scheduling requirement is that direct relevance crystallization run **after Prophetic Rete has incorporated the changes relevant to the present Moment**. The purpose is not merely sequencing aesthetics. It lets the shortcut consume the same current Law/write/read knowledge that would otherwise force the hot path to ask repeatedly whether the relevance relation changed.

The intended cycle is:

```text
1. Change feeds arrive
   - Law text revision
   - world structural revision
   - Relation generation
   - relevant Property-value changes

2. Prophetic Rete updates / invalidates its possibility-space conclusions
   - what can read this property?
   - what can write this property?
   - which ranges can possibly intersect?
   - which branches are proved impossible?

3. Formation relevance maintenance runs against those current conclusions
   - Category/concept narrowing
   - bidirectional / Magic-Set traversal
   - retained slow-adapter roads
   - soundness proof for any candidate shortcut

4. Sound routes may crystallize into direct Law → Singular(+PropertyPath) relevance Relations

5. The hot path follows current direct edges
   - no category traversal
   - no generic join reconstruction
   - condition evaluation still decides truth unless the stronger truth claim itself is separately proved
```

The important distinction is between **checking validity** and **rediscovering relevance**. The hot path may still perform a constant-time revision/dependency stamp check; it should not have to walk the graph merely to ask whether the shortcut is stale.

---

## 6. Invalidation: "after Prophetic" does not mean "valid forever"

Running after Prophetic Rete does not remove the derived-state problem. A direct edge is a cache of a proof and must declare its dependency cone.

At minimum, a crystallized relevance edge may depend on some subset of:

- `Law::textRevision()` — the reading/writing Law changed;
- `Universe::structuralRevision()` — a relevant being/category membership/property vocabulary changed;
- `Universe::relationGeneration()` — the Relation graph changed;
- the relevant Property-value feed — **only if the proof itself depends on current values rather than invariant ranges**;
- concept-Singular / Category definition revisions once those become explicit first-class sources.

The derived-state ledger's rule applies literally:

```text
derived from → invalidated by → guarded by test
```

A future implementation should record this triple beside the shortcut structure and add a row to `DERIVED_STATE_LEDGER.md`.

### 6.1 Dependency-cone invalidation, not world-wide panic

The mature form should not invalidate every direct edge because *something* changed somewhere in the world.

If:

```text
Law L ─relevant-to→ pawn.17.position
```

was proved from:

- `L`'s text;
- `pawn.17`'s membership in `category.chess.piece`;
- one `instance-of` edge;
- the existence/type of `position`;

then `sun.color` changing should not invalidate the edge.

Prophetic Rete already asks which property/read/write families can possibly matter. Formation Rete should use that answer to maintain a **dependency cone** per crystallized shortcut. The ideal invalidation cost is proportional to the shortcuts whose premises actually changed, not to the total shortcut population.

### 6.2 Primary versus sub-Relation semantics

A direct relevance shortcut should not casually become an eternal primary Relation if its truth is derived from premises that may cease to hold. `PRIMARY_AND_SUB_RELATIONS.md` governs this distinction.

The likely shape is:

- the underlying meaningful authored Relations remain primary where the ontology says they are primary;
- the optimized `relevant-to` route is a derived/sub-Relation whose provenance names the premises that justified it;
- when a premise becomes **provably impossible**, the derived route may dissolve/re-kind according to the primary/sub-Relation doctrine;
- when merely uncertain or temporarily less optimal, it must fall back/deprioritize rather than silently disappear as the sole correctness path.

The exact Relation kind/provenance representation remains an implementation decision, but the soundness direction is fixed.

---

## 7. Hot-path complexity after crystallization

Let `r` be the number of downstream destinations that genuinely need to hear one completed write.

Route discovery may cost, depending on the graph and constraints:

```text
O(V_relevant + E_relevant)
```

or, in the ideal bidirectional-depth intuition:

```text
O(b_rel^(d/2))
```

but if that result survives for `T` Moments, the discovery cost amortizes to:

```text
O(discovery / T)
```

per Moment.

After a sound direct edge exists, finding the destination can be constant-time/indexed and propagation becomes output-sensitive:

```text
O(r)
```

plus constant-time currency checks.

That is the meaningful lower bound: if `r` downstream evaluations genuinely must be informed, the engine must perform at least `r` acts of propagation or an equivalent batched operation.

The long-run architecture therefore aims for:

```text
rare bounded discovery / repair
        +
work proportional to actual relevant consequences
```

rather than repeated reconstruction of the entire candidate space.

---

## 8. Comparison with strong indexed Beta

This addendum does **not** claim that Formation Rete has universally better asymptotic complexity than every good Beta implementation.

A strong indexed Beta network can also make one changed fact reach only matching partial tuples, often approaching `O(1 + r)` lookup/propagation for a selective indexed join. The difference is architectural:

**Indexed Beta**

```text
fact
→ alpha memory
→ partial tuple memory
→ indexed Beta join
→ larger tuple memory
→ terminal
```

**Prophetic + Formation with crystallized relevance**

```text
Law/action write
→ pre-proved relevance Relation
→ relevant Singular + PropertyPath
→ live condition frontier
```

Both may approach the same best asymptotic hot-path class. Formation Rete's possible advantage is that the semantic join structure already exists in the authored world and can be reused, rearranged, governed and progressively specialized rather than duplicated as opaque tuple memory.

The deepest optimization is therefore not "traversal instead of joins." It is:

> **use traversal to discover relevance, Prophetic Rete to prove which pruning/shortcuts are safe, and then materialize the result so the runtime no longer performs that join at all.**

This is partial evaluation / dynamic specialization of the Law graph expressed through Earthcall's own ontology.

---

## 9. Relation to the old Prophetic §9 "ActionNode → Beta back-pointers"

`PROPHETIC_RETE.md` §5 still names its largest unfinished implementation piece as **ActionNode → Beta back-pointers**: after an ActionNode computes its value, jump immediately to the relevant downstream Beta criteria instead of paying the generic assert/propagate/drain round trip.

Formation Rete changes the literal target but preserves the idea.

The modernized form is:

```text
ActionNode / completed property write
        ↓
Prophetic write→read relevance proof
        ↓
crystallized Formation relevance edge
        ↓
relevant Law / Singular / PropertyPath frontier
        ↓
live condition decision
```

The invariant is the same as Zach's original B-Time note: **do not rediscover at runtime what the authored Law graph already lets the engine know in advance.**

The obsolete part is only the assumption that a classical Beta chain is necessarily the destination. Formation Rete explicitly replaced the Beta cross-product as Earthcall's fundamental join representation. Existing Beta machinery may remain as a local incremental-discrimination implementation detail where it is useful; the direct relevance graph should target the semantic condition frontier, not canonize one internal node type.

---

## 10. Proposed implementation ladder — not yet authorized as code

This addendum is architecture, not a claim that the following has landed.

1. **Concept-resolution semantics.** Specify the exact in-world Relation vocabulary by which a concept-Singular is fulfilled/bound by a concrete Singular. Do not invent a new C++ kind.
2. **Bidirectional route query.** Extend the existing relevance traversal so it can accept constraints from both the Law side and the destination Category/Singular/property side, with bounded work and a complete fallback.
3. **Multi-way intersection.** For several Relation constraints, intersect candidate identities directly rather than materializing pairwise tuple products when the graph/index exposes a better route.
4. **Write→read proof graph.** Extend Prophetic derived state from global read/write summaries to explicit writer→reader relevance candidates, still fail-open on opaque/incomplete analysis.
5. **Runtime write-result seam.** Surface enough provenance at a successful ActionNode/property write to identify the writer, concrete destination, PropertyPath and completed value/range without reconstructing them downstream.
6. **Crystallized relevance record.** Reify or derive the Law→Singular(+PropertyPath) shortcut with provenance and the declared invalidation triple.
7. **Currency before use.** Make current revision/dependency stamps a constant-time gate. Stale/uncertain means fall back, never "probably still valid."
8. **Parity test against the correctness floor.** Run direct relevance on/off against the same worlds and assert identical reached beings, firings and releases. Mutation-test every invalidation input.
9. **Measure before default-on.** The Sep 16 Slow Adapter correctly shipped off when it lost to the existing chess index. Direct relevance earns default-on status only where measurements show a real win without weakening the correctness floor.

---

## 11. The architectural claim, compressed

Classical Beta asks:

> which tuples of unknowns satisfy this join?

Formation Rete can ask:

> which authored conceptual positions can these Singulars inhabit, and which already-existing Relations constrain their coexistence?

Magic-Set-style bidirectional traversal then asks:

> given constraints from both the Law and the candidate/property side, where can the two relevance frontiers possibly meet?

Prophetic Rete asks:

> which of those roads are impossible before runtime, and which write→read relationships are safe to retain?

Direct relevance crystallization finally asks:

> if that road has already been proved, why search it again?

The intended progression is therefore:

```text
possible tuple space
→ Category/concept domain
→ constrained Relation subgraph
→ bidirectional meeting frontier
→ proved relevance route
→ direct Law → Singular(+PropertyPath) edge
→ work proportional to actual consequences
```

The ambition is not to make search magically disappear. It is to **pay discovery when knowledge is genuinely absent, preserve the knowledge when it becomes sound, and stop paying discovery once the world already knows the answer.**

---

*Recorded and formalized by GPT-5.6 Sol (OpenAI) from Zach's Formation/Prophetic Rete refinements, 2026-09-16. Zach is the originator of the architectural ideas identified above; Sol's contribution is their complexity analysis, synthesis with indexed Beta/Magic Sets, and integration with the existing Earthcall documentation and derived-state discipline.*
