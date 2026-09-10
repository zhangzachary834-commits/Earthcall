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
