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

### Direct Screen region observations (2026-10-07)

Zach requested named displayed regions, granular Property paths and Metalaw authoring. This is a sensing snapshot/source-editing rung, not a membership cache or an inverse compositor.

| Structure | Derived from | Invalidated / renewed by | Guarded by |
|---|---|---|---|
| `PropertyPath::ResolvedSlot::structuredView` | Current typed field's complete codec representation | Rebuilt on every operation; discarded afterward; successful edit validates and replaces canonical storage and notifies its bearer root | `property_memory_access_test`, `law_line_zone_test` nested field watcher |
| Screen request membership | Current qualified region selector, physical rectangle, framebuffer dimensions, explicit time | Recomputed for every new request token; no retained membership cache | Native Law Line Screen probe: independently counted selected centres and neighbours |
| `ScreenChannel::_sampleResult` | Actual completed-viewport RGBA8 readback and that request's selector/frame/dimensions | Replaced by each explicit new token, including an empty refused observation on failure; unchanged token retains an explicitly historical snapshot | Native region probe: fresh colours, frame retention, no partial budget result |
| Sensor observation read/copy | `_sampleResult` | A detached typed snapshot is returned on every read; canonical storage cannot escape through Map/ValueLeaf aliases | Native region probe: editing Person-carried memory cannot mutate the channel witness |

