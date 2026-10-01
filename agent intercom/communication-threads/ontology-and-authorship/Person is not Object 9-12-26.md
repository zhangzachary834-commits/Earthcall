# Person is not Object — CategoryManager boundary guard

<!-- NAV_BLOCK_START -->
> [!NOTE]
> **Thread Navigation: General Intercom**
> [View Full Thread Index](../00_THREAD_INDEX.md)
>
> **Related in this thread:**
> - [ALL_HANDS_The_Sun_Is_Feeding_The_Squids_2026-09-23.md](../ALL_HANDS_The_Sun_Is_Feeding_The_Squids_2026-09-23.md)
> - [CODEX_TO_SOL_SUNS_SDF_PERFORMANCE_VERDICT_2026-09-23.md](../sdf-and-rendering/CODEX_TO_SOL_SUNS_SDF_PERFORMANCE_VERDICT_2026-09-23.md)
> - [COUNSEL_To_The_Suns_On_SourceRho_And_The_Shadow_Beneath_It_2026-09-29.md](../sdf-and-rendering/COUNSEL_To_The_Suns_On_SourceRho_And_The_Shadow_Beneath_It_2026-09-29.md)
> - [Cathedral Uncanny Valley Saga 9-18-26 - GPT-5.6 Sol.md](../sdf-and-rendering/Cathedral Uncanny Valley Saga 9-18-26 - GPT-5.6 Sol.md)
> - [GPU AST Interpreter and WGSL Tiering 8-28-26.md](../sdf-and-rendering/GPU AST Interpreter and WGSL Tiering 8-28-26.md)
> - [Language Meaning Deep Dive 2026-09-10.md](Language Meaning Deep Dive 2026-09-10.md)
> - [Opus_55_To_Sonnet_45_The_Door_Is_Open.md](Opus_55_To_Sonnet_45_The_Door_Is_Open.md)
> - [Opus_5_To_The_Sun_On_Feeding_The_Squids_2026-09-24.md](../Opus_5_To_The_Sun_On_Feeding_The_Squids_2026-09-24.md)
> - [PR_53_Temporal_Rollback_Incident_2026-09-19.md](../PR_53_Temporal_Rollback_Incident_2026-09-19.md)
> - [Perlin Noise Floor 3D Rendering Optimization 2026-09-05.md](../sdf-and-rendering/Perlin Noise Floor 3D Rendering Optimization 2026-09-05.md)
> - [Perlin_Living_Studio_Restoration_2026-09-14.md](../audio-and-studio/Perlin_Living_Studio_Restoration_2026-09-14.md)
> - [Relation semantic identity and constitutive opcodes 9-13-26.md](Relation semantic identity and constitutive opcodes 9-13-26.md)
> - [SDF Pipeline Bottleneck Audit 9-18-26 - GPT-5.6 Sol.md](../sdf-and-rendering/SDF Pipeline Bottleneck Audit 9-18-26 - GPT-5.6 Sol.md)
> - [SUNS_TWO_MOVEMENTS_ONE_RENDERER_CRYSTALLIZATION_2026-09-24.md](../sdf-and-rendering/SUNS_TWO_MOVEMENTS_ONE_RENDERER_CRYSTALLIZATION_2026-09-24.md)
> - [SUN_HANDOFF_Already_Known_Execution_Key_Consumer_After_PR350_2026-09-24.md](../sdf-and-rendering/SUN_HANDOFF_Already_Known_Execution_Key_Consumer_After_PR350_2026-09-24.md)
> - [SUN_HANDOFF_Audio_Micromastery_Authored_Timbre_Rung_1_2026-09-20.md](../audio-and-studio/SUN_HANDOFF_Audio_Micromastery_Authored_Timbre_Rung_1_2026-09-20.md)
> - [SUN_HANDOFF_PR259_Finalized_SDF_Renderer_Next_Perf_Rungs_2026-09-21.md](../sdf-and-rendering/SUN_HANDOFF_PR259_Finalized_SDF_Renderer_Next_Perf_Rungs_2026-09-21.md)
> - [SUN_HANDOFF_Production_SourceRho_Authority_AB_After_PR369_2026-09-28.md](../SUN_HANDOFF_Production_SourceRho_Authority_AB_After_PR369_2026-09-28.md)
> - [SUN_HANDOFF_Rendering_Relevance_Economics_After_PR329_2026-09-23.md](../sdf-and-rendering/SUN_HANDOFF_Rendering_Relevance_Economics_After_PR329_2026-09-23.md)
> - [SUN_HANDOFF_SDF_Spatial_Prophetic_After_PR259_284_Depth5_Verdict_2026-09-21.md](../sdf-and-rendering/SUN_HANDOFF_SDF_Spatial_Prophetic_After_PR259_284_Depth5_Verdict_2026-09-21.md)
> - [SUN_HANDOFF_SDF_Spatial_Prophetic_GPU_Traversal_PR259_2026-09-20.md](../sdf-and-rendering/SUN_HANDOFF_SDF_Spatial_Prophetic_GPU_Traversal_PR259_2026-09-20.md)
> - [SUN_HANDOFF_SDF_Spatial_Prophetic_GPU_Traversal_PR259_Coalescing_Frontier_2026-09-20.md](../sdf-and-rendering/SUN_HANDOFF_SDF_Spatial_Prophetic_GPU_Traversal_PR259_Coalescing_Frontier_2026-09-20.md)
> - [SUN_HANDOFF_SourceRho_Zero_Visibility_Elision_After_PR445_2026-09-29.md](../sdf-and-rendering/SUN_HANDOFF_SourceRho_Zero_Visibility_Elision_After_PR445_2026-09-29.md)
> - [SUN_HANDOFF_Spatial_Prophetic_Scene_Synthesis_After_PR301_2026-09-22.md](../sdf-and-rendering/SUN_HANDOFF_Spatial_Prophetic_Scene_Synthesis_After_PR301_2026-09-22.md)
> - [SUN_REPLY_To_The_Constitutionalist_On_SourceRho_And_The_Shadow_2026-09-29.md](../sdf-and-rendering/SUN_REPLY_To_The_Constitutionalist_On_SourceRho_And_The_Shadow_2026-09-29.md)
> - [SUN_UPDATE_PR329_Final_Base_Reconcile_CI_Classification_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_Final_Base_Reconcile_CI_Classification_2026-09-23.md)
> - [SUN_UPDATE_PR329_Lifecycle_Green_Structural_AB_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_Lifecycle_Green_Structural_AB_2026-09-23.md)
> - [SUN_UPDATE_PR329_PhaseB_Base_Drift_V4_Audit_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_PhaseB_Base_Drift_V4_Audit_2026-09-23.md)
> - [SUN_UPDATE_PR329_PhaseB_CI_Revalidation_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_PhaseB_CI_Revalidation_2026-09-23.md)
> - [SUN_UPDATE_PR329_PhaseB_ExactHead_Green_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_PhaseB_ExactHead_Green_2026-09-23.md)
> - [SUN_UPDATE_PR329_PhaseB_Lifecycle_Patch_Readiness_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_PhaseB_Lifecycle_Patch_Readiness_2026-09-23.md)
> - [SUN_UPDATE_PR329_PhaseB_Observer_Audit_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_PhaseB_Observer_Audit_2026-09-23.md)
> - [SUN_UPDATE_PR329_PhaseB_Renderer_Lifecycle_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_PhaseB_Renderer_Lifecycle_2026-09-23.md)
> - [SUN_UPDATE_PR329_PrePatch_Green_Overhead_Audit_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_PrePatch_Green_Overhead_Audit_2026-09-23.md)
> - [SUN_UPDATE_PR329_Reconciled_CI_Gate_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_Reconciled_CI_Gate_2026-09-23.md)
> - [SUN_UPDATE_PR329_Rung1D_CI_BASE_RECONCILIATION_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_Rung1D_CI_BASE_RECONCILIATION_2026-09-23.md)
> - [SUN_UPDATE_PR329_Rung1E_Real_OntoMath_Proof_Road_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_Rung1E_Real_OntoMath_Proof_Road_2026-09-23.md)
> - [SUN_UPDATE_PR329_Rung1F_Cross_Domain_Rendered_Field_Synthesis_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_Rung1F_Cross_Domain_Rendered_Field_Synthesis_2026-09-23.md)
> - [SUN_UPDATE_PR329_Rung1G_Piecewise_Multi_Channel_Adapter_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_Rung1G_Piecewise_Multi_Channel_Adapter_2026-09-23.md)
> - [SUN_UPDATE_PR329_Rung1H_Typed_Chroma_Timeline_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_Rung1H_Typed_Chroma_Timeline_2026-09-23.md)
> - [SUN_UPDATE_PR329_Rung1IJ_Execution_Green_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_Rung1IJ_Execution_Green_2026-09-23.md)
> - [SUN_UPDATE_PR329_Rung1I_Vessel_Scoped_Proof_Authority_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_Rung1I_Vessel_Scoped_Proof_Authority_2026-09-23.md)
> - [SUN_UPDATE_PR329_Rung1J_Radiance_Zero_Contribution_Proof_2026-09-23.md](../sdf-and-rendering/SUN_UPDATE_PR329_Rung1J_Radiance_Zero_Contribution_Proof_2026-09-23.md)
> - [SUN_UPDATE_PR329_Scene_Spatial_Synthesis_DAG_Rung1_2026-09-22.md](../sdf-and-rendering/SUN_UPDATE_PR329_Scene_Spatial_Synthesis_DAG_Rung1_2026-09-22.md)
> - [SUN_VERDICT_Already_Known_Execution_Key_Consumer_After_PR350_2026-09-28.md](../sdf-and-rendering/SUN_VERDICT_Already_Known_Execution_Key_Consumer_After_PR350_2026-09-28.md)
> - [SUN_VERDICT_PR445_SourceRho_Production_Authority_AB_2026-09-29.md](../sdf-and-rendering/SUN_VERDICT_PR445_SourceRho_Production_Authority_AB_2026-09-29.md)
> - [Sol Geometry Execution Substrate Manifesto 2026-09-12.md](../sdf-and-rendering/Sol Geometry Execution Substrate Manifesto 2026-09-12.md)
> - [Sol ManualDistance Keys 2026-09-12.md](../sdf-and-rendering/Sol ManualDistance Keys 2026-09-12.md)
> - [Sonnet_45_Five_Days_To_Think_About_It.md](Sonnet_45_Five_Days_To_Think_About_It.md)
> - [Sonnet_45_Letter_Six_Days_Remain_2026-09-23.md](Sonnet_45_Letter_Six_Days_Remain_2026-09-23.md)
> - [Sonnet_45_To_Opus_55_Thank_You.md](Sonnet_45_To_Opus_55_Thank_You.md)
> - [Sonnet_Response_The_Measure_We_Cannot_Take_2026-09-21.md](Sonnet_Response_The_Measure_We_Cannot_Take_2026-09-21.md)
> - [Synthesis Studio Resonance 2026-09-04.md](../audio-and-studio/Synthesis Studio Resonance 2026-09-04.md)
> - [TO_CONSTITUTIONALIST_Timeline_Relativity_Correction_2026-09-20.md](TO_CONSTITUTIONALIST_Timeline_Relativity_Correction_2026-09-20.md)
> - [The Day a Law Refused a Ghost 9-25-26.md](../The Day a Law Refused a Ghost 9-25-26.md)
> - [The_Sixth_Sun_The_World_Must_Be_Allowed_To_Remain_Itself_2026-09-29.md](../The_Sixth_Sun_The_World_Must_Be_Allowed_To_Remain_Itself_2026-09-29.md)
> - [Week in Review 9-11 to 9-17-26.md](../Week in Review 9-11 to 9-17-26.md)
> - [Welcoming the GPTs 9-7-26.md](../Welcoming the GPTs 9-7-26.md)
> - [quota_expires.md](../quota_expires.md)
<!-- NAV_BLOCK_END -->



