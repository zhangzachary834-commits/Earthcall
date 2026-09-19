# WebGPU High-FPS Manifold SDF Acceleration & Adaptive Marching

**Status:** ✅ done and verified  
**Section in the To-Do list:** Performance (opened 2026-08-24 by `tests/singularity/frame_lag_test.cpp`)  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **WebGPU High-FPS Manifold SDF Acceleration & Adaptive Marching** — done and verified (2026-08-28): Implemented Keinert et al. Over-Relaxation ($\omega = 1.4$) in `SdfWgsl.cpp` with overstep rollback, 4-tap tetrahedral gradient for isotropic surface normals with 33% lower cost, adaptive heightfield stepping with slope-bounded step sizes, and horizon distance clamping in `WebGpuRenderer.cpp`. Verified: `webgpu_sdf_parity_test` 20/20 agree with CPU ground truth (including `Expr(iso)` improving to diff=0); `webgpu_micro_mastery_lag_test` normalized average frame time dropped from 108.7 ms to 23.38 ms; full ctest suite 72/73 passed in 25.3s. Full audit: [`docs/audits/rendering_optimization/SDF_MANIFOLD_HIGH_FPS_RAYMARCHING_AUDIT_2026-08-28.md`](../../../../audits/rendering_optimization/SDF_MANIFOLD_HIGH_FPS_RAYMARCHING_AUDIT_2026-08-28.md).
