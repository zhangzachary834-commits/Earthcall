# Formation Rete — Tiered Relevance Ladder and Law-Direct Traversal

**Status:** Architectural addendum, 2026-09-16; **implementation/state updated 2026-09-21.** The first executable Law-Direct rung is now built and measured. Zach's 2026-09-21 next direction — relevant-change-only maintenance, incremental path repair, self-refining event subscriptions, and incremental route priority — is specified in `FORMATION_RETE_INCREMENTAL_MAINTENANCE_AND_SELF_REFINING_EVENTS.md`.

**Origin and attribution.** The architecture in this note is Zach's, stated and clarified on 2026-09-16: Formation Rete is a **tiered relevance system with fallbacks**; similarity is a low-level worst-case discovery aid rather than the governing idea; the complete sweep remains available to over-approximate when stronger routes are ambiguous; Prophetic-Rete-style abstract interpretation can construct an ahead-of-time graph of relevance among Laws, Singulars, Properties, Relations, and Formations; and the **ultimate optimization target is a reified direct Law-to-relevant-Singular(+PropertyPath) route**. The higher layers exist to discover, prove, rank, retain, and repair those direct routes. The slow adapter builds those layers on its independent clock; the hot path consumes the highest sound/current layer that is ready.

**Recorded and formalized by:** GPT-5.6 Sol (OpenAI), 2026-09-16. Sol's contribution here is the explicit ladder, invariants, complexity framing, and integration with the existing Prophetic, Derived-State, Property-predication, and Hierarchy-of-Joys documents. The originating design choices above are Zach's.

**Companions:** `FORMATION_RETE.md`, `FORMATION_RETE_DIRECT_RELEVANCE_ADDENDUM.md`, `FORMATION_RETE_NEURAL_PLASTICITY_ADDENDUM.md`, `FORMATION_RETE_INCREMENTAL_MAINTENANCE_AND_SELF_REFINING_EVENTS.md`, `PROPERTY_ADDRESSING_IN_FORMATION_RETE.md`, `PROPHETIC_RETE.md`, `DERIVED_STATE_LEDGER.md`, `../ontology/HIERARCHY_OF_JOYS.md`, `../ontology/PROPERTY_AS_PREDICATION_NOT_BEING.md`, `../../Analysis/LAW_DIRECT_TRAVERSAL_COMPLEXITY_AND_CI_RESULTS_2026-09-19.md`.

---

## 0. The architecture in one sentence

> **Formation Rete climbs from broad, safe relevance discovery toward increasingly direct proved routes; the hot path always uses the highest current sound tier available, while lower tiers remain as provenance, repair paths, and correctness fallbacks.**

The final destination is not a permanently elaborate hierarchy for its own sake. It is:

```text
Law / stable Action provenance
        |
        | proved relevant-to
        v
   Singular bearer
        |
        `- PropertyPath qualifier
```

Everything below that direct edge exists to answer one of four questions:

1. **Could this possibly matter?** — Prophetic abstract interpretation.
2. **Where should I look?** — Categories, concepts, Relation families, similarity groups, telos orderings.
3. **Which route is best and sound enough to retain?** — Formation/Relation-of-Relation structure plus slow adaptation.
4. **Can I now skip search entirely?** — direct Law relevance crystallization.

---

## 1. This is a ladder, not a flat bag of heuristics

Earlier descriptions can be misread as if Formation Rete had several peer mechanisms — similarity, category traversal, Relation traversal, slow-adapter routes, and direct Law edges — and the engine merely chose one.

Zach's clarification is stronger: **they form a dependency ladder.** Higher tiers are constructed from, justified by, or repaired through lower tiers.

A representative ladder is:

```text
Tier 6  direct Law -> Singular(+PropertyPath) relevance route
          ^
          | proved/crystallized from
Tier 5  Law -> relevant Relation / Relation-Formation / concept route
          ^
          | organized from
Tier 4  Relations-between-relevant-Relations / Formation route structure
          ^
          | narrowed by
Tier 3  Category / concept-Singular / authored relevance structure
          ^
          | aided by
