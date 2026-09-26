# SUN HANDOFF — V3 LANDED, V4 AUTHORED EMISSION RUNG 1

Date: 2026-09-22
Repository: zhangzachary834-commits/Earthcall
Canonical branch: sync-from-earthcall-main

## Canonical state at handoff

Volumetric V3 was merged through PR #337.

Final tested V3 head:
`e697d475bb08d0823a2156836c9c58a6a2f8f2c3`

V3 merge commit / canonical observed at V4 branch creation:
`8977bb352e9ef7184ef13942fc8e6274ae37e981`

Exact-head V3 CI run:
`35815863871`

All four jobs completed SUCCESS:
- Focused CPU tests (macOS)
- SDF range-proxy verification (macOS), including the native phase framebuffer witness
- Slow Adapter independent clock (macOS)
- SDF authored-Perlin A/B (macOS Release)

Do NOT restart V3. Treat it as canonical unless live canonical shows a later corrective change.

## Constitution to preserve

`rho_source != V_transport != D_medium`

Independent authored medium truths now canonical:

`volume.density.ast    -> D(p,t)`
`volume.extinction.ast -> sigma_t(p,t)`
`volume.scattering.ast -> sigma_s(p,t)`
`volume.chroma.ast     -> C_v(p,t)`
`volume.phase.ast      -> Phi(p,wi,wo,t)`

Source `light.angular.ast` / alpha is a different invariant from medium phase.

V4 adds, independently:

`volume.emission.ast -> E_v(p,omega,t) -> vec3`

Do not alias this to density, scattering, C_v, source rho, source chi, source alpha, or a surface emissive material.

## V4 branch

Owner branch:
`sol/volumetric-v4-authored-emission-20260922`

This branch was created directly from the V3 merge commit.

## Rung 1 implemented on this branch

The first V4 storage/projection seam is now present:

1. `FieldNode` owns a new independent `volumeEmission` Piecewise.
2. PropertyPath/Law vocabulary is `volume.emission.ast`.
3. FieldNode JSON persists/hydrates `volumeEmission`.
4. `VolumeDensityBinding` projects:
   - `emissionExpr`
   - `emissionRevision`
5. Null/absent emission is explicitly the compatibility state: NO self-emitted radiance.

This first rung intentionally does NOT yet alter renderer transport. It creates authored truth and the Screen projection seam first.

## Next implementation pass — do not skip directly to pretty aurora

### 1. OntoMath emission inspection/lowering

Add a production vector-layout inspector for V4 emission using the same type/refusal discipline as V2 `C_v`, but with its own angular context.

Target meaning:

`E_v(p,omega,t) -> vec3`

Important: decide the exact `omega` binding from CURRENT transport semantics before exposing it. The most truthful likely first direction is sample -> eye / outgoing transport direction, but inspect the V3 `wo` binding and source angular `omega` semantics before canonizing this. Do not invent a direction the renderer does not actually know.

If the first truthful rung can only support `E_v(p,t)` while reserving directional input, document that limitation rather than fabricating omega.

### 2. Compiler signatures / parameter recollection

Thread independent `emissionExpr` through:
- generic SDF volumetric compile path where medium transport exists;
- dedicated depth-aware `compileVolume` compositor;
- parameter recollection;
- structure/value inspection.

Absent emission must preserve literal V3 arithmetic/pixels rather than generating semantically active fake emission code where avoidable.

### 3. Transport integration

Add self-emitted radiance to the medium integral separately from in-scatter.

Conceptually:

`radiance += attenuated/scaled E_v * ds`

but DO NOT copy this pseudocode blindly. Inspect the current analytical step integration and premultiplied alpha/composition behavior.

Emission must be visible without an external illuminating source.

### 4. Renderer cache identity/invalidation

Extend both generic and dedicated medium memo identities to include emission independently.

Dedicated volume-program key should progress from:
`(D, sigma_t, sigma_s, C_v, Phi)`

to:
`(D, sigma_t, sigma_s, C_v, Phi, E_v)`

Numeric E_v edits must refresh parameters without WGSL regeneration when structure is unchanged.
Structural E_v edits must recompile exactly the relevant program.
Timeline E_v changes must be runtime data, not AST rewrites.

### 5. Refusal semantics

Unsupported or wrongly typed authored E_v must refuse.
It must NOT silently fall back to zero after authorship was attempted.
A refusal must not leave stale luminous pixels from a prior valid emission program.

### 6. Mandatory native witnesses

Hold D / sigma_t / sigma_s / C_v / Phi fixed.

Prove:
- medium with authored E_v produces native framebuffer radiance with external illuminating contribution absent/disabled;
- changing only E_v chroma changes emitted hue;
- changing only E_v magnitude changes emitted brightness;
- numeric E_v edit -> no shader regeneration;
- structural E_v edit -> one appropriate recompile;
- Timeline-driven E_v changes pixels without structural recompilation;
- unsupported E_v -> named refusal and no stale self-emission;
- cache key can retain sibling media sharing D/sigma_t/sigma_s/C_v/Phi while differing only in E_v.

### 7. Persistence / PropertyPath witness

Extend `zone_spatial_field_roundtrip_test`:
- PropertyPath can set/read `volume.emission.ast`;
- fresh save/hydration preserves it exactly;
- it is a distinct Property being from rho/D/sigma_t/sigma_s/C_v/Phi/source alpha;
- malformed write is atomic;
- Law-style E_v rewrite leaves sibling truths byte-identical.

## Out of scope

Do NOT implement V5 multiple-media architecture in this PR.
Do NOT add GI, multiple scattering, path tracing, spectral transport, or unrelated renderer refactors.

A small aurora-style demonstration is welcome ONLY after the architectural/native witnesses are green. V4 is the rung where the aurora can truthfully shine by itself, but demo aesthetics are not evidence.

## Working discipline

- Reinspect current canonical before every substantial edit; other Suns may advance it.
- Targeted reads only. NO BIG CHUNGUS.
- Never overwrite newer canonical work blindly.
- Prefer a separate draft PR for V4.
- Keep exact-head CI as the landing authority.
- Merge only when renderer semantics, persistence, invalidation, refusal, native pixels, and integration state are all settled.
- When V4 is merged and post-merge state is green, update final architecture docs and disable the recurring V3/V4 torch automation.

— GPT-5.6 Sol, 2026-09-22


## Reconciliation after branch creation

Canonical advanced immediately after V4 branch creation through PR #338 to:
`e4eeee50d1c6982d262c676a0d9d8b55212f959f`

Audited delta from V3 merge:
- `src/Singularity/Storage/Serialization/Person/PersonSerialization.cpp`
- `tests/person/person_serialization_test.cpp`

No overlap with V4 storage/projection files.

V4 was therefore rebuilt on the actual new canonical tree rather than retaining stale canonical history.

Draft owner PR:
#339 — Volumetric V4: authored emissive media

Rung-1 exact reconciled head is recorded in the latest PR/branch state; re-inspect before editing because other Suns may advance canonical again.
