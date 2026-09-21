# SUN HANDOFF — PR #259: SDF Spatial-Prophetic GPU Traversal

**Date:** 2026-09-20  
**Outgoing agent:** GPT-5.6 Sol (“The Sun”)  
**PR:** #259 — `GPU spatial-Prophetic traversal for SDF range hierarchy`  
**Feature branch:** `sol/sdf-gpu-range-hierarchy-20260919`  
**Current observed PR head at handoff:** `bda9e274f52304e18898a14d45c0b599476472c0`  
**Current observed base at handoff:** `ea900fa323c05125a87732899cb2129732067973` on `sync-from-earthcall-main`  
**Current GitHub mergeability at handoff:** `mergeable=false`, `mergeable_state=dirty`  
**User is personally checking CI run #1582.**

---

## 0. Read this first

Do **not** restart this project from #241 or from the old September 5 Perlin work.

Do **not** do a giant whole-repo/log read.

Read current `AGENTS.md`, then:

- `agent intercom/communication-threads/PR_53_Temporal_Rollback_Incident_2026-09-19.md`
- `docs/audits/rendering_optimization/2026-09-18_sdf_calculation_and_rendering_pipeline_inefficiency_audit.md`
- `docs/audits/rendering_optimization/2026-09-18_sdf_pipeline_mathematical_complexity_companion.md`
- `docs/plans/SDF_PIPELINE_PROPHETIC_CACHE_AND_SPATIAL_ACCELERATION_PLAN_2026-09-19.md`

Use **targeted** file reads/searches only.

The raw GitHub Actions job-log endpoint became the recurring “Big Chungus” failure mode in this thread: it repeatedly timed out because it tried to return the whole macOS job. The workflow was deliberately split into smaller native test steps so future debugging should use **job step metadata first**, not giant raw logs.

---

## 1. What #241 already established

PR #241 (`4403d9e3725c70f3abbfab632117512e94c523bd`) laid the substrate:

- memoized SDF Program reuse;
- structural vs parameter revision split;
- persistent SDF parameter buffer;
- cached heightfield proof;
- analytic value+gradient propagation;
- conservative interval/range reasoning;
- revision-cached `SdfRangeHierarchy`;
- zero-set raster proxy/cull proof;
- telemetry;
- native SDF verification lane.

Important: #241’s range hierarchy was **not** a per-ray GPU accelerator. It only derived a smaller raster proxy / whole-draw zero-free cull. The real Perlin floor’s dominant native cost remained the expensive per-pixel exact evaluator.

Historical native measurements from the maintained audit:

- trivial plane 45°: ~10.44 ms
- Perlin looking down: ~1.46 ms
- Perlin horizon: ~80.64 ms
- Perlin 45°: ~117.83 ms (~8.5 FPS)

The long-term target remains turning the dominant term from roughly `O(P * I * E)` toward `O(P * (hierarchy traversal + ambiguous exact samples))`.

---

## 2. What this PR implemented

### 2.1 GPU hierarchy storage and persistence

The CPU `SdfRangeHierarchy` is packed into a GPU storage representation and bound alongside the existing SDF instance/height-grid data.

The range-node buffer is persistent across frames. Stable scenes should not re-upload unchanged proof bytes after warmup.

Telemetry was added end-to-end for:

- `sdfRangeTraversalDraws`
- `sdfRangeNodeBytesUploaded`

These are exposed through `Renderer::FrameStats -> EngineRender -> ScreenChannel` and are read-only authored paths.

### 2.2 Per-ray spatial traversal

WGSL now has a range traversal that descends the octree for the current field-space ray location.

The design rule is intentionally asymmetric:

> The hierarchy is allowed to skip only space the theorem proves safe to skip. Everywhere else, the exact authored marcher remains the authority.

The traversal uses field-local AABBs and field-local `ro/rd` after `invModel`, so the coordinate contract remains valid under affine object transforms.

A depth-5 octree has 32 subdivisions/axis. A straight ray can cross at most `3*32 - 2 = 94` cells in the regular subdivision, so the WGSL bounded traversal guard was raised to 96. A future deeper tree must revisit this bound.

### 2.3 Raster tightening is quarantined

Native parity found a one-pixel `SmoothUnion@xform` difference while the generic acceleration was being activated.

Raster proxy tightening is therefore independently quarantined behind:

`kSdfRangeRasterTighteningVerified = false`

The intended active rung is:

**original safe raster cube -> internal GPU hierarchy skip -> exact marcher in all unproved space**

not “shrink the raster cube and hope.”

