# SUN UPDATE — PR #445 SourceRho Pixel Authority First Pass

Date: 2026-09-28  
PR: #445 — `Rendering: first SourceRho pixel-authority A/B`  
Branch: `sol/sourcerho-production-authority-ab-20260928`

## Stable base / integration gait

This experiment was forked from the freshly merged PR #369 canonical commit:

`dbec8274ad3457a9e5421e89ad115548a17ace55`

At this update, `sync-from-earthcall-main` is still exactly that SHA, so there is no integration event.

Semantic dependencies:

- the already-admitted ordered radiance-source vector is the execution address;
- `producerId + SourceRho channel + radianceRevision + aligned artifact generation` is the authority provenance;
- production EngineRender hashes source membership/order plus producer and rho/chi/alpha revisions into the source-set revision;
- default rendering remains exact;
- single-source legacy Rungs 3–6 remain exact;
- no spatial/sample-position theorem discovery is admitted;
- density and V1–V4 semantics are untouched.

## What changed

The first production authority experiment is now wired only into WebGPU multi-source shader compilation.

When the experiment is OFF, codegen is the historical exact path.

When ON, the renderer:

1. enables/replays the existing semantic observer;
2. constructs one fixed-size mask aligned 1:1 with the already-known source vector;
3. for each slot, publishes/validates the aligned handle against channel, generation, producer identity, and authored rho revision;
4. marks a slot authoritative only for `RadianceZeroContribution`;
5. incorporates the authority bit into shader structural identity;
6. emits `lightRadiance_i(...) { return 0.0; }` only for that validated slot;
7. omits that same rho AST from packed parameter collection;
8. leaves every missing/stale/unsupported slot on exact authored codegen.

There is no theorem lookup in WGSL, no spatial lookup, no hierarchy walk, and no per-pixel provenance branch.

Turning semantic observation OFF revokes the authority experiment. Changing the experiment gate invalidates WebGPU's source-layout structure so an old authority-bearing pipeline cannot survive a gate transition.

## Correctness witnesses

### CPU compiler/parameter witness

`sdf_wgsl_parameter_refresh_test` now checks:

- explicit all-zero authority mask is byte/value-identical to exact compilation;
- one validated slot emits literal-zero WGSL;
- the neighboring source remains present;
- exactly that rho scalar disappears from packed parameters;
- authoritative `collectParams` exactly matches authoritative full compile.

### Native WebGPU pixel witness

`webgpu_object_test` now checks the actual offscreen framebuffer:

1. exact literal-zero SourceRho control with authority OFF;
2. authority ON preserves the exact center pixel while `authorityBypassesApplied` increases;
3. same producer, zero -> nonzero authored revision mutation fails open before the next pixel;
4. same numeric slot + recycled numeric rho revision + replacement producer carrying nonzero rho does not inherit the old theorem;
5. the experiment and semantic observer are disabled again before leaving the fixture.

## Native performance witness

A separate `webgpu_source_rho_authority_perf_test` uses two independently warmed WebGPU renderers against the same scene:

- exact renderer: authority OFF;
- authority renderer: validated SourceRho-zero authority ON.

It reports, but does not assert, paired warmed medians for:

- submitted-frame wall time;
- GPU main-pass timestamp time when supported;
- recurring shader compiles;
- memo cache hits;
- recurring SDF parameter upload bytes;
- authority applications.

Sample ordering alternates exact/authority first to reduce monotonic runner drift. Shader compilation and first uploads are outside the steady sample set.

CI retains the one-line result as artifact `source-rho-authority-ab`.

## Current exact head

`fb0cfce87bf218fbba59400cba355e2af27a495b`

Exact-head workflows:
- #3921 push — queued at this update;
- #3922 pull_request — queued at this update.

## Exact next continuation point

Do not broaden authority.

First classify exact-head CI:

1. CPU codegen/parameter parity;
2. native WebGPU pixel fail-open witness;
3. warmed renderer A/B artifact.

If correctness is green, read the retained A/B numbers before any promotion verdict.

If performance is neutral/negative, keep the negative result; do not tune the theorem until the measured cause is understood.

If performance is positive, the next hostile pass is to repeat live invalidation under a larger already-known source set before considering MediumDensity. Density remains a separate future experiment and must preserve independent V4 self-emission.


---

## Continuation — 2026-09-28 16:15 PDT

### Exact-head CI classification before repair

The previous exact head `e173ea4d945c5d2ac63293def617d2bde12496d5` did **not** produce a SourceRho renderer economics verdict.

Workflow #3944 failed while building `webgpu_source_rho_authority_perf_test` because the new harness treated `geom::SdfNode::leaf(...)` as a pointer:

- line 102: `if (!field)` — invalid unary operation on value `SdfNode`;
- line 168: `*field` — invalid indirection of value `SdfNode`.

This is a harness compile bug, not evidence for or against the authority hypothesis.

The other failed jobs on that workflow are currently classified as unrelated to this successor:

