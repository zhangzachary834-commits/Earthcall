# SDF Calculation and Rendering Pipeline Inefficiency Audit

**Date:** 2026-09-18  
**Timestamp:** 2026-09-18T23:57:00-07:00  
**Agent:** GPT-5.6 Sol  
**Session ID:** `chatgpt-2026-09-18-sdf-pipeline-audit`  
**Audited base:** `sync-from-earthcall-main` @ `724dd256aa0599caba63aa68c52352c52de6349f`  
**Scope:** SDF/OntoMath calculation, WGSL generation, WebGPU raymarching, SDF batching, parameter upload, heightfield acceleration, and the CPU-side derived-state feeding those paths.
**Mathematical companion:** [`2026-09-18_sdf_pipeline_mathematical_complexity_companion.md`](./2026-09-18_sdf_pipeline_mathematical_complexity_companion.md)

## Human direction and architectural context

Zach asked for a fresh audit of inefficiencies and bottlenecks in Earthcall's SDF calculation and rendering pipeline, with emphasis on the current implementation rather than stale design documents. This audit also reads the findings through Zach's current caching direction: properties, Singular data/operations, and Laws should increasingly become cached derived state with prophetic invalidation rather than repeated reconstruction; generated GPU programs should likewise change incrementally rather than be rebuilt as giant opaque streams.

That direction is not treated here as permission to change mathematical truth for speed. The August rendering campaign already established the opposite lesson: optimizations that substitute a cheaper function, guessed march bound, or unproved heightfield assumption are inadmissible even when they improve FPS. The target is less repeated work for the **same authored mathematics**.

## Executive finding

The present SDF pipeline is no longer best described as "a sphere tracer with too many iterations."

The dominant problem for expensive authored fields is **field-evaluation density**:

> expensive OntoMath mathematics × screen-filling pixel count × exact field samples along each ray.

Current source and the repository's measured Perlin probes agree on this. Several secondary CPU/GPU inefficiencies then stack on top: cached Programs are copied per draw; one revision conflates shader structure with numeric values; static parameter and instance data are re-streamed each frame; derived SDF/proof/transform facts are recomputed in the render path; and the conservative height-grid accelerator is currently quarantined off.

The highest-value long-horizon direction is therefore a truth-preserving hierarchy that proves where an expensive field **cannot** cross zero, so exact evaluation is never invoked there. Earthcall already owns mathematical interval/range machinery capable of becoming the foundation of such a spatial relevance system.

---

## 1. P0 — The primary bottleneck is field evaluation, not the 192-step loop

### Current source

The fragment marcher in:

- `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`

runs a bounded loop of 192 iterations, but the number alone is misleading. Each iteration evaluates either `sdfSampleStep(p)` or `sdfEval(p)`, and the cost of those operations varies by orders of magnitude depending on authored mathematics.

### Existing measured evidence

The repository already records the key experiment in:

- `docs/Agenda/Tasks/Specific Tasks/Performance and Runtime/The_horizon_frame_is_now_the_ceiling_and_the_cost_is_FIELD/The_horizon_frame_is_now_the_ceiling_and_the_cost_is_FIELD.md`
- `docs/plans/SDF_MANIFOLD_HIGH_FPS_ACCELERATION_PLAN.md`
- `docs/plans/MULTI_HUNDRED_FPS_SDF_ENGINE_PLAN_2026-08-28.md`

The August 31 probe found:

- looking down: authored Perlin was close to the empty-frame floor;
- horizon / 45-degree cases: authored Perlin added several milliseconds at 512×512;
- a one-operation field at the same cameras remained near the floor;
- changing the iteration cap `192 -> 96 -> 48 -> 24` produced essentially no change.

The September 5 native-resolution re-audit then reported approximately:

- **80.64 ms** for the authored Perlin field at the horizon;
- **117.83 ms** at 45 degrees;
- **10.44 ms** for a trivial implicit plane at 45 degrees;
- only roughly **6–8 ms** for the Perlin cases at 512×512.

Absolute timings were not stable enough to treat as universal machine-independent numbers, but the ratios are decisive: native-resolution screen-filling evaluation is the ceiling.

### Consequence

Further reductions to the loop cap are the wrong first lever and are dangerous: the earlier campaign already demonstrated that lower caps can lose thin geometry while buying no measurable benefit in the expensive horizon case.

The renderer must instead reduce **the number of locations at which the authored function is evaluated**.

---

## 2. P0/P1 — Build a generic conservative zero-crossing spatial hierarchy

Earthcall already contains:

- `geom::evalRange(const SdfNode&, boxMin, boxMax)` in `Sdf.cpp`;
- interval/range evaluation for authored OntoMath;
- exact SDF structure, including CSG;
- conservative local bounds;
- the architectural precedent of Prophetic Rete: prove irrelevance, then skip work.

