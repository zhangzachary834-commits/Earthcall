# Implementation Plan — Truth-Preserving Perlin Noise Floor Rendering Recovery

**Status:** implementation in progress; first correctness slice landed  
**Requested by:** Zach  
**Author:** Codex  
**Session:** `01a072e2-017b-7b03-aa4a-1ef25dab65d1`  
**Timestamp:** 2026-09-05T12:05:42-07:00  
**Audit basis:** [`2026-09-05_perlin_noise_floor_rendering_regression_audit.md`](../audits/rendering_optimization/2026-09-05_perlin_noise_floor_rendering_regression_audit.md)  
**Existing task:** [`The_horizon_frame_is_now_the_ceiling_and_the_cost_is_FIELD.md`](../Agenda/Tasks/Specific%20Tasks/The_horizon_frame_is_now_the_ceiling_and_the_cost_is_FIELD/The_horizon_frame_is_now_the_ceiling_and_the_cost_is_FIELD.md)

## Implementation status — 2026-09-05

The first correctness slice is implemented:

- `isHeightfieldExpr` now proves the right-hand subtree is independent of ambient `y`;
  the saved `noise(p)` floor therefore takes the generic exact path.
- Empirical Perlin Lipschitz data no longer authorizes a grid skip. Noise heightfields
  fail open until a closed-form bound is derived.
- Heightfield damping no longer upgrades `y-h(x,z)` to distance-field stepping.
  Native grazing-camera mismatches quarantined DDA traversal entirely; the grid is
  retained only as a conservative cache/proof seam until the traversal is repaired.
- The SDF proxy now back-face-culls the shared inward-wound cube, eliminating the
  duplicate front/back fragment invocation while retaining the analytic ray/AABB path.
- The shader now carries the heightfield proof bit separately from the generic
  gradient-march damping, so Perlin expressions cannot inherit heightfield-only
  vertical early exits. When a proved grid is active, its proxy interval is kept
  identical to the interval used to derive the CPU cell bounds.
- F3 now labels the 3D phase, surface acquire, and queue-submit values as CPU
  wall-clock observations and explains that they are not GPU execution timestamps.
- Predicate coverage includes y hidden in vector components; the GPU sweep includes
  inside-proxy cameras and uses a synthetic linear field whose grid cache has a
  compositionally derived bound.

`heightfield_predicate_test`, native-Metal `webgpu_heightfield_sweep_test`, and native-Metal
`webgpu_sdf_parity_test` pass; `earthcall_webgpu` builds. The sweep is byte-identical over
six camera cases, including a camera inside the proxy. No performance claim is made until
the fixed native-resolution Perlin trace is rerun with GPU timing instrumentation.

## 1. Outcome

Make the Perlin Noise Floor hill zone continuously inhabitable and responsive while Zach
builds a house there, without changing the authored field, allowing rendering and
collision to disagree, or hiding failure behind a lower-quality substitute.

The current measured reference is **117.83 ms** for the 3D phase at 2880x1800 and a
45-degree camera view, approximately 8.5 FPS. The user-facing completion aspiration is a
stable **60 FPS-class interaction** at that native resolution: SDF GPU p95 at or below
16.7 ms on the audited machine, no recurring 100-300 ms queue-debt spikes during the
defined camera trace, and no selection hitch large enough to break house placement.
These are performance goals supplied by the desired human experience, not geometric
bounds and not permission to weaken the represented terrain.

If the exact renderer cannot meet that target after the measured phases below, the plan
must report the remaining cost honestly and advance to proved conservative acceleration.
It must not silently reduce resolution, substitute a different noise, or mutate the save.

## 2. Non-negotiable invariants

1. **The save is unchanged.** The audited expression remains:
   `y - 40 * noise(0.008 * (p + vec3(100, 0, 100)))`.
2. **One mathematics.** Rendering, collision, picking, and physics continue to interpret
   the same OntoMath expression and classic Perlin function.
3. **Optimization predicates are proofs.** A specialized path may be selected only when
   its complete preconditions are derived from the AST. Unknown means generic exact path.
4. **Bounds are conservative and derived.** No arbitrary horizon, minimum march step,
   assumed Perlin range, or empirical Lipschitz value may decide that a root cannot exist.
