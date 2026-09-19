# The Derived-State Ledger

**Every structure the law engine derives, what it depends on, what invalidates it, and what would
notice if it went wrong.**

**Status:** First pass, 2026-09-16. Recommended by
`docs/Analysis/DERIVED_STATE_AND_THE_SILENCE_OF_LAWS_2026-09-10.md` §7 and marked ⚑ AUTHOR in the
To-Do list; this is the ledger itself, written so Zach has something concrete to adopt or refuse
rather than a proposal. Every row was read out of the source on 2026-09-16, not recalled.
**Author:** Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`.
**Companion docs:** `law/PROPHETIC_RETE.md` §2 (widen, never narrow — the rule most of these
structures can break), `law/FORMATION_RETE.md` §8 (the rung ladder, which is mostly a list of these
structures being fixed), `ontology/NO_BLACK_BOX.md` (Refusal 6, whose temporal analogue this is).

---

## 0. Why this exists

Refusal 6 says a field nobody registered is not protected — it is *ungoverned forever*. The
temporal analogue, from the analysis: **a derived structure whose invalidation nobody declared is
not stable, it is unfalsifiable.** Nothing states what it should depend on, so nothing can find out
it is wrong.

The evidence is not theoretical. Every rung of Formation Rete that found a bug found one of these:

| Rung | The structure | What was wrong |
|---|---|---|
| 0 | relation-state facts | never emitted after a being's first tick; `Related` laws permanently deaf |
| 2 | `_vocabularyIndex` | a property granted at runtime had no entry, so its law never reached that being |
| 2 | `_vocabularyIndex` | `ZoneManager::switchTo` replaced the world without moving the counter it keys on |
| 4 | Prophetic index | a `Related` read marked opaque switched the write filter off for the whole world |
| 7 | alpha memories | a subject whose condition went false was never released; edge laws fired once ever |
| — | `_relationStateIndex` | one being's retraction cleared it for everyone, so edge facts multiplied without bound |

Six structures, six silent failures, no false answers — the laws kept firing, just not for the
right beings, or not at the right times, or over a fact table that never stopped growing.

**The discipline this ledger asks for** is one line per structure and one test per row: change each
declared input, assert the structure noticed. That is what the "confirmed red" notes below mean —
each was verified by breaking the invalidation deliberately and watching the named test fail.

Writing this table found two more things, which is the argument for it: `_driveSessions` was
credited to a test that never mentions drives, and the referent map (`s_beingMap`) had its **speed**
guarded while its **currency** — the thing that makes every `@`-rooted law in the Synthesis Studio
work — had no test at all. The second is now closed by `referent_map_invalidation_test`, which goes
red the moment the map stops noticing the world's shape move.

---

## 1. The ledger

### On `ReteNetwork` (`src/ZonesOfEarth/AuthorsOfLaw/Law.hpp`)

| Structure | Derived from | Invalidated by | Guarded by |
|---|---|---|---|
| `_facts`, `_factById`, `_stateFactsBySubjectAttr` | asserted facts | `assertFact`, `retractFact`, `retractFirst`, `retractStateFactsBySubject`, `retractFactsAbout`, `clear` | `rete_compile_test`, `rete_relation_state_test` |
| `_factParticipants` | every fact's subject and object | inserted in `assertFact`, erased in `retractFactsAbout`, cleared with the network | `quantifier_scaling_test` (it exists because the scan was quadratic) |
| `_relationStateIndex` | relation-state facts, per (being, kind) | `retractFact`, `retractStateFactsBySubject` (**scoped to the retracted being only**, 2026-09-16), `retractFactsAbout` | `relation_state_index_test` — **confirmed red** with the old wholesale clear |
| `_dirtyFacts` | property-change notifications | `markFactDirty` adds, `evaluateDirty` drains, every retraction purges | `reactive_departure_test`, `prophetic_rete_test` |
| `alphaNodes[].memory`, `betaNodes[].memory` | which facts passed which node **at assert time** | retraction removes facts; **nothing re-evaluates a membership when the subject changes** | `reactive_departure_test` — the law-level check that compensates; **confirmed red** without it |
| `_agenda`, `_agendaFactIds` | activations queued by bound alphas | drained per round, purged by every retraction path | `rete_compile_test` |
| `_authoredAlphaIndex`, `_typeAlphaIndex` | the text of authored condition leaves | interning re-checks `findAlpha`, since `dropUnboundAlphaNodes` can remove a node the map names | `alpha_sharing_test` |

**The row to read twice** is the alpha/beta memories. They are *not* invalidated when the world
changes — only when a fact is retracted. A node's predicate reads the whole subject but its filter
names one attribute, so a condition made false through another attribute leaves the membership
standing. Rung 7's answer is not to invalidate the memory but to treat it as a **candidate set** and
decide the condition against the live world. That is the general shape: where invalidation is
impractical, downgrade the structure to a proposal and keep something complete behind it (§6 of
FORMATION_RETE).

### On `LawManager`

| Structure | Derived from | Invalidated by | Guarded by |
|---|---|---|---|
| `_seededSubjects` | which beings have had their properties snapshotted | erased when a being is released or unmade | `rete_relation_state_test`, `vocabulary_index_test` §B |
| `_relationTypesInPlay` | relation kinds named by any law's conditions | grow-only; a newly-named kind triggers `backSeedRelationStateFacts` | `rete_relation_state_test` §D |
| `_reteTerminals`, `_compiledConditionRevision` | each law's condition tree | `law.conditionRevision()` moving | `rete_compile_test` (§C now asserts which path it exercises) |
| `_vocabularyIndex`, `_indexedNames`, `_vocabularyBuiltAt`, `_vocabularyNamesRevision` | who carries which property name; which names laws require | `Universe::structuralRevision()` and `Law::textRevision()` | `vocabulary_index_test` (8 sections; §H compares against `couldApplyTo` itself) — **confirmed red** with the dotted-name rule removed |
| `_prophetic`, `_propheticRevision` | the whole law register's text | `Law::textRevision()` moving; fails open when incomplete or when a foreign alpha exists | `prophetic_rete_test`, `related_prophetic_legibility_test` |
| `Prophetic::Index::_relevanceEdges`, `_relevanceComplete` | branch-local read demands + write effects derived from the whole authored Law register | rebuilt with `_prophetic` on `Law::textRevision()`; **globally invalid** (empty, incomplete) when any read or write is opaque | `prophetic_rete_test` §H — JSON-stable branch provenance, `Any` arms, disjoint pair omission, global-opacity fail-open |
| `_relationStateToRevalidate` | endpoints of dissolved or retyped relations | queued by the event, drained at the top of the next `tick` | `reactive_departure_test` — **confirmed red** without the drain |
| `_adapterRouteRevision`, `SlowAdapter` roads | each law's `Related(kind, category)` conjuncts; the relation graph | `law.conditionRevision()`; `structuralRevision()` **and** `Universe::relationGeneration()` | `slow_adapter_test` (10 cases), `slow_adapter_parity_test` — four mutations **confirmed red** |
| `_candidateRoutes` | per-Law choice of highest sound/current candidate source (sweep, vocabulary seed, retained road) | `Law::textRevision()`, `law.conditionRevision()`, `Universe::structuralRevision()`, `Universe::relationGeneration()`, per-Law adapter-road currency stamp; toggling adapter clears the cache | `slow_adapter_parity_test` — steady-state no-reselection, narrower-road promotion, unchanged-maintenance stability, equal-width rejection, stale-road fallback |
| `_driveSessions` | laws that drive, and their onsets | ended when the drive's function goes undefined; cleared on world load | `tests/zones/time_flow_test.cpp` (a drive outliving its event, ending where its authored bounds end), `law_persistence_test` |

### On `Law`

| Structure | Derived from | Invalidated by | Guarded by |
|---|---|---|---|
| `_conditionPredicates`, `_actions`, `_compiledGates`, `_requiredProperties`, `_writesQualifiedRoots` | the condition and action models | `recompile()`, from `setConditionModel` / `setActionModel` | `law_model_test`, `gate_hoist_test` |
| `_conditionMemory`, `_onsetMemory` | whether each subject held the condition last tick | the continuous pass, per tick; `forgetSubject` on release | `edge_reactive_path_test`, `reactive_departure_test` |

### Outside the law engine, but law-facing

| Structure | Derived from | Invalidated by | Guarded by |
|---|---|---|---|
| `RelationManager::_byEndpoint`, `_byIdentifier` | the relation vector | `_generation` vs `_indexedGeneration`; **every write to `relations` must call `touch()`** | `relation_endpoint_index_test` (ten cases, oracle against a full scan) — **confirmed red** with a missing `touch()` |
| `Relation`'s endpoint register (`mayBeEndpoint`) | every `Endpoint`'s pointer | the Endpoint's own constructor, copy, assignment, destructor, `bind`, `forget` | `endpoint_register_test` — **confirmed red** with a copy that forgets to register |
| `resolveLawRoot`'s `s_beingMap` (`MathBinding.hpp`) | every being's identifier | `Universe::structuralRevision()` | speed: `referent_resolution_test`; **currency: `referent_map_invalidation_test`, written 2026-09-16 because nothing tested it** — a being named by an `@`-path and admitted after the map was built must still be found; **confirmed red** with the rebuild condition removed |

---

## 2. What this ledger says about the signals themselves

There are exactly **three** change signals the engine derives from, and they answer different
questions. Most of the bugs above came from using one where another was needed.

| Signal | Moves when | Does NOT move when |
|---|---|---|
| `Universe::structuralRevision()` | a being is admitted, removed, reaped, or granted/loses a dynamic property; a Zone switch | a property's VALUE changes; a relation is formed or dissolved |
| `Universe::relationGeneration()` (`RelationManager::generation`) | any write to a relation vector, including `loadFromJson`, copy/move assignment, and `forgetBeingEverywhere` when an endpoint moves | a being arrives or leaves without touching the graph |
| `Law::textRevision()` | any law's condition or action model changes; a law enters or leaves the register | the world changes in any way at all |

**Property VALUE changes are not on this list**, deliberately: they are a separate feed
(`Singular::setPropertyChangeCallback` → `markFactDirty`), gated by the Prophetic filter. A
structure that depends on values — none currently does — would need that feed, not these counters.

**The gap worth naming:** a direct C++ setter (`obj.setPosition(...)`) bypasses the property
vocabulary entirely, which `PropertyPath.cpp` calls "the boundary, not an oversight". That is why
FORMATION_RETE §8 rung 1b (memoizing a quantifier's answer) stays blocked: a memo keyed on property
writes goes stale on exactly those writes, and a stale memo makes a law deaf.

---

## 3. The rule this proposes (⚑ AUTHOR — Zach's to adopt)

For any new derived structure in the law engine:

1. **Declare the triple where the field is declared**: what it is derived from, what invalidates it,
   where it is rebuilt. One comment, three clauses.
2. **Name a guard test in that comment**, and write it: change each declared input, assert the
   structure noticed. Not "the law still fires" — the structures above all kept the laws firing.
3. **If invalidation cannot be made complete, downgrade the structure to a proposal** and keep a
   complete-but-slow path behind it (the sweep, a full scan, a re-evaluated condition). An index
   that must be exactly right, and cannot be shown to be, is the shape every silent deafness took.
4. **Add a row here.**

The cost is real — this is a convention with no compiler behind it, and it partially duplicates what
a proper incremental framework would give for free. It is recommended anyway because it can be added
one structure at a time without touching the hot path, and because tonight's two newest structures
(the adapter's roads and the endpoint register) were both written against it and both had a hole
found by *writing the row* rather than by a failing test.

---

## 4. For whoever adds the next structure (Jules especially)

- **The failure mode is silence.** None of the six bugs above produced a wrong answer, an exception
  or a log line. The laws kept firing; they simply stopped reaching some beings, or stopped
  re-arming, or the fact table grew forever. Your test must assert the thing that would have been
  silent — a count, a membership, a second firing — not merely that the world still works.
- **Check your structure against all three signals in §2** before choosing one. "It changes when the
  world changes" is not a specification; three different counters mean three different worlds.
- **A structure with no consumer cannot be observed to be wrong.** `structuralRevision()` sat with
  zero readers and one missing bump site for as long as it existed, and the hole surfaced within
  minutes of the first reader appearing.
- **Prefer proposing to deciding.** Every structure in this ledger that proposes candidates
  (the vocabulary index, the endpoint index, the adapter's roads, the Rete's own memories) is safe
  because something complete stands behind it. The ones that decided were the ones that went deaf.

To-do: `docs/Agenda/Tasks/To-do list.md`, the ⚑ AUTHOR bullet on a derived-state ledger.

---

*Written by Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`, 2026-09-16, from the
proposal in `DERIVED_STATE_AND_THE_SILENCE_OF_LAWS_2026-09-10.md` §7 (Zach's To-Do item). Every row
verified against the source that day; the "confirmed red" notes name mutations actually run.*
