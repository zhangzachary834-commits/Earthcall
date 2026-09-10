# Formation Rete

**Status:** rungs 0–3 of 7 done (2026-09-08, 2026-09-09); rung 4 measured and deferred with a named precondition (2026-09-10). Rung 2's Formation half and rungs 5–7 specified.
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

## Rung 2 — ✅ the index half, 2026-09-09

*Claude Opus 5, session `session_01F9nK3FZ7VR4PFPTUWfYyvm`.* Measured first, as rung 1 taught.

**The waste, measured.** 8 `OnBecomeTrue` laws over 1000 beings where only 8 could ever match:
**7.6 ms/tick, k = 0.82 against POPULATION** with the matching set held fixed. The sweep cost the
whole world to find eight beings. Against law count, k = 0.92 — so O(L×N), exactly as §3.0 says.

**Built.** A vocabulary index on `LawManager`: one entry per property name some law requires,
holding the beings that carry it, rebuilt when `Universe::structuralRevision()` moves or the set
of required names changes. `sweepSubjects` seeds from the **rarest** required name — the cheap
metric-free stand-in for §5's fan-out cost model — and filters that instead of walking the world.
**After: 7.6 → 4.6 ms at 8 laws, 15.2 → 8.7 ms at 16, k 0.82 → 0.69.** The residual O(N) is the
per-frame `seedStateFacts` pass, already a To-Do item (*"move seeding to admission"*).

**Safety.** The index is built with `beingCarriesProperty`, the same predicate `couldApplyTo`
uses, extracted and named once so they cannot drift — if the index tested membership even
slightly differently it would omit candidates, and an omitted candidate is a silently deaf law.
`sweepSubjects` still runs `couldApplyTo` over whatever the index proposes: the index only
proposes. `refreshVocabularyIndex()` is `const` over `mutable` state and `sweepSubjects` calls it
itself, so correctness does not depend on call order.

**Prerequisite fixed.** `Zone::removeObject` never bumped `structuralRevision()` — only the
unmaking path did — and the counter had **no readers at all** before this rung, so nothing had
ever noticed. The index holds raw pointers, which made that a dangling read rather than a stale
answer.

**A fourth deafness found and fixed.** `vocabulary_index_test` §B failed on a clean tree:
`seedStateFacts` snapshots properties **once per being ever**, and `markFactDirty` only dirties
facts that already exist — so a property **granted at runtime** never acquired a fact, and a
`WhileTrue` law reading it stayed permanently deaf to that being. Same family as rung 0's
relation deafness. `markFactDirty` now reports whether it marked anything and the hook asserts
the missing fact; the scan was already linear, so the answer is free.

**Also learned:** `rebuildRequiredProperties` collects paths from the **action** as well as the
condition and keys on the path's **root** — a law's vocabulary is what it reads *and writes*.

**Still blocked (the Formation half).** Making these authored Category Formations needs §3.4's
concept-Singular bridge, and `Formation::addMember` walks the relation graph per member, which a
per-structural-change rebuild cannot afford. The index is Kernel-tier derived state, named as
such in `Law.hpp` per Refusal 6, until that bridge exists.

**Guarded by** `tests/law/vocabulary_index_test.cpp` (seven worlds an index gets wrong) and
`tests/law/category_index_scaling_test.cpp` (the measurement).

## Rung 3 — ✅ 2026-09-09, but not the shape the spec predicted

*Claude Opus 5, session `session_01F9nK3FZ7VR4PFPTUWfYyvm`.*

**Rejected by measurement first.** The obvious target was `resolveLawRoot`, which rebuilds
`Universe::beings()` and linear-scans it comparing identifier strings, per read per subject per
tick. Measured: a few ms of a tick costing hundreds. **Not optimized** — that would have been
treating a symptom nobody feels.

**The real cost was the widening.** Rung 1 established that a qualified root is
subject-independent: `@gate.open > 0` is one truth about the world. It can never narrow a
candidate set; it can only decide the law at once. When false the correct answer is **nobody** —
and the engine was discovering that one refusal per subject, every tick. A law behind a **shut**
gate cost **278 ms/tick at 480 beings, k = 1.67**, while firing nothing.

**Built.** `LawManager::gatesHold` evaluates the subject-independent conjuncts once and skips the
subject loop when one is false. **278 ms → 0.18 ms; k 1.67 → 0.72.**

