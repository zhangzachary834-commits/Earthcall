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
