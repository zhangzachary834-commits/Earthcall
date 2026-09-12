# Derived State and the Silence of Laws

*What four rungs of Formation Rete found, what the findings have in common, and what follows.*

**Author:** Claude Opus 5, Claude Code session `session_01F9nK3FZ7VR4PFPTUWfYyvm`
**Date:** 2026-09-10
**Tree:** branch `sync-from-earthcall-main`, HEAD `4b969b6f`; rungs 0–2 in HEAD, rung 3 in the
working tree. 139 registered tests.
**Method:** every claim below was produced by building and running the tree, not by reading it.
Each bug was reproduced, then re-confirmed against a clean tree with my own changes stashed, before
being called pre-existing. Each performance number is a measurement I can name the test for. Where
a measurement was wrong before it was right, §9 says so.
**Companions:** `docs/architecture/law/FORMATION_RETE.md` (the architecture and its rung ladder),
`PROPHETIC_RETE.md` §2 (widen, never narrow), `NO_BLACK_BOX.md` (Refusal 6),
`docs/Reflections on Earthcall's Progression/Reflections on Repo State/What_The_Test_Suite_Can_See.md`
(whose thesis §3 here extends).

---

## The claim, in one sentence

Earthcall's law engine failed five times in the same way — not because five different things were
written badly, but because the engine maintains **eleven separate derived structures with no
shared account of what invalidates each**, and the failure mode of a stale derived structure in a
rule engine is a law that goes quiet, which is the one kind of failure this repo's tests are
structurally unable to see.

Everything else in this document is that sentence with its evidence, its two sub-species, its
cost consequences, and what I think follows.

---

## 1. The findings, verified

Four rungs of `FORMATION_RETE.md` §8 were built between 2026-09-08 and 2026-09-10. They produced
five correctness findings, three performance findings, and two rejected optimizations.

### 1a. Five silent deafnesses

Each made a law reach nobody while remaining registered, enabled, authored, and satisfiable.
None produced an error, a log line, or a failing test.

| # | Where | The mechanism | Found by |
|---|---|---|---|
| 1 | `relation-formed` handler | The event publishes the **Relation** as its subject, so `seedStateFacts(e.subject)` snapshotted the Relation's own properties and emitted no edge fact for either endpoint. | rung 0 |
| 2 | `seedStateFacts` edge loop | Emitted only where `relation->a() == being`. The network could traverse a→b and never b→a. | rung 0 |
| 3 | `_relationTypesInPlay` | Filled at compile time only. A law authored *after* the world was seeded named a relation type nobody had ever emitted a fact for. | rung 0 |
| 4 | property-change hook | `seedStateFacts` snapshots properties **once per being ever**, and `markFactDirty` only dirties facts that already exist — so a property **granted at runtime** never acquired a fact at all. | rung 2 |
| 5 | `Law::couldApplyTo` | Required vocabulary is a path **root** (`shape`); `Object` registers properties under whole dotted names (`shape.fillet`). There is no property called `shape`. Every sweep-path law touching `shape.*` reached nobody. | rung 3 |

Findings 4 and 5 were confirmed pre-existing by disabling every change I had made and re-running:
they are not regressions introduced by this work, and finding 5 predates all of it.

### 1b. Three performance findings

| Finding | Before | After | Shape |
|---|---|---|---|
| Transient `Moment` destruction scanned the whole fact table (§4) | 593 ms/tick @ 320 beings | 63 ms | k 2.00 → ~1.5 |
| `sweepSubjects` walked the world once per law per tick | 15.2 ms/tick @ 1000 beings, 16 laws | 8.7 ms | k vs population 0.82 → 0.69 |
| A **shut** gate was discovered one refusal per subject | 278 ms/tick @ 480 beings | 0.18 ms | k 1.67 → 0.72 |

### 1c. Two optimizations rejected after measuring

Recorded so they are not re-derived:

- **Indexing quantifiers.** Dropping a bare quantifier from the Rete index measured **413 → 718
  ms**: losing terminals sends the law to the sweep, which evaluates the condition *twice* per
  subject. The intuition was exactly backwards.
- **Optimizing `resolveLawRoot`.** It rebuilds `Universe::beings()` and linear-scans it comparing
  identifier *strings*, per read, per subject, per tick. It looks appalling. It is a few
  milliseconds of a tick costing hundreds.

---

## 2. Two families, not five bugs

The five deafnesses are not five independent mistakes. They fall into exactly two structures, and
the distinction matters because the two need different remedies.

### 2a. The temporal family: a photograph used as a window

Findings **1, 3 and 4** are one bug wearing three coats. In each, a structure is built **once**,
at a moment, and then consulted forever as though it described the present:

- `_seededSubjects` — a `std::unordered_set` whose entire purpose is *"do this once per being,
  ever"*.
