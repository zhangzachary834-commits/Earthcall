# Authorable Pixel Writer

**Status:** implemented and structurally verified; Person-facing visual verification remains  
**Section in the To-Do list:** Person-facing surface  
**Created:** 2026-09-07 by Zach and Codex (GPT-5)  
**Save:** `saves/worlds/basic_pixel_changer.json`  
**Test:** `tests/law/basic_pixel_changer_test.cpp`

---

## Human intent

Zach asked for a **Basic Pixel Changer Law**: clicking a 2D display should replace the
addressed pixel with the selected color, so an authorable display is not confined to the
rigid, serialized `ShapeKind` vocabulary. He then made two architectural requirements
explicit:

1. any individual pixel or selected set of pixels must be capable of being elevated as a
   Property, even though every raw pixel need not permanently be one; and
2. a named region's bounds must be authored with OntoMath, not chosen from rigid region
   presets (outside deliberately narrow tests).

Those requirements originate with Zach. The Screen-channel act, live Property projection,
and defined-set interpretation described below are Codex's implementation of that human
direction.

## What landed

- `Shape2D` picking now reports normalized local `hoveredU` and `hoveredV`, closing the
  missing Sense coordinate path.
- append-only `ActionNode::Kind::WritePixel` (`21`) reads face, `u`, `v`, and color from
  PropertyPaths and acts through a Screen-channel sink; it publishes the past-tense
  `surface-pixel-written` edge only after a successful write.
- `Shape2D` now manifests its face texture, so the pixel act has a visible 2D consumer.
- the write crosses `Object::ownMaterial()`, preserving Material copy-on-write instead of
  repainting every Object that shared the original Material.
- `AddProperty` with `surface.pixel.<face>.<x>.<y>` elevates exactly one live sample as an
  enumerable, writable, persisted `vec3` Property.
- append-only `ActionNode::Kind::ElevatePixels` (`22`) grants an arbitrary authored
  Property name. Its membership is the defined set of an OntoMath `Piecewise` evaluated
  at each texel center with local variables `u` and `v`. The companion
  `surface.selection.<name>` Property persists the face and exact expression.
- a named set reads as a row-major `list<vec3>` and accepts either one `vec3` to fill the
  set or a same-sized list for per-sample writes. Batches cause one texture upload.
- writes through either PropertyPath or the Screen act participate in the ordinary change
  feed for every affected elevated Property.

This adds no domain class, top-level subsystem, ShapeKind, or region enum. Pixel storage
remains dense Material/Screen substrate until a Person authors the elevation.

## Authored example

`basic_pixel_changer.json` contains the Zone `BasicPixelChanger`, the Shape2D Object
`basic-pixel-canvas`, and the Law `law-basic-pixel-changer`. The Law hears
`object-clicked`, requires that exact canvas identity, and performs `WritePixel` from
`@interaction-channel.hoveredFace`, `.hoveredU`, `.hoveredV`, and
`@creation-channel.activeColor`.

The save records **Zach** as author. Its `injected_by` envelope records **Codex (GPT-5),
session `01a07d15-f266-7902-bc11-cf7b06b0b343`** as the mechanism that wrote the file by
Zach's authority.

## Verification and remaining frontier

The focused booted test loads the actual authored save from an isolated SaveRoot, performs
the real 2D pick/click/event/Law/Screen path, verifies exact texel mutation and Material
divergence, then uses authored `AddProperty` and `ElevatePixels` actions to test live pixel
and OntoMath-set reads, writes, serialization, and JSON round trips.

Still open beyond this task: Person-facing visual/feel confirmation, authored texture
resolution, stroke identity/history/provenance, and compiled/GPU evaluation for very large
or continuously changing selections. These are not grounds for a rigid region vocabulary.

## First-click render crash, found and fixed after this landed

Zach's first live click into the canvas (via `Run Earthcall.command`) aborted the app with
a wgpu validation panic: `RenderPipeline with '' label uses attachments with formats []`
against the swapchain's `[Some(Bgra8Unorm)]` render pass. The `drawImage2D` pipeline
(`WebGpuRenderer.cpp`, the textured screen-space blit this Law's `WritePixel` act renders
through) built its `WGPUFragmentState` but never attached it to the pipeline descriptor, so
wgpu created a pipeline with zero color targets — creation succeeded silently, and the
first real draw against the live BGRA pass then aborted at encoder finish.

Codex diagnosed this from the panic and added the missing `ipd.fragment = &ifrag`
assignment with an explanatory comment at the exact line. Claude (session
`01Mvd55GFWyUMrYWt2ERGSRE`) confirmed `earthcall_webgpu` builds clean and launches without
crashing on an unrelated world, but has no driver for interacting inside this native GPU
window, so the actual repro (click the pixel-changer canvas, confirm no abort) is recorded
in the Person Verification List rather than claimed here as verified.

## Second bug, one level up: the canvas rendered red, not white

After the crash fix, Zach reported clicking still appeared to do nothing — the canvas
was actually rendering red with white label text. This traced to a Zone-identity-store
bug unrelated to rendering: `saves/zones/BasicPixelChanger/`'s stale snapshot (predating
`faceColors` being authored) was silently overriding the World's authored canvas
wholesale, so `faceColors[0]` sat at its C++ default (legacy cube red). Fixed as a general
architecture fix, not a one-off — see
[Zone_identity_store_field_level_merge](../Zone_identity_store_field_level_merge/Zone_identity_store_field_level_merge.md).
The stale identity files were deleted with Zach's authorization. Whether the canvas now
actually shows white and paints visibly is recorded in the Person Verification List.

---

**Signed:** Codex (GPT-5)  
**Session:** `01a07d15-f266-7902-bc11-cf7b06b0b343`  
**Date:** 2026-09-07  
**Timestamp:** 2026-09-07 11:54 PDT
