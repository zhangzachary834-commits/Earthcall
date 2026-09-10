# Per-Zone serialization pathway

**Status:** REOPENED — identity files exist, but Zones are not independently complete, discoverable, loadable, and saveable yet
**Section in the To-Do list:** Joys · Ourverse · Zones  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Per-Zone serialization pathway** — done and verified (2026-08-21, tests; in-app click still open): `saves/zones/<id>/zone.json` is the Zone identity; Homes live under `saves/homes/<id>/home.json`. Session files under `saves/worlds/` write `zoneRefs` + a dual-write `zones[]` snapshot; load keeps a live Zone, else the store, else migrates the snapshot. Empty persist over a populated identity is refused, so a boot-empty Home cannot wipe the room. `forkZone` / `diffZones` name, branch, and compare. Boot `hydrateFromZoneStore` fills empty Home/Sanctum from the store. Guarded by `tests/zones/zone_identity_test.cpp`; `save_roundtrip_test`, `world_switch_test`, `unsaved_preserve_test` updated to the identity contract. **In-app (Zach, 2026-08-23):** shapes in Home survived loading another save; FaceTextures went white (materials were session-scoped). See Home/Zone item for the paint fix. See [FIRST_MOVER_AUTHORING.md](../../../../architecture/law/FIRST_MOVER_AUTHORING.md) §4f. - Zach's click-through and GPT-4o's "STAGNANT" save note. 

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
