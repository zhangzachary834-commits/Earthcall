# Implementation Plan: Ontological Rete Architecture

> **STATUS, 2026-09-16 — this plan is live, and its first half is built.** Sections 1–5 below are
> Antigravity's original text of 2026-09-03 and are kept intact for provenance. Two of its
> migration steps have since been **reversed by doctrine** (retiring the sweep, deleting Beta
> nodes) — see §7. What to implement next is §8, which supersedes §5.
>
> **Current architecture docs, in reading order:**
> `../architecture/law/FORMATION_RETE.md` (the rung ladder and §6's correctness floor) ·
> `../architecture/law/FORMATION_RETE_TIERED_RELEVANCE_LADDER.md` (Zach's tiered model, recorded by
> GPT-5.6 Sol, 2026-09-16) · `FORMATION_RETE_DIRECT_RELEVANCE_ADDENDUM.md` ·
> `PROPERTY_ADDRESSING_IN_FORMATION_RETE.md` ·
> `../architecture/ontology/PROPERTY_AS_PREDICATION_NOT_BEING.md` ·
> `../architecture/law/PROPHETIC_RETE.md` §2 · `../architecture/law/DERIVED_STATE_LEDGER.md`.
> **The record of what was actually built and measured:**
> `../Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md`.


**Origin.** The architectural foundation in this document was conceived by Zach on 2026-09-03, following an audit that exposed the relational limitations of the current C++ Rete. Zach provided the core paradigm shift: leveraging Earthcall's Category framework as "possibility receptacles," pre-computing relational joins as actual `Relation` beings, and turning the Rete network itself into an observable `Formation`. My (Antigravity's) contribution is internalizing this telos and formalizing its mechanical execution—specifically mapping how this discrete topological approach natively resolves the $O(N)$ sweep bottleneck and $O(N^2)$ continuous math explosion.


## 1. Motivation & Context
The current C++ Rete implementation (`Law.cpp`, `PropheticRete.hpp`) suffers from two fatal flaws when applied to Earthcall's continuous, dynamic simulation:
1. **Directional Blindness:** To avoid $O(N^2)$ memory explosion on continuous variables, the network drops remote state from the index, breaking declarative triggers (see `docs/audits/rete_directional_blindness.md`).
2. **The Sweep Bottleneck:** To mask the blindness, continuous laws fall back to an $O(N)$ brute-force sweep over the entire Universe every tick, setting a hard ceiling on scale.

This plan outlines the migration to an **Ontological Rete**, conceived by Zach. It rips the rule engine out of hidden C++ structs and reconstructs it natively using Earthcall's existing `Singular`, `Relation`, and `Formation` primitives. 

## 2. Layer 1: Category Filtering (Possibility Receptacles)
The $O(N)$ Universe sweep will be retired. Instead, the engine will leverage Earthcall's `Category` framework as semantic indices.

* **Instant Classification:** Upon creation, every `Singular` is filtered and placed into `Category` Formations (via `instance-of` or `composed-by` Relations).
* **Semantic Pruning:** When a Law looks for targets, it does not linear-search. It queries the relevant `Category` Formations.
* **Prophetic Integration:** The engine uses Prophetic Rete's bounds-checking to evaluate entire Categories at once. If a Category's intrinsic bounds are incompatible with the Law's conditions, the entire sub-graph is pruned instantly without inspecting a single instance.

## 3. Layer 2: Reified Condition Joins (Materialized Relations)
To solve the $O(N^2)$ join thrashing for continuous data (e.g., Distance, Line-of-Sight), Earthcall will stop calculating math inside rule engine join nodes. 

* **Topological Edges:** Continuous relationships will be evaluated by dedicated, highly-optimized subsystems (like spatial partitioning) which will emit first-class `Relation` beings (e.g., a `Near` relation between `A` and `B`).
* **Discrete Listening:** The rule engine will simply listen for discrete `relation-state` facts (e.g., "A new Near relation was formed"). 
* **The Result:** The combinatorial cross-product completely vanishes from the rule evaluator. Continuous math is converted into discrete graph topology before the Law ever sees it.

## 4. Layer 3: The Network as an Observable Formation
This architecture fulfills **Refusal #6 (No Black Box)** for the engine's own evaluation logic.

* **Reifying the Graph:** Beta nodes and Alpha filters will cease to be `std::vector<BetaNode>`. They will be represented as actual `Relation`s connecting `Category` Formations to `Law` Singulars.
* **Governable Logic:** Because the Rete network is now a standard `Formation`, it is fully legible to the rest of the engine. Laws can query the network. Meta-laws can optimize the network's category-prioritization on the fly. 
* **Dynamic Restructuring:** As new Categories are generated dynamically by First Movers, the network's shortest-path BFS graph traversals can automatically rewire to find the fastest evaluation routes.