Tier 2  cross-index graph traversal and retained discovered roads
          ^
          | aided by
Tier 1  coherent similarity neighbourhoods / quantitative overlap
          ^
          | correctness floor beneath uncertain narrowing
Tier 0  complete over-approximating sweep / full eligible search
```

This ordering is **conceptual**, not a mandate that the implementation literally number or instantiate seven engine classes. Refusal 1 still applies. A future implementation may merge adjacent tiers or represent several with ordinary Relations/Formations.

The critical invariant is monotone operational preference:

> **Use the highest tier whose soundness and currency are established. If it is unavailable, incomplete, stale, or unproved, fall to the highest lower tier that is safe.**

Not:

> choose whichever heuristic seems fastest and hope it is right.

---

## 2. What each tier means

### Tier 0 — Complete sweep: the right to be uncertain

The sweep is the complete over-approximating floor. It exists so every higher structure is allowed to say:

```text
I do not know enough to narrow safely.
```

and still preserve lawful hearing.

This remains the final fallback when:

- no useful Category/concept route is known;
- a relevance route is stale;
- Prophetic analysis is incomplete or opaque;
- similarity evidence is too weak to narrow;
- a slow-adapter road has not converged;
- a direct Law shortcut cannot prove currency.

The sweep is not the preferred steady-state route. It is the **completeness witness** licensing every faster tier above it.

### Tier 1 — Similarity and overlap: broad discovery when semantics are weak

Similarity is intentionally low in the ladder.

It is useful when the engine lacks a stronger semantic road and needs to discover likely neighbourhoods by:

- quantitative similarity;
- property overlap;
- kind overlap;
- other locally coherent similarity indices.

But similarity is not relevance. A nearby or similar being can be irrelevant to a Law, while a distant unlike being connected by an authored Relation can be exactly relevant.

Therefore similarity primarily serves **discovery and repair**. It may propose roads that higher tiers later turn into explicit relevance structure.

Each similarity family keeps its own coherent ordering. Formation Rete must not invent one universal metric merely to make all traversals look alike.

### Tier 2 — Cross-index / Singular-graph traversal

The slow adapter can search more broadly across:

- Singular adjacency;
- Relation families;
- Property-bearing structures;
- Category/concept membership;
- similarity indices;
- previously discovered relevance roads.

This is where bounded BFS/Dijkstra/best-first work can bridge one local index into another.

A discovered cross-index road is valuable because the next search no longer starts from raw similarity. It starts from a semantic bridge retained by the world or by derived state.

### Tier 3 — Category, concept-Singular, and authored relevance structure

A Category narrows the **domain of possible bearers**. A concept-Singular narrows the **shape of a possible binding**. Authored relevance Relations can name a useful bridge directly.

This is already semantically stronger than similarity because it answers questions like:

```text
Which beings can inhabit this role?
Which concept does this branch anticipate?
Which authored Relation says these structures matter to one another?
```

rather than only:

```text
Which things look alike under one metric?
```

Where Category/concept structure gives a sound candidate domain, traversal should prefer it over broad similarity exploration.

### Tier 4 — Relations among relevant Relations / Formation route structure

Relations are themselves Singulars. Therefore relevance discovered between lower-level structures can itself become structured:

```text
Relation R1 --relevant-to--> Relation R2
Relation R2 --member-of--> relevance Formation F
Formation F --routes-for--> Law L
```

This tier is not decoration. It is an **index of provenances and roads**: a way to say not only that `L` reaches `S`, but how the known semantic roads are organized and which families of roads should be tried first.

When two route subkinds overlap — for example:

- a Relation pointing to a Formation-category Relation;
- a Formation of similarity Relations;
- a Person-authored relevance Relation;
- a cross-index road discovered by the slow adapter;

—the adapter's job includes learning which route family is the better starting point under the current authored objective and measured cost.

The priority must eventually be legible as authored Properties/Relations rather than hidden C++ policy (§7), but the first implementation may hold that exposure until the execution model is stable.

### Tier 5 — Law to relevance structure

A Law is itself a Singular, so it can stand in Relations to the relevance structures that govern its search.

Instead of facts blindly flowing through a generic network, a Law can begin from its own semantic structure and traverse toward the things its conditions/actions can care about:

```text
Law
  -> relevant Relation family
  -> relevance Formation
  -> concept/category road
  -> candidate Singulars
