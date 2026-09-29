# SUN HANDOFF — V3 + V4 LANDED: authored angular scattering and self-emissive media

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
Canonical: `sync-from-earthcall-main`

## Status

Volumetric V3 and V4 are both landed. Do not restart either implementation campaign.

V3 landed through PR #337:
- merge commit: `8977bb352e9ef7184ef13942fc8e6274ae37e981`
- final exact-head CI: `35815863871` GREEN

V4 landed through PR #339:
- final tested head: `62b35042e55dfea99609026174e40ce55b825cdd`
- exact-head CI: `35840840411` GREEN after the Slow Adapter retry settled successfully
- merge commit: `48e1ad23acbf2a2c55991f2e19fccb19b592784b`

The V4 merge commit has tree `f978d8a788bace3b86151dbadc62dd2cd06b67c2`, exactly the same tree as the green tested V4 head. Therefore the landed code content is the exact code content that passed the final PR gate.

## Landed constitution

Preserve:

```text
rho_source != V_transport != D_medium
D_medium != sigma_t != sigma_s != C_v != Phi != E_v
Phi != source alpha
E_v != source rho/chroma/alpha
```

Authored medium truth now includes:

```text
volume.density.ast    -> D(p,t)
volume.extinction.ast -> sigma_t(p,t)
volume.scattering.ast -> sigma_s(p,t)
volume.chroma.ast     -> C_v(p,t)
volume.phase.ast      -> Phi(p,wi,wo,t)
volume.emission.ast   -> E_v(p,omega,t) -> vec3
```

V3 phase and V4 emission may use physically related direction vectors at a sample, but they remain distinct authored invariants and compiler contexts. Source `light.angular.ast` / alpha is separate from both.

## What V3 established

V3 made medium angular scattering independently authorable rather than reusing source alpha. It includes persistence/PropertyPath reachability, OntoMath CPU/WGSL lowering, independent numeric/structural/time invalidation, refusal semantics, renderer integration, cache identity, and native directionality witnesses while D/sigma_t/sigma_s/C_v remain fixed.

## What V4 established

V4 made the medium itself a source of radiance through independent authored `E_v`.

The production path now includes:
- `FieldNode::volumeEmission` authored storage;
- PropertyPath/Law vocabulary `volume.emission.ast`;
- JSON persistence/hydration;
- `VolumeDensityBinding::{emissionExpr, emissionRevision}`;
- emission-owned vec3/angular OntoMath inspection and WGSL lowering;
- generic and dedicated depth-aware transport integration;
- six-channel medium program identity `(D, sigma_t, sigma_s, C_v, Phi, E_v)`;
- independent numeric parameter refresh and structural recompilation semantics;
- Timeline lowering;
- named refusal for unsupported authored emission;
- no stale luminous output after refusal.

Absent E_v preserves the pre-V4 compatibility state: no self-emitted radiance.

## Native V4 proof

The final native framebuffer witness deliberately removes alternate explanations for emitted light:
- admitted external radiance sources are empty;
- authored `sigma_s = 0`, so ordinary scattering cannot create the measured light;
- control and emissive media hold D/sigma_t/sigma_s/C_v/Phi fixed;
- control with no E_v renders `(0,0,0)`;
- authored E_v renders `(255,45,15)`;
- numeric-only E_v edits visibly change pixels with `volumeProgramCompiles == 0`;
- unsupported E_v produces a named refusal and no stale self-emission.

A production bug discovered by this witness was repaired: the numeric refresh path originally recollected volume parameters without `medium.emissionExpr`, even though structural compilation already included it. The final path includes E_v in value refresh as well.

The later telemetry-only assertion requiring `volumeProgramCacheHits >= 1` on a numeric authored edit was removed because content-revision changes intentionally enter the value-refresh branch. The truthful proof is pixel change plus zero program recompiles.

## Final CI interpretation

Final V4 exact-head run `35840840411` ultimately completed SUCCESS. The V4-native SDF/WebGPU job, Focused CPU, authored-Perlin A/B, and Slow Adapter job are green. The Slow Adapter job initially failed an unrelated cadence assertion and passed on retry; no unrelated Slow Adapter code was changed in V4.

## What is now possible

Earthcall's participating medium is no longer merely density-shaped colored scattering. A medium can own its spatial existence, extinction, scattering strength, chroma, angular scattering response, and self-emitted radiance as separate authored truths.

This is sufficient substrate for a truthful aurora-style world: the auroral medium can emit its own visible radiance without pretending an external light source is illuminating colored fog.

## Do not smuggle in V5

This handoff closes V3/V4 only. It does not authorize multiple-media architecture, multiple scattering, GI, path tracing, spectral transport, or unrelated renderer rewrites.

Any successor work should begin from the landed V3/V4 constitution above and define its own bounded rung.

— GPT-5.6 Sol, 2026-09-23
