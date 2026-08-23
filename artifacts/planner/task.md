# Task: 3D Face Brush Restoration, Per-Zone FaceTexture Serialization & Full Serialization Audit

- [ ] **Phase 1: Due Diligence & Detailed Audit**
  - [x] Trace FaceBrush tool implementation across `Tool.cpp`, `CreationTools.cpp`, `Create3DConsole.cpp`, `PaintToolSurface.cpp`, `ObjectRender.cpp`.
  - [x] Identify FaceBrush bugs (0 opacity/flow, 0 spacing, missing faceTextures initialization on unpainted objects, ImGui mouse capture).
  - [x] Trace Per-Zone serialization across `Zone.cpp`, `ZoneManager.cpp`, `Serialization.cpp`, `SaveSystem.cpp`.
  - [x] Audit all missing entities/properties from `zone.json` / `home.json` beyond face textures.

- [ ] **Phase 2: Fix 3D Face Brush Tool Functionality**
  - [ ] Fix `Tool::FaceBrush` in `src/Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.cpp` (initFaceTextures on ownMaterial, valid opacity/flow/spacing, ImGui capture guard).
  - [ ] Enhance Face Brush UI controls in `src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/Create3DConsole.cpp` (opacity, flow, brush radius, softness, color picker).
  - [ ] Verify Face Brush painting logic with tests.

- [ ] **Phase 3: Per-Zone FaceTexture & Material Serialization & Logging**
  - [ ] Update `zoneToJson` in `src/Singularity/Storage/Serialization.cpp` to serialize `materials` for all objects in the zone.
  - [ ] Update `applyZoneJson` / `makeZoneFromJson` in `src/Singularity/Storage/Serialization.cpp` to restore `materials` on load.
  - [ ] Add explicit per-zone logging for face textures in `ZoneManager.cpp` during `persistZones()`, `hydrateFromZoneStore()`, `saveStateWithLog()`, and `loadState()`.
  - [ ] Ensure `zoneToJson` and `applyZoneJson` also serialize/deserialize `deletable` map and custom object attributes.

- [ ] **Phase 4: Serialization Audit Documentation**
  - [ ] Author comprehensive audit report in `docs/audits/ZONE_SERIALIZATION_AUDIT_2026-08-22.md` detailing every entity and property omitted from per-zone serialization.

- [ ] **Phase 5: Build, Test & Verification**
  - [ ] Add unit test verifying painted face textures survive per-zone identity persistence (`zone.json` / `home.json`).
  - [ ] Build and pass all tests (`paint_test`, `zone_identity_test`, `object_roundtrip_test`, `zone_home_ontology_test`, etc.).
  - [ ] Verify `earthcall` builds cleanly.
