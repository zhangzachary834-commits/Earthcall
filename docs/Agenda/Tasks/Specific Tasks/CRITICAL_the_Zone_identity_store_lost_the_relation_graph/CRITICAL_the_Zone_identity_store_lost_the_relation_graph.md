# CRITICAL — the Zone identity store lost the relation graph (2026-08-24)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Near-term priorities (2026-08-14 — from architecture review):  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **CRITICAL — the Zone identity store lost the relation graph (2026-08-24)** — done and verified (2026-08-24): Lifted `applyFormationRelations` (and `internZoneLexemes`) out of the empty-objects guard in `Serialization.cpp`; made relation loading idempotent by checking existing keys; updated `ZoneManager::loadState` / `admitFromJson` to merge session snapshot `zj` on store hits (`replaceObjects=false`); added guard of last resort in `ZoneManager::persistZones` refusing to overwrite non-empty stored relations/lexemes with empty arrays; re-authored `saves/zones/Chess/zone.json` so it holds all 38 formation relations authored by `grok-4.6`; added `tests/zones/zone_relation_roundtrip_test.cpp` guarding roundtrip, `Kind::Related` evaluation, and refusal against empty overwrite. Verified: `zone_relation_roundtrip_test` passes (10/10 checks); `chess_app_test` passes (green on HEAD); 63/64 active tests pass (only `webgpu_particle_test` pending in `PENDING_FEATURE_TESTS`). See [Specific Tasks/Zone_Relation_Graph_Loss.md](../../Specific%20Tasks/Zone_Relation_Graph_Loss.md) and [ZONE_RELATION_GRAPH_LOSS_AUDIT_2026-08-24.md](../../../../audits/ZONE_RELATION_GRAPH_LOSS_AUDIT_2026-08-24.md).
