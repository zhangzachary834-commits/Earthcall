# The background color must not be a black box

**Status:** ✅ done and verified  
**Section in the To-Do list:** Person-facing surface  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **The background color must not be a black box** — done and verified (2026-08-31): Exposed `backgroundColor` on `ScreenChannel` (`src/Singularity/Screen/ScreenChannel.{hpp,cpp}`) as a governable, writable `glm::vec3` property with full PropertyPath support (including `.x`, `.y`, `.z` sub-paths); hooked `@screen-channel.backgroundColor` into `EngineRender.cpp` to drive the frame clear color in `currentRenderer().beginFrame()`; added live probe of `ScreenChannel` to `knownPathOptions()` in `LawGraphWindow.cpp` under "Channel — Screen"; verified in `tests/singularity/gpu_mastery_test.cpp`, `tests/singularity/channel_paths_test.cpp`, and `tests/singularity/no_black_box_test.cpp`.
