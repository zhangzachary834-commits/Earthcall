# Face paint bridge writes the shared Material

**Opened:** 2026-10-16 by the [full-system audit](../../../../../audits/FULL_SYSTEM_AUDIT_2026-10-16.md#1-findings-most-severe-first) (F2). Claude Code / Claude Opus 5.5 / session `d6fe31dc-53d9-49c1-a4b7-87479d9d605b`.

`FacePropertyBridge::texture()` (`src/ConstructedBeing/Singular/Object/Object/ObjectProperties.cpp:528`) returns a mutable `FaceTexture*` from `materials.resolveOrDefault(_owner->materialId())`. Five `setValue` branches write through it: `activeLayer`, `useLayers`, `layerOpacity`, `blendMode`, and `resolution` when the face already has a texture. Every Object naming that Material changes. Only `color`, via `setFaceColor`, diverges correctly. The code has been this way since `10a17f03f` (2026-08-13).

**Witness W3:** two Objects share `material.audit-shared-stone`. Writing `face.0.resolution = 64` on one leaves both still naming the shared Material, and the other's face 0 is now 64 wide.

**Done when:**
- Each write branch diverges through `Object::ownMaterial()` first. Reads may keep resolving the shared Material.
- A suite test asserts that the second Object is untouched.

## Status 2026-10-16: done and verified

`ownTexture()` diverges via `ownMaterial()` after validation; `paint_test` §9 asserts that the second Object and the shared Material are untouched, and that a refused write does not fork.
