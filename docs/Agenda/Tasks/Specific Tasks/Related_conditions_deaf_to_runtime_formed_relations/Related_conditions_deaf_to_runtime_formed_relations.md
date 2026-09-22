# `Related` conditions are deaf to relations formed at runtime

**Status:** ✅ **FIXED 2026-09-08** by Claude Opus 5, session `session_01K1PtKNZtSDU9XGwKZQ7ZzF`,
as rung 0 of the Formation Rete ladder — see
[Formation Rete](../Formation_Rete/Formation_Rete.md) for the whole ladder, and
`tests/law/rete_relation_state_test.cpp` for the guard. The diagnosis below is unchanged and
was correct; three code paths carried the defect, not one:
the `relation-formed` handler (fixed by asserting edge facts for both endpoints directly),
`seedStateFacts`' `relation->a() != being` filter (widened to either end), and
`_relationTypesInPlay` being compile-time-only (now back-seeds the types a compile first names).
Each was reverted individually and the matching test section confirmed red.
Two adjacent findings are recorded in `FORMATION_RETE.md` §10: `relation-destroyed` has the same
wrong-subject error and is **deliberately left alone** (it widens, and exact retraction would
narrow until edge facts carry a relation identifier), and `assertFact` does not deduplicate, so
an idempotence check was added.

**Originally:** open. Found and **probe-proven** 2026-09-08 by Claude Opus 5, session
`01Jf1mZyMWX69HHkG43qMv3F`, while auditing whether the Rete can honestly evaluate
multi-subject conditions (Zach's question). Architecture context:
[FORMATION_RETE.md](../../../../architecture/law/FORMATION_RETE.md) §1.2(a), §8 step 0.

## The bug

`LawManager::seedStateFacts` is guarded by `_seededSubjects` (`Law.cpp:2209`) — a being is
seeded **once, ever**. Relation-state facts are emitted there, keyed
`subject = relation->a()`.

On `relation-formed`, the bus handler calls `seedStateFacts(e.subject)` (`Law.cpp:1527`) —
but `e.subject` is **the Relation being itself** (`RelationManager.cpp:167`), not its source
endpoint. The endpoint is already in `_seededSubjects`, so the call returns immediately.

**No relation-state fact is ever asserted for an edge formed after the first tick.**

## Why nothing catches it

A `WhileTrue` law with a `Related` condition compiles Rete terminals, so `hasTerminals` is
true and it **never falls through to the sweep** (`Law.cpp:1778`). It never receives a
candidate subject, so `applyTo` never runs, so nothing re-evaluates the condition. The law
is deaf permanently and silently — exactly what `PROPHETIC_RETE.md` §2 forbids.

## Evidence

`scratch/probes/rete_relation_blindness_probe.cpp` (scratch, excluded from the build glob):

```
[pre ] law FIRED        shape.fillet = 0.500   raw Related predicate: TRUE
[post] law stayed DEAF  shape.fillet = 0.000   raw Related predicate: TRUE
```

Identical law and graph; the only difference is whether the relation was formed before or
after the first tick.

## Related defects of the same shape

- `_relationTypesInPlay` is populated at compile time (`Law.cpp:2297`), so a law authored
  *after* beings were seeded names a relation type nobody ever seeded facts for.
- Relation-state facts are emitted only where `relation->a() == being`, so the network can
  traverse a→b and never b→a. Formation Rete requires bidirectionality (FORMATION_RETE.md §2).
- On the reactive path, onset and `conditionMemory` are stamped `true` for every terminal
  subject (`Law.cpp:1805–1815`) *before* `applyTo` re-checks, so a subject failing an
  `@`-rooted conjunct gets `time.sinceApplied` running for a law that is not holding.

## Fix shape

Give relation-state facts an incremental update path, emitting for **both** endpoints, on
relation formation *and* dissolution. Coordinate with the existing Performance item *"Idle
tick is O(beings) due to per-frame Rete fact seeding — move seeding to admission"*:
admission is the right home for relation formation too.

Land the probe as a real regression test in `tests/law/` as part of the fix.

## Historical note

`ConditionModel.hpp:37` records that kinds 12 and 13 — `ForAnyPair` / `ForAllPair` — were
retired *"in favour of modelling pairs as Relations."* This bug means the mechanism chosen
as the replacement for multi-subject joins was left half-wired. Fixing it is finishing that
migration, not patching an incidental defect.
