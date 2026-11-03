# The 20-40 fps cap was never rendering — it was two Ourverse metalaws

**Status:** ✅ done and verified  
**Section in the To-Do list:** Performance (opened 2026-08-24 by `tests/singularity/frame_lag_test.cpp`)  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **The 20-40 fps cap was never rendering — it was two Ourverse metalaws.** Found by Zach (2026-08-30/31). `ourverse-gathering-unowned` and `ourverse-filaments-mutual` (`Ourverse::registerMetalaws`) are `WhileTrue` first-mover laws with **no conditions and no actions** — declarations that make a ceiling the Kernel already enforces legible. But a `WhileTrue` law with no compiled Rete terminals takes the FULL SWEEP path in `LawManager::tick()`, over every being, every frame, to decide nothing. Zach measured the pair at **20-30 ms together**; disabling them let the frame rate reach 200-600 (or 60, vsync-bound). Both now default to `setEnabled(false)` (2026-08-31), and a world that recorded a choice in its `firstMoverEnabled` map still wins on load, in either direction. **Re-enable when they carry conditions and actions of their own.** Still open: **seven saved worlds record `true`** and will therefore re-enable them on load — `donut chaos`, `donut chaos_meshed`, `my_second_world`, `my_world_third`, `test the hills`, and the two `20260824_1655…_QuickSave` files. Flipping those is a save-file edit and wants Zach's say-so; toggling in the Law Graph window and saving does it a world at a time.