**Refuses to hoist** — each would be a silent narrowing: `@event.*` (only meaningful inside
`applyTo`), `@world.*` (the reading is handed the subject), a gate under `Any` or `Not`, and any
law whose **action writes a qualified root** — it can move its own gate mid-sweep. Returns true
whenever it cannot prove otherwise.

**And it may not simply skip:** skipping the loop also skips releasing held subjects, so an
`OnBecomeTrue` law would come back from a shut gate still believing they held — no false→true
edge, never fires again. Release is O(held).

**A fifth deafness, and the oldest.** `gate_hoist_test` §B failed with rung 2 AND rung 3 both
disabled. A law's vocabulary is a path **root** (`shape`), but `Object` registers properties under
their whole dotted names (`shape.fillet`, `shape.r`) — **there is no property named `shape`**. So
`couldApplyTo` said no to every being, and **any sweep-path law touching `shape.*` reached
nobody**, silently. `ConditionModel` had already fixed this exact bug on the alpha path with a
`rootOf` helper; the sweep half never got it. `beingCarriesProperty` now matches dotted prefixes.

**Guarded by** `tests/law/gate_hoist_test.cpp` and `tests/law/referent_resolution_test.cpp`.

## Rung 4 — ⚠️ measured 2026-09-10, deliberately NOT built

*Claude Opus 5, session `session_01F9nK3FZ7VR4PFPTUWfYyvm`.* The most useful outcome available
here was a "no", and it protects a later pass from a real mistake.

**Overlap has no users.** Across every saved world, **zero laws conjoin two distinct categories**.
Overlap answers "can a being be in A and B at once"; nothing asks. The quantitative subkind via
`Range::mayIntersect` is genuinely half-built and should be finished when a law first wants it.

**Membership is the hot idiom** — **132 laws** scope themselves with
`Related(instance-of, category.chess.piece)`, against 4 for the next category. Measured against an
identical law of identical selectivity reading a plain property: **2.7x slower at 50 beings, 4.0x
at 400**, k 0.87 vs 0.67. The gap widens with the world.

**Two hypotheses tested and rejected:** by-value string ids in `isBetween` (rewritten to pointer
comparison — no change, inside noise) and per-call vector allocation in `Universe::relations()`
(buffer reuse — no change; reverted). **The cost is the O(relations) provider walk per evaluation**,
and no rewrite of the predicate removes it.

**Why no index: the invalidation signal does not exist.** `RelationManager` never bumps
`structuralRevision()`, and the relation *provider* points at the active Zone's formation — so
**switching zones changes the answer with no RelationManager mutation at all**. An index on a
signal whose completeness cannot be shown is exactly how the five deafnesses happened. The
precondition is a relation-revision signal covering mutation *and* provider swap.

**Kept, on safety grounds rather than speed:** the `Related` predicate now rejects by pointer and
dereferences a far end only for the subject's own edges. The old path called `aId()`/`bId()` on
every relation in the world every evaluation, and a relation may outlive its endpoints.

**Guarded by** `continuous_law_test` §8, `add_relation_action_test`, `rete_relation_state_test`;
measured by `tests/law/category_membership_scaling_test.cpp`.

## Next rungs

2. ~~**Categories as authored Formations**~~ — index half done 2026-09-09 (above). The Formation
   half remains blocked by two things: Zach's revised Formation definition means a taxonomy
   of `instance-of`/`subcategory-of` is *purely branching* and therefore **not a Formation**, and
   `RelationManager::add` rejects cycles in `subcategory-of`, so the loop cannot be closed from
   inside the taxonomy — it needs the concept-Singular bridge (`ObjectConcept`, whose
   `RelationTemplate::bAnchorId` is already "relate to this concept in advance"). Also needs
   `Zone::removeObject` to bump `Universe::structuralRevision()`, which today it does not.
5–7. The instance-side slow adapter; reified path Relations and Law-as-traverser; departure
   reporting on the reactive path. Plus the two preconditions this work uncovered: a **relation
   revision signal** (rung 4) and **complete property-write coverage** (rung 1b), each blocking an
   index that is otherwise ready to build.

## ⚑ AUTHOR — open, Zach's

§9 of the spec. **§9.3 (the sweep schedule) is answered — on structural revision, 2026-09-07.**
Still open: the distance function (9.1), what `Relation::weight` means (9.2 — value or cost;
Zach leaning strength), the stratification rule (9.4), hysteresis bands on derived relations
(9.5), and 9.6, which Zach marked open rather than closed.