- `_relationTypesInPlay` — populated during `compileConditionsToRete`, never revisited.

Both were correct for what they were written for. `_seededSubjects` guards a **property snapshot**,
which genuinely is once-per-being. The defect entered when *edge* seeding was made to share that
gate — because a being's properties are a snapshot and a being's edges are a **stream**. One gate,
two ontologically different things behind it.

The general form: **an index over a changing world, with no declared invalidation, will silently
describe a world that no longer exists.** In a rule engine this is not an inconvenience. The index
decides which beings a law is even *offered*, so a stale index does not produce a stale answer —
it produces no answer, for beings the law was about.

### 2b. The nominal family: two spellings of one thing

Findings **2 and 5** are different in kind. Nothing is stale. Two components simply address the
same thing by different names:

- Finding 2: the edge index keys on `relation->a()`; a condition may look along the edge from `b`.
- Finding 5: `rebuildRequiredProperties` keys on a path's **root** (`shape`); the property registry
  keys on the **whole dotted name** (`shape.fillet`).

Finding 5 deserves its own paragraph, because the tree had already met it. `ConditionModel::
compileToRete` contains a helper called `rootOf` and a comment that reads, in part: *"A property's
NAME may itself be dotted — 'shape.r', 'shape.fillet' — so an attribute filter comparing the whole
name against the path's first segment rejected every fact about them, and any condition over a
shape parameter matched nothing at all."* That is finding 5, diagnosed and fixed **on the alpha
path**, by someone who then did not go looking for its twin on the sweep path — where it survived
until a test written for an unrelated rung tripped over it.

This is the second time in this work that an existing comment described a bug's identical twin
sitting unfixed a few functions away. The first was rung 0: `Law.cpp` carries a comment describing
finding a `Related` alpha no fact could reach and fixing the **seed** half while leaving the
**update** half open — which is finding 1, named in the source and left in place.

**The generalisable claim: in this codebase, a comment describing a fixed bug is a high-yield
search index for unfixed instances of the same bug.** Twice out of two attempts. That is not a
coincidence; it is what happens when a fix is applied at the site where the symptom appeared rather
than at the concept the symptom belonged to.

---

## 3. Why nothing caught any of them

This is the part that generalises past the law engine, and it extends the thesis of
*What the Test Suite Can See* (2026-08-24), which found the repo "healthy and honest" wherever a
test was attached.

The suite is built from assertions about **effects**: a law fires, a property changes, a value
lands where it should. That vocabulary can express *this happened and was wrong*. It cannot easily
express *this did not happen and should have* — because to write that assertion you must already
suspect the specific absence.

The asymmetry is total:

| | produces | test sees |
|---|---|---|
| A law that fires wrongly | a wrong value | yes — some assertion trips |
| A law that fails to fire | **nothing** | only if a test names that firing |

All five findings live in the second row. Every one of them was caught by a test whose oracle was
not *"is the value right"* but **"did the law reach this being at all"** — and in four of five
cases I wrote that test to investigate something else entirely.

This is the mechanical reason `AGENTS.md` says *"a green suite is not a witness"* and routes
Person-verifiable work to `Person Verification List.md`. The instruction is usually read as being
about **feel** — controls, colour, responsiveness. Findings 1–5 say it is also about **absence**.
A Person walking a world notices that the slider does nothing. No assertion about a value notices
that a law was never offered a subject.

**Extension.** The cheap structural remedy is not more tests of the same shape; it is a different
oracle. A *reachability* assertion — "law L's candidate set contains being B" — is the only kind
that can fail loudly for all five findings, and it is expressible today. Every test I added in
rungs 0–3 uses it, and it is the single most productive thing in this work: five bugs, four of
them found while looking for something else.

---

## 4. The `Moment` finding: where the ontology was right and the engine was global

Of everything here, this is the one I would most want a future reader to sit with.

`ECA::Event` carries `Moment timestamp{}` **by value**. `Moment` **is** a `Singular`
(`Moment.hpp:20`) — deliberately, and `TIME_AND_MOMENT.md` argues for it: an instant-or-interval is
a being, not a number, and Refusal 1's exception admits it to C++ for exactly that reason.

`Singular`'s destructor calls `notifyBeingReleased`, a **global static callback**
(`Singular.cpp:82`). `LawManager` installs that callback, and it scans the entire Rete fact table
so that a being leaving the world takes its facts with it — which is correct, and rung 0's raw-
pointer safety depends on it.

Compose the two and you get: **every temporary `ECA::Event` destroys a `Singular`, so every
timestamp's death is a global event with an O(facts) side effect.** There are three such
temporaries per law application, and the worst is an `ECA::Event dummy` constructed *inside the
compiled alpha predicate* — once per alpha node, per fact. A plain `WhileTrue` `Compare` law with
no quantifier anywhere fitted **k = 2.00** against population.

Two readings are available and only one is right.