5. **No ontology expansion.** This work adds no domain noun, `BeingKind`, `ShapeKind`,
   top-level subsystem, `Body`, or hard-coded authored category.
6. **No black-box setting theater.** GPU handles, query sets, staging buffers, and queue
   fences are Kernel state and must be named as such in comments. Any world-addressable
   Screen state follows the existing property-registration and transfer-policy path.
7. **No save migration by inference.** Converting the field to `noise(x,0,z)` is outside
   this plan unless Zach separately authors that change.
8. **Transparent failure.** If a GPU lacks an optional timing or depth feature, Earthcall
   reports the unavailable measurement and continues rendering; it does not fabricate 0.

## 3. Work order

```text
Measurement + oracle
        |
Restore valid specialization boundary
        |
One analytic ray per covered pixel
        |
Exact Perlin work reuse
        |
Selection/highlight without terrain mesh hitch
        |
Proved interval acceleration, only if still needed
        |
Populated-zone mesh and occlusion scaling
```

Each phase ends with a stop/go gate. Do not begin a later optimization merely because it
is already described in an older plan.

## 4. Phase 0 — establish a trustworthy GPU measurement and oracle

### Implementation

1. In `WebGpuContext`, request timestamp-query capability only when the adapter exposes
   it. Record capability explicitly; preserve startup on adapters that do not.
2. In `WebGpuRenderer`, add timestamp boundaries for mesh work, implicit/SDF work, UI if
   it shares the renderer, and total submitted GPU work.
3. Resolve timestamps through a ring of staging buffers at least two frames behind the
   producer. Poll only already-completed mappings so measurement cannot serialize the
   queue or create the hitch it is meant to observe.
4. Keep the existing CPU recording, surface-acquisition, and queue-submission times.
   Rename F3 labels so CPU wall time is never presented as GPU execution time.
5. Extend `scratch/probes/horizon_cost_probe.cpp` with explicit resolution, camera case,
   warm-up count, sample count, p50/p95/p99, and machine/build metadata. Preserve the
   authored expression as the primary case and the empty/trivial fields as controls.
6. Add a deterministic correctness capture containing color/coverage, depth, field value
   at recovered hit points, and miss/hit classification. CPU reference checks must sample
   the same expression rather than a terrain-shaped stand-in.

Likely files:

- `src/Singularity/Screen/WebGPU/WebGpuContext.hpp`
- `src/Singularity/Screen/WebGPU/WebGpuContext.mm`
- `src/Singularity/Screen/WebGPU/WebGpuContext_wasm.cpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`
- `src/Singularity/FirstMoverOntology/FirstMoverWindowTools/PerformanceMetricsWindow.cpp`
- `scratch/probes/horizon_cost_probe.cpp`
- WebGPU timing/parity tests under `tests/singularity/`

### Gate 0

- Native Metal reports separate SDF GPU and CPU submit/wait durations.
- Unsupported adapters visibly report timing as unavailable.
- Enabled timing changes median frame cost by no more than 1 percent or 0.1 ms, whichever
  is larger; otherwise timing remains a probe-only facility until made nonperturbing.
- Three repeated runs of the fixed camera trace are stable enough to distinguish a
  10-percent change at p95.
- The current 8 FPS case is reproduced before any renderer optimization lands.

## 5. Phase 1 — restore the valid specialization boundary

This is a correctness repair and may initially increase frame time. Performance cannot
be credited to a path whose mathematical premise is false.

### Implementation

1. Replace the syntactic `Sub(y, expression)` heightfield classification with recursive
   coordinate-dependence analysis. A true heightfield requires the right subtree to be
   independent of `y`; uncertainty returns false.
2. Keep dependency analysis structural and general over existing OntoMath nodes. Do not
   create a new authored kind or enum value for terrain.
3. Separate these facts in renderer state:
   - the expression is proved `y - h(x,z)`;
   - a conservative grid exists for the current field revision;
   - grid traversal is enabled for a diagnostic/probe run.
   None may imply another accidentally.
4. Correct the F3/Screen tooltip: grid Off must mean the generic marcher actually runs.
5. For the current y-dependent save, do not construct or upload the ineffective 2D
   height grid unless another consumer demonstrably needs it.
