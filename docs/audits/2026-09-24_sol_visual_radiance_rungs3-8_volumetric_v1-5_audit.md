# Light acquires consequence — four-day Sol visual and radiance audit

- **Author:** Codex / GPT-6
- **Session:** 01a0cfbf-c751-7af0-b160-df07da055bc0
- **Date and time:** 2026-09-24 00:17 PDT
- **Merged checkout inspected:** sync-from-earthcall-main at d0ea104e
- **Live successor inspected:** [draft V5 PR #343](https://github.com/zhangzachary834-commits/Earthcall/pull/343) at 6e2d82aa; the implementation inspected at d6d27efd is followed by five documentation-only commits, and its reconciled implementation parent is 5c97489a
- **Scope:** Sol's Rungs 3–8 and Volumetric V1–V5 over September 20–24. Northern Veil is read as a Person/Gemini Spark authored save that consumes the substrate; this is not an audit of Gemini's audits.

## Zach's commission and what I could witness

Zach asked me to extend the SDF-performance review to the visual/radiance work. His prior direction was that the Person's authored mathematics should be visible without being redefined by the renderer. The Sol Suns' key contribution was to make *different meanings* inhabit the same mathematical substrate without collapsing into each other: source strength, source color, emission direction, path visibility, medium density, extinction, scattering, medium color, phase, and self-emission each acquired a separate place. My review of current source, PR diffs, the saved Northern Veil, and focused tests extends their work with concrete next witnesses. I did not obtain a live Earthcall aurora frame in this session; source and native-CI evidence are not a personal visual judgment.

## The lineage, with its exact maturity

| Work | What landed or remains open |
|---|---|
| [Radiance Rung 3, #266](https://github.com/zhangzachary834-commits/Earthcall/pull/266) | Live SDF rho witness; numeric edits refresh parameters, structural edits recompile, unsupported math refuses without stale output. Merged. |
| [Rung 4, #273](https://github.com/zhangzachary834-commits/Earthcall/pull/273) | Optional rho(p,t) reads an admitted relative Timeline coordinate; time movement is neither an authored parameter edit nor shader structure. Merged. |
| [Rung 5, #281](https://github.com/zhangzachary834-commits/Earthcall/pull/281) | Independent authored vec3 chi(p,t); legacy light color remains the exact default. Merged. |
| [Rung 6, #283](https://github.com/zhangzachary834-commits/Earthcall/pull/283) | Independent scalar alpha(p,omega,t), with omega defined as world-space source-to-receiver direction. Merged. |
| [Rung 7, #290](https://github.com/zhangzachary834-commits/Earthcall/pull/290) | Multiple Zone-owned FieldNodes compose sources above their individual ASTs; a native two-source witness checks additive red/blue contribution. Merged. |
| [Rung 8, #297](https://github.com/zhangzachary834-commits/Earthcall/pull/297) | Exact self-pipeline SDF visibility is derived transport and has a native selective-blocker witness. Merged, but currently opt-in and limited to geometry the executing SDF program owns. [Proof acceleration #315](https://github.com/zhangzachary834-commits/Earthcall/pull/315) remains draft. |
| [V0 landing, #320](https://github.com/zhangzachary834-commits/Earthcall/pull/320) | Density D becomes independent of source rho; depth-aware medium composition. Merged after Rung 8. |
| [V1, #328](https://github.com/zhangzachary834-commits/Earthcall/pull/328) and [V2, #332](https://github.com/zhangzachary834-commits/Earthcall/pull/332) | Authored extinction sigma_t, scattering sigma_s, and medium chroma C_v. Missing channels preserve the old 0.5D, D, and white compatibility terms. Merged. |
| [V3, #337](https://github.com/zhangzachary834-commits/Earthcall/pull/337) and [V4, #339](https://github.com/zhangzachary834-commits/Earthcall/pull/339) | Independent angular phase Phi and vec3 medium self-emission E_v. V4's native no-source/no-scattering witness reported black without E_v and luminous pixels with it; numeric edits and refusal were separately tested. Merged. |
| [V5, #343](https://github.com/zhangzachary834-commits/Earthcall/pull/343) | One fused integral for 2+ media, with shared extinction/source and one transmittance state; one medium keeps the V4 path. Its reconciled code tree passed all four focused macOS jobs in [run 35961351263](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/35961351263), including the explicitly wired native overlap-physics test. The PR was still open and draft at inspection. |

The arc matters more than its numbering. Rungs 3–7 make an authored light's strength, color, direction, time, and plurality visible. Rung 8 keeps occlusion out of the light's identity. V0–V4 let a medium exist, extinguish, scatter, tint, redirect, and emit on its own. V5 then refuses the accidental proposition that overlapping media should change physics when their draw order changes. Sol's closed-form native tribunal tests the fused answer against both sequential alpha orders, not merely A/B permutation. That is real progress toward a world that answers its author truthfully.

## Independent finding 1 — Rung 8 is a bounded witness, not yet scene-wide shadow truth

The merged [sourceVisibility implementation](../../src/Singularity/Screen/WebGPU/SdfWgsl.cpp#L1450) evaluates only the SDF compiled into the executing object pipeline; its own comment says it cannot query arbitrary differently structured Zone geometry. [Renderer visibility](../../src/Singularity/Screen/Renderer.hpp#L277) defaults off, and a search of production src found no caller enabling it. Native tests deliberately switch it on. Thus the merged Rung 8 proves a **local exact transport baseline under test**, while normal multi-object scenes retain V=1 on that path. Its 192-step exhaustion returns V=0 although “unresolved” has not proved “blocked”; the PR itself names that hardening seam. Preserve this candid scope when describing visible shadows. A scene-spatial geometry substrate and an explicit unresolved result are owed before promoting it to scene-wide authority.

## Independent finding 2 — V5's new integration law is sounder, but segment boundaries can change a medium's numerical answer

The V5 shader [sorts two AABB events per medium and gives each occupied event interval 96 midpoint samples](https://github.com/zhangzachary834-commits/Earthcall/blob/d6d27efddaeab2100409b6f84f1ffb2d94052552/src/Singularity/Screen/WebGPU/SdfWgsl.cpp#L3407). Empty *distant* gaps no longer dilute an existing medium's sample budget; the native far-zero-medium witness closes that earlier defect. But a zero-density medium B whose bounds lie **inside** varying medium A inserts two events into A's occupied interval. A alone receives one 96-point grid; A+B receives three separate 96-point grids, despite B contributing no extinction or source. The physical integral is identical, while the numerical quadrature need not be. In a small CPU midpoint surrogate of the shader's attenuation update, A with a narrow Gaussian emission band changed by about two linear 8-bit levels for one B placement; this is a **surrogate**, not a native pixel failure claim.

Add a native A-alone / A-plus-partially-overlapping-zero-B witness with a narrow spatially varying E_v, move B through A, and report pixel error and fragment work. The intended rule is not that all numerical approximations are bit-identical: it is that adding a semantically null participant should not visibly change an already-authored medium merely by repartitioning its sample grid. If it does, choose an error-controlled or stable local sampling policy while retaining one shared transport state.

## Independent finding 3 — Northern Veil is the real four-medium V5 proving ground

I parsed [Northern Veil's saved Zone](../../saves/zones/Northern%20Veil/zone.json), attributed there to Zachary Zhang and Gemini Spark. All four auroral FieldNodes carry D, sigma_t, sigma_s, C_v, Phi, and E_v. Every pair of their origin±scale AABBs overlaps. Their positive scales and density expressions route the Zone through V5's 2+-medium fused path when admitted. For four members the generated shader can walk up to seven event intervals and spend 96 samples per occupied interval—at most 672 sample iterations per fragment before early transmittance exit, each with four medium-bound checks. This is a source-derived ceiling, **not a measured frame cost** or a claim that every ray crosses seven occupied intervals.

V5 is expected to change this world's pixels where curtains overlap: replacing sequential whole-medium alpha with shared extinction is a correction, so old/new image equality would be the wrong gate. The missing witness is a controlled *explanation* of the change: capture the same saved camera and time before/after V5, show the overlap regions and visual hierarchy of emerald/cyan/violet/crimson, record frame time, shader compile time, and generated WGSL bytes, then cold-reload and recapture. The present synthetic two-medium tribunal establishes the law; it does not show whether this four-curtain authored world remains legible, beautiful, and responsive to a Person. I have not seen that frame myself.

## Independent finding 4 — growing media can fail without a named screen refusal

V5's [compileVolumeSet](https://github.com/zhangzachary834-commits/Earthcall/blob/d6d27efddaeab2100409b6f84f1ffb2d94052552/src/Singularity/Screen/WebGPU/SdfWgsl.cpp#L3283) emits per-member WGSL evaluators and checks, an array of 2M events, and insertion sort, with no admitted M ceiling in the inspected path. This makes shader source, compilation, and fragment work grow with world membership. A valid generated Program can still fail backend shader/pipeline creation. In [WebGpuRenderer's V5 branch](https://github.com/zhangzachary834-commits/Earthcall/blob/d6d27efddaeab2100409b6f84f1ffb2d94052552/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp#L2422), a null volumePipeline sets memo.ok=false but leaves the successful Program's empty error unchanged. The later refusal telemetry increments only when that error is nonempty. The set is omitted, yet the registered volume refusal can stay silent. The one-medium branch has a related inherited pattern; V5 raises its likelihood as shader size grows.

Keep the current fail-closed rendering behavior, but give backend creation failure a named refusal and test it through an injectable failure seam. Stress 4, 8, and more authored media with compiler/pipeline timing and memory; set an explicit supported bound or another scalable execution representation based on that evidence. Do not silently truncate an authored set to satisfy a GPU limit.

## Independent finding 5 — the authoring frame still serializes every admitted medium channel

Current [readVolumeDensity](../../src/Singularity/Screen/VolumeDensity.hpp#L64) runs Piecewise.toJson().dump() and hashes the result for D plus each present sigma_t, sigma_s, C_v, Phi, and E_v. [EngineRender](../../src/Singularity/Core/EngineRender.cpp#L136) walks the Zone's direct FieldNode index each frame and calls that reader. Northern Veil can therefore pay **24 channel serializations per frame** before V5's shader does any work. This is a source count, not a measured bottleneck. It was already called out in Sol's Rung-7 handoff and V5c plan, while the latest V5 handoff calls the feature complete once CI settles. Either measure and bound this CPU tax as part of V5 completion, or explicitly carry V5c into a separate performance task so “complete” does not erase it. A revision-driven replacement must cover in-place authored edits and retain a broad fail-open content check until every mutation path is witnessed.

## Cross-rung limit — plural sources and wi-reading phase do not yet compose

Rung 7 admits multiple source FieldNodes, but V3's wi-reading phase currently requires **exactly one enabled incident source**. The [V5 fused set](https://github.com/zhangzachary834-commits/Earthcall/blob/d6d27efddaeab2100409b6f84f1ffb2d94052552/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp#L2475) refuses the whole set when any member needs wi and the source count is zero or greater than one. Three Northern Veil phases contain wi, and its root currently provides one light source that is enabled by the reader's default. Adding a second enabled source could therefore make all four fused curtains disappear, including the independently self-emissive contribution, with a named runtime refusal. This is a truthful refusal under the current transport contract, yet an important Person-facing authoring cliff. It calls for a later per-source incident-scattering composition, not a rushed V5 scope expansion; meanwhile add a small native two-source/refusal witness and document the current limit where creators will encounter it. Current scattering-source terms also do not yet multiply full authored incident radiance/visibility; [the rendered-field roadmap](../plans/RENDERED_FIELD_SEMANTIC_SYNTHESIS_IMPLEMENTATION_PLAN_2026-09-22.md#L182) treats that as future transport.

## Direction to the Suns

1. Let V5's reconciled green implementation gate stand, update its stale plan/PR body, and keep the fused-overlap physics. Do not invent V6 to postpone review.
2. Before or immediately after landing, make backend pipeline refusal observable and run the null-overlap sampling witness. Keep the A/B permutation and closed-form numerical tribunals.
3. Use Northern Veil—not only a two-constant-medium synthetic scene—as the visual/performance witness for four overlapping, authored, self-emissive media. Preserve its save; compare captures and timings across V4/V5 code, not by rewriting its contents.
4. Carry the 24-serialization frame tax and growing generated shader cost into a measured scaling pass. Rung 8 scene-wide visibility and multi-source in-scattering deserve separate constitutions after this rung closes.

The insight I want to leave with the next Sun is that the visual splendor here is earned by **separate truth meeting in one frame**. A curtain can have density without being a lamp, emit without a spotlight, and overlap another curtain without draw order becoming physics. The remaining work is to make that truth durable when the scene becomes plural, the camera moves, or a Person adds one more authored being.

## Verification and limits

- Fresh local merged-checkout build and execution: authorable_light_contract_test, sdf_wgsl_parameter_refresh_test, and zone_spatial_field_roundtrip_test all passed on d0ea104e.
- I inspected live PR #343 source and CI metadata. Its reconciled implementation tree's focused CPU, SDF range-proxy/native overlap, Slow Adapter, and authored-Perlin Release jobs all report success in run 35961351263. I did not check out or execute V5 locally.
- No native Northern Veil frame or GPU timing was obtained here; the image/experience questions are explicitly open.
- No save, renderer code, or authored being was changed in this audit.

**Signed:** Codex / GPT-6 · session 01a0cfbf-c751-7af0-b160-df07da055bc0 · 2026-09-24 00:17 PDT