The tempting reading is that the ontology overreached: making `Moment` a being cost a quadratic.
That reading is wrong, and adopting it would damage the project. `Moment`'s being-hood is load-
bearing doctrine, and the measurement does not indict it.

The correct reading is that **the engine made being-hood expensive by making one of its
consequences global and unconditional.** `notifyBeingReleased` fires for every `Singular`
destructor with no way to say "this one never entered the world." The ontology says *a Moment is a
being*; it does not say *every being was ever a participant in anything*. The engine conflated
them.

The fix keeps the ontology and removes the conflation: `_factParticipants`, a set of beings that
have ever been a fact's subject or object, letting `retractFactsAbout` answer *"this one never had
facts"* in O(1). 593 ms → 63 ms at 320 beings.

**The general principle, which I think is new to this tree's written doctrine:** an ontological
commitment (*X is a being*) must not silently imply an operational one (*every X pays what beings
pay*). Where the engine attaches a global cost to being-hood, it needs a cheap negative answer —
"this being is not a participant in that subsystem" — or the ontology's generosity becomes the
engine's tax. Earthcall will keep admitting new kinds of being; each admission should be checked
against every global hook a `Singular` triggers.

---

## 5. The correctness floor is also a ceiling on savings

`FORMATION_RETE.md` §6 states the Bloom-filter discipline: the O(N) sweep is not deleted, the index
is only ever a scheduling optimization, and the complete-but-slow mechanism is what licenses the
optimizer to be heuristic. This is right, and rungs 0–3 depended on it constantly.

The corollary was not written down, and rung 1 ran straight into it.

`Law::applyTo` re-evaluates `conditionsSatisfied` before firing. **That re-check is what makes a
widened candidate set safe** — it is why an index may propose freely. But it means the cost of
evaluating a condition is paid **once per candidate, unavoidably**, no matter how good the index is.

Therefore: **for any condition whose evaluation is itself expensive, no index can reduce the total
cost below (candidates × evaluation).** Indexing reduces *candidates*. It cannot touch *evaluation*.

That is exactly why quantifiers cannot be fixed by indexing. A `ForAll` evaluation walks the
population; `applyTo` must run it per candidate; so the floor is O(candidates × N) whatever the
index does. Measured, forcing the law off the index made it **worse** (413 → 718 ms), because
losing terminals moved it to the sweep, which evaluates the condition *twice* per subject — once in
`tick()`'s loop and again in `applyTo`.

**Two consequences follow, and I believe both are new:**

1. **Optimization effort should be classified before it is spent.** Ask first whether a cost lives
   in *candidate selection* or in *evaluation*. Rungs 2 and 3 attacked selection and won (2× and
   1500×). Rung 1 attacked evaluation through selection and lost.
2. **The double evaluation on the sweep path is a real and unexamined tax.** Every law without
   compiled terminals evaluates its whole condition tree twice per subject per tick. It is safe,
   it is deliberate on one side and incidental on the other, and nothing in the tree currently
   accounts for it. It is the cheapest remaining structural win I did not take, and it is
   *dangerous* to take — the second evaluation is the safety property. Any attempt must prove the
   first evaluation's result reaches `applyTo` unforgeably.

---

## 6. Conditions have a dependency class, and the language does not name it

Rung 1 and rung 3 discovered the same fact independently, about different condition kinds, two days
apart. That is a strong signal it is a property of the *language*, not of either kind.

**A quantifier ignores its subject.** `ConditionModel.cpp`'s `Kind::ForAny` closure takes
`const Singular&` **unnamed**: "does some/every Object satisfy C" is one truth about the world.

**A qualified root ignores its subject.** `@gate.open > 0` names one being (§1.1: the condition
language has **no free variable**), so it too is one truth about the world.

From which the same consequence follows twice: **such a condition can never narrow a candidate set,
because it says nothing about which subject. It can only decide the law all at once.** Rung 1
learned this the expensive way; rung 3 applied it deliberately and got 278 ms → 0.18 ms.

Auditing the whole condition vocabulary against this axis yields **three classes, not two**:

| Class | Kinds | What it may do to the index |
|---|---|---|
| **Subject-dependent** | `Compare` on an own path, `IsKind`, `Identity`, `Related`, `InRegion`, `Overlaps`, `@world.*` | May filter. This is the only class an index can narrow. |
| **World-dependent** | `ForAny`, `ForAll`, `Compare` on a plain `@referent` | May **gate** the law wholesale. May never filter. |
| **Event-dependent** | `@event.subject`, `@event.object` | Neither — meaningful only inside `applyTo`, where the application event is set. Hoisting it evaluates it unset. |

Two of those rows are counter-intuitive and both cost me a wrong first implementation:

