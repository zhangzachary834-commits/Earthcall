# Addendum: GPU Micro-Mastery and Authored Mathematical Frontiers

*This document links rendering economics to OntoMath design without equating performance mechanisms with mathematical correctness or claiming an unimplemented world is shipped.*

## A substrate for representation, not ontology

The [GPU Micro-Mastery Architecture](../architecture/Singularity/GPU_MICRO_MASTERY_ARCHITECTURE.md) describes reusable GPU buffers and batched/instanced rendering as ways to reduce per-object allocation and draw-call overhead. Such techniques can make a large Law-authored scene more practical, but they neither define what a `Singular` is nor authorize its behavior.

Performance claims need their actual measurement conditions. The cited headless micro-mastery test reports roughly **182 ms per frame** for 4,500 objects, which is not evidence of 60 FPS for arbitrary scenes. That source also documents a limitation: per-Object mesh instances do not automatically collapse into one instanced draw batch simply because their shapes are equivalent.

## Mathematical frontiers remain authored and bounded

The [Far Lands Framework](../architecture/mathematics/FAR_LANDS_FRAMEWORK.md) explicitly labels its layered-terrain example **design specified, not yet implemented**. It proposes `Piecewise` distance bands and a recursive `FunctionDef` registered through `FunctionRegistry`, so terrain deformation is deliberate authored mathematics rather than an accidental floating-point bug.

The proposal has a stated `kMaxCallDepth = 32` recursion ceiling. Consequently, its evocative “infinitely many layers” language describes extensible authored structure, **not** an implemented, unbounded recursion guarantee. GPU reuse and rendering optimizations may carry such a field to a screen efficiently; they cannot eliminate recursion bounds, numerical limitations, or shader compilation costs by themselves.

## Linked references

- [GPU Micro-Mastery Architecture](../architecture/Singularity/GPU_MICRO_MASTERY_ARCHITECTURE.md)
- [Far Lands Framework (design)](../architecture/mathematics/FAR_LANDS_FRAMEWORK.md)
