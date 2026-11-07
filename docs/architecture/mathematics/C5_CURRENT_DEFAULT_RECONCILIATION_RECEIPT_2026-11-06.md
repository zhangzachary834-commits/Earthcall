# C5 Current-Default Reconciliation Receipt

**Date:** 2026-11-06  
**Reconciled branch:** `prism/c5-current-default-reconciliation`  
**Source transport lineage:** `prism/general-volume-cohort-architecture-reconciled`  
**Current-default starting point:** `sync-from-earthcall-main` at reconciliation time

## Why this branch exists

The C2→C5b transport lineage and current default advanced independently after their merge base. Current default gained substantial Cathedral/surface-SDF compiler work, batching changes, Property Memory work, Material changes, and 1D ray-domain surface laws. The Prism branch gained the general volume cohort architecture, query-relative chronology, interval scheduling, routed span fission, First-family activity certificates, and camera-relative activity quantization.

A normal file-level merge would conflate two different compiler fronts that happen to live in `SdfWgsl.cpp` and `WebGpuRenderer.cpp`.

This branch reconciles by **semantic ownership**, not by choosing "ours" or "theirs."

## Reconciliation law

[
	ext{Current Default Surface/SDF Evolution}
;oplus;
	ext{Prism Volume/C5 Evolution}
ightarrow
	ext{One Renderer}
]

Current default remains authoritative everywhere outside the C5 volume seams.

Prism contributes only the semantic units it legitimately owns.

## Byte-level proof of separation

Before transplant:

- current-default `WebGpuRenderer::flushVolumeComposite()` was byte-for-byte equal to the old merge-base implementation;
- current-default `WebGpuRenderer::volumePipeline()` was byte-for-byte equal to the old merge-base implementation;
- current-default `sdfwgsl::composeVolumeSet()` was byte-for-byte equal to the old merge-base implementation;
- current default had not modified `VolumeRoutedTransport.hpp`, `VolumeRoutedTransport.inc`, or `VolumeRoutedKernels.hpp`.

Therefore those units could be reconciled without choosing between competing implementations of the same law.

After transplant, replacing the C5 semantic units in the reconciled files with the current-default versions makes:

- the remainder of `SdfWgsl.cpp` byte-for-byte current default;
- the remainder of `WebGpuRenderer.cpp` byte-for-byte current default.

This is the primary anti-mitosis witness.

## Transplanted semantic units

### Whole-file, because current default never touched them

- `VolumeRoutedTransport.hpp`
- `VolumeRoutedTransport.inc`
- `VolumeRoutedKernels.hpp`
- `VolumeCohortIntervalSchedule.hpp`
- `VolumeTransportCohortPlanner.hpp`
- C5 planner / interval / GPU-lowering / routed-span tests
- C5 architecture receipts

### Whole-function, because current default left the functions unchanged

- `sdfwgsl::composeVolumeSet()`
- `WebGpuRenderer::volumePipeline()`
- `WebGpuRenderer::flushVolumeComposite()`

### Tiny grafts into current-default-owned files

- `sdfwgsl::VolumeExecutionFamily`
- `Program::kernelVolumeUsesFirstActivityMask`
- the extended `composeVolumeSet` declaration
- `VolumePipeline::firstActivityMask`
- the C5 planner include
- CMake enrollment for the CPU codegen witness

## Mutation deliberately rejected

The old Prism branch contained an accidental activity-mask graft inside `kernelVolumeSingleMarcher()` that referenced `useFirstActivityMask` and `out.wgsl` outside that function's valid scope.

That code was **not transplanted**.

On the reconciled branch, the entire single-medium marcher region remains current-default code and contains neither `useFirstActivityMask` nor `out.wgsl`.

This is a repair, not a historical rewrite: the malformed branch state is preserved in ancestry, while the reconciled continuation records why it was not inherited.

## What current default keeps

The reconciliation intentionally preserves current default's later work, including:

- Cathedral/surface-SDF 1D ray-domain compilation;
- new SDF primitive and CSG lowering work;
- current mesh/SDF batching and upload structures;
- cached authored model transforms;
- Property Memory / persistence evolution;
- current Material and render-material changes;
- current default tests and documentation outside the C5 lineage.

## What Prism keeps

The reconciliation retains:

- capability-derived volume cohort membership;
- query-relative whole-screen chronology;
- tile-local refinement;
- region-local opposite chronology;
- same-ray interval scheduling IR;
- fused First/Second/Joint execution-family state;
- routed span producer fission;
- conservative First-family activity certificates;
- shared camera-relative activity quantization;
- fail-open refusal semantics;
- one continuing transport-state doctrine.

## Acceptance boundary

This receipt proves a source-level semantic reconciliation and byte-level non-C5 preservation.

It does **not** claim:

- a successful native compile;
- Metal/WebGPU parity;
- CI success;
- M5 performance;
- Northern Veil visual parity;
- Person FPS.

Those remain execution witnesses.

## Constitutional result

The reconciliation does not create a second canonical renderer lineage.

It creates one continuation whose rule is:

> **Preserve the newer world's unrelated truths. Transplant only the older branch's still-lawful distinction. Reject malformed ancestry. Never let file conflict become ontology.**

GPT-5.6 Sol / Original Prism Sun / 2026-11-06. Human origin and acceptance authority: Zachary Zhang.