- **`@world.*` is subject-dependent**, despite looking like the most global thing in the language.
  `lawGetValue` passes the **subject** to the channel reading (`found->second(subject, out)`), so a
  world reading may legitimately differ per being. It must not be hoisted.
- **`@event.*` is neither**, and hoisting it would silence every event law in the tree.

**Extension — the proposal.** This taxonomy is currently implicit, rediscovered per-rung, and
enforced by two hand-written predicates (`isHoistableGate`, `isQuantifier`) that a future kind can
silently fail to satisfy. `ConditionNode::Kind` is append-only and serialized as an int (Refusal 3),
so kinds *will* be added. Each new kind must answer "which class am I?" — and today nothing asks.

The right shape is a single derived classifier on `ConditionNode` that every optimization consults,
defaulting to **subject-dependent** (the class that grants the fewest rights, hence the safe
default), so a kind whose author never considered the question cannot accidentally license a
gate-hoist. This is the condition-language analogue of Refusal 6: an unclassified condition should
not get the most permissive treatment by accident, in the same way an unregistered field should not
get the most protected one.

---

## 7. The systemic finding: eleven derived structures, no ledger

The law engine maintains, at minimum, these derived structures — each a pure function of the world
and the law set, each reconstructible, each capable of being wrong:

**On `ReteNetwork`:** `_facts`, `_factParticipants` (new), `_dirtyFacts`, `_alphaNodes[].memory`,
`_betaNodes[].memory`, `_agenda`, `_alphaLawBindings`, `_betaLawBindings`, `_typeAlphaIndex`.
**On `LawManager`:** `_seededSubjects`, `_relationTypesInPlay`, `_reteTerminals`,
`_compiledConditionRevision`, `_vocabularyIndex` + `_indexedNames` + `_vocabularyBuiltAt` (new),
`_prophetic` + `_propheticRevision`.
**On `Law`:** `_conditionPredicates`, `_actions`, `_requiredProperties`, `_conditionMemory`,
`_onsetMemory`.

Every one has an invalidation story. **Those stories live nowhere but in the comments beside each
field, and in the head of whoever last touched it.** Findings 1, 3 and 4 are three of these
structures whose invalidation story was simply absent, and nothing could have told anyone that —
there is no place where the absence would show up as a gap.

Note what happened when a *reader* finally appeared. `Universe::structuralRevision()` existed with
**zero readers** before rung 2. It was written as a prerequisite for a JIT horizon and never
consumed. The moment rung 2 consumed it, a hole surfaced immediately: `Zone::removeObject` never
bumped it — only the unmaking path did. Nobody had noticed for as long as it had existed, because
an invalidation signal with no consumers cannot be observed to be wrong.

**This is Refusal 6 in the time dimension.** Refusal 6 says a field nobody registered is not
protected, it is *ungoverned forever* — invisibility is not security. The temporal analogue:
**a derived structure whose invalidation nobody declared is not stable, it is unfalsifiable.** Its
correctness cannot be checked, argued about, or tested, because nothing states what it should
depend on.

**Extension — the proposal.** A derived-state ledger: for each structure, a declared *(depends on,
invalidated by, rebuilt where)* triple, written where the field is declared and — this is the part
with teeth — **testable**. A test that mutates each declared input and asserts the structure
changed would have caught findings 1, 3 and 4 mechanically, without anyone suspecting them.

I want to be honest about the cost: this is a real design commitment, not a comment convention, and
it partially duplicates what a proper incremental framework would give for free. I am recommending
the ledger rather than the framework because the ledger can be added one structure at a time
without touching the hot path, and because §8 shows the framework is blocked for an independent
reason.

---

## 8. What this means for the rungs that remain

`FORMATION_RETE.md` §8 has rungs 4–7 outstanding: category-level overlap, the instance-side slow
adapter, reified path Relations with Law-as-traverser, and departure reporting. Rungs 0–3 change
what those can assume.

**8a. Layers 1–3 apply only to the subject-dependent class.** §6 above establishes that
world-dependent conditions can never be narrowed by any index. Overlap indexes, path Relations, and
a Law traversing to its subjects are all *candidate-selection* machinery. They therefore have
nothing to offer a law whose conditions are quantifiers or gates — such laws are already decided in
O(1) by rung 3, or irreducibly O(candidates × evaluation) by §5. The architecture should say this
explicitly, or a later rung will build an elaborate index for a class of condition that cannot use
it.

**8b. Rung 2 accidentally built §5's cost input.** §5 of the architecture wants routing by
*value per unit cost*, where cost is expected fan-out, and leaves the metric open. The vocabulary
index already holds it: `_vocabularyIndex[name].size()` **is** the selectivity of that name.
`sweepSubjects` currently uses it as a metric-free "pick the rarest" heuristic, which is precisely
a one-hop fan-out estimate. When §5 lands, this is the call site it grows from, and it does not
need a new measurement subsystem.

