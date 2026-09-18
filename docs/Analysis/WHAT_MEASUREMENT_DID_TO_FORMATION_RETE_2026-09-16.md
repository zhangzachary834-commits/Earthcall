# What Measurement Did to Formation Rete

**Claim:** across seven changes to the law engine between 2026-09-14 and 2026-09-16, the cost was
never once where the design said it was. Every speedup came from a cause the plan did not name, the
one structure built exactly as specified made things **slower**, and three of the six defects were
found not by a failing test but by measuring a thing that turned out to be measuring nothing.

**Author:** Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`, 2026-09-16 18:15 PDT.
**Architecture:** Zach. **Companions:** `DERIVED_STATE_AND_THE_SILENCE_OF_LAWS_2026-09-10.md` (the
same engine, read for correctness rather than cost), `../architecture/law/FORMATION_RETE.md` §8,
`../architecture/law/FORMATION_RETE_TIERED_RELEVANCE_LADDER.md`,
`../architecture/law/DERIVED_STATE_LEDGER.md`,
`../Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md` (the per-rung record).

---

## 0. Method, and what it is worth

Four instruments, in descending order of trustworthiness:

1. **Registered scaling tests** — `category_membership_scaling_test`, `quantifier_scaling_test`,
   `referent_resolution_test`. They measure a law against a control law of identical selectivity,
   back to back, and assert the **ratio**. Shared machine load largely cancels.
2. **`frame_lag_test`** — calibrates against the machine, reports `STANDING` (a cost already in the
   baseline) separately from `LAG` (your change), and stops enforcing timings when the machine
   moves under it.
3. **Temporary in-tree probes** — `std::chrono` around a suspect block, printed per call. Powerful
   and treacherous; §6 is about how they lied twice.
4. **Wall clock on whole tests** — nearly worthless this week. The machine spent 2026-09-16
   memory-thrashing (15 GB used, ~180 MB free, ~147M swapouts, 63% idle CPU) and `chess_app_test`
   wandered between **23.3 s and 84.6 s for identical binaries**. Every wall-clock comparison below
   is labelled with that caveat or excluded.

Every claim of a fix below was additionally checked by **mutation**: break the fix, watch the named
test go red, restore. A guard that has never been seen to fail is a guard nobody has tested.

---

## 1. The headline numbers

| # | Change | Before | After | Where the cost actually was |
|---|---|---|---|---|
| 1 | Category-scoped `Related` law (400 beings, 8 matching) | **17.4x** the property control | **1.0x** | not the relation walk — an *opaque Prophetic read* turning the world-wide write filter off |
| 2 | One candidate evaluated in a chess event sweep | **49.1 µs** | **8.7 µs** | 45 µs of it was constructing and destroying a transient `ECA::Event` |
| 3 | Transient `Moment` lifetime (6,000 relations live) | **35.4 µs** | **0.56 µs** | its destructor walked every relation in every live `RelationManager` |
| 4 | Vocabulary index rebuild (Synthesis Studio, 535 beings, 43 names) | **132–208 ms** | **~14 ms** | `listProperties()` materialised once per *(being, name)* pair |
| 5 | Chess: time spent on non-matching candidates, whole test | **629 ms** | **111 ms** | consequence of #2 |
| 6 | Edge facts for an uninvolved being, after 4 deaths | **5** | **1** | one being's retraction cleared the dedupe index for *everyone* |
| 7 | `ForAll` law at 320 beings (rung 7's first attempt) | 118 ms → **218 ms** | **118 ms** | verifying *and then* applying evaluated the condition twice |

Rows 1–6 are wins. **Row 7 is the change I made that made things worse**, caught by a registered
scaling test before it shipped, and it is the most instructive entry in the table (§4).

---

## 2. The cause chains, in full

Each of these took the same shape: a plausible cause, a measurement that refuted it, and a real
cause one or two levels away from where anyone was looking.

### 2.1 The category-scoped law — five links from symptom to cause

`FORMATION_RETE.md` §8 rung 4 stated the problem as *"the cost is the O(relations) provider walk per
evaluation"*, measured at 2.7x–4.0x on 2026-09-10. I built the endpoint index that walk implies —
`RelationManager::relationsInvolving`, keyed by pointer **and** kept identifier, oracle-tested
against a full scan across ten cases.

**It bought nothing.** Index ≈ scan, both ~16x the control:

| beings | Related, scanning | Related, indexed | `Compare` control | ratio |
|---|---|---|---|---|
| 50 | 1.27 ms | 1.27 ms | 0.28 ms | 4.5x |
| 400 | 7.94 ms | 7.44 ms | 0.46 ms | **17.4x / 16.3x** |

Profiling put ~98% of the tick in `ReteNetwork::evaluateDirty → retractFact`, a linear `find_if` over
~49,000 facts. But a trace of *which* facts went dirty showed the `Compare` arm re-asserting **zero**
facts per tick and the `Related` arm **eight**. The two arms differed only in the condition. The
chain, once followed:

```
a Related leaf is marked "opaque read" in PropheticRete
  -> Index::complete() == false
     -> LawManager::propheticHears() fails open for EVERY property write in the world
        -> the law's own `add position.z` re-asserts a fact each tick
           -> evaluateDirty -> retractFact -> linear walk of 49k facts
