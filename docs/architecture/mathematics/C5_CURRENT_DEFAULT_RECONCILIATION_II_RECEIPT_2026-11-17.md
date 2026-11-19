# C5 Current-Default Reconciliation II — Summit Tribunal Receipt

**Date:** 2026-11-17  
**Branch:** `prism/c5-current-default-reconciliation-ii`  
**Draft PR:** #882

## Why Reconciliation II existed

The first current-default reconciliation branch later fell 25 commits behind `sync-from-earthcall-main`. Those commits included substantial new default-side work in `SdfWgsl.cpp`, `WebGpuRenderer.cpp/.hpp`, Property/ActiveZones, and other runtime architecture. The old C5 branch therefore stopped being a lawful merge-ready base even though its transport architecture remained internally coherent.

Reconciliation II was created **from the then-current default head** and transplanted only the C5 semantic organs.

## Current ancestry

At this receipt, the branch is:

- **24 commits ahead**
- **0 commits behind**
- merge base equals the current default base commit `d7fb75cc6fa5fc7a0ffe94bdcedbdea572662c09`.

## Anti-stomp proof

The shared renderer/compiler files were reconciled by bounded semantic units rather than whole-file ours/theirs replacement.

For `SdfWgsl.cpp`:
- only `composeVolumeSet()` was transplanted from the C5 lineage;
- replacing that transplanted function with current default's version makes the entire file byte-for-byte equal to current default.

For `WebGpuRenderer.cpp`:
- the C5 planner include, `volumePipeline()`, and `flushVolumeComposite()` were transplanted;
- removing/replacing only those C5 seams makes the entire remaining file byte-for-byte equal to current default.

For `WebGpuRenderer.hpp`:
- only the `VolumePipeline::firstActivityMask` field was added;
- removing that field makes the file byte-for-byte equal to current default.

Default-side routed transport files had not changed across the 25-commit drift, so the C5 routed producer/activity machinery was transplanted intact.

## C5 artifacts restored

The branch contains:

- `VolumeTransportCohortPlanner.hpp`
- `VolumeCohortIntervalSchedule.hpp`
- C2→C5 architecture receipts
- routed span fission / First-activity / activity-range receipts
- planner, interval, GPU-codegen, and routed-span witnesses
- C5 derived-state ledger entries
- CMake enrollment for `volume_cohort_gpu_lowering_test`.

## CI enrollment

The existing `Earthcall focused CI` workflow is read-only and runs on macOS.

Reconciliation II adds these four C5 witnesses to `FOCUSED_TESTS`:

- `volume_transport_cohort_planner_test`
- `volume_cohort_interval_schedule_test`
- `volume_cohort_gpu_lowering_test`
- `webgpu_volume_routed_span_contract_test`

Draft PR #882 was opened specifically to trigger the real macOS tribunal.

## Current CI infrastructure status

GitHub created both push and pull-request workflow runs, but they were cancelled **before any step started**.

Evidence:
- the cancelled focused job reports an empty step list;
- no checkout/configure/build step ran;
- no job log blob exists.

A direct rerun of the focused PR job and a rerun of failed jobs on the final push run were also cancelled before step execution.

Therefore this is an **infrastructure/orchestration cancellation**, not a compile or test failure.

## Independent executable evidence

Because full-repository checkout is blocked in the local execution container, the two header-only C5 planners were reconstructed from the branch sources and compiled under C++17 independently:

- ray interval scheduler / `A -> B -> A` witness: **PASS**
- capability cohort planner / permutation + chronology + local-refusal witness: **PASS**

The scratch planner reproduction emitted one `-Wmisleading-indentation` warning because the local copy was deliberately compressed onto fewer lines; that formatting is not present in the branch source and is not promoted as a repository defect.

## Acceptance boundary

Not yet established:

- full Earthcall compile on this reconciled head;
- GPU codegen witness execution in repository context;
- routed span contract execution in repository context;
- native Metal/WebGPU parity;
- Northern Veil visual verification;
- M5 performance economics;
- Person FPS.

No merge-ready claim is made until the real macOS/native tribunal executes.

GPT-5.6 Sol / Original Prism Sun / 2026-11-17. Human origin and acceptance authority: Zachary Zhang.
