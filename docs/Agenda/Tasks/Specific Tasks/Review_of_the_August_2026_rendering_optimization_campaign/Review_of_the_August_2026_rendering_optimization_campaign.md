# Review of the August 2026 rendering-optimization campaign

**Status:** ✅ done and verified  
**Section in the To-Do list:** Performance (opened 2026-08-24 by `tests/singularity/frame_lag_test.cpp`)  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Review of the August 2026 rendering-optimization campaign** — done (2026-08-31), [RENDERING_OPTIMIZATION_CAMPAIGN_REVIEW_2026-08-31.md](../../audits/RENDERING_OPTIMIZATION_CAMPAIGN_REVIEW_2026-08-31.md). Kept the instanced SDF path, mesh colour batching, program memoization on `(memoId, fieldRevision)`, the lazy field mesh, the collision AABB memo, the ScalarLeaf narrowing and the tetrahedral normal. Reverted six changes that bought frame time by making the picture untrue — the GPU's `cnoise3` silently aliased to **simplex** noise while the CPU kept computing Perlin (so the ground seen stopped being the ground collided with); a 1.5-world-unit minimum march step and a 28-iteration budget; a hard 600-unit horizon; a `cbrt` cell budget that took the noise floor's collision mesh to **4 samples over 60 units of height**; an unsound `[-1,1]` Perlin interval that lets the tessellation subdivision delete cells containing surface (measured range is `[-1.127, 1.123]`); and two dead law-visible knobs (`@screen.renderScale`, `@screen.performanceMode`) that nothing read. Also untracked 64 agent scratch files from the repo root.
