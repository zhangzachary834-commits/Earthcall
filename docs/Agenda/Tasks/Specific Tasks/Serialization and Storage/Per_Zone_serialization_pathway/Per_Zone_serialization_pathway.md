# Per-Zone serialization pathway

**Status:** PARTIAL (2026-10-07) — Law and Material closure are now enforced at Move to Zone and shared across Zones; matter generations, Category roots, and a single detached whole-closure transaction remain open (see the 2026-10-07 section at the end)
**Section in the To-Do list:** Joys · Ourverse · Zones  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Per-Zone serialization pathway** — done and verified (2026-08-21, tests; in-app click still open): `saves/zones/<id>/zone.json` is the Zone identity; Homes live under `saves/homes/<id>/home.json`. Session files under `saves/worlds/` write `zoneRefs` + a dual-write `zones[]` snapshot; load keeps a live Zone, else the store, else migrates the snapshot. Empty persist over a populated identity is refused, so a boot-empty Home cannot wipe the room. `forkZone` / `diffZones` name, branch, and compare. Boot `hydrateFromZoneStore` fills empty Home/Sanctum from the store. Guarded by `tests/zones/zone_identity_test.cpp`; `save_roundtrip_test`, `world_switch_test`, `unsaved_preserve_test` updated to the identity contract. **In-app (Zach, 2026-08-23):** shapes in Home survived loading another save; FaceTextures went white (materials were session-scoped). See Home/Zone item for the paint fix. See [FIRST_MOVER_AUTHORING.md](../../../../../architecture/law/FIRST_MOVER_AUTHORING.md) §4f. - Zach's click-through and GPT-4o's "STAGNANT" save note. 

## Why the “done” status was wrong

**Authorial correction from Zach, recorded by Codex session
`01a0707e-f743-71b1-8fb9-63975012e66d`, 2026-09-09 14:04 PDT:** Zach never
wanted a conglomerate world/session file to be the step that makes a Zone complete. The
required interaction is Creator Console → Zones → **Move to Zone**, followed by **Save
Zone**. No Assets → Quick Save → name world → Load world → select Zone ceremony.

The 2026-08-21 work established stable Zone identity but did not finish per-Zone
persistence. Zach's Go observation is the decisive Person witness: Go's identity boots
378 shapes and 381 relations, while its five face-textured Materials and three authored
Laws remain in `saves/worlds/go_app.ecform`; the Zone is visible but not fully itself until
that conglomerate file is loaded. Other Zones do not enter the Zone list at all until a
world file admits them. A Zone pathway with those properties is partial, not done.

## Required completion contract

- Boot catalogs every valid Zone/Home identity independently of `saves/worlds/`.
- Moving to a Zone gathers and validates its complete referenced closure—Objects,
  Materials, Laws/triggers, Categories, Relations/Formations, and physical matter—before
  switching the Person into it.
- Missing or ambiguous dependencies refuse the move loudly; the current Zone remains
  unchanged. Laws cannot tick against a partially admitted Zone.
- Saving from the Zone window commits that Zone alone, without a world-name prompt or an
  implicit save of every live Zone.
- Shared Singulars remain shared roots addressed by stable identifiers. Self-contained
  loading does not mean duplicating a Material, Law, Relation, Person, or Ourverse record
  into every Zone file.
- `saves/worlds/` remains a read-compatible migration/import/export/recovery surface until
  every authored file is preserved through conversion; it is not the ordinary runtime UX.
- Assets may continue to expose actual assets and explicit legacy import, but ordinary
  Save/Load controls move to Zones and the conglomerate session list is retired from the
  normal path.

## Proof required before this can be marked done again

1. Fresh boot, no Assets load: Go and every other valid Zone appear in the Zone window.
2. Click **Move to Zone** on Go: shapes, face textures, Laws, and triggers work immediately.
3. Change Go and click **Save Zone**: restart, move to Go, and observe the change; unrelated
   Zone files remain byte-identical.
4. A missing Material/Law/matter generation refuses before switching or firing any Law.
5. A shared Material or Law referenced by two Zones retains one stable identity and is not
   duplicated by either save.
