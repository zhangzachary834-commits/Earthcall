# SUN HANDOFF — Volumetric V2 authored scattering + medium chroma

Date: 2026-09-22
Branch: `sol/volumetric-v2-authored-scattering-chroma-20260922`
Base at start: `f284b7794cd4fb8002b9e6e560cb59c26f6f21ea` (V1 merge)

## Constitution

Do not collapse authored meanings merely because they meet inside one transport integral.

```text
rho_source != V_transport != D_medium
D_medium != sigma_t != sigma_s != C_v
C_v != source chi != source alpha
```

V0 owns `volume.density.ast` as independent density truth. V1 owns
`volume.extinction.ast` as independent sigma_t truth. V2 adds two further
medium-owned channels:

```text
volume.scattering.ast   sigma_s(p,t) -> scalar
volume.chroma.ast       C_v(p,t)     -> vec3
```

`volume.scattering.ast` is deliberately scalar in this rung. Spectral/RGB
character belongs to `volume.chroma.ast`, so coefficient magnitude and chroma
remain independently authorable and independently invalidatable. Do not smuggle
future phase-function semantics into either channel.

## Compatibility migration

Absence must preserve the exact pre-V2 image:

```text
sigma_s = D
C_v = vec3(1)
```

That reproduces the old white compatibility term
`(D / sigma_t) * (oldT - T)`. Once authored, sigma_s and C_v are sole authority
for their meanings. Neither may mutate D, sigma_t, source chroma, source alpha,
or source radiance.

## First bounded pass completed

The V2 owner branch was created directly from the merged V1 canonical head.
Storage/authoring sovereignty is now established in `FieldNode`:

- independent `volumeScattering` Piecewise;
- independent `volumeChroma` Piecewise;
- PropertyPath/Law surfaces `volume.scattering.ast` and `volume.chroma.ast`;
- independent JSON persistence/hydration;
- absent channels remain empty, leaving compatibility resolution to the renderer.

## Next implementation pass

Continue from this branch; do not restart.

1. Project both channels through the medium binding without aliasing density/extinction.
2. Extend OntoMath CPU/WGSL inspection, lowering and parameter recollection:
   - scalar sigma_s;
   - vec3 C_v;
   - same admitted medium Timeline coordinate;
   - unsupported authored expressions refuse, never fall back to white/stale output.
3. Replace both white-scattering fossils (legacy SDF medium seam and dedicated
   depth-aware volume compositor) with the compatibility-preserving evaluators.
4. Extend program/cache identity so shared D/sigma_t with different sigma_s/C_v
   cannot collide.
5. Add focused witnesses for value/structure/time/refusal independence.
6. Add native witness: two media share byte-identical D but visibly scatter
   different chroma/strength.
7. Reconcile against canonical before landing; exact-head CI is mandatory.

## Explicitly out of scope

No V3 phase function, V4 emissive media, GI, or unrelated renderer refactor.


## Second bounded pass — renderer/compiler projection

Head before canonical reconciliation: `4238949dc1735ee045ef150c314062fbf6856bf7`.

Implemented:
- `VolumeDensityBinding` now carries independent sigma_s and C_v expression/revision channels;
- EngineRender medium-set identity includes D, sigma_t, sigma_s and C_v revisions;
- production OntoMath inspection/lowering/recollection now includes:
  - `inspectScatteringExpression` with absent `sigma_s=D` compatibility;
  - `inspectVolumeChromaExpression` with absent neutral-white compatibility and vector type validation;
  - both channels use the existing admitted medium Timeline coordinate;
- both renderer seams now replace the white-scattering fossil with:
  `C_v * (sigma_s / sigma_t) * (oldT - T)`;
- the legacy SDF seam and dedicated depth-aware volume compositor accumulate RGB/vector medium radiance rather than a white scalar;
- generic SDF memo invalidation includes sigma_s/C_v structure and value revisions;
- dedicated volume program identity is now the 4-tuple `(D, sigma_t, sigma_s, C_v)`, preventing shared-density/shared-extinction media with different scattering/chroma from colliding;
- refusal never falls back to D/white for authored unsupported V2 expressions;
- focused witnesses cover compatibility, independent numeric refresh, structural invalidation, Timeline lowering, and refusal.

Compatibility remains exact when both V2 channels are absent:
`sigma_s=D`, `C_v=vec3(1)`, reproducing the historical white term.

Still required before landing:
1. exact-head CI on the reconciled V2 head;
2. native pixel witness proving two media with byte-identical D can differ visibly in scattering strength/chroma;
3. final integration audit against canonical and PR state.


## Third bounded pass — native sovereignty witness

Pre-reconciliation implementation head: `2bf6923f7bd4c7dbf75795f9090625c69436bc08`.

Added the missing V2 landing witness in the existing native WebGPU object/composition harness rather than inventing a parallel test path:

- two `VolumeDensityBinding` media share the exact same `densityExpr` pointer and the exact same `extinctionExpr` pointer;
- they differ only in authored `scatteringExpr` and `volumeChromaExpr`;
- one is strong red scattering, the other weaker blue scattering;
- native RGBA8 readback asserts the red/blue hue split and the sigma_s strength difference;
- returning to the first medium asserts zero recompiles plus a volume-program cache hit, guarding the V2 four-channel cache identity.

The Zone persistence witness was also extended so `volume.scattering.ast` and
`volume.chroma.ast` are:
- PropertyPath-authorable;
- distinct Property beings from rho, D, and sigma_t;
- projected through `VolumeDensityBinding` with independent revisions;
- persisted/hydrated through the real Zone identity store;
- independently rewritable without mutating rho/D/sigma_t/C_v siblings.

Canonical advanced from `71d5d972` to `458949c1` during this pass. The single
canonical commit is task/document organization only and has no overlap with V2
production or test files. Reconcile onto that exact canonical before judging CI.

Landing gate after reconciliation:
1. exact-head focused CI green, including native WebGPU pixels;
2. branch ahead-only / no canonical overwrite noise;
3. PR integration state clean;
4. final handoff/PR comment records the landed architecture.


## Native witness execution evidence

On exact witness head `c2b24b1418c880d2930abf17ef6463314275258d`,
workflow run `35801207455` completed the **SDF range-proxy verification**
job successfully. Its successful steps include:
- Build SDF proof and GPU parity witnesses;
- Verify generic WebGPU SDF parity;
- Verify WebGPU SDF distance parity;
- Verify WebGPU authored-color parity;
- **Verify WebGPU object/radiance parity**.

That object/radiance parity step is the native harness containing the V2
same-D/same-sigma_t red-strong versus blue-weak pixel assertions, so the
native sovereignty witness executed successfully on macOS WebGPU.

Canonical then advanced to `c59d97d83e71106adcd877d0b3d1b14eb9134e19` via
PR #333. The only code delta from the prior base is
`src/ConstructedBeing/Material/Material.cpp` (face-texture init idempotency);
it does not overlap any V2 production/test file. Reconcile V2 onto this exact
canonical head and require the new exact-head workflow to finish green before
landing.
