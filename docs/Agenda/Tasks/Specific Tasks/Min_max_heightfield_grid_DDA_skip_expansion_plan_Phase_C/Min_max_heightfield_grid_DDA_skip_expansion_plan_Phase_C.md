# Min/max heightfield grid DDA skip (expansion plan Phase C)

**Status:** ✅ original implementation recorded; current proof boundary re-audited 2026-09-05

## Re-audit and correction — 2026-09-05

The current implementation pass corrected two claims in the original record before relying
on this path. The saved Perlin floor is not admitted as a heightfield: its `Noise` argument
reads the full ambient `p`, including `y`, so `isHeightfieldExpr` now proves y-independence
recursively and the saved field falls open to the generic marcher. The former Perlin
Lipschitz value was empirical rather than closed-form, so `computeHeightGrid` now refuses
`Noise`; no Perlin grid may delete a ray segment on sampled evidence. A 2D linear field
remains the cache/proxy test subject because its bound is compositionally derived.

The SDF proxy now back-face-culls its inward-wound cube to avoid launching the expensive
analytic fragment program twice. Native Metal then found that DDA candidate-cell traversal
could lose grazing roots, so traversal is quarantined rather than made live. The grid still
exists as a conservative proof/cache seam, but cannot upload or skip a ray until repaired.
`heightfield_predicate_test`, `webgpu_heightfield_sweep_test` (six camera cases, byte
identical cache-present/cache-absent), and `webgpu_sdf_parity_test` (21 GPU/CPU shapes)
now pass on native Metal.
See [`PERLIN_NOISE_FLOOR_RENDERING_RECOVERY_PLAN_2026-09-05.md`](../../../../plans/PERLIN_NOISE_FLOOR_RENDERING_RECOVERY_PLAN_2026-09-05.md).
**Section in the To-Do list:** Performance (opened 2026-08-24 by `tests/singularity/frame_lag_test.cpp`)  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

> **Historical record notice (2026-09-05):** The verbatim record below predates the
> proof-boundary re-audit. Its statements that `Noise` had a usable Lipschitz bound,
> that the saved Perlin field produced a grid, that the Perlin GPU sweep was bit-identical,
> and that the full suite was green are retained for provenance only; they are not current
> verification claims. The re-audit above is authoritative for the implementation now in
> the tree.

✅ **Min/max heightfield grid DDA skip (expansion plan Phase C)** — done and verified (2026-08-31/09-01). `geom::isHeightfieldExpr` (Sdf.hpp/.cpp) matches the decidable `Sub(y, h)` structural pattern only — a Union/Intersect/other CSG-combined field, or `h - y` (reversed operands), correctly refuses rather than guesses. `geom::computeHeightGrid` builds a `dimX x dimZ` conservative `(hMin,hMax)` grid via a new PER-AXIS Lipschitz estimator (`estimateLipschitz`, handling ScalarLeaf/ValueLeaf/Component/VectorConstruct/Add/Sub/Scale-by-constant/Noise; anything else refuses, dimX=0, "no acceleration" rather than an unsound bound) — per-axis matters because the real authored Perlin-floor field feeds the *whole* 3D point into `noise()`, so it genuinely depends on `y`, while a properly-authored 2D heightfield (extracting only `Component(p,"x")`/`Component(p,"z")`) does not; a single blanket constant couldn't tell those apart and would pay the full y half-extent either way. `Object::rebuildHeightGrid()` builds it lazily (`_heightGridDirty`, same shape as `rebuildFieldMesh()`), `WebGpuRenderer::drawImplicit` gained a `heightGrid` parameter wired through a new `group(1) binding(1)` storage buffer (`SdfInstanceData::heightGridOffset/DimX/DimZ`, 0 = no grid = unmodified marcher), and `kMarcher`'s `fs()` runs a new `heightGridAdvance()` (Amanatides-Woo 2D DDA) before the existing per-step loop — it only ever fast-forwards `t` or discards on a *proven* miss; the fine marcher's own correctness logic is untouched. Gated live by `@screen-channel.heightGridDdaEnabled` (default true), read every frame in `EngineRender.cpp` next to `setWireframe`. **Verified**: `heightfield_predicate_test` (new) densely samples 20 000 random points against the grid's own bound for both the real field and a synthetic 2D heightfield — 0 violations in both, and confirms an unhandled op (`Pow`) refuses. `webgpu_heightfield_sweep_test` (new) renders the real Perlin-floor field from 5 camera angles (straight down, grazing horizon, 45° down, close oblique, near-parallel) with the grid ON and OFF — **bit-identical pixels at every angle** (diffPixels=0), so the skip never removes real geometry. `scratch/probes/horizon_cost_probe.cpp` extended with a `--no-grid` A/B toggle so on/off can be measured in one binary/process rather than trusting separate builds: for the *real* field (which pays the full y half-extent since it depends on y) the horizon case improves modestly (~10-15%, noisy on a loaded machine); a syntactic 2D heightfield gets a much tighter bound (60 vs the naive global 152 width at the same resolution, tightening further with resolution) — the mechanism is real and correctly implemented, but this specific save file's own authoring choice (3D noise argument instead of 2D) is what caps its win. Full ctest: 74-79/75-79 pass across runs (varies only by which throwaway scratch executables were present), same single pre-existing failure as always (`smooth_tessellation_cache_test`, Bugs.md #11). Built and tested in an isolated `git worktree` from HEAD since the primary working tree had another concurrent session's uncommitted, currently-broken `AddRelation` law-action work (`ActionModel.cpp`/`ControlPatterns.cpp`) blocking `earthcall_core` for everyone until that session finishes or someone reverts/completes it — not touched here. Files: `Sdf.hpp/.cpp`, `Object.hpp`, `ObjectCollision.cpp`, `ObjectRender.cpp`, `Renderer.hpp`, `OpenGLRenderer.hpp/.cpp`, `WebGpuRenderer.hpp/.cpp`, `SdfWgsl.cpp`, `ScreenChannel.hpp/.cpp`, `EngineRender.cpp`, plus the two new tests and the probe extension. **Left open**: Phase F (Hi-Z pre-pass) turned out to need more than the plan estimated — see the note directly below.
