# Volatile object identifier spam reduction & stable ID assignment (Structural Debt §5)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Near-term priorities (2026-08-14 — from architecture review):  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Volatile object identifier spam reduction & stable ID assignment (Structural Debt §5)** — done and verified (2026-08-25): Assigned stable IDs directly at construction time during deserialization in `Serialization.cpp` (`zoneObjectsFromJson`), in `CategoryManager::create` and `loadFromJson`, in `EngineInit` baseline objects, and in `BodyPart` and sub-objects; added verbose logging flag and atomic volatile tracking in `ObjectIdentity.hpp`; aggregated volatile ID warnings into a concise summary on save load in `ZoneManager`. Spurious multi-hundred-line terminal spam eliminated. Verified: full build, ctest 65/65 passed (100%), lag probe verified (0 broken invariants, 0 regressions). (Ref: `docs/Agenda/Tasks/Specific Tasks/Week_Of_2026-08-24_Structural_Debt.md` §5).
