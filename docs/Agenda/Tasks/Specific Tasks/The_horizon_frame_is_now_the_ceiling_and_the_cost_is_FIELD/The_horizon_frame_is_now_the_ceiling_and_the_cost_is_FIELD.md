# The horizon frame is now the ceiling, and the cost is FIELD EVALUATION — not marching

**Status:** open  
**Section in the To-Do list:** Performance (opened 2026-08-24 by `tests/singularity/frame_lag_test.cpp`)  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**The horizon frame is now the ceiling, and the cost is FIELD EVALUATION — not marching.** (opened 2026-08-31, measured.) With the interaction bloat gone (Bugs.md #16), Zach reports the Perlin world's 3D Render phase at **~70 ms at the horizon** against near-zero looking at the ground, and wants the 200-600 FPS a blank zone gives. Measured with `scratch/probes/horizon_cost_probe.cpp` against `perlin-ground-plane`'s authored mathNode at 512x512 / 60 deg: **(a)** looking down, the marcher is indistinguishable from an empty frame; **(b)** at the horizon and 45 deg down it costs 3-8 ms above the empty-frame floor, a ~50-100x swing from camera angle alone; **(c)** the same cameras with a one-operation field (`y`) sit at the floor, so **~95% of it is evaluating the mathematics**; **(d)** the iteration cap is IRRELEVANT — 192 -> 96 -> 48 -> 24 changed nothing across four builds, because horizon rays terminate on distance, not on budget. Two consequences worth keeping: the campaign's headline change (cutting 192 to 28) bought nothing measurable and only cost the marcher its ability to hit thin geometry; and removing the three finite-difference gradient `sdfEval` per step — the whole of what a symbolic gradient could save in the march loop — moved the horizon **not at all** (~19% at 45 deg), almost certainly because the four samples share `floor(P)` and the compiler already CSEs Perlin's lattice hash. **So symbolic normal emission is demoted and the min/max height quadtree (expansion plan Phase C) is promoted to first** — the lever is fewer field evaluations along the ray. Absolute milliseconds were not steady on the measuring machine; a Release build on a quiet machine is owed before anyone quotes a number. Full reasoning in the addendums now at the end of all four Antigravity documents.

## 2026-09-05 re-audit after the 8 FPS regression

Zach reported that the earlier 40-70 ms observation no longer described the current build:
after later optimization attempts the zone ran at approximately 8 FPS, while F3's `3D
rendering phase` rapidly oscillated from roughly 1 to 100 to 300 ms. Codex reproduced the
current ceiling at native 2880x1800 resolution: the actual Perlin field cost 80.64 ms at
the horizon and **117.83 ms at 45 degrees**, compared with 10.44 ms for a trivial implicit
plane at 45 degrees. At 512x512 the Perlin cases remained only 6-8 ms, locating the
regression in the screen-filling exact-field fragment workload at Retina resolution.

The re-audit also found that Phase C's current heightfield fast-path premise is false for
the saved expression: `y - 40*noise(0.008*(p + vec3(100,0,100)))` reads `p.y` inside the
noise, so it is not proved to be `y-h(x,z)` and must not use stepping justified by
`df/dy=1`. The 2D height grid produced no material speedup for this field. A temporary
single-proxy-face experiment preserved the generic marcher's hashes in five camera cases
and reduced the native generic path by about 15 percent; the much larger result obtained
with the unproved fast path changed pixels and is rejected.

Full measurements, limitations, truth-preserving optimization order, selection-mesh
hitch finding, and authorship boundary:
[`2026-09-05_perlin_noise_floor_rendering_regression_audit.md`](../../../../audits/rendering_optimization/2026-09-05_perlin_noise_floor_rendering_regression_audit.md).

The staged implementation plan is
[`PERLIN_NOISE_FLOOR_RENDERING_RECOVERY_PLAN_2026-09-05.md`](../../../../plans/PERLIN_NOISE_FLOOR_RENDERING_RECOVERY_PLAN_2026-09-05.md).

— Codex, session `01a072e2-017b-7b03-aa4a-1ef25dab65d1`, 2026-09-05T11:59:28-07:00
