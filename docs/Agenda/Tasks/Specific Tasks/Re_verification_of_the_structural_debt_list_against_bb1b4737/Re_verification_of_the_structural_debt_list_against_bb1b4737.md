# Re-verification of the structural-debt list against `bb1b4737`

**Status:** open  
**Section in the To-Do list:** Housekeeping:  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**Re-verification of the structural-debt list against `bb1b4737`** — (2026-08-24) all of the 08-24 audit's Priority-1 items are still standing verbatim (`Body : public Object` at `Body.hpp:13`; `EventEntity` with 6 live refs incl. `Law.hpp:737`; `Ourverse.hpp:92-93` `cameraPos`/`ownedObjects`; `src/Time/Duration/` empty and the four `src/Time/` files still in `IMGUI_SOURCES` at `CMakeLists.txt:144-148`; the tracked `Object/Formation` symlink with 21 stale includes). Newly found and **not** in that list: **6 broken relative links remain under `docs/Agenda/`** even though the three reflection trees were merged into one (§7 landed physically, unrecorded, links unfixed); `saves/worlds/chess.json` and `chess_app.json` are **byte-identical** (`md5 3616b980…`) and both tracked, so no world of record is identifiable; `/scratch/attic/earthcall_webgpu` is on `.gitignore:288` **and still an 18 MB blob in `HEAD`** (ignoring is not untracking); the ~480 volatile ids in §5 now measure **152** via `./build/chess_app_test`. Full reasoning: [Reflections on Repo State/What_The_Test_Suite_Can_See.md](../../Reflections%20on%20Earthcall%27s%20Progression/Reflections%20on%20Repo%20State/What_The_Test_Suite_Can_See.md).
