# Authorable Material Color Picker

**Task:** Replace the privileged Creator Console material-color click path with an authored RGB picker whose selected color is a Singular Property and whose application to Material is performed by Laws.

**Status:** First usable rung implemented; Person verification remains open.

## What Zach asked for

Zach asked for a Law that can change Materials directly, without the Creator Console route (`3D tools → Material Color → picker`), while preserving the quality of the existing 3D color-picker experience. This extends the already-working Basic Pixel Changer: the pixel writer now reads `@material-color-picker.selectedColor`, so the same authored color instrument can drive pixels and Materials.

## What is authored

- `scripts/author_basic_material_color_picker.py` merges an RGB picker into `saves/zones/BasicPixelChanger/zone.json` and writes four stable Law roots under `saves/laws/`.
- `material-color-picker` is an ordinary 2D Object with an authored `selectedColor` `vec3` Property; the red, green, and blue strips are ordinary 2D Objects whose bounds are their authored geometry, not a new ShapeKind.
- Three `object-clicked` Laws map `@interaction-channel.hoveredU` into one component of `selectedColor`, mirror the vector to `@creation-channel.activeColor` for compatibility, and publish `color-selection-changed`.
- The apply Law maps `selectedColor` to `@material.material-color-picker-preview.baseColor` and `@material.authored-color-target.baseColor`; the target Material is shared by name, so this is deliberately the place where a future object-specific copy-on-write target law must be authored.
- The pixel Law now maps its `WritePixel` color from `@material-color-picker.selectedColor`; it no longer depends on the Creator Console's transient color state.

## Runtime boundary fixed

Zone departure exposed a lifetime bug in `LawManager::remove`: a Law's `Singular` destructor announced itself while its owning slot was still visible to the manager's release callback. Removal now detaches the owning `shared_ptr` before final destruction, preserving the callback's invariant that every Law it visits is still alive.

## Remaining frontier

This rung is intentionally RGB-strip based. The polished HSV square/ring picker can be authored next as additional OntoMath mappings and Laws once the desired hue/saturation/value coordinate vocabulary is settled. Material targeting also needs a Person-authored selection Law for “which Material/object” rather than silently repainting a shared Material; existing `Object::ownMaterial`/`setFaceColor` remains the safe copy-on-write route for object paint.

## Verification

Automated coverage is in `tests/law/basic_pixel_changer_test.cpp`: it loads the authored Zone closure, clicks the RGB strip through `InteractionChannel`, checks `selectedColor`, checks the Creation-channel bridge, checks the target Material, paints a canvas texel, elevates an individual pixel Property, and verifies an OntoMath-defined region Property. The final visual check is recorded in `docs/Agenda/Tasks/For Zach/Person Verification List.md`.

Authored by Zach; implementation and test extension by Codex (GPT-5), session `01a07d15-f266-7902-bc11-cf7b06b0b343`, 2026-09-10 22:40 PDT.