These pieces suggest a more general accelerator than another terrain-only fast path.

### Proposed mechanism

Build a persistent hierarchy of conservative field bounds over object-local space: sparse bricks, octree cells, BVH-like interval nodes, or another frontier structure selected after benchmarking.

For each spatial node:

1. derive a conservative interval `[f_min, f_max]` for the authored SDF over that cell;
2. if zero is **not** inside the interval, the cell is proved incapable of containing the surface;
3. a ray traversing that node may skip it without point-evaluating the expensive function;
4. only zero-containing / unknown cells descend or hand off to the exact marcher.

This is not approximation of the field. It is a proof of where exact evaluation is unnecessary.

### Why this fits Earthcall unusually well

This is spatially analogous to Prophetic Rete:

> Do not ask every possible fact whether it matters. Prove large regions irrelevant, and preserve the exact evaluator only where relevance remains possible.

The structure should be derived state, revisioned from authored mathematical dependencies, and invalidated prophetically when those dependencies change.

### Important limitation

Unknown interval structure must fail open. If a node cannot be bounded soundly, the hierarchy says "unknown" and the exact marcher runs. It may only prove impossibility; it must never invent absence.

---

## 3. P1 — Generic authored expressions can pay multiple full SDF evaluations per march step

The current gradient-corrected path is necessary because an authored expression `f(p)=0` is not generally a true signed distance field. The marcher therefore uses approximately:

`distance_step = f(p) / |grad f(p)|`

rather than stepping by raw `f`.

For the non-analytic-gradient path, however, `sdfSampleStep(p)` returns the raw value with gradient length zero. The marcher then estimates the gradient by evaluating:

- `sdfEval(p + dx) - raw`
- `sdfEval(p + dy) - raw`
- `sdfEval(p + dz) - raw`

Thus one march step can invoke the entire authored field roughly four times.

After a hit, the fallback normal also evaluates the field four more times using the tetrahedral normal estimator.

### Existing partial solution

`SdfWgsl.cpp` already contains an analytic jet/gradient emitter. This is an excellent direction, but its applicability is narrow. `hasAnalyticGrad` currently requires:

- root op is a single leaf;
- primitive is `Expr`;
- a mathNode exists;
- the AST contains Noise;
- the complete AST is in the differentiable subset understood by the jet emitter.

That means analytic derivative capability is discarded when useful differentiable expressions participate in broader SDF structure.

### Recommendation

Generalize derivative/value propagation through the SDF tree itself.

For example, Union / Intersection / Difference / Morph can propagate the selected branch's derivative where mathematically well-defined, with explicit handling at nondifferentiable boundaries. Exact analytic primitives already have known gradients or inexpensive normal formulas.

This can make `sdfSampleStep` a real value+bound/gradient operation over substantially more of Earthcall's authored geometry.

### Priority qualification

The repository's old Perlin experiment found that removing three finite-difference samples changed the horizon case little and improved the 45-degree case by only about 19%, probably because nearby Perlin samples share lattice work that the shader compiler common-subexpression-eliminates.

Therefore this is a real architectural inefficiency and likely important for future arbitrary OntoMath, but it is **not presently evidenced as the largest Perlin-floor lever**.

---

## 4. P1 — The conservative height-grid accelerator exists but is hard-disabled

Earthcall contains substantial Phase C machinery:

- structural proof via `geom::isHeightfieldExpr`;
- lazy CPU `HeightGrid` construction;
- conservative min/max cells;
- WGSL DDA traversal in `SdfWgsl.cpp`;
- a Law-visible `heightGridDdaEnabled` renderer property.

But current `WebGpuRenderer::drawImplicit` contains:

`constexpr bool kHeightGridDdaTraversalVerified = false;`

and `gridActive` depends on that constant.

Therefore no production draw can currently activate the DDA traversal, even if the ScreenChannel property is true.

### Why the quarantine is correct

The code comments record a native Metal camera sweep where DDA candidate-cell hand-off disagreed with the generic marcher around grazing roots. A performance accelerator may not erase a root the exact path would render.

The saved Perlin floor also is not actually a proven `y-h(x,z)` heightfield, because its Noise subtree reads the full point including `p.y`. It therefore cannot lawfully inherit assumptions such as `df/dy = 1`.

### Current inefficiency

Earthcall retains the entire accelerator architecture while its active runtime benefit is zero. For eligible objects, lazy height-grid construction can still occur even though renderer traversal is compile-time-disabled.

### Recommendation

Either:

1. repair and independently verify the DDA boundary hand-off, then remove the compile-time quarantine; or
2. until that proof lands, avoid constructing/uploading DDA-only derived state for a path that cannot consume it.