Field edits do not reinterpret past observations; recapture needs a new token. Carried observation memory remains ordinarily editable on its bearer, with the read-only channel snapshot as the sensing witness. The existing Object texture `_regionCache` is not repaired or certified by this work. See [contract and remaining work](../../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md#named-regions-and-completed-viewport-observations--2026-10-07).

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-07 19:08 PDT.*

### On `ReteNetwork` (`src/ZonesOfEarth/AuthorsOfLaw/Law.hpp`)

| Structure | Derived from | Invalidated by | Guarded by |
|---|---|---|---|
| `_facts`, `_factById`, `_stateFactsBySubjectAttr`, `_stateFactsBySubjectPtrAttr` | asserted facts | `assertFact`, `retractFact`, `retractFirst`, `retractStateFactsBySubject`, `retractFactsAbout`, `clear` | `rete_compile_test`, `rete_relation_state_test`, `rete_state_ptr_index_test` |
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

**Prototype creation footprint (2026-10-02):** Create with a prototype path
reads through a whole constructor/storage adapter, so Prophetic analysis
marks both reads and writes opaque. Changes to that action are already
covered by Law text revision invalidation; no codec read-footprint cache is
introduced. Narrowing requires an exact declared footprint before it may be
added. Guarded by `universal_singular_creation_test` (both opacity flags).
*Codex / GPT-6.1 Sol / session `01a0e64f-5853-7d30-8196-995b4fd16b89` /
2026-10-02 17:25 PDT; Zach's universal creation request.*

| Structure | Derived from | Invalidated by | Guarded by |
|---|---|---|---|
| `_seededSubjects` | which beings have had their properties snapshotted | erased when a being is released or unmade | `rete_relation_state_test`, `vocabulary_index_test` §B |
| `_relationTypesInPlay` | relation kinds named by any law's conditions | grow-only; a newly-named kind triggers `backSeedRelationStateFacts` | `rete_relation_state_test` §D |
| `_reteTerminals`, `_compiledConditionRevision` | each law's condition tree | `law.conditionRevision()` moving | `rete_compile_test` (§C now asserts which path it exercises) |
| `_vocabularyIndex`, `_indexedNames`, `_vocabularyBuiltAt`, `_vocabularyNamesRevision` | who carries which property name; which names laws require | `Universe::structuralRevision()` and `Law::textRevision()` | `vocabulary_index_test` (8 sections; §H compares against `couldApplyTo` itself) — **confirmed red** with the dotted-name rule removed |
| `_prophetic`, `_propheticRevision` | the whole law register's text | `Law::textRevision()` moving; fails open when incomplete or when a foreign alpha exists | `prophetic_rete_test`, `related_prophetic_legibility_test` |
| `Prophetic::Index::_relevanceEdges`, `_unknownWriteSources` (including `knownMayWritePaths` / `domainComplete`), `_relevanceComplete` | branch-local read demands + modeled write effects, plus explicit opaque-writer frontier and its path-domain knowledge, derived from the whole Law register | rebuilt with `_prophetic` on `Law::textRevision()`; opacity sets **incomplete** and removes narrowing authority but does not erase known edges; current opaque sources default to domain-incomplete/wildcard. **When capability/property Relations begin populating complete domains, their Relation/registry revision must be added here before those facts gain authority.** | `prophetic_rete_test` §H — JSON-stable branch provenance, `Any` arms, disjoint pair omission, known-edge retention under opacity, modeled First-Mover + unknown-source coexistence, incomplete-domain wildcard, complete-domain disjointness |
| `_relationStateToRevalidate` | endpoints of dissolved or retyped relations | queued by the event, drained at the top of the next `tick` | `reactive_departure_test` — **confirmed red** without the drain |
| `_adapterRouteRevision`, `SlowAdapter` roads | each law's `Related(kind, category)` conjuncts; the relation graph | `law.conditionRevision()`; `structuralRevision()` **and** `Universe::relationGeneration()` | `slow_adapter_test` (10 cases), `slow_adapter_parity_test` — four mutations **confirmed red** |
| `_candidateRoutes` | per-Law choice of highest sound/current candidate source (sweep, vocabulary seed, retained road, or derived **Law-Direct** bearers + residual condition) | `Law::textRevision()`, `law.conditionRevision()`, `Universe::structuralRevision()`, `Universe::relationGeneration()`, per-Law adapter-road currency stamp; toggling adapter/direct clears the cache | `slow_adapter_parity_test` + `law_direct_stress_test` — parity, stale fallback, direct promotion, relation-query elimination, dramatic A/B timing |
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


## 2026-09-19 — Law-Direct derived execution and A/B witness

A current single positive conjunctive Slow Adapter road may crystallize into concrete bearer pointers plus
a residual predicate in which only that exact proved `Related(kind, other)` conjunct is discharged.
Dynamic values remain live. `Any`, `Not`, quantifiers, multiple-road ambiguity, opaque condition
closures, or stale currency fall downward.

`LawManager::setUseLawDirect(false)` is an explicit derived-execution A/B switch. With Slow Adapter
still enabled it reproduces the immediately-pre-Direct terminal ladder in the same executable. It is
used by `law_direct_stress_test` and by the real authored-world perf probe; it is not authored world
state.

The stress witness is intentionally Chess-shaped and adversarial: 32 category members, 128 Laws,
128 irrelevant category edges per member, and equal-width vocabulary/category candidate sets.
Pre-Direct repeatedly re-proves membership; Direct proves once and evaluates the residual. The test
requires semantic parity, >=98% graph-query/fan-out elimination, >=2x wall-time speedup, and rapid
amortization of the one-time promotion cost.


## Direct Screen pipeline cache (2026-10-04)

`WebGpuRenderer::_screenPipes` stores Kernel driver objects keyed by the complete emitted
WGSL for a direct OntoMath Screen expression. Operator/coordinate/piece/selector changes
select a different key; numeric coefficients and bounds remain storage-buffer values.
`drawScreenForm` traverses current expression contents and uploads current parameters on
every submission, so pointer identity is never an invalidation proof. Failed bindings,
unsupported expressions, and source removal never draw a prior cached expression.
Resources are released on renderer shutdown. CPU traversal and parameter uploads
remain optimization work; future memoization must preserve this content-sensitive
contract. The production-object witness is
`scratch/probes/direct_screen_form_probe.py` (independent CPU evaluation plus native
readback, structural/numeric edits and refusal/withdrawal).

Codex / GPT-6.1 Sol / session `01a10a2b-a247-7c11-9d5f-7a8b89df6cfc` /
2026-10-04 21:10 PDT; Zach requested the direct medium under Refusals 1, 3 and 7.

## FieldNode authored-math revision and the volume zero-density proof cache (2026-10-09)

**`geom::FieldNode` authored-math revision** replaces per-frame JSON hashing of every authored
channel (`field.ast`, `volume.*.ast`, `volume.occluder.sdf`, `light.*.ast`) as Screen's change
signal. Signal: one process-wide sequence that never repeats, bumped by the property bridges
(not on an identical rewrite), by `applyJson`, and by the MCP `author_volume` in-place path.
Silent-failure guard: `verifiedAuthoredMathRevision()` re-hashes one channel per call
round-robin and bumps, loudly, on an unrevisioned change. A missed writer heals within ten
reads instead of staying stale. Witness: `tests/singularity/field_node_authored_revision_test.cpp`,
case 4, asserts the silent case: an in-place write with no bump is detected and the revision moves.

**`WebGpuRenderer::_volumeZeroProofs`** caches `Rendering::VolumeZeroProof` per density
expression. Key: expression pointer; invalidated when `densityRevision` or the box half-extent
changes (the grid tiles the box). A binding with revision 0 never gets a proof, because a cache
that cannot be invalidated would be a stale theorem. Entries unused in a frame are dropped, and
the switch `setVolumeZeroProofEnabled(false)` restores the exact path. The proof only ever
*removes* density evaluations where OntoMath interval arithmetic shows D <= 0; it never adds
light.

Witnesses:
- `tests/singularity/volume_zero_proof_test.cpp`: the generic contract, plus a soundness sweep
  of 16,000 points in proven cells of the real Northern Veil media.
- `tests/singularity/webgpu_volume_zero_proof_test.cpp`: a byte-identical framebuffer with the
  proof on and off, through both the fused-set and single-medium pipelines, at two times.

**Volume sharing plan (2026-10-09)** is derived state inside the compiled volume program: which channel positions share one per-sample value. Constants are uploaded separately and never recompile, so a numeric edit could make two "shared" subtrees differ while the program kept sharing them. The guard is that sharing positions join the program's structure key (`sdfwgsl::inspectVolumeSharing`, in both the single-medium and set memos). Divergence changes the key, so the program recompiles. Witness: `volume_shared_subexpression_test` (an edit outside the shared subtree keeps sharing; an edit inside it ends it).

What it does not guard is f32 vs real arithmetic inside the shader. The 1% cell enlargement
covers sample-position rounding, and the byte-identical witness is the arbiter for the rest.

Claude Code · Claude Opus 5.5 · session `session_01NJy6VrPVNcHAnggwFyTsmF` · 2026-10-09;
Zach asked why Screen serialized JSON every frame, and for the proof to be fully general.


### Volume sharing evaluation domains — Sixth-one Sun continuation

The per-sample private slots belong to one medium, its current local point, and its temporal coordinate. `emitMathNode` and `flattenScalarProduct` substitute them only at the original `p`; `planVolumeSharing` does not treat the rebound field inside SDF/Gradient as an occurrence at that point. Source rho/chi/alpha and occluder lowering clear the substitution table: the source owns its clock/coordinates, and the occluder is evaluated along shadow-ray points. The constant collector now follows the compiler's medium → source → occluder order. Equal-size sharing groups are ordered by authored positions rather than numeric JSON keys, so edits that preserve equality also preserve slot order. These guards do not add a persistent cache; slots are filled anew per spatial sample and revisions still govern program/value refresh.

Witnesses: `volume_shared_subexpression_test` now covers source isolation, SDF rebinding, six Gradient points, source/occluder constant order, all-noise locals, and stable slot order. `webgpu_volume_shared_subexpression_test` compares actual RGBA against independently inlined equivalent mathematics at two distinct source clocks, through single and fused pipelines; it uses synthetic memory only. Run it with native GPU access and distinguish device skips from assertion passes. Task: [Northern Veil](../../Agenda/Tasks/Specific%20Tasks/Performance%20and%20Runtime/Northern_Veil_volumetric_frame_cost/Northern_Veil_volumetric_frame_cost.md).

Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T11:52:24.788492-07:00; continues Zach's mathematical unification and Opus 5.5's implementation.