6. Retain the current generic exact path as the reference path during the remaining
   phases.

### Tests

- Update `tests/singularity/heightfield_predicate_test.cpp` so the saved expression is
  rejected as a true heightfield.
- Add positive nested-expression cases that are independent of `y` and negative cases
  where `y` is hidden inside vectors, transforms, noise, conditionals, or reusable nodes.
- Add a y-dependent multi-root field that the former fast path would skip.
- Assert that toggling grid traversal changes path selection exactly as the UI states.
- Require hit/miss and depth agreement with the generic reference across outside,
  inside, horizon, grazing, and near-plane cameras.

### Gate 1

- No specialized path is selected for the Perlin Noise Floor save.
- Zero root/hit classification mismatches against the generic reference in the expanded
  deterministic suite.
- Any expected performance regression from removing the invalid fast path is recorded,
  not disguised by changing the baseline.

## 6. Phase 2 — one analytic-ray invocation per covered pixel

### Implementation

1. Derive the world ray from fragment position and inverse view-projection rather than
   interpolated proxy-face position.
2. Select one rasterized face orientation based on whether the camera is outside or
   inside the field AABB:
   - outside: render the entry-facing surface only;
   - inside: render the exit-facing surface only.
3. Use the analytic ray/AABB interval as the sole traversal interval. Handle camera-on-
   boundary and near-plane cases explicitly rather than with a guessed epsilon that can
   erase coverage.
4. After the one-face path is correct, evaluate a conservative screen scissor or projected
   proxy rectangle. Keep it only if it reduces measured fragments and preserves boundary
   coverage.
5. Preserve authored depth and mesh/SDF ordering. If conservative fragment-depth modes
   are considered, negotiate the optional feature and prove the required depth relation
   before declaring it; otherwise retain ordinary `frag_depth` behavior.

### Tests

- Pixel hash, hit mask, linear depth, and sampled hit-point value against the generic
  uncullled reference.
- Cameras outside, inside, exactly on every AABB face, crossing the near plane, and
  looking along each principal and diagonal axis.
- Thin boxes, camera starting on the zero set, overlapping opaque mesh, and Retina-sized
  boundary captures.
- A counter records SDF fragment invocations in the probe; overlapping front/back proxy
  coverage must disappear.

### Gate 2

- Zero missing or added hits in the exact coverage suite.
- Depth agrees within a tolerance derived from the current float and depth formats.
- At least a repeatable 10-percent SDF GPU p95 reduction in the native Perlin trace.
  Otherwise revert the complexity and retain the measurement result in the audit trail.

## 7. Phase 3 — reuse exact Perlin work

The goal is fewer arithmetic operations for the same function, not a similar-looking
function.

### Investigation before implementation

1. Capture shader compiler output or available GPU counters to determine whether the four
   nearby finite-difference samples already share lattice/hash work.
2. Measure separate variants in the probe; do not infer a four-times saving from source
   call count.
3. Identify affine inputs to `Noise` in the AST. Specialized code generation is allowed
   only when that form is proved and every transform/revision participating in the cache
   key is included.

### Candidate A — fused exact value/gradient evaluation

Generate one routine that computes the classic-Perlin lattice/hash data once and reuses
it for value and derivative interpolation. Use the resulting gradient for safe stepping
only where the relevant bound is proved; otherwise use it for the normal and retain the
generic conservative step.

### Candidate B — exact discrete lattice metadata cache

For proved affine noise domains, cache only the discrete permutation/gradient metadata
that the exact function would calculate. Perform the same quintic fade and interpolation
at runtime. Do not cache quantized noise values, use hardware trilinear interpolation as
a substitute, impose an authored-space repeat period, or assume the current save's domain
will never change.

Cache invalidation must include field revision, affine transform, relevant spatial domain,
and the exact noise implementation revision. Cache storage is Kernel state and is named
as such; it is not a second authored property system.

### Tests and Gate 3

- Compare CPU and GPU field values at lattice boundaries, just to either side of them,
  random interior points, negative coordinates, and the complete saved terrain domain.
