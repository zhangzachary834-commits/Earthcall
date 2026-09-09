# Formation Rete

*The Ontological, Graph-Routed Successor to Standard Rete*

**Status:** **Rungs 0, 1 and 2 of §8 are done** (2026-09-08 / 2026-09-09). Rung 0 closed §1.2(a):
relation-state facts now have an incremental update path and both endpoints. Rung 1 measured
§1.2(b) — and the measurement found a **larger quadratic that was masking it**, in transient
`Moment` destruction rather than in quantifiers; that is fixed, and §8 rung 1 records why the
remaining quantifier cost is not removable by indexing. Rung 2 built the vocabulary index and
fixed a fourth deafness; its Formation half stays blocked behind §3.4's concept-Singulars.
Rungs 3–7 remain specified, not implemented. §1 is **verified against the tree**. §3–§7 are design. §9 holds the ⚑ AUTHOR
decisions that are Zach's alone; §9.3 is answered, the rest are open.

**Companion docs:** `PROPHETIC_RETE.md` (§2's widen-never-narrow rule, which governs every
index here), `B-time Rete.md` (Zach's originating notes — the possibility-space filter and
the First-Mover grounding question this document's §7 answers), `AUTHORED_CATEGORIES.md`
(a category is a rooted acyclic Formation), `HIERARCHY_OF_JOYS.md` (telos as Lexeme; rank
as depth from root), `NO_BLACK_BOX.md` (Refusal 6, which §4A depends on),
`ALGORITHMS_AS_LAW.md` §3 (bounds are doctrine), `ourverse/OURVERSE.md` (the shared
superfilament Zach names as the ontological precedent for §3.2),
`LAW_EXECUTION_FRONTIER.md` (the *orthogonal* axis — that document lowers the cost **per**
evaluation via bytecode; this one lowers **how many** evaluations happen. They compose;
they do not compete).

---

## Origin and attribution

**The architecture is Zach's.** The paradigm — Categories as possibility receptacles,
reifying joins as `Relation` beings, the Rete network as an observable `Formation`, the
continually-improving path structure, reified and re-categorizable Relation paths, and the
Law traversing to its own subjects — is his, stated across two sessions (2026-09-03 and
2026-09-04). The framing question is also his, and recovering it is most of why this
revision exists: *"can Rete honestly and swiftly evaluate multi-subject conditions — the
joining problem, where you have to know which subjects and properties to join?"*

**Antigravity** (session `b7b980a8`, 2026-09-03) wrote the first audit and the first draft
of this document. Its audit asked Zach's question and reached a wrong diagnosis
("Directional Blindness"); §1 corrects the record. That correction is not a dismissal —
the audit put the right question in writing, and the two real defects in §1.2 were found
by going and checking its claim.

**Claude Opus 5** (this document's reviser) contributed: the verification in §1 and §10,
including the probe that proved the `Related` deafness; the identification of the burned
pair quantifiers as this architecture's charter (§1.3); the mapping to FCA, HNSW, hub
labeling, magic sets and worst-case-optimal joins (§4); the cost-versus-telos separation
(§5); the Bloom-filter framing of the sweep (§6); and the stratification requirement (§7).
Where a section says "extension," it marks a place where I carried Zach's idea somewhere he
had not yet taken it, and it is flagged so authorship stays legible.

Some of §4 is convergence rather than contribution: Zach described the slow adapter's
behavior before either of us named HNSW, and described root-distance indexing before
either of us connected it to hub labeling — which the Hierarchy of Joys had already
implemented as "rank is depth from the root along `grounds`."

---

## 1. What the tree actually does today

### 1.1 The question, answered

**Can Earthcall's Rete honestly and swiftly evaluate multi-subject conditions?**
**No — and not for the reason the first audit gave.** Three mechanisms exist:

| Mechanism | Honest? | Swift? |
|---|---|---|
| `ForAny` / `ForAll` quantifiers | yes — correct results | **no** — ~5.5x an equivalent `Compare` at 320 beings, measured (§1.2b) |
| `ForAnyPair` / `ForAllPair` | — | **burned on purpose** (§1.3) |
| `Related` — the designated replacement | ~~**no** — provably deaf (§1.2a)~~ **fixed, rung 0** | yes |

So a multi-subject condition today is either *honest but quadratic* or *fast but deaf*.
There is no third option. That is the real finding.

**Correcting the record.** The first audit's "Directional Blindness" — a law failing to
wake when a *remote* subject changes — **does not reproduce**, in any activation mode:

- **WhileTrue with terminals.** `ReteNetwork::retractFirst` removes only non-state facts;
  state facts persist in node memories across ticks, and `collectTerminalSubjects` is
  **polled every tick**, not edge-driven. The subject stays in the terminal memory, and
  `applyTo` re-evaluates the *whole* condition tree — qualified-root conjunct included —
  before firing (`Law.cpp:382`). The tick the remote being changes, it fires.
- **OnBecomeTrue.** Never takes the reactive path at all; `Law.cpp:1778` gates it on
  `WhileTrue`. It sweeps and calls `conditionsSatisfied` every tick.
- **OnEvent.** Fires from the agenda drain — which is what choosing an event law *means*.

So dropping a qualified-root conjunct from the index (`ConditionModel.cpp:500`) really is a
**widening**, exactly as its comment claims, and `PROPHETIC_RETE.md` §2 is being honored.

There is also **no free variable** in the condition language: a qualified root resolves via
`resolveBeingToken` (`ActionModel.cpp:206`) to *one named being*, or to
`@event.subject` / `@event.object` / `@world`. The many-to-many join a classical Beta
network exists to solve is **not expressible**, so its absence is not the defect.

### 1.2 The two real defects

**(a) `Related` is deaf to relations formed at runtime. — probe-proven.**
`seedStateFacts` is guarded by `_seededSubjects` (`Law.cpp:2209`): once per being, ever.
Relation-state facts are emitted there, keyed `subject = relation->a()`. On
`relation-formed` the bus handler calls `seedStateFacts(e.subject)` (`Law.cpp:1527`) — and
`e.subject` is **the Relation being itself** (`RelationManager.cpp:167`), not its source
endpoint. The endpoint is already seeded, so the call returns immediately. No
relation-state fact is ever asserted for an edge formed after the first tick.

And there is no safety net: a WhileTrue law with a `Related` condition compiles terminals,
so `hasTerminals` is true and it **never falls through to the sweep**. It never receives a
candidate, so `applyTo` never runs, so nothing re-checks. Deaf, permanently, silently —
the failure `PROPHETIC_RETE.md` §2 exists to forbid, in the mechanism the architecture
designated as *the* answer to multi-subject joins.

This is the same class of bug as the comment at `Law.cpp:2227`, which describes finding a
`Related` alpha no fact could reach and fixing the **seed** half while leaving the
**update** half open. `_relationTypesInPlay` has the same shape: it is filled at compile
time (`Law.cpp:2297`), so a law authored *after* beings were seeded names a relation type
nobody ever seeded facts for.

**(b) Quantifiers are a hidden quadratic. — MEASURED 2026-09-09; real, but not the dominant
one.** The description below is correct and stands. What measurement added: the engine's
dominant quadratic was elsewhere entirely (transient `Moment` destruction — §8 rung 1(i)), it
was masking this one, and with it removed a bare `ForAll` fits k ≈ 1.83 against an identical
`Compare` law at ≈ 1.50. It is also **not removable by indexing**, because the cost sits in the
per-subject re-evaluation `applyTo` must do for safety. See §8 rung 1.
`ForAny`/`ForAll` compile to a closure that loops `Universe::instance().beings()`
(`ConditionModel.cpp:424`) — a vector the provider **rebuilds on every call**. In
`compileToRete`, quantifiers fall through to the leaf path, where `targetAttr` is set only
for `Compare` and `Related` — so a quantified condition becomes **an alpha node with no
attribute filter whose predicate scans the entire Universe.** The network believes it is
holding a cheap filter; it is holding a full scan, woken by every property change of every
being.

On the sweep path it is cleaner to state. `collectPaths` correctly returns early for
quantifiers (the inner condition is about the instances, not the subject), so a law whose
only condition is a `ForAny` has **empty `requiredProperties`** → `sweepSubjects` returns
*every* being → **O(N) subjects × O(N) inner scan = O(N²) per tick.**

Two things the tree gets *right* here, and which must not be "fixed": `collectPaths`
returning early, and `PropheticRete::walkCondition` filing a quantifier's inner demands as
instance reads that do **not** propagate to the parent, while still recording their
`readNames` (`PropheticRete.cpp:496–506`). Demands narrow; read-names widen. Each goes the
sound direction, and the reactive path stays reachable because of it.

### 1.3 The charter: kinds 12 and 13

`ConditionModel.hpp:37` — *"12 and 13 were the pair quantifiers (ForAnyPair / ForAllPair),
retired in favour of modelling pairs as Relations."* Those are the burned enum values
CLAUDE.md warns about, and `fromJson` still logs a message telling anyone loading an old
save to model pairs as Relations instead.

**Earthcall built the join, used it, and consciously replaced it with a graph strategy.**
The join key became *the edge*: "which subjects do I join?" → "the ones the Relation
names." That decision is the historical charter for everything below — Formation Rete is
not replacing a compromised design, it is **finishing a migration the codebase already
began and left half-wired.** Defect (a) is the missing half.

---

## 2. The core paradigm

Formation Rete keeps what Rete is actually good for — **incremental discrimination**, never
recomputing what did not change — and discards the part Earthcall's language cannot ask
for: the Beta cross-product. In its place, the join key is an authored edge, and finding
which subjects to join becomes **traversal of a bidirectional, typed hypergraph** whose
nodes, edges, and index are all ordinary beings.

The engine is reified into the world using `Singular`, `Relation`, and `Formation`. This
satisfies **Refusal 6** for the engine's own machinery: the evaluation index is a Formation,
so it can be read, governed, and optimized by Laws — and it requires **zero new C++
classes**, which is Refusal 1 satisfied by construction rather than by discipline.

**Verified structural support (nothing new is admitted to the type system):**

| Needed | Exists today |
|---|---|
| Relations as members of Formations | `class Relation : public Singular` (`Relation.hpp:46`) + `Formation::addMember(Singular*)` |
| Categories of Relations | `Formation::relationTypeTag` already types a Formation *by relation kind* |
| Formation-Category | *"A Formation without a root is a set; a Formation with one is a category"* (`Formation.hpp`, quoting `AUTHORED_CATEGORIES.md` §3) |
| Telos ordering of members | `Formation::membersOrderedByTelos()`, `satisfiesJoyBounds()` |
| Traversal bound | `Formation::kMaxFormationDepth = 32` |
| Numeric-domain overlap | `Range::mayIntersect` (`PropheticRete.hpp:88`), `rangeOfPiecewise` |

The one structural thing that is **missing**: relation-state facts are emitted only where
`relation->a() == being`, so the network can traverse a→b and never b→a. Bidirectionality
is declared by the architecture and not yet built — same function as defect (a).

---

## 3. The layers

### 3.0 Layer 0 — Category filtering (possibility receptacles)

Laws do not linear-search for targets; they query Category Formations. This is the
generalization of something the engine already does badly: `couldApplyTo`'s vocabulary
filter is a **degenerate, implicit, unauthored** category ("beings carrying `position`").
Making it an authored Formation makes the index legible, governable, and a strict
improvement independent of everything else here.

**Constraint (non-negotiable).** Prophetic pruning of a Category may only ever conclude
IMPOSSIBLE. A Category's bounds are sound only if membership is closed *and* the bounds are
invariants **of membership**, not observations of current members. Pruning on observed
bounds is precisely the narrowing that cost the Synthesis Studio every WhileTrue law it
had. This is the one place this architecture could reintroduce silent deafness at scale.

### 3.1 Layer 1 — Overlap

Beyond `instance-of` there is `composed-by`, and Zach adds **ontological overlap**, itself
a *category of relation* with subkinds: **property overlap**, **kind overlap**, and
**quantitative overlap** of numerical domains. Rich relation kinds are what let traversal
reach the right category quickly and narrow the sweep.

The quantitative subkind is **already half-built**: `Range::mayIntersect` is literally "do
two numerical domains overlap," and the abstract interpreter already computes those ranges
from authored mathematics. Point the existing prophetic index at category definitions
instead of law conditions and that subkind falls out — no new subsystem.

**Category-level overlap.** Overlap between *categories* is O(C²) with C small, and changes
only when a category's **definition** changes, not when a member moves. This is where the
top-down structure comes from — and Zach's point is that it is **not** a rectangular
hierarchy: because a Category is itself inside a Formation, it carries graph structure, so
there is more overlap available and therefore **more skips**. The ontological precedent he
names is Ourverse: a shared superfilament above the individual Zones, not merely a stack of
them (`ourverse/OURVERSE.md`).

**Instance-level overlap is retained, not abandoned.** An earlier draft of this document
called instance overlap simply "a trap." That overstates the correction and drops Zach's
actual position. The trap is only the *brute-force* reading — materializing every pairwise
overlap every frame, which recreates the O(N²) cross-product as persistent state and
inherits defect (a) at N² scale. What Zach specified instead is a **slow adapter**:

- it runs on an **independent clock**, and is **capped**;
- its target is *"finds better overlap over time,"* never *"tracks every possible overlap at
  every Moment"*;
- it works on **narrow parts of the phase space**, not the whole;
- and — decisively — **it does not evaluate truth-values.** It collects most-similar paths
  between existing Singulars and Categories. The Law's own conditions still decide truth.

That last clause is what makes approximation admissible at all, and §6 states the condition
under which it stays admissible.

### 3.2 Layer 2 — Relations of Relations: the path index is itself authored structure

Because a Relation is a Singular, there can be Relations *between* Relations, and Formations
*of* Relations. The valuable ones are the Relations between (1) narrowed-down Categories
themselves and (2) specific Singulars within those Categories.

The move is that **the discovered paths become beings.** A path is not a transient result of
a search; it is a `Relation` — or a Formation of Relations — that persists, and that a
condition searcher can simply traverse rather than recompute. And because paths are beings,
they can themselves be **categorized and placed in Formations**, which means the
path-structure used to read the graph can be **rearranged** without touching the graph it
indexes. The optimizer's own data structure is authored, legible, and re-orderable.

This is the layer the first draft lost entirely: it mentioned the index being made of
Relations only as a recursion *hazard* (§7 here), never as the purpose.

### 3.3 Layer 3 — The Law traverses to its own subjects

A Law is a Singular, so it participates in Formations and Relations directly. A Law can
therefore stand in a Relation with its relevant category-Relations — a Relation to
Relations — and **traverse to the subjects it governs** instead of waiting to be woken by
facts flowing to it.

*Extension (Opus 5):* this **inverts Rete's direction.** Rete is forward-chaining and
data-driven; a Law traversing to find its subjects is **goal-driven backward chaining.**
Neither is better — forward wins when few facts change and many rules might fire, backward
wins with a specific goal against a large fact base. §4C names the technique for combining
them, which is the frontier answer to exactly this.

### 3.4 The Formation-Category

Zach specifies a hybrid Singular, created **at runtime, not added to the type system**: a
Formation that *is* a Category yet is composed of bidirectional parts.

The type already exists — a Formation-Category is a **rooted Formation**, per
`AUTHORED_CATEGORIES.md` §3 as quoted in `Formation.hpp`. What must actually be built is
(a) members that are Relations (already legal) and (b) the **bidirectional index**, which is
the one genuinely missing structure (§2).

Zach: Given my recent revision to the definition of Formation, namely that there must be at least one Relation between multiple branches, 
a merely branching structure that doesn't ever do that can only count as a Formation by bridging the gap 
with concept-Singulars, which are constructions that anticipate Singulars with specifically shaped properties. 
(i.e. in this case, Relations with concept Singulars are like a way of saying "I'm relating 
to this concept in advance".) 

**What that costs the index, checked against the tree (Opus 5, 2026-09-08).** Zach's revised
definition is not a refinement here, it is a blocker on the obvious implementation, and the
tree already enforces the thing that makes it one:

- A category built from `instance-of` and `subcategory-of` alone is **purely branching**. Under
  the revised definition that is a hierarchical Relation, not a Formation — so "the index is
  just a Formation" (§2) does **not** follow from taxonomy edges by themselves.
- And it cannot be fixed by adding taxonomy edges, because `RelationManager::add`
  **rejects cycles in `subcategory-of`** (`RelationManager.cpp:110`). The one edge type that
  could close a loop between branches is the one the engine refuses to let close. The
  acyclicity `AUTHORED_CATEGORIES.md` requires and the cross-branch Relation a Formation
  requires are in direct tension, and the resolution has to come from outside the taxonomy.
- Which is what the concept-Singular bridge supplies, and the machinery already exists:
  `ObjectConcept` is *"the word for the thing… an extra-spatial Object storing the CONCEPT of a
  set of objects"*, and its `RelationTemplate::bAnchorId` is literally a relation to a being
  **named now and resolved later** — "relating to this concept in advance," in code, since
  `INTERACTION_AS_LAW.md` §7. Its own comment names the case: a control's
  `instance-of -> category.control.button` is an edge whose far end is deliberately outside the
  set, and dropping it produced *"a hundred beings that were not buttons, that no archetype law
  could see."*

So rung 2 (§8) may not simply promote `couldApplyTo`'s implicit vocabulary filter into a rooted
Formation and call it a Formation-Category. It must say which concept-Singulars the members
relate to, and those relations are what make the structure a Formation at all rather than a
tree wearing the name. Filed as a constraint on rung 2, not on rung 0.

---

## 4. Theoretical grounding

Named so they are not rebuilt from scratch — CLAUDE.md's frontier-approach rule, and the
same move as Claude Fable volunteering Rete itself rather than brute-forcing law matching.

### A. Category lattices — Formal Concept Analysis
Categories that overlap by shared properties do not form a tree; they form a **lattice**,
and the canonical construction is FCA, where category overlap is the lattice meet.

**Why it works *here* specifically:** FCA needs a **total** object-attribute incidence
relation. Most systems cannot supply one, because objects have hidden fields. **Refusal 6 is
the precondition that makes the derived lattice trustworthy** — there is no unregistered
field for it to miss. "No black box" turns out to buy a computational capability, not only
a governance property. Bound the exponential worst case with **iceberg lattices** (a support
threshold) — again incremental and capped, which is the same shape as the slow adapter.

### B. The slow adapter — HNSW
"Bounded neighbors per node, built incrementally on an independent clock, approximate rather
than complete, upper layers giving long-range skips, greedy traversal top-down" is a precise
description of **Hierarchical Navigable Small World** graphs. Its parameters map onto what
Zach specified: `M` (max neighbors — the cap), `efConstruction` (build effort — the
independent clock), `efSearch` (query effort — how hard the searcher looks). Zach arrived at
the hierarchy for ontological reasons; it is the same structure that makes the algorithm
work.

**Hub labeling.** Shortest-path answers on a large dynamic graph are not stored per pair
(O(N²)); they are stored as distances to a small hub set and answered by intersecting hub
sets (pruned landmark labeling). Root Categories are the natural hubs — and this is already
Earthcall's own design, not an import: `HIERARCHY_OF_JOYS.md` §1 defines rank as **depth
from the root along `grounds`**, which is a one-hub labeling structure that exists today.

**Two caveats.** (i) HNSW assumes roughly metric behavior; overlap has three subkinds that
may not form one metric space, and greedy traversal can stall where the triangle inequality
fails badly — see §9.1. (ii) HNSW **rots** under heavy update. The independent clock needs
*two* rates, an improve rate **and** a revisit/decay rate, or "finds better overlap over
time" degrades into "found better overlap once."

### C. Forward-backward hybrid — Magic Sets
The Datalog technique that takes a backward-chaining goal and restricts a forward-chaining
engine to only the facts that goal could ever reach. That is, almost exactly, "use the Law's
position in the graph to decide which facts get pushed to it" (§3.3) — decades of
literature, directly applicable, and the piece that lets the two directions cooperate
instead of duplicating each other.

### D. Why traversal beats joins — worst-case-optimal joins
Ngo–Ré–Rudra and Leapfrog Triejoin establish that replacing pairwise joins with multi-way
traversal/intersection is **asymptotically better**, not merely a pragmatic dodge. This is
the result that retroactively justifies burning kinds 12 and 13 (§1.3) as a principled
choice rather than a retreat.

---

## 5. Routing objective: cost and telos are two axes

Shortest ≠ cheapest, and cheapest ≠ best. A 2-hop path through a category with 10,000
members costs far more than a 5-hop path through narrow ones, so hop count is the wrong
metric; the edge weight must be **selectivity** — expected fan-out — and the search
minimizes the sum of logs. That is still Dijkstra; only the weights change.

**Do not collapse cost and value into one float.**

- **Cost** = expected branching factor of traversing the relation. Computational.
- **Value** = telos / joy-rank. Authored. `HIERARCHY_OF_JOYS.md` already ranks it, and the
  To-Do list (line 341) already earmarks joy-rank for **agenda ordering** — *"the missing
  piece of the Rete and the missing force of the Hierarchy of Joys are the same slot."*

Combined, traversal is **best-first over value per unit cost**: the engine looks first where
the telos of the world intends it to look, scaled by what looking costs. One float carrying
both would silently make cheap paths look holy.

`Relation::getWeight()` exists, is a registered governable property, and refuses to default
outside developer mode. Whether it carries *value* (strength/telos) or *cost* (fan-out) is
unresolved — §9.2.

---

## 6. The sweep is the correctness floor

**The O(N) sweep is not deleted.** An earlier plan's step "retire the sweep" is inverted
here, for two independent reasons.

**Zach's reason (membership).** Every Singular has to be *joined into* the Formations in the
first place, and that itself takes sweeping, searching, and sweeping again. The index is
maintained by traversal; the maintenance has a floor.

**The soundness reason (extension, Opus 5).** An approximate index that **misses** a
candidate *narrows*. That is the direction `PROPHETIC_RETE.md` §2 forbids — the law goes
deaf and nothing reports it. Approximation is admissible only because the overlap index
proposes candidates rather than deciding truth (§3.1) — and that is only safe if something
**complete** stands behind it. Hence:

> **The overlap index may only ever be a scheduling optimization over a complete-but-slow
> mechanism. Its job is to make the sweep rare, never to make it unnecessary.**

This is Bloom-filter discipline: one-sided error, and the *direction* of the error is the
whole design. It is also how a query planner relates to a table scan — the scan is what
makes it safe for the optimizer to be heuristic. Nobody deletes the sequential scan. The
sweep stops being the bottleneck to eliminate and becomes **the correctness floor that
licenses everything above it.**

---

## 7. The grounded tower

Because the index is a Formation of Relations, the overlap adapter will index its own index,
and traversal can descend into index edges while traversing. That is partly the point —
it is the meta-optimization Layer 2 wants — but reflective architectures (3-Lisp, the CLOS
metaobject protocol) all landed on the same requirement: a **grounded tower**, where each
meta-level is implemented by the level below and the tower bottoms out in something
non-reflective.

Zach answered this in `B-time Rete.md` before it was asked here, and his answer is richer
than "it bottoms out in C++": the strata run **First Movers / joystick authoring → the
governing Singularity-Kernel and its cabinet of sub-laws → metalaws acting collectively as
the abstract interpreter → the main Rete engine** — *"not a clean transition break, more
like fluidly and intricately connected stratums."* The C++ sweep is the floor of that
tower, not the whole of its ground.

What remains to specify is **stratification**: a rule for which layer's edges a traversal at
layer N may *read* but not *walk into*. `kMaxFormationDepth = 32` is already the doctrinal
bound ("bounds are doctrine, not limits"); the layering rule is not yet written. Note also
Zach's own observation in `B-time Rete.md` that this structure "is no longer a DAG in the
strict sense — it becomes very cyclic," which is exactly why the stratification rule cannot
be left implicit.

---

## 8. Build order

Rungs, in order, per `LAW_MIGRATION_FRAMEWORK.md` §2 — never skipped.

0. ✅ **Finish the migration kinds 12 and 13 began.** *Done 2026-09-08 (Opus 5, session
   `session_01K1PtKNZtSDU9XGwKZQ7ZzF`).* Relation-state facts now have an incremental update
   path and are emitted for **both** endpoints. Three changes in
   `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`: the `relation-formed` handler asserts edge facts
   for `relation->a()` and `relation->b()` instead of calling `seedStateFacts` on the Relation
   being; `seedStateFacts`' edge loop matches a being at **either** end; and
   `compileConditionsToRete` back-seeds the edges that already existed whenever a relation type
   enters the vocabulary for the first time. Guarded by `tests/law/rete_relation_state_test.cpp`
   — each of the three was independently reverted and the corresponding section confirmed red.
   Still open, and still coordinating with the To-Do item *"Idle tick is O(beings) due to
   per-frame Rete fact seeding — move seeding to admission"*: admission is the right home for
   relation formation too, and the back-seed is deliberately kept off the per-tick path.
1. ⚠️ **Give quantifiers a real index** — defect (b). *Measured 2026-09-09 (Opus 5, session
   `session_01F9nK3FZ7VR4PFPTUWfYyvm`). The measurement changed the conclusion, and the rung is
   left MEASURED AND BOUNDED rather than fixed.* Three things came out of it:

   **(i) The dominant quadratic was not the quantifier.** `ReteNetwork::retractFactsAbout`
   scans the whole fact table, and it is called from `Singular::notifyBeingReleased`, which
   fires for *every* `Singular` destructor — and `ECA::Event` carries `Moment timestamp{}` **by
   value** while `Moment` **is** a `Singular`. So every transient Event paid a full fact-table
   scan: one in `conditionsSatisfied`, one in `publishAppliedEvent`, and one **per alpha node
   per fact** in the `ECA::Event dummy` inside the compiled alpha predicate
   (`ConditionModel.cpp`). An ordinary WhileTrue `Compare` law with no quantifier anywhere
   fitted **k = 2.00** against population. Fixed with a participant set
   (`ReteNetwork::_factParticipants`) so the call answers "this being never had facts" in O(1):
   **320 beings, 593 ms/tick → 63 ms/tick, k 2.00 → ~1.5.** Engine-wide, not law-shape specific.

   **(ii) The quantifier penalty is real, and smaller.** With that removed, a bare `ForAll`
   fits **k ≈ 1.83** against an identical `Compare` law at **≈ 1.50** — about 5.5× slower at 320
   beings, with the gap widening in N. Two measurements had to be thrown away first: one whose
   action `set` a constant, so `propertyValueUnchanged` kept the fact table quiet and the
   per-fact predicate never ran again; and one using `ForAny` over a population where every
   being satisfied the inner condition, so it short-circuited on the **first** being and never
   scanned. `ForAll` over a satisfying population is the shape that actually walks N.

   **(iii) No index can remove it, and trying made it worse.** The cost is in **evaluation**,
   not candidate selection: `Law::applyTo` re-evaluates `conditionsSatisfied` per subject, and
   that re-check is precisely what makes a widened candidate set safe (§6). Dropping a bare
   quantifier from the index was measured at **413 → 718 ms** at 320 beings — losing terminals
   sends the law to the sweep, which evaluates the condition *twice* per subject, once in
   `tick()`'s continuous loop and again inside `applyTo`. What was kept is narrow: a quantifier
   **conjunct** is skipped inside `All` (it is a constant, not a filter — `targetAttr` is set
   only for `Compare` and `Related`, so its alpha admits or rejects every fact together), with a
   guard for the case where *every* conjunct is skipped and `currents` would otherwise return
   the sentinel `0` as though it were a node id. A quantifier **disjunct** inside `Any`, and a
   **bare** quantifier, keep their node — measured, that is the cheaper of two bad options.

   Guarded by `tests/law/quantifier_scaling_test.cpp`, which asserts the control has not gone
   quadratic again and that the quantifier gap does not widen.

1b. **The memo that would actually fix (ii) — BLOCKED, not merely unwritten.** A quantifier's
   answer is subject-independent, so it could be computed once per world-state and reused,
   keyed on a revision that bumps on any property write. **Do not build this yet.**
   `PropertyPath.cpp` states plainly that a direct C++ setter (`obj.setPosition(...)`) writes
   the transform without passing through the property vocabulary at all — *"it is the boundary,
   not an oversight"*. A memo keyed on property writes would therefore go stale on exactly those
   writes and answer a quantifier falsely, which makes laws deaf: the narrowing
   `PROPHETIC_RETE.md` §2 forbids. The precondition is complete write coverage, and that is a
   rung of its own.
2. ⚠️ **Categories as authored Formations**, replacing the implicit `couldApplyTo` vocabulary
   filter. *Built 2026-09-09 (Opus 5, session `session_01F9nK3FZ7VR4PFPTUWfYyvm`) — the INDEX
   half. The Formation half is blocked; see below.*

   **Measured before building, as rung 1 taught.** With 8 `OnBecomeTrue` laws over 1000 beings
   of which only 8 could ever match, `sweepSubjects` cost **7.6 ms/tick and fitted k = 0.82
   against POPULATION with the matching set held constant** — the sweep paid for the whole world
   to find eight beings, and ~99% of it could not have matched. Against law count, k = 0.92. So
   the O(L×N) §3.0 describes is real.

   **Built:** a vocabulary index on `LawManager`, one entry per property NAME some law requires,
   holding the beings that carry it. Rebuilt when `Universe::structuralRevision()` moves or when
   the set of required names changes; `sweepSubjects` seeds from the **rarest** required name and
   filters that, instead of walking the world. Only names laws ask for are indexed — Magic Sets
   in miniature (§4C), the goal restricting what the engine bothers to know.
   **Measured after: 7.6 → 4.6 ms/tick at 8 laws, 15.2 → 8.7 ms at 16, k against population
   0.82 → 0.69.** The residual O(N) is the per-frame `seedStateFacts` pass over all beings —
   already on the To-Do list as *"move seeding to admission"*, and the next thing worth removing.

   **§3.0's IMPOSSIBLE-only rule is honored by construction.** The index is built with
   `beingCarriesProperty`, the *same* predicate `couldApplyTo` uses — extracted and named once so
   the two cannot drift, the same reasoning as `ReteNetwork::alphaFeedsAnyBeta`. A name absent
   from the index means nobody carries it, which is a provably-IMPOSSIBLE narrowing rather than an
   observation about current members. And `sweepSubjects` still runs `couldApplyTo` over whatever
   the index proposes: **the index only proposes.**

   `refreshVocabularyIndex()` is `const` over `mutable` state and `sweepSubjects` calls it
   itself, so correctness cannot depend on call order — a stale index would return `{}` for an
   unindexed name, which is a law reaching nobody.

   **Prerequisite fixed:** `Zone::removeObject` did not bump `structuralRevision()` — only the
   unmaking path did. The counter had **no readers at all** before this rung, so nothing had ever
   noticed. The index holds raw pointers, so that hole was a dangling read, not a stale answer.

   **NOT a Formation, and blocked rather than skipped.** §3.4 records why: under Zach's revised
   definition a purely branching taxonomy is not a Formation, `RelationManager::add` refuses the
   `subcategory-of` cycles that would close the loop, and the bridge needs concept-Singulars.
   Separately, `Formation::addMember` walks the relation graph per member
   (`reintegrateRelationsFor`), which a per-structural-change rebuild cannot afford. The index is
   therefore Kernel-tier derived state, named as such in `Law.hpp` per Refusal 6, until the
   concept-Singular bridge exists.

   Guarded by `tests/law/vocabulary_index_test.cpp` (seven worlds an index gets wrong) and
   `tests/law/category_index_scaling_test.cpp` (the measurement).
3. **Alpha subscription for named `@referents`.** O(1) per referent, no joins; removes the
   widening at `ConditionModel.cpp:500`.
4. **Category-level overlap** (§3.1), including the quantitative subkind via the existing
   `Range::mayIntersect`.
5. **The instance-side slow adapter** — capped, two-rate clock, candidates only.
6. **Reified path Relations** (§3.2), then **Law-as-traverser** (§3.3) with magic-set
   restriction.
7. **Departure reporting on the reactive path.** The sweep currently owns edge detection
   ("Edge detection requires knowing when a being LEAVES the match set", `Law.cpp:1844`);
   the WhileTrue path already diffs `conditionMemory` for releases, so this is reachable —
   but it is a prerequisite for reducing sweep frequency, and no earlier plan listed it.

Steps 0 and 3 alone remove most of what motivated the original audit. Step 6 may well
conclude that Beta nodes are never worth reifying, since the language cannot express the
joins they exist for.

---

## 9. Open questions — ⚑ AUTHOR, Zach's decisions

1. **The distance function.** What is the metric across the three overlap subkinds
   (property, kind, quantitative)? One unified function, or three layered indices? Does it
   satisfy the triangle inequality closely enough for greedy traversal not to stall?
2. **What `Relation::weight` means.** Value (strength/telos, in the sense of the Hierarchy
   of Joys) or cost (fan-out)? §5 argues for two axes; if weight is strength, traversal cost
   needs its own home. Zach is *"not too sure yet"* and leaning toward strength.
3. ~~**The sweep schedule.**~~ **ANSWERED — Zach, 2026-09-07: on structural revision.**
   `Universe::structuralRevision()` (`Universe.hpp:241`) is the signal: sweep when the counter
   has moved past what the index was built at, otherwise trust the index. Verified before
   relying on it (Opus 5, 2026-09-08): it is bumped at exactly four sites — first grant of a
   dynamic property (`Singular.cpp:330`), removal of one (`Singular.cpp:359`), admission to a
   Zone (`Zone.cpp:354`), and reaping (`Law.cpp`, `reapUnmadeBeings`) — and deliberately **not**
   on a property's value changing, which is the separate change feed. That is exactly the shape
   of "which beings carry a property named X".
   **Two things a later rung must handle before depending on it:** it has **no readers at all
   today**, so Formation Rete would be its first consumer; and `Zone::removeObject` does **not**
   bump it, so a being removed outside the unmaking path leaves the counter still. Since
   Formations hold raw pointers, that is a dangling read rather than merely a stale one. Rung 2
   prerequisite. The maximum-staleness half of the question is still open.
4. **The stratification rule** (§7): which layer's edges may be read but not walked.
5. **Hysteresis on derived relations.** A `Near`-style relation needs a threshold and will
   chatter at the boundary; the standard fix is a two-threshold band (enter at *r*, leave at
   *r+ε*). Is that band **authored on the relation type**? It is an authorial decision, not
   a subsystem detail.
6. **Zach has further caveats not yet recorded here.** This section is open, not closed.

---

## 10. Verification record

| Claim | Status |
|---|---|
| Directional Blindness does not reproduce (all three activation modes) | verified by reading `Law.cpp` 382, 1778, 1843–1844; `retractFirst`; `collectTerminalSubjects` |
| Qualified roots name one being; no free variables | verified — `ActionModel.cpp:206` |
| `Related` deaf to runtime-formed relations | **FIXED 2026-09-08**, rung 0. Was probe-proven; the probe no longer existed on disk (`scratch/` is ignored) and has been replaced by the registered test `tests/law/rete_relation_state_test.cpp`, which reproduces the same witness — `shape.fillet` 0.500 when the law hears, 0.000 when deaf — and additionally covers the far-endpoint and late-authored-law shapes |
| Pair quantifiers existed and were burned | verified — `ConditionModel.hpp:37`, and the `fromJson` audit-log message |
| Quantifiers compile to an unfiltered Universe-scanning alpha | **MEASURED 2026-09-09** — real but not dominant: bare `ForAll` k ≈ 1.83 against an identical `Compare` at ≈ 1.50, ~5.5× at 320 beings. Not removable by indexing; see §8 rung 1 |
| **The engine's dominant quadratic was transient `Moment` destruction, not quantifiers** | **MEASURED AND FIXED 2026-09-09** — `ECA::Event` holds a `Moment` by value, `Moment` is a `Singular`, and every `Singular` destructor scanned the whole fact table through `retractFactsAbout`. A plain `Compare` law fitted k = 2.00. 593 ms → 63 ms per tick at 320 beings. Guarded by `tests/law/quantifier_scaling_test.cpp` |
| Structural support for Layers 2–3 (Relations of Relations, Formations of Relations) | verified — `Relation.hpp:46`, `Formation::addMember`, `relationTypeTag` |
| Prophetic index keeps quantified conditions reachable on the reactive path | verified — `PropheticRete.cpp:496–506`: demands do not propagate, `readNames` do |

**2026-09-09, rung 2.** A **fourth** deafness of the same family as §1.2(a), found by
`vocabulary_index_test` §B and confirmed pre-existing by re-running it against a clean tree:
`seedStateFacts` snapshots a being's properties **once per being, ever**, and
`markFactDirty` only dirties facts that already exist — so a property **granted at runtime** had
no fact to dirty and never acquired one. A `WhileTrue` law reading that property stayed
permanently deaf to that being, silently. Fixed: `markFactDirty` now reports whether it marked
anything, and the property-change hook asserts the missing state fact for an already-seeded being.
The scan was already linear over the fact list, so the answer costs nothing.

Also learned, and worth stating because it is easy to assume otherwise:
`Law::rebuildRequiredProperties` collects paths from the **action** as well as the condition, and
keys on each path's **root** segment. A law's stated vocabulary is what it reads *and writes*, so
an `IsKind` law still requires `shape` if its action writes `shape.fillet`.

*Update 2026-09-08.* The probe was a scratch file excluded from the build glob and is now
gone; rung 0 landed `tests/law/rete_relation_state_test.cpp` in its place, as this section
asked. Each of rung 0's three changes was reverted individually and the matching test section
confirmed red, so the test guards the code paths rather than merely passing beside them.

Two corrections to this document's own record, found while doing the work:

- **A relation-removal event does exist.** It is `relation-destroyed`, published from four sites
  in `RelationManager.cpp` and handled at `Law.cpp`. But it carries the **Relation** as its
  subject, so `retractStateFactsBySubject` drops facts keyed on the Relation and never on either
  endpoint — the same wrong-subject error as `relation-formed` had. This is left alone on
  purpose: a stale edge fact only *wakes* an alpha, and the compiled predicate behind it re-reads
  the live graph and answers false, so it widens (§6 permits) and never fires falsely. Retracting
  it "properly" would be the dangerous change, because facts carry no relation identity — two
  edges of the same type from one being share one fact shape, and dropping it on the first
  dissolution would silence the second. That would be a **narrowing**. If exact retraction is
  ever wanted, edge facts must carry the relation's identifier first.
- **`assertFact` does not deduplicate.** It pushes a new fact and a new id every call. With three
  paths now asserting edge facts, rung 0 added an idempotence check
  (`ReteNetwork::hasRelationStateFact`, a pointer compare that never dereferences a possibly-dead
  endpoint) so duplicates cannot stack into every matching alpha memory. Skipping an
  already-live fact is not a narrowing: the fact it would add is already in those memories.

**The suite is 112 registered tests, not 76.** Five fail on this branch — `smooth_tessellation_cache_test`
(known, Bugs.md #11), `chess_extended_rules_test`, `synthesis_studio_app_test`,
`zone_boot_hydration_relations_test`, and `frame_lag_test`. All five were confirmed failing on a
clean tree with rung 0 stashed, so none is rung 0's. `LawManager::tick` measured **0.154 ms
against a 1.653 ms baseline** with the fix in.

---

*Rungs 1 and 2 by Claude Opus 5, session `session_01F9nK3FZ7VR4PFPTUWfYyvm`, 2026-09-09.*
*Rung 0 implemented, and §3.4 / §9.3 / §10 updated, by Claude Opus 5, session
`session_01K1PtKNZtSDU9XGwKZQ7ZzF`, 2026-09-08.*
*Revised by Claude Opus 5, session `01Jf1mZyMWX69HHkG43qMv3F`, 2026-09-08T02:30:47-07:00.*
*First draft by Antigravity, session `b7b980a8-6cb5-452f-a382-0c554ebd1d69`, 2026-09-03T22:42:13-07:00.*
*Architecture by Zach, 2026-09-03 and 2026-09-04.*
