# BodyPart constitution as an explicit Relation

**Status:** open follow-up — Zach author decision recorded 2026-10-13.  
**Origin:** o3's Body/Object seam audit in `docs/Analysis/CLOCK_POINTER_BODY_SEAMS_ANALYSIS_2026-10-13.md`, corrected against current source.

## Correction to the audit

The live source already has:

```cpp
class BodyPart : public Formation
```

so the earlier claim that BodyPart still *inherits* Object is stale. The August task `Stop_BodyPart_inheriting_Object` was real and remains completed.

The remaining seam is subtler: `BodyPart` still privately owns its visual constitution through `std::unique_ptr<Object> _primaryObject` and `_subObjects`. That is composition in C++, but the ontology cannot necessarily see **why** those Objects belong to that BodyPart.

## Author decision

A BodyPart is a Formation. Its 2D/3D Object manifestations are distinct beings connected by an explicit first-class relation:

```text
BodyPart  ←→  constituted-by  ←→  2D/3D Object(s)
```

The Objects may carry geometry, rendering, collision, material, painting, and other manifestation machinery. They are not therefore identical to the BodyPart.

The governing distinction is:

**constitution does not imply identity, and shared manifestation does not imply shared being.**

## Acceptance

- Preserve `BodyPart : Formation`; do not regress to Object inheritance.
- Make BodyPart ↔ constituting Object membership legible through explicit `constituted-by` Relation(s), rather than only private ownership vectors.
- Decide whether the current unique_ptr storage remains as machine custody beneath the Relation graph or should itself be replaced; ontology must not depend on invisible pointer ownership.
- Ensure Laws/Metalaws can traverse from a visual Object to the BodyPart/Person it constitutes.
- Ensure generic Object affordances do not silently imply unrestricted authority over the BodyPart/Person merely because the manifestation uses Object machinery.
- Add a witness that reconstitution/replacement of a visual Object does not change the BodyPart's identity.
