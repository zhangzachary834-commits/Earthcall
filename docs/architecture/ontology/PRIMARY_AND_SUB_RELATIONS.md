# Primary Relations and Sub-Relations

**A Relation between two Singulars does not disappear. What is part of it may.**

**Status:** Principle stated by Zach, 2026-09-15. Not yet built. The engine currently erases
Relations outright (§5), so this document describes a direction the tree has not taken yet, not
behaviour it has. Four ⚑ AUTHOR questions (§6) stand between the principle and a first rung.
**Origin:** Zach, answering Formation Rete §9.1(d) on 2026-09-15, when asked what happens to
routing Relations discovered by the slow adapter that later go stale. He stated it as a principle
**for Earthcall**, not only for routing. Recorded and cross-read against the tree by Claude Opus 5
(session `session_01JE2AguCX12mpJ9YwFUqgmQ`). Every "reading" or "extension" below is mine and
marked as such.
**Companion docs:** `law/FORMATION_RETE.md` §9.1 (where it was stated, and the relevance-graph
answer it completes), `law/PROPHETIC_RETE.md` §2 (the IMPOSSIBLE-only rule, which is exactly this
principle's dissolution test), `mathematics/ONTOMATH_FRAMEWORK.md` §6 (the past integrated rather
than replayed), `ontology/TIME_AND_MOMENT.md` (what a *when* is, for the history this keeps),
`ontology/NO_BLACK_BOX.md` (history and priority must be registered, not hidden).

---

## 0. What this is not

**Not garbage collection.** Nothing here decides a Relation is unused and reclaims it. Disuse is
never a reason for a sub-Relation to go, let alone a primary one.

**Not a ban on forgetting derived state.** Rete facts, indices, caches, the endpoint register:
these are the engine's derived views of Relations, not Relations. They are dropped and rebuilt
freely. Formation Rete rung 4 retracts an edge *fact* when the edge is gone. That is not a
violation of this principle, because no Relation was removed.

**Not an event log.** "Keeps history" does not mean replaying a list of every change to recover
the past. `ONTOMATH_FRAMEWORK.md` §6 already refuses that shape for continuous state. What the
primary Relation keeps is its sub-Relations' record, which it can consult if one becomes relevant
again (§3).

---

## 1. The principle, in Zach's words

