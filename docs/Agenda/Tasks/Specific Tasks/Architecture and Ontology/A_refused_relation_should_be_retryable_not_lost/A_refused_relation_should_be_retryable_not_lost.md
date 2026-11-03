# A refused relation should be retryable, not lost

**Status:** open  
**Section in the To-Do list:** Feature-sized (split out of Housekeeping 2026-08-13):  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**A refused relation should be retryable, not lost** — ⚑ AUTHOR (ontology decision, not a patch). `Formation::add` refusing an edge with an unbound endpoint is correct; *discarding* it is the part that isn't. Because Earthcall admits structure by validity, load order is now a correctness property: a refusal is a statement about a moment, and there is no "ask again later". Holding refused edges pending and re-offering them as endpoints appear would make the whole class of ordering hazard structurally impossible instead of fixed at one call site (fixed twice already: Bug #7, 2026-08-24 and 2026-08-27). Related: **the load path deserves one stated invariant** — *anything that binds beings to beings must run after every source of beings has arrived, or must be re-runnable* — which currently exists only as three separate bug fixes and no sentence.
