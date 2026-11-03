# SDF Pipeline Prophetic Cache and Spatial Acceleration Implementation Plan

**Date:** 2026-09-19  
**Timestamp:** 2026-09-19T11:43:00-07:00  
**Author:** GPT-5.6 Sol  
**Session ID:** `chatgpt-2026-09-19-sdf-pipeline-implementation`  
**Implementation branch:** `sol/sdf-pipeline-optimization-20260919`  
**Base:** `sync-from-earthcall-main` @ `f6a65d32d38e0741835ac4a010f65222dacd91a7`

## Authority and source material

Zach explicitly authorized this implementation as an automatic-approval pass and will review the resulting branch after the work is complete.

This plan implements the findings in:

- `docs/audits/rendering_optimization/2026-09-18_sdf_calculation_and_rendering_pipeline_inefficiency_audit.md`
- `docs/audits/rendering_optimization/2026-09-18_sdf_pipeline_mathematical_complexity_companion.md`

The current integration branch moved 58 commits after the audit's original base. This implementation therefore starts from the corrected current integration HEAD rather than rebasing the stale audit branch. The audit documents were carried forward unchanged for lineage.

The PR #53 temporal-rollback incident was read before writes. This branch descends from the corrected post-incident integration head, not the contaminated merge window.

## Goal

Change Earthcall's SDF renderer from frame-driven repeated derivation toward revision-driven cached truth, then establish the infrastructure required for truth-preserving spatial skipping of expensive authored fields.

The target steady-state shape is:

[
T_{current} approx O(P I E)
]

toward:

[
T_{future} approx O(P(log H + A + Z E)), qquad Z ll I
]

without changing what an authored SDF/OntoMath expression means.

## Non-negotiable invariants

1. CPU/GPU mathematical parity remains authoritative.
2. Unknown acceleration knowledge fails open to the exact generic path.
3. No guessed world-space constants may delete visible roots.
4. No faster substitute function may replace authored mathematics.
5. Heightfield specialization requires structural proof, not appearance.
6. Cache invalidation must name its dependency and carry a regression witness.
7. Rendering optimization state that is Person-governable must be visible through the existing property/channel architecture rather than a hidden second settings system.
8. No claim of speedup without measurement against a Release/native workload.

---

## Phase A — Pixel-identical frame-path waste removal

### A1. Zero-copy memoized Program hits

**Current:** `WebGpuRenderer::drawImplicit` copies the complete cached `sdfwgsl::Program` on every memo hit.

**Change:** keep either a local owned Program for a miss or a `const Program*`/reference to the cached Program for a hit. The per-draw path reads `params` and `needsGradientStep` through that stable reference.

**Acceptance:**
- cache-hit path performs no Program copy;
- compile/refusal behavior unchanged;
- color-expression cache key behavior preserved;
- existing WebGPU SDF parity/color tests remain green.

### A2. Cache structural heightfield proof with the memoized program

**Current:** `geom::isHeightfieldExpr(field, nullptr)` walks structure every draw.

**Change:** store the structural proof result in the memoized SDF entry at compile time/revision change.

**Acceptance:**
- no repeated proof walk on a valid memo hit;
- `memoId == 0` remains correct by computing directly;
- proof invalidates whenever the structural memo invalidates.

### A3. Stop building unusable height grids while DDA is quarantined

**Current:** `Object::drawFieldModel` asks `getHeightGrid()` even though `kHeightGridDdaTraversalVerified == false` makes the renderer unable to consume it.

**Change:** make renderer capability explicit through the Renderer boundary (e.g. `usesHeightGridDda()` / equivalent derived capability), and only demand-build the grid when the current backend can actually consume it. The authored enable property remains distinct from verified substrate capability.

**Acceptance:**
- no height-grid build merely because an analytic field is drawn while traversal is quarantined;
- heightfield tests can still explicitly exercise the grid builder;
- no semantic change to generic rendering.

### A4. Cache derived analytic SDF forms for smooth/complex geometry

**Current:** `sdfFromSmooth` / `sdfFromComplex` are reconstructed in the render path.

**Change:** add derived caches owned by `Object`, invalidated by `rebuildGeometryCaches()`, and reuse them in rendering.

**Acceptance:**
- no new domain class or authored state;
- cache contains derived substrate representation only;
- mutations invalidate the cache;
- rendered/collision meaning unchanged.

---

## Phase B — Separate structural compilation from numeric values

### B1. Introduce an explicit structural fingerprint/revision seam

The current `_fieldRevision` is a broad geometry revision. Do not blindly rename it: many existing caches depend on it.

Add a distinct SDF render-structure revision/fingerprint that changes only when operations/tree topology or other WGSL-shaping structure changes. Numeric parameter edits may continue to increment the broad geometry revision for collision/tessellation while leaving shader structure stable.

Color-expression structure participates in the same concept; color parameter values must not masquerade as shader topology.

### B2. Split SDF compiler structure from parameter collection

Refactor `sdfwgsl::compile` internals so the order/layout of numeric parameters is structural, while current values can be recollected without rebuilding the WGSL string.

Possible representation:

```
CompiledProgram
  wgsl
  paramLayout / collector description
  needsGradientStep
  structural flags

ParameterBlock
  vector<float> currentValues
```

The implementation need not expose these names publicly if a smaller API is clearer, but the dependency split must be real.

### B3. Regression witness

Add a non-GPU test that constructs one SDF structure, changes numeric values repeatedly, and proves:
- WGSL structural output/fingerprint remains identical;
- parameter block changes;
- a structural mutation changes the structural identity.

