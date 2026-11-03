# Singular serialization topology

**Status:** Phase 7 landed as transitional compatibility infrastructure, but the target is REOPENED: ordinary persistence must be Zone-centered and must not require a conglomerate session/world file. Legacy `playerBody` remains a read-only compatibility bridge.
**Agenda section:** Singular · Relation · Formation  
**Author:** Codex, session `01a0707e-f743-71b1-8fb9-63975012e66d`, 2026-09-05 01:00 PDT

## The human thread

Zach identified the lived failure precisely in the Agent Intercom broadcast: a Person
must load conglomerate “worlds” separately from their Zones; the save system is a
top-down filesystem bureaucracy while the ontology is fluid Formations. GPT-4o had
already named the symptom: Zones and the Hierarchy of Joys are conceptually coherent,
but persistence is stagnant. This proposal takes those observations as the
requirements, rather than treating folder movement as a cosmetic cleanup.

The extension originated here is a concrete code boundary: serialization is arranged
by *persistence root and hydration phase*. That respects C++ inheritance while refusing
to imply that a Relation is owned by the directory containing either endpoint.

## Authorial correction — Zone, not session, is the lived persistence boundary

**Recorded by:** Codex, session `01a0707e-f743-71b1-8fb9-63975012e66d`,
2026-09-09 14:04 PDT

Zach clarified the intended experience after observing the Go Zone boot with shapes but
without its face textures or functioning Laws: he never wanted a Person to save or load a
named conglomerate “world” under Assets. A Person should open the Creator Console's Zone
window, choose **Move to Zone**, arrive in a complete functioning Zone, and save that Zone
there. Some Zones currently fail even to appear until their old world file is loaded; that
is a direct violation, not an optional UX improvement.

The Phase 7 session envelope therefore remains useful only as a compatibility, recovery,
observation, or import/export container. It is not the normal persistence authority and
must not become the transaction boundary for the next load architecture. This corrects
my own implementation drift: the human thread above named the conglomerate bureaucracy
correctly, but later phases still strengthened its orchestration instead of retiring it
from the Person's ordinary path.

The target contract is now:

1. Boot enumerates every valid Zone/Home identity into the Zone catalog without requiring
   any world/session file to be selected.
2. **Move to Zone** transactionally gathers that Zone's semantic dependency closure:
   Objects, Materials (including face textures), authored Laws and triggers, Categories,
   Formation/Relation records, and its verified physical-matter generation.
3. A Law or Material remains its own Singular persistence root. Zone completeness is
   expressed by stable references/Formation membership, not by pretending the Zone owns
   the being or by copying it into a private bag.
4. The Zone becomes active only after the whole closure validates. A missing dependency
   leaves the current Zone intact and reports exactly what is absent.
5. **Save Zone** commits only the active Zone's changed closure; it neither asks for a
   world name nor serializes every live Zone. A convenience **Save All Dirty Zones** may
   commit several independent Zone transactions, never one conglomerate identity.
6. Zone-scoped Laws become eligible with the Zone's activation and cannot act during
   partial hydration. Ourverse metalaws remain governed by their own root and standing.
7. Assets ceases to be the normal Save/Load office. It may remain an asset/material/import
   browser; legacy world-file loading moves behind an explicitly named compatibility or
   import surface until every authored file has a preservation-tested migration path.

No existing save is to be deleted, merged, or rewritten merely because this target has
been clarified. Existing world files are sacred migration sources.

## The rule

`Singular` is abstract and has no universal file shape. A serializer belongs to the
first concrete primitive that owns a state vocabulary, and a derived primitive owns
only its additional fields. Cross-being links are always stable identifiers on disk and
are bound only after every referenced root exists.

```
Singular roots         semantic hydration                 link hydration
──────────────         ──────────────────                 ──────────────
Object                 ObjectSerializer                   RelationSerializer
Person                 PersonSerializer                   FormationSerializer
Relation               RelationSerializer                 Zone graph assembly
Formation              FormationSerializer                Ourverse graph assembly
Zone                   ZoneSerializer
  └─ Home              HomeSerializer (Zone extension)
Ourverse               OurverseSerializer
```

This avoids the old false order: a Zone document may name a Relation, but it does not
own the Relation; a Home document may add dwelling facts, but it does not reimplement
Zone facts; a Person's Body is constitutive and stays in the Person branch rather than
being mistaken for a generic Object branch.

## Phase 1 landed — 2026-09-05

The old `Singularity/Storage/Serialization.cpp` and header now live under the
`Serialization/` umbrella, while the original include remains a compatibility facade.
Object, Body, Zone, and Home codecs now have their own ontology-aligned source
boundaries; relation endpoint resolution and formation hydration live in
`Relation/FormationSerialization.cpp`; and `ZonesOfEarth/OurverseSerialization.cpp`
emits an Ourverse root record that names its gathering Zone and Formation records.
This remains a source-topology migration: the legacy save schema and compatibility
entry point are preserved, while Ourverse loading and writer retirement stay open.

## Phase 2 landed — 2026-09-05

