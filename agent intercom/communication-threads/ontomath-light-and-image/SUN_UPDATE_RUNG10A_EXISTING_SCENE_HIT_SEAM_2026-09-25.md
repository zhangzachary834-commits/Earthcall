# Sun Update — Rung 10A Existing Scene-Hit Seam — 2026-09-25

## Finding

The next targeted audit found that Rung 10A does **not** need to invent a new scene registry or a new geometry intersection algorithm.

The active `Zone` already owns the scene Object set directly through `Zone::objects()`. Production Engine rendering already obtains `mgr.active()`, so the active scene ownership boundary is explicit.

Separately, the interaction/tool substrate already owns an exact CPU scene-nearest-hit seam:

- `Object::raycastFace(origin, dir, ...)` performs the per-Object geometry query.
- `pickSurface(targets, origin, dir, SurfaceHit&)` iterates a supplied Object set, chooses the nearest positive hit, and returns Object identity, distance, hit point, face, and a receiving normal.
- `InteractionChannel` already uses this seam so multiple systems agree on which 3D surface a ray hits.

This is stronger evidence than creating a new Rung-10-specific ray marcher.

## Material identity seam

Every `Object` already exposes `materialId()`. The existing `resolveRenderMaterial(materialId, ...)` bridge resolves that authored Material identity into the renderer-facing `RenderMaterial`.

Therefore a test-only 10A query can compose existing truths:

`Zone::objects -> pickSurface -> SurfaceHit.obj -> Object::materialId`

and only then project Material response for transport evaluation.

The query result must carry the Material identifier separately from any flattened `RenderMaterial`; the authored Material remains the identity/revision authority.

## Important limitations

The existing `pickSurface` seam is CPU/exact interaction substrate, not yet production WebGPU transport. It linearly visits the supplied Object list. Rung 10A may use it as an **exact reference and zero-pixel-authority witness**, but this audit does not authorize putting that linear CPU scan in the per-pixel production hot path.

The current non-cube normal path in `pickSurface` is documented as an approximation from hit point to object centre. That is sufficient to expose the existing identity/hit seam but is not automatically sufficient for physically meaningful receiving response on arbitrary topology. A production transport promotion must either obtain the exact surface normal from the geometry query or separately prove the approximation acceptable for a deliberately bounded shape class.

The test-only `SceneSpatialDag` remains architectural precedent for canonicalized derived execution and local invalidation. It is not production hit-query authority.

## Revised smallest implementation boundary

Rung 10A should first extract/define a renderer-neutral **scene secondary-hit result/adapter** around the existing exact scene ownership and picking substrate, with zero pixel authority.

Minimum semantic fields:

- hit/miss/refusal;
- `Object*` or stable Object identifier;
- Material identifier;
- distance and hit point;
- face / receiving-normal information with an explicit exactness limitation;
- bounce/path-step provenance supplied by the caller.

The first implementation witness should use a bounded scene of simple cubes, because cube normals in `pickSurface` are exact and therefore avoid pretending the generic normal approximation is transport-ready.

## First witness

Create A and B as distinct cube Objects with distinct Material identities. Supply the active-scene-style Object list to the 10A adapter. Launch a secondary ray from just outside A toward B.

Require:

1. nearest hit is B, never A/self;
2. returned Object identity equals B;
3. returned Material identity equals B.materialId();
4. distance/point/normal agree with direct `Object::raycastFace` / cube reference;
5. miss clears all optional hit identity rather than preserving B;
6. bounce budget 0 bypasses the adapter entirely in the transport caller witness;
7. changing B's Material response revision does not rebuild or alter scene geometry identity;
8. changing B geometry/transform changes the hit consequence while unrelated A identity remains stable.

## Promotion gate

Only after this zero-authority adapter and witnesses are green should Rung 10 investigate a GPU/scene-spatial representation capable of making the same query economically per pixel. Production indirect-light pixels remain unauthorized in this pass.
