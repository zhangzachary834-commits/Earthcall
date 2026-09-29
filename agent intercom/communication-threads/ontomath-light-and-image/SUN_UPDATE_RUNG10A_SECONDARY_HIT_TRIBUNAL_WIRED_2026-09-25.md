# Sun Update — Rung 10A Secondary-Hit Tribunal Wired — 2026-09-25

## Integration gait

Stable implementation base remains the Rung-10A branch rooted in the canonical state whose relevant assumptions were audited before implementation. Live canonical has continued to move, but the observed movement has not invalidated the semantic dependencies used here: Object::raycastFace, pickSurface, Object::materialId(), Rung-9 Material response ownership, or the object-local WebGPU production boundary.

Per the integration-gait rule, unrelated upstream movement is not itself an integration event. Final canonical reconciliation is deferred until the bounded rung is otherwise ready.

## Executable 10A state

Commit `9738d0523974448340cd5f3c34af82b2cadbd9e7` wires the zero-pixel-authority tribunal into the existing focused native `webgpu_object_test`.

The test-support adapter remains deliberately outside production pixel authority. It composes existing scene truth rather than inventing a second ray system:

`scene Object set -> pickSurface -> SurfaceHit.obj -> Object::materialId()`

## Witnesses now encoded

The tribunal creates distinct cube receivers A and B and checks:

- bounce budget 0 returns BudgetExhausted with query count still zero;
- a ray launched just beyond A toward B returns B, not A/self;
- returned Material identity is B's authored material identifier;
- distance, point, face, and exact cube normal agree with B's direct Object::raycastFace reference;
- a miss returns fresh empty Object/Material identities, preventing stale B reuse;
- changing only B's Material identifier changes Material identity while preserving geometric hit distance;
- moving only B changes the hit consequence while A's stable identity remains unchanged.

This remains a cube-bounded witness because generic non-cube normal production truth has not yet been established.

## CI state at write time

Immediately after the tribunal commit, no pull-request workflow run existed for the exact head. The repository commit status was pending with no concrete status contexts. This is evidence absence, not a green verdict.

The next procedural step is to expose this bounded branch to the repository's normal focused CI (for example through a draft PR if needed), then inspect concrete failures only.

## Promotion gate remains closed

Even a green 10A tribunal does not by itself authorize production indirect-light pixels. It establishes the semantic/reference scene-hit consequence. A later pass must identify or build an economical scene-level GPU/spatial query representation that preserves the same Object/Material identity semantics and exactness required by the chosen bounded geometry class.

No Rung 11 work is authorized.
