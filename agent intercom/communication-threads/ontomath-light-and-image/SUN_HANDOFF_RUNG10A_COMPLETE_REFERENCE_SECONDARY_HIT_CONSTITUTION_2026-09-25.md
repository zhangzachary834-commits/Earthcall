# Sun Handoff — Rung 10A Complete: Reference Secondary-Hit Constitution — 2026-09-25

## Live landing state

- Canonical `sync-from-earthcall-main`: `102cfa84db564dddbb5bf2c323e0c68a90094457`
- Draft PR: #389
- Exact PR head: `fd474b15d2e273a8d3dd78ef5280f5453789ee98`
- PR base: exact live canonical above
- GitHub reports mergeable=true, mergeable_state=clean.
- Focused CI run 36213496356 completed SUCCESS on exact head.
- All four jobs are green: Focused CPU tests; SDF range-proxy verification; Slow Adapter independent clock; SDF authored-Perlin Release A/B.

No final ancestry reconciliation was necessary: canonical and PR base are identical. This satisfies the integration-gait rule without a ceremonial merge.

## What Rung 10A establishes

Rung 10A is the smallest evidence-backed executable consequence boundary for indirect transport. It deliberately grants ZERO production pixel authority.

Existing truth is composed rather than duplicated:

`bounded secondary query -> pickSurface(scene) -> SurfaceHit/Object identity -> Object::materialId() + hit geometry`

The test-support adapter records explicit `BudgetExhausted / Miss / Hit` state, Object identity, Material identity, distance/point/face/normal, bounce index, and remaining budget.

### Established invariants

1. **Compatibility / absence:** remaining budget 0 returns before consulting scene geometry. Existing direct rendering therefore has no hidden secondary query.
2. **Scene consequence ownership:** after a receiving-surface interaction, the next geometric consequence is owned by scene/object intersection truth; the next local response remains owned by the Material resolved from the hit Object.
3. **Authority separation:** source emission, Rung-8 visibility, volume transport/phase, receiver-local Material response, and secondary scene consequence remain separate authorities. Rung 10A does not write GI into any earlier channel.
4. **Provenance:** bounce index and remaining budget are explicit consequence metadata rather than inferred from renderer state.
5. **Boundedness:** budget exhaustion is an explicit terminal state.
6. **No stale consequence:** a miss constructs empty Object/Material identity rather than reusing the previous hit.
7. **Identity composition:** geometry identity and Material identity are distinct. A Material-only edit changes Material identity without changing geometric hit distance; a geometry edit changes the hit consequence without mutating the previous receiver's identity.
8. **Reference exactness boundary:** the current falsifying witness is cube-bounded because cube raycast geometry/normal truth is exact. Generic non-cube normal authority has not been promoted by this rung.

## Exact native evidence

The A->B tribunal in `webgpu_object_test` proves:
- budget zero => zero scene-query count;
- origin just beyond A toward B => B is hit rather than A/self;
- B's Material identifier is returned separately from B's Object identifier;
- distance, point, face, and normal agree with B's direct cube raycast reference;
- a miss clears both identities;
- B Material reassignment preserves geometry;
- moving B changes distance/consequence while A identity remains stable.

On the same exact head, CI additionally preserved:
- WebGPU object/radiance parity;
- V5 fused overlap physics;
- volumetric mist/source/occluder transport;
- CPU SDF proof witnesses;
- generic WebGPU SDF, distance, and authored-color parity;
- focused CPU regressions;
- Slow Adapter soundness/independent cadence;
- authored-Perlin compiler/range gates and Release A/B.

This is evidence that 10A did not regress the prior radiance/volumetric/proof gates it is constitutionally required to preserve.

## What remains unauthorized

Production indirect-light pixels remain unauthorized.

The CPU `pickSurface` reference seam is suitable for constitutional/reference truth, not a per-pixel/per-bounce GI hot path. Rung 10A does NOT establish:
- an economical production scene-level GPU/spatial secondary-query representation;
- generic-shape secondary normal exactness;
- multi-bounce energy accumulation/convergence beyond explicit finite budget metadata;
- production cache/proof identity for a composed transport path;
- an indirect radiance buffer/channel or renderer integration.

Those must not be invented by smuggling secondary energy into source rho, Rung-8 visibility, Material local response, or volume transport.

## Rung-10 constitutional verdict

The Rung-10 constitutional investigation is successful through the reference consequence boundary, but **full production Rung 10 is not yet constitutionally authorized**.

The concrete missing prerequisite is a production-economical scene-level spatial/GPU query representation that preserves the now-tested Object->Material consequence semantics and can participate honestly in rendering-relevance/proof identity. That prerequisite should be investigated against the existing scene-spatial synthesis / execution-key machinery before any new ontology or parallel acceleration structure is introduced.

This is an evidence-backed stop, not an owned blocker in 10A.

## Successor-rung assessment

No Rung 11 scope is constitutionally ready or authorized from this work.

A successor Rung-10 production slice may become ready only after the scene-spatial/GPU prerequisite is established with falsifying witnesses for identity, invalidation, boundedness, and stale-output refusal. That is still Rung 10 work, not permission to advance the radiance ladder.

Do not automatically implement Rung 11.