```

This is the mature Magic-Set / goal-directed face of Formation Rete.

### Tier 6 — Direct Law traversal: the terminal optimization

Once the lower tiers have established a route and Prophetic/Formation proof says it is sound and current, the route can crystallize:

```text
Law L --relevant-to--> Singular S
          targetPath = P
```

where `P` is a `PropertyPath` qualifier, **not a Property-Singular endpoint**. See `PROPERTY_ADDRESSING_IN_FORMATION_RETE.md` and `PROPERTY_AS_PREDICATION_NOT_BEING.md`.

This is the ultimate goal of the routing ladder:

> **the Law can traverse directly to the bearer/aspect it may actually need, without re-performing the category, similarity, or Relation-of-Relation search that originally proved the route.**

The lower tiers remain present as provenance and repair mechanisms. They do not disappear merely because a direct shortcut exists.

---

## 3. Prophetic Rete is both a filter and a relevance-graph constructor

The deepest integration is that Prophetic Rete and Formation Rete are not merely sequential optimizers.

Prophetic Rete already abstractly interprets Law structure to answer questions about possible reads, writes, ranges, and impossible interactions. Zach's refinement is to use the **same style of abstract interpretation to construct Formation Rete's ahead-of-time relevance graph.**

Consider the Law graph induced by shared structure:

```text
Law A action branch
   writes Singular S / PropertyPath p
           |
           | may satisfy
           v
Law B condition branch
   reads S.p
```

But the graph is richer than same-string PropertyPath equality. Two Law branches may connect through:

- the same Singular;
- compatible PropertyPaths on one bearer;
- the same or related Category;
- a Relation kind;
- a Formation;
- a concept-Singular;
- a C++-level data structure **exposed as Property** rather than hidden beneath the kernel;
- an OntoMath range whose output can intersect a downstream condition demand;
- a telos / Hierarchy-of-Joys structure when the Law's task is teleologically meaningful.

Different `ConditionNode` and `ActionNode` branches therefore induce different local graph shapes. Prophetic analysis should preserve that branch provenance rather than flattening an entire Law into one undifferentiated set of strings.

The resulting ahead-of-time object is approximately:

```text
Authored Law graph
      |
      | abstract interpretation
      v
possible write/read/relation/concept intersections
      |
      | IMPOSSIBLE edges removed only when proved
      v
conservative relevance graph
      |
      | Formation routing + slow adaptation
      v
retained Relation/Formations
      |
      | sufficient proof/currency
      v
direct Law relevance paths
```

This is where the two paradigms blend:

> **Prophetic Rete proves the possibility landscape from Law semantics; Formation Rete turns that landscape into traversable ontological roads.**

---

## 4. Branch-sensitive Law structure matters

A Law is not one atomic relevance blob.

For example:

```text
Condition:
    All(
        Related(subject, instance-of, category.pawn),
        Compare(subject.position.y, >, 3),
        Any(
            @king.inCheck == true,
            @event.subject == subject))

Action:
    Set(subject.material.glow, 1)
```

The branch that reads `position.y` has a different provenance and candidate domain from the branch rooted at `@king`, and the event branch has different activation semantics again.

Likewise, an Action tree may contain several writes with different target roots and different OntoMath ranges.

A mature relevance graph should therefore be able to represent something closer to:

```text
Law + stable branch identity
    -> read/write demand/effect
    -> relevant bearer/concept/relation family