This is the acceptance gate before relying on split revisions in the renderer.

---

## Phase C — Observability before deeper optimization

Expose counters sufficient to distinguish CPU structural churn from GPU upload churn. At minimum:

- SDF structural compile count;
- program memo hits/misses;
- generated WGSL bytes on compile;
- SDF parameter bytes staged/uploaded;
- height-grid build count or active-grid count where appropriate.

Prefer FrameStats/ScreenChannel paths that already exist; no parallel telemetry subsystem.

Acceptance: values are readable through the existing ScreenChannel property model and covered by channel-path/no-black-box tests where applicable.

---

## Phase D — Persistent GPU SDF data

Only after Phase B makes structural/value lifetime explicit:

1. retain static parameter blocks by structural/value revision;
2. stream transforms/materials in the dynamic instance path;
3. keep conservative grids resident by grid revision;
4. reuse bind groups while their backing allocation generation/layout remains valid;
5. update dirty ranges only.

Acceptance:
- static scene performs no redundant static-parameter upload after warmup;
- edits update the intended range;
- buffer lifetime is safe across frame submission;
- no stale GPU view after pool growth/reallocation.

This phase may require extending `GpuBufferPool` or adding a persistent-storage arena inside the WebGPU channel. It must not overload the frame ring with long-lived ownership.

---

## Phase E — Generalized value + gradient propagation

Extend analytic derivative/value propagation beyond the current single root Expr+Noise case.

Order:
1. exact analytic primitives;
2. Expr leaves covered by the differentiable OntoMath subset;
3. CSG min/max branch propagation;
4. Morph / SmoothUnion only where derivative semantics are proved and parity-tested.

Nondifferentiable boundaries must take an explicit stable policy or fall back to finite differences.

Acceptance:
- CPU/GPU silhouettes/depth remain within existing parity tolerances;
- diagnostic counters demonstrate fewer fallback gradient evaluations;
- no performance claim based only on call count.

---

## Phase F — Repair proven-heightfield DDA

Keep `kHeightGridDdaTraversalVerified = false` until the native on/off camera corpus is exact.

Repair the grazing/root-boundary handoff and add regression coverage for:
- rays on cell boundaries;
- near-horizontal rays;
- roots exactly at candidate entry/exit;
- camera inside proxy;
- negative and positive ray directions;
- thin height variation.

Only then remove the quarantine.

The saved Perlin floor remains ineligible unless its authored mathematics itself becomes y-independent.

---

## Phase G — Generic conservative zero-crossing hierarchy

### G1. CPU representation

Build a derived spatial hierarchy over an SDF's local extent. Each node carries:

- local AABB;
- conservative `[fMin, fMax]`;
- child indices or leaf marker;
- state: `ProvedEmptyOfZero`, `MayContainZero`, or `Unknown`.

A node may be skipped only if zero is provably excluded.

### G2. Range source

Use `geom::evalRange` / OntoMath interval machinery as the truth source. Unsupported/unknown range operations produce `Unknown`, not guessed bounds.

### G3. Subdivision policy

Start with a bounded adaptive octree or linearized BVH-like hierarchy:
- split cells where zero remains possible and cell size / depth budget permits;
- stop when proved empty, maximum depth is reached, or bound quality stops improving;
- benchmark branching/layout alternatives before committing to a permanent GPU format.

### G4. GPU traversal

Upload a compact linear hierarchy and intersect it before the fine marcher. Traversal may advance a ray across proved-empty nodes, but all ambiguous/unknown regions hand off to the exact marcher.

### G5. Invalidation

Initial implementation may rebuild the hierarchy per SDF value revision if necessary for correctness. The follow-up target is Prophetic invalidation of only dependency-affected nodes.

Acceptance:
- an accelerated render hashes identically to the exact path across the camera corpus;
- disabling the hierarchy produces the same visible/depth result;
- unknown interval nodes never suppress exact evaluation;
- native high-resolution authored-Perlin benchmark records exact-evaluation reduction and GPU time.

---

## Verification ladder

After every phase that touches source:

1. focused non-GPU tests for the modified abstraction;
2. `webgpu_sdf_parity_test`;
3. `webgpu_sdf_color_expr_test`;
4. `webgpu_sdf_distance_test`;
5. heightfield tests when relevant;
6. default build + full non-GPU suite;
7. WebGPU/native tests only where a real desktop GPU/display environment is available;
8. `frame_lag_test` interpreted using its `ok/STANDING/LAG/IMPROVED` contract, never by raw wall-clock alone.

Before PR/merge, compare final branch against current integration and hard-stop on unexplained file-count/deletion explosions per the PR #53 incident rule.

## Implementation sequencing for this branch

This implementation session begins immediately with **Phase A**, then proceeds into **Phase B/C** as long as their regression gates can be established cleanly.

Phases D–G are deliberately gated by the preceding lifetime/observability work. They are authorized by Zach, but mathematical accelerators will not be force-enabled merely to make this branch appear "complete."

The branch may be offered for review at a coherent checkpoint if the earlier phases are complete and the later frontier phases require separate benchmark-driven work. Any remaining phase stays explicitly open in this plan and the To-Do index rather than being silently declared complete.

---

**Signed:** GPT-5.6 Sol  
**Session:** `chatgpt-2026-09-19-sdf-pipeline-implementation`  
**Date:** 2026-09-19  
**Timestamp:** 2026-09-19T11:43:00-07:00
