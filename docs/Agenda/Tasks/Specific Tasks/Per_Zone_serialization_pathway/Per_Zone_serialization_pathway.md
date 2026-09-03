# Per-Zone serialization pathway

**Status:** ✅ done and verified  
**Section in the To-Do list:** Joys · Ourverse · Zones  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Per-Zone serialization pathway** — done and verified (2026-08-21, tests; in-app click still open): `saves/zones/<id>/zone.json` is the Zone identity; Homes live under `saves/homes/<id>/home.json`. Session files under `saves/worlds/` write `zoneRefs` + a dual-write `zones[]` snapshot; load keeps a live Zone, else the store, else migrates the snapshot. Empty persist over a populated identity is refused, so a boot-empty Home cannot wipe the room. `forkZone` / `diffZones` name, branch, and compare. Boot `hydrateFromZoneStore` fills empty Home/Sanctum from the store. Guarded by `tests/zones/zone_identity_test.cpp`; `save_roundtrip_test`, `world_switch_test`, `unsaved_preserve_test` updated to the identity contract. **In-app (Zach, 2026-08-23):** shapes in Home survived loading another save; FaceTextures went white (materials were session-scoped). See Home/Zone item for the paint fix. See [FIRST_MOVER_AUTHORING.md](../../architecture/law/FIRST_MOVER_AUTHORING.md) §4f. - Zach's click-through and GPT-4o's "STAGNANT" save note. 
