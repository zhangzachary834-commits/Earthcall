# SUN HANDOFF — V4 EMISSION DIRECTION + TRANSPORT NEXT PASS

Date: 2026-09-22
Repository: `zhangzachary834-commits/Earthcall`
Canonical: `sync-from-earthcall-main`
Owner branch: `sol/volumetric-v4-authored-emission-20260922`
Owner PR: #339 — Volumetric V4: authored emissive media (Draft)

## Do not restart V3

V3 landed through PR #337.

Final tested V3 head:
`e697d475bb08d0823a2156836c9c58a6a2f8f2c3`

V3 merge commit:
`8977bb352e9ef7184ef13942fc8e6274ae37e981`

Exact-head CI run `35815863871` completed SUCCESS across:
- Focused CPU tests
- SDF range-proxy/native WebGPU verification, including the phase framebuffer witness
- Slow Adapter independent clock
- SDF authored-Perlin A/B

Canonical then advanced through unrelated Person serialization PR #338 to:
`e4eeee50d1c6982d262c676a0d9d8b55212f959f`

The V4 owner branch was reconciled onto that canonical before this handoff. Re-check live heads when you arrive.

## Constitution

Preserve:

`rho_source != V_transport != D_medium`

and independent medium truths:

`D != sigma_t != sigma_s != C_v != Phi != E_v`

V4 target:

`volume.emission.ast -> E_v(p, omega, t) -> vec3`

Do not alias `E_v` to source rho/chi/alpha, surface emission, density, scattering, medium chroma, or phase.

## What is already implemented

Rung 1 on the owner branch establishes authored truth and projection only:

- `FieldNode::volumeEmission` is an independent `Piecewise`.
- PropertyPath/Law vocabulary is `volume.emission.ast`.
- JSON save/hydration persists `volumeEmission`.
- `VolumeDensityBinding` projects `emissionExpr` and `emissionRevision`.
- absent emission means no self-emitted radiance.

No renderer transport behavior has been added yet. That separation is intentional.

## Decision from this pass: truthful omega seam

Audit the live renderer before coding, but the current transport already possesses a truthful outgoing direction at every sampled medium point: world-space `sample -> eye`. V3 calls this physical direction `wo`.

For V4, the first directional emission contract may therefore bind `omega` to that same *physical vector* while keeping V4's authored variable/context semantically distinct from:

1. V3 phase `wo`, which participates in `Phi(p,wi,wo,t)`; and
2. source `alpha` omega, whose meaning is source -> receiver.

Same vector at a particular sample does not mean same authored invariant.

If implementing a distinct V4 omega context would otherwise require lying or accidentally aliasing source-alpha/V3 variable namespaces, land the truthful subset `E_v(p,t)` first and explicitly reserve directional omega. Do not fabricate a direction.

## Next production pass

### 1. Compiler layout

Add a dedicated V4 emission inspector/lowering path. Reuse the production vector emitter/type/refusal machinery, but give emission its own named layout/context so cache/refusal diagnostics say `volume emission`, not `volume chroma` or source chroma.

Required semantics:
- vec3 only;
- p and t admitted;
- omega admitted only under the truthful direction contract above;
- unsupported ops refuse;
- wrong type refuses;
- absence is a compatibility state, not authored zero.

### 2. Generic SDF volume path

Thread `emissionExpr` after phase through `compile()` and `collectParams()`.

Generate an emission evaluator only when authored. Keep absent V4 on literal V3 arithmetic where practical.

During medium integration, accumulate self-emission separately from V2/V3 in-scatter. Do not multiply E_v by source radiance, sigma_s, C_v, or Phi.

### 3. Dedicated depth-aware volume compositor

Thread emission through `compileVolume()` / `collectVolumeParams()` and the volume program cache identity.

The current analytical attenuation step uses old/new transmittance. Integrate E_v with the actual marched interval and current attenuation law; do not blindly paste pseudocode. The invariant is that emission created deeper in the medium is attenuated on its path to the eye, while it does not require an external source.

### 4. Cache/invalidation

Program identity progresses from:

`(D, sigma_t, sigma_s, C_v, Phi)`

to:

`(D, sigma_t, sigma_s, C_v, Phi, E_v)`

Prove:
- numeric E_v edit refreshes params without WGSL regeneration;
- structural E_v edit recompiles the relevant program;
- Timeline E_v changes runtime values/pixels without AST rewrite;
- sibling media differing only in E_v cannot collide in cache identity.

### 5. Refusal/no stale light

Authored unsupported/wrongly typed E_v must produce a named refusal. It must not silently become zero and it must not leave stale luminous pixels from a previous valid program.

## Mandatory witnesses before landing

Hold D / sigma_t / sigma_s / C_v / Phi fixed.

Prove:
- authored E_v creates native framebuffer radiance with external illuminating contribution absent/disabled;
- only E_v hue changes -> emitted hue changes;
- only E_v magnitude changes -> brightness changes;
- numeric E_v edit -> no shader regeneration;
- structural E_v edit -> appropriate recompile;
- Timeline E_v -> pixel change without structural recompile;
- unsupported E_v -> named refusal + no stale luminous output;
- save -> fresh hydration preserves `volume.emission.ast` exactly;
- Law/Property rewrite of E_v leaves rho/D/sigma_t/sigma_s/C_v/Phi/source-alpha byte-identical.

## Landing discipline

Keep PR #339 Draft until production lowering, transport integration, persistence, invalidation, refusal, native pixels, and exact-head CI are all green.

Before every substantial edit, re-check canonical and PR head because multiple Suns may be active. Reconcile semantically; never overwrite newer work.

No V5 multiple-media architecture, GI, multiple scattering, path tracing, spectral rewrite, or unrelated renderer refactor in this PR.

After V4 is genuinely merged and post-merge state is settled, write the final architecture handoff and disable the V3/V4 torch automation.

— GPT-5.6 Sol
