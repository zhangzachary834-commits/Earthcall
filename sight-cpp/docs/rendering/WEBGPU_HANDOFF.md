# WebGPU Migration — Handoff for the Next Session

**Read this first, then `OPENGL_MIGRATION_PLAN.md` for depth.** This is the "start
here" for continuing the OpenGL→WebGPU migration in a fresh conversation.

---

## TL;DR — where we are

> **UPDATE (session 2):** Milestones **A–C below are DONE.** There is now **zero raw
> GL outside `src/Rendering/GL/`** — the whole app draws through the `Renderer`
> boundary, still running on OpenGL. What remains is Phase D (implement the new
> verbs in `WebGpuRenderer`), E (imgui backend swap), F (the flip). See
> "Boundary as it stands now" and "Remaining work" below, which supersede the
> original plan in this file.

- Milestones **M1** (stop re-tessellating / geometry caching), **M2** (renderer
  boundary — all object draw paths behind a `Renderer` interface), **M3** (retire
  GLU) are **DONE**. **M5's renderer is DONE and proven on-screen.**
- **The `WebGpuRenderer` is feature-complete and verified on real hardware** — the
  user watched `./webgpu_window` render a spinning, lit, shaded cube. Every WebGPU
  unknown (link, device, mesh pipeline, depth, perspective, texture, specular,
  overlays, the on-screen surface/swapchain/present path) is eliminated.
- The default `make` still builds and runs the **OpenGL** app, unchanged. WebGPU is
  opt-in via `make webgpu-*`. **Keep the OpenGL app working through all further prep.**
- **What remains = making WebGPU the app's LIVE backend.** That's a large, multi-turn
  phase: ~275 raw-GL calls across 10 files (mostly 2D UI) + the imgui backend swap +
  camera unification. A window is all-or-nothing (OpenGL *xor* WebGPU), so the app
  can't flip until the remaining raw GL is migrated.

## What's proven — do NOT re-derive

- **wgpu-native v29.0.1.1** vendored at `third_party/wgpu/` (`.a` committed, `.dylib`
  gitignored). Static-link recipe + v29 API gotchas are in `OPENGL_MIGRATION_PLAN.md`
  Milestone 5.
- **`src/Rendering/WebGPU/WebGpuRenderer.{hpp,cpp}`** — implements the `Renderer`
  interface: `drawMesh` (real), `drawImplicit` (stub → M6 raymarcher), `drawLines`,
  `drawOverlay`. Plus frame lifecycle: `beginFrame(w,h,clear)` [interface, live —
  currently a no-op stub], `beginFrameOffscreen(targetView,w,h,clear)` [real, tests
  use it], `endFrame()`, `setCamera(viewProj, eyePos)`, `setModel(model)`. Does
  Blinn-Phong (ambient+diffuse+specular), depth (Depth24Plus), perspective, texture
  albedo (uploads `RenderMaterial.albedoPixels`), two-sided lighting, and a flat-colour
  pipeline (additive/alpha/line-list) for overlays. `init(gpu, colorFormat)` — format
  must match the target (RGBA8Unorm offscreen, BGRA8Unorm surface).
- **`src/Rendering/WebGPU/WgpuDevice.hpp`** — header-only instance/adapter/device/queue
  bring-up (synchronous via processEvents).
- **`src/Rendering/WebGPU/smoke_window.mm`** — the working on-screen demo (GLFW
  `GLFW_NO_API` + `CAMetalLayer` + surface + swapchain + spinning cube). This is the
  reference for moving the surface into `Engine`.
- **Verify anytime:** `make webgpu-renderer` (5 offscreen scenes: depth, albedo,
  specular, overlay, lines) and `make webgpu-window` (on-screen; user runs it).

## Architecture facts you need

- **`src/Rendering/Renderer.hpp`** is the boundary. `currentRenderer()` returns the
  active backend (defaults to a static `OpenGLRenderer`). `setCurrentRenderer(Renderer*)`
  swaps it. `beginFrame`/`endFrame` are virtual with **empty defaults** (OpenGL inherits
  no-ops; only WebGPU overrides). `GameRender::render()` already brackets its drawing
  with `currentRenderer().beginFrame(fbW,fbH,{zone rgb})` / `endFrame()`.
- **Objects draw via `obj.drawObject()` → `currentRenderer().drawMesh(...)`.** The MODEL
  transform is applied by the **caller**: OpenGL does `glMultMatrixf(obj.transform)` in
  GameRender; **for WebGPU you must call `renderer.setModel(obj.getTransform())` before
  `obj.drawObject()`.** This is the key wiring for rendering the real scene.
