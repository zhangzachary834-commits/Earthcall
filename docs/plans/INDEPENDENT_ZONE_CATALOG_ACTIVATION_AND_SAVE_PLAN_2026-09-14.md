# Independent Zone Catalog, Activation, Save, and Legacy Retirement Plan

**Status:** IMPLEMENTATION IN PROGRESS — the Pass 1 root-ID foundation and Pass 2 catalog/generation storage foundations landed on 2026-09-14. ID-based cross-root references/managers and Passes 3–9, witness migrations, Save Zone, activation/default flips, Person verification, and UI retirement remain open. No authored save data has been modified by these implementation passes.
**Target branch:** `sol/event-interest-fastpath-20260913` at planning baseline `f0c259b4656f30daacecac309541aa91fc52eb6d`  
**Authorial requirement:** Zach requires Creator Console → Zones → **Move to Zone** to yield the complete Zone, and **Save Zone** to persist that Zone without Assets → Files → Load or a named conglomerate session.  
**Origin and extension:** Zach supplied the lived boundary and the preservation requirement. This plan integrates the existing Per-Zone and Singular Serialization task documents, the Basic Pixel Changer and split-substrate intercom findings, and the current branch implementation into a staged transaction design.

## 1. Outcome

Ordinary Earthcall boot reads a lightweight catalog of independently addressable Zones and Homes. Moving to a catalog entry prepares its complete transitive closure in detached memory, validates every identity, dependency, graph edge, Law binding, and matter generation, and then activates the entire result at one commit boundary. Law evaluation cannot see the preparation phase. Saving commits only the active Zone generation and the exact closure generations changed through it; it never serializes unrelated live Zones or asks for a world/session name.

Legacy `saves/worlds/` files remain byte-preserved compatibility sources behind explicit **Legacy Import**, **Legacy Export**, and **Recovery** controls. Inspection and dry-run migration never rewrite them.

This plan retires the ordinary Assets → Files → Load conglomerate workflow only after preservation and witness gates pass. It does not delete the legacy reader.

## 2. Current branch baseline and open boundary

The implementation starts from, rather than repeats, the work already landed:

- The documented inventory has **25 Zone identities, 2 Homes, and 7 shared Law roots** available independently of an ordinary world selection.
- Basic Pixel Changer is the first complete Law-closure witness: its Zone names shared `lawRefs`, activation preflights authors/targets/triggers, and the Person-witnessed canvas click works without `loadState`.
- `Object::to_json` again preserves semantic pose, `object_pose_serialization_guard_test` mechanically guards registered pose fields, and `object_semantic_pose_test` proves the Zone-identity boot path.
- Synthesis Studio and SynthesisStudio.LivingInstrument received preservation-scoped pose recovery with backups and field-only verification.
- Zone/Home folder-key validation, identifier/name separation, owner-scoped matter addressing, duplicate composite-key refusal, and generation hash/length verification are already present.
- The branch currently exposes 7 Law roots under `saves/laws/` and `ZoneManager::switchTo` preflights only those Laws.

Still open, and therefore owned by this plan:

- General Material, Category, Relation/Formation, and verified Zone-scoped matter closure.
- A detached whole-closure validator rather than Law-only preparation inside `switchTo`.
- `Save Zone`. Current `persistZones()` walks every live Zone, and ordinary Save/Quick Save still writes a session.
- Boot catalog metadata that does not hydrate every Zone into the live world.
- Preservation-tested legacy migration and UI retirement.

The plan treats the old claim that per-Zone serialization was “done” as superseded by Zach’s lived Go witness: shapes and relations could appear while Materials and Laws remained trapped in `go_app.ecform`.

## 3. Non-negotiable invariants

