# Formation Rete

**Status:** rungs 0–4 and 7 done; rungs 5–6 have their mechanism (the slow adapter, 2026-09-16) built, tested and shipped OFF by default after measurement — see the entry below. Rung 2's Formation half remains. §9.1 answered 2026-09-16; §9.2 leaning toward removing weight.
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
  **Superseded 2026-09-14** (Rung 4 below): the stale fact was *not* harmless for `OnBecomeTrue`,
  which never re-fired on a re-formed edge. Retraction is now exact without a relation identifier:
  it happens a tick later, and only when the graph holds no edge of that type involving the being.
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

## 2026-09-14 — the precondition for rungs 4 and 5, and two fixes on the way

*Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`.*

**The goal was the signal rungs 4 and 5 are blocked on.** Both need to know when the relation
graph changes. **No new system is needed** — one was started and reverted after Zach asked whether
an existing change framework could be used, and there was one:

| Change | Already announced by |
|---|---|
| relation added | `relation-formed` (EventBus, consumed by `LawManager`) |
| relation removed (`remove`, both `removeBetween`, `removeInvolving`) | `relation-destroyed` |
| Zone switched | `zone-entered` event, **and now** `structuralRevision()` |
| being freed | `structuralRevision()` via `reapUnmadeBeings` |
| `RelationManager::loadFromJson` | **nothing** |
| `RelationManager::forgetBeingEverywhere` | **nothing** |
| `RelationManager` copy / move assignment | **nothing** |
| `Relation::setTypeLexeme` (kind changed in place) | **nothing** |

**Superseded the same day** — see *Rung 4 — ✅ 2026-09-14* below: the index lives inside `RelationManager` and invalidates on its own generation stamp, so the bottom rows needed no world signal. Original note: make those four bottom rows announce (publish the existing events, or bump
the existing counter), then build rung 4's category-membership index to invalidate on exactly the
signals in this table. Rung 5 follows on the same signals. Do **not** add a parallel revision counter.

**Fix 1 — `OnBecomeTrue` fired every frame (live regression).** `698059e0` routed `OnBecomeTrue`
through the reactive path but applied `subjects` instead of `newlyTrue`. Held true for 10 ticks: 1
firing disconnected, **10 connected** — and the engine always connects. The second time this edge
check was lost in a performance commit. Guarded by `tests/law/edge_reactive_path_test.cpp`;
`rete_compile_test` §C was described here as unable to catch it ("its law never compiles
terminals") — **wrong, corrected 2026-09-16**: §C's law has one terminal, runs on the reactive path,
and does go red when the regression is reintroduced. The claim was made from reading the test rather
than running it; §C now prints and asserts which path it exercises.

**Fix 2 — Zone switches left the rung 2 index stale.** `switchTo` replaced the beings in front of
the Person without moving `structuralRevision`, so a sweep-path law kept looking at the previous
Zone. Now bumps the existing counter. Guarded by `tests/law/zone_switch_invalidation_test.cpp`.

**For Jules and any agent touching this:**
- The continuous pass in `LawManager::tick` must keep `WhileTrue` → every holding subject and
  `OnBecomeTrue` → only newly-true subjects. Don't merge those into one loop.
- Any new way of replacing the beings or relations in front of the Person (a new activation path,
  streamed chunks, merges, hot reload) must bump `structuralRevision` or publish the existing
  events. Add a section to `zone_switch_invalidation_test` that goes through it.
- A test whose law is created disabled may never compile terminals, and will then silently cover
  only the sweep path. Build laws enabled and connected when you mean to test the reactive path.
- If a verified fix appears to regress while other sessions are committing, rebuild before believing
  it. Stale binaries produced three convincing false failures in this pass.

## Rung 4 — ✅ 2026-09-14, and the cost was not where either measurement said

*Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`. Zach chose this rung when asked what
was next.* The 2026-09-10 entry above attributed the category-membership gap to the O(relations)
walk per evaluation. Built, that turned out to be wrong, and so was the precondition table in the
section above.

**What was built.**
1. **An endpoint index inside `RelationManager`** (`relationsInvolving`), keyed by endpoint pointer
   *and* by kept identifier, rebuilt lazily from a generation stamp that every write to `relations`
   bumps (`touch()`). Because it lives inside the owner of the vector, the four "announces nothing"
   rows above needed no world-wide signal: `loadFromJson`, copy/move and `forgetBeingEverywhere`
   now touch the stamp. `EngineInit` installs it on `Universe`; `Related` uses it when present.
   Oracle: `relation_endpoint_index_test` evaluates ten cases through the index and a full scan and
   requires both to match. Two mutations (pointer-only keys, `add` without `touch`) confirmed red.
2. **A regression of my own, fixed.** The 2026-09-10 "kept for safety" pointer-only `Related`
   stopped matching edges whose endpoint is unbound or forgotten, even when the kept identifier names
   the subject (a save loaded without a resolver, or a being reborn under its stable name). Guarded by
   `related_identity_endpoint_test`.
