# Perlin Noise Floor Rendering Regression Audit

**Status:** Audit only; no renderer, save, configuration, or test behavior changed  
**Requested by:** Zach  
**Observed symptom:** approximately 8 FPS after recent optimization attempts; F3 `3D rendering phase` rapidly oscillating through roughly 1, 100, and 300 ms  
**Authored intent:** Zach wants to inhabit the Perlin Noise Floor hill zone and build a house there without the GPU repeatedly freezing the experience  
**Date:** 2026-09-05

## Executive conclusion

The regression is reproducible. At a 2880x1800 Retina framebuffer, the current Perlin
floor costs approximately **117.83 ms per frame at a 45-degree view**, or about **8.5
FPS**. A trivial implicit plane under the same conditions costs 10.44 ms. The dominant
cost is therefore the exact classic-Perlin implicit-field fragment workload multiplied
across a large screen-filling proxy and 5.18 million pixels, not object count or ordinary
C++ scene traversal.

The current heightfield fast path must not be treated as the solution. The saved field's
noise expression reads `y`, while the fast path assumes the expression subtracted from
`y` is independent of `y`. That proof is false for the authored mathematics and can
permit steps that skip roots. Performance work must fail open to the generic exact
renderer whenever its preconditions cannot be proved.

The strongest measured low-risk candidate is to ensure that only one proxy face launches
the analytic ray per covered pixel. A temporary back-face-culling experiment preserved
the generic marcher's pixel hashes in the five existing camera cases and reduced the
native-resolution generic path by about 15 percent. The much larger measured speedup when
combined with the current fast path cannot be accepted because that combination changed
pixels. Correct camera-inside handling and wider parity tests are prerequisites.

## Human thread and authorship

Zach supplied both the governing end and the key live observation: the older 40 ms figure
was stale, current performance was approximately 8 FPS, and the F3 rendering measurement
oscillated sharply rather than remaining at one steady cost. This audit followed that
observation back through the timer, WebGPU queue boundary, SDF proxy, marcher, exact
Perlin program, selection path, and the saved terrain expression.

The ranking, controlled probes, diagnosis of the invalid heightfield proof, and proposed
optimization frontier are Codex's analysis within that human-authored end. Nothing in
this audit authorizes changing the terrain expression or its save. If Zach intends the
surface to be an ordinary single-valued `h(x,z)` hill, that is an authoring decision for
Zach rather than a renderer optimization inferred by the engine.

## Governing discipline

This audit applies `docs/ENGINEERING_DISCIPLINE.md` and the repository's ontology:

1. Run the real path; source inspection alone is not verification.
2. Preserve end-to-end coherence between authored OntoMath, rendering, collision, and
   interaction.
3. Prefer falsifying measurements before implementation.
4. Derive bounds; do not pick constants that merely make a frame faster.
5. Treat rendering as a `Singularity/Screen` channel reading authored mathematics, never
   as an authority that changes what the terrain is.
6. Preserve the save and its authorship. No save-file mutation occurred.

## Pipeline traced

`Engine::render()` measures the complete 3D phase with a CPU wall clock. The phase enters
`EngineRender`, records the scene's mesh and field draws, submits them through
`WebGpuRenderer`, and then returns. The Perlin floor is one `ShapeKind 10` implicit object
whose world-space extent is `[1000, 30, 1000]` and whose field is:

```text
y - 40 * noise(0.008 * (p + vec3(100, 0, 100)))
```

The WebGPU path rasterizes a twelve-triangle bounding cube. Each covered fragment derives
a ray, intersects the analytic box, and marches up to 192 steps. The generated WGSL
evaluates the same classic Perlin construction used by the CPU. Generic steps evaluate
the field at the current position plus three forward finite-difference positions; a hit
then performs four more evaluations for its normal.

Relevant implementation points:

- `src/Singularity/Core/Engine.cpp` — CPU wall timer around the 3D phase.
- `src/Singularity/Core/EngineRender.cpp` — far plane, object traversal, field draw, and
  surface/submit timers.