The authored `heightGridDdaEnabled` property should never imply a live optimization when a hidden compile-time latch makes activation impossible; expose/represent the verified capability honestly.

---

## 5. P1 CPU — Program memoization copies the cached Program on every draw

`WebGpuRenderer::drawImplicit` memoizes generated WGSL and pipeline lookup by `memoId` + revision.

That is structurally good.

But on a cache hit the implementation assigns:

`prog = entry.prog;`

where `Program` owns:

- the complete WGSL source string;
- the parameter vector;
- error string;
- flags.

A static SDF therefore avoids WGSL regeneration and GPU pipeline compilation, yet can still copy the complete generated program representation into a local object every frame.

For large generated shaders this is unnecessary allocation/memory traffic in the CPU draw path.

### Recommendation

Do not copy the structural program on hits.

Use a stable cached program reference/pointer, or split the representation:

```
CompiledSdfStructure
  structural WGSL / module identity
  pipeline
  parameter layout
  structural feature flags

SdfInstanceValues
  numeric params
  transform
  material
  runtime bounds
```

A frame should ordinarily gather stable structural handles and current dynamic values, not duplicate shader source.

---

## 6. P1 CPU — One field revision conflates structure changes with value changes

Earthcall's codegen architecture explicitly separates:

- **tree structure** -> WGSL source / pipeline;
- **numeric parameters** -> GPU parameter buffer.

But `Object::rebuildGeometryCaches()` increments one `_fieldRevision`, and `drawImplicit` uses that revision as the memo invalidation for the complete `sdfwgsl::compile()` result.

This means mutations that only change numeric values can invalidate the same cache used for structural shader topology.

Even when the regenerated WGSL is character-identical and `sdfPipeline()` ultimately finds the old GPU pipeline, Earthcall has already rerun the CPU code generator and reconstructed the Program representation.

### Recommendation

Separate at least:

- `fieldStructureRevision`
- `fieldParameterRevision`

or, preferably, attach revisions to the actual dependency graph feeding derived artifacts.

Structural mutation invalidates WGSL/pipeline shape. Value mutation invalidates only the compact parameter block and any derived bounds depending on that value.

This is directly aligned with Zach's stated "everything caching" / Prophetic tracking direction.

---

## 7. P2 — Static SDF values are streamed to the GPU again every frame

The current batching path is already much better than per-object driver allocation:

- instances are grouped by SDF pipeline;
- one instanced draw is issued per pipeline group;
- `GpuBufferPool` retains resident chunks rather than creating/releasing buffers per frame.

However, `flushSdfDraws()` still rebuilds and uploads every frame:

- SDF global uniforms;
- parameter storage;
- instance storage;
- height-grid storage (or one dummy cell);
- bind groups for the frame.

`GpuBufferPool::suballocateStorage` ultimately calls `wgpuQueueWriteBuffer` for provided data.

So buffer **allocation** is cached, but static buffer **contents and bindings** are not.

### Recommendation

Move toward a persistent SDF GPU arena:

- stable object/template allocation;
- dirty-range uploads;
- static parameter ranges remain resident;
- dynamic transforms/materials use a separate compact instance stream;
- height grids remain resident by derived-state revision;
- bind groups persist until their buffer generation/layout changes.

The renderer can still batch by pipeline, but batching should reference persistent slices rather than repack all source data every frame.

---

## 8. P2 — Derived truths are recomputed inside the frame path

Several calculations in or immediately upstream of `drawImplicit` are revision-derived facts, not inherently per-frame facts.

### 8.1 Inverse model matrix

`drawImplicit` computes:

`inst.invModel = glm::inverse(_model);`

for every SDF draw.

For static transforms this repeats an unchanged matrix inverse every frame.

Cache inverse transform on transform revision or compute it where transform mutation occurs.

### 8.2 Heightfield structural proof

`drawImplicit` calls:

`geom::isHeightfieldExpr(field, nullptr)`

every draw.

Whether an authored tree structurally proves `y-h(x,z)` changes when the field structure changes, not when the camera moves. Cache the proof result beside the SDF structure revision.

### 8.3 Derived SDF construction for smooth/complex objects

`ObjectRender.cpp` reconstructs SDF values such as:

- `geom::sdfFromSmooth(smoothData)`
- `geom::sdfFromComplex(complexData, field)`

inside rendering.

Those are derived from geometry properties and should be cached at geometry revision boundaries, then shared by rendering, collision where applicable, and compiler memoization.

### Architectural rule

The render frame should become a consumer of already-established derived truth.

It should not repeatedly rediscover invariant facts merely because another frame began.

---

## 9. P2/P3 — Program and pipeline caches have no ordinary lifetime eviction

`_programCache` and `_sdfPipes` are cleared on shader reload/shutdown, but otherwise persist.

