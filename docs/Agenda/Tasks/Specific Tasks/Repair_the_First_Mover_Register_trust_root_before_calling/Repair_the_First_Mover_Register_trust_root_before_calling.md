# Repair the First Mover Register trust root before calling save-path authorization enforced

**Status:** open  
**Section in the To-Do list:** First Movers  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**Repair the First Mover Register trust root before calling save-path authorization enforced** — `evaluate` rejects a model grantor only when that grantor is present in the same serialized register and labeled `Model`; a valid signature from an absent grantor is never proven to terminate in a Person. Production has no `FirstMoverSession` or register save/load caller, so `SaveSystem` normally takes the no-session allow branch. Add the absent-grantor regression first, decide the trusted Person proof with Zach, then connect one real injection entry point. See [FIRST_MOVER_TRUST_AND_PROVENANCE_ANALYSIS_2026-08-20.md](../../../../Analysis/FIRST_MOVER_TRUST_AND_PROVENANCE_ANALYSIS_2026-08-20.md). **Do not trust a save's own `kind: person` label as its root.**