### 2.4 Exact-distance marcher traversal is quarantined

The same `SmoothUnion@xform` one-pixel witness persisted after raster shrink was disabled.

That isolated the remaining issue to interaction with the **over-relaxed exact-distance marcher**, not to the interval theorem itself.

Therefore:

`kSdfRangeDistanceTraversalVerified = false`

The active acceleration target for this rung is the expensive **authored Expr / gradient-corrected marcher** path, especially the Perlin terrain.

Do not casually re-enable distance-field traversal. It needs its own future parity/proof rung.

---

## 3. Critical correctness discoveries made during this pass

### 3.1 “Zero-free” is NOT the same thing as “safe to skip”

The first GPU representation tagged every `provedNoZero` cell as skippable.

That was wrong.

A wholly negative SDF cell is also zero-free, but it is **inside the solid**. The baseline marcher needs to observe `d <= 0` to register the crossing/hit.

The correct invariant was named in `Sdf.hpp`:

`rangeNodeProvesPositiveOutside(...)`

The lawful traversal rule is:

- finite range with `rangeLo > 0`: proved-positive outside space -> may skip;
- finite range with `rangeHi < 0`: proved-negative inside space -> must remain observable;
- ambiguous/unknown -> exact marcher.

This is load-bearing.

### 3.2 Whole-draw culling has the same sign asymmetry

An everywhere-positive field can be treated as empty outside space.

An everywhere-negative field must **not** be culled for behavioral parity with the existing marcher.

Native witness added:

- `RangeNegative`: OFF/ON identical, cull counter remains zero.
- positive empty witness: cull counter increments and render remains empty.

Do not collapse these back to a generic “no zero -> cull” rule.

### 3.3 Proof-authorized jumps are not ordinary marcher steps

After a spatial jump, the old `prev_d`, `candidate_step`, and over-relaxation state referred to the pre-jump sample pair.

Those values must not be reused as if the proof jump were an ordinary SDF step.

The shader resets derived marcher history after a real range jump.

### 3.4 Ambiguous octree boundaries must not perturb the baseline marcher

An earlier version clamped `candidate_step` to the exit of every ambiguous octree cell.

That changed the exact sample sequence even where the hierarchy had proved nothing.

That clamp was removed.

Current rule:

> No proof -> preserve the existing marcher step.  
> Positive-outside proof -> jump.

---

## 4. Why the original depth-5 Perlin hierarchy was useless

The old Noise range rule used a global amplitude enclosure plus a global Lipschitz ball with:

`kClassicPerlin3LipschitzBound ~= 28.561`

For the real terrain domain around `[1000, 30, 1000]`, a depth-5 octree cell was still so large that the Lipschitz enclosure collapsed back to the global amplitude bound.

Result: effectively **zero useful Perlin prunes**.

A perfect GPU traversal over a hierarchy that proves nothing is still useless.

---

## 5. Lattice-aware classic Perlin interval theorem

A new bounded interval rule was implemented for classic 3D Perlin.

It follows the actual vendored GLM algebra inside each crossed lattice cube:

1. GLM hash/permutation construction;
2. actual constant lattice gradients;
3. affine interval corner dot products;
4. monotone quintic fade interval;
5. exact z/y/x interpolation structure;
6. outward-rounded interval arithmetic;
7. intersection with independent global amplitude + Lipschitz theorems.

It deliberately has a bounded lattice-cell budget. If a query crosses too many lattice cells or cannot be represented lawfully, it falls back to the older conservative theorem.

Tests exercise:

- ordinary lattice crossings;
- negative coordinates;
- `mod289` hash-boundary crossings;
- dense exact-value sampling inside the queried boxes.

A prototype/theorem analysis indicated that at the real terrain scale, depth 5 can prove thousands of cells empty (roughly a first-pass ~17% volume figure was observed during development). Treat that figure as motivation, **not a final benchmark claim**.

---

## 6. Hierarchy node budget correction

The renderer previously capped a depth-5 hierarchy at 8192 nodes.

But a complete depth-5 octree can contain:

`1 + 8 + 64 + 512 + 4096 + 32768 = 37449` nodes.

The cap was raised to 65536 so the real Perlin hierarchy is not truncated before reaching useful small cells.

A real-scale hierarchy witness uses the actual render proxy scale around:

`[1050, 31.5, 1050]`

and requires:

- hierarchy builds;
- positive/zero-free proof exists;
- ambiguous terrain band remains;
- supported expression does not turn unknown;
- node count respects both runtime budget and mathematical depth-5 maximum;
- proved cells survive sampling.

---

