# Singular serialization topology

**Status:** Phase 1 landed — source boundaries moved without changing the save schema; Ourverse load integration and writer retirement remain open.  
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

## Target source layout

All of this remains under the existing `Singularity/Storage/` modality. No top-level
subsystem is added.

```
src/Singularity/Storage/
  Serialization.hpp                 // narrow compatibility facade; no definitions
  Serialization/
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
  are read through a legacy adapter first. No save is rewritten merely by opening it.
- Writers emit a versioned root record and preserve unknown keys byte-for-byte where a
  reader lacks semantic support. This follows the existing unsupported Law-node policy.
- Stable identifiers, never generated counters, are the keys of cross-file links.
- No serializer may hide a registered, authored property. The implementation audit must
  enumerate every registered non-derived property per root and either round-trip it or
  name the physical `.ecmatter` location that does.
- The `.ecform`/`.ecmatter` split remains the separate open task. Directory surgery must
  not claim it has retired the current quadruple write.

## Missing persistence roots to settle before migration

| Root | Current state | Required decision |
|---|---|---|
| Person | Member `serialize`/`deserialize`; Body helpers now have a Person-side module | Add a Person root adapter without changing PersonDatabase format. |
| Zone | `ZonesOfEarth/ZoneSerialization` owns the extracted codec and graph assembly | Keep base fields, object references, and graph binding phased. |
| Home | `HomeSerialization` owns dwelling augmentation | Preserve Zone composition; do not duplicate base Zone state. |
| Ourverse | `OurverseSerialization` emits the semantic root record; load is not wired | Add root loading and a graph-link regression test. |
| Relation / Formation | Formation link hydration is extracted and idempotent | Extend file orchestration without embedding links as Zone-owned subtrees. |
| Object | `ConstructedBeing/ObjectSerialization` owns the extracted ADL codec | Later separate semantic records from legacy geometry/material adapters. |

## Migration sequence and proof

1. Keep the extracted layout behind the façade; no schema change in this rung.
2. Add a `singular_serialization_topology_test`: create a Person, Home, non-Home Zone,
   Ourverse, Relation and Formation crossing those roots; save/load; assert identity,
   endpoint binding, Home-only state, and material/geometry references survive.
3. Run the focused tests: `save_roundtrip_test`, `zone_identity_test`,
   `zone_relation_roundtrip_test`, `object_roundtrip_test`, `person_database_test`, and
   the new topology test. Then run the full suite.
4. A Person must still click Save → quit → reopen → Load and inspect a cross-Zone
   Relation. Add that check to the Person Verification List only when code changes make
   it newly relevant.

## Boundary of this pass

This pass did not modify authored save files, delete legacy writers, or invent a DSL.
The focused graph, identity, Save As, and Person database tests pass; the Home ontology
binary stalls on this host's HomeServices/XPC connection before producing test output.
Ourverse load integration and writer retirement are separately tracked before the
open split-substrate task can claim the save system is fully fluid across roots.
