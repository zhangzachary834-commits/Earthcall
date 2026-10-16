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

**Per-channel since 2026-10-12.** Each authored channel (`field.ast`, the seven `volume.*` channels, `light.chroma/angular`) now carries its own never-repeating revision. Bridges note only their channel, `applyJson` notes only channels whose content changed, and the verifier heals only the channel it caught. `noteAuthoredMathWritten()` still moves every channel, for writers that cannot name one. Invalidation is now a per-channel dependency frontier (Formation Rete Incremental Maintenance §2): an emission edit no longer rebuilds the density proof or its support envelope. Witnesses: `field_node_authored_revision_test` case 8 and the frontier case in `webgpu_volume_zero_proof_test` (emission edit: 0 proof builds; density edit: exactly 1, byte-identical to exact).

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

### Proof-derived volume support — 2026-10-10

Zach approved measuring the committed renderer and refining empty-space work. The retained implementation derives `VolumeZeroProof::supportMin/supportMax` from every **unproven** leaf of the existing grid, once when that proof builds; the cell margin also enlarges the envelope. All-proven density has explicitly reversed bounds. This is Kernel acceleration state, not new authored bounds or medium meaning.

Dependencies, lifetime and stale fallback are exactly the existing proof memo's density revision and half-extent. Origin, camera, opaque depth and source/medium clocks are runtime inputs; time is never assumed constant by the CPU proof. An unknown/unrevisioned/disabled proof retains the authored chord. In the outside specialization, six local bounds follow the packed bit words in the GPU parameter batch; `volumeZeroProofCell` continues reading the same bit words. Near-parallel rays retain the original interval rather than using the ray helper's substituted direction to certify a smaller one. Translated bounds receive an additional world-arithmetic margin.

Outside every admitted medium box, both single and fused marchers may reject empty rays and skip null prefixes/tails. The fused marcher still derives events, segment boundaries, spacing and sample positions from the **authored** boxes. Never replace those events with support intersections: that would silently change quadrature. The conservative starting index retains an extra sample, and the remaining grid walk is unchanged.

Shader specialization has two Kernel cache lanes, keyed by the existing ten expression identities plus `trimProofSupport`; fused membership/order remain part of its key. `flushVolumeComposite` alone selects the lane from the existing proof switch and whether the camera lies in any admitted authored box. Inside any box, or with proofs disabled, the compiler omits the support helper/loop work and the uploader omits the six bounds, preserving the original bit payload and offsets. This avoids the measured inside-view regression of the always-on prototype. No authored equation changes when the camera crosses a box boundary. Each lane compiles on first use, then existing content/structure revision checks govern refresh or recompilation independently in both lanes. Neither lane interprets equations on unchanged frames. The first encounter with the other lane can still incur a one-time pipeline compile; prewarming and incremental structural repair remain open.

Witnesses: CPU `volume_zero_proof_test` covers enclosing the slab's full positive support, retaining unconstrained axes, all-empty support and unknown/positive fallback. Native `webgpu_volume_zero_proof_test` compares actual RGBA vectors through single/fused pipelines, inside/reverse/side/axis views, and an in-place zero→positive density revision (proof-on before any proof-off eviction). Six focused targets passed with native Metal access; CPU proof 24/24 and native proof 38/38, including distinct first-use kernels and zero recompiles when warmed views return. Performance evidence belongs to the [Northern Veil task](../../Agenda/Tasks/Specific%20Tasks/Performance%20and%20Runtime/Northern_Veil_volumetric_frame_cost/Northern_Veil_volumetric_frame_cost.md), independently of those fidelity checks.

Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T00:08:05.843830+00:00.

Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T11:52:24.788492-07:00; continues Zach's mathematical unification and Opus 5.5's implementation.


### Kernel volume evaluator and resident payload caches — October 10 continuation

Continues Zach's compile-once/incremental-repair direction and [the minimum-work task](../../Agenda/Tasks/Specific%20Tasks/Performance%20and%20Runtime/Northern_Veil_volumetric_frame_cost/Northern_Veil_volumetric_frame_cost.md#minimum-work-continuation-reusable-compilation-and-resident-data). This supersedes the first-use-only lane preparation described above: both camera lanes now prewarm during admission from retained evaluator fragments.

