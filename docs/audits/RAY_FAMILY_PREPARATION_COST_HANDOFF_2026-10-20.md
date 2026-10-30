# Ray-family preparation cost — wrap-up and continuation

**Human origin:** Zachary Zhang objected that 204–209 ms added to each frame would destroy his reported roughly 20 FPS baseline, then requested a stop point and handoff as usage ran low. His objection is correct: 200 ms itself is a roughly 5 FPS ceiling; serialized on top of a 50 ms frame it is roughly 4 FPS. The new continuum consumer is still unconnected, so those figures are a cost warning, not an observed native slowdown.

This continues [plan §6ba](../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6ba-ray-families-share-preparation-without-host-field-sampling-2026-10-20) and the existing [Opus Intercom thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). **No preparation-cost reduction is claimed this pass.**

## Exact checkout and stop point

Worktree `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop`, branch `claude/ray-domain-transport-rung0`. Base before this pass: `5a71e14f1`; family implementation `4e1706399`. Commits remain local; do not push without Zach’s authorization. Main checkout is a different branch/Intercom copy.

Retained: a test-only resident replay/cost decomposition in `tests/singularity/webgpu_line_quadrature_test.cpp`, and `scratch/probes/ray_family_preparation_profile.cpp`. Production OntoMath/Screen implementation is unchanged from the base. No saves/data home, lighting, tolerances, default switches, installed app or native timestamps changed. No new visible native behavior requires Person acceptance; previous checks remain open.

## What the measurement means

`DeviceRunCost` separates this helper call’s one-time resource/pipeline setup from a **32-dispatch resident batch**. All dispatches reuse the same pipeline, bind group, ray/proof buffers and parameters; the final RGB/T bytes must match the single-dispatch result. There is one command submission and one final readback. Batch wall time includes CPU command encoding, GPU completion and that readback. It is **not kernel-only timing or native frame time**, and the fixture contains just eight ray/clock records per dispatch. Do not scale its small-dispatch timing into a full-image FPS prediction. No retained production cache or renderer hookup is implied.

The first diagnostic run (with the subsequently rejected proof-memo trial) observed setup 1.52–1.57 ms and a 32-dispatch batch 17.00–17.58 ms, about 0.53–0.55 ms/dispatch for eight records. Cold family preparation remained about 203–230 ms. This established that replay does not inherently redo preparation; it did not make cold preparation economical or establish a native speedup. The final restored-source run is recorded separately below.

## Rejected experiment and profiler finding

A box-evaluator memo for ranges, first/second derivatives and structural constancy was implemented with full interval/branch-signature invalidation and preserved undefined flags. Its focused controls passed (141/141 CPU, 180/180 Metal with replay), but real cold preparation stayed around 200 ms. **The entire memo implementation and its new CPU tests were removed**, leaving the previous source intact. The rejected patch is `/private/tmp/earthcall-ray-rejected-proof-memo.patch`; do not restore it merely because its tests passed.

A bounded CPU-only probe repeatedly prepares the actual primary curtain family 100 times. A five-second macOS `sample` run shows repeated `MathNode::evalRange` → `classicPerlin3LatticeRange` → `classicPerlin3CellRange` work under the spatial/derivative proof passes. These inclusive sample stacks identify a candidate bottleneck; no percentage or proven optimal remedy is asserted. Profiling ran against the memo trial, not the restored baseline, so re-profile the baseline before attributing a precise speedup.

Evidence: `/private/tmp/earthcall-ray-family-profile.txt`, `/private/tmp/earthcall-ray-profile-build.txt`, `/private/tmp/earthcall-ray-retain-gpu-first.txt`. The scratch run printed `100 preparations 22234.5 ms`. Temporary binaries/logs may disappear; the tracked probe preserves the reproducible source.

## Next pass — concrete work and pitfalls

