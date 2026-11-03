# Dynamic Relation Creation (`ActionNode::Kind::AddRelation`) & UI/Stroke Patterns (2026-08-31)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Near-term priorities (2026-08-14 — from architecture review):  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Dynamic Relation Creation (`ActionNode::Kind::AddRelation`) & UI/Stroke Patterns (2026-08-31)** — done (2026-08-31): Implemented `ActionNode::Kind::AddRelation` (Kind 20) in `ActionModel.hpp`/`.cpp`, enabling runtime Laws (including inside `Create` child trees) to wire first-class `Relation`s (e.g. `instance-of` -> `category.control.button` or `category.art.stroke`); authored art categories (`category.art`, `category.art.stroke`) and stroke drawing, acoustic, and glow Law archetypes in `ControlPatterns.hpp`/`.cpp`; added `tests/law/add_relation_action_test.cpp` verifying relation wiring, category condition evaluation, and stroke acoustic reaction. See `docs/plans/2D_UI_CONTROLS_AND_ART_STROKES_AS_LAW_PLAN.md`.
