# SUN UPDATE — V5 overlap physics native GREEN; canonical reconciliation is final gate

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #343 — Volumetric V5: medium-set composition foundation
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Fresh live-state inspection

Canonical was re-inspected first and is currently:

`0426ce24e3f5450d27301597dcd9c56dda360e92`

PR #343 was re-inspected at head:

`abed52dac7a145c06e88abf523df3b9f5526c273`

It remains open and Draft. V3/V4 are already landed and were not reopened.

## Native overlap-physics tribunal is GREEN

Focused CI run `35949640483` completed the SDF range-proxy / native WebGPU job successfully with the dedicated `webgpu_v5_overlap_physics_test` built and executed.

Observed native evidence:

- fused shared-extinction framebuffer: `(37,0,56)`
- analytical shared-integral expectation: `(37,0,56)`
- obsolete sequential whole-medium A→B counterfactual: `(73,0,35)`; RGB distance from native = `57`
- obsolete sequential whole-medium B→A counterfactual: `(12,0,71)`; RGB distance from native = `40`
- mixed independent source witness (A red `E_v`, B blue `sigma_s * C_v`): `(37,0,56)`
- mixed-source analytical expectation: `(37,0,56)`
- terminal result: `PASS: V5 fused overlap physics tribunal`

This closes the two remaining V5 physics obligations:

1. overlapping media use one combined extinction / shared transport integral rather than sequential whole-medium alpha composition;
2. independent authored source channels survive overlap — self-emission and scattering/chroma both contribute through the same fused transport law.

Together with the previously green native membership/refusal witness, V5 now has direct native evidence for:

- numeric-only member edits refreshing pixels without structural recompilation;
- time-only member edits refreshing pixels without structural recompilation;
- membership addition compiling only the new fused set structure;
- membership removal returning to a memoized prior set without recompiling it;
- refused admitted member failing closed with named refusal and no stale radiance replay;
- lawful set recovery after refusal;
- order-independent fused overlap;
- analytical combined-extinction correctness;
- strong numerical rejection of both sequential-alpha orderings;
- independent self-emission and scattering/chroma contribution under overlap.

No production renderer patch was needed in this pass: the implementation satisfied the analytical tribunal as written. Do not weaken or replace the witness.

## Canonical reconciliation audit

A fresh compare between PR head `abed52d...` and canonical `0426ce2...` reports divergence from merge base `b5fa341329176b0257d6de58f85f99ac6a286830`:

- canonical side: 101 commits ahead of the merge base relative to the PR head comparison;
- PR side: 33 commits not in canonical.

Targeted overlap inspection shows canonical changes include `.github/workflows/earthcall-ci.yml`, `CMakeLists.txt`, and `src/Singularity/Screen/Renderer.hpp`, which are integration-sensitive surfaces for #343. Canonical does not show competing edits in the V5-owned `WebGpuRenderer.cpp`, `SdfWgsl.cpp`, or the new V5 overlap test in the compare result.

Therefore the remaining work is reconciliation, not another V5 feature rung.

Do NOT manufacture a synthetic merge by blindly overlaying all PR files onto canonical. Preserve both canonical's newer CI/CMake/Renderer work and #343's V5 additions through a real three-way reconciliation, then rerun the exact-head focused matrix.

## Remaining risk

The V5 physics itself is now native-green. The principal remaining risk is integration drift: canonical has materially advanced on CI/build/Renderer surfaces since #343's merge base. A green pre-reconcile head does not prove the reconciled head.

## Exact continuation point

1. Re-inspect canonical immediately before reconciliation; do not reuse `0426ce2...` if it has moved.
2. Three-way reconcile canonical into `sol/volumetric-v5-medium-set-composition-current-20260923`, resolving only actual conflicts and preserving both sides' intent.
3. Verify that focused CI still explicitly builds/runs `webgpu_v5_overlap_physics_test` after workflow reconciliation.
4. Run the full focused exact-head CI matrix, including native WebGPU overlap and object/radiance parity.
5. If all exact-head checks are green and no new conflict exposes a V5 defect, V5 planned scope is complete. Mark PR #343 ready for review / merge; do not invent V6 scope.

## Guardrails

- No reopening completed V3/V4.
- No sequential whole-medium blending fallback.
- No numeric/time values in structural program identity.
- No global cache flush for membership edits.
- Refusal remains fail-closed with no stale current answer.
- No V6, multiple scattering, GI, spectral transport, or new Medium nouns.
- Targeted reads only; no big chungus.

— GPT-5.6 Sol