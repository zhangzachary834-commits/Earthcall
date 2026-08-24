# Zone Serialization & 3D Face Brush Architectural Audit
**Date**: August 23, 2026  
**Auditors**: Gemini Spark & Claude Opus 5  
**Target Subsystems**:
1. 3D Face Brush Tool & Creator Console Integration (`Tool.cpp`, `CreationTools.cpp`, `Create3DConsole.cpp`)
2. Per-Zone Identity Persistence (`Zone.cpp`, `ZoneManager.cpp`, `Serialization.cpp`, `SaveSystem.cpp`)

---

## 1. Executive Summary

An audit of the 3D Face Brush tool and Per-Zone Identity serialization was conducted:
1. **3D Face Brush Restoration**: The 3D Face Brush tool was present in the codebase but dysfunctional due to:
   - Passing `0.0f` opacity and `0.0f` flow in `Tool::FaceBrush`, causing strokes to draw with zero opacity ($t = 0$).
   - Passing `0.0f` spacing to `paintStroke`, causing division by zero ($N / 0$) on continuous strokes.
   - Missing `mat->initFaceTextures(faces)` on newly diverged materials, causing unpainted objects to reject brush strokes immediately.
   - Missing `ImGui::GetIO().WantCaptureMouse` guards, allowing mouse clicks on UI windows to accidentally paint on the background 3D scene.
   - These issues have been resolved, and full controls (Radius, Softness, Opacity, Flow, Brush Color) are wired to the Creator Console.

2. **Per-Zone FaceTexture Serialization**:
   - `zoneToJson()` previously omitted the `materials` dictionary and face textures, meaning when zones were saved to `saves/zones/<id>/zone.json` or `saves/homes/<id>/home.json` and hydrated, painted objects lost their custom materials and textures.
   - This has been resolved: `zoneToJson()` now gathers all referenced materials from the global `MaterialManager` and embeds them under `zj["materials"]`. `applyZoneJson()` restores these materials and textures upon hydration.
   - Per-zone serialization logging has been integrated into `ZoneManager::persistZones()` and `hydrateFromZoneStore()`, recording painted object counts and total face texture allocations.

3. **Per-Zone Serialization Completeness Audit**:
   - Beyond face textures, several in-memory properties and entity states are currently omitted from per-zone serialization. A detailed gap analysis is documented below.

---

## 2. 3D Face Brush Tool Architecture & Wiring

### Interaction Chain
1. **Selection & Mode Activation**:
   - In `CreatorConsoleState`, `current3DMode` is set to `Mode3D::FaceBrush`.
   - `CreationTools::apply3DMode()` syncs `@creation-channel.activeTool = "FaceBrush"` and `@creation-channel.active3DMode = "FaceBrush"`, arming the `tool-face-brush-law` First Mover.
2. **Per-Frame Actuation**:
   - `EngineUpdate::update()` calls `Rendering::stepCreationTools()`.
   - `dispatchActiveTool()` checks `creatorToolIsUp(engine, Mode3D::FaceBrush)` and calls `Tool::FaceBrush(window, engine, zoneMgr, dt, targets)`.
3. **Raycasting & Geometry Hit-Testing**:
   - `buildMouseRay()` builds a world-space ray (or crosshair ray if cursor is locked).
   - `Object::raycastFace()` performs exact hit-testing against the object's geometry (cubes, polyhedra, smooth surfaces, Bézier patches), returning `hitFace` and `hitUV`.
4. **Copy-on-Write Material Divergence & Texture Mutation**:
   - `hitObj->ownMaterial()` diverges the object from `material.default` onto its private `material.<objID>` being.
   - `mat->initFaceTextures(faces)` ensures the texture buffer array is sized to match the geometry.
   - `PaintToolSurface` executes the selected brush mode (Normal Stroke, Airbrush, Chalk, Spray, Smudge, or Clone) into the CPU RGBA8 buffer.
   - `FaceTexture::updateWholeGPU()` uploads dirty regions to the active GPU texture.