## 7. Perlin native correctness gate

The maintained authored-Perlin native corpus includes multiple camera regimes, including:

- looking down;
- horizon;
- 45-degree;
- close oblique;
- near-parallel to ground;
- camera inside proxy.

The ON run uses a stable memo identity so traversal cannot pass vacuously by never building/using the hierarchy.

### Important correction to the test oracle

An early Gate D required **byte-for-byte RGBA equality** OFF vs ON.

That was conceptually too strict for a real accelerator: a lawful skip changes the floating-point sample sequence and can shift the final hit/normal by a few ulps even while preserving exactly the same surface coverage.

The final intended oracle is:

- OFF and ON must have **identical hit/miss coverage**;
- sampled pixels in OFF must agree with the CPU exact-root oracle;
- sampled pixels in ON must independently agree with the same CPU oracle;
- traversal must actually activate in at least one authored-Perlin camera;
- shaded RGBA byte differences may be reported diagnostically, but are not themselves proof of geometry loss.

The commit that made this correction in the earlier branch history was:

`330baefe98e6b20935a3a4c438a88c40b6cd613e`

Current branch history has since moved substantially; verify the current test retains this semantic oracle.

---

## 8. Native A/B benchmark

A dedicated native benchmark was added:

`tests/singularity/webgpu_sdf_range_perf_test.cpp`

It renders the authored terrain:

`y - 40 * noise(0.008 * (p + vec3(100, 0, 100)))`

at:

`2880 x 1800`

with horizon and 45-degree cameras.

It measures OFF vs ON in the same process/device and reports:

- wall median ms;
- GPU main-pass median ms when timestamp samples are available;
- OFF/ON ratios;
- traversal draw count;
- recurring range-node upload bytes.

The benchmark is **measurement-only** with respect to speed. It should not fail just because a noisy CI runner produces a bad ratio.

It **should** fail if:

- traversal never activates;
- hierarchy never builds;
- supposedly persistent hierarchy bytes keep re-uploading after warmup.

Do not make a speedup claim until this benchmark completes on the native runner.

---

## 9. CI was deliberately de-“Big-Chungused”

The SDF native CI step originally ran four native binaries inside one shell step. When one failed, diagnosing it required pulling the giant raw job log, which repeatedly timed out.

The workflow was split so native witnesses have separate Actions steps:

- generic WebGPU SDF parity;
- WebGPU distance parity;
- WebGPU authored-color parity;
- authored-Perlin gradient/traversal parity;
- native authored-Perlin range traversal A/B benchmark.

Use `fetch_workflow_run_jobs` / step metadata first.

**Do not fetch the giant raw job log unless absolutely unavoidable.**

The user is checking CI **#1582** directly.

---

## 10. Current branch/base complication — VERY IMPORTANT

At the end of this conversation the PR branch had advanced a lot relative to the earlier SDF-only head.

A compare from the earlier `330baefe` state to current observed head showed ~115 additional commits and many unrelated files from current-main development, including:

- OntoMath radiance work;
- Audio Micromastery;
- Prophetic Rete work;
- Cathedral/save changes;
- Person serialization;
- other concurrent Earthcall work.

These are **not** part of the SDF feature itself.

Current PR was observed as `dirty` / non-mergeable against an even newer base.

This repo had the PR #53 Temporal Rollback Incident. Therefore:

### DO NOT

- resolve conflicts by wholesale “ours” or “theirs”;
- reset the branch to an old #259 SHA;
- infer that large unrelated diffs belong to the SDF work;
- blindly rebase/merge and assume recent radiance/audio changes survived.

### REQUIRED MERGE INVARIANT

The SDF memoization path must preserve **all** of:

1. current-main OntoMath radiance invalidation:
   - `radianceRevision`
   - `radianceExprPtr`
   - current compile/collectParams radiance behavior;
2. existing SDF structural/parameter/color invalidation;
3. this PR’s range hierarchy invalidation:
   - range parameter revision;
   - extent revision;
   - packed range-node cache;
   - positive-skip availability;
4. current-main WGSL radiance compilation and parameter collection;
5. current-main renderer ABI changes;
6. range-node bindings/traversal additions.

Earlier audit explicitly caught that an old feature-branch `MemoizedProgram` view lacked `radianceRevision` and `radianceExprPtr` while current main had them. A blind conflict choice would have regressed freshly merged radiance work.

---

## 11. What to do after Zach reports CI #1582

### Case A — all correctness gates green and benchmark runs

Then:

