# Fully realize the Ourverse vision

**Status:** open  
**Section in the To-Do list:** Joys · Ourverse · Zones  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**Fully realize the Ourverse vision.** **First rung (2026-08-18):** vessel of unity — unowned gathering Zone, mutual filaments, shared Joys, first-mover metalaws, `convenesToward` empty. See [docs/architecture/ourverse/OURVERSE.md](../../../../architecture/ourverse/OURVERSE.md). **Second rung — Engine object bag elimination (done and verified 2026-08-25):** Retired dead Game object list (`ownedObjects`, `addOwnedObject`, `getOwnedObjects`), dead physics on Ourverse (`onUpdate`, `updateObjectCollisions`, `clearDynamicObjects`), unused `setCamera`/`cameraPos`, and `struct InteractionEvent` from `Ourverse.hpp`/`Ourverse.cpp`; removed dead baseline object insertion in `EngineInit.cpp`. (Ref: [docs/audits/OURVERSE_GAME_ELIMINATION_AUDIT_2026-08-19.md](../../../../audits/OURVERSE_GAME_ELIMINATION_AUDIT_2026-08-19.md)). **Third rung — World-state decoupling & dead husk deletion (done and verified 2026-09-10):** Deleted empty Game husks `OurverseUI.cpp`, `OurverseNodeGraph.cpp`, and `OurverseSaveLoad.cpp` from `src/ZonesOfEarth/Ourverse/`; stripped all unused includes (`imgui`, `glm`, `Object`, `unordered_map/set`, dead `extern ZoneManager mgr`) from `Ourverse.hpp`/`.cpp`; renamed `Engine::_world` to `_ourverse` and added `Engine::getOurverse()`. Remaining: populate ecumenical layer; Community auto-gathering at birth.