6. Legacy world files remain readable and are never rewritten merely by inspection/load.

## Live failure: Zone identities contain no 3D placement — 2026-09-09

**Reported by Zach; diagnosed and recorded by Codex session
`01a0707e-f743-71b1-8fb9-63975012e66d`, 2026-09-09 18:11 PDT.** Zach observed
that Sanctum of Beginnings boots with all shapes stuffed into one place, while
Synthesis Studio's large floor/platform and separate lower prisms collapse visually into
one short white rectangular prism. He warned that Chess appeared next in the same blast
radius and that loading a save did not reliably repair Synthesis Studio.

This is one systemic persistence omission, not three render bugs. Commit `946a6240`
(`Implement Substrate Split Serialization`, 2026-09-01) deliberately removed
`transform`, `center`, `authoritativeAxis`, `targetRotation`, and
`rotationResponsiveness` from Object semantic JSON and placed them only in the
conglomerate `.ecmatter` writer. Per-Zone identities never gained their own matter
generation. Their Object reader still accepts these JSON keys, but the writer emits none.

Read-only inspection of the authored files confirms the exact visible result:

- `Sanctum of Beginnings/zone.json`: 129 Objects; 129 missing transforms.
- `SynthesisStudio/zone.json`: 193 Objects; 193 missing transforms.
- `SynthesisStudio.LivingInstrument/zone.json`: 137 Objects; 137 missing transforms.
- `Chess/zone.json`: 39 Objects; 39 missing transforms.
- `studio.platform.floor` should carry scale/placement `14 × 0.2 × 14` at y = -0.1
  in `synthesis_studio.ecmatter`; without that record it becomes the default unit cube at
  the origin. `studio.console.desk` similarly loses `5.2 × 0.8 × 2.4` and occupies the
  same origin. Their overlap is the one short prism Zach sees.

The recent composite-identity/refuse-on-ambiguity fix exposed this older loss rather than
causing it. Old Synthesis sidecars have no `owner_identifier`, while
`SynthesisStudio` and `SynthesisStudio.LivingInstrument` share dozens of `studio.*` and
`hud.*` identifiers. The safe loader now refuses those ambiguous legacy records instead
of spraying one Zone's transform into whichever same-named Object happens to win. Before
that refusal, wrong last-writer restoration could mask the transformless identities.

Correct placement still exists in legacy matter sources, including
`synthesis_studio.ecmatter`, `chess_app.ecmatter`, and Zach's
`random syntehsis studio stuff.ecmatter`; this is fragmented authority, not evidence that
the authored placement bytes are gone. The `SynthesisStudio` identity's 123 additional
Sphere identifiers beyond the 70-object base world are not to be deleted or called
corruption: they may be authored/spawned history, and their intended transforms require a
Person-chosen recovery source if competing sidecars disagree.

### Required repair boundary

1. Restore every Person-meaningful, Law-addressable pose field to semantic Object
   persistence. Transform/position/rotation were misclassified as “purely physical”; raw
   topology density may live in matter, but authored placement may not disappear from the
   semantic Zone record.
2. Give each Zone its own verified, content-addressed matter generation for genuinely
   physical/heavy geometry, and load it only through that Zone's activation transaction.
3. A legacy Zone Object missing placement must never silently receive the identity
   transform. Mark the Zone incomplete and identify recoverable candidate sidecars.
4. Compatibility recovery may resolve an ownerless record inside the one requested Zone
   when unique there; it must not search every live Zone and must refuse competing values.
   Choosing among genuinely different authored sidecars is a Person decision.
5. Add round-trip coverage proving semantic pose survives Zone identity alone, plus real
   activation fixtures for Sanctum, Synthesis Studio, and Chess. The persistence-coverage
   audit must fail if any registered pose field again has no semantic writer.

No save file was modified during this diagnosis.

### Repair boundary item 1 — fixed (Claude Sonnet 5, session `01MsayKP3NYfQAyBtyQ8xeA1`, 2026-09-09)

