# Authored-save lint probe (31c)

**Status:** ✅ done and verified  
**Section in the To-Do list:** First Movers  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Authored-save lint probe (31c)** — **done and verified (2026-08-26)**: Implemented `scratch/probes/authored_save_lint_probe.py` auditing all active save files in `saves/` for (1) past-tense `noun-verbed` kebab-case event naming rules, (2) volatile generated identifiers (`law-N`, `object-N` in law definitions and `@law-N` targets), and (3) orphan trigger event listeners without engine or ECA cascade publishers. Scanned 29 active saves (95 laws analyzed across 17 worlds/fixtures/test saves), successfully flagging legacy test saves (`basic_cube_law_test.json`, `shape_generator_3d_law.json`) using `onMouseClicked` / `law-N` while confirming `saves/worlds/chess.json`, `chess_app.json`, `chess_first_mover.json`, and `category_seed.json` are 100% clean. (Ref: [The Fifth Domain Arrived Sideways §4](../../../../Reflections%20on%20Earthcall%27s%20Progression/Reflections%20on%20the%20Substrate/The_Fifth_Domain_Arrived_Sideways.md)).
