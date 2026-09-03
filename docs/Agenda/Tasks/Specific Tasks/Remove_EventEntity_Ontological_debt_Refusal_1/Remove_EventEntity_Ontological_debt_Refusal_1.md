# Remove `EventEntity` (Ontological debt / Refusal #1)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Near-term priorities (2026-08-14 — from architecture review):  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Remove `EventEntity` (Ontological debt / Refusal #1)** — done and verified (2026-08-24): Deleted `src/Singularity/Core/EventEntity.hpp` and `.cpp`; refactored `EventBus` custom events (`Event::Custom`) to carry `std::shared_ptr<Relation> relation` with semantic type and endpoint bindings; removed `_activeCustomEvents` from `LawManager` and removed active event bookkeeping (`addActiveEvent`/`removeActiveEvent`) from `Universe`; updated Law audit logging and Rete assertion to bind from `e.relation->a()` / `e.relation->b()`; updated `no_black_box_test.cpp` to remove `EventEntity` probe. Verified: full clean build + ctest 65/65 passed (100%), lag baseline probe verified (0 timing regressions, 0 broken invariants). (Ref: `scratch/audits/audit_report_2026-08-13.md`).
