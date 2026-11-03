# Rendering costs ~0.2 ms per object, and that is the whole frame budget in a lived-in zone

**Status:** open  
**Section in the To-Do list:** Performance (opened 2026-08-24 by `tests/singularity/frame_lag_test.cpp`)  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**Rendering costs ~0.2 ms per object, and that is the whole frame budget in a lived-in zone.** Measured 2026-09-02 with a temporary probe in `Engine::tick`: `zone='Sanctum of Beginnings' objects=129  frame=30.428 ms (32.9 fps)  laws=0.544  zone=1.786  render3d=25.487`. **Correction to an earlier version of this entry: the 60 fps figure was never a target — Zach's MacBook Air panel caps there. On an external LG monitor a blank zone runs 200-300 fps.** So the engine is fine when empty and the cost is per-object render: 129 objects in Sanctum, 122 in Ourverse Gathering, 71 in Cavern of Light. The law path is 0.544 ms (1.8% of the frame) and is not the problem. This is also what made the Synthesis Studio's controls die after a few seconds of drawing — its draw law was minting 60 objects a second, and 600 objects is ~7 fps. Worth profiling `render3d` per draw call; note that ShapeKind 1 (Polyhedron) draws **one mesh per face**, so a world full of them costs many times a world of spheres. — Claude Opus 5, 2026-09-02
