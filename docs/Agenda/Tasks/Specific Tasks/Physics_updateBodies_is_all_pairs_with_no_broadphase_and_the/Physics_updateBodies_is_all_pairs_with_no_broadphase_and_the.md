# `Physics::updateBodies` is all-pairs with no broadphase, and the legacy engine is on by default

**Status:** open  
**Section in the To-Do list:** Performance (opened 2026-08-24 by `tests/singularity/frame_lag_test.cpp`)  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

⚑ AUTHOR — **`Physics::updateBodies` is all-pairs with no broadphase, and the legacy
  engine is on by default.** Measured on an idle machine: `Zone::update` costs
  1.1 / 3.3 / 11.1 / 40.7 ms at 64 / 128 / 256 / 512 objects — a fitted `n^1.75`, i.e.
  quadratic, and the one number that held steady across every machine state the session saw. `sample(1)` puts 100%
  of a chess frame in `Zone::update → Physics::updateBodies (Physics.cpp:368) →
  dispatchCollision → gjkEpaCollision → epaPenetration → addBorderEdge
  (CollisionDispatcher.cpp:214)`. The pair loop is `Physics.cpp:327-343`; a second one is at
  `:222`. There *is* an AABB pre-filter at `:353`, but B's AABB is recomputed inside the
  inner loop, so the corner walk is paid per pair before the filter can reject anything.
  `g_legacyEngineEnabled = true` at `Physics.cpp:33` and nothing in the app ever clears it —
  the only caller of `setLegacyEngineEnabled` in the tree is
  `tests/zones/ground_plane_test.cpp:64`. **Two decisions are Zach's, not an agent's:**
  (a) should the legacy engine be on by default at all, and (b) a broadphase (uniform grid
  or sort-and-sweep over cached AABBs) is a change to how the world *behaves* under
  contact, not just to how fast it runs. This is the thing `ce5c1cbe` ("Attempt to fix
  chess lag") was reaching for; that commit fixed a real `Singular` copy/move slicing bug
  and left the quadratic standing.
