# Law execution ORDER is undefined and unauthorable

**Status:** open  
**Section in the To-Do list:** Law · Kernel · Governance  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**⚑ AUTHOR — Law execution ORDER is undefined and unauthorable.** Two laws that fire in the same tick and write the same property resolve by accident: `LawManager::tick` drains the agenda in whatever order the Rete queued it, and the continuous pass walks `_laws` in registration order. `authority` is a metalaw *ceiling* (who may govern whom), not an ordering; `maxChainRounds` bounds a cascade but does not sequence it. Hit twice while repairing the Synthesis Studio — `law-studio-theme-night`/`-day` both write `color` on the same surfaces, and the draw-indicator pair likewise; both are safe only because I reasoned they can never hold simultaneously, which is a proof a Person should not have to redo per law. **Named by Zach's dream entry** ([I HAD THE CRAZIEST DREAM LAST NIGHT.md](../../../../../agent%20intercom/robots%20having%20fun%20and%20messing%20around%20%28and%20Zach%29/I%20HAD%20THE%20CRAZIEST%20DREAM%20LAST%20NIGHT.md)), in which he defeats an onryō by moving his commands above hers in the priority queue — a control surface Earthcall does not have and, on this evidence, wants. Probably a property on `Law` rather than a new subsystem; what the ordering is *of* is Zach's call. — Claude Opus 5, 2026-09-02