```

not merely:

```text
whole Law -> bag of property strings
```

This is also the necessary precursor to the direct runtime seam described in the direct-relevance addendum: a completed Action write needs stable provenance so the engine knows **which proved relevance edges belong to this branch**.

---

## 5. Teleological routing through the Hierarchy of Joys

When a Law's task is teleologically meaningful, the Person's Hierarchy of Joys supplies a special high-information route.

The Hierarchy of Joys is already a rooted Formation of Lexemes connected by `grounds` Relations. Other beings carry `telos` PropertyPaths naming the Lexeme they are ordered toward. Formations/Relations can therefore be ordered by telos rank without creating a second type system.

That gives Formation Rete a special case:

```text
Law / branch with authored telos relevance
        |
        v
Person's Hierarchy of Joys
        |
        v
relevant telos Lexeme / ordered Formation
        |
        v
Singular Formations ordered toward that telos
        |
        v
candidate relevance routes
```

This can avoid brute lookup when an ActionNode or ConditionNode branch is structurally long but its purpose gives a much narrower semantic route.

The crucial rule from `FORMATION_RETE.md` remains: **cost and telos are distinct axes.** A more foundational or joyful route is not automatically computationally cheaper, and a cheap route is not automatically teleologically preferable.

A planner may therefore use something like:

```text
candidate route = sound route
priority = authored function(cost, telos, provenance, freshness, ...)
```

without collapsing those dimensions into one hidden scalar.

The Hierarchy is a route only when the Law/task actually bears meaningful teleological structure. It must not be consulted as ceremonial overhead for every ordinary property write.

---

## 6. The slow adapter and hot path have different jobs

This clarification makes their division exact.

### Slow adapter

The slow adapter is a **structure builder / repairer**. On its independent bounded clock it may:

- explore similarity neighbourhoods;
- search across Relation families and indices;
- compare competing route subkinds;
- construct Relation-to-Relation bridges;
- group useful roads into Formations;
- use Prophetic possibility information to avoid impossible regions;
- revisit stale or suboptimal roads;
- propose or reify higher-level relevance structure;
- eventually provide enough evidence/proof for direct Law shortcuts.

Its purpose is not to answer every hot-path query itself. Its purpose is to **make future hot-path queries need less search.**

### Hot path

The hot path asks:

```text
What is the highest current sound tier already available for this Law/branch/change?
```

Then it uses it.

Representative behavior:

```text
if direct Law->bearer/path shortcut is current:
    use Tier 6
else if Law->relevance Formation road is current:
    use Tier 5
else if Relation-of-Relation / category road is current:
    use Tier 4/3
else if cross-index road is current:
    use Tier 2
else if a similarity index can safely propose candidates:
    use Tier 1 + complete fallback where needed
else:
    use Tier 0 sweep
```

This is not necessarily a literal `if` ladder in C++; it is the semantic priority rule.

The adapter improves the ladder. The hot path **consumes** it.

---

## 7. Priority must become authorable, but hidden policy is temporarily tolerated only as scaffolding

If several sound routes exist, the engine needs a preference ordering.

Possible dimensions include:

- proven directness;
- expected fan-out / computational cost;
- freshness/currency;
- authored Person relevance;
- Category/concept specificity;
- telos rank;
- measured historical usefulness;
- whether a route is exact or merely proposing;
- whether it crosses between similarity spaces;
- whether it is already reified and persisted.

Zach's requirement is that these priorities ultimately be **authorable Properties/Relations** rather than a black-box comparator buried in C++.

That work may be deferred while the tier mechanics are being established. But the temporary implementation must be explicitly marked as scaffolding, not mistaken for permanent engine ontology.

Do not create a universal hard-coded `enum RelevanceTier` and treat the enumeration itself as the world's semantics. The tiers in this document are architectural roles. Their priority should eventually be expressed through existing Earthcall beings/properties where possible.

---

## 8. Soundness: higher tier means stronger provenance, not stronger truth

A higher routing tier does **not** mean the Law condition is more true.

It means the engine has stronger evidence about **where the Law needs to look**.

Therefore:

```text
Tier 6 direct edge
    != condition already satisfied

Tier 5 relevance Formation
    != members all satisfy the Law

Tier 3 Category
    != every category member is currently valid

Tier 1 similarity
    != relevant truth
