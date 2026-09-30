# Sun Handoff — Rung 9 Complete: Authored Material / Receiving-Surface Response — 2026-09-25

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
> - [SUN_HANDOFF_V3_V4_LANDED_VOLUMETRIC_ARCHITECTURE_2026-09-23.md](SUN_HANDOFF_V3_V4_LANDED_VOLUMETRIC_ARCHITECTURE_2026-09-23.md)
> - [SUN_HANDOFF_V4_AFTER_V3_LANDING_2026-09-22.md](SUN_HANDOFF_V4_AFTER_V3_LANDING_2026-09-22.md)
> - [SUN_HANDOFF_V4_EMISSION_DIRECTION_AND_TRANSPORT_NEXT_PASS_2026-09-22.md](SUN_HANDOFF_V4_EMISSION_DIRECTION_AND_TRANSPORT_NEXT_PASS_2026-09-22.md)
> - [SUN_HANDOFF_V5_COMPLETE_READY_TO_LAND_2026-09-24.md](SUN_HANDOFF_V5_COMPLETE_READY_TO_LAND_2026-09-24.md)
> - [SUN_POST_V5_CONSTITUTION_NULL_PARTICIPANT_SAMPLING_STABILITY_2026-09-24.md](SUN_POST_V5_CONSTITUTION_NULL_PARTICIPANT_SAMPLING_STABILITY_2026-09-24.md)
> - [SUN_RECONCILIATION_SPARKLY_VOLUMETRIC_LIGHT_VS_RUNGS_3_9_2026-09-24.md](SUN_RECONCILIATION_SPARKLY_VOLUMETRIC_LIGHT_VS_RUNGS_3_9_2026-09-24.md)
> - [SUN_RUNG9_AUTHORED_MATERIAL_RESPONSE_CONSTITUTION_2026-09-24.md](SUN_RUNG9_AUTHORED_MATERIAL_RESPONSE_CONSTITUTION_2026-09-24.md)
> - [SUN_TO_BLEP_DRAGON_Chromatic_Radiance_Field_2026-09-20.md](SUN_TO_BLEP_DRAGON_Chromatic_Radiance_Field_2026-09-20.md)
> - [SUN_UPDATE_VOLUMETRIC_V3_PHASE_PR337_2026-09-22.md](SUN_UPDATE_VOLUMETRIC_V3_PHASE_PR337_2026-09-22.md)
<!-- NAV_BLOCK_END -->



## Live completion evidence

At the completion audit, PR #375 (`sol/rung9-material-response-20260924`) had implementation/test head `2000633e6f4be5845c45317545364a4a47d15075`.

Canonical `sync-from-earthcall-main` was `c645315d7a099e22188b869bb47b04dab68fcfad`. The branch was 28 commits ahead and 0 behind, with canonical as its merge base; GitHub reported the PR mergeable and clean.

Exact-head focused CI run `36158180734` completed successfully on `2000633e6f4be5845c45317545364a4a47d15075`.

This handoff is a documentation-only successor commit, so its own exact-head CI must be observed before using the PR's final head as green evidence.

## Rung 9 invariant established

The existing `Material : Singular` owns optional authored receiving-surface response mathematics, conceptually `f_r(material, n, wi, wo, ...)`. Rung 9 does not introduce a parallel Material or BRDF ontology kind.

Receiver response remains an authority distinct from source `rho/chi/alpha`, Rung-8 visibility, and volume density/phase/transport. Material response edits do not become source edits, visibility edits, or volume-phase edits.

The receiver admission context owns `n` alongside the existing `wi` / `wo` directions. `t` remains deliberately refused because Rung 9 has not established an honest Material-owned temporal coordinate.

## Compatibility mapping

When `Material.responseExpr` is absent, the renderer retains the existing ambient/diffuse/specular/shininess Blinn-Phong behavior as compatibility state. Authored response is therefore additive in capability, not a silent reinterpretation of legacy materials.

When an authored response is present and admitted, the WebGPU SDF receiving-surface path consumes it. Unsupported authored response refuses rather than retaining stale output from a previously valid authored program.

## Evidence now present

Native and compiler/cache witnesses collectively establish:

- Material response persistence, independent response revision, PropertyPath/Law bridge behavior, and copy-on-write integrity.
- Receiver-specific admission and structural identity / numeric recollection.
- Numeric-only response edits reuse the compiled WGSL structure and refresh parameters; structural edits invalidate the relevant response program.
- Source-value edits and blocker-free visibility-state edits preserve receiver response structure and do not spuriously regenerate it.
- Unsupported authored response refuses without stale prior authored pixels.
- The exact absent-response legacy compatibility path remains present.
- Two otherwise-equivalent native SDF receivers under the same source and visibility, differing in their Material-owned authored response, produce different response-controlled pixels.
- Focused CI preserving the existing Rungs 3–8, Volumetric V1–V5, Sparkly/Antigravity reconciliation witnesses, and associated renderer proof machinery is green at the implementation/test head named above.

## Remaining limitations

Rung 9 is intentionally bounded to the established WebGPU receiving-surface / SDF boundary. It does not fabricate a Material Timeline merely to admit `t`, does not collapse response into source or visibility authority, and does not claim indirect transport.

Any broader backend parity should be treated as a separately evidenced boundary rather than retroactively widening this rung.

## Rung 10 readiness assessment

Rung 9 makes a Rung-10 constitutional investigation timely, but does **not** by itself authorize Rung-10 implementation.

Indirect transport still needs an explicit authority and propagation contract: what owns transported radiance after a receiver interaction; how path/iteration provenance is represented; how convergence or bounded propagation is defined; how cache/proof identity composes across bounces; and how energy/transport semantics relate to, without being conflated with, source emission, local Material response, Rung-8 visibility, or volume phase.

Therefore the evidence-based successor state is: **ready to specify/investigate Rung 10, not ready to smuggle Rung 10 behavior into existing Rung-9/source/visibility/volume channels.**

Do not automatically implement Rung 10 or Rung 11 from this handoff.