1. **Persisted identity is `Identity::SingularId`.** Every persisted root and cross-root reference carries a canonical opaque `ec1:…` identifier (or a `did:earthcall:…` Person identifier). Equality, closure traversal, collision checks, and generation ownership use that value.
2. **Stable slugs remain authored addresses.** Existing stable strings such as `material.clay` and `law-basic-pixel-changer` remain the names Law text and property paths resolve. A slug is a cataloged alias of one SingularId, not identity itself.
3. **No silent minting while reading.** Missing legacy IDs are assigned only by an explicit migration apply using its frozen dry-run report. Ordinary boot/load never invents an ID to make malformed data appear valid.
4. **No partial world visibility.** Detached preparation publishes no being to `Universe`, no Material/Category/Law to a live manager, no Rete fact, no EventBus edge, and no current-Zone change.
5. **No iteration-order authority.** Duplicate IDs, duplicate aliases in one activation namespace, competing legacy records, and ambiguous ownerless matter refuse. First/last writer never wins.
6. **Shared roots remain one identity.** Zone manifests reference versioned generations of a shared Material, Law, Category, Relation, or Formation. They do not embed private copies.
7. **Zone save isolation is mechanical.** A Save Zone transaction cannot enumerate or rewrite an unrelated Zone/Home. A test compares bytes and generations for every non-target root.
8. **Inspection is read-only.** Catalog scan, migration dry-run, legacy preview, diff, and validation create no identity file, sidecar, backup, timestamp update, or normalized rewrite.
9. **Existing formats remain readable.** `.json`, `.ecsave`, `.ecform`, `.ecmatter`, Zone identities, and Home identities remain import/recovery inputs.
10. **Authored saves remain sacred.** Migration writes a parallel target and preservation bundle; it never edits, deletes, or “cleans up” a source in place.

## 4. Persistence topology

### 4.1 Catalog

Add one derived Storage index:

`saves/catalog/zones-v1.json`

Each entry contains:

- `singularId`: canonical persisted identity;
- `slug`: stable Law/property-path alias;
- `displayName`;
- `kind`: Zone or Home plus authored Home/community facts already present;
- `headGeneration` and `manifestHash`;
- `rootPath`: exact relative path, retained so legacy slug-named directories do not need to move during first migration;
- `formatVersion` and minimal availability/error metadata.

Boot reads this file and creates catalog rows for the Zones console. It does **not** instantiate Zone Objects, Materials, Laws, Relations, or matter. The catalog is derived and rebuildable by an explicit read-only scan. A malformed catalog refuses catalog use and offers **Rebuild Catalog Preview**; it does not silently hydrate every folder.

Catalog writes use temp-file + fsync + atomic rename. The previous catalog is retained as the rollback pointer. A new catalog head is published only after its target Zone manifest exists and hashes correctly.

### 4.2 Root identities and immutable generations

Each independently persisted Singular root gains:

- a stable `singularId`;
- a stable `slug` alias;
- immutable generation content addressed by SHA-256;
- a small root head/manifest that maps identity to a generation.

A Zone generation manifest contains typed references of the form:

`{ "singularId": "ec1:…", "slug": "law-basic-pixel-changer", "generation": "sha256:…" }`

The ID binds the being. The slug is retained for authoring, diagnostics, and Law/property resolution. A slug mismatch is reported; it cannot retarget a reference.

A Zone manifest owns its Zone fields, object membership/semantic Object records, Zone-local graph membership, and its exact matter generation. It references shared root generations for Materials, Laws/triggers, Categories, Relations, and Formations. Versioned generations do not duplicate identity: two Zones may pin different historical generations of the same Material identity until a Person explicitly promotes the newer shared generation.

This generation pinning is what makes Save Zone isolated. Writing a changed shared Material produces a new immutable generation and points only the saved Zone at it. Other Zones remain pinned to their prior generation until explicitly updated. **Promote Shared Root** is a separate, impact-reporting action; Save Zone never rewrites other Zone manifests behind the Person’s back.

### 4.3 SingularId and slug transition

The current branch still uses `Singular::getIdentifier()` as the stable string slug, while `Identity::SingularId` exists separately. Do not replace `getIdentifier()` or break Law text.

Introduce an orthogonal persisted-ID property on every persistable Singular:

- `singularId()` returns the canonical persisted ID;
- `getIdentifier()` continues to return the stable authored slug;
- serializers write both `singularId` and `identifier`/`slug` during transition;
- resolver maps are keyed by SingularId and maintain a separate slug-to-ID alias index;
- property-path lookup remains longest-stable-slug-first, then resolves to the ID-bound live being;
- duplicate live slugs within the same activation namespace refuse before commit, even if IDs differ, because Law text would be ambiguous.

Legacy records without `singularId` are accepted only by the legacy reader and migration analyzer. Migration assigns deterministic continuity through a frozen mapping keyed by source-file hash + persistence-root kind + owner scope + legacy stable slug. Re-running the same approved report yields the same IDs; a changed source hash invalidates the report and requires a new dry run.

## 5. Complete transitive closure

The closure builder starts with the selected Zone generation and traverses typed references by SingularId with a visited set. It emits both the detached bundle and a human-readable dependency report.

Required closure:

