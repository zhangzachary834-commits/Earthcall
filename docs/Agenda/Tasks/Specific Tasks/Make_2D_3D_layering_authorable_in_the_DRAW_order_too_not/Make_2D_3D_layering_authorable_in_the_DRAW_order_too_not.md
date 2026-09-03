# ⚑ Make 2D/3D layering authorable in the DRAW order too, not just the pick

**Status:** open  
**Section in the To-Do list:** Performance (opened 2026-08-24 by `tests/singularity/frame_lag_test.cpp`)  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**⚑ Make 2D/3D layering authorable in the DRAW order too, not just the pick.** Done for picking on 2026-09-02: `Object::pickPriority()` is a readable/writable property (default = `zOrder2D` for a 2D being, 0 for a 3D one, which reproduces the old hardcoded rule exactly), and `InteractionChannel::observe` now compares numbers instead of always letting a screen-space being win. Zach's framing, and it is Refusal 7: *"the 2d buttons get hidden behind 3d stuff; that should be an authorable property (overlap detection, response laws) not hardcoded layering limitations."* **Still hardcoded: `EngineRender::render` draws every 2D being after the whole 3D pass, unconditionally.** A Person cannot author a 3D being that draws in front of the HUD. Doing it properly probably means one ordered pass keyed on the same `pickPriority`, plus the "response laws" half Zach named — an authored law that fires when two beings overlap on screen, which needs an `Overlaps`-style condition in screen space. — Claude Opus 5, 2026-09-02
