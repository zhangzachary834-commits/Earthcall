# Retire `World` into `Zone`

**Status:** ✅ done and verified  
**Section in the To-Do list:** Near-term priorities (2026-08-14 — from architecture review):  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Retire `World` into `Zone`** — done and verified (2026-08-20): `class World` deleted; objects, add/remove/update live on `Zone`; Spawn/Create/reaper take Zone; `BeingKind::World = 6` burned (never matches, picker does not offer it); save still writes/loads `zones[].world.objects`; `LawContext.hpp` (duplicate of Universe) deleted; Universe stays a non-`Singular` kernel. Did not populate World properties. Did not register `Ourverse::ownedObjects`. Verified: reconfigure + `cmake --build build -j8`; 18 tests including `save_roundtrip_test` (21/21, zone world JSON keeps three spawns), `ground_plane_test`, `action_spawn_test`, `no_black_box_test` (211 writes / 0 fail, World off the sealed ledger), `shape_generator_law_test`, `world_switch_test`, `test_observation_load_test`. In-app load of `my_world` not clicked this session. **Remaining (Ourverse half):** Engine bag on Ourverse — 2026-08-19 audit. See [WORLD_UNIVERSE_REFUSALS_AUDIT_2026-08-20.md](../../../../audits/WORLD_UNIVERSE_REFUSALS_AUDIT_2026-08-20.md). 