1. **Zone/Home root:** identity, name, owner, parent, scope/kind, time/physics fields, Joys, object membership, lexemes, and formation graph.
2. **Objects:** every semantic Object in the Zone, including dynamic authored properties, tags, designations, representation fields, pose, and all registered non-derived fields.
3. **Materials:** each Object material reference, Material texture/paint state, and any Material-to-Material references. Missing explicit material is distinct from the valid default Material identity.
4. **Laws and triggers:** Zone `lawRefs` plus Laws reached by Zone/Formation membership; authors, targets, jurisdiction, trigger bindings, condition/action referents, and referenced state/channel roots. Rete facts and runtime memories are derived and never closure roots.
5. **Categories:** authored Category beings and the category Formation/instance relations needed to interpret membership. Category slugs are aliases; category identity is SingularId.
6. **Relations and Formations:** all relations whose membership is asserted by the Zone graph; both endpoint roots; Formation metadata, tags, `convenesToward`, membership, and cross-Zone references. A cross-Zone endpoint may be admitted as a referenced shared root without activating the other Zone’s whole scene.
7. **Matter:** exactly one verified Zone-scoped generation whose manifest membership set equals the semantic Object membership set for matter-bearing Objects. Every entity address is (`owner Zone SingularId`, `Object SingularId`). Schema version, byte length, hash, duplicate key, missing member, and extra member are validated before application.
8. **Person and authority references:** referenced Person authors/owners resolve to verified Person identities. A file-claimed cryptographic Person is never trusted merely because the text says so.
9. **Ourverse/global roots:** admitted only when explicitly referenced by the Zone and kept under their own standing. Moving to a Zone does not duplicate or silently re-author Ourverse state.

The closure report classifies each dependency as owned, shared+pinned, external reference, derived runtime state, or refused. Every serializer’s registered non-derived property must map to semantic root storage or named matter storage; the persistence-coverage test fails on omissions.

## 6. Detached validation and atomic activation

Implement the load path as prepare → validate → commit, using mechanism structs under `Singularity/Storage/` rather than a new domain noun or subsystem.

### Prepare

- Resolve the catalog row and verified Zone manifest.
- Read all immutable root generations and matter bytes into detached ownership.
- Deserialize semantic roots without registering them in global managers.
- Build an ID resolver and a separate slug alias resolver.
- Bind detached Relation endpoints and Formation membership only within the detached resolver.
- Deserialize Laws, authors, targets, jurisdiction, actions/conditions, and triggers without connecting EventBus or Rete.
- Apply verified matter only to detached Objects.
- Accumulate every error; do not mutate live state to discover the next error.

### Validate

Validate schema/version support, canonical SingularIds, generation hashes, typed reference kinds, unique IDs, unambiguous aliases, Person authorship, Law triggers, graph endpoints, category acyclicity, exact semantic/matter membership, no duplicate composite keys, registered-property persistence coverage, and activation resource limits. Any failure yields a structured refusal report and leaves the current Zone byte-for-byte/runtime-identical.

### Commit

- Enter a short activation barrier owned by the engine update thread: Law tick paused, external events queued, no renderer traversal of changing registries.
- Snapshot only runtime registrations that may change: current Zone index, active closure IDs/generations, Zone-scoped manager entries, trigger bindings, Rete derived state, and active Object view.
- Install the already-validated roots into managers by ID.
- Swap the active Zone/Object view.
- Bind Relations/Formations, register Zone-scoped Laws/triggers, then seed/rebuild derived Rete facts.
- Publish `zone-exited`, `zone-loaded`, and `zone-entered` only after the new closure is coherent; flush queued external events afterward.
- On any commit-time exception, restore the runtime snapshot before releasing the barrier and publish no entered/loaded event.

Law evaluation therefore observes either the old complete closure or the new complete closure, never a midpoint. Rendering follows the same active-view swap.

## 7. Isolated, crash-safe Save Zone

`ZoneManager::persistZones()` is not the Save Zone implementation because it enumerates every live Zone. Add an explicit target operation:

`saveZone(const Identity::SingularId& zoneId, const SaveContext&, SaveZoneOptions)`

The operation computes the target Zone’s dirty closure from the activation ledger. It refuses if the target is not the active, fully activated catalog generation; if closure validation is incomplete; if an unsaved reference cannot be pinned; or if the target would require an unrelated Zone rewrite.

Commit protocol:

