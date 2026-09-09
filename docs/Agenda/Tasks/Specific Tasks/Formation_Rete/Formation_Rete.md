# Formation Rete

**Status:** rungs 0 and 1 of 7 done (2026-09-08, 2026-09-09). Rungs 2–7 specified.
**Spec:** [`docs/architecture/law/FORMATION_RETE.md`](../../../../architecture/law/FORMATION_RETE.md) — §8 holds the rung ladder.
**Architecture:** Zach, 2026-09-03 / 09-04. First draft Antigravity. Revised and implemented by Claude Opus 5.

---

## Rung 0 — ✅ done 2026-09-08

*Claude Opus 5, session `session_01K1PtKNZtSDU9XGwKZQ7ZzF`.*

Finished the migration `ConditionNode::Kind` 12 and 13 began. Earthcall burned the pair
quantifiers *"in favour of modelling pairs as Relations"* — so `Related` is the designated
answer to multi-subject joins, and it was **permanently, silently deaf** to any relation
formed after a being's first tick.

Three defects, all in `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`:

1. `relation-formed` publishes the **Relation** as its subject, and the handler called
   `seedStateFacts(e.subject)` — which snapshots the Relation's own properties and emits no
   edge fact for either endpoint. It now asserts edge facts for `relation->a()` and
   `relation->b()` directly.
2. Edge facts were emitted only where `relation->a() == being`, so the network could traverse
   a→b and never b→a — the one structural gap the spec's §2 names. The loop now matches a being
   at **either** end (pointer compare; the far endpoint is never dereferenced, which is what
   `control_patterns_test` would otherwise crash on).
3. `_relationTypesInPlay` is filled at compile time, so a law authored **after** the world was
   seeded named a relation type nobody had emitted facts for, and was deaf to every edge that
   already existed. `compileConditionsToRete` now back-seeds the types it is the first to name.

There was no safety net for any of it: a WhileTrue law with a `Related` condition compiles Rete
terminals, so it never falls through to the sweep, never receives a candidate, and never
re-checks.

**Guarded by** `tests/law/rete_relation_state_test.cpp`. Defect docs from the audit session: [Related deafness](../Related_conditions_deaf_to_runtime_formed_relations/Related_conditions_deaf_to_runtime_formed_relations.md) (now closed) and [quantifier scan](../Quantifier_conditions_compile_to_an_unfiltered_Universe_scan/Quantifier_conditions_compile_to_an_unfiltered_Universe_scan.md) (rung 1, open). Each of the three changes was reverted
individually and the matching section confirmed red — the test guards the code paths rather than
passing beside them.

**Two things found on the way, both recorded in the spec's §10:**

- `relation-destroyed` **does** exist (four sites in `RelationManager.cpp`) but has the same
  wrong-subject error. **Deliberately left alone** — a stale edge fact only wakes an alpha and
  the predicate re-reads the live graph, so it widens and never fires falsely. Exact retraction
  would be a *narrowing* until edge facts carry a relation identifier, because two edges of one
  type from one being share a fact shape.
- `assertFact` does not deduplicate. Rung 0 added `ReteNetwork::hasRelationStateFact` so three
  assert paths cannot stack duplicates into every matching alpha memory.

**Measured:** `LawManager::tick` at 0.154 ms against a 1.653 ms baseline.

## Rung 1 — ✅ measured 2026-09-09, and it changed the answer

*Claude Opus 5, session `session_01F9nK3FZ7VR4PFPTUWfYyvm`.* The spec marked §1.2(b) *"read, not
measured"* and said measure first. Doing that produced three findings, in order of size.

**(i) The dominant quadratic was not the quantifier — it was every law.** `ECA::Event` carries
`Moment timestamp{}` **by value**, and `Moment` **is** a `Singular`. Every `Singular` destructor
calls `Singular::notifyBeingReleased` → `ReteNetwork::retractFactsAbout`, which scans the whole
fact table. So every transient `Event` paid a full scan: one in `conditionsSatisfied`, one in
`publishAppliedEvent`, and one **per alpha node per fact** in the `ECA::Event dummy` inside the
compiled alpha predicate. An ordinary `WhileTrue` `Compare` law with no quantifier anywhere
fitted **k = 2.00** against population.

Fixed with `ReteNetwork::_factParticipants`, a deliberate superset so the call can answer "this
being never had facts" in O(1). **320 beings: 593 ms/tick → 63 ms/tick, k 2.00 → ~1.5.**
Engine-wide — it is paid by every law application in every world.

**(ii) The quantifier penalty is real, and smaller.** Bare `ForAll` k ≈ 1.83 against an identical
`Compare` at ≈ 1.50; ~5.5× at 320 beings, widening in N.

**(iii) It is not removable by indexing, and trying made it worse.** The cost is in *evaluation*,
not candidate selection — `applyTo` re-evaluates `conditionsSatisfied` per subject, and that
re-check is exactly what makes a widened candidate set safe. Dropping a bare quantifier from the
index measured **413 → 718 ms**: losing terminals sends the law to the sweep, which evaluates the
condition *twice* per subject. Kept narrowly: quantifier **conjuncts** are skipped inside `All`
(they are constants, not filters), with a guard for the all-conjuncts-skipped case that would
otherwise return the sentinel `0` as a node id. Disjuncts and bare quantifiers keep their node.

**Two measurements were wrong before they were right,** and both would have flattered the engine:
the first `set` a constant, so `propertyValueUnchanged` kept the fact table quiet and the per-fact
predicate never ran again; the second used `ForAny` over a population where every being satisfied
the inner condition, so it short-circuited on the first being and never scanned.

**Guarded by** `tests/law/quantifier_scaling_test.cpp` — asserts the control has not gone
quadratic again, and that the quantifier gap does not widen. Thresholds are loose on purpose: the
fitted exponent is machine-load sensitive (1.43–1.54 quiet, 1.68 under a concurrent build).

**Rung 1b — the real quantifier fix, BLOCKED not unwritten.** A quantifier's answer is
subject-independent and could be memoized on a world-revision key. Do not build it yet:
`PropertyPath.cpp` states that a direct C++ setter (`obj.setPosition(...)`) bypasses the property
vocabulary entirely — *"the boundary, not an oversight"* — so a memo keyed on property writes goes
stale on those writes and makes laws deaf. That is the narrowing `PROPHETIC_RETE.md` §2 forbids.
The precondition is complete write coverage.

## Next rungs

2. **Categories as authored Formations**, replacing `couldApplyTo`'s implicit vocabulary filter.
   Blocked by two things now written down: Zach's revised Formation definition means a taxonomy
   of `instance-of`/`subcategory-of` is *purely branching* and therefore **not a Formation**, and
   `RelationManager::add` rejects cycles in `subcategory-of`, so the loop cannot be closed from
   inside the taxonomy — it needs the concept-Singular bridge (`ObjectConcept`, whose
   `RelationTemplate::bAnchorId` is already "relate to this concept in advance"). Also needs
   `Zone::removeObject` to bump `Universe::structuralRevision()`, which today it does not.
3–7. Alpha subscription for named `@referents`; category-level overlap; the instance-side slow
   adapter; reified path Relations and Law-as-traverser; departure reporting on the reactive path.

## ⚑ AUTHOR — open, Zach's

§9 of the spec. **§9.3 (the sweep schedule) is answered — on structural revision, 2026-09-07.**
Still open: the distance function (9.1), what `Relation::weight` means (9.2 — value or cost;
Zach leaning strength), the stratification rule (9.4), hysteresis bands on derived relations
(9.5), and 9.6, which Zach marked open rather than closed.
