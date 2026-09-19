# Primary Relations and Sub-Relations

**A Relation between two Singulars does not disappear. What is part of it may.**

**Status:** Principle stated by Zach, 2026-09-15; its four open questions answered by him
2026-09-16 (§6). Not yet built. The engine currently erases
Relations outright (§5), so this document describes a direction the tree has not taken yet, not
behaviour it has. §6 holds his answers and what each still leaves to design; those design choices, not the
principle, are what stands between this and a first rung.
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
the past. `ONTOMATH_FRAMEWORK.md` §6 already refuses that shape for continuous state, and Zach's
§6(d) answer says the same here: what is kept is a **record that the sub-Relation existed** plus a
**mathematical model of its past**, not a tape of what happened to it.

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
| **Sub-Relation**, founding premise now **false as the world stands** (Zach, 2026-09-16: not "impossible for ever" — *"very few things are impossible forever in Earthcall"*) | a relevance sub-Relation conditioned on "this polyhedron is red", and it is now blue | **Re-kinded** (§6b, Zach 2026-09-16): it changes from its active kind to a historical one and stays connected to its primary. Only the ACTIVE VARIANT is deleted. |
| **Sub-Relation**, merely **less optimal**, founding premise still possible | a better route or stronger reason has appeared, but the original reason still holds | **Deprioritised, not removed.** |

The line between rows two and three is **logical impossibility, not usefulness.** A sub-Relation
that is no longer the best choice keeps its kind. Only one whose reason for existing *cannot* hold
is re-kinded out of the active set.

"Hybrid" is Zach's word for this. It is neither pure append-only (nothing ever changes) nor pure
mutation (the present overwrites the past). Structure the world can no longer support stops being
live — and, per §6(b), stops being live by CHANGING KIND rather than by being destroyed, so the
fact that it existed survives in the thing itself, still attached to its primary.

---

## 3. Why it is shaped this way

*Zach's reason, from §1:* a primary Relation "keeps history in case it ever needs to be used
again." A route discovered once, or a reason that was once true, has value the next time the world
returns to that state. Deleting it makes the world pay to rediscover it.

*Extension (Opus 5), three connections already present in the tree:*

**The test is "false as the world now stands", and that is affordable BECAUSE nothing is destroyed.**
Zach, 2026-09-16: *"by logically impossible when I said earlier I just meant false in the current
state not impossible forever (very few things are impossible forever in Earthcall)"*. A premise is
an authored `ConditionModel` (§4), so this is the same evaluation a law does every tick — no
abstract interpretation required. The reason a test this eager is safe is §6(b): a sub-Relation that
fails it changes kind and stays attached to its primary, so a premise that becomes true again can
restore it. Under deletion the same test would destroy structure that is about to be needed.

**Deprioritisation needs an ordering, and what carries it is unsettled.** Row three needs a way to
rank sub-Relations that are all still valid. `Relation::weight` was the obvious carrier — but
Formation Rete §9.2 holds its meaning open, and as of 2026-09-15 Zach is leaning toward **removing
weight entirely** as too vague. So this principle states the requirement without naming the field:
whatever expresses "less optimal" must not be removal, because removal is not available here.

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
only if nothing in the world can ever make it red again.

**ANSWERED — Zach, 2026-09-16:**

> by logically impossible when I said earlier I just meant false in the current state not impossible
> forever (very few things are impossible forever in Earthcall)

So the test is **the premise is false as the world now stands**, not "no future world could satisfy
it". That is decidable today: evaluate the premise's `ConditionModel` against the world, which is
what every law already does each tick.

*Reading (Opus 5):* this only works because §6(b) re-kinds rather than destroys. A "false right now"
test fires often — the polyhedron is blue this second and red the next — and under deletion that
would be catastrophic, erasing structure that is about to be true again. Under re-kinding it moves a
sub-Relation out of the active set and keeps it attached to its primary, where the same test can
move it back. **Deletion needed "impossible for ever"; re-kinding can afford "false now".** The two
answers hold each other up.

It also means this principle does NOT inherit `PROPHETIC_RETE.md` §2's IMPOSSIBLE-only rule, which
§3 above suggested it would. That rule governs an INDEX that may not narrow what a law can see.
Re-kinding is not an index: it is a change to the world that the world can undo, and the sweep
re-tests every premise anyway.

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

## 6. Zach's answers, 2026-09-16

The four questions §6 held open are answered. His words first, then what each one lands on in the
tree (verified, Opus 5) and what it still leaves to design — marked, so nothing of mine is mistaken
for his.

### (a) What is the primary Relation?

> The primary relation would simply be "Primary" kind and the others would be constitutive of it.
> Constitutiveness could be modeled in various ways: a property that is a Formation of the sub
> Relations, or a hierarchical Singular graph of those Relations, or a linear authored property
> vector of them.