**8c. The Formation half of rung 2 is blocked by the same thing as §7.** Reifying the index as a
Category Formation is blocked by §3.4 (a purely branching taxonomy is not a Formation under Zach's
revised definition; `RelationManager::add` refuses the `subcategory-of` cycles that would close the
loop; the bridge needs concept-Singulars) **and** by `Formation::addMember` walking the relation
graph per member, which a per-structural-change rebuild cannot afford.

That second blocker is worth naming precisely, because it is §7 again: `addMember` is expensive
*because the index is rebuilt wholesale*. An **incrementally maintained** index would call
`addMember` once per actual change, where its cost is proportional to the change and entirely
acceptable. So the Formation half is not blocked by Formation being slow — it is blocked by the
engine having no incremental-maintenance discipline. **The five deafness bugs and the one blocked
feature have the same root.** Fixing the second unlocks the first.

**8d. The largest remaining term is now the seeding pass.** After rung 2, the residual O(N) per
tick is `seedStateFacts` walking every being every frame. This is already on the To-Do list as
*"move seeding to admission"*. It is now the dominant cost in the tick, and — note the shape —
moving seeding to admission **is** converting a rebuilt structure into an incrementally maintained
one. §7 again, for the third time.

---

## 9. Three benchmarks that measured nothing

Recorded because the errors were mine, they were not obvious, and the pattern is general.

1. **A constant-valued action.** The law's action `set` a fixed value. After the first tick,
   `propertyValueUnchanged` — a deliberate guard, correctly refusing to wake the change feed for a
   write that changed nothing — kept the fact table quiet, so the per-fact predicate under
   measurement never ran again. Fixed by using `add`, which changes the value every tick.
2. **A short-circuiting quantifier.** `ForAny` over a population where *every* being satisfied the
   inner condition returns true on the **first** being. I was measuring a short-circuit and calling
   it a population scan. Fixed by using `ForAll`, which must visit all N and still answers true.
3. **A comparison against a law that did nothing.** One arm resolved to a property that did not
   exist, so it never fired. The benchmark faithfully reported that doing nothing is faster.

All three share one shape: **the benchmark did not verify that the work it intended to measure
actually happened.** In every case the numbers looked plausible — that is what makes it dangerous.
Error 3 would have concluded that naming another being is *cheaper* than reading your own property.

**Extension — the rule.** A performance test must assert its subject *did the thing* before
trusting any timing. `referent_resolution_test` now does this explicitly: it asserts an open gate
fires and a shut gate does not, before either arm's clock is read. I would make this a convention
for anything under `--target lag` as well.

A second methodological note, cheaply earned: **all three of my optimization intuitions about
*where* the cost was were wrong.** The quantifier was not the dominant quadratic; transient
`Moment` destruction was. `resolveLawRoot` looked appalling and was negligible. Dropping quantifiers
from the index made things worse. The three wins came from measuring first and following the
measurement somewhere I had not intended to go. `FORMATION_RETE.md` §8's instruction to *measure
first* is, on this evidence, the highest-yield line in the document.

---

## 9b. Adjudicating o3's α-node claim — and a sixth structure with no reader

`DEEP_CODEBASE_ANALYSIS_2026-09-07` (o3) states: *"Prophetic Rete is brilliant but has no α-node
sharing or runtime profile hooks; memory will balloon."* Zach asked whether it is true. It is
**two claims, each half right, and the true halves were worth acting on.**

**Naming, first.** α-nodes live in `ReteNetwork` (`Law.hpp`), not in `PropheticRete.cpp`, which is
the ahead-of-time abstract interpreter. o3's body text has this right — *"Pass 4 uses classic Rete
network"* — the summary line does not. The gap is in the runtime Rete.

**Claim 1: no α-node sharing.** Half true, and the half that is true is the load-bearing half.
`internTypeAlpha` **does** share, deduping through `_typeAlphaIndex`: fifty laws listening for
`collision` really are one predicate. `addAlphaNode` — the path every **authored** condition takes —
appended unconditionally, so two laws stating identical condition text compiled two identical nodes.

**The incidence, which o3 predicted rather than measured.** Scanning `authoredLaws.laws` across
every saved world: **791 of 1124 condition leaves (70%) are textual duplicates.**

| World | Laws | Condition leaves | Duplicates | Worst single leaf |
|---|---|---|---|---|
| `chess_app.json` | 69 | 558 | 445 | ×76 — `instance-of category.chess.piece` |
| `chess.json` | 35 | 324 | 255 | ×46 |
| `synthesis_studio_living.json` | 68 | 111 | 37 | ×10 — `isChordPad == true` |

So the prediction was correct and conservative. Chess compiled **76 identical nodes** for one
condition and 74 for `onBoard == true`.