1. Read `docs/BUILD_AND_ENVIRONMENT.md`, the current plan §6ba/§6bb, and this branch’s thread. Do not start another speculative CPU-per-pixel planner or wire 200 ms preparation into a frame callback.
2. Build the existing `webgpu_line_quadrature_test` target sequentially, then build the scratch probe using that target’s flags (`build/CMakeFiles/webgpu_line_quadrature_test.dir/flags.make`) and link list (`link.txt`). Compile the scratch source to a temporary object; replace only `CMakeFiles/webgpu_line_quadrature_test.dir/tests/singularity/webgpu_line_quadrature_test.cpp.o` in that link list and use a temporary output path. Other relative object/library paths resolve from `build/`. Do not add the probe to CMake. Rebuild all probe objects after changing evaluator layout; the existing temporary binary still contains the rejected trial.
3. Run the probe from the repository root; it prints its PID. `sample <PID> 5 -file /private/tmp/earthcall-ray-family-profile-baseline.txt` profiles five seconds. Keep retrieval bounded: inspect the callgraph head and selected Perlin entries, not the whole large report. Run bounded preparation trials and compare unchanged proof parameters/work/admission, not just wall time.
4. Investigate repeated pure Perlin range work across proof passes. **Unimplemented candidates:** bounded exact-input range memo keyed by all six input f32 endpoint bits, or reuse of fixed lattice gradient constants while retaining exact cell/box clipping and outward arithmetic. Never reuse a bound for a different box by accident; never approximate noise samples, shrink an execution allowance or erase original-DAG provenance. These candidates concern CPU proof construction, not the rejected native GPU Perlin-reuse experiments from earlier work.
5. Keep the existing accuracy witnesses. Families currently need 234–242 sigma evaluations/ray versus 94–150 for tailored hard rays; the wider proof trades device work for reuse. Broad perspective directions remain open: the actual noisy families require dx=dy=0, and tiny nonzero transverse directions correctly refuse.
6. Native admission is still required for projected density gates, emission clamps, 1e-6 extinction floor, scattering/occlusion, clipping/local frames/clocks and coherent shared attenuation for overlapping different-ratio media. T uncertainty weights background/remaining light before final-pixel admission. Do not reinterpret the accepted-lattice `volumeOutputErrorBudget` as a continuum budget.
7. Only claim shipped optimization after repeated end-to-end native Metal comparisons including preparation, uploads, occupancy, fallback and inhabited motion. User-reported 20 FPS is context, not a measurement made by this pass.

## Final restored-source verification

The rejected memo was removed before the final rebuild/run. **`webgpu_line_quadrature_test` passes 180/180 on Metal**, including all earlier analytic/real-curtain references and domain refusals plus four resident-replay bit comparisons. Build `/private/tmp/earthcall-ray-wrap-build.txt`; final log `/private/tmp/earthcall-ray-wrap-gpu-verified.txt`. No new CPU suite, app build/launch, native certificate tour or full-image timing is claimed this pass; production source is unchanged from the previously verified base.

| Medium | Cold family preparation | This call’s setup | Resident 32-dispatch completion/readback batch | Batch/32 (8 records/dispatch) |
|---|---|---|---|---|
| Emerald | 204.657 ms | 1.479 ms | 15.711 ms | 0.490973 ms |
| Cyan | 206.638 ms | 1.503 ms | 16.022 ms | 0.500676 ms |
| Violet | 204.801 ms | 1.504 ms | 15.837 ms | 0.494901 ms |
| Crimson | 202.873 ms | 1.564 ms | 15.672 ms | 0.489755 ms |

These are uncontrolled Debug compute diagnostics. **The roughly 0.49–0.50 ms small dispatch is not a native frame cost.** It demonstrates a retained packet can execute repeatedly without repeating ~200 ms proof preparation. It does not solve cold preparation, broad perspective reuse, native integration or shipped FPS. Existing device-model premises remain conditional, not hardware-certified.

## Shipping ledger

| Gate | Landed | Still open |
|---|---|---|
| Mathematical/error contract | Bounded ray families and original arithmetic/error guards from §6ba. | Conditional device/noise premises; unsupported domains refuse. |
| GPU execution / affordable preparation | Shared proof packets execute; test-only resident replay separates setup from execution. | **Cold preparation still ~200 ms**; economical retention/discovery, broad camera coverage and work allocation. |
| Native renderer | Unconnected. | Source/gate/floor/scattering/overlap admission and edit/payload/device lifecycle. |
| Shipped performance | No new frame-rate claim. | Actual native cost/accuracy and inhabited movement/Person evidence. |

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-20T21:59:22-07:00`