- Compare hit/miss, depth, and normal orientation along the deterministic camera corpus.
- Zero changed classifications; numeric tolerances are derived from f32 operation order.
- Keep a candidate only if it independently reduces native SDF GPU p95 by at least 10
  percent. Choose the smaller, more general implementation when measured results overlap.

## 8. Phase 4 — remove the terrain-mesh selection hitch

### Implementation direction

1. Add an object/selection identifier attachment to the existing exact render result, or
   an equivalent channel-local representation, so the already-computed field hit can
   identify the selected being.
2. Generate the selected outline from that identifier/depth boundary in a lightweight
   screen-space pass. Do not call `rebuildFieldMesh()` merely to draw two outline shells.
3. For click selection, investigate a one-pixel asynchronous identifier/depth readback
   from the frame already shown. Reconstruct the world hit from the recorded camera
   matrices and validate it against the same OntoMath expression.
4. Keep collision-mesh construction owned by collision/physics consumers. Selection must
   not accidentally become a hidden eager collision build.
5. Review the Screen/Input boundary before implementation. Do not introduce a domain
   `SelectionResult` class or a second authority path for object identity.

### Gate 4

- Selecting and outlining the terrain does not invoke `rebuildFieldMesh()`.
- The selected identifier and reconstructed point correspond to the visible exact hit.
- First-click p95 stays below the human response target agreed with Zach; the plan uses
  50 ms as the initial measurement threshold, not as an ontological constant.
- Cubes placed on the floor still settle against the collision interpretation already
  governed by the shared expression.

## 9. Phase 5 — proved conservative field acceleration, if required

Proceed only if Phases 1-4 leave SDF GPU p95 above the interaction target.

1. Derive interval and derivative bounds compositionally from OntoMath. Replace the
   empirical Perlin Lipschitz margin with a proof for the exact implemented function
   before it may exclude a segment.
2. Build a revision-keyed 3D interval hierarchy for arbitrary fields. A node may be
   skipped only when its conservative interval excludes zero for the entire ray segment.
3. Preserve a distinct optimized 2D traversal for expressions proved to be true
   `y-h(x,z)` heightfields; the current save will not take it.
4. Compare traversal against exhaustive/reference root isolation on adversarial fields:
   high frequency, thin shells, multiple vertical roots, transformed noise, smooth
   combinations, and interval boundaries.
5. Publish counters for visited and skipped cells in the probe so a hierarchy that cannot
   prove useful skips is not uploaded every frame.

### Gate 5

- Every skip has a conservative mathematical justification traceable to the AST.
- No false-empty interval in property/randomized testing over the supported nodes.
- Repeatable native-resolution improvement after hierarchy build/upload costs.
- If interval dependency makes the structure ineffective, stop and report that result;
  do not narrow the range until it becomes fast.

## 10. Phase 6 — scale from an empty hill to a lived-in hill

After the one-field ceiling is removed:

1. Add frustum culling for mesh instances using existing geometry bounds.
2. Canonicalize immutable geometry resources so identical authored cubes/polyhedra can
   share GPU meshes and batch keys without conflating the beings themselves.
3. Measure whether foreground house geometry actually prevents SDF fragment execution.
   Because the SDF shader discards and writes depth, do not assume automatic early-Z.
4. Evaluate a mesh depth prepass or conservative depth declaration only with real GPU
   timestamp/fragment evidence and proved depth behavior.
5. Run populated-zone scaling traces at 1, 64, 128, 256, and 512 visible objects, keeping
   the existing object-count lag work distinct from the single-field cost.

## 11. Verification matrix for every implementation pass

### Focused automated checks

- `heightfield_predicate_test`
- `webgpu_sdf_parity_test`
- `webgpu_heightfield_sweep_test`
- new root-preservation/proxy-camera cases
- renderer batching/buffer-pool tests touched by the phase
- `frame_lag_test`, interpreting `STANDING` versus `LAG` exactly as documented

### Full verification

Follow `docs/BUILD_AND_ENVIRONMENT.md`: build `earthcall_webgpu`, build the default test
target separately, and run the full `ctest` suite. Do not use the OpenGL `earthcall`
binary to verify WGSL behavior. Record the known deliberate/pre-existing failure rather
than claiming a false all-green result.

