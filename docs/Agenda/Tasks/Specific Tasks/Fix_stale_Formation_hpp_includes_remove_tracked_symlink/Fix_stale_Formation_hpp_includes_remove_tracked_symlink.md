# Fix stale `Formation.hpp` includes & remove tracked symlink (Structural Debt §4 & §3)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Near-term priorities (2026-08-14 — from architecture review):  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Fix stale `Formation.hpp` includes & remove tracked symlink (Structural Debt §4 & §3)** — done and verified (2026-08-25): Removed tracked git symlink `src/ConstructedBeing/Singular/Object/Formation -> Relation/Formation`; rewrote all includes across 24+ files in `src/`, `tests/`, and `scratch/` to `#include "Relation/Formation/Formation.hpp"` and canonical Menu headers; cleaned `IMGUI_SOURCES` in `CMakeLists.txt` (kept `src/Time/` sources intact). Verified: full clean build + ctest 65/65 passed (100%), lag baseline probe verified (0 broken invariants). (Ref: `docs/Agenda/Tasks/Specific Tasks/Week_Of_2026-08-24_Structural_Debt.md` §4).