```

The fix was not an index at all: a **typed** `Related` declares what its Rete node can wake on (its
relation kind by root, plus `type`, `directed`, `entityA`, `entityB`); only an untyped one stays
opaque. After: **1.0x** the control at 400 beings; fitted growth 0.238 against the control's 0.231.

**The general result, and the reason it belongs in an architecture document:** *opacity is not
local.* One condition kind declining to say what it reads switched off a filter for every law in
every world. `category_membership_scaling_test` now asserts the ratio stays under 4x; restoring the
opaque marking trips it at 16.9x.

### 2.2 The transient Moment — the cost of being a Singular

Rung 5 was supposed to build a similarity adapter. Measuring what it would speed up instead found
this. In `chess_app_test`: **413 event sweeps, 12,856 candidates, 38 matches (0.3%)**. A non-matching
candidate cost 49.1 µs. Timing the pieces:

| Piece of one candidate evaluation | Cost |
|---|---|
| construct + destroy one `ECA::Event` | **45.2 µs** |
| the `Related` predicate | 5.6 µs |
| one `Compare` | 0.7 µs |

An `ECA::Event` carries a `Moment`; a `Moment` **is** a `Singular`; every `Singular` destructor
announces its release; and `RelationManager::forgetBeingEverywhere` walked every relation in every
live manager — and every `Law`'s three group Formations own one. Measured directly: 39.5 µs of the
42 µs `Moment` lifetime.

The fix is the same shape as rung 1's `_factParticipants`: an O(1) "does any relation hold this
pointer?" register maintained by `Relation::Endpoint`'s own special members. **35.4 µs → 0.56 µs**
at 6,000 relations; per-candidate **49.1 → 8.7 µs**; chess's total miss cost **629 → 111 ms**.

This is the second time the same trap has been sprung by the same object: `DERIVED_STATE…` §4 records
the first, where transient Moments made `retractFactsAbout` quadratic. The lesson did not generalise
the first time because it was recorded as a fact-table problem rather than as a rule:
**anything that runs inside `Singular::~Singular` runs for every transient Event, and must be O(1)
for a being it has never seen.**

### 2.3 The Synthesis Studio — and a measurement that measured nothing

Rungs 5 and 6 were both deferred on the evidence that *"no continuous law takes the sweep path in
real worlds"*. Zach's reply — "but synthesis studio has tons of objects and laws" — was correct, and
the evidence was worthless: the probe had been run against `synthesis_studio_living_test`, which
**exits before its first tick** when launched from the build directory (it resolves
`saves/worlds/...` relative to the working directory). Zero sweeps was zero measurement.

Run properly: **336 sweeps over 168 ticks** in a 535-being world. And the cost was not the
candidates — those laws sweep 3–4 each, at ~9 µs per condition — but **83% in `sweepSubjects`**,
concentrated in two ticks costing **132 ms and 208 ms**. Those were vocabulary-index rebuilds, which
fire whenever `structuralRevision` moves — which *granting a dynamic property* does, which is what
playing the Studio does constantly.

| Rebuild implementation | Cost |
|---|---|
| ask each being about each indexed name (`beingCarriesProperty` × 535 × 43) | **132–208 ms** |
| walk each being's property list once, test names against the set | 34 ms |
| invert: test the being's own names against the indexed set | 17 ms |
| plus `string_view` keys, no allocation per dotted root | **~14 ms** |

A second, independent defect in the same function: the set of indexed names was rebuilt **once per
law per tick** purely to compare against the cache. Now keyed on `Law::textRevision()`.

**A 200 ms freeze is a Person-visible stutter**, and it had been in the engine since rung 2 landed,
invisible because no test measured a world that grants properties during play.

### 2.4 The slow adapter — built as specified, measured as worse

Rungs 5–6, built to Zach's clarification that route structure belongs on the adapter's clock rather
than being rebuilt per frame. Synthetic world, one law, 8 beings in the category:

| beings | sweep | adapter | ratio |
|---|---|---|---|
| 100 | 0.91 ms/event | 0.80 ms | 0.87x |
| 400 | 2.48 ms/event | 1.83 ms | 0.74x |
| 1600 | 14.12 ms/event | 8.67 ms | **0.61x** |

Then the same question asked of a real world — chess, whose 132 route-carrying laws are the idiom
this was built for:

| chess_app_test, whole run | adapter off | adapter on |
|---|---|---|
| run 1 | 23.3 s | 24.7 s |
| run 2 | 24.1 s | 25.2 s |

**Slower.** The reason is exact: chess's pieces carry `chessRole` and `gridX`, which nothing else
carries, so rung 2's vocabulary index already narrows 917 beings to 32 — and the adapter's road
returns *the same 32*. It adds bookkeeping and removes nothing.

Per Zach's instruction ("leave elements as inactive scaffolding if u measure it to be worse off dont
delete it altogether") it ships **off**, with its maintenance gated too, so off costs nothing. The
synthetic result says precisely where it becomes worth turning on: **a world where many beings share
a law's vocabulary but few are in its category.**

**The general result:** a higher tier must be **narrower**, not merely higher, to earn the hot path.

### 2.5 The dedupe index — a cost that only appears over time

`retractStateFactsBySubject` cleared `_relationStateIndex` **wholesale**. That index is what stops
three separate paths from stacking duplicate edge facts. So after *any* being's retraction, the next
edge fact asserted for *any* being was a duplicate.

Measured with a purpose-built test: four unrelated beings living and dying, while a bystander gains
edges — bystander ends with **5 edge facts where 1 belongs**. Every Rete propagation walks all five;
nothing ever removes them. The laws fire correctly throughout, which is why nothing caught it.

This is a **monotone** cost: it does not show up in any single frame, only in a long session. No
scaling test would have found it; it took writing down what the structure depends on
(`DERIVED_STATE_LEDGER.md`) and asking what invalidates it.

---

## 3. Frame cost, before and after

`frame_lag_test`, world `Chess`, against `tests/singularity/frame_lag_baseline.txt`:

| Reading | `LawManager::tick` median | Whole simulation frame | Verdict |
|---|---|---|---|
| Baseline (recorded 2026-08-28) | 1.653 ms | 2.456 ms | — |
| 2026-09-15, after rung 7 | 0.189 ms | 0.199 ms | 0 broken invariants, 0 timing regressions |
| 2026-09-16, after the vocabulary-index rebuild fix | **0.120 ms** | 0.128 ms | 0 regressions |
| 2026-09-16, after the adapter (off) + dedupe fix, machine thrashing | 0.190 ms | 0.204 ms | 0 regressions |

The third row is the honest best reading; the fourth is the same code on a machine that was swapping,
and is included rather than dropped because selecting the flattering run is how baselines rot. The
aspiration for `LawManager::tick` is 2.000 ms and all readings are far under it; the standing costs
that remain are elsewhere (world load, `groundScan`, rotation).

---

## 4. The change that made things worse, and why it is the most useful row

Rung 7's defect was real: a subject whose condition went false was never released, so an
`OnBecomeTrue` law fired **once in its lifetime** and a `WhileTrue` law's onset never reset. The
first fix was the obvious one — verify each candidate's condition against the live world before
counting it as holding.

`quantifier_scaling_test` immediately refused it:

| `ForAll` law, 320 beings | before the fix | first fix | restructured |
|---|---|---|---|
| ms per tick | 118.6 | **218.7** | 118.1 |
| gap vs control (guard: < 0.65) | 0.571 | **0.684 — fails** | 0.570 |

The cause is embarrassing and general: `Law::applyTo` **already** evaluates the condition. Verifying
first and applying second evaluates every expensive condition twice. The restructure makes the
application *be* the verification — `Applied` means it held, `ConditionsFailed` means it did not —
and runs a separate check only where nothing is applied.

Two things this row demonstrates that a table of wins cannot:
- a correctness fix has a cost profile, and it must be measured like any optimization;
- **the guard that caught it was a ratio test written for an unrelated rung two weeks earlier.**
  Scaling tests pay for themselves by catching the change you were not thinking about.

---

## 5. Five beliefs the measurements overturned

| Belief | Held by | Refuted by |
|---|---|---|
| "The category-scoped cost is the O(relations) walk" | `FORMATION_RETE.md` §8 rung 4, from a 2026-09-10 measurement | the endpoint index changed nothing; the cause was Prophetic opacity |
| "Real worlds don't take the sweep path" | me, 2026-09-15 | Zach's objection; the Studio sweeps twice per tick |
| "`rete_compile_test` §C can't catch the edge regression — its law never compiles terminals" | me, in three documents | it has one terminal, runs the reactive path, goes red when the regression returns |
| "Narrowing candidates always helps" | the shape of rungs 2–6 | chess: same 32 candidates, measurably slower |
| "A cache with a revision key is current" | the referent map, since it was written | nothing tested the key; only its *speed* was guarded |

Four of the five were mine. The third was asserted in an architecture document, a test header and a
commit message before anyone ran it — which is the same error class as the bugs this engine keeps
producing: **a claim about which code something reaches, made from reading rather than running.**

---

## 6. When the instrument is the defect

Two probes produced confident wrong numbers, and both failure modes are worth naming because they
are invisible in the output.

**The probe that measured itself.** A trace of sweep cost reported **15.6 ms per sweep** in the
Studio. The timer's closing call and the `fprintf`'s arguments were in the same statement, and one of
those arguments called `Universe::beings()` — building a 535-element vector inside the measurement.
Closing the timer into a local first: **1.3 ms per tick**, a 12x error, in the direction that would
have justified a large unnecessary optimization.

**The probe that measured nothing.** Reported zero sweeps in three worlds, and the conclusion —
"continuous laws never sweep in practice" — went into two documents and deferred two rungs. The test
harness had returned `1` before its first tick. **A probe reporting zero is not evidence until you
have seen the thing run at all.** (Fixed in `CMakeLists.txt`: tests whose subject is an authored save
now get the source root as their working directory.)

A third instrument effect, smaller: both scaling probes assert ratios between arms measured back to
back, and a concurrent build skews them. `quantifier_scaling_test` measured 0.491–0.603 quiet and
0.677–0.719 under load — either side of its 0.65 guard. Both now run serially under `ctest`, like the
lag probe. When one fails in CI, A/B it against the same binary with the change stubbed out before
believing it; that is how the load skew was distinguished from a real regression.

---

## 7. What actually found the defects

Of the six real defects in this window, **two** were found by a failing test. The others:

| Defect | Found by |
|---|---|
| `Related` deafness on unbound endpoints | writing the guard test for a different change |
| Prophetic opacity (17x) | profiling after an optimization *failed to help* |
| transient `Moment` cost (45 µs) | measuring what a rung was *supposed* to speed up |
| vocabulary rebuild (200 ms) | Zach objecting to a conclusion |
| empty road when a category is absent | re-reading my own code before shipping it |
| dedupe index cleared wholesale | writing down what a structure depends on |

The two instruments that paid best were not tests at all: **an optimization that fails to help is a
measurement**, and **writing the invalidation story down finds the holes in it**. The second is the
argument for `DERIVED_STATE_LEDGER.md`: the act of filling in the table found a structure credited to
a test that never exercises it, and a cache whose currency had no test in the tree.

And the guards themselves were only trustworthy because each was deliberately broken:

```
departure check removed        -> 4 cases red (Compare/InRegion/Zone/Related), control green
graph generation ignored       -> road served stale after a relation formed
structural revision ignored    -> road served stale after a being arrived
capped road served             -> truncated candidate set accepted
route collection into Any/Not  -> the being that qualifies another way goes deaf
dotted-name rule removed       -> vocabulary index omits a law's beings
wholesale index clear restored -> 5 edge facts where 1 belongs
referent map rebuild removed   -> every @-rooted law goes silent
BFS frontier -> stack          -> shortest route reported as 3 hops instead of 2
Related marked opaque again    -> category law back to 16.9x
```

Ten mutations, ten red. The one that mattered most was the ninth: **every other case in the traversal
test passed under a depth-first frontier**, so the property the whole structure rests on — shortest
in hops — was unguarded until a case was written specifically to distinguish it.

---

## 8. What this implies for the tiered ladder

`FORMATION_RETE_TIERED_RELEVANCE_LADDER.md` §11 says a tier earns hot-path priority "by soundness
first and measured value second". The measurements here sharpen that into three operational rules:

1. **Narrower, not merely higher.** A tier returning the tier-below's candidate set is a pure loss
   (chess). Promotion requires a *measured reduction in candidates*, not better provenance alone.
2. **Selection must be O(1) per law.** The sweep's per-candidate cost is now ~8.7 µs; a selector
   consulting several structures per candidate loses to what it replaces.
3. **Any tier built on the Prophetic index inherits global failure.** One opaque law makes the whole
   index incomplete; the relevance graph must fail open everywhere, not locally.

And one rule for whoever measures next, which this week cost the most to learn:

> **Before reporting a measurement, prove the thing ran.** A zero, a flat line, or a suspiciously
> round improvement is a claim about the instrument until the instrument is checked.

---

*Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`, 2026-09-16 18:15 PDT. Numbers from
`category_membership_scaling_test`, `quantifier_scaling_test`, `frame_lag_test`,
`endpoint_register_test`, `relation_state_index_test`, and temporary probes since removed; the
per-rung record with dates is in `../Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md`.
The architecture measured here is Zach's.*