3. **The real cost.** With the index built, a category-scoped law was *still* ~17x the property
   control at 400 beings. Profiling showed 98% of the tick in `evaluateDirty → retractFact`, a linear
   walk of ~49k facts. The cause was in `PropheticRete`: every `Related` read was marked **opaque**,
   which made the index incomplete, which switched `propheticHears` off for **every property write
   in the world**. The law's own `add position.z` then re-asserted a fact every tick. A typed
   `Related` now declares what its Rete node can wake on (its relation type by root, plus `type`,
   `directed`, `entityA`, `entityB`); an untyped one stays opaque.
   **Result at 400 beings: index 1.0x the control, scan 1.2x (was ~17x).**
4. **Proof the narrowing is safe:** `related_prophetic_legibility_test` plays a relation-graph
   scenario twice, once with the filter live and once forced open by an opaque `Overlaps` law, and
   requires identical observables at every step. `category_membership_scaling_test` now asserts the
   index arm stays within 4x of the control; making `Related` opaque again trips it (16.9x).
5. **Two pre-existing deafnesses the differential test exposed** (identical in both arms, so not
   caused by the filter):
   - **`OnBecomeTrue` never re-fired when a removed edge re-formed.** Rung 0 left the edge fact
     behind on `relation-destroyed` and called it a harmless widening. For `WhileTrue` it is. For
     `OnBecomeTrue` the stale fact kept the subject "holding" forever. Now `relation-destroyed`
     queues both endpoints, and `tick()` retracts the fact only if no edge of that type still
     involves the being (by pointer or kept identifier). It can't decide on the spot because
     `removeBetween` publishes synchronously from inside its `remove_if`.
     Confirmed red with the re-validation call removed.
   - **Retyping an edge never reached `Related` laws**, whether through the `type` property or
     `Relation::setTypeLexeme`, which assigned `type` silently. Both now go through the same
     re-validation.

**Still open, recorded rather than fixed:**
- `ReteNetwork::retractFact` does a linear `find_if` over `_facts`. Every world that holds a
  genuinely opaque law (`Overlaps`, closures, untyped `Related`) still pays O(facts) per dirty
  write. `_facts` is order-sensitive (`retractFirst(consumed)` removes a prefix), so an
  id→position index needs care.
- `retractStateFactsBySubject` clears the *entire* `_relationStateIndex`, so the idempotence guard in
  `assertRelationStateFact` forgets every being's edge facts whenever any being's state is retracted.

**For Jules and any agent touching this:**
- Never mark a condition kind opaque to be safe without measuring. Opacity is not local: one opaque
  law turns the write filter off for the whole world.
- If you change what a `Related` Rete node wakes on (`ConditionNode::compileToRete`'s attribute
  filter), update `case Kind::Related` in `PropheticRete.cpp` and add a step to
  `related_prophetic_legibility_test`.
- Any new write to `RelationManager::relations` must call `touch()` and get a case in
  `relation_endpoint_index_test`.

## Rung 5 — ⚠️ measured 2026-09-15; the adapter is deferred, the cost it targets is fixed

*Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`. Zach: "NOW DO PHASE 5".* The spec's rung
is **the instance-side slow adapter**: an HNSW-style overlap index (§3.1, §4B) that proposes
candidates, never truth, so that sweeps can be rarer. Per rungs 1 and 4, I measured before
building.

**Where sweeps actually happen.** Across every saved world there are 353 laws: 283 `OnEvent`, 68
`WhileTrue`, 2 `OnBecomeTrue`. In chess, Go, the pixel changer and Synthesis Studio, **no
continuous law took the sweep path**; all have Rete terminals. The live sweeps are `OnEvent`
laws with `Scope::Everyone`: in `chess_app_test`, **413 event sweeps, 12,856 candidates, 38
matches (0.3%)**, already narrowed from 917 beings by the rung 2 vocabulary index.

**What a missed candidate cost, measured by part:** **49 µs**, of which **45 µs was building and
destroying the transient `ECA::Event`** in `Law::conditionsSatisfied`; `Related` was 5.6 µs and a
`Compare` 0.7 µs. Of the Event's cost, **39.5 µs was `RelationManager::forgetBeingEverywhere`**.
Every Singular destructor calls it, an Event is a `Moment` is a `Singular`, and it walked every
relation in every live `RelationManager` (every Law's formations own one) with no guard. This is
the rung 1 trap (`_factParticipants`) again, in the relation graph instead of the fact table.

**Fixed.** `Relation::Endpoint` now counts the pointer it holds in a process-wide register
(its copy, assign, destruct, `bind` and `forget` are the only writes to `ptr`).
`forgetBeingEverywhere` returns in O(1) when `Relation::mayBeEndpoint` says no relation holds the
being. It's a deliberate superset: it also counts relations no manager owns, so it can only say
"maybe" too often. **Chess event sweeps: 49.1 → 8.7 µs per missed candidate; transient `Moment`
42 → 2.2 µs; time on misses over the test 629 → 111 ms.**
Guarded by `tests/relation/endpoint_register_test.cpp`: an oracle that every pointer any relation
holds is reported, plus a timing guard (6,000 relations: 0.56 µs, 35.4 µs with the early return
removed). Both mutations were confirmed red, the timing one and a copy that doesn't register.

**Why the adapter itself is not built:**
1. **It has no consumer.** Continuous laws in real worlds don't sweep. Event sweeps are already
   exact-narrowed, and after this fix their cost is dominated by `Related` evaluation (≈5 µs,
   unindexed in the test harness) and the ~260 µs of the few candidates that pass the cheap
   conjuncts, not by candidates an approximate index could skip.
2. **Its metric was ⚑ AUTHOR** (§9.1 *answered by Zach 2026-09-15, after this was written: no
   single metric; see the ⚑ AUTHOR section below*). §9.1 (the distance function across property, kind and
   quantitative overlap) and §9.2 (what `Relation::weight` means) decide what "most similar" is.
   Building HNSW before Zach answers them would be choosing for him.
3. **§6 says its only job is to make the sweep rarer**, and §8 rung 7 (departure reporting on
   the reactive path) is the named prerequisite for that. Rung 7 comes before rung 5 has
   anything safe to schedule.

**Measured next targets, recorded rather than fixed:**
- A candidate that passes the cheap conjuncts costs ~260 µs because chess move laws carry
  **subject-independent** `Not ForAny(…)` conjuncts that walk the world. Rung 3 hoisted such
  gates for the continuous path only (`LawManager::gatesHold`); `Scope::Everyone` event sweeps
  re-evaluate them per candidate. Hoisting there needs the same guard `gatesHold` has: a law that
  writes a qualified root can flip its own gate mid-sweep.
- ✅ *Fixed in the same pass:* `tests/support/test_harness.hpp` installed the relation *provider*
  but not the endpoint *index* `EngineInit` installs, so app tests evaluated `Related` by scanning
  while the app indexes. It now installs both; the 15 harness tests keep their prior results
  (11 pass; the 4 that already failed still fail with the same assertions).

**For Jules and any agent touching this:**
- Anything that runs inside `Singular::~Singular` runs for every transient `ECA::Event`. It must be
  O(1) for a being it has never seen. That has now cost the engine twice (rung 1 and rung 5).
- Never write `Relation::Endpoint::ptr` directly; use `bind`/`forget`, or the register goes stale and
  a freed being keeps its relations pointing at it.

## Rung 6 — ⚠️ measured 2026-09-15, not built

*Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`. Zach: "NOW DO THE NEXT PHASE".* The
rung is **reified path Relations and the Law traversing to its own subjects**, with magic-set
restriction (§3.2, §3.3, §4C). The measured target was the ~260 µs that a chess candidate
passing the cheap conjuncts costs, which comes from `Not ForAny(…)` conjuncts whose inner
condition names `Related(instance-of, …)`. That is exactly where restricting a quantifier to one
category's members, by traversal instead of the world, would apply.

**Measured in `chess_app_test`:** **41 quantifier evaluations in the whole test, 7 ms in total.**
Each one rebuilds `Universe::beings()` (917, 22 µs) and runs the inner condition on 52
kind-matching Objects (152 µs), true 24% of the time. Traversal would cut the 152 µs toward the
32 pieces. In absolute terms that saves a few milliseconds per game, against 111 ms spent on
event-sweep misses and seconds of world loading. **Not built:** a data structure with its own
invalidation and soundness burden, for a cost that doesn't show up.

What would change that: a world where a quantifier or `Scope::Everyone` law ranges over a
category that is a small fraction of a large population. When one exists, the design is already
constrained by Zach's §9.1 answer: traversal begins from the condition's named category Singulars,
possibly from both ends, and `ForAny ∃b: Related(T, X) ∧ φ(b)` may be restricted to the neighbours
of `X` exactly. Only the existential form qualifies (`ForAll` cannot be restricted this way), and
only when every neighbour of `X` is a bound, in-world being. Otherwise it must fall back to the
world.

## Rung 7 — ✅ departure on the reactive path, 2026-09-15

*Claude Opus 5, same session.* §8 names this as the prerequisite for making sweeps rarer (and so
for rung 5). Probed first, and it was a **live correctness bug**, not only a prerequisite.

**The defect.** A connected LawManager takes a `WhileTrue`/`OnBecomeTrue` law's holding subjects
from its Rete terminal memory. An alpha keeps a subject for as long as *any* fact about it once
passed, but its predicate reads the whole subject while its attribute filter names one
attribute. So a condition made false through an attribute the filter doesn't name never releases
the subject. **Measured red, each independently:**