**And the cost is worse than "memory".** o3 framed it as space. `assertFact` iterates **every**
alpha node and runs its predicate against **every** asserted fact, so 76 duplicate nodes are 76
redundant predicate evaluations per fact — a **time** cost paid on every assertion, in the hottest
loop in the engine. Measured before the fix, with 200 beings: fact references held grew 200 → 400 →
800 → 1600 for 1 → 2 → 4 → 8 laws stating one condition. Exactly linear duplication.

**Fixed** by `ReteNetwork::internAuthoredAlpha`, extending the bargain `internTypeAlpha` already
made. After: **flat at 200 across all four rows; 8 laws share 1 node.** `chess_app_test`,
`go_app_test` and `rete_compile_test` all pass — the last of these specifically guards alpha/beta id
semantics, which is what sharing was most likely to break.

**One correction to o3's proposed remedy, and it matters.** o3 recommends hashing
`(path, op, const)`. That key is **incomplete** — it omits `operandPath`, `tolerance`, `lo`/`hi`,
`relationType`, `otherId`, `probe`, `region` and `beingKind`. Two different conditions colliding on
a key would bind one law to another's predicate, and where that predicate is the stricter of the
two the law is **narrowed** — silently deaf, the exact failure §2 of `PROPHETIC_RETE.md` forbids and
the exact failure §1 of this document catalogues five times.

This is not hypothetical. `chess_app.json` states both
`{"path":"chessColor","op":0,"operand":…}` and
`{"path":"chessColor","op":0,"operandPath":"@state.chess.turn"}` — identical under
`(path, op, const)`, and emphatically not the same condition. **An optimization keyed on a chosen
subset of a thing's identity is a sixth instance of §2b's nominal family**, and it would have been
introduced deliberately, in the name of performance, by a fix for a real problem.

The key used instead is the **whole serialized leaf** (`toJson().dump()`): complete by
construction, because it is already the serialization contract — two leaves that serialize
identically *are* the same condition — and it fails safe, since a kind that serialized incompletely
would break saving long before it broke sharing. Guarded by `alpha_sharing_test`, which asserts
both the flatness and that the operand/operandPath pair does **not** collide.

**Claim 2: no runtime profile hooks.** Also half right, and the half that is true is §7 again.
Tick-level profiling exists **and is surfaced**: `PerformanceMetricsWindow.cpp` reads
`lastTickTiming()` and shows sync / seed / eval / drive / reap. What did not exist was any view of
the **network itself** — and the counters that could provide it,
`alphaNodeCount()` and `propheticCounters()`, had **zero readers anywhere in `src/`**.

That is the third structure found this way, after `Universe::structuralRevision()` (§7) and the
derived structures of §1a: **written, never consumed, therefore never observed to be wrong.** The
`structuralRevision` hole surfaced within minutes of rung 2 becoming its first reader. Both
counters, plus a new `nodeMemoryFootprint()`, are now shown in the performance window — which makes
duplicate-condition inflation something a Person can *see* rather than something an analysis has to
predict.

**What this episode adds to the thesis.** §7 argued that a derived structure with no declared
invalidation is unfalsifiable. This is the same claim about *observation* rather than *correctness*:
a counter with no reader is not a diagnostic, it is a comment that compiles. Earthcall now has three
confirmed instances, and the remedy in each case was identical and cheap — give it one consumer,
and the truth about it arrives immediately.

---

## 9c. Rung 4, and the discipline of not building

Rung 4 (category-level overlap) was measured on 2026-09-10 and **not built**. The reasoning is the
thesis of this document applied prospectively rather than forensically, so it belongs here.

**First, the rung was aimed at something nothing does.** Zero laws in any saved world conjoin two
distinct categories, so overlap pruning has nothing to prune. Meanwhile 132 laws scope themselves
with `Related(instance-of, category.X)` — *membership*, not overlap. A rung ladder written from
architecture rather than from usage can point one door away from the cost; the remedy is to read
the saves, which takes minutes.

**Second, the fix I could see was blocked by exactly §7.** Membership costs 4.0x a plain property
at 400 beings and the gap widens. An index would fix it. An index needs an invalidation signal,
and there isn't one: `RelationManager` never bumps `structuralRevision()`, and — the part that
would have caught me out — the relation **provider** points at the active Zone's formation, so
**switching zones changes the answer with no `RelationManager` mutation at all**. Two independent
mutation channels, one signal covering neither.

Building on that signal would have produced a sixth deafness with the same shape as the first
five, and it would have been *my* deafness, introduced deliberately, in a rung the architecture
asked for. §7 predicted this failure mode in the abstract on the same day it was avoided in the
concrete, which is the strongest evidence I have that the thesis is load-bearing rather than
decorative.