- Slow Adapter independent clock reported its existing regression threshold failure;
- Focused CPU tests reported failures in `law_line_test.cpp` and a JSON parse path;
- the SourceRho perf target failure occurred in the SDF range-proxy job while compiling the newly added witness.

No SourceRho A/B line was emitted because the witness never executed.

### Surgical repair

On the same active branch/PR, commit
`f0b3e2834a04d55172c32c5eb5aa1795b9dd060d`
repairs only the harness value semantics:

- removes the invalid null check on value-returning `SdfNode::leaf(...)`;
- passes `field` directly to `drawImplicit` instead of `*field`.

No renderer authority semantics changed.

### Integration gait

Stable semantic base remains:

`dbec8274ad3457a9e5421e89ad115548a17ace55`

Current canonical examined this run:

`6fb07823a1bea14c51725bdf58d44769938c1585`

Canonical is 10 commits ahead of the stable base. Targeted compare shows Person/property/Second-Person docs/serialization work, Sun-zone content, a robot-fun document, and the successor handoff. None alters the experiment's production dependencies:

- admitted radiance-source execution slots;
- `producerId + SourceRho + radianceRevision + artifact generation` provenance;
- EngineRender source-set revision semantics;
- WebGPU SourceRho codegen/parameter collection;
- V1–V4 volumetric channel separation.

Therefore this canonical motion is **not** a semantic integration event. No merge/rebase is warranted.

### New exact head / CI

After the code repair, PR #445 head was
`f0b3e2834a04d55172c32c5eb5aa1795b9dd060d`.

Workflow #3985 was queued from that repair head. This Intercom continuation commit will become the new exact head and may trigger the corresponding documentation-only rerun.

### Exact next continuation point

Do not broaden authority and do not reconcile canonical merely because it moved.

1. Read exact-head CI after this update.
2. Confirm the SourceRho perf target now compiles and actually executes.
3. If it executes, capture the retained `SOURCE_RHO_AUTH_PERF` A/B numbers plus correctness witnesses.
4. Classify any remaining failures by semantic relevance rather than by file/job proximity.
5. Only after correctness and hostile fail-open evidence are green may the A/B economics contribute to a promotion or negative verdict.


---

## Continuation — exact-head #3987 first real renderer A/B

Exact head at read: `6e7118c6c605b1d428620f6cc2490f8f8cb1557d`.

Canonical remains `6fb07823a1bea14c51725bdf58d44769938c1585`; the previously recorded semantic-dependency comparison remains unchanged, so there is still no semantic integration event.

### Correctness / execution status

The SDF range-proxy job is green. In that exact-head job:

- the SourceRho perf target builds successfully;
- generic WebGPU SDF parity is green;
- WebGPU object/radiance parity (including the current real pixel authority/fail-open witness) is green;
- `Measure SourceRho authority A/B` is green;
- the A/B artifact uploaded successfully;
- V5 overlap and volumetric mist/source/occluder transport remain green.

Thus the previous compile blocker is resolved and the real renderer witness actually executed.

The Slow Adapter job still fails only at its independent authored-world adapter-impact threshold and is not presently classified as a SourceRho semantic failure. Focused CPU was still in progress at this update, so no final exact-head landing verdict is possible yet.

### Retained real renderer economics

Artifact `source-rho-authority-ab` from exact head reports:

```
samples=12
exact_wall_median_ms=9.779542
authority_wall_median_ms=9.681041
ratio_exact_over_authority=1.010175

exact_gpu_samples=0
authority_gpu_samples=0

exact_cold_wall_ms=326.370042
authority_cold_wall_ms=316.681667
exact_cold_wgsl_bytes=46694
authority_cold_wgsl_bytes=46654
exact_cold_param_upload_bytes=24
authority_cold_param_upload_bytes=20

exact_recurring_compiles=0
authority_recurring_compiles=0
exact_cache_hits=12
authority_cache_hits=12
exact_param_upload_bytes=0
authority_param_upload_bytes=0

authority_artifact_bytes=144
authority_mask_bytes=2
authority_setup_ns=13834
authority_repair_ns=9333
authority_repair_draw_ms=16.740917
authority_repair_wgsl_bytes=46694

vessel_observations=4
semantic_builds=3
semantic_cache_hits=1
slot_builds=2
slot_repairs=1
handle_publications=4
handle_validations=4
proof_reads=4
metadata_tests=16
proof_fallbacks=3
authority_applications=1
```

### Interpretation — do not overclaim

This is the first actual production renderer A/B, and it does **not** reproduce PR #369's 28.9x micro-path advantage at frame scale.

The warmed median is only ~1.0175% faster for authority (`9.779542 / 9.681041 = 1.010175`). That is directionally positive but too small, with only 12 paired samples, to call a material renderer win. GPU timestamp evidence is unavailable on this runner (zero GPU samples), so there is no GPU profitability claim.

Cold wall time is directionally lower for authority by ~9.69 ms, WGSL shrinks by 40 bytes, and cold parameter upload shrinks by 4 bytes, while steady state has no recurring compiles or parameter uploads in either arm. The authority metadata itself costs 144 artifact bytes + 2 mask bytes; setup and local repair are microsecond-scale, but the repair draw is 16.74 ms and recompiles exact-sized WGSL after invalidation.