`Object::to_json` (`ObjectSerialization.cpp`) now writes `transform`, `center`,
`authoritativeAxis`, `targetRotation`, and `rotationResponsiveness` again —
`from_json` never stopped reading them, so this closes the gap directly at its
source rather than routing around it. Matter (`buildMatterFlatBuffer`) still also
writes transform for legacy/heavy-geometry compatibility; the two are redundant
by design where both exist, and semantic JSON is now the field of record whether
or not a matter generation exists for a given Zone.

New `tests/zones/object_semantic_pose_test.cpp`, 12/12 green:
- a direct `to_json`/`from_json` round-trip proves all five fields survive;
- the actual failing shape — a Zone identity written via `persistZones()` with
  NO World and NO matter sidecar at all, then hydrated through
  `ZoneManager::hydrateFromZoneStore()` (the real boot path, not `loadState`) —
  proves a `studio.platform.floor`-shaped object (14×0.2×14 at y=-0.1, the exact
  real dimensions Sol named) keeps its authored position and scale from the Zone
  identity alone, where before this fix it silently became a unit cube at the
  origin.

### Repair boundary item 5 — fixed (Jules, Gemini 3.1 Pro, session `jules-16649574473755973133-4583545e`, 2026-09-10)

Registered `authoritativeAxis`, `targetRotation`, and `rotationResponsiveness` in `Object::buildProperties()` (`src/ConstructedBeing/Singular/Object/Object/ObjectProperties.cpp`), alongside existing registered pose properties `position`, `rotation`, `transform`, and `center`. Built mechanical persistence-coverage guard test `tests/zones/object_pose_serialization_guard_test.cpp` (38/38 green):
- dynamically inspects `Object::listProperties()` for registered pose fields (`position`, `rotation`, `transform`, `center`, `authoritativeAxis`, `targetRotation`, `rotationResponsiveness`);
- sets non-default values on all pose fields (translation, rotation, scale, non-default axis `(0,0,1)`, target rotation `(45,30,15)`, responsiveness `8.5f`);
- exercises real `to_json`/`from_json` behavior and the real Zone identity boot path (`ZoneManager::persistZones()` writing `zone.json` to a temporary `SaveRoot` with NO World and NO matter sidecar, then `ZoneManager::hydrateFromZoneStore()`);
- mechanically fails if any registered pose property lacks semantic Object serialization coverage or fails to round-trip through the Zone boot path.

Also updated `no_black_box_test` `kWriteExemptions` for `authoritativeAxis` normalization behavior (309/309 green).

Items 2-4 of the repair boundary (Zone-scoped matter generations, refuse-not-default on missing legacy placement, Zone-scoped ownerless-matter recovery with Person-choice on conflict) remain for future passes.

### Synthesis Studio authored-pose recovery — 2026-09-11

Zach reported the remaining lived failure directly: every 3D form in the
Synthesis Studio appeared as a cube except the orbiting sphere ecology. That
description identified `SynthesisStudio.LivingInstrument` in particular: it
contains the `studio.living.satellite.*` beings. Codex traced this to an
important aftermath of the original omission rather than a failure of the new
writer: after the broken boot supplied identity transforms, a later
`persistZones()` faithfully serialized those already-collapsed live values.
The repair above prevents future loss but cannot infer the old values back into
an identity that now explicitly says “unit cube at the origin.”

At Zach's explicit request to fix the visible Studio, Codex performed a
field-scoped recovery from the corresponding authored `.ecform` roots:

- `SynthesisStudio`: 193 current Objects; all membership and 123 later
  target-only Sphere beings preserved; 70 source IDs matched with zero
  `shapeKind` mismatches; only `transform` and `center` changed on the 32
  matching 3D Objects.
- `SynthesisStudio.LivingInstrument`: all 137 source and target IDs matched
  one-to-one with zero `shapeKind` mismatches; only `transform` and `center`
  changed on its 67 3D Objects (29 Cube-topology rectangular forms and 38
  Sphere-topology resonators/satellites).