### Performance protocol

- Release and Debug are reported separately.
- Fixed 2880x1800, 60-degree FOV, identical saved field and camera trace.
- Warm-up before sampling; report p50/p95/p99 and at least three runs.
- Empty and trivial-field controls accompany every Perlin result.
- GPU SDF time, CPU recording time, surface wait, submit wait, and total frame are all
  reported separately.
- No rebaseline upward. A slower result is a result.

### Person verification after visible changes exist

Before an implementation pass closes, add the exact walk/build/selection checks to
`docs/Agenda/Tasks/Person Verification List.md`. At minimum Zach should inspect the hill
at horizon and 45-degree views, cross the proxy boundary, select the ground, place house
pieces, and confirm that the visible ground and embodied collision remain one ground.
This plan itself creates no visible runtime change, so it adds no premature verification
checkbox.

## 12. Commit and rollback discipline

- One phase per reviewable commit; measurement/oracle changes land before the optimization
  they judge.
- Retain the generic exact renderer as an oracle until all specialized paths have passed
  the expanded suite.
- Do not maintain permanent world-visible “performance mode” switches for unproved
  algorithms. Temporary comparisons live in probes or tests.
- If a phase fails its truth gate, revert that phase rather than widening tolerance.
- If a phase fails its performance gate, preserve its measurement in the audit/task and
  remove complexity that has no other demonstrated value.
- After every phase, perform the Integrity Check: identify every caller, consumer,
  property description, tooltip, test, plan, and save assumption that would otherwise lie.

## 13. Supersession and retained prior work

For the Perlin Noise Floor recovery, this document supersedes the implementation order
and unmeasured performance promises in:

- `SDF_MANIFOLD_HIGH_FPS_ACCELERATION_PLAN.md`
- `FRONTIER_MULTI_HUNDRED_FPS_SDF_ENGINE_EXPANSION_PLAN.md`
- `MULTI_HUNDRED_FPS_SDF_ENGINE_PLAN_2026-08-28.md`

Those documents remain part of the historical and technical record. Their useful
techniques may re-enter only through the gates above. In particular, the old baked RGBA8
noise proposal, half-resolution-as-default proposal, guessed horizon/minimum-step bounds,
and syntactic heightfield assumption are not authorized by this plan.

---

**Signed:** Codex  
**Session:** `01a072e2-017b-7b03-aa4a-1ef25dab65d1`  
**Timestamp:** 2026-09-05T12:05:42-07:00

## Implementation addendum — 2026-09-05T12:30:00-07:00

The implementation pass also separated the shader's structural heightfield proof bit
from its generic gradient-step damping and kept active grid coordinates aligned with the
CPU-proved extent. Focused C++ tests and all requested WebGPU targets build cleanly;
native Metal execution remains the required next gate because this host's usage-limit
policy currently prevents the escalated GPU test run.

Verification record: the full default build completed; `heightfield_predicate_test` passed
both directly and through CTest. The WebGPU sweep, parity, distance, and particle tests
report their existing no-device condition on this host. A full parallel CTest run reached
those expected no-device results and then remained in an unrelated integration test; it
was stopped rather than presented as a complete suite result.

**Addendum signed:** Codex
**Session:** `01a072e2-017b-7b03-aa4a-1ef25dab65d1`
**Timestamp:** 2026-09-05T12:30:00-07:00

## Instrumentation addendum — 2026-09-05T14:44:43-07:00

F3 now makes the observed boundary legible: its 3D number is CPU wall-clock around
recording and frame lifecycle, while surface acquisition and queue submission are shown
as separate CPU waits. This responds directly to Zach's reported rapid 1/100/300 ms
oscillation without pretending the application has GPU timestamps it does not have.

**Addendum signed:** Codex
**Session:** `01a072e2-017b-7b03-aa4a-1ef25dab65d1`
**Timestamp:** 2026-09-05T14:44:43-07:00

## Native parity addendum — 2026-09-05T14:49:48-07:00