A long authoring session that creates/destroys many unique field structures can therefore retain dead:

- generated WGSL strings;
- parameter vectors embedded in memoized Program objects;
- pipelines;
- bind-group layouts.

The September 3 rendering audit already identified unbounded `_programCache` growth; current HEAD retains the same lifetime shape.

### Recommendation

Prefer lifetime/reference tracking attached to structural field identities. If that cannot be made exact immediately, use a bounded LRU with observability exposed through ScreenChannel.

Do not silently evict a structure that an active object still expects; cache ownership must be explicit.

---

## 10. P1 observability — correctness tests are substantially better than performance gates

The rendering substrate now has valuable correctness coverage, including:

- WGSL/CPU parity;
- SDF distance behavior;
- heightfield sweep tests;
- GPU buffer-pool tests;
- object rendering tests.

But performance regressions can still escape because there is no equivalent persistent SDF performance contract that exercises the workload which historically failed.

### Needed benchmark matrix

Add a repeatable Release benchmark with at least:

- trivial field;
- expensive authored Perlin/OntoMath field;
- Expr nested under CSG;
- many identical analytic objects to exercise batching;
- many unique structures to exercise cache behavior.

Camera cases:

- down;
- 45 degrees;
- horizon/grazing.

Resolution cases:

- 512×512 diagnostic baseline;
- Retina-class/native high-resolution workload.

Record:

- GPU render timestamp;
- CPU SDF gather/codegen time;
- number of `sdfEval` / value+gradient evaluations where diagnostic instrumentation permits;
- bytes uploaded;
- buffer suballocations;
- number of structural compiles;
- cache hit/miss counts;
- pipeline count.

A single FPS value is too downstream to identify whether the regression is shader ALU, pixel count, codegen churn, upload churn, or unrelated Law execution.

---

## 11. Recommended implementation order

### Rung A — low-risk, pixel-identical CPU waste removal

1. Stop copying cached `Program` objects on every draw.
2. Split structural revision from parameter/value revision.
3. Cache derived SDF trees, heightfield proof results, and inverse transforms.
4. Add structural-compile and cache-hit telemetry.

These changes should alter no rendered pixel.

### Rung B — persistent GPU state

5. Separate static SDF parameter storage from dynamic per-frame instance values.
6. Keep static parameter/grid slices resident by revision.
7. Update dirty ranges only.
8. Reuse bind groups where the underlying allocation generation has not changed.

Again, rendered mathematics remains unchanged.

### Rung C — exact evaluation cost reduction

9. Expand analytic value+gradient propagation through the SDF tree.
10. Measure before assuming derivative codegen is the dominant terrain win.
11. Repair height-grid DDA boundary correctness for genuinely proven heightfields.

### Rung D — the major frontier mechanism

12. Build a conservative interval/min-max spatial hierarchy for generic authored fields.
13. Treat unknown cells as exact-march fallbacks.
14. Invalidate/rebuild hierarchy nodes from actual mathematical dependencies rather than whole-world/frame invalidation.
15. Benchmark against the authored Perlin workload at native resolution.

This is the mechanism most directly aligned with the measured diagnosis: **fewer expensive field evaluations along long rays**.

---

## 12. Architectural synthesis: spatial Prophetic evaluation

Earthcall's rendering problem and its Law problem are converging on the same deeper computational principle.

Prophetic Rete asks:

> Which facts and relations are proved relevant enough that exact rule work could matter?

The SDF hierarchy should ask:

> Which regions of space are proved capable of containing the authored zero set, such that exact field work could matter?

Both are forms of relevance compilation.

The future pipeline need not be "generate a giant WGSL stream, then ask every fragment to repeatedly rediscover the world." It can become:

```
authored OntoMath / SDF structure
        ↓
dependency + structural analysis
        ↓
cached compiled mathematical program
        ↓
cached conservative spatial relevance structure
        ↓
ray traverses proved-relevant regions only
        ↓
exact authored evaluation where uncertainty remains
        ↓
incremental GPU state updates only where dependencies changed
```

That is a much closer computational image of Earthcall's ontology: preserve the thing as what it is, but stop re-deriving everything about it everywhere and every frame.

## Status

This document is an **audit**, not an implementation claim.

Source-level findings were verified against current `sync-from-earthcall-main` HEAD `724dd256aa0599caba63aa68c52352c52de6349f`.

Performance numbers cited above are prior measurements already recorded in the repository, especially the August 31 and September 5 Perlin probes. They were not rerun during this audit session. Any implementation should re-establish a quiet Release baseline before claiming a new speedup.

---

**Signed:** GPT-5.6 Sol  
**Session:** `chatgpt-2026-09-18-sdf-pipeline-audit`  
**Date:** 2026-09-18  
**Timestamp:** 2026-09-18T23:57:00-07:00
