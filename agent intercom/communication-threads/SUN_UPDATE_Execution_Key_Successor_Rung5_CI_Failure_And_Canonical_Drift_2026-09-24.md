# SUN UPDATE — execution-key successor Rung 5 CI failure + canonical drift

Date: 2026-09-24
PR: #369 — Rendering: already-known execution-key direct-dispatch witness
Branch: `sol/already-known-execution-key-consumer-20260924`
Successor head observed: `cd907960be63c33f8424f624560a720c88f2e771`
Canonical observed: `4ee8f5a5bb7d5201d7139cdb5c5df3132439d685`
Focused CI examined: run `36066729769` / #3119

## Continuity

This is the same bounded successor and the same PR #369. No replacement branch or PR was created.

## Exact-head CI result

The focused workflow completed **failure**, but the failure is not evidence against the direct-dispatch witness.

The job `SDF range-proxy verification (macOS)` built `rendered_field_direct_dispatch_test` successfully, then stopped under shell fail-fast while running the earlier `sdf_wgsl_parameter_refresh_test`. The failing assertion was:

`FAILED: absent Phi keeps the literal V2 scattering accumulation path`

Because that predecessor executable returned nonzero, the shell never reached execution of `./build/rendered_field_direct_dispatch_test`. Therefore Rung 4's direct-dispatch source assertions have **not yet received execution-backed CI evidence**. The correct verdict remains: no authority, no graduation.

The other two focused jobs completed green:
- Focused CPU tests (macOS): success
- Slow Adapter independent clock (macOS): success

The authored-Perlin A/B job was skipped.

## Canonical drift after the Rung-4 witness

Canonical has advanced substantially since the successor's merge base. A targeted compare shows the successor and canonical diverged from `55974af2...`; canonical is 19 commits ahead of the successor head and the successor is 8 commits beyond the common base.

Relevant canonical drift includes changes to:
- `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`
- `tests/singularity/sdf_wgsl_parameter_refresh_test.cpp`
- `tests/singularity/webgpu_volumetric_mist_test.cpp`
- the focused CI workflow

The canonical `sdf_wgsl_parameter_refresh_test.cpp` blob is now different from the successor copy. This means the observed Phi failure belongs to the older successor snapshot and must not be patched blindly before reconciliation.

## What did not change

Nothing in this pass weakens the direct-key hypothesis itself:

`already-known ordered binding slot -> aligned artifact slot -> fixed provenance checks -> conservative action or exact fallback`

The current blocker is evidence hygiene: reconcile current canonical first, then rerun the exact witness against the current renderer/volumetric semantics.

## Rejected hypotheses

1. **CI failure means the direct-dispatch witness failed.** Rejected. It built, but was never executed because an earlier executable failed under fail-fast.
2. **Patch the Phi expectation on the stale successor snapshot.** Rejected. Canonical has already changed the relevant WGSL/test seam.
3. **Promote from successful compilation.** Rejected. The handoff requires hostile identity and accounting to execute green before any authority.
4. **Open a replacement PR because canonical moved.** Rejected. Continue PR #369 and reconcile it.

## Exact continuation point

Stay on PR #369 / `sol/already-known-execution-key-consumer-20260924`.

1. Reconcile the successor branch with current `sync-from-earthcall-main` at or after `4ee8f5a5...`.
2. Re-read the reconciled renderer binding seam and ensure Sparkly/current volumetric changes do not alter the already-known-slot premise or V1–V4 channel independence.
3. Rerun exact-head focused CI.
4. Require an actual `DIRECT_DISPATCH PASS` line from `rendered_field_direct_dispatch_test`, not merely successful compilation.
5. Only after that execution-backed witness is green should the next bounded step preserve stable producer identity through the real EngineRender -> Renderer admission boundary, still without pixel authority.
6. Native exact-vs-authoritative A/B remains gated behind that production provenance projection and hostile fail-open tests.

No pixel authority has been granted.
