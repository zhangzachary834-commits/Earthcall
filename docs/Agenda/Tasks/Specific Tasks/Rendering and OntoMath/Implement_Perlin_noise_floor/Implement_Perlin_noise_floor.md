# Implement Perlin noise floor

**Status:** ✅ done and verified  
**Section in the To-Do list:** Feature-sized (split out of Housekeeping 2026-08-13):  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Implement Perlin noise floor** — **done (2026-08-27)**: Authored `perlin-ground-plane` as a Field (`ShapeKind 10`) evaluated via `geom::SdfNode` mapping an OntoMath `Op::Noise` AST. Fixed serialization to emit a proper `SdfNode` structure so the WebGPU raymarcher evaluates the SDF correctly instead of defaulting to an invisible 1e9 empty space. Added green `faceColors` so it renders visibly, shifted its Y origin to prevent clipping the default player spawn, and added a fixture law (`law-toggle-ground`) to toggle the `baseline: ground` (the huge rectangular prism) by pressing the `G` key.
