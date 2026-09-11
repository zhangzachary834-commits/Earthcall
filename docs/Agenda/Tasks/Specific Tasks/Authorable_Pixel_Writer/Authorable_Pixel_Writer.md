# Authorable Pixel Writer

**Status:** implemented and Person-verified; authored-domain authoring surface remains
**Section in the To-Do list:** Person-facing surface  
**Created:** 2026-09-07 by Zach and Codex (GPT-5)  
**Zone:** `saves/zones/BasicPixelChanger/zone.json`
**Law root:** `saves/laws/law-basic-pixel-changer/law.json`
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

The `BasicPixelChanger` Zone contains the Shape2D Object `basic-pixel-canvas` and names
the shared Law root `law-basic-pixel-changer` through `lawRefs`. The Law hears
`object-clicked`, requires that exact canvas identity, and performs `WritePixel` from
`@interaction-channel.hoveredFace`, `.hoveredU`, `.hoveredV`, and
the canvas's own authored `paintColor` Property. The authored color-picker apply Law maps
its `selectedColor` into `paintColor` on `color-selection-changed`; the click itself needs
neither Creator Console state nor a global named-being lookup.

The save records **Zach** as author. Its `injected_by` envelope records **Codex (GPT-5),
session `01a07d15-f266-7902-bc11-cf7b06b0b343`** as the mechanism that wrote the file by
Zach's authority.

## Verification and remaining frontier

The focused booted test activates the actual authored Zone and shared Law root from an isolated SaveRoot, performs
the real 2D pick/click/event/Law/Screen path, verifies exact texel mutation and Material
divergence, then uses authored `AddProperty` and `ElevatePixels` actions to test live pixel
and OntoMath-set reads, writes, serialization, and JSON round trips.

**Person witness — Zach, 2026-09-10 20:36 PDT:** after the Zone-scoped Law closure landed,
Zach relaunched, moved to the canvas, clicked it, and reported: “IT WORKS” and “I put red
dots on it.” This confirms the native WebGPU surface, click sensing, authored Law, Material
copy-on-write, and visible pixel manifestation together—not merely the headless test.

Still open beyond this task: a Person-facing authored-domain surface, authored texture
resolution, stroke identity/history/provenance, and compiled/GPU evaluation for very large
or continuously changing selections. These are not grounds for a rigid region vocabulary.

**Regression and repair — Zach, 2026-09-11:** after the full RGB/HSV interface appeared,
Zach reported that clicking the canvas no longer manifested a pixel, then discovered the
decisive clue himself: changing the Creator Console 3D-tool color to red made marks appear.
The writer was active but painting its legacy white selection onto the white canvas.
`saves/worlds/basic_pixel_changer.json` and `.ecform` still embedded an older copy of the
Law that read `@creation-channel.activeColor`, and `loadState` admitted that compatibility
bag after activating the Zone's canonical Law closure. Both Zach-owned artifacts now carry
the canvas-local `paintColor` path. More importantly, the loader now overlays the active
Zone's shared Law roots onto colliding embedded copies before its one register replacement,
so an old World cannot silently undo a Zone Law again. The focused test supplies all ten
live Zone objects, deliberately loads a stale Creator-Console copy, and proves the authored
picker still changes the exact addressed texel. Native confirmation remains on the Person
Verification List.

## Next human-authored rung: the domain of writing

**Zach's direction, 2026-09-10 20:36 PDT:** “a pixel writer where the domain of
pixel-write [is] authored.” This sharpens the next rung: not merely authoring the color or
the clicked coordinate, but authoring the set of coordinates at which writing has meaning.

Let a Person author an OntoMath defined set

\[
D = \{(u,v) \in [0,1]^2 \mid \varphi(u,v,\text{world properties})\text{ is defined}\}.
\]

The point-write Law binds `u` and `v` to the Interaction channel, uses an existing
`ConditionNode::Zone` over `φ` so a click outside `D` does not satisfy the Law, and keeps
the existing `WritePixel` act for the accepted coordinate. The set-write path uses the
same `φ` through existing `ElevatePixels` to grant a named, writable Property, then uses
ordinary `Set` / `Map` / `Flow` Law vocabulary on that Property. This composition needs no
`RegionKind`, `BrushKind`, new `ShapeKind`, or new action kind. A Person-facing authoring
surface and an OntoMath-authored support for the canvas itself remain implementation work.

**Recorded by:** Codex (GPT-5), session
`01a07d15-f266-7902-bc11-cf7b06b0b343`, 2026-09-10 20:36 PDT.

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