So the primary is **not a new C++ concept**: it is a Relation whose KIND is `Primary`, and the
sub-Relations are **constitutive of** it. That keeps Refusals 1 and 3 — an authored, Lexeme-grounded
kind, not a class and not an enum member — and it answers the identity worry §5 raised: a Relation's
identity is its endpoints and its kind, and `Primary` is simply the kind that stands for "these two
beings are related at all", with the specific kinds hanging under it.

*In the tree already (Opus 5):* **constitutiveness is not new here either.** `Relation` carries
`ConstitutiveOpcode` / `ConstitutiveStatus` and `evaluateConstitutive()`, which reads an authored
opcode off the relation kind's **Lexeme** (`relation.constitutiveOpcode`) and answers Holds /
Violated / Invalid / NotApplicable. One opcode exists today (`CppInheritance`). A sub-Relation's
constitutive membership in its primary is the same shape: authored on the kind, evaluated against
the world, and legible.

*Left to design:* which of Zach's three carriers holds the sub-Relations — a Formation-valued
property, a graph of Relations-between-Relations, or an authored property vector. He listed them as
alternatives, not a choice. The Formation reading is the one that needs no new vocabulary
(`Formation` is already a Singular holding members, and a Relation is a Singular), but a Formation
of a relation's parts is a claim about what a Formation is, and that is Zach's to make.

### (b) What happens when a sub-Relation's premise becomes impossible?

> Either way, the sub relations should change in kind and preserve the history by being connected to
> the primary Relation. We only delete the active variants in both cases.

This **replaces "dissolved"** in §2 with something more careful, and it applies to BOTH rows — the
impossible one and the merely-less-optimal one. Nothing is destroyed: a sub-Relation whose premise
has failed **changes kind** — from the active kind to a past/historical one — and stays connected to
its primary. What is deleted is only the **active variant**: the live, matchable form. The record
remains, re-kinded.

*In the tree already (Opus 5):* a Relation's kind can change in place, and as of 2026-09-15 that
change **announces itself** — `setTypeLexeme` and a write to the `type` property both re-validate
the endpoints' Rete facts, so a law watching the active kind stops seeing a re-kinded relation on
the next tick, and one watching the historical kind starts. The mechanism a re-kinding needs is
therefore live, and the question "does the law notice?" is already answered by rung 7's work.

*Left to design:* the naming and shape of the historical kind (one `was-<kind>` per active kind, or
a single `past` kind carrying the former kind as a property), and whether "delete the active
variant" means removing the edge from the graph or removing it from what the active kind matches.

### (c) May a Person sever a primary Relation?

> This should be resolved by the same constitutive ownership and stakeholder framework used for the
> second person framework.

*In the tree already (Opus 5):* `ourverse/SECOND_PERSON_FRAMEWORK.md` §1 already splits exactly this
way — **constitutive** properties are closed by default (Body, Soul, anything constitutive of a
Person), while **authored/incidental** state is evaluated by a stakes framework comparing the
level-tier of the reader against that of the property. `Singular::_stakeholders` records who moved
what, by which law, when. So severance is not a new permission: a primary Relation between two
Persons is constitutive and closed by default; other primaries answer to stakes.

*Left to design:* the tier a primary Relation sits at, and what the Relations console's **Break**
button means under it (§5 lists it as one of the four paths that erase a Relation outright today).

### (d) What does the primary keep about a re-kinded sub-Relation?

> Record of its existence and a mathematical model of its past (OntoMath).

Not a log. A **record that it existed** plus **a closed-form model of how it behaved** — which is
exactly what `mathematics/ONTOMATH_FRAMEWORK.md` §6 already argues for the world's past generally:
integrate the authored rate, do not replay a history of events. A relation's past becomes a
Person-readable mathematical object rather than a tape.

*In the tree already (Opus 5):* `Relation::events` (`std::vector<RelationEvent>`: timestamp,
description, `deltaWeight`) is a log, and a registered property. It is the closest thing today and
it is the shape this answer moves away from — worth noting, since `deltaWeight` also depends on
`weight`, which Zach is leaning toward removing (§9.2 of `law/FORMATION_RETE.md`).

*Left to design:* which quantity the model is OF, once weight's future is settled.

## 7. For whoever builds the first rung (Jules especially)

Everything you need to know:
- **Don't add new ways to erase a Relation.** Every existing one (§5) is a place this principle is
  broken today. Adding a fifth makes the migration larger.
- **Don't remove the four erase paths yet either.** §6(a) and §6(c) are answered in principle, but
  the design choices under them are not made, and chess, Go and Formations depend on those paths.
- **A sub-Relation leaving the active set is a KIND CHANGE, not a deletion** (§6b). The mechanism
  exists and already announces itself: `Relation::setTypeLexeme` and a write to the `type` property
  both re-validate the endpoints' Rete facts (2026-09-15, Formation Rete rung 4's follow-up), so a
  law watching the active kind stops seeing it on the next tick.
- **Constitutiveness is authored on the relation KIND**, through the Lexeme
  (`relation.constitutiveOpcode`, `Relation::evaluateConstitutive`). That is where a sub-Relation's
  membership in its primary belongs — not in a C++ field on Relation.
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