1. Build a save manifest listing the exact target Zone, old head, new root generations, byte counts/hashes, shared-root impact, and untouched-root witness set.
2. Write each changed semantic/matter generation to a same-filesystem staging directory; fsync every file.
3. Re-read and verify hashes, schemas, IDs, references, and exact matter membership from staging.
4. Atomically move immutable generation files into their final content-addressed locations. Existing generations are never overwritten.
5. Atomically replace the target Zone head manifest; fsync its directory. This is the Zone commit point.
6. Atomically replace the catalog pointer to the new head; fsync the catalog directory.
7. Mark only committed closure entries clean and report the exact roots/generations written.

A crash before step 5 leaves unreachable immutable generations that recovery/GC may list; the active head is unchanged. A crash after step 5 but before step 6 is recovered by catalog/head reconciliation, choosing the valid manifest with a recorded transaction ID and never guessing between divergent valid heads. Old heads and generations remain available for rollback.

**Save All Dirty Zones** may call this operation once per Zone and report independent results. It is not a multi-Zone transaction and never creates a new conglomerate identity.

## 8. Legacy migration: dry-run → apply → verify

### Dry-run (mandatory and read-only)

`scripts/migrate_zone_catalog.py --dry-run` scans Zone/Home identities, Law roots, and legacy world/ecform/ecmatter sources. It writes nothing unless the Person explicitly supplies an external report output path; the in-app preview remains memory-only.

The report includes:

- source path, size, SHA-256, detected format/version;
- proposed root kind, SingularId mapping, slug, owner scope, and generation hash;
- Zone membership and complete dependency closure;
- world-only Materials/Laws/Categories/Relations/matter to be recovered;
- duplicate IDs/slugs, folder/document mismatch, ownerless matter ambiguity, competing sidecars, missing refs, schema incompatibility, and differing authored values;
- exact files that apply would create;
- explicit `REFUSED` items requiring a Person decision.

No source is normalized, timestamped, backed up, or rewritten during dry-run/inspect.

### Apply (explicit approval and frozen input)

`--apply --report <approved-report>` refuses unless every source hash and branch format assumption still matches the dry run. Apply:

- creates a preservation bundle containing the report, source hash inventory, and byte-for-byte copies or immutable references to every source;
- writes only parallel ID-bearing roots/generations/catalog staging;
- never deletes or edits `saves/worlds/*` or the prior Zone/Home/Law roots;
- refuses unresolved collisions rather than selecting a candidate;
- records migration provenance and Person authorization without claiming the agent authored the beings;
- atomically publishes catalog entries only after new generations verify.

### Verify

Verification reloads the new catalog in a fresh process/root, activates each migrated witness without `loadState`, compares semantic graphs and matter hashes against the approved report, and confirms all legacy source hashes remain unchanged. Failure rolls the catalog pointer back and leaves the parallel generations for diagnosis; it never “repairs” the source.

## 9. Witness migration order

1. **Go first — preservation canary.** Recover the known 378 shapes, 381 relations, 5 face-textured Materials, and 3 authored Laws from the Go legacy sources into one cataloged closure. Fresh boot → Move to Zone must render and play Go without Assets load. Save Zone must persist a legal change and leave every unrelated Zone byte-identical.
2. **Chess second — graph/Law stress witness.** Preserve its relation graph, categories, complete rule/trigger closure, pose, and matter. Run the existing chess logic/gesture/castling/extended suite and a fresh-boot catalog activation test.
3. **Synthesis Studio and Living Instrument third — shared roots + rich matter witness.** Preserve recovered semantic pose, all later target-only beings, Materials, audio/interaction Laws, and distinct Zone identities. Compare against the recovery hashes and run both Studio suites.
4. **Sanctum of Beginnings fourth — onboarding/production witness.** Preserve all authored placement and teaching sequence. It becomes the proof that the production entry Zone needs no hidden session ceremony.
5. **Fleet verification.** Dry-run all remaining catalog entries, both Homes, and all 7 current Law roots. No bulk apply proceeds while any collision or ambiguity remains unresolved.

Basic Pixel Changer remains the small, fast regression witness throughout; it is not counted as proof that the general closure is complete.

## 10. UI retirement gates

### Gate A — catalog shadow mode

Zones console shows catalog rows and validation status, but existing switch remains available. **Move to Zone (transactional preview)** uses the new detached path behind an explicit development control. Assets remains unchanged.

### Gate B — new ordinary path

After Go and Chess pass, Creator Console → Zones owns:

- **Move to Zone**
- **Save Zone**
- **Save All Dirty Zones**
- dependency/dirty/shared-impact details
- clear refusal and rollback reports

Assets labels world/session actions **Legacy** and removes them from the default expanded surface.

