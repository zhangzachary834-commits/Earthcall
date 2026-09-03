# Structural debt from the week of 2026-08-17 → 08-24

**Status:** open  
**Section in the To-Do list:** Housekeeping:  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**Structural debt from the week of 2026-08-17 → 08-24** — eight small, independent items found in a weekly review, each verified against the tree: a `Singular` copy/move slicing fix that shipped unguarded and mis-labelled under "chess lag" (`ce5c1cbe`); a possible new hot loop in `maybeStartDriveSession`; `src/Time/` as a top-level directory holding an empty placeholder `class Time {}` (Refusal #2, and absent from `CLAUDE.md`'s tree) with its four files pasted into `IMGUI_SOURCES` at `CMakeLists.txt:145-148`; a **tracked symlink** `src/ConstructedBeing/Singular/Object/Formation -> ../../../Relation/Formation` holding up 21 stale includes after the Formation move; ~480 volatile object identifiers on a chess load; seven near-duplicate chess generator scripts across `scripts/` and `scratch/`; three near-identical `docs/` reflection trees; and 37 MB of unreviewable save blobs in git. Also names a recurring **pointer-plus-shadow-string** pattern (`Relation::_savedA`, `Home::_stakeIds`, `Home::_inhabitants`) that wants one type with one invariant. Full list with file:line and ⚑ AUTHOR calls marked: [Specific Tasks/Week_Of_2026-08-24_Structural_Debt.md](Specific%20Tasks/Week_Of_2026-08-24_Structural_Debt.md).
