# In-world test observation feature

**Status:** open  
**Section in the To-Do list:** Feature-sized (split out of Housekeeping 2026-08-13):  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**In-world test observation feature** — PARTIAL (2026-08-19). **Done and verified (headless live path, not a Person click):** loading a test dump no longer calls `loadState` (that erased Home) and no longer leaves `Person.position` behind the camera (`LocomotionChannel` then snapped the view back, so load looked like a no-op). Grave / Toggle Dev Mode → Observe puts the dump into an isolated Zone `test.<stem>`, aims the Person at the cluster, and reports loudly. Home survives. Guarded by `tests/zones/test_observation_load_test.cpp` (18/18): synthetic cube plus, when present, the real `saves/tests/basic_cube_law_test_final.json` (6 objects in the active world). `earthcall` rebuilds. **Remaining:** run the *suite* in the live engine so tests create/modify beings on screen as they execute (Zach's original intention — witnessing the work, not only the dump). A Person still has to open Grave and click Observe to confirm it feels right. See [The Walk Writes Back §3](../../Reflections%20on%20Earthcall%27s%20Progression/Reflections%20on%20Trajectory/The_Walk_Writes_Back.md).
