# C5b-2a — Fused GPU Execution-Family Bridge Receipt

**Date:** 2026-11-05  
**Branch:** `prism/general-volume-cohort-architecture-reconciled`

## What landed

C5b-1 established a ray-lifetime interval schedule capable of representing `A -> B -> A`. C5b-2a now gives the production WebGPU fused V5 marcher a physical, stateful bridge between the two derived capability families without inventing a second ray state or changing the accepted V5 sample lattice.

### Compiler lane

`sdfwgsl::VolumeExecutionFamily` is a kernel-derived structural tag:

- `First`
- `Second`
- `Ungrouped` only means no grouped lane; it is refused inside a partially tagged grouped set.

`composeVolumeSet(..., executionFamilies)` requires a complete First/Second tag for every member when the lane is present.

The tags are execution/compiler state, not authored medium kinds, not Zone semantics, and not persistent near/far identity.

### Existing V5 chronology remains authoritative

The fused marcher already partitions each ray at every authored medium AABB entry/exit, sorts those events, and advances one shared:

[
(L,T)
]

state through the resulting occupied segments.

C5b-2a does **not** add support-proof edges to that event list and does **not** re-space quadrature. This avoids changing the accepted V5 sampling reference merely because an optimization proof became available.

On each existing event segment the shader derives:

`ecC5bExecutionMask`

from which tagged families occupy the segment midpoint:

- bit 0: First family may participate;
- bit 1: Second family may participate;
- both bits: local joint segment.

Because AABB entry/exit events delimit every membership transition, family occupancy is constant across that segment except at its already-authored boundaries.

Member work is then gated by the segment family bit before the existing per-member AABB/support/proof path.

## Physical meaning

A world arranged so one ray encounters:

[
First ightarrow Second ightarrow First
]

can now execute inside one fused GPU fragment invocation as three existing V5 box-event segments while preserving one transmittance/radiance state.

Overlap remains local:

[
First ightarrow Joint ightarrow Second
]

when both family bits are present on an event segment.

This is a real GPU execution bridge, not framebuffer compositing of independent full-ray answers.

## Renderer wiring

`WebGpuRenderer::flushVolumeComposite()` derives the family tags from the C2 capability partition while preserving the original `activeMedia` order.

Mixed joint programs receive the tag vector. All-routed and all-general programs remain untagged.

The tag vector participates in the fused program's content/structure identity. A regroup therefore invalidates/recompiles the correct WGSL lane rather than reusing stale family structure.

A tagged grouped program is explicitly excluded from `KernelRoutedTransport` full-ray admission. C5b-2a's state carrier is the fused V5 program.

## Witness

`tests/singularity/volume_cohort_gpu_lowering_test.cpp` is enrolled in the existing CPU-only SdfWgsl codegen lane in `CMakeLists.txt`.

It pins:

1. ungrouped V5 emits no C5b family mask;
2. grouped V5 emits First and Second bits;
3. the generated program contains exactly one transmittance state and one integrated-radiance state;
4. grouped and ungrouped two-medium programs retain the same four authored AABB events;
5. a three-member `First, Second, First` program emits two First assignments, one Second assignment, six authored AABB events, and still one transport state;
6. reversing family assignment changes WGSL structure;
7. incomplete tag count refuses;
8. `Ungrouped` inside a grouped set refuses.

## What C5b-2a does not claim

It does not yet replace First-only segment work with `KernelRoutedTransport`'s specialized classifier/root/subpanel producer.

Today the family mask lets one fused stateful consumer know which capability family owns an existing V5 segment and skip members from absent families. The actual member evaluators in that consumer remain the general fused V5 evaluators.

Therefore the next optimization rung is **C5b-2b**:

- lower First-only intervals into routed-compatible execution quanta;
- preserve exactly the same continuing (L,T) state;
- keep Second-only runs on general OntoMath;
- keep both-family runs joint;
- refuse locally if routed interval production cannot be proven;
- never duplicate a whole ray just to specialize one subinterval.

That is where the routed producer becomes a backend implementation of a RayTransportIR interval rather than a separate full-ray renderer.

## Evidence boundary

Source changes and codegen witnesses are committed and source-audited. No GitHub CI run or native M5/Metal execution result is inferred by this receipt.

No FPS claim is attached.

GPT-5.6 Sol / Original Prism Sun / 2026-11-05. Human origin and acceptance authority: Zachary Zhang.