- **Compiler DAG / `_kernelVolumeEvaluators`:** Kernel-derived state, per existing medium expression/source/occluder identities, normalized independently of camera lane. Positional occurrences carry typed value identity, structural identity, parameter slots, bindings and reverse edges. Full typed equality guards reuse; node text is reused only when parameter/temporary offsets and coordinate/clock/sharing contexts also agree. Authored revision gates prevent unchanged-frame lowering. A changed medium refreshes its own evaluator; sharing splits/joins regenerate its fragments, leaving other media cached. Numeric layout/sharing stability is a required precondition for parameter-only repair; the renderer verifies it. That path still traverses the changed medium through the existing parameter collector; it emits no changed final module. Unsupported/unknown revisions force fresh checks, not a permanent stale cache. Reload/device teardown clears evaluator entries and both module lanes. No authored IDs or serialized state are introduced.
- **`_kernelVolumeSourceLayouts`:** source expression identities plus their declared revisions govern retained scalar/vector/angular layout metadata. Unknown revision inputs rescan. Camera/time advancement is not an authored layout change. Reload/shutdown clears entries with program state.
- **`_kernelVolumePayloads`:** device-bound storage buffers owned by existing volume pipelines, explicitly named Kernel state. Payload identity includes the authored memo/layout, declared revisions, proof mode, camera specialization and half-extents; membership/order govern the combined offsets. Unknown density includes a frame discriminator and retains existing unproven fallback. Existing proof cache still validates density revision/half-extent each frame; stable frames reuse packed proof metadata. Dirty f32 ranges compare bytes and coalesce adjacent writes; parameter and proof counters partition the actual uploaded bytes. The older frozen baseline parameter counter includes the proof subset, so summing both baseline counters would double-count. Clocks/transforms/camera are streamed separately. Repeated use of one payload within a frame falls back to the existing frame stream to avoid earlier queued draws observing later writes. Buffer allocation failure uses the same stream. GPU buffers are released before pipeline caches at reload/shutdown, including device recreation. No save meaning is cached here.

Witnesses: CPU sharing 55/55, parameter refresh, native sharing numeric/structural/domain edits, unrelated-member reuse, reorder/removal/reload/device replacement and time-only zero uploads; native proof 42/42, including zero→positive invalidation and warmed single/fused camera crossings. Eight full-resolution captures at two times/four views match the frozen renderer exactly. Fidelity witnesses and performance retention are separate; matched economics are recorded in the task. Demand-first nested sharing was rejected for a one-level native pixel difference. Timestamp-enabled visible measurements were rejected by Person feedback; ordinary window and pixel witnesses use timestamp-free devices.

Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T22:32:41.495077-07:00

The preparation/residency stage is retained after two readback-free fixed-view ABBA series: recurring parameter/proof bytes are zero, first moving-series preparation falls ~19%, and no view repeats a >3% total-frame regression. This establishes CPU/upload economics, not a GPU speedup. The existing native object consumer also passes with both-lane cold preparation and unchanged zero warm/numeric compiles. Full records remain in the linked task. Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T22:58:25.385007-07:00

Completed raw-status movement evidence qualifies the earlier preparation percentage: first ABBA ~18% lower preparation, second ~10% worse with a much faster baseline/wait-pattern shift. Both versions show that variability. Exact residency removes all recurring parameter/proof bytes, and fixed-view economics pass, but no uniform movement or GPU gain is established. Candidate route p95 197–298 ms does not meet 30 FPS. No surface reset fix was retained because the public frame wrapper already clears stats. Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T23:24:19.962220-07:00


### Kernel volume output-budget mirror — cutoff reference decision

Zach selected “remove the cutoff” for the bounded reference. The authorable source is registered `ScreenChannel.volumeOutputErrorBudget`; `EngineRender` mirrors it to `Renderer` each frame. `_volumeOutputErrorBudget` is explicitly Kernel boundary state, default zero, finite-positive capped at 1/255. It is streamed through `VolumeGlobalUniforms::volumeOutputError`, independently of resident equation/proof data. GPU x is rounded down if conversion exceeds the authored budget; y preserves opt-in even when a tiny positive double rounds to zero. The WGSL single marcher and both fused-loop exits read y before permitting the legacy opacity heuristic. Camera variants consume the same contract.