- The sources were `saves/worlds/synthesis_studio.ecform` and
  `saves/worlds/synthesis_studio_living.ecform`. Their furniture poses agree
  with `scripts/author_synthesis_studio.py` and the last correct historical
  Zone revision: floor `14×0.2×14 @ (0,-0.1,0)`, desk
  `5.2×0.8×2.4 @ (0,0.4,0)`, and easel `4.2×2.4×0.1 @ (0,2.2,2.4)`.
- Exact pre-repair bytes remain at
  `saves/backups/SynthesisStudio.pose-recovery-2026-09-11/zone.before.json`
  and
  `saves/backups/SynthesisStudio.LivingInstrument.pose-recovery-2026-09-11/zone.before.json`.
  The recovery tool verifies expected source/target SHA-256 values, refuses
  duplicate IDs or shape mismatches, verifies its backup, and atomically
  replaces the target: `scripts/recover_zone_object_pose.py`.

Semantic before/after comparison proved Object membership unchanged and every
non-`transform`/`center` field unchanged. The two existing executable guards,
`object_semantic_pose_test` and `object_pose_serialization_guard_test`, passed
2/2 after recovery. Desktop manifestation remains a Person witness: fully quit
the already-running broken instance **without saving its stale live Zone**,
relaunch, and Move to each Studio Zone from Creator Console → Zones.

Origin: Zach's live report and explicit repair request. Diagnosis, preservation
tool, and field-scoped recovery: Codex (GPT-5), session
`01a0707e-f743-71b1-8fb9-63975012e66d`, 2026-09-11 10:29 PDT.

## Law closure first rung — 2026-09-10

**Implemented by Codex (GPT-5), session
`01a07d15-f266-7902-bc11-cf7b06b0b343`, 2026-09-10 19:36:53 PDT.** This
directly follows Zach's correction above: Creator Console → Zones → Move to Zone must
make the Zone live without a legacy World load. The diagnosis and boundary also draw on
every unread message in the agent-intercom threads **Basic Pixel Changer Zone Identity
Bug 9-7-26** and **Law Engine Rungs 0-1 9-9-26**, especially Sonnet's shared-root
proposal and Sol's preflight-before-mutation invariant.

The Basic Pixel Changer exposed the missing rung exactly: its Zone identity booted the
white canvas and Material, but its only authored Law still lived under the legacy
World's `authoredLaws`. The old automated test called `loadState`, so it proved that old
path and could not see the lived failure Zach found.

This pass adds one stable shared Law root at
`saves/laws/law-basic-pixel-changer/law.json` and makes
`saves/zones/BasicPixelChanger/zone.json` name it through `lawRefs`. Zone activation now:

- reads and validates every named Law root before changing the current Zone;
- verifies root/document identifiers, recorded Person authors, targets, and triggers;
- refuses missing, malformed, ambiguous, or colliding dependencies with the old Zone and
  Law register unchanged;
- registers and binds the closure before `zone-loaded` / `zone-entered`; and
- releases Zone-scoped Laws when the Person leaves, while preserving engine-owned First
  Movers and unrelated global Laws.

Ordinary Zone persistence preserves authored `lawRefs` and writes the referenced shared
Law root rather than embedding a copy in the Zone. Shared-Law writes use an adjacent
temporary file plus atomic rename. On a legacy installation with exactly one local,
non-cryptographic Person profile, boot restores that unambiguous profile so the saved
author resolves as the actual Person; multiple profiles are refused rather than guessed,
and a file-claimed cryptographic identity is not auto-trusted.

`basic_pixel_changer_test` now uses only Zone/Law identities—no World loader—and passes
24 checks covering the real click, addressed texel isolation, individual pixel Property
elevation, OntoMath-defined region Property elevation, missing-root atomic refusal, and
Law persistence/re-entry. The full project and `earthcall_webgpu` compile; seven adjacent
Zone/save tests and `save_system_error_test` pass. The GLFW-based
`law_persistence_test` and `no_black_box_test` could not be completed in this
non-interactive run because macOS application services hung during initialization; both
were stopped, not reported as green.

