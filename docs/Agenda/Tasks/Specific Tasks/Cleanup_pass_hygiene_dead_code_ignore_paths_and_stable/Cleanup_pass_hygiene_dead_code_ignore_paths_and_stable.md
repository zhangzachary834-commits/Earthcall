# Cleanup pass: hygiene, dead code, ignore paths, and stable physics law identifiers

**Status:** ✅ done and verified  
**Section in the To-Do list:** Housekeeping:  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Cleanup pass: hygiene, dead code, ignore paths, and stable physics law identifiers** — done and verified (2026-08-17): Fixed miniaudio paths in `.ignore` and `.claude/settings.json`; untracked `build.log`, `.DS_Store`, `imgui.ini`, and `saves/tests/*_final.json`; deleted duplicate audit report and duplicate fixture save; removed dead `Core::Engine*` parameter from CreatorConsole; removed unused `shapeKindLabel`, `kDevToolShapeKinds`, and includes from `DeveloperToolsWindow.cpp`; removed dead `Tool currentTool` local in `tests/test_save_helper.hpp`; updated docs test count to 48 registered, 47 pass and To-do list path; switched `setObjectID` to `setLawIdentifier` in `DefaultPhysicsLaws.cpp`; full build and test suite verified (48 registered, 47 pass).