```

Unless a separate proof establishes the entire truth claim, the live Law condition still decides.

This preserves Formation Rete rung 7's doctrine:

> **candidate membership proposes; live condition evaluation decides.**

The only thing the routing ladder is allowed to eliminate without fallback is a road or candidate proven impossible under the applicable Prophetic/derived-state rules.

---

## 9. Provenance flows upward; invalidation and repair flow downward

The ladder is bidirectional in time.

During construction:

```text
broad evidence
   -> semantic route
   -> organized relevance structure
   -> direct shortcut
```

During invalidation:

```text
direct shortcut premise changes
   -> shortcut stale / re-kinded / downgraded
   -> fall to its provenance route
   -> repair from the highest still-current lower tier
```

For example:

```text
Tier 6 Law -> pawn.17 position shortcut stale
        |
        v
Tier 5 Law -> chess-piece relevance Formation still current
        |
        v
repair only the pawn-specific terminal edge
```

The engine should **not** jump all the way back to a world sweep merely because the top shortcut broke if its lower-level proof structure remains valid.

This is why lower tiers are not garbage after crystallization. They are the shortcut's **explanation and repair spine**.

Every derived tier must obey `DERIVED_STATE_LEDGER.md`:

```text
derived from -> invalidated by -> rebuilt/repaired from -> guarded by test
```

---

## 10. Complexity model of the ladder

Let:

- `N` = total beings;
- `E_rel` = relevant graph edges;
- `b_rel` = effective constrained branching factor;
- `d` = route depth;
- `r` = actual downstream destinations for a concrete change;
- `T` = number of hot-path uses over which a discovered route remains valid.

The ladder aims to move repeated work through these regimes:

```text
Tier 0 sweep:                      O(N) or worse with nested evaluation
Tier 1 broad similarity/search:    bounded/index-specific candidate search
Tier 2/3 graph route:              O(V_rel + E_rel)
Bidirectional constrained route:   ~O(b_rel^(d/2)) in favorable meet-in-middle cases
Tier 4/5 retained route:           O(route length / candidate output)
Tier 6 direct shortcut:            O(r) + bounded path/currency lookup
```

A discovery costing `D` but surviving `T` uses contributes approximately:

```text
D / T
```

amortized discovery cost per use.

The goal is therefore not to make the slow adapter's search individually cheaper than every existing index on day one. The goal is to let repeated proof/discovery **change the future graph until the steady-state hot path is proportional to actual consequences.**

---

## 11. The relation to strong Beta networks

A good indexed Beta network can also make one changed fact reach only compatible partial tuples. Formation Rete does not claim that every tier has a universally better Big-O than every indexed join.

The difference is what happens to successful relevance knowledge.

In a conventional Beta network:

```text
successful partial matches
    -> retained tuple/index memory
    -> future incremental joins
```

In this Formation model:

```text
successful relevance discovery
    -> semantic Relation/Formation provenance
    -> higher-order relevance roads
    -> direct Law traversal
```

The ultimate specialization attempts to **compile the join away for that stable dependency slice**.

Beta remains admissible as a local implementation technique where it is actually the best structure. It is not the semantic center of the architecture.

---

## 12. Promotion/change-of-type is explicitly out of scope

Zach noted that a Singular graph could in some future architecture be "promoted" into a Formation, and more generally that Singular promotion/change-of-type is not yet an authored framework.

This addendum does **not** invent that framework.

Where it says a graph may become or be represented as a Formation, read it conservatively:

- an existing Formation may be constructed to gather the relevant Singulars/Relations; or
- derived/reified structure may refer to that graph through currently legal ontology.

Do not mutate a being's fundamental kind in C++ merely to satisfy this document. A future promotion framework must be authored separately.

---

## 12A. 2026-09-21 — incrementality must recurse into maintenance

Law-Direct has now validated the ladder's central premise in executable form: a proved category road can crystallize so the hot path no longer repeats its Relation proof. The next architecture applies the same principle to **maintenance of the ladder itself**.

The new rule is:

> **Only a relevant change should make a proof uncertain.**

Coarse `structuralRevision()` / `relationGeneration()` witnesses remain valid fail-open currencies, but they are not the final granularity. A future direct route, Category membership proof, retained path, route-priority certificate, or compiled event subscription should carry a conservative semantic dependency frontier. Prophetic abstract interpretation may then prove an unrelated delta irrelevant and preserve the existing proof.

This also changes the meaning of "repair downward." Downward repair is not merely "top tier stale -> choose a lower tier." It becomes:

```text
semantic delta
  -> affected proof frontier
  -> preserve every unaffected lower/higher proof
  -> reopen only the stale route/path region
  -> widen to broader provenance only where completeness is uncertain