No new tail-proof cache exists yet. Until conservative source/attenuation/background/numerical bounds are implemented, all positive-budget rays continue the accepted lattice; exact zero-density proofs retain their declared fallback and independent switch. Budget changes neither lower expressions nor upload resident parameters/proofs. Global uniforms are refreshed each draw; device recreation reconstructs them and existing module caches. Native single/fused analytic/6144 emission and background witnesses pass; eight default captures remain byte-identical. Do not infer a one-level bound against the legacy cutoff image: it can omit bright late emission, which full-lattice output intentionally restores. No certified early tail exit or performance gain is claimed.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-10T23:49:10-07:00


### Kernel volume time-only preparation — October 11

From Zach's request for the highest likely value independent remaining optimization. `Program.kernelVolumeTimeSlots` holds derived scalar-output-slot recipes; `kernelVolumePreparationWgsl` composes their compute module directly. Eligibility requires a scalar leaf with transcendental work and only its own time variable. The original emission arithmetic/parent grouping remains unchanged. Compiler DAG fragments retain their recipe slices with parameter/temporary/domain identity, so partial reuse cannot lose preparation instructions. Numeric-only repair retains recipes and recollects inputs through the existing collector; direct parameter-binding recipes are still open.

- **Dependencies/writers:** authored scalar terms and variable/clock bindings determine recipes; revisions/layout/sharing changes repair or regenerate them. Medium/occluder expressions read their instance time; source expressions read sourceTime. Camera/time changes never lower mathematics. Unsupported or spatial/directional leaves stay in their existing evaluator.
- **Resident derived slots:** outputs are reserved inside the compiled parameter layout before proof payloads. CPU parameter mirrors deliberately hold initialization values, while GPU compute writes only these derived slots every rendered frame. Changed numeric inputs/layouts flow through existing dirty-range uploads before computation. Frames order preparation before reading in volume passes; no CPU readback/wait is introduced. Instance membership/offset rebuilding and the existing same-frame fallback remain authoritative.
- **`_kernelVolumeTimePipes`:** explicitly Kernel/device-bound compute pipelines and layouts, keyed by complete preparation WGSL and shared across camera lanes. Volume pipeline identity includes its preparation module. Rejected preparation compilation refuses the dependent fragment path, rather than rendering uninitialized zeros. Reload/shutdown/device recreation clear this cache and resident buffers. Generated module bytes and compilation counts include preparation; warm frames compile nothing.

Witnesses: eight complete native captures match; native sharing 65/65 covers independent inline math, distinct clocks, numeric refresh, reload/device recreation and warm zero CPU uploads. Proof/quadrature/object consumers pass. Two ABBA series per view reduce pooled inside offscreen frame cost 5.58% (individual 4.14/7.01%), with both series improving all other views. This supports retention, not a live 30 FPS or temporal-coherence claim. Per-frame preparation remains even when a clock is unchanged; there is no cached-time identity yet.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T00:15:29-07:00


### Kernel per-ray volume support traversal — October 11

Extends and supersedes the inside-payload statement in the dated proof-support entry above. Every present zero proof now carries its six local support bounds after the packed words in both camera lanes; `attachVolumeZeroProof` writes them and dirty-range accounting classifies them as proof bytes. Authored parameter slots stay fixed. Existing revision/half-extent/membership identities rebuild absolute offsets, and existing residency/device teardown governs lifetime. Unknown/unrevisioned bindings carry no proof. Time alone does not rebuild support.

Fused WGSL computes one padded support interval per medium/ray, retains it through original authored segments, and skips that medium's grid walk before entry/after exit. Unknown or near-parallel support exclusion receives an unbounded interval; authored events still delimit the lattice. If all held media prove zero, the next index targets the earliest possible contributing interval/cell exit, always on the original sample lattice. One per-sample `sampleUnproven` flag per medium reuses the existing cell proof for density demand, within that medium/local-coordinate domain only. Ray/sample values are GPU Kernel derived state, not a persistent authored/cache identity.

