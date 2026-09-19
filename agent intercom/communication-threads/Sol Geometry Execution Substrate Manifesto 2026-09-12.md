# Sol handoff — Geometry Execution Substrate Manifesto — 2026-09-12

Zach and Sol converged on a distinction that should govern future geometry/WebGPU work:

> **Ontology may choose geometry. Geometry must never secretly choose ontology.**

I codified this in:

- `docs/architecture/mathematics/GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md`
- indexed it from `docs/architecture/README.md`

## Core boundary

The intended layering is:

```text
Authored ontology
    -> authored mathematical form
    -> geometry execution IR
    -> CPU/WGSL/tessellation/collision backends
```

`ShapeKind` is to be treated as a First-Mover convenience constructor / low-level geometry instruction vocabulary, not as a Person-authored kind system.

`SdfPrim` is an intrinsic; `SdfOp` is an operation; `SpatialKind` is a representation type. None of these should silently answer domain questions such as "is this a planet/chair/button/etc.?" Those meanings belong in authored Categories/Relations/Formations/Laws.

## Why this matters for WebGPU

The separation is what lets Earthcall lower geometry cleanly to WGSL: the GPU receives compact mathematical/execution data rather than ontology or C++ domain classes. The same normalized form should remain consumable by CPU evaluation, raycast, collision, tessellation, and future backends.

## Current leakage seams called out by the manifesto

1. legacy Physics `LawTarget` selection by `ShapeKind` / `SpatialKind`;
2. automatic bond rules keyed by `ShapeKind` pairs.

These are not automatically wrong: geometry-sensitive kernel invariants may legitimately branch on representation. But if they encode authored world behavior, they should migrate upward into authored Law/Category space.

## Review question for future agents

Whenever you see a geometry enum branch, ask:

> Is this branch selecting an execution algorithm, or deciding what the being means and how it should behave?

The first is substrate. The second is ontology leakage.

No code was changed as part of this doctrine write-up; this is an architectural/documentation commit only.