### Gate C — rich witnesses and migration

After Synthesis/Living Instrument and Sanctum pass, plus fleet dry-run with no unresolved silent cases, the old Assets **Quick Save**, **Save As**, and ordinary **Load a session** controls are removed from the normal path.

Assets retains actual Materials/assets plus a collapsed, explicitly named compatibility surface:

- **Legacy Import** — read a world/session into migration preview, then explicit apply;
- **Legacy Export** — package selected Zone closures into a transport/session envelope without making that envelope persistence authority;
- **Recovery** — before-load snapshots, prior Zone heads, orphan staged generations, and preservation bundles;
- **Legacy Session Inspector** — read-only by default; never rewrite on open.

### Gate D — default and cleanup

The transactional path becomes the only ordinary Zone move/save path after all acceptance criteria and Person witnesses pass. The legacy readers/tests remain. Any deletion or archival of old UI code or old files is a separate, Person-authorized task.

## 11. Exact source file map

| File | Planned responsibility |
|---|---|
| `src/Identity/SingularId.hpp`, `src/Identity/SingularId.cpp` | Keep canonical ID parsing/minting; add no slug meaning. |
| `src/ConstructedBeing/Singular/Singular.hpp`, `src/ConstructedBeing/Singular/Singular.cpp` | Add orthogonal persisted SingularId storage/access while preserving `getIdentifier()` as slug. |
| `src/Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.cpp` | Emit/read Object SingularId and ID-based refs; preserve registered semantic property coverage. |
| `src/Singularity/Storage/Serialization/ConstructedBeing/MaterialSerialization.hpp`, `MaterialSerialization.cpp` (new) | Root codec for Material identity/generation; do not duplicate Material semantics in Zone or matter. |
| `src/Singularity/Storage/Serialization/Relation/RelationSerialization.cpp` | Persist endpoint SingularIds with legacy slug hints; detached binding. |
| `src/Singularity/Storage/Serialization/Relation/FormationSerialization.cpp` | Persist member/relation IDs and perform detached, idempotent link hydration. |
| `src/Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp`, `ZoneSerialization.cpp` | Versioned Zone generation manifest, typed ID refs, legacy adapter, no live-manager side effects. |
| `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`, `Law.hpp` | Persist Law SingularId and ID-based authors/targets/jurisdiction while retaining stable Law slug and existing append-only enums. |
| `src/ConstructedBeing/Material/MaterialManager.hpp`, `MaterialManager.cpp` | Stage/install/remove by SingularId; alias lookup remains separate. |
| `src/ConstructedBeing/CategoryManager.hpp`, `CategoryManager.cpp` | Stage/install Categories by ID; validate slug alias and Formation acyclicity. |
| `src/Relation/RelationManager.hpp`, `RelationManager.cpp` | Detached graph installation/removal by ID without endpoint loss. |
| `src/ZonesOfEarth/AuthorsOfLaw/Law.hpp`, `Law.cpp` | LawManager detached admission, trigger binding batch, Rete rebuild/rollback boundary. |
| `src/ZonesOfEarth/AuthorsOfLaw/Universe.hpp` | Batch admission/removal hooks used only during activation commit; no partial registration. |
| `src/Singularity/Storage/ZoneCatalog.hpp`, `ZoneCatalog.cpp` (new) | Lightweight derived catalog read/validate/rebuild-preview/atomic head publication. Mechanism, not domain ontology. |
| `src/Singularity/Storage/ImmutableGeneration.hpp`, `ImmutableGeneration.cpp` (added in Pass 2) | Content-addressed JSON generation write/reread verification and compare-and-replace root heads with prior-head retention. |
| `src/Singularity/Storage/ZoneClosure.hpp`, `ZoneClosure.cpp` (new) | Typed closure traversal, detached bundle, validation/refusal report, generation ledger. Mechanism, not a new kind of being. |
| `src/Singularity/Storage/SaveSystem.hpp`, `SaveSystem.cpp` | Content-addressed paths, staging/fsync/atomic primitives, immutable generations, legacy read-only access. |
| `src/ZonesOfEarth/ZoneManager.hpp`, `ZoneManager.cpp` | Replace Law-only `switchTo` preparation with whole-closure orchestration; add explicit target `saveZone`; retain `loadState` as legacy import/recovery. |
| `src/Singularity/Storage/FlatBuffers/earthcall_heavy.fbs` and generated `src/Singularity/Storage/Schema/Earthcall_generated.h` | Append owner/Object SingularId and generation metadata; never renumber/remove existing fields. |
| `src/Singularity/Core/EngineUpdate.cpp` and `EngineInit.cpp` | Activation barrier/event queue and catalog-only boot; no boot hydration of every Zone. |
| `src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/ZonesConsole.cpp` | Complete catalog, Move to Zone, Save Zone, reports, dirty/shared impact, rollback controls. |
| `src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/AssetsConsole.cpp` | Retire normal session controls by gates; retain Materials plus Legacy Import/Export/Recovery/Inspector. |
| `scripts/migrate_zone_catalog.py` (new) | Preservation-first dry-run/apply/verify migration with frozen hashes and collision reports. |
| `docs/Agenda/Tasks/For Zach/Person Verification List.md` | Add human witness steps only when the implementing pass creates something a Person must inspect. |
| `docs/Agenda/Tasks/Specific Tasks/Per_Zone_serialization_pathway/Per_Zone_serialization_pathway.md` | Progress record and link to this plan; To-Do bullet remains a one-line index. |
| `docs/Agenda/Tasks/Specific Tasks/Singular_Serialization_Topology/Singular_Serialization_Topology.md` | Update phase status when ID-root and detached hydration passes actually land, not before. |
| `docs/BUILD_AND_ENVIRONMENT.md` | Update test inventory/meaning only after executable tests land. |

