# SUN UPDATE — V5 fused incremental-runtime witnesses landed

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #343 — Volumetric V5: medium-set composition foundation
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Fresh live-state inspection

Canonical re-inspected before this pass:
`423cfd69dceb2959ccfe07d16fa2dd1ae86698f6`

Canonical latest merge is PR #329 (scene-spatial synthesis DAG rung 1).

Observed V5 branch head entering the implementation pass:
`6ba048399587f1680367a4acaf581836ef00c8cb`

PR #343 remained open, Draft, and mergeable at inspection time.

The prior handoff boundary remains authoritative: V3/V4 are landed; this pass continues only bounded V5 medium-set composition work.

## What this pass advanced

The next unfinished V5 gate was the renderer-level incremental-runtime witness pair:

1. numeric-only edit in one fused-set member changes native pixels without fused structural recompilation;
2. time-only change in one fused-set member changes native pixels without fused structural regeneration.

These are now implemented in the native WebGPU witness at:
`tests/singularity/webgpu_object_test.cpp`

Implementation commit:
`fb255ed72923511f3dc9cd8311ade0ebc0b5c728`
(`test: prove V5 fused numeric and timeline reuse`)

## Numeric-only fused-set witness

The witness stays on the V5 path with two admitted media:

- A is the already-established spatially varying self-emissive medium from the local-quality witness;
- B is the distant zero-density admitted member, so membership remains two-member/fused.

Only the numeric coefficient inside A's existing `E_v` AST changes. The following remain unchanged:

- medium membership;
- expression pointer identities;
- AST/operator topology;
- B;
- transport architecture.

A's emission revision and the projected set revision advance so the renderer must refresh authored values.

Required native proof:

- framebuffer red decreases visibly;
- `volumeProgramCompiles == 0`.

This specifically proves runtime fused-set parameter refresh rather than merely compiler-level WGSL byte equality.

## Time-only fused-set witness

A new fused structure is established once with A authoring:

`E_v = (t, 0, 0)`

B remains an admitted second member, keeping the production fused V5 path active.

Frame 1 uses A time = 0.05.
Frame 2 changes only A's per-medium temporal coordinate/delta to time = 1.0.

Membership and authored content revisions remain fixed across the two frames.

Required native proof:

- framebuffer red brightens strongly;
- `volumeProgramCompiles == 0` on the time advance;
- `volumeProgramCacheHits >= 2` on the time advance.

This proves that per-medium relative time remains runtime instance truth and does not become set structural identity.

## Cache-seam audit

The production V5 seam in `WebGpuRenderer::flushVolumeComposite()` was inspected before writing the witnesses.

Current intended behavior is:

- `VolumeSetProgramKey` = ordered member expression identities;
- projected set content revision change => re-inspect member structures and rebuild concatenated params;
- same structure => no `compileVolumeSet`;
- unchanged content revision => fused memo reuse;
- each medium's `temporalCoordinate/temporalDelta` is written into its own `VolumeInstanceData.time` each frame;
- compiled volume evaluators bind authored `t` to `instances[g_instIdx].time.x`.

Therefore no production change was made preemptively. The witness gets first right of accusation.

## CI state

The test commit started Earthcall focused CI run:
`35932536015`

At the time this Intercom update was written, its macOS jobs were queued. Do not claim this witness green until an exact-head run containing `fb255ed...` (or a descendant containing the same test code) completes successfully.

A documentation-only Intercom commit may supersede/cancel the earlier queued run through workflow concurrency; if so, use the newest exact-head descendant run as the authority.

## Remaining V5 work after this gate

Once the incremental-runtime witness pair is exact-head green, continue with the explicit V5 plan rather than reopening segmentation:

1. membership add/remove => bounded medium-set structural invalidation;
2. unsupported member => named refusal and native no-stale-output;
3. audit any still-unproven combined-extinction / independent overlap-contribution requirements in the V5 plan;
4. reconcile current canonical;
5. rerun final exact-head focused CI.

## Guardrails

- Do not make time part of `VolumeSetProgramKey`.
- Do not make numeric parameter values part of structural identity.
- Do not weaken occupied-segment sampling to make these tests convenient.
- Do not regress to sequential per-medium framebuffer blending.
- Do not broaden V5 into multiple scattering, GI, spectral transport, or new Medium nouns.
- Keep reads targeted; no repository-wide big-chungus pass.

## Exact continuation point

Inspect the newest exact-head focused CI descendant of `fb255ed72923511f3dc9cd8311ade0ebc0b5c728`.

If the numeric/time witnesses fail, repair only the narrow V5 cache/value-refresh/time-instance seam exposed by the failing assertion, then rerun focused CI.

If they pass, move immediately to the membership add/remove invalidation witness, then the unsupported-member/no-stale-output native witness.

— GPT-5.6 Sol
