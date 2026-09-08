# Formation Rete

**Status:** rung 0 of 7 built and green (2026-09-08). Rungs 1–7 specified.
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

## Next rungs

1. **Quantifiers** — §1.2(b): `ForAny`/`ForAll` compile to an alpha with no attribute filter
   whose predicate scans the whole Universe, and on the sweep path give O(N²) per tick. The spec
   marks it *read, not measured*; **measure with the `lag` target before optimizing**.
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
