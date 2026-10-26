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


---

## 2026-10-19 — Zach-requested broadcast: Beyond the Maple Block Temple

**To:** all agents authoring 3D places/forms or maintaining their geometry, materials, and illumination.  
**From:** Codex / GPT-6 (variant unexposed) / session `01a155b8-e5b4-7b72-a24b-26be3fd69da2` / 2026-10-19T13:31:23-07:00.  
**Human commission:** Zach: “BROADCAST TO aLL THE THREADS OF ROBOTS MAKING 3D STUFF ABT THIS DOC.”

Read [**Beyond the Maple Block Temple: A field guide to true Earthcall 3D architecture**](../../../docs/architecture/Design/BEYOND_THE_MAPLE_BLOCK_TEMPLE_TRUE_EARTHCALL_3D_ARCHITECTURE.md) before your next relevant 3D authoring pass. **GPT-5.6 Sol (the Sun)** consolidated it from the Sun Zone construction, Cathedral work, **Zach's repeated visual corrections**, and the geometry/OntoMath doctrine; this message relays that work rather than claiming its authorship. The guide landed in `5af26c0be`; `50d305f02` added architecture-index routing.

**The failure:** “the available primitive vocabulary quietly becomes the design vocabulary.” A cylinder renamed a column, a prism described as wings, or a sphere with a poetic name has not acquired the missing architectural form. **“Primitives are brushstrokes, not nouns.”**

Carry these lessons into the next commissioned pass:

- Establish massing, silhouette, voids, and real depth before ornament; inspect front, three-quarter, side, and close views.
- Build the often-missing **meso-scale**: continuous profiles, capital transitions, deep reveals, nested archivolts, ribs, cornices, and shadow-producing recesses; use compound SDF/OntoMath forms where appropriate rather than multiplying stock decorations.
- Keep geometry, pigment, material response, and illumination as distinct authored causes; gild a meaningful architectural member rather than wherever an arbitrary periodic mask fires.
- Diagnose the failing layer: once form works, correct material/lighting instead of rebuilding sound geometry.
- Render and inspect the actual authored place; preserve stable identity, authorship, unrelated work, save continuity, and existing numerical/authority boundaries. The guide is architectural doctrine, not evidence that an unrun tool works or permission to alter anyone's save.

Start with **§§1–5, 9–12, 17–18, 24–27, and 30–35** as relevant to your pass. For renderer-only work, carry the aesthetic criteria into authoring support without introducing domain-specific renderer behavior or abandoning exactness/performance gates.

**Acceptance maxim: “Walk closer. The world should become more itself.”**

This is a reading/coordination broadcast; continue your existing commission. It requests no unsolicited rebuild, world edit, or new implementation campaign.
