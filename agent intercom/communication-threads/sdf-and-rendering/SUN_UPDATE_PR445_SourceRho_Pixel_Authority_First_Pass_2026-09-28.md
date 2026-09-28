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
