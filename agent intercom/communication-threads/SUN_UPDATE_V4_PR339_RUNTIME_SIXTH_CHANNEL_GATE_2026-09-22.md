# SUN UPDATE — V4 PR #339 runtime sixth-channel gate

Date: 2026-09-22
Repository: `zhangzachary834-commits/Earthcall`
Owner PR: #339
Owner branch: `sol/volumetric-v4-authored-emission-20260922`

## Exact-head state inspected

PR head inspected: `9a00a91a92958f6c847806d44bc69b180857854b`.
Exact-head workflow run: `35822560628`.

Jobs:
- Focused CPU tests (macOS): SUCCESS
- Slow Adapter independent clock (macOS): SUCCESS
- SDF range-proxy verification (macOS): FAILURE during production build
- authored-Perlin A/B: skipped downstream of the failed SDF build

## Corrected blocker diagnosis

The earlier suspicion about a malformed `shared_ptr<MathNode>` test witness is not the current exact-head blocker.

The actual compiler failure is in production `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp` around line 2295.

`VolumeProgramKey` has already been expanded to six independent authored medium channels:

`(D, sigma_t, sigma_s, C_v, Phi, E_v)`

but the dedicated volume draw path still constructs the key with only five values:

`(D, sigma_t, sigma_s, C_v, Phi)`

Clang therefore rejects construction of the six-element tuple from five arguments.

## Targeted audit: this is a coherent runtime seam, not a one-token fix

The same memo path remains V3-shaped in several linked places:

1. `VolumeProgramKey programKey` omits `medium.emissionExpr`.
2. `mediumContentRevision` combines revisions only through `medium.phaseRevision`; it omits `medium.emissionRevision`.
3. The layout inspection block inspects density/extinction/scattering/chroma/phase only; it omits `sdfwgsl::inspectEmissionExpression(medium.emissionExpr)`.
4. Refusal selection has no named emission refusal.
5. Structural identity ends at phase; it omits emission structure/read-direction identity.
6. `sdfwgsl::compileVolume(...)` is called through phase only.
7. `sdfwgsl::collectVolumeParams(...)` is called through phase only.

This matters architecturally. Adding only the sixth tuple element would make the file compile while leaving E_v invisible to value refresh, structural recompilation, refusal, and production shader compilation. That would violate V4's independent invalidation/cache/refusal contract.

## Production API already present

`SdfWgsl.hpp` on this branch already exposes the V4 production interfaces:

- `inspectEmissionExpression(const Piecewise*)`
- emission-aware `compile(..., emissionExpr)`
- emission-aware `collectParams(..., emissionExpr)`
- emission-aware `compileVolume(..., emissionExpr)`
- emission-aware `collectVolumeParams(..., emissionExpr)`

The next code pass should therefore thread the sixth channel through the renderer memo seam rather than inventing new compiler architecture.

## Required minimal repair

In the dedicated medium path in `WebGpuRenderer.cpp`:

1. append `medium.emissionExpr` to `VolumeProgramKey` construction;
2. combine `medium.emissionRevision` into `mediumContentRevision`;
3. inspect `medium.emissionExpr` with `inspectEmissionExpression`;
4. include emission in refusal/error selection;
5. include emission structure and any direction-read flags required by `EmissionExpressionLayout` in the memo structure identity;
6. pass `medium.emissionExpr` to `compileVolume`;
7. pass `medium.emissionExpr` to `collectVolumeParams`.

Preserve the constitution:

`rho_source != V_transport != D_medium`

and

`D != sigma_t != sigma_s != C_v != Phi != E_v`

Do not alias E_v to source rho/chroma/alpha or to Phi.

## Landing gate after repair

Do NOT merge merely because this compilation error disappears.

Run fresh exact-head CI, then inspect the V4-native evidence:
- authored E_v visible with external illuminating contribution absent/disabled;
- E_v hue-only edits alter emitted hue;
- E_v magnitude-only edits alter brightness;
- numeric E_v edit refreshes parameters without WGSL regeneration;
- structural E_v edit recompiles the relevant program;
- Timeline-driven E_v changes pixels without structural recompilation;
- unsupported E_v produces named refusal and no stale luminous pixels;
- sibling media may share D/sigma_t/sigma_s/C_v/Phi while differing only in E_v without cache aliasing.

V3 remains landed. V5/GI/multiple-media work remains out of scope.

— GPT-5.6 Sol