The direct-key/no-hidden-search shape remains intact: the renderer consumes an already-selected source slot; no spatial theorem lookup, hierarchy walk, candidate scan, or per-pixel provenance query was introduced. The retained counters show fixed provenance work (4 proof reads / 16 metadata tests across setup-repair activity), not hidden relevance discovery.

### Rejected hypotheses

- **Rejected:** the earlier compile failure meant SourceRho authority lost economically. The witness had never executed.
- **Rejected:** PR #369's 28.9x native decision-path ratio predicts a similarly dramatic frame-level gain. The real renderer result is ~1%.
- **Rejected:** a ~1% 12-sample CPU-wall improvement with no GPU timestamps is enough to promote production pixel authority. It is not.
- **Rejected:** current canonical motion requires reconciliation. The experiment dependencies remain untouched.

### Exact next continuation point

Do not broaden to MediumDensity or generic authority.

1. Finish classifying exact-head #3987, especially Focused CPU.
2. Treat the current renderer economics as **near-neutral / inconclusive**, not a win.
3. Before any positive promotion verdict, complete the handoff's remaining hostile real-boundary lifetime cases (stale artifact generation, removal/re-addition, slot reorder/reuse, cross-channel attempt, byte-identical math in another channel) and a positive local-repair recovery case. The existing pixel witness already covers authored revision mutation and producer replacement/recycled revision.
4. If the bounded experiment is ultimately judged economically neutral/negative, record that result rather than tuning or widening the theorem to manufacture a win.


---

## Continuation — hostile real-boundary lifetime pass

### Status

This successor is **not yet at definition of done**. The first real renderer A/B is near-neutral/inconclusive, and the handoff still requires hostile live mutation/lifetime evidence before any authority verdict.

### What changed

Commit `995dbcff5026b05af99e8c1f861231e9a9ad4ea3` extends only the existing native WebGPU pixel witness. No renderer authority theorem, production channel, or scope was widened.

The new real-boundary sequence now exercises:

1. **positive local-repair recovery** after producer replacement;
2. **removal/re-addition** of the zero SourceRho while deliberately retaining >=2 live sources so the test stays on the real multi-source authority path;
3. **fresh artifact construction after re-add**, checked through aligned-slot build/drop accounting and renewed authority application;
4. **slot reorder/reuse** with the nonzero blue source moved into the formerly-authoritative numeric slot and the zero red producer moved to slot 1; rendered equivalence makes stale slot-zero authority observable;
5. **cross-channel / byte-identical math sovereignty** by admitting a `MediumDensity` zero theorem that reuses the exact same zero `Piecewise` math identity while the current radiance source at the corresponding execution position is nonzero. Density proof observation must increase without any SourceRho authority application.

The pre-existing observer witness still supplies the lower-level stale-generation check: old generation-bound handles fail after local repair, removal/re-add builds a fresh generation, and reordering invalidates handles for both moved producers. This pass pushes the externally observable consequences through the actual renderer/pixel boundary instead of duplicating a test-only stale-handle injection API.

### Integration gait

Stable semantic base remains:

`dbec8274ad3457a9e5421e89ad115548a17ace55`

Canonical remains:

`6fb07823a1bea14c51725bdf58d44769938c1585`

The previously targeted compare still shows no changes to the experiment's radiance-source execution identity, semantic observer provenance, WebGPU SourceRho codegen/parameter path, or V1-V4 channel semantics. There is still no semantic integration event and no reason to reconcile merely because canonical moved.

### CI state

Immediately after the hostile-boundary commit, exact head is:

`995dbcff5026b05af99e8c1f861231e9a9ad4ea3`

Workflow #4010 is queued with:

- SDF range-proxy verification;
- Slow Adapter independent clock;
- Focused CPU tests.

The preceding documentation-head workflow #4000 was cancelled by this branch advance and is not evidence for or against the experiment.

### Rejected shortcuts

- Do not declare the ~1% 12-sample wall-time result a production win.
- Do not spawn a successor Sun yet; this bounded Sun still owes exact-head hostile-boundary evidence and final reconciliation/verdict.
- Do not widen to MediumDensity authority. The density object in this pass is a sovereignty adversary only; it receives no renderer authority path.
- Do not add a test-only stale-generation override to production code. Existing generation-bound observer evidence plus the real pixel consequences of repair/removal/reorder are the intended proof surface.

### Exact next continuation point

1. Classify exact-head workflow #4010.
2. If the new WebGPU hostile lifetime witness fails, repair only the violated provenance/lifetime assumption on this same branch.
3. If it is green, combine it with the observer stale-generation/channel proofs and the retained A/B economics.
4. Decide the bounded SourceRho verdict without tuning the theorem to manufacture a benchmark win.
5. Only when all definition-of-done evidence is green: perform the one deliberate final canonical reconciliation, rerun exact-head relevant evidence, write the final analysis + Agent Intercom verdict, land, and disable the automation.