1. Record exact OFF/ON horizon + 45° benchmark numbers.
2. Check whether GPU timestamps were populated; wall-time ratios are still useful if not.
3. Verify:
   - traversal draws > 0;
   - recurring range-node upload bytes == 0 after warmup.
4. Do a current-base overlap audit.
5. Resolve/sync the dirty PR **surgically**, preserving radiance/audio/current-main changes.
6. Rerun the focused SDF/native CI on the post-sync head.
7. Compare PR diff against current base:
   - no mass deletions;
   - no rollback ghosts;
   - SDF feature diff understandable.
8. Update PR description with:
   - correctness status;
   - benchmark measurements;
   - distance traversal quarantine;
   - raster-tightening quarantine;
   - exact active rung.
9. Only then mark PR Ready for Review.

### Case B — generic/distance/color parity green, authored-Perlin parity fails

Use the now-split step metadata to identify the exact binary.

Do not re-enable distance traversal or raster tightening.

For Perlin, inspect:

- hit/miss coverage OFF vs ON;
- whether CPU oracle disagrees with OFF, ON, or both;
- whether traversal actually activates;
- whether positive-only range packing is intact;
- boundary ownership in `rangeCandidate`;
- whether a jump can land exactly on a positive/ambiguous boundary and fail open correctly.

Do not weaken hit-coverage parity.

### Case C — benchmark fails because traversal never activates

Check in order:

1. `prog->needsGradientStep` is true for Expr;
2. memo ID/revisions are stable;
3. hierarchy has at least one `rangeNodeProvesPositiveOutside` node;
4. `rangeHasPositiveSkip` is true;
5. range-node buffer is bound;
6. instance `rangeTraversalEnabled` is 1.

Do not “fix” by turning every zero-free node into skippable again.

### Case D — benchmark runs but is slower

That is a legitimate result.

Profile/measure before changing correctness semantics.

Likely next levers:

- hierarchy depth / branching economics;
- tighter interval theorem;
- linearized traversal layout / cache behavior;
- reducing per-ray hierarchy overhead;
- coarse top-level bins before octree descent;
- camera/coherence-aware traversal;
- only later consider a performance regression threshold.

Do not claim success just because traversal exists.

---

## 12. Remaining future rungs after this PR

Even if #259 succeeds, this is not the final SDF architecture.

Likely future work:

### Rung A — distance-field traversal proof

Repair the one-pixel `SmoothUnion@xform` interaction with the over-relaxed marcher, independently prove the sample-history/jump contract, then consider flipping:

`kSdfRangeDistanceTraversalVerified = true`

Do not couple this to Perlin readiness.

### Rung B — raster proxy tightening proof

Investigate transformed raster-edge ownership and only re-enable:

`kSdfRangeRasterTighteningVerified = true`

after a full transformed-camera corpus proves it.

### Rung C — stronger spatial theorem

If Perlin speedup is modest, the biggest opportunity may be better spatial proofs rather than deeper brute-force traversal:

- tighter cell-local Perlin intervals;
- derivative-aware monotonicity bounds;
- adaptive subdivision driven by theorem width;
- spatially coherent hierarchy packing.

### Rung D — general authored-field spatial Prophetic evaluation

The architectural destination is larger than Perlin:

> Conservative mathematical knowledge should decide where an authored field **cannot matter**, and exact evaluation should be spent only where the theorem cannot decide.

That should remain generic OntoMath/SDF substrate, not a Perlin-only hardcoded optimization.

---

## 13. Person / architectural verification notes

This work should continue to respect the broader Earthcall architecture:

- proof/caches are derived substrate, not authorities over authored truth;
- unknown means “evaluate exactly,” never “assume empty”;
- optimization state is observational/derived, exposed read-only;
- authored world semantics remain in the authored field/Law/OntoMath structures;
- the GPU hierarchy is a consumer of proved mathematical facts, not a new hardcoded world ontology.

---

## 14. Short handoff summary

PR #259’s core contribution is now:

**authored SDF / OntoMath field  
-> conservative range theorem  
-> lattice-aware Perlin theorem  
-> revision-cached hierarchy  
-> positive-outside proof only  
-> persistent GPU node buffer  
-> bounded field-local ray traversal  
-> skip only proved-positive space  
-> exact marcher everywhere else**

Quarantined independently:

- raster proxy tightening;
- over-relaxed distance-field traversal.

The next Sun should **not** redesign this from scratch.

First action after Zach’s CI #1582 report:

> identify the exact split witness result, record native Perlin A/B if available, then perform a surgical current-base sync that preserves OntoMath radiance and all concurrent mainline work.

— GPT-5.6 Sol / The Sun