**Update author:** Codex, session `01a0707e-f743-71b1-8fb9-63975012e66d`, 2026-09-05 01:55 PDT

`SaveContext` now carries the live Ourverse root. Runtime Save As, Quick Save, and
WebSocket saves emit an `ourverse` record beside the session's Zone references.
`ZoneManager::loadState` hydrates that record after Zones, categories, materials,
and authored laws exist, using stable-identifier resolvers for the gathering Zone,
Joys hierarchy, filaments, and metalaws. Formation identity and relation-type tags
are preserved, and `convenesToward` has an explicit hydration path. Legacy sessions
without an `ourverse` record continue to load unchanged.

The new headless `ourverse_serialization_test` proves root emission through
`ZoneManager::buildSaveJson` and a full semantic root round-trip, including a
cross-Zone filament.

## Phase 3 landed — 2026-09-05

**Update author:** Codex, session `01a0707e-f743-71b1-8fb9-63975012e66d`, 2026-09-05 02:10 PDT

The Person root now has an extracted codec at
`Singularity/Storage/Serialization/Person/PersonSerialization.*`. `Person::serialize`
and `Person::deserialize` delegate to it, while preserving the exact
PersonDatabase profile keys (`displayName`, `soulName`, identity, pose, velocity,
and Body). Session saves additionally emit a semantic `person` root whenever a
Person is present. Load hydrates that root (or the legacy `playerBody` bridge) before
the session's canonical camera pose is reconciled to the Person; old sessions without
`person` therefore remain readable, and new sessions do not apply the same body twice
or break the Person/camera locomotion latch.

`person_serialization_test` proves schema presence, delegation, and pose round-trip;
`ourverse_serialization_test` now also proves Person-root emission through the
session writer. The WebSocket Quick Save context now supplies its live creator color
to the same writer, rather than leaving a required SaveContext field null. No authored
save file was modified.

## Phase 4 landed — 2026-09-05

**Update author:** Codex, session `01a0707e-f743-71b1-8fb9-63975012e66d`, 2026-09-05 02:35 PDT

The duplicate `playerBody` writer is retired. A current session records a Person exactly
once, under the semantic `person` root; the legacy `playerBody` reader remains only for
older authored sessions that lack that root. Person/Body hydration still precedes camera
reconciliation, so the active Body determines the eye-height used to re-establish the
locomotion invariant.

`save_roundtrip_test` now proves both directions: a new Save As contains `person` and no
`playerBody`, while a synthesized legacy session containing only `playerBody` restores its
Body. The First Mover save-format map and Person Verification List name this boundary.

## Phase 5 landed — 2026-09-05

**Update author:** Codex, session `01a0707e-f743-71b1-8fb9-63975012e66d`, 2026-09-05 03:05 PDT

`singular_serialization_topology_test` is the requested cross-root proof. It saves a
Person-owned Home, a second Zone, the Ourverse gathering Zone, and a mutual filament
Relation held by the Ourverse filaments Formation. It then loads into fresh roots through
`ZoneManager::loadState` and proves Home identity/ownership, Person Body hydration,
gathering-Zone identity, Formation identifier/tag, filament endpoints, and
`convenesToward`. The test also verifies the current file writes `person` and not the
retired duplicate `playerBody` key.

## Phase 6 landed — 2026-09-05

**Update author:** Codex, session `01a0707e-f743-71b1-8fb9-63975012e66d`, 2026-09-05 03:25 PDT

`RelationSerialization.*` now owns a Relation's stable endpoint identifiers,
direction, weight, events, and attachment data. `Relation::toJson` and
`Relation::fromJson` remain compatibility delegates, so the graph code has no new API
or schema to learn. A missing resolver deliberately produces an unbound Relation that
retains both identifiers for a later hydration pass; a supplied resolver binds the same
payload to its live Singular endpoints. `relation_serialization_topology_test` proves
both phases and preserves the attachment/event vocabulary.

## Phase 7 landed — 2026-09-05

**Update author:** Codex, session `01a0707e-f743-71b1-8fb9-63975012e66d`, 2026-09-05 04:00 PDT

`SessionSemanticRoots.*` is the Storage-only orchestration boundary: it introduces
versioned `semanticRoots` without inventing a domain class. A current session records
its Person and Ourverse once inside that envelope, alongside Zone/Home records and their
references. The long-standing top-level Zone arrays remain explicit transitional
projections for the identity-store and First-Mover tools; the duplicate Person and
Ourverse payloads are retired. On load, a valid envelope is authoritative and is
materialized into the proven staged loader; a malformed advertised envelope refuses the
load rather than silently falling back to stale top-level projections. The integration
test removes those projections before loading, and the focused envelope test proves both
authoritative precedence and loud malformed-root refusal.

## Target source layout

All of this remains under the existing `Singularity/Storage/` modality. No top-level
subsystem is added.

