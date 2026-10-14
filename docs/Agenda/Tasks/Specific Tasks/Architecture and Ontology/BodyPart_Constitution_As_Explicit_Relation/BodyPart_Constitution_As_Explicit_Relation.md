# BodyPart constitution as an explicit Relation

**Status:** PARTIAL (2026-10-13) — constitution edges, registration and gift-shop removal landed; offset-on-Relation, authority gate and picking seam open.  
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

## Pass 1 — 2026-10-13 (what landed)

Zach's request, after o3's seam audit: *"go verify if theres a gift shop in bodypart like a ton of domain methods defined there that shouldnt be there"* and *"WHY IS PRIMARY OBJECT PRIVATE?!?! NO BLACK BOX"*.

**Verdict on the gift shop: confirmed.** o3 was wrong that BodyPart still *inherits* Object, but right that it still *re-sold* Object: 8 Automation forwarders (`hasAutomations`, `sampleAutomations`, `setAutomationRest`, `advanceAutomations`, `clearAutomations`, `addAutomation`, `automationState` ×2), `setFaceColor`, `getPrimaryShape`, an always-`false` `hasCustomTextures()`, an empty `update()`, and `getRaycastTransform()` — a leftover *override* from the Object-inheritance era that has overridden nothing since. Further findings:

- `setColor` only wrote a private `color[3]`; the declared `updateColor()` was never defined, so the Character Console colour picker painted nothing until a shape change.
- `_primaryObject`, `color`, `_dimensions`, `partType`, `_localTransform`, `_subObjectLocalOffsets` and Zach's `isLiteral`/`isSymbolic` (2025-08-11) were not registered. BodyPart had no `buildProperties` override, so none of this was readable by any law (Refusal 6).

**Changed** (`src/Person/Body/BodyPart/BodyPart.{hpp,cpp}`):

- The forwarders are removed. Callers now reach the manifestation directly with `getPrimaryObject()->…` (`LocomotionChannel.cpp`, `BodySerialization.cpp`, `CharacterConsole.cpp`, tests). `setPrimaryShape` stays: it changes the shape and repaints the face set, so it is a compound act, not a forwarder.
- **`constituted-by` Relations.** These are directed, with `a` = the part and `b` = the Object, and weight 1.0. They live in the part's own `relations()`.
  - The primary Object's edge is formed with `retain()` at birth, because that is custody, not an interaction, so no `relation-formed` echo fires.
  - A sub-Object's edge is formed with `add()` in `addSubObject`. That covers the Law path in `ActionModel.cpp`, which creates into a body part, and it publishes `relation-formed`.
  - `removeSubObject` dissolves the edge.
  - `addRelation()` is deliberately not used: its topology integration would try to admit the part as its own member.
- **Visible to Law:** `EngineInit.cpp` adds the Person's constitution edges to Universe's relation provider, its `relationsInvolving` index and its generation stamp (equality-mixed, so a cached traversal cannot go deaf). `Related(constituted-by, …)` can now walk from a clicked Object back to its BodyPart.
- **Registered paths:**
  - Writable: `isLiteral`, `isSymbolic`, `dimensions` (re-measures collision), `color` (paints through `Object::setFaceColor`), `localTransform`.
  - Read-only, with the reason stated in a comment: `partName`, `partType` (identity / concrete C++ member), `transform`, `scaledTransform` (derived per frame), `primaryObject`, `constitutedBy` (derived from the graph), `subObjectLocalOffsets` (writes still go through `setSubObjectLocalOffset`).
- `Formation::buildProperties` is now `protected`, so a Formation kind can extend the vocabulary instead of replacing it.
- Witnesses:
  - `tests/person/body_part_test.cpp` `testConstitutionIsExplicitRelation`: the edge exists, the index traversal finds it, and replacing a manifestation keeps the part's identity, dissolves the old edge and forms the new one.
  - `no_black_box_test` now audits `BodyPart`.

**Answer to "why is primary object private":** C++ `private` was never the black box; the absence of a registered path and a graph edge was. The `unique_ptr` stays as machine custody (it decides only lifetime). Meaning now lives in the Relation and in `primaryObject`/`constitutedBy`.

## Still open (next rungs)

1. **Local offset belongs on the edge.** `_subObjectLocalOffsets` is a predicate of the constitution, not of either being. Move it onto the Relation (`AttachmentData::localOffset` already exists), and only after confirming that Physics' attachment application ignores `constituted-by`.
2. **Authority does not flow through manifestation.** Body kernel guards and TransferPolicy must refuse a write to a constituting Object that would move or reshape a Person's body without consent. Today, writing the Object directly is not gated as a body write.
3. **Picking seam.** Object face-raycast uses the primary Object's *unscaled* transform. The part is drawn and collided at `scaledTransform()`, so clicks on body parts may hit a unit cube rather than the visible limb. The candidate fix is to let the primary Object carry the scaled transform and drop the extra `pushModel(scale)` in `draw()`. That changes picking, so it needs Person verification.
4. Constitution edges are not saved; they are rebuilt from the part's custody each load. That is honest for born-with-the-body parts. Law-added sub-Objects persist through `BodySerialization`'s sub-object list, which re-forms their edges via `addSubObject`.
5. Body → Person and BodyPart → Body membership are still not explicit Relations.

*Claude Code / Claude Opus 5.5 / session `6ba58694-54b8-4b8f-9788-9a6efec3c7df` / 2026-10-13T18:20-07:00 (PDT)*
