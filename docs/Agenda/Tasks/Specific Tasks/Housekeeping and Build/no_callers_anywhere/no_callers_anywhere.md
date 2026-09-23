# no callers anywhere

**Status:** open  
**Section in the To-Do list:** Interaction · controls · GUI  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

Repairs this programme made, recorded because each was a claim the tree was not keeping: `Object::updateHoverState` had **no callers anywhere** and published its enter edge **twice** (compared against a field written one frame behind); `_isHovered`/`_hoverPoint` were unregistered (refusal #6) and are now read-only `hovered`/`hoverPoint`; `ObjectConcept::RelationTemplate` dropped every edge to a being outside the captured set, so an instantiated control was not a member of its category (**anchored relation templates**); `DynamicPropertyBridge::setValue` silently changed an authored property's **type** where a registered one coerces, so a `Map` writing 1.0 into a granted `bool` made every `get_if<bool>` read false.
