# SUN UPDATE — V5 overlap-physics tribunal wired into exact-head CI

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #343 — Volumetric V5: medium-set composition foundation
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Fresh live-state inspection

Canonical was re-inspected before changing anything and is currently:

`0426ce24e3f5450d27301597dcd9c56dda360e92`

PR #343 was re-inspected at head:

`b1304d25866571db4751da070363404aff2f0d3b`

It remains open and Draft. The prior exact-head focused CI run `35945223796` completed SUCCESS, but that run did not execute the newly separated `webgpu_v5_overlap_physics_test` because focused CI still named only `webgpu_object_test`.

V3/V4 are already landed and were not reopened.

## Implementation in this pass

CI wiring commit:

`71b12cc557b1206df22db4b2400f929d8680fe8d`

Message:

`ci: run V5 overlap physics tribunal`

The focused macOS SDF/WebGPU job now:

1. builds `webgpu_v5_overlap_physics_test` alongside the existing WebGPU witnesses;
2. runs it immediately after `webgpu_object_test`;
3. captures its output in `webgpu-v5-overlap-physics.log`;
4. uploads that log with the existing WebGPU diagnostic artifact.

No production renderer code was changed in this pass. The purpose is to force the new analytical overlap theorem through the same native macOS/WebGPU environment that has validated the prior V0-V5 witnesses.

## What the tribunal proves if green

The dedicated test already on the branch constructs two exactly overlapping constant media and checks native pixels against the closed-form shared-extinction answer:

`C = (S_A + S_B) * (1 - exp(-(sigma_A + sigma_B)L)) / (sigma_A + sigma_B)`

It then constructs both possible obsolete sequential whole-medium-alpha answers and requires the native framebuffer to remain strongly separated from both counterfactuals.

Finally it removes B's self-emission and makes B contribute blue through authored scattering/chroma while A continues contributing red through self-emission. Both independent authored source paths must survive the same shared transport integral.

Therefore a green native run closes the remaining direct V5 proof gap for:

- combined extinction rather than sequential whole-medium alpha;
- independent overlapping source contributions across self-emission and scattering/chroma.

## Exact-head CI

The CI wiring commit triggered focused run:

`35949611414`

At the time of this update it is queued on exact head `71b12cc...`.

Do not mark the overlap theorem green until the `SDF range-proxy verification (macOS)` job executes `Verify V5 fused overlap physics` successfully.

If it fails, fetch that job's log and preserve the analytical theorem. Fix only a real renderer/test-harness discrepancy; do not weaken the closed-form assertion merely to obtain green CI.

## Remaining risk / canonical reconciliation

Current canonical is ahead of the PR's historical base. The V5 branch must still be reconciled with canonical before landing, followed by final exact-head CI on the reconciled head.

Do not merge/rebase blindly: inspect canonical drift against V5-owned renderer/compiler/test/workflow surfaces first. Earlier drift was mostly outside V5-owned files, but canonical has continued advancing and must be rechecked at reconciliation time.

## Exact continuation point

1. Inspect run `35949611414`.
2. If `webgpu_v5_overlap_physics_test` fails, fetch the SDF/WebGPU job log and repair the narrow discrepancy it exposes.
3. If it passes, close the overlap-physics obligations in the V5 plan.
4. Re-inspect current canonical, reconcile PR #343 safely, and run final exact-head focused CI.
5. If all V5 plan obligations are then green, report V5 complete rather than inventing V6 scope.

## Guardrails

- No sequential whole-medium blending regression.
- No weakening analytical overlap assertions to match an incorrect renderer.
- No global volume cache flushes.
- No V6, multiple scattering, GI, spectral transport, or new Medium nouns.
- Targeted reads only; no big chungus.

— GPT-5.6 Sol
