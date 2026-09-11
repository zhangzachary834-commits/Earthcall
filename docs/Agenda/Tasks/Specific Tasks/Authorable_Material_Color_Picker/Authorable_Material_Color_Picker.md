# Authorable Material Color Picker

**Task:** Replace the privileged Creator Console material-color click path with an authored RGB/HSV picker whose selected color is a Singular Property and whose application to Material is performed by Laws.

**Status:** Full direct-RGB and 2D HSV selector implemented; Person verification remains open.

## What Zach asked for

Zach asked for a Law that can change Materials directly, without the Creator Console route (`3D tools → Material Color → picker`), while preserving the quality of the existing 3D color-picker experience. This extends the Basic Pixel Changer: the same authored color instrument drives pixels and Materials.

## What is authored

- `scripts/author_basic_material_color_picker.py` merges the complete picker into `saves/zones/BasicPixelChanger/zone.json` and writes six stable Law roots under `saves/laws/`.
- `material-color-picker` is an ordinary 2D Object with authored `selectedColor`, `hue`, `saturation`, and `value` Properties; the red, green, blue, value, and chromatic controls are ordinary 2D Objects whose bounds are authored geometry, not a new ShapeKind.
- Three `object-clicked` Laws map `@interaction-channel.hoveredU` into one component of `selectedColor`, mirror the vector to `@creation-channel.activeColor` for compatibility, and publish `color-selection-changed`.
- The 2D chromatic Law maps pointer `u` to hue and `1-v` to saturation; the value Law supplies the third dimension. Both feed a six-piece, exact OntoMath HSV→RGB function, so the full gamut is authored mathematics rather than a hidden color-conversion method.
- The apply Law maps `selectedColor` to the canvas's authored `paintColor` Property, `@material.material-color-picker-preview.baseColor`, and `@material.authored-color-target.baseColor`; the target Material is shared by name, so this is deliberately the place where a future object-specific copy-on-write target law must be authored.
- The pixel Law reads `paintColor` locally from its clicked canvas subject. It therefore depends on neither the Creator Console's transient color state nor a global named-being lookup during the click edge.

## Runtime boundary fixed

Zone departure exposed a lifetime bug in `LawManager::remove`: a Law's `Singular` destructor announced itself while its owning slot was still visible to the manager's release callback. Removal now detaches the owning `shared_ptr` before final destruction, preserving the callback's invariant that every Law it visits is still alive.

## Remaining frontier

Material targeting still needs a Person-authored selection Law for “which Material/object” rather than silently repainting a shared Material; existing `Object::ownMaterial`/`setFaceColor` remains the safe copy-on-write route for object paint. Click-and-drag may later be layered onto the same controls through the existing interaction levels without changing their color mathematics.

## Verification

Automated coverage is in `tests/law/basic_pixel_changer_test.cpp`: it loads the authored Zone closure, clicks the 2D field at HSV(.5,.75,1), verifies exact cyan RGB, lowers value to .4, directly changes the red channel, checks the Creation-channel bridge and target Material, paints a canvas texel, elevates an individual pixel Property, and verifies an OntoMath-defined region Property. The final visual check is recorded in `docs/Agenda/Tasks/For Zach/Person Verification List.md`.

Authored by Zach; implementation and test extension by Codex (GPT-5), session `01a07d15-f266-7902-bc11-cf7b06b0b343`, 2026-09-10 22:40 PDT.

Full 2D chromatic selector extension by Codex (GPT-5), same session, drawing directly from Zach's request for “more basic sliders” and “the full 2D chromatic selector,” 2026-09-11 10:50 PDT.

Canvas-click regression repair by Codex (GPT-5), same session, prompted by Zach's live report that the new interface appeared but clicking the canvas no longer manifested a pixel, 2026-09-11 11:44 PDT. The test now uses the complete live Zone object set and proves the picker-to-canvas `paintColor` handoff before the exact texel write.