---

## 3. Per-Zone FaceTexture Serialization & Logging

### Implementation Details
- **Export (`zoneToJson`)**:
  - Collects all unique `materialId` strings from `zone.objects()`.
  - Serializes each referenced `Material` (including its Base64-encoded `faceTextures` array and PBR parameters) into `zj["materials"]`.
- **Import (`applyZoneJson`)**:
  - Restores all materials from `zj["materials"]` into the global `MaterialManager` before objects are instantiated.
  - Objects re-link to their restored `material.<objID>` beings with exact painted pixels preserved.
- **Structured Logging (`logIo`)**:
  - `PERSIST Zone '<id>': N object(s), M painted object(s), K face texture(s)`
  - `HYDRATE Zone '<id>': N object(s), M painted object(s), K face texture(s)`

---

## 4. Comprehensive Audit: Missing Per-Zone Serialized State

The following in-memory properties and subsystem states exist on `Zone` and `Object` but are not yet serialized in per-zone identity files (`zone.json` / `home.json`):

| Subsystem / Property | In-Memory Representation | Serialized in Monolithic World Save? | Serialized in Per-Zone `zone.json`? | Status / Impact |
| :--- | :--- | :--- | :--- | :--- |
| **Face Textures & Materials** | `Material::faceTextures`, `MaterialManager` | Yes (`j["materials"]`) | **Fixed** (added to `zj["materials"]`) | Preserves painted surfaces across zone loads. |
| **Zone Deletability Permissions** | `Zone::_deletable` (`map<string, bool>`) | No | **Fixed** (added to `zj["deletable"]`) | Preserves per-person deletion locks. |
| **Spatial Field Root** | `Zone::_spatialRootObject` (`geom::FieldNode`) | No | **Missing** | Spatial distance fields and vector fields on zones are not persisted. |
| **Smooth Quadric Surfaces** | `Object::_hasSmooth`, `Object::smoothData` | No | **Missing** | Custom quadric coefficients / bounding trims are lost on reload. |
| **Complex Composite Shapes** | `Object::_hasComplex`, `Object::complexData` | No | **Missing** | Multi-patch composite shape structures are not persisted. |
| **General Object Attributes** | `Object::_attributes` (`map<string, string>`) | Partial (only `baseline` & `mass`) | **Missing** (custom attributes dropped) | Gameplay tags, physics markers, and tool metadata are lost. |
| **Rigid Body Dynamics** | `Physics::_rigidBodies` (velocity, angular vel) | No | **Missing** | Moving objects reset velocity to zero on zone re-entry. |
| **Zone-Scoped Laws & Rules** | `Zone`-specific `Law` instances | World-level only | **Missing** | Local laws authored specifically for a zone are saved in world saves, not zone identity. |
| **Object Automations** | `Object::_automations`, `_pendingRotation` | No | **Missing** | Active programmatic rotations or automations reset on load. |

---

## 5. Verification & Test Suite

1. **`tests/zones/zone_facetexture_test.cpp`** (16/16 checks passing):
   - Asserts that painted face textures diverge onto private materials.
   - Asserts that `zone.json` carries `materials` and `faceTextures`.
   - Asserts that a clean `MaterialManager` correctly restores all painted face pixels on `hydrateFromZoneStore()`.
2. **`tests/singularity/creation_tools_test.cpp`**:
   - Asserts `toolNameForMode(Mode3D::FaceBrush) == "FaceBrush"`.
   - Asserts `apply3DMode(state, channel, Mode3D::FaceBrush)` sets `state.currentTool` and `@creation-channel.activeTool`.
   - Asserts `creatorToolLawIdForMode("FaceBrush") == "tool-face-brush-law"`.
3. **`tests/zones/zone_identity_test.cpp`** (26/26 checks passing):
   - Asserts Home and Zone identity persistence across session files.
