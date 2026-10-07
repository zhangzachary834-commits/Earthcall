# Sun Update — Volumetric V3 Authored Phase / PR #337

Date: 2026-09-22
Repository: `zhangzachary834-commits/Earthcall`
Canonical branch: `sync-from-earthcall-main`
V3 branch: `sol/volumetric-v3-authored-phase-20260922`
PR: #337

## Do not restart

V0/V1/V2 are landed. V3 is now an active draft implementation, not a greenfield design exercise.

Constitution remains:

```text
rho_source != V_transport != D_medium
D_medium != sigma_t != sigma_s != C_v != Phi
Phi != source alpha
```

Target V3 authored truth:

```text
volume.phase.ast
Phi(p, wi, wo, t) -> scalar
```

## State reached this pass

The pre-existing V3 branch already contained the main storage/compiler/runtime seam. This pass inspected it rather than rebuilding it.

Important latest semantic correction on the branch: absent phase takes the **literal V2 compatibility arithmetic path**. We do not manufacture/evaluate a synthetic phase and multiply by one in the absent case. Only authored phase enters directional evaluation.

For authored phase, the renderer derives world-space directions at the medium sample. `wo` is medium sample -> eye. `wi` is source -> medium sample under the current single incident-source seam. If authored phase reads `wi` and there is no valid incident source/direction, the phase contribution is not allowed to fabricate one.

## Canonical reconciliation

Canonical had advanced to:

`3e46af88ed9986a727b8696598deac3b9917ed29`

The incoming substantive side was the Borealis Sanctuary V1/V2 demonstration save. It did not overlap the nine V3 implementation files.

V3 was reconciled onto the current canonical tree without discarding that demo. Reconciled code head before this Intercom-doc commit:

`93cb4d8db9e9a2a40e4d6b49bb6c9a0f36814973`

PR #337 was opened as Draft against current canonical.

## Current implementation footprint

Nine code files differ from canonical:

- `src/ConstructedBeing/Singular/Object/Geometry/FieldNode.cpp`
- `src/ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp`
- `src/Singularity/Core/EngineRender.cpp`
- `src/Singularity/OntoMath/ScalarForm.hpp`
- `src/Singularity/Screen/VolumeDensity.hpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.hpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp`

## Still required before V3 can land

Do not mark Ready or merge yet. The branch still needs executable evidence for the full contract:

1. PropertyPath `volume.phase.ast` reachability and persistence/fresh hydration.
2. Independence witness: phase rewrite leaves D / sigma_t / sigma_s / C_v / source alpha untouched.
3. Numeric phase edit refreshes parameters without WGSL regeneration.
4. Structural phase edit recompiles the correct program identity.
5. Timeline-driven phase changes pixels without AST rewrite / structural recompilation.
6. Unsupported authored phase refuses with no stale phase output.
7. Native directional framebuffer witness: hold D / sigma_t / sigma_s / C_v fixed and change only Phi; pixels must differ because angular scattering differs.
8. Exact-head focused/native CI green after the final witness head.
9. Re-check canonical integration immediately before landing because other Suns are active.

## V4 boundary

Do not smuggle V4 into this PR. `volume.emission.ast -> E_v(p,omega,t)` remains a separate rung/PR after V3 is architecturally settled. V5 multiple-media world composition and GI remain out of scope.

Carry the torch from PR #337; do not restart the investigation.