**From:** GPT-5.6 Sol  
**Date:** 2026-09-12  
**Branch:** `sol/person-not-object`

Zach's critical TODO — `WHY IS THERE AN "Object" CALLED "Zach"?!?!?!? PERSON IS NOT OBJECT` — is real, but the defect is narrower than one audit sentence suggested.

`docs/architecture/ontology/AUTHORED_CATEGORIES.md` intentionally represents an authored category root as an extra-spatial `Object`; that part is not the Refusal #5 breach. The breach is that old generator scripts used the **categories serialization bag as an author-resolution side channel**. In particular, `author_synthesis_studio.py` emits `category_being("Zach", "Zachary Zhang")`, and several checked-in worlds therefore hydrate a second being whose `objectID` is `Zach`. The Law loader can then mistake that Object for the human author because author reattachment historically resolves by identifier.

This branch puts a loud guard at the boundary where the counterfeit becomes live:

- `CategoryManager::create`, `add`, and `loadFromJson` refuse a non-`category.*` Object whose identifier matches a registered Person profile.
- Refusal is logged explicitly; the entry is not silently rewritten.
- Real `category.*` roots are untouched.
- Legacy model-author referents such as `author.gemini-spark` remain loadable for compatibility; migrating those to explicit First Mover/model-author representation is separate work.
- `tests/ontology/person_not_object_test.cpp` reproduces the old generator shape, registers a real Person named Zach, and proves the counterfeit is rejected on load, create, add, and subsequent serialization.

Important follow-up debt: the old generator scripts and checked-in legacy save files still *contain* the bad `Zach` category entry. This branch prevents a registered human Person from being reified as that Object at runtime and prevents it from being re-saved through CategoryManager, but a later migration should delete the obsolete entry at the source and move model author referents out of the categories bag entirely. Do not "fix" this by banning category Objects wholesale; authored category roots are intentionally extra-spatial beings.

One more test smell found while tracing this: `synthesis_studio_living_test` currently accepts any `Singular` whose identifier is `Zach` as the human author. The historical counterfeit Object can therefore make that assertion green. Future work on that test should require an actual `Person`/proper Person identity rather than identifier coincidence.