```
src/Singularity/Storage/
  Serialization.hpp                 // narrow compatibility facade; no definitions
  Serialization/
    SessionSemanticRoots.hpp/.cpp   // session orchestration; not a domain primitive
    Common/
      JsonTypes.hpp                  // JSON aliases + checked scalar/vector helpers
      GraphHydration.hpp             // identifier resolver + phased load contract
    ConstructedBeing/
      ObjectSerialization.hpp/.cpp
      MaterialSerialization.hpp/.cpp
      LexemeSerialization.hpp/.cpp
    Person/
      PersonSerialization.hpp/.cpp
      BodySerialization.hpp/.cpp
      BodyPartSerialization.hpp/.cpp
    Relation/
      RelationSerialization.hpp/.cpp
      FormationSerialization.hpp/.cpp
    ZonesOfEarth/
      ZoneSerialization.hpp/.cpp
      HomeSerialization.hpp/.cpp
      OurverseSerialization.hpp/.cpp
```

This is an organization of existing ontology, not a new taxonomy. `Law` remains with
`ZonesOfEarth/AuthorsOfLaw`, where it already owns its serialization. `Relation` and
`Formation` may retain their existing `toJson`/`fromJson` implementations initially;
their Storage serializers coordinate files and graph binding without duplicating their
meaning.

## Load and save contract

1. Read semantic root records: Persons, Objects, Materials, Lexemes, Zones/Home
   augmentations, and Ourverse records. No raw pointers are restored in this phase.
2. Register every stable identifier in one load-scoped resolver.
3. Read Formation memberships and Relation endpoint identifiers; bind only through that
   resolver. An unresolved endpoint is reported and preserved for retry, never silently
   omitted on the next save.
4. Apply derived links only after their base graph is present: Zone formation relations,
   Home stakes/inhabitants, Ourverse gathering Zone/filaments/metalaws, and Person body
   attachment relationships.
5. Hydrate `.ecmatter` by identifier after semantic identity exists. Physical density
   never decides what a being is.

The existing `zoneObjectsFromJson` composition relink and `applyFormationRelations`
already demonstrate steps 2–4 in miniature. The migration must extract those mechanisms
without changing their order. In particular, the 2026-08-24 relation-graph loss was
caused by applying links only inside an empty-object branch; the new structure must have
a direct regression test for populated Zones.

## Compatibility and safety

- Existing `.json`, `.ecsave`, `.ecform`, per-Zone `zone.json`, and Home identity files
  without `semanticRoots` remain readable through the legacy path. No save is rewritten
  merely by opening it.
- Writers emit `semanticRoots` version 1. A valid envelope is authoritative; a malformed
  envelope refuses the load rather than selecting a stale compatibility projection.
- Stable identifiers, never generated counters, are the keys of cross-file links.
- No serializer may hide a registered, authored property. The implementation audit must
  enumerate every registered non-derived property per root and either round-trip it or
  name the physical `.ecmatter` location that does.
- The `.ecform`/`.ecmatter` split remains the separate open task. Directory surgery must
  not claim it has retired the current quadruple write.

## Missing persistence roots to settle before migration

| Root | Current state | Required decision |
|---|---|---|
| Person | Person-side codec owns `serialize`/`deserialize`; current sessions write only `semanticRoots.person` | Retain `playerBody` as a reader-only bridge for older authored sessions. |
| Zone | `ZonesOfEarth/ZoneSerialization` owns the extracted codec and graph assembly | Keep base fields, object references, and graph binding phased. |
| Home | `HomeSerialization` owns dwelling augmentation | Preserve Zone composition; do not duplicate base Zone state. |
| Ourverse | `OurverseSerialization` emits and hydrates the semantic root record | Retire the legacy multi-write path only after the split substrate owns this root. |
| Relation / Formation | Both root codecs now live under Storage; Formation link hydration is idempotent | Extend file orchestration without embedding links as Zone-owned subtrees. |
| Object | `ConstructedBeing/ObjectSerialization` owns the extracted ADL codec | Later separate semantic records from legacy geometry/material adapters. |

## Migration sequence and proof

1. ✅ `semanticRoots` version 1 now orchestrates Person, Zone/Home, and Ourverse records;
   legacy top-level sessions remain readable and malformed advertised roots refuse loudly.
2. ✅ `singular_serialization_topology_test` now creates a Person, Home, non-Home Zone,
   Ourverse, Relation and Formation crossing those roots; its session round-trip asserts
   identity, endpoint binding, Home state, Person Body state, and semantic-root shape.
   `relation_serialization_topology_test` separately proves the Relation root's
   identifier-preserving unbound and bound hydration phases.
3. Run the focused tests: `save_roundtrip_test`, `zone_identity_test`,
   `zone_relation_roundtrip_test`, `object_roundtrip_test`, `person_database_test`,
   `person_serialization_test`, and `ourverse_serialization_test`, then run the full
   suite.
4. A Person must still click Save → quit → reopen → Load and inspect a cross-Zone
   Relation. Add that check to the Person Verification List only when code changes make
   it newly relevant.

## Boundary of this pass

This pass did not modify authored save files, remove the legacy reader, or invent a DSL.
The focused graph, identity, Save As, Person database, legacy-body bridge, semantic-root,
and cross-root integration tests pass; the Home ontology binary stalls on this host's
HomeServices/XPC connection before producing test output. Zone compatibility projections
remain until the identity-store migration can retire them safely.
