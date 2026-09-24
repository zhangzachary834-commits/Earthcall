# SUN UPDATE — V5 occupied-segment sampling landed; native proof gate queued

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #343 — Volumetric V5: medium-set composition foundation
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Re-inspection first

At the start of this pass, PR #343 was still Draft, open, and mergeable at:

`abce495e40d919c38eff0f9c360cbf24b41d328e`

Canonical then advanced materially while this pass was running. The latest canonical observed before this Intercom write is:

`423cfd69dceb2959ccfe07d16fa2dd1ae86698f6`

The V5 branch is 89 canonical commits behind its merge base, but a targeted merge-base -> canonical file comparison found **no overlap with the nine V5 production/test files** touched by this PR. Reconciliation is still mandatory before landing, but no concurrent canonical renderer/compiler edit was found in this pass.

## The concrete defect

`compileVolumeSet(...)` previously marched exactly 96 samples over the full union-AABB ray span.

That made empty distance physically irrelevant but numerically expensive: adding a distant disjoint medium could enlarge the union interval and reduce sample density inside an already-existing medium.

The fused transport law itself was not the defect. Active media already summed into one `totalExtinction` and one `totalSource`, then advanced one shared transmittance/radiance integral. Preserve that.

## Witness committed first

Commit:

`c32ea4c042a3303b99b99546e134300774bad59f`

adds a native local-quality witness beside the existing V5 AB/BA permutation witness.

The witness constructs:

- medium A at the origin;
- constant positive density;
- authored low extinction;
- zero scattering;
- red self-emission varying spatially as `0.05 * exp(3z)`;
- medium B at z=-90 with a valid but exactly zero density field.

B therefore contributes no transport. Its only effect on the old implementation is to enlarge the fused union span.

The test compares A alone with A + far zero-contribution B and requires the red-channel result to remain within two framebuffer bytes.

The witness-only CI run was auto-cancelled by subsequent branch commits, then explicitly re-run as workflow `35922208098` to preserve the before/fix evidence ordering. At this update the re-run is queued on the macOS runner pool; no pass/fail claim is made yet.

## Sampling fix

Commit:

`dcc6ab7649c5dd82bd16c6f35de3e1c92fa28233`

replaces the global-union fixed march with **event-segmented occupied-interval transport**.

For every admitted medium, the generated fused WGSL now:

1. intersects that medium's AABB with the camera ray and visible/depth-limited interval;
2. records entry/exit events;
3. sorts the bounded event list in shader-local storage;
4. walks adjacent event intervals;
5. rejects empty gaps before spending any samples;
6. gives each occupied topological interval the established 96-sample local march;
7. carries one continuous `transmittance` and `integratedRadiance` state across interval boundaries;
8. inside every sample, still evaluates all media active at that world point and sums them into the same `totalExtinction` / `totalSource` before the shared transport update.

This is intentionally **segment the ray, not the physics**.

Single-medium V0-V4 still returns through the untouched `compileVolume(...)` path.

## Compiler witness

Commit:

`e41d4f9d82ea50b067de791cfe59674003871b86`

extends the focused compiler witness to require:

- fused per-medium evaluators;
- one shared `totalExtinction` / `totalSource`;
- generated `mediumEvents` storage;
- occupied-segment detection;
- sampling from `segmentStart`, not the union start;
- the existing shared interval-gain transport law.

## Current CI state

The implementation head before this Intercom-only commit is:

`e41d4f9d82ea50b067de791cfe59674003871b86`

Its initial exact-head workflow was cancelled when the witness-only historical run was re-run under the workflow's branch concurrency policy. That cancellation is not a test failure.

Witness-only run:
- workflow: `35922208098`
- state at this update: queued
- jobs queued: SDF range-proxy verification, Slow Adapter independent clock, Focused CPU tests

A fresh exact-head run must be observed after this Intercom commit. Do not call the sampling fix green until the current code head's focused CPU/native WebGPU jobs complete successfully.

## Remaining risks

1. **Native WGSL validation remains the gate.** The event array / insertion-sort control flow has been statically reviewed but still needs the real WebGPU compiler witness.
2. **Cost scales with occupied topological segments.** The policy deliberately preserves local quality by giving each occupied segment 96 samples; partially overlapping many-media scenes can cost more than the old fixed 96-global march. Do not weaken correctness before measuring this.
3. **Canonical reconciliation remains required.** The branch is far behind current canonical even though the targeted collision check found no V5-file overlap.
4. Existing V5 plan gates still remain after this local-quality rung: time-only fused-set witness, numeric-only fused renderer/no structural compile, membership invalidation, refused-member native no-stale-output, canonical reconciliation, and final exact-head CI.

## Exact continuation point

1. Let witness-only run `35922208098` complete and inspect the native WebGPU job output. The expected pre-fix behavior is that the new local-quality assertion fails because far B dilutes A's sampling density.
2. Run/observe exact-head CI on the current branch after this Intercom commit. If shader compilation or the local-quality witness fails, repair only this sampling seam.
3. Re-run the existing AB/BA permutation witness and compiler fused-integral witness; they must stay green.
4. Once local sampling is proven, reconcile current canonical before advancing to the next unfinished V5 witness. Do not start GI, multiple scattering, spectral transport, or another renderer architecture campaign here.

— GPT-5.6 Sol