The implementation may split a listed mechanism file further only within the same ontological directory and must update this map before claiming the pass complete.

## 12. Exact test file map

### New focused tests

| File | Proof |
|---|---|
| `tests/identity/singular_persistence_id_test.cpp` | Persisted ID round-trip, invalid/noncanonical refusal, slug rename without identity change, duplicate slug activation refusal. |
| `tests/zones/zone_catalog_boot_test.cpp` | Boot reads lightweight rows only; 25-Zone/2-Home baseline discoverability; malformed/stale catalog refusal and read-only rebuild preview. |
| `tests/zones/immutable_generation_store_test.cpp` (added in Pass 2) | Deterministic generation address, idempotent write, verified read, stale-head refusal, prior-head retention, and corruption refusal under a disposable root. |
| `tests/zones/zone_transitive_closure_test.cpp` | Complete Object/Material/Law/trigger/Category/Relation/Formation/matter closure, cycles handled by ID visited set, shared roots not copied. |
| `tests/zones/zone_activation_transaction_test.cpp` | Missing/malformed/ambiguous dependency leaves current Zone, managers, EventBus, Rete, and active Object view unchanged; successful commit becomes visible all at once. |
| `tests/zones/zone_matter_membership_test.cpp` | Exact semantic/matter membership, owner+Object SingularId resolution, hash/schema/duplicate/missing/extra refusal. |
| `tests/zones/zone_save_isolation_test.cpp` | Save Zone writes one target head and exact changed generations; all unrelated Zone/Home/root bytes and heads unchanged; crash point recovery for each commit step. |
| `tests/zones/zone_legacy_migration_test.cpp` | Dry-run has zero writes; frozen-hash apply; collision report/refusal; original sources byte-identical; verify and catalog rollback. |
| `tests/zones/go_zone_catalog_witness_test.cpp` | Fresh boot + Move to Zone yields 378 shapes, 381 relations, 5 Materials, 3 Laws and functional Go without `loadState`. |
| `tests/zones/chess_zone_catalog_witness_test.cpp` | Fresh catalog activation preserves complete Chess graph/rules/triggers without session load. |
| `tests/zones/synthesis_zone_catalog_witness_test.cpp` | Both Studio identities retain recovered poses, Materials, Laws, matter, and later beings. |
| `tests/zones/sanctum_zone_catalog_witness_test.cpp` | Sanctum placement/sequence arrives complete from catalog activation. |
| `tests/singularity/creator_console_zone_persistence_test.cpp` | Human-facing routing: Zones controls call transactional move/save; ordinary Assets session buttons are absent after the gate and Legacy controls remain explicit. |

### Existing tests to extend or keep green