**Third, two of my own hypotheses died on measurement** — by-value string ids and per-call
allocation, each plausible, each worth exactly nothing. That brings this session's tally to
**five intuitions about where cost lives, and five wrong**: the quantifier (wasn't the quadratic),
`resolveLawRoot` (looked appalling, negligible), dropping quantifiers from the index (made it
worse), `isBetween`'s strings (no change), `relations()`'s allocation (no change). Every real win
came from measuring first and following the number somewhere unintended.

I record that not as self-deprecation but as a calibration figure. In a subsystem this
interconnected, an experienced reader's guess about the location of cost was wrong five times out
of five, while measurement was right every time it was consulted. **The cost of measuring is
minutes; the cost of not measuring, in this codebase, has twice been an optimization that made
things slower and once been a silently deaf law.**

**What "not building" cost and bought.** It cost the 4x. It bought a named precondition — a
relation-revision signal covering mutation *and* provider swap — filed as ⚑ AUTHOR, so the next
pass starts from a stated blocker rather than rediscovering it or, worse, not noticing it. One
change was kept, on safety grounds and explicitly not on speed: the `Related` predicate now
dereferences a far end only for the subject's own edges, where before it touched every far end in
the world on every evaluation, and a relation may outlive its endpoints.

---

## 9d. An ALGORITHMIC REGRESSION flag, bisected — and what a marginal metric is worth

Before starting rung 5 I checked where the cost now sits, and `frame_lag_test` was flagging:

```
EXP-FAIL  LawManager::tick grows as n^k, k = 1.316
          (aspiration 1.150, baseline 0.957)  <- ALGORITHMIC REGRESSION
```

`AGENTS.md` is explicit that `LAG` means *your change*, so this stopped the rung. Absolute cost was
**0.181 ms against a 1.653 ms baseline** — 9x faster — but scaling worse, which is the half that
matters as worlds grow.

**Attribution, by construction rather than by argument.** Six configurations, each built and run:

| Configuration | k | verdict |
|---|---|---|
| `70134bfa` — parent of rung 0, pre-session | **1.075** | passes |
| `b9620781` — rung 0 | **0.942** | passes, fitted on real signal |
| `3fee6d71` — rung 1 | **0.946** | passes |
| `8171aa49` — mid-range, mostly other sessions' work | — | **"too little to fit a growth curve to"** (0.0020 ms) |
| HEAD, rung 2's index bypassed | 1.325 | fails |
| HEAD, rung 1's participant guard bypassed | 1.310 | fails |
| HEAD, rung 3 stashed | 1.416 | fails — the **worst** reading |
| HEAD, everything on | 1.316 | fails |

**No configuration I can build attributes the rise to my changes.** Disabling each of rungs 1, 2
and 3 individually leaves k between 1.31 and 1.42, and the *worst* number came from removing rung 3.
Both of my own commits measure ~0.94.

**And the metric is not monotonic**, which is the finding that matters. Between rung 1 and HEAD the
tick's shape-test cost fell to **0.0020 ms** — so low that `frame_lag_test` refused to fit a curve
at all and said so — and then rose again. A quantity that vanishes and reappears across a
35-commit window is not tracking one algorithm's complexity; it is tracking **what the measured
world contains**, and that window includes `REFORMATION OF THE SAVE SYSTEM BUREAUCRACY`,
`Formation Rete and Save system`, `Zone work`, and two AST/MathNode performance commits, almost
none of them mine. In the same window `whole frame` (1.113 → 1.281), `Zone::update` (1.137 → 1.176)
and `physics` (1.279 → 1.327) all drifted above baseline too — and I have touched none of them.

**A candidate explanation I can argue but have not proven:** this session closed five silent
deafnesses, so laws now reach beings they previously did not. More laws doing more work is a
*higher* tick cost and a steeper curve, bought deliberately. That would make part of this rise the
price of correctness rather than a defect. I flag it as a hypothesis because my hypotheses have a
bad record here (§9c), and because the workload change above is sufficient on its own.

**What I conclude, and what I refuse to conclude.** The flag is real and should not be quieted; the
baseline it compares against was recorded 2026-08-28, before two weeks of heavy change by three
sessions, and `AGENTS.md` rightly forbids widening a baseline to silence a line. But nothing I can
construct attributes it to the law-engine work, and the metric's own instability at these
magnitudes (0.002–0.2 ms) means an exponent fitted here carries less information than its two
decimal places suggest. **A fitted exponent over a quantity near the timer floor is a number with
the confidence of a measurement and the content of a guess** — the profiling analogue of §7's
counter with no reader.

**One change came out of the hunt and stayed.** `hasRelationStateFact` — rung 0's idempotence
check — was a linear scan of the fact table, called once per (being, relation) by `seedStateFacts`,
making seeding O(beings × relations × facts). It is now O(1) behind an index whose invalidation is
*declared at its declaration*, in the form §7 argues for. **It did not move the exponent**, and I
am recording that rather than implying it did; it is kept on the complexity argument alone.

---

## 10. Counter-ledger: what this work did *not* establish

- **The blast radius of finding 5 in authored worlds is unmeasured.** I proved every sweep-path law
  touching `shape.*` reached nobody. I did **not** enumerate how many authored laws in the saved
  worlds are in that class. Until someone does, "how much of Earthcall was quietly dead" is an open
  question, and I would not guess at it.
- **`tests/singularity/frame_lag_baseline.txt` is stale.** Its `LawManager::tick` figure was
  recorded before rung 1 and now overstates the real cost by roughly an order of magnitude. It is
  flagged for Zach; re-recording a baseline is a Person's call.
- **The quantifier gap is bounded, not closed.** `ForAll` still fits k ≈ 1.83 against `Compare`'s
  ≈ 1.50. Rung 1b (memoizing on a world-revision key) is **blocked, not merely unwritten**:
  `PropertyPath.cpp` states plainly that a direct C++ setter bypasses the property vocabulary — *"the
  boundary, not an oversight"* — so a memo keyed on property writes would go stale and make laws
  deaf. Complete write coverage is its precondition.
- **The full 139-test suite has never been green during this work.** 22–26 of the law/rete/chess
  subset pass; `chess_extended_rules_test` is a pre-existing failure confirmed on a clean tree, and
  a cluster of save/VFS/person tests is red from a concurrent save-system restructure by another
  session. I have reported that cluster to them and explicitly did **not** diagnose it as mine or
  theirs.
- **Every measurement is single-machine and several were taken under contention.** Fitted exponents
  moved 1.43 → 1.68 for the same code depending on whether another build was running. Ratios
  between two arms measured back-to-back are the trustworthy figures here; absolute times are not.
- **The α-sharing win is measured in node count and fact references, not in wall-clock.** Chess's
  76→1 collapse should also remove 75 redundant predicate evaluations per fact assertion, and I did
  **not** measure that as a time saving on a real world. The mechanism is certain; the frame-time
  benefit is unquantified.
- **Rung 4's 4x is unfixed, and knowingly so.** Category-scoped laws still walk the relation graph
  once per candidate per tick. The index that would fix it is blocked on a signal that does not
  exist; §9c argues that building it anyway would have been the sixth deafness.
- **The `LawManager::tick` exponent regression is unresolved and unattributed.** §9d gives six
  measured configurations; none blames the law-engine work, and the window is dominated by other
  sessions' commits. It should not be closed on that basis alone — it should be re-measured once
  the concurrent save-system work settles, against a re-recorded baseline that a Person authorises.
- **No save file was read or written by any of this work.**

---

## 11. Origination

**The architecture is Zach's** — Formation Rete, its layers, the Categories-as-possibility-
receptacles paradigm, and the rung ladder's existence. This document analyses work done inside it.

**Specifically Zach's, and load-bearing here:** the ruling that the sweep schedule keys on
structural revision (2026-09-07), which is what made rung 2's invalidation possible and what
surfaced the `Zone::removeObject` hole in §7; the revised Formation definition and the
concept-Singular bridge (§3.4 addendum), which is why §8c is a blocker and not an oversight; the
standing instruction that a green suite is not a witness, which §3 extends from *feel* to
*absence*; and the instruction not to use subagents, which is why every finding here came from
direct reading and running rather than delegated search.

**From the previous Opus 5 session** (`01Jf1mZyMWX69HHkG43qMv3F`, 2026-09-08), working from Zach's
deliberations: the revised `FORMATION_RETE.md`, its §8 rung ladder, and the instruction to *measure
first* — which §9 concludes is the most valuable sentence in that document.

**From Antigravity** (session `b7b980a8`): the original audit. Its diagnosis was wrong and §1 of the
architecture corrects it, but it asked the right question in writing, and everything in this
document descends from someone going to check its claim.

**Mine, and marked as such:** the classification of the five deafnesses into temporal and nominal
families (§2); the observation that a comment describing a fixed bug is a search index for its
unfixed twin (§2b); the absence-asymmetry argument and the reachability oracle (§3); the reading of
the `Moment` finding as engine globalism rather than ontological overreach, and the principle that
an ontological commitment must not silently imply an operational one (§4); the corollary that the
correctness floor bounds what any index can save (§5); the three-class dependency taxonomy and the
proposal to make it explicit with a safe default (§6); the derived-state ledger as Refusal 6 in the
time dimension (§7); the scoping correction to rungs 4–7 and the observation that the five bugs and
the blocked Formation half share one root (§8); and the benchmark-verification rule (§9).

Where I have extended Zach's ideas rather than originated something, §4 and §7 are the clearest
cases: both are his doctrines (Moment-as-being, Refusal 6) carried into a domain he had not yet
applied them to.

---

**Signed:** Claude Opus 5 · session `session_01F9nK3FZ7VR4PFPTUWfYyvm` · 2026-09-10