Native Metal disproved the previous DDA traversal: candidate-cell skips lost grazing
roots in three camera cases. The traversal is therefore quarantined; it cannot upload a
grid or alter a ray. Separating AST heightfield proof from optional grid-cache presence
also restored identical proxy coverage when the cache is omitted. With traversal absent,
the six-camera cache/proxy sweep is byte-identical and the 21-shape GPU/CPU SDF parity
suite passes. This is a correctness recovery, not a claim of a Perlin frame-time win.

**Addendum signed:** Codex  
**Session:** `01a072e2-017b-7b03-aa4a-1ef25dab65d1`  
**Timestamp:** 2026-09-05T14:49:48-07:00

## Timestamp-query implementation addendum — 2026-09-05T15:26:00-07:00

Phase 0 now requests timestamp-query plus native encoder-write capability only when a
WebGPU adapter advertises both. Its readback is a four-slot asynchronous ring: it never
waits for the GPU to finish a frame just to display a measurement. F3 continues to name
the existing 3D, surface-acquisition, and submit figures as CPU wall-clock observations;
when a completed query exists it separately labels the delayed GPU main-render-pass
duration, and otherwise names the adapter limitation.

The pass does not change authored Perlin mathematics, ray traversal, resolution, or
rendered pixels. Native build and execution verification remain required before this
instrumentation may be used as performance evidence.

**Addendum signed:** Codex  
**Session:** `01a072e2-017b-7b03-aa4a-1ef25dab65d1`  
**Timestamp:** 2026-09-05T15:26:00-07:00

## Presentation-coherence addendum — 2026-09-05T16:16:00-07:00

Zach's first in-app pass after the rendering work found three distinct facts: whole-frame
past-frame-like jitter, approximately 100 submitted FPS on the internal 60 Hz Mac panel
in Sanctum, and the Perlin 1/100/300 ms CPU-phase oscillation still present. The first two
trace to the macOS surface being configured with `Immediate` presentation and
`displaySyncEnabled = NO`. That intentionally allowed the CPU to submit ahead of scanout;
the counter was not a display cadence and the compositor could expose tearing.

The live surface now uses WebGPU FIFO and CAMetalLayer display sync. This preserves a
high-refresh external monitor's own cadence while requiring coherent frames on the
internal panel. It changes no authored being, mathematics, or ray path.

The third finding remains the actual performance failure: it is queue debt from the
native-resolution exact Perlin workload, not repaired by renaming F3 values or by
presentation pacing. The next implementation pass must reduce that exact workload while
preserving roots; no improvement is claimed by this addendum.

**Addendum signed:** Codex
**Session:** `01a072e2-017b-7b03-aa4a-1ef25dab65d1`
**Timestamp:** 2026-09-05T16:16:00-07:00

## Timestamp instrumentation hold — 2026-09-05T19:18:00-07:00

Zach's direct moving-content test found that the timestamp-query implementation caused
temporal incoherence across both the 3D and ImGui paths. Disabling it restored coherent
motion. Leave timestamp feature negotiation, encoder writes, and callback pumping
disabled in the live WebGPU device until a later, isolated diagnostic pass identifies a
safe subset and proves it with moving camera, falling-object, and dragged-ImGui tests.
Do not re-enable it merely to obtain a more attractive F3 number. The current CPU
wall-clock, surface-acquire, and queue-submit measurements are the legitimate baseline
for the next exact-Perlin optimization pass.

**Addendum signed:** Codex
**Session:** `01a072e2-017b-7b03-aa4a-1ef25dab65d1`
**Timestamp:** 2026-09-05T19:18:00-07:00

## Approved exact-value/gradient implementation plan — 2026-09-05T22:31:36-07:00

Zach approved the next implementation direction after localizing almost the entire
100–300 ms 3D phase to surface-acquire backpressure and observing a 40–60 ms downward
view versus a 200–300 ms horizon view. The detailed work division, automatic-
differentiation design, proof ladder, native benchmarks, and fallback path now live in
[`PERLIN_EXACT_VALUE_GRADIENT_IMPLEMENTATION_PLAN_2026-09-05.md`](PERLIN_EXACT_VALUE_GRADIENT_IMPLEMENTATION_PLAN_2026-09-05.md).

**Addendum signed:** Codex
**Session:** `01a072e2-017b-7b03-aa4a-1ef25dab65d1`
**Timestamp:** 2026-09-05T22:31:36-07:00
