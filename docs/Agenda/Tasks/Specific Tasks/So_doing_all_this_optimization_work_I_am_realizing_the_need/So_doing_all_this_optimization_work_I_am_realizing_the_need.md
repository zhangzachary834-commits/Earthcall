# So doing all this optimization work I am realizing the need for a robu

**Status:** open  
**Section in the To-Do list:** Stuff for Zach to write when I don't know which section it belongs in (agents if you're reading this please move the bullet points below to their proper section):  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

So doing all this optimization work I am realizing the need for a robust Time framework more. I'm thinking this because we need granular control over the relationship of Laws with time variables. One optimization is to evaluate law facts asynchronously and also execute the ActionNodes asynchronously any property updates, rather than per-frame (and it also fits world-logic too). In one sentence: decoupling rete evaluation from the per-frame rendering would increase performance with a slight philosphy change. There is a real trade off there though if there is the future feature of tic, and if more things happen. And granular control over the frames themselves would still be available because nothing is a black box. You could have hybrids too where some laws are evaluated asycnrhonously and others are evaluated synchronously. You'd have to manage how their properties intersect carefully and not write the synchronous laws in ways that assume the asyconrhonous ones are gonna reach values as if they were also synchronous. However, this should be constructed from primitives by the Person rather than being a hardcoded black box.
