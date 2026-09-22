# Organize `tests/` by directory

**Status:** ✅ done and verified  
**Section in the To-Do list:** Housekeeping:  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Organize `tests/` by directory** — done and verified (2026-08-18): Grouped the flat suite into `constructed-being/`, `person/`, `zones/`, `law/`, `singularity/`, `identity/`, and `support/`; added `tests/README.md`. CMake already `GLOB_RECURSE`s; ctest names remain file stems. Helper header moved to `tests/support/` with that path on the include line. Reconfigured; built and ran one test from each region plus the helper-using law test (`basic_cube_law_test`, `paint_test`, `first_mover_test`, `ourverse_test`, `identity_test`, `channel_paths_test`, `ground_plane_test`, `no_black_box_test`, `control_patterns_test`).