This is deliberately **not** the completion of the reopened task. General Material and
Category shared roots, Zone-scoped matter generations, a detached whole-closure
transaction across every root kind, Save Zone isolation, and migration of the other
authored app Laws remain open under the completion contract above.

## 2026-09-18 — `chess_app` → independent Chess Zone boot

**Authorized by Zach. Implemented by GPT-5.6 Sol.** Zach explicitly asked to migrate the Chess Zone whose legacy source is exactly `saves/worlds/chess_app.json` so Chess no longer requires loading the conglomerate World.

The migration keeps the historical `chess_app.{json,ecform}` files untouched as compatibility/recovery artifacts and changes the ordinary source of truth instead:

- `saves/zones/Chess/zone.json` keeps the newer serialized forms of the 39 board/piece/seat/promotion beings, restores the legacy bundle's complete persisted Relation graph, embeds the three Chess materials, and admits the formerly session-only extra-spatial dependency beings (Chess/category roots, `state.chess`, `object.chess.status`, `grok-4.6`, and `codex-gpt5`) exactly once.
- The Zone names all 69 authored Chess Laws through `lawRefs`; each existing Law is copied unchanged from the legacy authored register into its own stable `saves/laws/<id>/law.json` root with its original `authors` and trigger set. The migration records GPT-5.6 Sol only in `injected_by`; it does **not** rewrite grok-4.6's or Codex's authorship.
- `ZoneManager::switchTo` now repeats the idempotent persisted-Relation hydration pass immediately after the referenced Laws are committed. This is the Zone-native counterpart of the second pass legacy `loadState` already needed: Law→category edges cannot bind before those Law beings exist.
- `scripts/author_chess.py` now emits the Zone identity + shared Law roots by default. Regenerating the legacy `chess_app` session requires explicit `--legacy-session`, preventing routine authoring from sliding Chess back behind a conglomerate-file dependency.
- `tests/law/chess_zone_native_boot_test.cpp` constructs an isolated SaveRoot with **only** the Chess Zone and its named Law roots, never creates a `worlds/` directory, never calls `loadState`, moves to Chess through `switchTo`, verifies materials/relations/all 69 Laws, and executes e2-e4.

Person-facing rendering/input acceptance is routed to [For Zach/Person Verification List.md](../../../For%20Zach/Person%20Verification%20List.md); automated closure verification is not treated as a substitute for Zach seeing and playing the Zone in the real app.

### 2026-09-18 Person witness correction — PR #222 booted as two cubes

Zach merged PR #222, pulled it, fresh-booted Earthcall, and entered Chess through the intended Zone-only path. The result was **a white cube sitting on top of a black cube**, not a chessboard.

The failure was in the migration data, not the new Law-root architecture. PR #222 deliberately preferred the then-current 39 gameplay Object payloads in `saves/zones/Chess/zone.json` over the legacy `chess_app` copies under the assumption that the Zone identity was the newer source. That assumption was wrong. Those 39 Zone payloads were descendants of the old transform-loss bug documented above: their transforms and centers had already collapsed to identity/origin, their 3D `shapeParams` had also picked up default 2D dimensions, and their fallback `faceColors` had reverted to legacy cube colors. The exact `saves/worlds/chess_app.json` source still carries the intended authored manifestation — board scale `8 × 0.28 × 8 @ y=-0.14`, e2 pawn at `(0.5, 0.22, -2.5)`, and 32 distinct piece placements.

The hotfix restores the exact 39 board/piece/seat/HUD Object payloads from `chess_app` into the independent Zone identity while preserving the Zone-native closure added by PR #222: the additional dependency beings, three materials, 143 relations, and 69 `lawRefs`. The cold-boot regression is strengthened to assert manifestation before behavior: board scale/placement, e2 start position, 32 pieces present, and 32 distinct piece positions. `scripts/author_chess.py` now refuses to emit a collapsed native Chess Zone if those invariants fail.

This is a direct correction of my PR #222 migration judgment: calling the older Zone payloads “newer serialized forms” confused chronological recency with semantic authority. Zach's live visual witness exposed what the previous headless test did not.

## 2026-09-19 — `go_app` → independent Go Zone boot

