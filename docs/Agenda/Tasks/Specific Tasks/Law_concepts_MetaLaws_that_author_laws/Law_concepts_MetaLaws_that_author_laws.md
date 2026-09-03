# Law-concepts: MetaLaws that author laws

**Status:** open  
**Section in the To-Do list:** Person-facing surface  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**Law-concepts: MetaLaws that author laws** (from Zach, 2026-08-19 — the walk's multiplier): the ImGui Law window is tedious to compose in; the remedy is the authoring analog of `ObjectConcept` — capture a law-*shape* once, instantiate from a picker ("a law like this one, but over that property, on that category"). Precedent already in-tree: `ControlPatterns` factories emit `ConditionModel`/`ActionModel` *data*; turn that shape Person-ward so the Law window becomes the inspection surface, not the composer. See [The Walk Writes Back §2](../../../../Reflections%20on%20Earthcall%27s%20Progression/Reflections%20on%20Trajectory/The_Walk_Writes_Back.md).


---

## Blocker found 2026-09-03 (Claude Opus 5, `session_01GsrBySNw4oG1zof5AQ21KM`)

**A Metalaw is not expressible today: no `ActionNode::Kind` authors a Law.**

Checked `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp`. The verb vocabulary runs
`Set`(0) · `Add` · `Scale` · `Lerp` · `Drive` · `Sequence` · `Parallel` · `Spawn` · `Map` ·
`Flow` · `Publish`(10) · `Create`(11) · `AddProperty` · `AddElement` · `RemoveProperty` ·
`RemoveElement` · `Destroy`(16) · `Synthesize`(17) · `PlayAudio`(18) · Zone-mint ·
`AddRelation`(20, landed 2026-08-31). A law can mint an Object, a property, an element, a
relation, an event, a Zone, and a sound. It cannot mint a **Law**.

So `Create`(11) is the exact precedent and the exact gap: it exists because a law had to be
able to author a being it was never shown. A Metalaw needs the same move one level up.

**This is not a Refusal 3 problem.** Refusal 3 forbids a new enum value for a *kind of
thing*; `ActionNode::Kind` is a verb vocabulary, append-only and serialized as ints, and the
project already extends it deliberately (`AddRelation` = 20 this week). The path is open and
simply unwalked.

**Note this is now Zach's second independent arrival at the need** — 2026-08-19 as
"Law-concepts: MetaLaws that author laws (the walk's multiplier)", and again 2026-09-02 in
[`Second-Nature Law Authoring.md`](../../../../Zones%20of%20Actualization/Second-Nature%20Law%20Authoring.md):
*"Law Authoring window is very expressive but very tedious for human authors. So we need to
author a much more human version with Laws. And yes, this is going to be Metalaws."* A need
that recurs unprompted after two weeks is evidence it is real and that the tedium is not
being worked around. It is also the same finding Fable 5 filed as *tedium as a protocol
finding* in *The Walk Writes Back* (2026-08-19).