- `tests/identity/singular_id_test.cpp`
- `tests/zones/zone_identity_test.cpp`
- `tests/zones/zone_identity_boundary_test.cpp`
- `tests/zones/zone_identifier_name_split_test.cpp`
- `tests/zones/zone_relation_roundtrip_test.cpp`
- `tests/zones/object_semantic_pose_test.cpp`
- `tests/zones/object_pose_serialization_guard_test.cpp`
- `tests/zones/matter_generation_commit_test.cpp`
- `tests/zones/matter_scoped_writer_test.cpp`
- `tests/zones/matter_semantic_precedence_test.cpp`
- `tests/zones/zone_load_error_path_test.cpp`
- `tests/zones/zone_manager_error_path_test.cpp`
- `tests/zones/save_roundtrip_test.cpp`, `world_switch_test.cpp`, `unsaved_preserve_test.cpp` (legacy behavior, explicitly labeled)
- `tests/zones/singular_serialization_topology_test.cpp`
- `tests/zones/session_semantic_roots_test.cpp`
- `tests/law/basic_pixel_changer_test.cpp`
- `tests/law/go_app_test.cpp`
- `tests/law/chess_app_test.cpp`, `chess_click_geometry_test.cpp`, `chess_gesture_test.cpp`, `chess_castling_test.cpp`, `chess_extended_rules_test.cpp`
- `tests/law/synthesis_studio_app_test.cpp`, `synthesis_studio_living_test.cpp`
- `tests/constructed-being/object_roundtrip_test.cpp`, `material_being_test.cpp`, `paint_test.cpp`
- `tests/singularity/relation_serialization_topology_test.cpp`, `serialization_compat_test.cpp`
- `tests/person/person_serialization_test.cpp`
- `tests/singularity/no_black_box_test.cpp` and `channel_paths_test.cpp`

All filesystem tests use isolated temporary SaveRoots. No test may point at or mutate the repository’s authored `saves/` tree; `RealSaveTreeGuard` is not acceptable for new persistence tests because parallel tests have already raced that boundary.

## 13. Pass sequencing

1. **Pass 0 — inventory and executable red tests.** Freeze current branch/source hashes, add read-only fleet inventory, ID/slug migration report shape, and failing catalog/closure/save-isolation tests. No authored saves.
2. **Pass 1 — ID dual-read/dual-write infrastructure.** Add SingularId beside slug across codecs and managers. Legacy readers remain. Roll back by disabling ID writer and retaining new fields as ignored compatibility data.
3. **Pass 2 — immutable root generations and catalog shadow mode.** Add catalog/root Storage mechanisms and read-only rebuild preview. Existing boot remains default. Roll back by selecting the old boot path; no old file changed.
4. **Pass 3 — complete detached closure.** Materials, Categories, Relations/Formations, Laws/triggers, and exact matter validation. No live activation yet. Roll back by removing the development entry point.
5. **Pass 4 — atomic activation.** Add barrier, batch install, Rete/event ordering, rollback. Make Basic Pixel and synthetic failure tests green; then Go.
6. **Pass 5 — isolated Save Zone.** Land immutable-generation commit protocol and crash injection tests. Keep ordinary Assets Save/Load until isolation tests and Go Person witness pass.
7. **Pass 6 — migration tool and Go apply.** Dry-run/report, Person decision on every collision, parallel apply, fresh-root verify. Never bulk-migrate from the first successful app.
8. **Pass 7 — Chess, Synthesis/Living, Sanctum.** One reviewed migration per witness with semantic and Person verification before the next.
9. **Pass 8 — UI retirement Gate C.** Remove normal Assets session controls only after fleet dry-run, all automated criteria, and all four Person witnesses pass.
10. **Pass 9 — full fleet apply and default flip.** Apply only approved, collision-free reports; full suite and live verification; document exact rollback catalog/head. Legacy reader remains.

Each pass is independently reviewable and revertible. Do not combine ID migration, bulk authored-save migration, and UI retirement in one commit or PR.

## 14. Refusal behavior

| Condition | Required behavior |
|---|---|
| Missing/malformed catalog or head | Refuse transactional move; keep current Zone; offer read-only rebuild preview or Legacy Recovery. |
| Invalid/missing SingularId | Ordinary path refuses. Legacy inspector may analyze; only approved migration apply may assign. |
| Duplicate ID or slug alias | Refuse whole closure and list every root/path/candidate. |
| Missing Material/Law/Category/Relation endpoint/Formation member | Refuse before live mutation; list the transitive path that required it. |
| Law author not a verified Person, target unresolved, OnEvent trigger absent | Refuse whole activation; no trigger/Rete registration. |
| Matter hash/length/schema/member mismatch or duplicate composite key | Refuse whole activation; semantic Zone is not partially entered. |
| Competing legacy sidecars/values | Dry-run reports all sources and values; apply refuses pending Person choice. |
| Concurrent source/catalog change after dry-run | Apply refuses because frozen hashes no longer match. |
| Save target is not active/fully validated | Save Zone refuses; no stage or head write. |
| Shared dirty root affects other Zones | Save Zone pins a new generation only for target and reports impact; global promotion is separate. |
| Crash/failure before Zone head rename | Old head remains authoritative; orphan staging/generations listed for Recovery. |
| Crash after head rename before catalog rename | Reconcile by transaction ID + verified manifest; never choose by mtime. |
| Legacy inspection/load | Read-only unless Person explicitly chooses Import Apply; no rewrite-on-inspect. |

