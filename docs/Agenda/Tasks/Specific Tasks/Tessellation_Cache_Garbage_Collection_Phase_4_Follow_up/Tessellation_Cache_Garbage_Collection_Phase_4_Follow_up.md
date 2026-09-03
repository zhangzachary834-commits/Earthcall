# Tessellation Cache Garbage Collection (Phase 4 Follow-up)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Basic design before creating fully working products with Earthcall:  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Tessellation Cache Garbage Collection (Phase 4 Follow-up)** — **done and verified (2026-08-26)**: Implemented reference-counted garbage collection for `s_smoothCache` in `src/ConstructedBeing/Singular/Object/ObjectCollision.cpp` via `Object::gcSmoothTessellationCache()`, `smoothTessellationCacheSize()`, and `clearSmoothTessellationCache()`; evicts cached `TessMesh` entries with `use_count() <= 1` (where no live `Object` holds a reference); wired periodic eviction into `Engine::update` (`EngineUpdate.cpp`); guarded by `tests/constructed-being/smooth_tessellation_cache_test.cpp` (8/8 checks pass covering cache insertion, sharing across identical shapes, retention while referenced, and eviction upon object deletion or shape modification).