**Authorized by Zach. Implemented by Gemini Spark.** Zach explicitly requested to make sure the Go zone is working in Earthcall and migrate the Go zone's files from the legacy conglomerates (both `.ecform` and `.ecmatter`) so Go no longer requires loading `saves/worlds/go_app.{json,ecform,ecmatter}`.

The migration establishes the Zone-native closure for Go:
- `saves/zones/Go/zone.json` carries all 385 beings (1 board, 361 intersections, 2 bowls, 10 supply stones, 2 player seats, 2 state beings `go_state` and `state.go`, plus the 7 category and First-Mover referent beings including `grok-4.6`), embeds the 5 face-textured Materials (`material.go.board` with its 19×19 Kaya wood grid and 9 star points, `go.black`, `go.white`, `go.bowl`, `go.intersection`), and names the 3 gameplay Laws via `lawRefs`.
- The 3 authored Go Laws (`law-go-click`, `law-go-place-black`, `law-go-place-white`) are copied into their own stable `saves/laws/<id>/law.json` shared roots with their original author (`grok-4.6` on Zach's authority) and `object-clicked` triggers.
- Scoped physical matter (`SaveChunk` `matter_go`) is built with `owner_identifier: "Go"` for all 376 physical entities, compiled to FlatBuffers `.ecmatter`, hashed (`snapshotId: 6e87a7ddd6ea386c`), and stored as `saves/zones/Go/zone.ecmatter` and `saves/zones/Go/zone.6e87a7ddd6ea386c.ecmatter`.
- The duplicate name-twin directory `saves/zones/Go Game/` is retired and removed, permanently resolving the live collision where `applyMatterFlatBuffer` reported `entity 'object.go.board' matches 2 live objects across Zones (Go, Go Game)` and refused to hydrate matter.
- `scripts/author_go.py` now emits the complete Zone-native Go closure by default and refreshes the legacy compatibility artifacts (`saves/worlds/go_app.{json,ecform,ecmatter}`).
- `tests/law/go_zone_native_boot_test.cpp` constructs an isolated SaveRoot containing **only** `saves/zones/Go/` and `saves/laws/law-go-*/`, with zero `worlds/` directory and no `loadState()`. It proves boot discovery, Move to Zone activation, material hydration, 361 intersection placement, and real gameplay execution (Tengen click places black stone, toggles turn to white; next click places white stone, toggles back to black). 39/39 checks green.

## 2026-10-07 — Material closure, shared Material roots, Save Zone isolation

**Implemented by Claude Code (Claude Sonnet 5.5), session `01GxayCUN2nc7DDaeg33kXhZ`, 2026-10-07.**
Zach asked "how far along is the serialization migration from the conglomerate world file to
Zone-centered saving … is there anything subtle left?" and then "plz do the rest of it".
The completion contract and proof list above are Zach's (2026-09-09 correction); Sol's
shared-root / preflight-before-mutation invariants (2026-09-10) are the pattern I extended from
Laws to Materials. What I originated: the Material closure rule, the `materialRefs` root
layout, the corpus guard, and the findings below. Coordinated with Codex / GPT-6.1 Sol (working
concurrently in Screen / Law Line files) through
`agent intercom/communication-threads/saves-and-zones/Zone_Native_Closure_Rungs_2026-10-07.md`.

### What the audit found (the "subtle" part)

1. **Nine Zones named Materials their own identity never defined.** Objects in FarLands,
   SynthesisStudio (18), SynthesisStudio.LivingInstrument (20), Borealis Sanctuary,
   Sanctuary of Sunlit Mist, Northern Veil, Luna's Moon Robot (5) and Neural Network v2 (6)
   resolved their Materials only because some *other* Zone, or a legacy world file, had left
   them in the live register — or, when nothing had, they rendered through `material.default`
   (white) with no error. This is the silent class: it worked on Zach's machine by accident of
   load order.
2. **Prism Cathedral's 13 Materials were registered under the wrong names.** The generator
   wrote the identity under `"id": "material.cathedral_basalt"` and a display label under
   `"name"`; `Material::fromJson` read only `name`, so the Materials were registered as
   "Cathedral Basalt Floor" and every Object naming `material.cathedral_basalt` resolved to
   nothing. Fixed in the loader (an explicit `"id"` is the identity), no save was edited.
3. **11 Materials had no authored definition anywhere** (all of Luna's, all of Neural Network
   v2's). Nothing existed to recover.

### What changed

- **`ZoneManager::prepareZoneMaterialClosure`** runs beside the Law preflight in both
  `switchTo` and `holdZoneClosure`, before any live state changes. Every `materialId` in the
  stored identity must resolve from the identity's embedded `materials`, a shared root named
  by `materialRefs`, or `default`. A miss — or a missing/mismatched root, or a duplicate ref —
  refuses loudly and the current Zone stays. Proof item 4 (Material half).
- **Shared Material roots**: `saves/materials/<stem>/material.json`
  (`SaveSystem::{write,read,exists,path}MaterialIdentity`), named by a Zone's `materialRefs`,
  exactly as `lawRefs` names a Law. Proof item 5 (Material half). An old embedded copy beside a
  ref is tolerated residue; the first Save Zone strips it.
- **Save Zone** now writes every shared root the Zone names (Materials and Laws) *before* the
  Zone identity, skips any root whose content is unchanged (byte-stable), and refuses if a
  named Material is no longer live. A crash leaves the old Zone pointing at roots that all exist.
  Proof item 3's "unrelated Zone files remain byte-identical" is now a test.
- **Data migration** `scripts/migrate_zone_materials.py` (dry-run by default). Patch, never
  regenerate: one key inserted at text level into each `zone.ecform` and `zone.json`, staged,
  verified equal to *old document + that key*, old bytes kept in
  `saves/backups/zone-material-refs-2026-10-07/`, then atomic rename. 42 shared roots written;
  31 recovered from `saves/worlds/*.json` and Zone identities. The 11 with no authored definition
  were declared as **engine-default roots tagged `injected_by.note`** — exactly what those Objects
  already rendered with, so nothing is invented and nothing changes on screen; they are now
  visible, repaintable beings instead of a dangling name. Authors recorded: `Zach` (authority);
  `injected_by`: this session.
- **Tests**: `tests/zones/zone_material_closure_test.cpp` (23 checks: shared root, dangling
  refusal, deleted-root refusal, Save Zone isolation, root rewrite on change, `id` loader) and
  `tests/zones/zone_material_corpus_test.cpp` (walks all 46 committed Zones against the same
  rule; negative-controlled by restoring FarLands' pre-migration file). The corpus test is in
  the source-root-CWD list in `CMakeLists.txt` — under ctest's default build-tree CWD it
  silently read a stale 4-Zone `build/saves/` and passed vacuously before that fix.

### Still open (honest list)

- **Zone-scoped matter generations**: only Go and Cathedral have a `zone.ecmatter`; Chess and
  most other physical Zones still source matter from `saves/worlds/`. Repair-boundary items 2–4.
- **Category roots** (shared Categories) — no `saves/categories/`; unchanged.
- **A truly detached whole-closure transaction**: ordering is now dependencies-first, but it is
  not all-or-nothing across files.
- **`earthcall_save_world` (MCP, `WebSocketServer.cpp`) still calls `saveStateWithLog`**, i.e. writes
  a conglomerate session. The Assets console already demoted session export to a collapsed
  "Legacy Session Management" header. Changing the MCP tool is a Foreign-actuation decision, left for Zach.
- **Already-embedded duplicate Materials** across Zones (e.g. `object-757` in Neural Network and
  new slate; Prism's `cathedral_basalt` beside its new root) are not deduplicated.
- **Material fields the loader drops**: `roughness` / `metalness` appear in authored Material
  JSON (the Prism set) but `Material::fromJson` never reads them — a No-Black-Box (Refusal 6)
  gap, not addressed here.
- **Person witness**: enter FarLands, SynthesisStudio, Luna's Moon Robot and Neural Network v2
  in the real app (see For Zach/Person Verification List.md).