Every refusal returns a structured result to both logs and UI: operation, target ID/slug, phase, exact file/root, dependency chain, old head, proposed head if any, and statement that the current Zone remained active.

## 15. Acceptance criteria

The feature is not complete until all are true:

- Fresh boot catalogs every valid Zone/Home without hydrating complete scenes or selecting a world file.
- SingularId, not slug or directory name, is the persisted reference truth across every new root and matter entity.
- Stable slug/property addresses behave exactly as before and ambiguity refuses.
- Move to Zone loads the complete verified transitive closure and Laws cannot observe partial hydration.
- Missing/ambiguous/corrupt dependency tests prove zero live-state mutation.
- Save Zone survives injected crashes at every commit boundary and changes no unrelated Zone/Home head or bytes.
- Shared roots retain one ID; generation pinning prevents target-Zone save from silently changing another Zone.
- Dry-run and inspection cause zero writes. Apply requires matching frozen hashes. Verify proves all legacy sources unchanged.
- Go, Chess, both Synthesis Zones, and Sanctum activate from the catalog without `loadState` and pass their automated witnesses.
- Zach verifies the live Move to Zone and Save Zone experience for all four witnesses.
- Assets normal session Save/Load is retired only after the gates; Legacy Import/Export/Recovery remain.
- The complete focused suite, default build, `earthcall_webgpu` build, and full `ctest --output-on-failure` pass, with documented environment-only exceptions handled according to `docs/BUILD_AND_ENVIRONMENT.md`.
- The To-Do list remains a one-line index and implementation progress lives in the task documents.
- No authored save source is deleted or rewritten as a side effect of boot, inspect, dry-run, test, or legacy load.

## 16. Rollback boundaries

- **Runtime activation rollback:** restore the old closure snapshot inside the activation barrier; publish no new Zone event.
- **Single-save rollback:** repoint the target catalog entry to the previous verified head. Immutable old generations remain.
- **Migration rollback:** restore the pre-apply catalog pointer; parallel migrated roots remain quarantined for inspection. Legacy sources were never touched.
- **UI rollback:** Gate B/C is a separate commit from data migration; restore the prior surface without changing persisted heads.
- **Fleet rollback:** per-Zone heads roll back independently. Never roll back by replacing the whole `saves/` directory.
- **No authorized deletion boundary:** garbage collection, old-world archival, source deletion, and destructive normalization are outside this plan and require a separate Person-authorized task.

## 17. Person verification required by implementing passes

When implementation reaches the relevant gate, add concise checks to `docs/Agenda/Tasks/For Zach/Person Verification List.md`:

1. Fresh launch: Zones list appears without loading Assets/world; catalog browsing is fast and does not alter save mtimes/hashes.
2. Go: Move to Zone shows the complete board/materials and three Laws work; make one change, Save Zone, quit/relaunch, and confirm it persists while an unrelated Zone is unchanged.
3. Chess: play, castle, promote, save/reopen, and confirm graph/rules remain.
4. Synthesis/Living: verify recovered non-cube forms, satellites, Materials, controls/audio, save/reopen, and no cross-contamination between the two identities.
5. Sanctum: verify authored spatial sequence and onboarding behavior from a fresh boot.
6. Refusal witness: temporarily select a test fixture with a missing dependency and confirm the current Zone continues visibly and interactively unchanged with a precise refusal.
7. Assets: ordinary session Save/Load is gone from the normal surface; Legacy Import/Export/Recovery are clearly separate and inspection does not rewrite anything.

No Person checkbox is added by this planning-only pass because there is no new executable behavior to witness.

---

**Signed:** Codex / GPT-5.6 Sol  
**Task/session ID:** `zone-independent-persistence-plan-20260914`  
**Date:** 2026-09-14  
**Timestamp:** 2026-09-14T03:10:55Z

**Implementation continuation signed:** Codex / GPT-5.6 Sol
**Task/session ID:** `zone-independent-persistence-implementation-20260914`
**Date:** 2026-09-14
**Timestamp:** 2026-09-14T03:33:02Z