| Condition | Made false by | Before |
|---|---|---|
| `Compare(x > y)` | `y` | never released |
| `InRegion(sphere)` | leaving the region | never released |
| `Zone(a − b ≥ 0)` | `b` | never released |
| `Related(instance-of, category.target)` | the target edge removed while another `instance-of` edge remains | never released |
| `All(hp > 0, gate > 0)` (control) | `gate` | released correctly (the join token is retracted) |

**Consequences:** an `OnBecomeTrue` law fired **once in its lifetime** however often its condition
went false and true again (leaving and re-entering a region, for instance). A `WhileTrue` law's
onset (`time.sinceApplied`) never reset. Nothing reported either.

**The fix (`LawManager::tick`, reactive branch).** Terminal membership now only proposes; each
candidate's condition is decided against the live world, **exactly once**. A subject about to be
applied is verified by the application itself: `Applied` means it held, `ConditionsFailed` means
it didn't. `conditionsSatisfied()` runs separately only where nothing is applied (an
`OnBecomeTrue` subject already holding, an absorbed drive) or where `applyTo` refused before
reaching the condition. `applyAndMaybeDrive` now returns the result. Also fixed on the way: an
`OnBecomeTrue` candidate whose application failed its condition used to stay remembered as holding.

**Cost:** the first version checked every candidate *and then* applied it, which doubled a
quantifier law (`quantifier_scaling_test`: 118 → 218 ms at 320 beings; gap 0.684 over its 0.65
guard). That's why it was restructured. After: **118 ms, gap 0.570, back to the pre-change
baseline.** `category_membership_scaling_test`: index 1.0x the control.

**Guarded by** `tests/law/reactive_departure_test.cpp`. All four red cases were confirmed red with
the verification removed, with the control green, and red again under the restructured loop.

**Verification, 2026-09-15.** Law/relation suite: 16/16 green (departure, edges, vocabulary,
category scaling, legibility, endpoint index and register, prophetic, gates, control patterns,
continuous, compile, relation-state, zone switch, alpha sharing). `frame_lag_test`: 0 broken
invariants, 0 timing regressions, `LawManager::tick` median 0.120 ms against a 1.653 ms baseline.
Full suite: the same 9 pre-existing failures as before this work (physics, ground plane, both
Synthesis Studio tests, pawn promotion, pixel changer, GPU, the OntoMath witness, cloud storage).

**Both scaling probes now run serially** (`CMakeLists.txt`, the same `RUN_SERIAL` the lag probe
has): they assert a RATIO between arms that run one after another, so contention skews them.
`category_membership_scaling_test` measured 1.2x against its 4x guard alone and aborted inside a
parallel suite run; serialised, it passes. `RUN_SERIAL` only holds off other *ctest* tests, so a
concurrent build elsewhere on the machine can still trip either one.

**`quantifier_scaling_test` is now close to its guard, and that is worth watching.** Quiet machine:
measured 0.603, 0.582 and 0.491 across three quiet runs against a 0.65 threshold (0.570–0.571
before this pass, so the margin narrowed). Under a concurrent build it exceeds it — 0.677 and
0.494 with the departure check, 0.719 and 0.524 with it disabled, so the overrun is machine load,
not this change. Its header already says the fitted exponent is
load-sensitive. If it fails in CI, A/B it against the same binary with the check stubbed before
believing it is a regression.

**For Jules and any agent touching this:**
- Terminal membership is a candidate set. Never treat "in the alpha memory" as "the condition
  holds".
- Don't make the verification conditional on a law being "exact" unless exactness is *proved*
  per condition kind. The four rows above are four kinds that looked exact and weren't.
- Keep one evaluation per candidate. Verifying and then applying looks harmless and doubles every
  expensive law.
- The edge/level split survives in a new shape (an `OnBecomeTrue` subject already holding is
  checked, not applied). It has been lost twice before; both guard tests must stay green.

## 2026-09-15 — Zach was right about Synthesis Studio, and it found the real cost

*Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`. Zach, on reading the rung 5 and 6
measurements: "but synthesis studio has tons of objects and laws".*

**He was right, and two of my measurements were worthless.** Rungs 5 and 6 were measured on chess,
and I reported "no continuous law takes the sweep path in real worlds". That claim came from a
probe run against `synthesis_studio_living_test` — which **exits before its first tick** when run
from the build directory (it returns early when `saves/worlds/...` is not found relative to the
working directory). Zero sweeps was zero measurement. **Run properly, Synthesis Studio Living
(137 authored objects, 535 beings in world, 68 laws) sweeps 336 times over 168 ticks.**

**What the sweep cost.** Not the candidates: those laws sweep 3–4 candidates each and their
conditions cost ~9 µs. **83% of the time was in `sweepSubjects`, and almost all of that in two
calls**, because `refreshVocabularyIndex` rebuilds the whole index whenever
`Universe::structuralRevision()` moves — which is exactly what granting a dynamic property or
admitting a being does, and this world does both while a Person plays.

**One rebuild cost 132–208 ms.** `beingCarriesProperty` walks a being's whole property list to
honour the dotted-child rule (`shape` matched by `shape.fillet`), and the rebuild called it once
per (being, name): 535 beings × 43 names, each materialising that being's property list again.

**Fixed, two changes:**
1. The index rebuild now walks **each being's own property names once** and tests them against
   the indexed set (with `string_view`, so a dotted root costs no allocation), instead of asking
   the being about each indexed name. **132–208 ms → ~14 ms per rebuild.**
2. The name set (`_indexedNames`) is no longer rebuilt on every call. It was collected from every
   law's `requiredProperties()` once **per law per tick** purely to compare against the cache; it
   is now keyed on `Law::textRevision()`, which every path that changes a law's required
   properties bumps.

**Guarded by** `tests/law/vocabulary_index_test.cpp` §H, which asks the invariants directly:
nothing is reached that `couldApplyTo` rejects, and every being whose condition holds IS reached.
Its probes compare against a qualified root on purpose, so they have no Rete terminals and take
the sweep path — the only way a test reaches the index at all. Dropping the dotted-child rule
from the rebuild turns it red.

**What this corrects in the rung 5 and 6 entries above:** "real worlds don't sweep" is false.
Chess doesn't; the Studio does, 2 per tick. The rung 5 and 6 conclusions still hold for the reason
given (the cost was not in candidate *selection* by similarity, and the Studio has no quantifiers
at all), but they were argued from one world, and that was not enough.

**For Jules and any agent touching this:**
- **Run app tests from the repo root.** `synthesis_studio_living_test`, and any harness test that
  loads `saves/worlds/...`, silently return 1 from the build directory. The same bites under
  `ctest`: `synthesis_studio_full_store_boot_test` (added 2026-09-15 by another session) reported
  "did not discover SynthesisStudio.LivingInstrument" there and passed from the repo root.
  **Fixed 2026-09-16**: `CMakeLists.txt` now gives both Living Studio tests the source root as
  their working directory, as `synthesis_studio_app_test` already had. The full-store test passes
  under `ctest`; `synthesis_studio_living_test` now RUNS and fails where it actually fails (a
  segfault in the save step), instead of returning 1 before its first tick. A test that fails
  loudly for the right reason is worth more than one that fails quietly for the wrong one. A probe that reports zero
  is not evidence until you have seen the thing run.
- Measure the Studio as well as chess. They stress opposite paths: chess is event laws with Rete
  terminals, the Studio is `WhileTrue` laws reading `@state` roots, which never compile terminals
  and therefore always sweep.
- Anything keyed on `structuralRevision` runs when a property is GRANTED, not only when a being is
  made. In a world that grants properties during play, "rebuild on structural change" means
  "rebuild during play".

## 2026-09-16 — the traversal primitive: bounded BFS

*Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`. Zach: "I think its fine to just
implement BFS for now we can always adjust it to dijkstras later if I decide weight is something
like strength."*

**Built:** `src/Relation/Traversal/RelevanceTraversal.{hpp,cpp}` — `Relevance::breadthFirstRoutes`,
the search every later rung needs. Shortest-in-hops, capped in two dimensions (`maxHops`,
`maxVisited`), filterable by relation kind, honouring or ignoring direction, and able to stop at a
named destination. It reads **no** `Relation::weight`, which is both what BFS means and the honest
thing while §9.2 is open and Zach is leaning toward removing weight.

**Shaped for the Dijkstra upgrade:** swap the frontier queue for a priority queue and give
`RouteQuery` a cost function. Nothing else changes — hop count is Dijkstra with unit weights, and
`Route` already carries the edges it crossed.

**The three properties it must have, and the mutations that proved each is guarded**
(`tests/relation/relevance_traversal_test.cpp`):
1. **Shortest in hops.** The test's last case puts a being 2 hops away down one branch and 3 down
   another, with kinds named so the LONG branch is explored first. Swapping the queue for a stack
   (depth-first) makes it report 3 hops — caught. *Every other case in the file passed under that
   mutation*, which is exactly why this case exists: the central claim was unguarded until it did.
2. **Bounded, and honest about it.** `cappedOut` separates "no route exists" from "none within
   these bounds". Ignoring the hop cap — caught.
3. **Identity, not just pointers.** A save loaded without a resolver has no endpoint pointers at
   all; the walk crosses those edges by the identifiers they kept and resolves the being back out
   of the world. Same rule as `Related` (rung 4).

Also: results are ordered deterministically (hops, then relation kind, then destination
identifier), never by pointer, so two runs of one world give one answer — a precondition for ever
saving or comparing what the adapter finds.

**Not built, deliberately:** retention. Turning a discovered route into a Relation that persists
needs §9.1(a) (the adapter is a First Mover, so authorship is settled) and the rules in
`docs/architecture/ontology/PRIMARY_AND_SUB_RELATIONS.md` (primary Relations never disappear;
sub-Relations dissolve only on proved impossibility). There is also still **no consumer** — see
the rung 5 and 6 measurements above. This is the primitive, ready for one.

