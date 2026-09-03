# `webgpu_particle_test` — the last deliberate skip — is landed (2026-08-24)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Near-term priorities (2026-08-14 — from architecture review):  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **`webgpu_particle_test` — the last deliberate skip — is landed (2026-08-24)** — done and verified (2026-08-24, Zach: "the constant webgpu particle test deliberate fail being annoying... plz lets fix it now"). Added `WebGpuRenderer::drawParticles(const geom::FieldNode&, int)` in `src/Singularity/Screen/WebGPU/WebGpuRenderer.{hpp,cpp}`: visualizes a `FieldNode`'s `OntoMath::VectorField` as GPU points, reusing the existing `flatPipeline`/`drawFlat` machinery with `WGPUPrimitiveTopology_PointList` (no new pipeline/shader). Particle positions are an index-seeded xorshift hash into the field's origin/scale box, carried along `baseFlow` by a random phase — stateless, deterministic per `(field, count)`, no simulation buffer or `dt` needed. `PENDING_FEATURE_TESTS` in `CMakeLists.txt` is now empty. Verified: fresh reconfigure + `--target earthcall` clean, full `ctest` 65/65 (was 64/65). See [docs/BUILD_AND_ENVIRONMENT.md](../../BUILD_AND_ENVIRONMENT.md).
