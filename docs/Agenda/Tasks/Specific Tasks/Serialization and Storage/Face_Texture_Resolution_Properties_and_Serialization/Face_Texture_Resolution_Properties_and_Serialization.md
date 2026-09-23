# Face Texture Resolution Properties & Serialization

**Task:** Expose `textureResolution`, `textureWidth`, `textureHeight`, and `face.<f>.resolution` as authorable properties on `Material` and `Object`, implement layer-preserving bilinear resampling in `FaceTexture::resize()`, and ensure complete 6-face serialization and hydration without data loss.

**Status:** Completed and verified (2026-09-18).  
**Author:** Gemini Spark, reviewed by Codex / GPT-6 Astra.

## What Was Implemented

1. **`FaceTexture::resize(newWidth, newHeight)` with Layer Preservation**:
   - Implemented bilinear resampling (`resampleBilinear`) over RGBA8 buffers when resizing a face texture up to $4096 \times 4096$.
   - Iterates through all layers in `layers`, bilinearly resampling each active buffer instead of zeroing them out.
   - Triggers `compositeLayers()` when `useLayers` is enabled to re-blend layers at the new resolution.
   - Marks `id = 0` to prompt fresh GPU texture upload.

2. **Authorable Material Properties**:
   - Added `textureResolution`, `textureWidth`, and `textureHeight` properties registered via `ComputedProperty` on `Material`.
   - Wired into `Material::toJson()` and `Material::fromJson()`.

3. **Authorable Object Properties & Property Paths**:
   - Registered `textureResolution` on `Object` (delegating to own Material).
   - Added `face.<f>.resolution` bridge in `FacePropertyBridge` for per-face resolution queries and resizing.
   - Restored full 6-face serialization in `ObjectSerialization.cpp:245` (`obj.faceColors[0..5]`).
   - Wired `textureResolution` into `ObjectSerialization.cpp` `to_json` and `from_json`.

4. **Automated Verification**:
   - Added a multi-layer preservation assertion in `tests/constructed-being/face_texture_test.cpp`, verifying that paint in Layer 1 survives a $2\times 2 \to 4\times 4$ resize.
   - Verified that all test suites pass with zero regressions:
     - `face_texture_test`: OK
     - `object_roundtrip_test`: ALL OK
     - `shape_hydration_integrity_test`: 49/49 passed
     - `save_roundtrip_test`: 28/28 passed
     - `zone_identity_test`: 32/32 passed
     - `zone_spatial_field_roundtrip_test`: 11/11 passed
