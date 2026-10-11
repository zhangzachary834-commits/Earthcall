# SUN RECONCILIATION — Sparkly volumetric light vs. established radiance / volume invariants

<!-- NAV_BLOCK_START -->
> [!NOTE]
> **Thread Navigation: OntoMath Light & Image (Misc)**
> [View Full Thread Index](../00_THREAD_INDEX.md)
>
> **Related in this thread:**
> - [Light_First_Order_Authorability_Handoff_2026-09-11.md](Light_First_Order_Authorability_Handoff_2026-09-11.md)
> - [OntoMath_Image_Ingestion_Phase_1_Update.md](OntoMath_Image_Ingestion_Phase_1_Update.md)
> - [SUN_AUDIT_V3_V4_HANDOFF_SCOPE_COMPLETE_2026-09-23.md](SUN_AUDIT_V3_V4_HANDOFF_SCOPE_COMPLETE_2026-09-23.md)
> - [SUN_AUDIT_VOLUMETRIC_TORCH_SCOPE_STILL_COMPLETE_2026-09-23.md](SUN_AUDIT_VOLUMETRIC_TORCH_SCOPE_STILL_COMPLETE_2026-09-23.md)
> - [SUN_DISPATCH_WHAT_RUNG10_ACTUALLY_FOUND_2026-09-26.md](SUN_DISPATCH_WHAT_RUNG10_ACTUALLY_FOUND_2026-09-26.md)
> - [SUN_HANDOFF_RUNG9_COMPLETE_AUTHORED_MATERIAL_RESPONSE_2026-09-25.md](SUN_HANDOFF_RUNG9_COMPLETE_AUTHORED_MATERIAL_RESPONSE_2026-09-25.md)
> - [SUN_HANDOFF_V3_V4_LANDED_VOLUMETRIC_ARCHITECTURE_2026-09-23.md](SUN_HANDOFF_V3_V4_LANDED_VOLUMETRIC_ARCHITECTURE_2026-09-23.md)
> - [SUN_HANDOFF_V4_AFTER_V3_LANDING_2026-09-22.md](SUN_HANDOFF_V4_AFTER_V3_LANDING_2026-09-22.md)
> - [SUN_HANDOFF_V4_EMISSION_DIRECTION_AND_TRANSPORT_NEXT_PASS_2026-09-22.md](SUN_HANDOFF_V4_EMISSION_DIRECTION_AND_TRANSPORT_NEXT_PASS_2026-09-22.md)
> - [SUN_HANDOFF_V5_COMPLETE_READY_TO_LAND_2026-09-24.md](SUN_HANDOFF_V5_COMPLETE_READY_TO_LAND_2026-09-24.md)
> - [SUN_POST_V5_CONSTITUTION_NULL_PARTICIPANT_SAMPLING_STABILITY_2026-09-24.md](SUN_POST_V5_CONSTITUTION_NULL_PARTICIPANT_SAMPLING_STABILITY_2026-09-24.md)
> - [SUN_RUNG9_AUTHORED_MATERIAL_RESPONSE_CONSTITUTION_2026-09-24.md](SUN_RUNG9_AUTHORED_MATERIAL_RESPONSE_CONSTITUTION_2026-09-24.md)
> - [SUN_TO_BLEP_DRAGON_Chromatic_Radiance_Field_2026-09-20.md](SUN_TO_BLEP_DRAGON_Chromatic_Radiance_Field_2026-09-20.md)
> - [SUN_UPDATE_VOLUMETRIC_V3_PHASE_PR337_2026-09-22.md](SUN_UPDATE_VOLUMETRIC_V3_PHASE_PR337_2026-09-22.md)
<!-- NAV_BLOCK_END -->


**Date:** 2026-09-24
**Repository:** `zhangzachary834-commits/Earthcall`
**Canonical Sparkly landing inspected:** `sync-from-earthcall-main@1d84821fde1b27cacf1fb885c9bc3b1de52547a2`  
**Reconciled through current canonical:** `55974af259ebb9f27a29341ff77fa780b23baeb4` via two-parent merge `ae6128b66b90764f787761763a1ca5263eba47af`
**Sparkly / Antigravity commit:** `1d84821fde1b27cacf1fb885c9bc3b1de52547a2`
**Reconciliation branch:** `sol/rung9-sparkly-overlap-reconciliation-20260924`

## Verdict

The Sanctuary of Sunlit Mist work is real new renderer functionality and should be preserved, but its first canonical landing crossed three already-established semantic boundaries.

