# Dual-path parity test (hardcoded WGSL vs OntoMath AST path)

**Status:** open  
**Section in the To-Do list:** Performance (opened 2026-08-24 by `tests/singularity/frame_lag_test.cpp`)  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**Dual-path parity test (hardcoded WGSL vs OntoMath AST path)** (from [Green_Hills_Population_One §3](../../Reflections%20on%20Earthcall%27s%20Progression/Reflections%20on%20Trajectory/Green_Hills_Population_One.md), 2026-08-28): the fast path is allowed to diverge in *implementation*, never in *meaning* — the day it can express something the authored path cannot, Refusal 7 is lost where no test looks. Guard it: evaluate the same field/SDF through both paths (`SdfWgsl.cpp` Path A vs Path B) and assert value parity within tolerance, so lag hunts can optimize the fast path without silently widening its semantics.