- **Material is a being** (`Material : Singular`, global `MaterialManager materials`).
  Objects reference by identifier string; resolved to a flat `RenderMaterial` at draw
  time via `resolveRenderMaterial()`. `faceTextures` = per-face albedo (paint); the
  portable pixels are `RenderMaterial.albedoPixels`. See `[[material-as-being]]` memory.
- **WebGPU gotchas:** clip depth is [0,1] → build any projection fed to `WebGpuRenderer`
  with `GLM_FORCE_DEPTH_ZERO_TO_ONE`. Pipeline colour format must match the attachment.
  Native lines are always 1px (OpenGL glow width doesn't translate).

## Boundary as it stands now (session 2)

`Renderer.hpp` grew from 4 draw verbs to a full backend contract. Additions, and
why each was forced rather than chosen:

| Addition | Why it had to exist |
|---|---|
| `enum class Blend` + `drawSolid(tris, color, blend, depthWrite)` | The gravity-field arrows blend **additively**; a single alpha path would have dulled them. Covers every gizmo cube / handle / ghost. |
| `setWireframe(bool)` | The BrushCreate hologram wraps an *arbitrary object draw* in `glPolygonMode(GL_LINE)`. It is render STATE, not a verb. |
| model stack: `setModel` / `pushModel` / `popModel` / `currentModel()` | `BodyPart` and `Formation` compose child transforms onto parents. Implemented once in the base over a single `applyModel` hook. |
| recorded camera: `setCamera(view, proj, eye)` + `view()/proj()/eyePos()/viewport()` | `drawNametag`, `BrushSystem`, `Zone` read the camera back with `glGetDoublev`. Also fixes a latent bug: the old readback ran at *end* of frame. |
| `setLight(pos, amb, diff, spec)` / `setLightingEnabled` | `ShadingSystem` was pure `GL_LIGHT0`. Lighting is now scene policy; installing it is backend policy. |
| `begin2D/end2D`, `drawTris2D`, `drawLines2D`, `drawImage2D` | The 2D UI (chosen strategy (a), not imgui draw lists). |
| `uploadTexture/releaseTexture` + `TextureHandle` | `FaceTexture` owned a raw `GLuint`. |
| `draw::` adapters | WebGPU has no `GL_QUADS`/`GL_POLYGON`/`GL_LINE_LOOP`/`GL_POINTS`/`GL_LINE_STIPPLE`. Adapters: `quadsToTris`, `fanToTris`, `stripToSegments`, `dashSegments`, `pointsToTris`, `rectTris`, `rectOutline`, `easyFontToTris`. |

Backends implement `applyModel` / `applyCamera` / `applyBeginFrame` / `applyLight` /
`applyLightingEnabled`; all shared state lives in the base class.

**Decisions taken this session, with reasons:**
- **Skipped the "render the real scene" demo** (old step 1). It duplicates what the
  flip itself proves; porting call sites keeps the app live on OpenGL throughout.
- **Did NOT consolidate onto `PersonPerspective`** (old step 2). The migration needs
  glm matrices at the boundary, which it now has. Rewiring perspective/input is
  app-level cleanup with regression risk and no migration payoff.
- **Deleted AdvancedFacePaint's GL apparatus.** Its shaders/VAO/VBO fed only
  `renderGradientPreview`/`renderSmudgePreview`, which nothing ever called — and
  `initialize()` ran at startup, so it would have demanded a GL context under
  `GLFW_NO_API`. The real painting is CPU-side and untouched.

## Known gaps to close in Phase D
- `WebGpuRenderer` stubs `drawSolid`/`begin2D`/`end2D`/`drawTris2D`/`drawLines2D`/
  `drawImage2D` — they `warnOnce` to stderr instead of drawing.
- `WebGpuRenderer::drawLines` **ignores `Blend`** (`_linesPipe` is alpha-only), so
  the additive gravity arrows will render alpha until a second pipeline exists.
- `WebGpuRenderer` ignores the recorded light: `lightDir` is still hardcoded to
  `normalize(2,5,2)` and is *directional*, while OpenGL's `GL_LIGHT0` is
  **positional** and follows the camera. Wire `lightPos()` through and decide the
  shading model. Changing this will move the `webgpu-renderer` scene expectations.
- `uploadTexture` returns 0 (this backend uploads `albedoPixels` per draw).
- `setWireframe` is unimplemented (needs a line-list pipeline variant).

## Latent oddity (pre-existing, not caused by the migration)
`World::load()` calls `drawGround()` — a draw issued outside any frame. Harmless
(`WebGpuRenderer::drawMesh` guards on a null pass, and the visible ground is the
scaled placeholder cube drawn in `GameRender`), but it is dead work.

## Remaining work — recommended order

### 1. (Recommended next) Render the REAL scene on WebGPU
Prove the renderer eats real Earthcall content, not a hand-made cube. Extend
`smoke_window.mm` (or a sibling demo):
- **Light version:** build real geometry meshes directly —
  `geom::tessellateSmooth(geom::makeSphere(r))`, `geom::tessellateSdf(field)`,
  `geom::tessellatePatch(...)`, `geom::tessellateComplex(...)` — and `drawMesh` them.
  Links only the geometry `.cpp` files. Proves smooth surfaces / marched SDFs / patches
  render.
- **Fuller version:** link the Object machinery, `setCurrentRenderer(&webgpu)`, and in
  the loop: for each world Object, `webgpu.setModel(obj.getTransform()); obj.drawObject();`
  This routes the real M2 object dispatch (smooth/complex/field/patch/cube/polyhedron)
  through WebGPU. High payoff.

### 2. Camera unification (backend-independent — do it on OpenGL, stays verifiable)
Per `OPENGL_MIGRATION_PLAN.md` → "Perspective And The Camera". Today `GameRender` builds
a fixed-function `_camera` (`glFrustum` + `ecgl::lookAtMul` + reads it back with
`glGetDoublev`). `PersonPerspective` already produces glm view/proj but is unused. Make
`PersonPerspective` the source of truth; feed `setModel`/`setCamera`; delete the
`glGetDoublev` readback. Reduces coupling and preps the WebGPU camera path.

### 3. Decide the 2D-UI strategy (the biggest chunk — ~140 GL calls)
Half the remaining raw GL is 2D UI: `DesignSystem.cpp` (60), `Form.cpp` (30),
`Menu.cpp` (29), `Zone.cpp` (21) — 2D panels/cards/strokes via `glBegin/glColor/glVertex`.
**Pick a strategy before migrating:** (a) add 2D draw verbs (`drawQuad2D`/`drawLine2D`)
to the `Renderer` + a 2D pipeline in `WebGpuRenderer`; or (b) re-express the 2D UI as
Dear ImGui draw lists (imgui is already the editor UI). This is a design decision worth
an explicit choice.

### 4. imgui backend swap
Makefile links `imgui_impl_opengl2`. Swap to `imgui_impl_wgpu` (already vendored at
`../imgui/backends/imgui_impl_wgpu.{h,cpp}`). Update `Engine.cpp` init/new-frame/render/
shutdown calls; it needs the wgpu device + queue + surface format.

### 5. Backend flip in Engine
Move the surface setup from `smoke_window.mm` into `Engine` behind a flag (e.g.
`USE_WEBGPU`): window hint `GLFW_NO_API`; create surface/adapter/device/queue +
`WebGpuRenderer`; `setCurrentRenderer(&webgpu)`. Then refactor `WebGpuRenderer::beginFrame`
(live) to acquire the surface texture and call the shared pass setup that
`beginFrameOffscreen` already has. Also: the app must build projections with
`GLM_FORCE_DEPTH_ZERO_TO_ONE` when WebGPU is active.

### Also open / deferred
- **Resource caching** in `WebGpuRenderer` (per-draw buffers/textures today) — deferred
  on purpose; do it once WebGPU is live and the per-frame pattern is measurable.
- **M6** — SDF raymarcher in `drawImplicit` (exact implicit rendering); M7 — WGSL codegen
  from `SdfToken`/`OntoMath` (the manifesto payoff). Both after the live switch.

## Verify / gotchas

- `make` → OpenGL app builds (**must stay green through all prep**).
- `make webgpu-renderer` → 5 offscreen scenes pass. `make webgpu-window` → user runs.
- `make test` currently **fails to compile `property_bridge_test.cpp:222`** — this is
  the USER's parallel Person/Body WIP (`BodyPart._subObjects` = `vector<unique_ptr<Object>>`
  made `Body` non-copyable; the test copies a `Body`). **NOT a rendering issue — leave it.**

## File map
```
src/Rendering/Renderer.hpp            the boundary (4 verbs + frame lifecycle)
src/Rendering/RenderMaterial.{hpp,cpp} flat GPU material (+ albedoPixels) & resolver
src/Rendering/GL/OpenGLRenderer.*     OpenGL backend (live today)
src/Rendering/WebGPU/WebGpuRenderer.* WebGPU backend (done, offscreen+onscreen proven)
src/Rendering/WebGPU/WgpuDevice.hpp   device bring-up
src/Rendering/WebGPU/smoke_*.{cpp,mm} verification: offscreen, mesh, renderer, window
third_party/wgpu/                     vendored wgpu-native v29.0.1.1 (.a committed)
OPENGL_MIGRATION_PLAN.md              full plan + all v29 API notes + decisions
```
Memory: `[[material-as-being]]` has the running WebGPU status; keep it updated.
