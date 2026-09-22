# Authored properties on a base Singular

**Status:** open  
**Section in the To-Do list:** Singular · Relation · Formation  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**Authored properties on a base Singular** — the innate slot exists, ungated, on `Singular` itself (arbitrary count, every `PropertyValue` kind at runtime, `AddProperty` does not ask TransferPolicy). Not yet the manifesto: only Object/ObjectConcept persist grants; `listProperties` misses un-looked-up names; revoke-after-lookup leaves a stale bridge that blocks re-grant; ~~Singular copy drops the map~~ **fixed and guarded (2026-08-24, Bugs.md #8)** — `tests/constructed-being/singular_copy_move_test.cpp`; `DataStructure::writeBounds` is dead; Law Graph AddProperty is double-only. Probe 59/59. See [SINGULAR_AUTHORED_PROPERTIES_AUDIT_2026-08-23.md](../../../../audits/SINGULAR_AUTHORED_PROPERTIES_AUDIT_2026-08-23.md).