**For Jules and any agent touching this:** the traversal may never decide anything. If you find
yourself asking a route whether a law holds, stop — the law's conditions answer that, and the
sweep is the floor (§6). That is the entire reason a capped, approximate search is admissible.

## 2026-09-16 — the slow adapter, built; rungs 5 and 6 have their mechanism

*Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`, working autonomously at Zach's
direction ("build autonomously from here because im gonna sleep try to do as mucu of the rungs as
u can"), and following his architectural note:*

> The mechanism that creates Relations between Relations and pre-loads Law Relations to these
> Relation Formations is also supposed to be in the slow adapter rather than constantly rebuilt
> every frame. Perhaps this resolves one of your earlier concerns.

**It does resolve it.** Rungs 5 and 6 were both deferred for "no consumer": the cost they targeted
was not where I measured. Zach's note says the point was never to make a per-frame step cheaper —
it is to move the work OFF the frame entirely, onto the adapter's own clock, and let ticks traverse
what is already there.

**Built:** `src/Relation/Traversal/SlowAdapter.{hpp,cpp}`.
- A **road** is "the beings related to `<category>` by `<kind>`" — one per `Related(kind, category)`
  a Law travels. `ConditionNode::collectCategoryRoutes` reads them off law text.
- `step()` is the **independent clock**: one bounded unit per tick, an IMPROVE rate that walks a
  road nobody has walked and a REVISIT rate that re-checks the least recently seen (§4B: an index
  built once and never re-checked "found better routes once"). It never rebuilds everything.
- `candidatesFor()` answers only when the road is **provably current**, and `sweepSubjects` uses it
  in place of the vocabulary index. Otherwise it returns false and the sweep runs exactly as before.
- `reify()` implements §3.2 literally — a Formation OF the Relations that carry a category's
  membership, `gathers` Relations between the gathering and each edge, and a `routes-through`
  Relation from the Law to the gathering. **Not called by the engine**: it writes Relations into a
  Person's world, which a save then carries, and save files are sacred. It is built and tested,
  waiting for Zach.

**Currency is two counters, and finding the second one was the real work.**
`Universe::structuralRevision` does NOT move when a relation is formed or dissolved. My first
version subscribed to `relation-formed`/`relation-destroyed` instead — and that has a hole no test
would have shown: **`RelationManager::loadFromJson` replaces the entire graph and publishes
nothing at all.** A road built before a save was loaded would have been served against a graph that
no longer existed. `RelationManager::generation()` already counts every write to `relations` (it is
what the rung 4 endpoint index keys on); it is now surfaced through
`Universe::relationGeneration()`, and a road is served only when BOTH numbers match what it was
built under. No new change system — Zach ruled that out on 2026-09-14 — just the existing stamp,
made visible.

**Measured, and the honest answer is two answers:**

| World | Sweep | Adapter | |
|---|---|---|---|
| synthetic, 400 beings, 8 in category | 2.48 ms/event | 1.78 ms/event | **0.71x** |
| synthetic, 1600 beings, 8 in category | 10.86 ms/event | 7.91 ms/event | **0.73x** |
| chess_app_test (whole run, twice each) | 23.3 s / 24.1 s | 24.7 s / 25.2 s | slightly WORSE |

The synthetic world is one where every being carries the law's properties, so the vocabulary index
proposes all of them and the road proposes 8. Chess is the opposite: `chessRole` and `gridX` are
carried only by pieces, so rung 2's index already narrows to 32 and the adapter adds bookkeeping
without removing candidates.

**So it ships OFF** (`LawManager::setUseSlowAdapter`, default false), per Zach's instruction: *"Leave
elements as inactive scaffolding if u measure it to be worse off dont delete it altogether."*
**Off means off**: while the flag is false no roads are noted and none are walked, so the adapter
costs nothing at all. (A first version gated only the query and kept maintaining the structure
unread — that is what made chess ~4% slower, and it is the difference between inactive scaffolding
and a tax nobody collects.) Turning it on re-registers every law's roads, so the structure is ready
for the world that needs it: many beings sharing a vocabulary, few in the category.

**Guarded by:**
- `tests/relation/slow_adapter_test.cpp` — every answer checked against a full scan through the
  same compiled `Related` predicate a Law uses. Ten cases: built-then-matching, relation formed
  after, relation removed after, a being ARRIVING with no relation event at all (the case that
  isolates the structural counter), a being freed, the member cap, direction, two categories, and
  reification, and — found by re-reading my own code before it shipped — **a category that is not
  in the world at all**. An edge keeps its endpoint's identifier when it has no pointer, and
  `Related(kind, name)` holds of such an edge, so a road built from "the category's own edges"
  found nothing to start from and reported an EMPTY membership: a deaf law, in the code written to
  prevent deaf laws. It now falls back to the whole graph. Four mutations confirmed red: ignore the
  structural revision, ignore the graph generation, serve a capped road, and the empty-road hole.
- `tests/law/slow_adapter_parity_test.cpp` — the same world with the adapter off and on must reach
  the same beings the same number of times, including a being that qualifies only through an `Any`
  arm and one admitted mid-run.

**The soundness rule, and the mistake that found it.** A road is usable only where EVERY being
satisfying the law must travel it — a `Related` CONJUNCT. Under `Any` the other arm can satisfy the
law alone; under `Not` being on the road is what disqualifies; a quantifier's inner condition is
about the instances it ranges over. `collectCategoryRoutes` therefore walks `All` chains only.
My first parity test could not see this: it used CONTINUOUS laws, which compile Rete terminals from
the very `Related` leaf in question and so never sweep — the adapter was never consulted, and the
test passed with the rule mutated away. Rewritten around OnEvent/Scope::Everyone laws (the shape
chess is written in, and the only shape that reaches `sweepSubjects`), the mutation now makes the
`Any` being go deaf and the test catches it.

**Two things found on the way, both recorded rather than worked around:**
- `LawManager`'s EventBus subscription outlives the manager (Law.hpp says so: "a connected
  LawManager must outlive all publishing"). Two connected managers in one process means the dead
  one still hears every event: whichever arm of a two-manager test runs second counts everything
  twice. The parity test therefore uses ONE manager and toggles the flag.
- Route registration must not live in `compileConditionsToRete`: that runs only for laws that want
  Rete terminals (`activation() != OnEvent`), which is exactly the set of laws that never sweep. It
  is now `syncAdapterRoutes`, keyed on the law's condition revision, called for every law.

**Two efficiency defects in my own code, found by re-reading it rather than by a test** (recorded
because the review is the part worth repeating): resolving an unbound edge's member walked every
being in the world PER EDGE — O(edges x beings) for exactly the case a road exists to make cheap —
and the candidate union de-duplicated with a linear scan, quadratic in members once a law names two
categories. Both now use a map.

**For Jules and any agent touching this:**
- The adapter may never decide anything. It proposes candidates; the law's conditions decide; the
  sweep is the floor (§6). `candidatesFor` returning false is always correct and merely slow.
- If you add a way to change the relation graph, make sure it calls `RelationManager::touch()`.
  That is now the adapter's currency test as well as the endpoint index's.
- Before claiming the adapter helps or does not, check WHICH laws sweep in the world you measured:
  a continuous law carrying a `Related` conjunct takes the reactive path and never touches this.

## 2026-09-16 — three smaller things, one of them a correction of my own record

*Claude Opus 5, same session, same autonomous stretch.*

**0. A note on tonight's timings.** The machine spent this stretch memory-thrashing (15 GB used,
~180 MB free, ~147M swapouts, 63% idle CPU), and `chess_app_test` wandered between 24 s and 85 s
for identical binaries. Wall-clock comparisons from tonight are therefore worthless and are not
reported as results. `frame_lag_test` is the exception and the reason it exists: it calibrates
against the machine and reported 0 broken invariants and 0 timing regressions, with
`LawManager::tick` at 0.190 ms against a 1.653 ms baseline.

**1. Retracting one being's facts cost every other being its duplicate-prevention.**
`ReteNetwork::retractStateFactsBySubject` cleared `_relationStateIndex` **wholesale**. That index is
what `hasRelationStateFact` reads, and rung 0 added that guard because three paths assert edge facts
and `assertFact` does not deduplicate — its comment warns of "a standing per-tick propagation tax
and, over a session of relations forming and dissolving, unbounded" growth. Clearing the index
quietly switched the guard off for everyone: after any object-destroyed or relation-destroyed, the
next edge fact asserted for ANY being was a duplicate.

Measured with the new guard test: four unrelated beings living and dying while a bystander gains
edges leaves the bystander with **5 edge facts where it should have 1**. Every propagation walks
them all, and nothing ever removes them — the law still fires correctly, which is why no existing
test could see it. Now only the retracted being's entries leave the index.
Guarded by `tests/law/relation_state_index_test.cpp`; the wholesale clear turns it red.

**2. `rete_compile_test` §C now states which path it runs on.** `LawManager::terminalCountOf` was
added so a test can ask rather than assume; §C prints it and asserts it is non-zero.

**3. THE CORRECTION.** On 2026-09-14 I wrote — in this doc, in `edge_reactive_path_test`'s header,
and in the analysis doc — that §C "could not catch" the `OnBecomeTrue` regression because its law,
created disabled and enabled late, never compiles terminals and so only exercises the sweep path.
**That was false, and I never ran it.** `syncReteCompilation` compiles terminals for every law with
a condition model, enabled or not. Measured 2026-09-16: §C's law has one terminal, runs on the
REACTIVE path, and goes red when the regression is reintroduced — as does
`edge_reactive_path_test`. All three places now carry the correction.

The mistake is worth keeping rather than quietly fixing: it is the same shape as the bugs this rung
ladder keeps finding — a claim about **which code something reaches**, asserted from reading rather
than running, in a document whose whole subject is analyses that are narrower than the truth.
`ENGINEERING_DISCIPLINE.md` names it in one line: *don't claim a doc is verified because you read
the source — run things.*

**Two To-do items downgraded with evidence rather than deleted:**
- `retractFact`'s linear scan: no saved world holds an `Overlaps`, an unknown kind or an untyped
  `Related`, and the tree's only hand-written condition closure is in `terminal_entry.cpp` — so the
  Prophetic write filter is ON in every authored world and almost nothing reaches that scan.
- Hoisting gates in event sweeps: chess's entire sweep cost is ~111 ms across a 23 s run, and the
  expensive conjuncts there are quantifiers, which cannot be hoisted across candidates because the
  law's own actions change their answer mid-sweep.

## Next rungs

2. ~~**Categories as authored Formations**~~ — index half done 2026-09-09 (above). The Formation
   half remains blocked by two things: Zach's revised Formation definition means a taxonomy
   of `instance-of`/`subcategory-of` is *purely branching* and therefore **not a Formation**, and
   `RelationManager::add` rejects cycles in `subcategory-of`, so the loop cannot be closed from
   inside the taxonomy — it needs the concept-Singular bridge (`ObjectConcept`, whose
   `RelationTemplate::bAnchorId` is already "relate to this concept in advance"). Also needs
   `Zone::removeObject` to bump `Universe::structuralRevision()`, which today it does not.
5–7. ✅ **Done or measured** (2026-09-15/16): rung 7's departure reporting is built; rung 5's slow
   adapter is built and ships inactive after measurement; rung 6's Law-as-traverser half rides on it
   and its reification is implemented but not run by the engine. The **relation revision signal**
   that rung 4 wanted turned out to exist already — `RelationManager::generation()`, now surfaced as
   `Universe::relationGeneration()`, no new counter. **Complete property-write coverage** (rung 1b)
   is still the open precondition, and a direct C++ setter still bypasses the property vocabulary.

**The ladder itself has been superseded in shape.** Zach, 2026-09-16: Formation Rete is a **tiered
relevance system with fallbacks** — sweep at the floor, similarity as a low-level discovery aid,
climbing toward a direct proved `Law → Singular(+PropertyPath)` route, with the slow adapter building
upward and the hot path consuming the highest sound and current tier. Recorded by GPT-5.6 Sol in
`docs/architecture/law/FORMATION_RETE_TIERED_RELEVANCE_LADDER.md` and companions.

**The implementation plan is now `docs/plans/ontological_rete_architecture.md` §§6–8** — what is
built mapped onto the tiers, the two steps of the 2026-09-03 plan that doctrine has since reversed
(retiring the sweep; deleting Beta nodes), and nine steps from here with their tests, their measured
constraints, and the four ⚑ AUTHOR gates that are Zach's.

## ⚑ AUTHOR — open, Zach's

§9 of the spec. **§9.3 (the sweep schedule) is answered — on structural revision, 2026-09-07.**
**§9.1 (the distance function) is answered — Zach, 2026-09-15: no single metric.** Several
locally coherent similarity indices (kind, taxonomic, property, quantitative-within-a-meaningful-
domain), traversed possibly at once and from both ends of a condition. Movement between them is
governed by a broader, reified **relevance graph** of four Relation families: similarity,
relevance, discovered routing (found by the slow adapter and retained), and Person-authored
relevance. Triangle inequality is a local efficiency property, not correctness, and the sweep stays
the floor. His full answer and the four sub-questions it raises (who authors a discovered route;
what makes properties commensurable; what licenses switching indices; retention and decay of
routes) are in the spec's §9.1. **Zach answered those the same day:** (a) the adapter is a First
Mover; (b) commensurability is set by the index being traversed (property root, property type
class, or any other objective gradient); (c) deferred to §9.2; (d) a **primary Relation between
two Singulars never disappears**. Of its sub-Relations, those whose founding premise has become
logically impossible are dissolved; those merely less optimal are deprioritised; the primary keeps
their history. (d) is an Earthcall-wide principle the engine doesn't yet follow (every
`RelationManager` removal erases outright); specified in `docs/architecture/ontology/PRIMARY_AND_SUB_RELATIONS.md`.
**§9.2 (what `Relation::weight` means) — Zach is now leaning toward removing weight entirely**
(2026-09-15): too vague, and the ML-like reading assumes things about a Relation's conditions that
the program does not guarantee. BFS was built meanwhile, reading no weight. The spec's §9.2 lists
what removal would touch (save files, `RelationManager`'s duplicate-merge rule, a registered
governable property, the developer-mode audit warning).
Still open: what `Relation::weight` means (9.2 — value or cost;
Zach leaning strength), the stratification rule (9.4), hysteresis bands on derived relations
(9.5), and 9.6, which Zach marked open rather than closed.
