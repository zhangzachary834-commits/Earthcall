# SUN UPDATE — V5 medium-set composition campaign opened

Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Read first

Prior landed handoff:
`agent intercom/communication-threads/ontomath-light-and-image/SUN_HANDOFF_V3_V4_LANDED_VOLUMETRIC_ARCHITECTURE_2026-09-23.md`

V3/V4 are landed. Do not restart them.

## Canonical race / reconciliation

This pass began from `496f42516264b804328ab3deb5905b74e5c66481`.

While the pass was active, canonical advanced one commit to:

`b5fa341329176b0257d6de58f85f99ac6a286830` — Interaction Robustness

Exact compare showed that incoming commit touches interaction/event/docs files and does not overlap the four V5a implementation/witness files.

Rather than carry a one-behind branch, the V5 slice was reconstructed byte-for-byte onto a fresh-current branch from `b5fa3413`.

## Architectural finding

Earthcall already projects a vector of participating media, but V5 is NOT therefore done.

The current depth-aware compositor integrates each medium independently, returns premultiplied `C_medium, alpha=1-T_medium`, and blends each whole-medium result using `ONE, ONE_MINUS_SRC_ALPHA`.

That is exact for one medium, but overlapping media are still sequential framebuffer composition. Whole-medium draw/group order can become an accidental physical law.

V5 must instead establish shared ray transport for overlap: at each ray interval combine the active media's extinction/source terms, update one transmittance, and accumulate one radiance answer. Medium ordering must not change pixels.

See:
`docs/plans/VOLUMETRIC_V5_MEDIUM_SET_COMPOSITION_PLAN_2026-09-23.md`

## V5a implemented

1. `VolumeDensity.hpp`
   - added canonical `volumeContentRevision(binding)`;
   - includes all six authored channels through V4;
   - added `appendVolumeSetIdentity(...)` beside the binding contract.

2. `EngineRender.cpp`
   - replaced duplicated medium identity assembly with the canonical helper;
   - repairs the V4-era omission where `emissionRevision` was absent from `volumeSetIdentity`.

3. `WebGpuRenderer.cpp`
   - renderer per-medium memo now consumes the same canonical six-channel fingerprint rather than duplicating the hash-combine recipe.

4. `zone_spatial_field_roundtrip_test.cpp`
   - added a focused witness that changing only `emissionRevision` changes both the per-medium content fingerprint and world medium-set identity.

## Why this matters

Before this pass, WebGPU knew E_v belonged to the six-channel program identity, but EngineRender's collection revision did not. That split would be poisonous for V5 incremental set composition: different layers of the architecture could disagree about whether the world-level medium set changed.

V5a makes the identity seam coherent before fused transport is introduced.

## Next exact continuation point

V5b: add an order-reversal witness for two spatially overlapping media, demonstrate the present sequential-composition fossil, then replace it with the smallest fused sample-level transport path that:

- preserves single-medium V4 pixels;
- sums active extinction/source terms per interval;
- is permutation invariant;
- keeps numeric-vs-structural invalidation;
- does not introduce GI or multiple scattering.

Do not "fix" V5 by sorting the media. Sorting only chooses a deterministic wrong order; it does not make independent whole-medium compositing into shared transport.

— GPT-5.6 Sol
