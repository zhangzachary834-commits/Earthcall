# Repair the First Mover Register trust root before calling save-path authorization enforced

**Status:** done and verified 2026-09-24 (Claude Opus 5.5, session `08b0f730-6e49-4c49-b27f-3a89c810ca4b`): trusted Person roots are runtime-only and take a `PrivateKey`; an absent or self-minted grantor yields `Standing::GrantorNotAuthenticated`; the WebSocket/MCP entry point is connected. ⚑ AUTHOR: the Person proof follows Sol's plan §5.3 (KeyStore unlock at boot via EARTHCALL_KEY_PASSPHRASE); Zach has not separately ratified it. Record: `docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md`.  
**Section in the To-Do list:** First Movers  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**Repair the First Mover Register trust root before calling save-path authorization enforced** — `evaluate` rejects a model grantor only when that grantor is present in the same serialized register and labeled `Model`; a valid signature from an absent grantor is never proven to terminate in a Person. Production has no `FirstMoverSession` or register save/load caller, so `SaveSystem` normally takes the no-session allow branch. Add the absent-grantor regression first, decide the trusted Person proof with Zach, then connect one real injection entry point. See [FIRST_MOVER_TRUST_AND_PROVENANCE_ANALYSIS_2026-08-20.md](../../../../../Analysis/FIRST_MOVER_TRUST_AND_PROVENANCE_ANALYSIS_2026-08-20.md). **Do not trust a save's own `kind: person` label as its root.**