> My principle for Earthcall is the primary Relation between two Singulars don't disappear—only
> their sub-Relations do, the ones that are part of the primary Relation, and this allows for a
> hybrid approach. The sub-relations that go stale because their foundational premise changed
> (e.g. if a relevancy sub-Relation was necessarily-conditioned on a certain fact like "____
> polyhedron is red") and that polyhedron is now blue) in a way that makes it logically impossible
> should disappear. The primary relation keeps history in case it ever needs to be used again.
> Ones who merely become less optimal but whose foundational premise that determined it was
> initially relevant gets deprioritized but not removed.

And an earlier statement of his that this one completes, left as a note in
`src/ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp` beside `AddRelation`:

> "Relations" cannot be created unless an actual interaction happens—there's no such thing as an
> "empty" Relation. That would be philosophically nonsensical. If a Law creates a Relation, it is
> creating an interaction.

*Reading (Opus 5):* the two belong together. A Relation begins with a real interaction and
therefore has a real past. Erasing it later would assert that the interaction never happened, the
same nonsense the first note rejects, in the other direction.

---

## 2. The three outcomes

| What | When | Outcome |
|---|---|---|
| **Primary Relation** between two Singulars | always | **Persists.** Never removed. Keeps the history of its sub-Relations. |
| **Sub-Relation**, founding premise now **logically impossible** | e.g. a relevance sub-Relation necessarily conditioned on "this polyhedron is red", and it is now blue | **Dissolved.** It leaves the primary's live structure; the primary's history records that it was. |
| **Sub-Relation**, merely **less optimal**, founding premise still possible | a better route or stronger reason has appeared, but the original reason still holds | **Deprioritised, not removed.** |

The line between rows two and three is **logical impossibility, not usefulness.** A sub-Relation
that is no longer the best choice stays. Only one whose reason for existing *cannot* hold goes.

"Hybrid" is Zach's word for this. It is neither pure append-only (nothing ever changes) nor pure
mutation (the present overwrites the past). Structure the world can no longer support is removed
from the live graph, and the fact that it existed is kept.

---

## 3. Why it is shaped this way

*Zach's reason, from §1:* a primary Relation "keeps history in case it ever needs to be used
again." A route discovered once, or a reason that was once true, has value the next time the world
returns to that state. Deleting it makes the world pay to rediscover it.

*Extension (Opus 5), three connections already present in the tree:*

**The dissolution test is the one soundness rule Earthcall already enforces.**
`PROPHETIC_RETE.md` §2 allows analysis to conclude only IMPOSSIBLE, never "unlikely" or "not
needed", because a too-narrow answer makes a law silently deaf. This principle draws the same line
through Relations: dissolve on proved impossibility, never on estimated uselessness. Earthcall
therefore has one rule for when something may be dropped, not two. It also means the machinery for
proving a premise impossible, the abstract interpreter over authored conditions and OntoMath
ranges, already exists.

**Deprioritisation is where `Relation::weight` lands.** Row three needs an ordering among
sub-Relations that are all still valid. That is the question Formation Rete §9.2 holds open (is
weight value or cost?), and Zach tied §9.1(c) to it. This principle does not answer §9.2. It adds
a requirement: whatever weight means, lowering it must be how "less optimal" is expressed, because
removal is not available.

**It makes the relevance graph safe to let improve itself.** The Formation Rete slow adapter is a
First Mover (Zach, §9.1(a)) that discovers and retains routing Relations. A self-improving
structure that could delete its own findings could also delete a Person-authored one or a route
the world will need again. Under this principle it can only add, deprioritise, or dissolve on
proof.

---

## 4. What a "premise" is

*Reading (Opus 5), to make row two testable. Not decided.*

A sub-Relation's **foundational premise** is the condition that made it true or relevant when it
formed, such as "this polyhedron is red". Zach's phrase is **necessarily-conditioned**: the
sub-Relation holds *because of* that fact, not merely alongside it.

For the dissolution test to be decidable, the premise has to be **stated as data on the
sub-Relation**, not inferred afterwards. The natural carrier already exists: the premise is a
`ConditionModel`, the same authored condition trees Laws use. That makes it:
- readable by the Prophetic analysis (§3), which can prove it unsatisfiable;
- serialisable with the Relation, so a save keeps why each sub-Relation existed;
- legible and governable, as `NO_BLACK_BOX.md` requires of anything a being carries.

The distinction that matters: a premise that is **currently false** is not the same as one that is
**logically impossible**. "The polyhedron is red" is false while it is blue. It becomes impossible
only if nothing in the world can ever make it red again. §6(b) asks which one Zach means.

---

## 5. The tree today — verified 2026-09-15

What exists, and where it disagrees with the principle:

| Fact | Where | Bearing |
|---|---|---|
| A Relation's identity **is** its two endpoints plus its kind | `src/Relation/Relation.hpp` header comment | Two kinds between the same pair are two separate Relations today. There is no "the" primary Relation between two Singulars, which is §6(a). |
| A Relation is a `Singular` | `class Relation : public Singular` | A Relation can be an endpoint of another Relation, so sub-Relations are expressible without a new class (Refusal 1). |
| A Relation already carries a history | `std::vector<RelationEvent> events` (timestamp, description, `deltaWeight`), registered as the `events` property | A seed for "keeps history". It records weight changes, not sub-Relations. |
| Relations are **erased outright** | `RelationManager::remove`, both `removeBetween` overloads, `removeInvolving`, each publishing `relation-destroyed` | Directly contrary to row one. |
| Callers that erase | `Formation.cpp` (`removeInvolving` when a member leaves; `remove` in two places), the Relations console's **Break** button | Each is a place a primary Relation can vanish today. |
| A freed endpoint keeps its name | `Relation::Endpoint::forget` nulls the pointer, keeps `savedId` | Already in the spirit of the principle: a being's death does not erase the Relation. |
| No law action removes a Relation | `ActionNode::Kind` has `AddRelation = 20` and no removal kind | Laws can only create interactions today. |
| `Relationship` is a Relation between two Persons | `src/Person/Relationship/Relationship.hpp` | The case where erasing a Relation would most plainly erase something human. |

---

## 6. ⚑ AUTHOR — open, Zach's

**(a) What is the primary Relation between two Singulars?** Either exactly one per pair, with every
kind-specific Relation between them (`instance-of`, `touching`, a relevance route) its
sub-Relation, or one primary per kind. The first reads most naturally from "*the* primary Relation
between two Singulars". It would change the identity rule in `Relation.hpp`, which says kind is part
of a Relation's identity.

**(b) "Logically impossible": for ever, or given the world as it now stands?** A premise false now
but reachable again (the polyhedron can be repainted) vs a premise no authored Law or First Mover
can ever restore. `PROPHETIC_RETE.md` §2 proves the second kind and is careful to scope it to
*authored* law, since First Movers and foreign channels can move properties the analysis cannot
see. The stricter reading dissolves almost nothing; the looser one needs a rule for when "not now"
becomes "not ever".

**(c) Can a Person sever a primary Relation?** The Relations console's **Break**, and any future
authored removal. Is that a real removal a Person may choose, a dissolution of sub-Relations with
the primary kept, or something the kernel refuses? A Person ending a `Relationship` is the sharpest
form of this question.

**(d) What does the primary keep about a dissolved sub-Relation?** The whole sub-Relation (premise,
weight, endpoints), or a record that it existed and why it ended? The first allows reinstatement
without rediscovery, which is what "in case it ever needs to be used again" suggests. The second is
smaller.

---

## 7. For whoever builds the first rung (Jules especially)

Everything you need to know:
- **Don't add new ways to erase a Relation.** Every existing one (§5) is a place this principle is
  broken today. Adding a fifth makes the migration larger.
- **Don't remove the four erase paths yet either.** Until §6(a) and §6(c) are answered, nothing
  says what they should do instead, and chess, Go and Formations depend on them.
- **Derived state is not a Relation.** Retracting a Rete fact, rebuilding an index or clearing a
  cache about an edge is fine and does not touch this principle.
- **No new class for "SubRelation" or "PrimaryRelation".** A Relation is a Singular and can relate
  Relations. The distinction should be an authored kind or category of Relation
  (`ontology/AUTHORED_CATEGORIES.md`), not a C++ type (Refusal 1) or an enum (Refusal 3).
- **The dissolution trigger must prove impossibility, never guess.** Reuse the Prophetic analysis.
  A heuristic "probably stale" test would recreate the silent-loss failure this principle exists to
  prevent.

Pitfalls to avoid:
- Treating "false right now" as "impossible" (§6(b)).
- Letting deprioritisation become de facto deletion by pruning everything below some weight.
- Storing history in a place no property path reaches: `events` is registered; a new store must be
  too (`NO_BLACK_BOX.md`).

To-do: `docs/Agenda/Tasks/To-do list.md`, the ⚑ bullet on primary Relations.

---

*Principle: Zach, 2026-09-15. Document, verification against the tree, and marked extensions:
Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`, 2026-09-15.*
