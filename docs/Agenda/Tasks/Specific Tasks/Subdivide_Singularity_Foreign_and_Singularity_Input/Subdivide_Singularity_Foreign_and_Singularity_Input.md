# Subdivide `Singularity/Foreign/` and `Singularity/Input/`

**Status:** ✅ done and verified  
**Section in the To-Do list:** Housekeeping:  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Subdivide `Singularity/Foreign/` and `Singularity/Input/`** — done and verified (2026-08-20): Foreign keeps `ForeignChannel` at the modality root (`PhysicalChannel` pattern); supporting files go in `Adapters/` (already), `API/` (`EarthcallAPI`, `SecurityManager`), `Web/` (webview/overlay + `web_ui`), `Sync/` (logger, sync-back, inference bridge); `py/` stays as the language leaf. Input splits by seam: `Keyboard/`, `Mouse/`, `Locomotion/`, `Interaction/` (`InteractionChannel` + `ControlPatterns`). Includes, CMake OBJCXX/WASM filters, and living architecture paths updated. Python launcher path unchanged (`Foreign/py/app.py`). Also retargeted `Singularity/FirstMoverWindowTools/` includes to `Singularity/FirstMoverOntology/FirstMoverWindowTools/` so the in-progress First Mover move on this branch would compile. Verified: reconfigured; `earthcall` builds; `foreign_integration_test`, `interaction_channel_test`, `control_patterns_test`, `channel_paths_test`, `no_black_box_test` pass.