- `src/Singularity/FirstMoverOntology/FirstMoverWindowTools/PerformanceMetricsWindow.cpp` — F3 attribution.
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp` — proxy rasterization, SDF marching,
  height-grid upload, batching, and submission.
- `src/ConstructedBeing/Singular/Object/Geometry/Sdf.cpp` — heightfield recognition and grid bounds.
- `src/ConstructedBeing/Object/ObjectRender.cpp` and `ObjectCollision.cpp` — selection
  outline and lazy collision-mesh construction.

## Controlled measurements

The repository's existing horizon probe was compiled to `/tmp` and run through native
Metal. A temporary native-resolution variant was also built in `/tmp`. These artifacts
did not modify repository files.

### Resolution and field cost

| 2880x1800 view | Measured frame submission time |
|---|---:|
| Empty scene | 1.28 ms |
| Trivial field `y`, horizon | 13.03 ms |
| Trivial field `y`, 45 degrees | 10.44 ms |
| Actual Perlin field, looking down | 1.46 ms |
| Actual Perlin field, horizon | 80.64 ms |
| Actual Perlin field, 45 degrees | **117.83 ms** |

At 512x512, the actual Perlin horizon and 45-degree views were approximately 6-8 ms.
The high-DPI result reproduces the reported 8 FPS and shows that the binding dimension is
screen coverage multiplied by expensive exact field evaluations.

### Height-grid comparison

The existing 128x128 height-grid sweep passed its five low-resolution camera views.
Alternating grid/no-grid measurements after warm-up showed effectively no benefit for
the actual saved field. This is expected: because the noise reads `y`, the conservative
vertical slack makes most grid cells unable to prove that a ray segment is empty.

The renderer nevertheless uploads roughly 128 KiB of height-grid data each frame for
this field. That upload is not the primary 100 ms cost, but it currently buys no measured
acceleration in the reproducer.

### Proxy-face experiment

The production SDF proxy uses `CullMode_None` because a camera may be inside the proxy.
A temporary build using back-face culling measured:

| Path, 2880x1800 at 45 degrees | Time |
|---|---:|
| Current fast path, no face culling | 117.83 ms |
| Current fast path, back-face culling | 34.28 ms |
| Generic marcher, back-face culling | 100.41 ms |

With the generic marcher, the hashes for all five 128x128 camera cases exactly matched
the uncullled reference. With the current heightfield fast path enabled, four views
changed and one to three lit pixels were missed. Thus duplicate proxy fragments are a
real optimization opportunity, but the dramatic fast-path number is entangled with a
correctness failure and cannot be reported as an acceptable win.

### Existing parity tests

`webgpu_sdf_parity_test` passed all 21 shapes in the current build; its noise case differed
by two lit pixels against a tolerance of ten. `webgpu_heightfield_sweep_test` also passed.
These are useful regression sentinels but do not prove root preservation for every ray
through this y-dependent field, especially at native resolution and grazing camera axes.

## Findings

### P0 — F3 does not contain a true GPU duration

F3's `3D rendering phase` is a CPU wall-clock interval around command recording and queue
submission. WebGPU submission is asynchronous. Several frames may appear cheap while the
GPU accumulates work, followed by a surface acquisition or queue-submission stall when
backpressure becomes visible to the CPU. The reported 1 -> 100 -> 300 -> 100 -> 1 ms
oscillation is consistent with this behavior, but the current instrumentation cannot
attribute the accumulated GPU cost to mesh, SDF, ImGui, or presentation.

Before another optimization campaign, add optional WebGPU timestamp queries around the
mesh, SDF, and UI passes, with delayed nonblocking readback, plus separate queue and
surface wait reporting. Instrumentation must not serialize every frame merely to obtain
a cleaner number.

### P0 — the current heightfield proof is invalid for the saved field

`isHeightfieldExpr()` recognizes a syntactic `Sub(y, h)` without proving that `h` is
independent of `y`. `WebGpuRenderer` then treats the existence of a height grid as proof
that `f = y - h(x,z)` has `df/dy = 1`, skips the generic gradient sampling, and permits
more aggressive stepping.

For the saved expression, `h` contains `noise(...p...)`, so it reads `p.y`. The claimed
derivative is not one, and the field need not be single-valued vertically. This can skip
roots and alter the surface. The test suite currently codifies the syntactic
classification rather than the mathematical prerequisite.

The screen control is also misleading: disabling the height-grid upload leaves the
`isProvenHeightfield` fast-path classification active. The tooltip's claim that Off is the
unmodified per-step marcher is therefore false.

Required direction: prove independence from `y` recursively before selecting any true
heightfield algorithm; otherwise fail open to the generic field renderer. Add adversarial
multi-root and grazing-ray tests, not only silhouette-tolerance tests.

### P1 — both proxy faces can launch redundant analytic rays

The fragment shader computes the analytic box interval from the eye; it does not need two
independent invocations for the front and back rasterized faces covering the same pixel.
`CullMode_None` avoids the camera-inside disappearance case but can nearly double the
expensive fragment work.

The frontier implementation should establish one invocation per covered pixel with
explicit outside/inside behavior. Deriving the ray from screen position and inverse
view-projection would remove its dependence on which proxy face rasterized. Scissoring or
a conservative projected proxy may then be used, provided boundary parity and depth
semantics are tested.

### P1 — exact classic Perlin is the dominant arithmetic cost

The 70-107 ms delta between the trivial and actual fields is the dominant measured cost.
Replacing classic Perlin with simplex noise, a repeating volume texture, or a quantized
bake would change the authored function and recreate the earlier rendering/collision
split. Those are not optimizations Earthcall may accept.

Truth-preserving candidates include:

- Cache or table the exact permutation/gradient lattice inputs, then retain the same
  quintic interpolation and output function.
- Specialize only when the compiler proves an affine input to `Noise`; fail open for all
  other expressions.
- Emit exact value-and-gradient work together so lattice calculations can be reused by
  stepping and hit normals.
- Verify CPU/GPU numeric values and roots, not only approximate silhouettes.

The current `kPerlinLipschitz = 6` is documented as an empirical bound with margin rather
than a closed-form proof. It must not become the basis for a segment-skipping guarantee
until derived rigorously from the implemented function.

### P1 — selecting the floor forces a large CPU mesh

Picking a field whose enormous bounding box catches the ray can lazily invoke
`rebuildFieldMesh()`. Highlight rendering also invokes that mesh and uploads/draws its
outline shells. For the floor, this is a 128x24x128 marching-tetrahedra domain plus its
triangle grid/support cloud. It can cause a substantial first-selection hitch and
sustained outline cost exactly while Zach is trying to place and manipulate a house.

Selection, picking, and highlighting should consume the same implicit mathematics where
possible, or a screen-space result derived from it. They should not require materializing
a large collision mesh merely to communicate which authored being is selected.

### P2 — the acceleration structure does not match this field

A 2D height range over `(x,z)` is poorly suited to an expression whose value varies with
`y` inside its noise term. A future conservative 3D interval/min-max structure could skip
only segments proved not to contain zero while preserving arbitrary authored fields.
Any interval hierarchy must derive conservative bounds from the AST and use invalidation
tied to the field revision.

### P3 — populated-house geometry is a later scaling concern

Cubes already benefit from merged meshes and cross-object instancing. Identical general
polyhedra do not necessarily batch because the batch key includes the mesh pointer, and
broader scene culling is limited. Canonical geometry caching, frustum culling, and
truth-preserving instance batching will matter once the zone contains a house, but they
cannot explain an 8 FPS zone containing only the Perlin ground.

## Optimization order recommended

1. Add nonblocking GPU timestamps and preserve current CPU/surface/submit measurements.
2. Strengthen the oracle: numeric CPU/GPU field parity, adversarial root tests,
   camera-inside/outside proxy coverage, grazing rays, and native-resolution probes.
3. Remove or constrain the false heightfield classification so every optimization has a
   proved precondition.
4. Establish exactly one analytic-ray invocation per covered pixel.
5. Reuse the exact Perlin lattice/value/gradient computation without substituting a
   different function.
6. Remove the forced collision-mesh build from field selection and highlighting.
7. Evaluate conservative 3D interval acceleration, then populated-zone mesh batching and
   culling.

## Explicit refusals

This audit does not recommend:

- changing the saved formula from `noise(p)` to `noise(x,0,z)` without Zach's explicit
  authorial decision;
- swapping classic Perlin for a visually similar noise;
- lowering ray steps or raising a minimum step without a root-preservation proof;
- making collision, picking, and rendering evaluate different surfaces;
- silently reducing Retina resolution while claiming identical representation;
- widening a performance baseline to make the regression disappear.

If Zach confirms that the intended ground is an ordinary single-valued heightfield, a
separately authorized save migration could make direct height traversal legitimate and
very fast. That would be a change in authored meaning, recorded with its author—not a
renderer patch.

## Audit limitations

- Timings are queue/submission wall times on this machine, not GPU timestamp-query data.
- Temporary culling variants were experiments in `/tmp`, not production-ready changes.
- Five low-resolution hash views do not prove all camera/proxy cases.
- No interactive selection feel-test was performed during this audit.
- The repository worktree already contained unrelated modified and untracked files; they
  were preserved and were not used as permission to alter save or source state.

## References

- `docs/ENGINEERING_DISCIPLINE.md`
- `docs/architecture/mathematics/ONTOMATH_FRAMEWORK.md`
- `docs/architecture/ontology/NO_BLACK_BOX.md`
- `docs/audits/rendering_optimization/RENDERING_OPTIMIZATION_CAMPAIGN_REVIEW_2026-08-31.md`
- [WGSL specification, fragment depth](https://www.w3.org/TR/WGSL/)
- [Apple: Calculating primitive visibility using depth testing](https://developer.apple.com/documentation/metal/calculating-primitive-visibility-using-depth-testing)

---

**Signed:** Codex  
**Session:** `01a072e2-017b-7b03-aa4a-1ef25dab65d1`  
**Timestamp:** 2026-09-05T11:59:28-07:00

## Temporal-coherence correction — 2026-09-05T19:18:00-07:00

Zach's moving-world witness overturned the interim diagnosis. The Flash-like image
phasing affected only things whose pixels changed position: the camera moved every world
pixel, falling objects ghosted only while falling, and a dragged ImGui window left
translucent copies along its path. Static content did not phase. This rules out the
authored Perlin expression, height-grid/DDA eligibility, culling, and locomotion as the
common cause.

The timestamp implementation was the remaining shared recent renderer/device change. It
requested native timestamp features at device creation, wrote timestamp queries around
each main encoder, and processed asynchronous callbacks during frame setup. Suspending
that entire optional instrumentation path restored coherent motion in Zach's immediate
in-app verification. The experiment identifies the *instrumentation bundle* as causal;
it does not yet distinguish feature negotiation, query encoding, or callback processing.
Accordingly, GPU-query values are not admissible evidence for this optimization campaign
until each of those mechanisms is isolated and revalidated with moving 3D and ImGui
content. The existing CPU recording, acquire, and submit measurements remain available.

**Addendum signed:** Codex
**Session:** `01a072e2-017b-7b03-aa4a-1ef25dab65d1`
**Timestamp:** 2026-09-05T19:18:00-07:00