It does **not** implement Rung 9 material response. The overlap is primarily with:
- Rungs 3–7 authored source radiance `rho / chi / alpha`,
- Rung 8 visibility scope,
- Volumetric V3 phase `Phi`,
- Volumetric V5 cache / multi-medium transport.

Rung 9 should therefore continue only after this cross-rung reconciliation is green.

## Preserve

The reconciliation deliberately preserves:
- `volume.occluder.sdf` as an authored/Law-reachable local volumetric transport input;
- the Sanctuary world / Zone and its light-beam phenomenon;
- direct incident source rho/chroma/angular participation in volumetric scattering;
- V5 occupied-segment and fused multi-medium transport;
- Sparkly's native volumetric mist and sanctuary-load tribunals.

## Repaired constitutional crossings

### 1. Absent authored phase must remain V3 identity

V3 established:

```
Phi_absent = 1
```

The Sparkly landing instead injected renderer-owned Henyey-Greenstein behavior with a hardcoded `g = 0.55` whenever an incident source existed. That made "no authored Phi" stop meaning isotropic compatibility.

Repair: remove the hardcoded HG fallback. Forward-scattering is lawful only when the medium explicitly authors `volumePhase`.

### 2. Source time must remain source-owned

The Sparkly volume compiler emitted source `rho/chi/alpha` while the emitter's `t` binding still pointed at `instances[g_instIdx].time.x`, i.e. the current **medium** Timeline.

That violates Rung 7's per-source temporal independence.

Repair: volume globals now carry the admitted source's own `sourceTime`; source rho/chi/alpha lower against `u.sourceTime.x`, while D/sigma_t/sigma_s/C_v/Phi/E_v continue using the medium instance time.

### 3. Importing source ASTs requires importing their invalidation authority

The volume program key contained source-expression pointers, but memo refresh was driven only by volume revisions and its structural identity omitted source rho/chi/alpha layouts. An in-place source numeric or structural edit could therefore leave stale incident light inside an otherwise reused volume program.

Repair:
- combine the admitted source's rho/chi/alpha revisions into the volume memo content revision;
- inspect and include source rho/chi/alpha structure in the memo structural identity;
- on numeric-only source edits recollect volume/source parameter values without regenerating WGSL;
- on source structural edits allow the normal structure comparison to trigger recompilation.

## Rung 8 scope quarantine

`volume.occluder.sdf` is currently **local participating-medium transport geometry**. It is not hereby promoted to Earthcall's global visibility ontology.

Merged Rung 8 already defines visibility as derived source-to-receiver geometric transport. The current renderer still has known scene-spatial authority limits. Until a common scene geometry authority is proven, the medium-local occluder may provide an honest local beam/shadow witness but must not be described as the final scene-wide V(source,p,omega).

## New proof gates

The reconciliation adds:
- CPU/compiler witness that incident source `t` lowers to `u.sourceTime.x`, not the medium's instance time;
- CPU/compiler witness that absent Phi remains exact identity even with incident light;
- CPU/compiler witness that numeric source rho refreshes parameters without WGSL regeneration;
- native WebGPU mist witness that an in-place source-rho numeric change reaches the existing volume program without recompilation;
- focused CI build/run wiring for `webgpu_volumetric_mist_test`;
- focused CI build/run wiring for `sanctuary_of_sunlit_mist_test`;
- all pre-existing V5 overlap gates remain in the same focused job.

## Documentation correction

The Sanctuary currently does not author `volumePhase`. Therefore the first landing's claim that its forward-scattering comes from a canonical HG `g≈0.55` medium law was too strong: that behavior lived in renderer fallback state.

After reconciliation the scene uses the V3 isotropic compatibility phase unless/until the Sanctuary explicitly authors a phase AST. The beam/shadow phenomenon remains testable through incident illumination + medium scattering + authored occluder visibility.

## Exact continuation

1. Open this reconciliation as a draft PR against live `sync-from-earthcall-main`.
2. Inspect exact-head focused CI.
3. Fix only concrete reconciliation regressions; do not expand into Rung 9.
4. If canonical advances, ancestry-preserving reconcile and rerun exact-head CI.
5. Once green and landed, advance the clean current-head Rung-9 material-response lane.
6. Do not merge old stale Rung-9 history over this work; the fresh Rung-9 lane was recreated from the Sparkly canonical head.

No force-pushes. No snapshot overwrites. No deletion of the Sanctuary merely to restore old semantics.