## 5. Migration Strategy
1. **Establish Category Formations:** Ensure all `Object` and `Being` instantiations rigorously attach to their root ontological Categories via reified `Relation`s.
2. **Abstract Spatial/Continuous Joins:** Migrate continuous conditions out of raw `ConditionNode` mathematical comparisons and into discrete `Relation` states managed by localized subsystems.
3. **Deprecate C++ Beta Nodes:** Phased removal of `ReteNetwork::BetaNode` in favor of resolving condition asts via graph traversal of Category Formations.
4. **Retire the Sweep:** Once the graph provides complete coverage without directional blindness, the O(N) Universe fallback for `WhileTrue` laws can be deleted.

---
*Authored by Antigravity, session b7b980a8-6cb5-452f-a382-0c554ebd1d69, 2026-09-03T20:19:01-07:00.*


---

# 6. What is built — 2026-09-16

*Added by Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`. Rungs 0–7 of
`FORMATION_RETE.md` §8, mapped onto the tier names in Sol's ladder so the two documents can be read
against each other.*

| Tier | Mechanism | Where | State |
|---|---|---|---|
| 0 — sweep | `LawManager::sweepSubjects` + `Law::couldApplyTo` | `Law.cpp` | **the floor, intact.** §6 of FORMATION_RETE inverts §5.4 below: it is never retired |
| 1 — similarity | `Relevance::breadthFirstRoutes` — bounded BFS, shortest-in-hops, capped, kind-filtered, identity-aware | `src/Relation/Traversal/RelevanceTraversal.*` | primitive only; **no similarity metric**, and per §9.1 there must never be a single one |
| 2 — retained roads | `Relevance::SlowAdapter` — per-Law `Related(kind, category)` membership on an independent clock (one improve + one revisit step per tick) | `src/Relation/Traversal/SlowAdapter.*` | built, tested, **OFF by default** (`LawManager::setUseSlowAdapter`) |
| 2 — vocabulary | the implicit category: "beings carrying property X", seeded from the rarest required name | `LawManager::_vocabularyIndex` | live; rebuild made ~14x cheaper 2026-09-16 |
| 2 — endpoint | `RelationManager::relationsInvolving`, keyed by pointer **and** kept identifier | `RelationManager` | live, oracle-tested against a full scan |
| 3 — authored categories | **not built.** `couldApplyTo`'s vocabulary filter is the degenerate, unauthored stand-in FORMATION_RETE §3.0 names | — | blocked on concept-Singulars (§3.4) |
| 4 — Relations between Relations | `SlowAdapter::reify()` — a Formation of the carrying Relations, `gathers` edges, `routes-through` from the Law | `SlowAdapter.cpp` | implemented and tested; **never called by the engine** (it writes into a Person's world; a save carries it) |
| 5 — Law → relevance Formation | the `routes-through` edge above | — | nothing consumes it yet |
| 6 — direct `Law → Singular(+PropertyPath)` | **not built** | — | see §8 step 2: most of the proof material already exists |

Supporting correctness work that the ladder depends on: edge facts for both endpoints and an
incremental update path (rung 0); departure on the reactive path, so terminal membership only
*proposes* and the live condition decides (rung 7); a typed `Related` made legible to Prophetic, so
one condition kind no longer switches the world-wide write filter off (rung 4).

# 7. Corrections to §5 — two steps doctrine has since reversed

**§5.4 "Retire the Sweep" is refused.** `FORMATION_RETE.md` §6 and Sol's Tier 0 both make the
complete sweep the **correctness floor**, permanently: an approximate index that misses a candidate
*narrows*, and `PROPHETIC_RETE.md` §2 forbids narrowing because the law goes deaf and nothing
reports it. Every structure built since is allowed to be approximate **because** something complete
stands behind it. The sweep stops being the bottleneck to eliminate and becomes the thing that
licenses the tiers above it.

**§5.3 "Deprecate C++ Beta Nodes" is narrowed.** Formation Rete supersedes the classical Beta
cross-product as the *semantic join model* — but Beta machinery may remain as a local incremental
algorithm where it is useful. It is not the ontology; it is also not forbidden. (Sol, 2026-09-16.)

**§4's "reify the network itself" carries a caution.** Refusal 1 still applies: the tiers are a
conceptual ladder, not a mandate to instantiate seven engine classes. Where a tier can be an ordinary
Relation or Formation, it should be.

# 8. The plan from here — 2026-09-16

*Ordering follows `FORMATION_RETE_TIERED_RELEVANCE_LADDER.md` §13. Each step below adds what that
document deliberately leaves out: where in this tree it goes, what proves it, and what must not
regress. Nothing here may be built without its ledger row
(`DERIVED_STATE_LEDGER.md`: derived from → invalidated by → guarded by).*

**The invariant every step serves:** use the highest tier whose soundness *and* currency are
established; otherwise fall to the highest safe tier below. Refusing to answer is always correct.

### ✅ Step 1 — Branch-stable provenance *(built 2026-09-18; unblocks step 2)*
Before 2026-09-18, `Prophetic::Index` keyed effects only on `lawId`, so a law whose
condition was `Any(A, B)` could not say *which arm* a relevance edge served. It now carries
`WriteEffect{lawId, branchId, ...}` and `ReadDemand{lawId, branchId, ...}`, with branch ids
derived deterministically from the authored node text. The identity therefore survives
`Law::recompile()` rebuilding executable predicates and survives the condition/action JSON
round trip used by save/load.
**Test:** two `Any` arms produce distinguishable branch ids, and those ids survive the JSON
round trip; action branches receive the same witness.
**Trap retained:** the id is derived from authored text, not pointer or vector position —
`_conditionPredicates` is cleared and rebuilt on every edit.

### ✅ Step 2 — The Prophetic relevance graph *(derived layer built 2026-09-18)*
Zach, 2026-09-16: *the same abstract interpretation that filters the possibility landscape can
construct an ahead-of-time graph of relevancy Relations.* Most of the proof machinery exists:
`Index::writeRangeOf(path)` (union of every authored write, path-normalized across referent
prefixes by `normalizedPaths`) and the `NoLawfulDriver` case in `Index::unreachable()`, which already
performs the **pairwise disjointness proof** — "some law writes this path, but the union of writes
is disjoint from the demand".
**Build:** the same test run per pair instead of against the union — an edge `A ⇒ B` wherever A's
write path normalizes onto B's read path and the ranges are not provably disjoint.
**Soundness:** an edge may be present when it need not be (widening); it may never be absent when it
could matter. `aboutInstances` reads belong to the quantified being, not the law's subject, and must
not be conflated.
**Trap, paid for on 2026-09-14:** opacity is not local. One condition kind marked "opaque read" makes
the whole index incomplete and switches the property-write filter off for the entire world (measured
~17x on a category-scoped law). A relevance graph built on this index must fail open **globally**
when `Index::complete()` is false, not locally around the opaque law.
**Test:** a law that writes into another's demand appears; one whose range is provably disjoint does
not; one opaque law makes every edge fail open.

**Built 2026-09-18:** `Prophetic::Index::relevanceEdges()` now materializes this graph as
derived C++ state, keyed by stable authored branch provenance and carrying `aboutInstances`.
It is deliberately not yet reified as world Relations and has no narrowing authority on the
hot path. `relevanceComplete()` is false and the graph is empty when either read or write
analysis is opaque. `prophetic_rete_test` §H guards branch identity across JSON round trips,
`Any`-arm distinction, pairwise disjointness, and global opacity. Step 3 now consumes these lower-tier structures through one cached query. The next bounded
implementation is Step 4's route competition; reifying roads into a Person's world remains gated below.

### ✅ Step 3 — The tier query contract *(built 2026-09-18)*
The hot path now asks one question: *what is the highest current sound route already selected for
this Law?* `LawManager::_candidateRoutes` stores that answer per Law. `sweepSubjects` no longer
tries the adapter and then scans every required property name each time it runs; after currency
checks it consumes one named source: complete sweep, vocabulary seed, or one current retained road.

**Steady-state selection is O(1) per Law.** The cache key is checked with constant-time revision /
generation comparisons. Expensive work is kept off the per-candidate loop: the vocabulary seed is
chosen when the route decision refreshes; the adapter exposes a no-copy `candidateViewFor()` for
the current single-road case. Multi-road union/ranking is deliberately deferred to Step 4 rather
than sneaking O(number-of-roads) selection back into the frame path.

**Higher must mean narrower.** A current adapter road is selected only when its candidate cardinality
is strictly smaller than the lower vocabulary/sweep route. Equal-width roads fall back. This is the
measured Chess lesson encoded as a contract rather than a special case.

**Currency:** the decision records `Law::textRevision()`, the Law's
`conditionRevision()`, `Universe::structuralRevision()`,
`Universe::relationGeneration()`, and an O(1) per-Law adapter-road currency stamp. The last
signal changes when that road moves between unbuilt/built/capped states or is rebuilt under a new
world/graph generation; a maintenance revisit under the same world leaves it unchanged. This lets a
Law promote upward when the independent clock finishes a road without making every slow-clock tick
churn every Law's tier cache.

**Test:** `slow_adapter_parity_test` now additionally proves steady frames do not reselect; a
narrower current road promotes; unchanged slow-clock revisits do not churn the choice; an equal-width
higher road is refused; and a stale road immediately falls to the lower complete tier while parity
remains identical.

### Step 4 — Route competition in the slow adapter
Zach, 2026-09-16: *where tiers overlap in subkinds the slow adapter has to judge which subkind is
best* — traverse to the Relation pointing at the Formation category Relation, or to the Formation of
similarity Relations? The adapter already has the clock and the caps; it needs to hold more than one
road per (law, question) and rank them.
**Gate:** the ranking function is temporary scaffolding and must be marked as such (step 5).
**Test:** with two roads to the same answer, the adapter serves the narrower one and both remain
available for repair.

### Step 5 — ⚑ Authorable priorities *(Zach: "okay for now to hold this property exposure part for later")*
Route preference must become legible Properties/Relations rather than hidden C++ policy.
**Do not** put it in `Relation::weight`: Zach is leaning toward removing weight entirely (§9.2), and
it is in save files, in `RelationManager::add`'s duplicate-merge rule (which *sums* weights), and in
a developer-mode audit warning.

### Step 6 — The teleological route *(Hierarchy of Joys)*
When a branch is genuinely teleological, route through the Person's Hierarchy of Joys: telos Lexemes
and the Formations ordered by them, instead of brute category lookup on a long branch. The hierarchy
is already a rooted Formation of Lexemes joined by `grounds`, and beings carry a `telos` path — no
second type system needed.
**Rule:** cost and telos stay distinct axes (`FORMATION_RETE.md` §5). One float carrying both would
make cheap paths look holy.

### Step 7 — Direct `Law → Singular(+PropertyPath)` crystallization *(Tier 6)*
The terminal optimization: once the lower tiers prove a branch can only need a particular bearer and
aspect, retain that edge and stop re-deriving it.
**Ontology:** `Law → Singular`, qualified by a `PropertyPath`. **Never** `Law → Property-being` —
Property is predication, not a being (`PROPERTY_AS_PREDICATION_NOT_BEING.md`); "Relations join
beings, Properties disclose them".
**Dependency worth knowing:** naming a bearer by identifier runs through `resolveLawRoot`'s referent
map, which had **no currency test at all** until 2026-09-16 (`referent_map_invalidation_test`). If it
stops rebuilding, every `@`-rooted law goes silent with no error.

### Step 8 — Downward repair
When a Tier-6 edge goes stale, repair from the highest surviving tier rather than falling to a
sweep. **Current behaviour is honest but blunt:** `SlowAdapter::current()` refuses the road when
either counter moved, and `step()` rebuilds one road per tick — a one-tick fall to the sweep. That is
affordable while a road costs O(degree); it will not be at Tier 6 with many shortcuts.
**Connection:** a stale shortcut is structurally a sub-Relation whose premise went false. Zach's rule
(`PRIMARY_AND_SUB_RELATIONS.md` §6b) is *re-kind, never destroy*, and his test is *"false in the
current state, not impossible forever"* — which is only affordable **because** nothing is destroyed.
Tier 6 edges should follow the same discipline: downgrade and keep the provenance, do not delete.

### Step 9 — Parity and adversarial invalidation tests, per tier
For every tier: force it stale and prove the next lower tier preserves the same lawful reach. The
pattern exists — `slow_adapter_parity_test` runs one world with the adapter on and off and demands
identical reach, including a being that qualifies only through an `Any` arm.
**Why that case matters:** a road is usable only where *every* satisfying being must travel it — a
`Related` **conjunct**. Under `Any` the other arm suffices; under `Not` being on the road
disqualifies; a quantifier's inner condition is about the instances. `collectCategoryRoutes` walks
`All` chains only, and the parity test goes red the moment it does not.

## What gates the plan — Zach's, not an agent's

1. **§9.2 — does `Relation::weight` survive?** Blocks step 5 and any cost/priority model.
2. **Reification into real worlds.** Step 4's Formations and step 7's edges are beings; admitting
   them to a Person's world is authorship, and saves carry them.
3. **Authored Categories / concept-Singulars.** Tier 3 and rung 2's Formation half.
4. **Singular promotion / change of type** — Zach, 2026-09-16: *"a framework I haven't authored
   yet"*; Sol's ladder §12 puts it out of scope. Several tier-promotion ideas want it.

---
*§§6–8 added by Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`, 2026-09-16 18:05 PDT,
after Zach's tiered-ladder clarification and GPT-5.6 Sol's recording of it. §§1–5 remain Antigravity's
text of 2026-09-03; the architecture in both is Zach's.*