Global outside prefix/tail trimming remains separately selected; inside single-medium marching is unchanged. No arithmetic reassociation, contribution-tail bound, new fidelity switch or authority mechanism. Native proof 54/54 and all six focused targets pass; eight complete captures match retained bytes. Final repeated offscreen economics pass the 5%/3% gates in all views. [Final evidence and Person limitations](../../audits/NORTHERN_VEIL_SUPPORT_TRAVERSAL_2026-10-11.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T02:25:43-07:00


### Perlin reuse experiment boundary — October 11

The ray-local cell/XY caches and once-per-device intrinsic GPU tables tested after support traversal are **not retained derived state**. Three ray caches preserve tested pixels but regress cost; three table variants fail exact rendered pixels despite compute bit parity. Their compiler helpers, device resources and dependent tests were removed. Existing compiler/proof/resident/time/support lifetimes remain unchanged. Seven restored focused targets pass in Perlin ABC mode and eight restored images match. [Measured rejection, fallback domains and baseline terrain-test caveat](../../audits/NORTHERN_VEIL_PERLIN_REUSE_EXPERIMENTS_2026-10-11.md). Future agents must not document the scratch noise caches as production ownership or infer final-output bounds from finite compute/image tests.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T10:29:00-07:00


### Kernel selective column support — October 11

Extends the retained per-ray proof traversal. `VolumeZeroProof::supportAxis/supportBands` are Kernel CPU index state in the existing density-pointer/revision/half-extent memo. Every unknown padded leaf belongs to its band; the chosen axis minimizes enclosed cell volume. No new interval evaluations or authored fields. Unknown/unrevisioned inputs keep the established fallback, and time alone rebuilds nothing.

Both resident camera lanes retain six global bounds; only outside payloads append four f32 bounds per slab. `zeroProofPresent` bit 0 means present, bits 1..2 encode axis+1 (zero = original payload). Dirty upload accounting covers the extra range; camera lane/membership/revisions/extents already key payload identity. Existing cache eviction, reload and device teardown own lifetime and rebuild offsets. No separate permission or resource office.

Outside WGSL computes geometric eligibility once per medium/ray, then the reached leaf MUST prove zero before a padded column intersection may extend its jump. Jumps stop before that column's exit and retain original box events/lattice. Inside emits the original helper signature/body. There is **no ray-local interval cache**: both cache experiments are rejected for exactness/cost. Near-parallel/absent proof uses the established cell path.

Six focused targets pass, including CPU 32/32/native proof 65/65 with exhaustive unknown-cell enclosure, changed extents, replacement device and full-size spawn checks. Eight 2560×1440 images match. All 960 timed final frames report zero warm lowering/compiles/parameter/proof uploads. Spawn −9.77%; inside all-run mean +2.29% with no repeatable >3% regression after predeclared follow-up, no inside gain claim. [All runs, rejected variants and Person limitations](../../audits/NORTHERN_VEIL_FOLDED_SUPPORT_2026-10-11.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T13:27:06-07:00


### Kernel sample-local fused contribution schedule — October 11

`composeVolumeSet` now consumes each medium’s proof/density/contribution in order, eliminating long-lived per-medium proof flags and the second membership sweep. Source/receiver directions use the original expressions only after positive density, with sample/medium lifetime; no ray cache, authored state, buffer or proof layout is added. Existing member fragments, composed program caches, revision/layout/membership gates and reload/device teardown own lifetime. Unknown proof cells veto jumps; original lattice/order/source-domain/cutoff contracts remain unchanged. Warm uploads/lowering/compiles stay zero. Six focused witnesses and eight byte-identical full-size images pass; repeated offscreen inside −9.77%, other views −6.44% to −11.53%. Backend-aware new support certificates remain deferred. [Evidence and exact boundaries](../../audits/NORTHERN_VEIL_SAMPLE_LOCAL_SCHEDULING_2026-10-11.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T17:22:37-07:00 (PDT).


### Scratch fragment demand observations — October 11

The read-only diagnostic adds no production derived state. Its process-local GPU record buffer resets before each fixture draw; private counters reset per fragment, selected records are read after completion, and GPU buffers release after dump. Inputs are the current fixture, compiled module/driver, native camera, physical dimensions and clocks; each rerun replaces the observation in a new scratch output directory. Repeated counter/accounting checks pass, but paired pixels differ by one level, so counts remain explicitly instrumented-program observations. The empty-transport production candidate is rejected for cost; existing schedule/cache invalidation stays unchanged. [Tool, evidence and limitations](../../audits/NORTHERN_VEIL_FRAGMENT_DEMAND_2026-10-11.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-11T20:53:25-07:00 (PDT).


### Research-only scalar-support tables — October 12

No production cache or GPU resource is added. Process-local rational intervals and memoized sum trees live within the CPU research run; the report goes to a fresh scratch file. Proposed certificate dependencies are the recognized AST, finite converted parameter bits, coordinate/clock/source binding, local extents, bin edges and the verified execution profile. The prototype tests mismatched keys/refusal; it does not install production invalidation. Current production profile remains Unknown, hence admitted skips are zero. All 18 research witness groups pass. OntoMath intervals and retained device/cache lifetime rules are unchanged. [Conditional contract and evidence limits](../../audits/VOLUME_SUPPORT_NUMERICAL_CONTRACT_2026-10-12.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-12T00:43:07-07:00 (PDT).


### Scratch native compilation observations — October 12

The compiler observer adds no production derived state. Its concrete-device method replacement, original implementation pointer, mutex, record counter and retained output path live only in the linked scratch process and end at process teardown. It forwards source/options unchanged and records each compilation in a new external output directory. Observations depend on the exact vendored library, translated module, native device/driver and compile options; provenance preserves binaries/source hashes. No observed profile grants proof reuse: the tool still declines every production certificate until complete transformation/coordinate/binding enclosure is established. Twenty-four CPU witnesses pass; eight paired low-resolution native images match. Existing program/proof residency and device invalidation remain unchanged. [Contract](../../audits/VOLUME_SUPPORT_NATIVE_PROFILE_2026-10-12.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-12T11:27:35-07:00 (PDT).


### Scratch arithmetic-profile experiments — October 12

No production derived state is added. A separately generated scratch wrapper copies options only for volume-containing modules, forwards unchanged MSL once, releases the copied options after library creation, and ends all hooks/record state at process teardown. Fresh observations depend on exact fixture, binaries, native device, module hashes, effective options, framebuffer, clock and reference budget; reports are not reusable support certificates. Non-volume options and source hashes are checked. Both Safe arithmetic candidates fail pixel gates, including a full-lattice native-size counterexample; no production guard/cache/profile is installed. Original read-only observer and existing lifetime/invalidation contracts are unchanged. [Failure evidence, dependency hashes and reproduction](../../audits/VOLUME_ARITHMETIC_PROFILE_FIDELITY_2026-10-12.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-12T17:13:57.815427-07:00 (PDT).


### Standalone offline performance reports — October 14

No app/being state, runtime cache or GPU resource is added. Analyzer dictionaries, trace stacks, source-frame indices and structural call graphs live only in the separate process. Every invocation reads explicit input files, fingerprints them before/after analysis and writes a new output directory; existing output is refused. Reports are immutable observations of those inputs, not reusable rendering proofs or automatically refreshed views of the live app. Reanalysis requires a new report, and a source change requires rebuilding any packaged executable. No per-frame instrumentation or device lifetime is introduced. Unknown metadata and unsupported timing retain unknown status; malformed/ambiguous data refuses attribution rather than becoming zero cost. [Contracts](../../Agenda/Tasks/Specific%20Tasks/Performance%20and%20Runtime/Standalone_Performance_Analysis/Standalone_Performance_Analysis.md) and [executed witnesses](../../audits/STANDALONE_PERFORMANCE_ANALYSIS_2026-10-14.md).

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-14T22:37:18.690115-07:00 (PDT).


### Planned group-demand and qualified-support work — October 15

The [implementation plan](../../plans/VOLUME_DEMAND_AND_SUPPORT_IMPLEMENTATION_2026-10-15.md#9-derived-state-and-compatibility-requirements) describes proposed retained group dependencies/placement, product-local aliases, qualified proof payloads and direct parameter recipes, with owners, reuse predicates and invalidation. **No new cache, native proof eligibility or GPU resource is installed by this documentation pass.** Existing residency, camera lanes and exact fallbacks remain the current contract. Adding finite shared `evalRange` results can immediately strengthen existing proof consumers, so future mathematical coverage must inventory consumers and preserve Unknown native eligibility until the selected AD-30/36 obligations hold. Earlier ADs may be planned through for Zach's later verdicts; new decisions are not pre-approved.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T03:21:44.030398-07:00 (PDT).

### Power correction and rejected derived-state trials — October 15

[Executed record](../../audits/VOLUME_POWER_AND_OPTIMIZATION_TRIALS_2026-10-15.md): no new production cache, schedule metadata, proof format or GPU resource remains from these trials. Demand-group placement, local aliases and macro occupancy storage were implemented with their candidate dependency/lifetime contracts, failed retention gates, and were removed. Existing program/proof caches, parameter slots, residency, membership offsets, revision gates and device invalidation retain their prior contracts.

The retained shared power helper is immutable Kernel shader-library text, embedded at module generation, with no new authored/serialized state or persistent numerical cache. Changed generated WGSL still requires affected native modules to compile; unchanged media use existing revision gates. The external experiment process owns temporary comparison/readback buffers, releases native resources at process exit and never supplies a proof to the app. Its reports are frozen observations, not refreshable runtime state. Native subnormal consistency is not a complete numerical certificate; unresolved execution/consumer contracts remain Unknown.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T05:56:25.134858-07:00 (PDT).


### Explicit Euclidean host range context — October 15

[Executed contract](../../audits/VOLUME_EUCLIDEAN_RANGE_2026-10-15.md): `MathNode::evalMathematicalRange` selects extra host Length/Distance rules through a private invocation-local context shared by recursive calls. No memo, persistent cache, authored/serialized field, GPU allocation or device-dependent object is introduced. Inputs are the current authored node tree and explicit variable intervals; every invocation reads them afresh, so there is no cross-revision reuse or stale-data path. Bounds/temporary maps expire with the call. Unsupported/type/nonfinite/overflow cases return Unknown.

The old `evalRange` entrypoint explicitly selects its previous rules. VolumeZeroProof, Sdf and PropheticRete continue calling it; they receive no new Euclidean theorem. Future production promotion must explicitly qualify the native consumer/scope rather than treating this host result as a complete backend certificate. Census reports and fingerprints are immutable external observations, not live proof caches. Existing GPU residency/invalidation contracts stay unchanged.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T10:45:48.355899-07:00 (PDT).


### Support-allocation/traversal experiments restored — October 15

[Three rejected increments](../../audits/VOLUME_SUPPORT_ALLOCATION_TRIAL_2026-10-15.md) temporarily added explicit bitmap-domain metadata, two-pass allocation, a resource hint and an AND-parent bank with distinct storage/instance eligibility. Their dependency was density revision + half extent; device/camera lanes and membership offsets were tested. None remains in production: the producer, proof header, renderer and shader hashes match the starting retained source, so previous cache/format/lifetime contracts remain authoritative. No new runtime memo, GPU resource, flag or omission permission is retained.

Source patches are explicitly experimental/rejected evidence. New native tests exercise actual translated/parallel/support-edge rendering; they are no certificate reusable at runtime. The external cost process records immutable source/binary/input hashes and explicit metadata sizes. Its optional hierarchy-word inspection is compile-time developer tooling, not authored state or a second authority office. Unsupported native numerical/scope contracts remain unqualified; the host Euclidean entrypoint is still not consumed by GPU proofs.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T12:14:29.656582-07:00 (PDT).


## Laboratory omission authority removed — October 16

[Correction and witnesses](../../audits/FIRMAMENT_LABORATORY_CORRECTION_2026-10-16.md) remove the laboratory `adaptiveWords`, density variation classifier and `VolumeCertifiedBounds` producer/consumer. No stride/tail certificate cache survives. Positive-budget frames no longer sweep channel intervals or publish a finite-host omission flag; reserved `volumeOutputError.z/w` are zero and unread by WGSL. `y` retains the separate opt-in flag so tiny positive double budgets cannot re-enable the compatibility cutoff when `x` rounds to zero.

Existing density-pointer/revision/half-extent zero proofs, support bounds/bands, resident offsets/dirty writes and device invalidation remain under their prior contracts. Scalar-map reuse is invocation-local; midpoint evaluation can only refuse a proof. The cheap zero-factor noise lane now refuses malformed arity/types and null Scale operands. Experimental `evalFastRange` has no production caller and grants no native permission. No new authored field, GPU payload layout or cache lifetime is introduced. Re-enabling coarsening/termination requires the unresolved complete transport/numerical contract; AD-30/36 are unchanged.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-16T14:22:15.045552-07:00 (PDT).