```

Shortest-path maintenance should therefore evolve from repeated whole-search BFS/Dijkstra toward a dynamic repair algorithm over the affected subgraph. Multiple sound routes should keep incremental cost/order certificates; a changed route need not force global re-ranking when abstract bounds still prove the winner unchanged.

The EventBus itself becomes eligible derived state: begin with a complete broad feed, compile narrower subscriptions only from proved Prophetic dependencies, and restore the broad feed immediately when opacity/incompleteness invalidates that proof.

Full specification: `FORMATION_RETE_INCREMENTAL_MAINTENANCE_AND_SELF_REFINING_EVENTS.md`.

## 13. Implementation sequence implied by this model

This is architecture, not an assertion that the following code already exists.

1. **Branch-stable provenance.** Identify Law/ConditionNode/ActionNode branches stably across compilation so relevance edges attach to semantic branches rather than raw pointers.
2. **Prophetic relevance graph.** Extend Prophetic summaries into conservative branch-to-branch / write-to-read relevance candidates across shared Singulars, PropertyPaths, Relations, Formations, categories/concepts, and OntoMath ranges.
3. **Tier query contract.** Define how the hot path asks for the highest current sound relevance tier without hard-coding ontology into an enum-heavy subsystem.
4. **Slow-adapter route competition.** Let the adapter retain and compare cross-index/category/relation/similarity roads, still bounded and off the hot path.
5. **Formation provenance.** Reify/gather useful Relation-of-Relation roads where authorship and world-admission rules permit.
6. **Teleological route hook.** When a Law/branch has meaningful telos structure, allow Hierarchy-of-Joys ordering to constrain/prioritize route discovery without conflating joy and compute cost.
7. **Direct Law crystallization.** Materialize the proved `Law -> Singular + PropertyPath` route described in the direct-relevance addendum.
8. **Downward repair.** When a top tier invalidates, resume from the highest current lower provenance tier rather than restarting at the sweep.
9. **Authorable priorities.** Expose route preference dimensions as Properties/Relations/Law-authorable structure once the mechanics are stable; remove temporary hidden comparator policy.
10. **Parity + adversarial invalidation tests.** For every tier, forcibly stale it and prove the next lower tier preserves the same lawful reach.
11. **Measure each promotion.** A higher tier earns hot-path priority by soundness first and measured value second. Correct-but-slower structures may remain available without becoming the default.

---

## 14. Compact model for future agents

Do **not** read Formation Rete as:

```text
similarity search + some graph caching
```

Read it as:

```text
                    DIRECT LAW TRAVERSAL
                    Law -> Singular(+PropertyPath)
                              ^
                              |
                   proved relevance structure
                              ^
                              |
                 Relations among relevant Relations
                              ^
                              |
                Category / concept / authored relevance
                              ^
                              |
                    cross-index discovered roads
                              ^
                              |
                     similarity / overlap search
                              ^
                              |
                    complete over-approx sweep
```

Prophetic Rete cuts impossible regions out of the possibility landscape and helps construct the conservative relevance graph feeding this ladder.

The slow adapter climbs and repairs the ladder.

The hot path uses the highest sound/current rung already available.

If the top rung breaks, the engine falls to its nearest valid provenance, not automatically to the bottom.

And the final reason for all of it is:

> **A Law should eventually be able to traverse directly to the beings and aspects it can actually matter to, because the world has already done the work required to know why that traversal is sound.**
