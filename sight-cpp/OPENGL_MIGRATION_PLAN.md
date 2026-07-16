# Earthcall OpenGL Migration Plan

> **Revised 2026-07-16.** This revision reverses the previous rendering recommendation.
> The earlier version of this document named `bgfx` as the migration target. That
> recommendation predated the manifesto and is now withdrawn — see
> [Why Not bgfx](#why-not-bgfx). The target is **WebGPU**, via `wgpu-native`.
>
> Also corrected in this revision: Phase 0 was described as the first step but was
> never built; `GL3Renderer` was described as a migration beachhead but is a dead
> stub; and the WebKit dependency was treated as architectural when it is a
> throwaway test program.

## Purpose

This document plans the migration of `sight-cpp` away from legacy OpenGL while
preserving Earthcall's own mathematical and ontological architecture.

The goal is not to replace Earthcall's object model with a normal game-engine
ontology. The goal is to move commodity rendering, GPU plumbing, import/export,
UI support, and selected numerical calculations onto stronger foundations while
keeping Earthcall's categories as the source of truth.

In short:

```text
Keep custom:
  Singular, Object, Person, Relation, Formation, Zone, Law,
  authorship, provenance, self-propagation, spatial taxonomy.

Offload or vendor:
  renderer backend, shader pipeline, viewport details, exact geometry kernels,
  mesh processing, physics solving, asset import/export, gizmos and editor UI helpers.
```

## Current State

The build is a small custom C++ app with a hand-written `Makefile`. It links
directly against GLFW, OpenGL, WebKit, and Cocoa, and compiles ImGui sources from
outside the `sight-cpp` folder.

Current dependency shape:

```text
Window/input:
  GLFW 3.4

Rendering:
  direct OpenGL / GLU calls — fixed-function pipeline

UI:
  Dear ImGui 1.92 WIP, via imgui_impl_opengl2 (the fixed-function backend)

Math:
  GLM vectors and matrices

Serialization:
  vendored json.hpp

Geometry:
  custom Earthcall code (SmoothSurface, PolyhedronData, ComplexShape, Sdf)

Physics:
  custom Earthcall code
```

### The rendering surface is small

An accurate inventory matters, because it determines how large this migration
actually is. It is smaller than it looks.

```text
873 GL call sites across 18 files, overwhelmingly fixed-function:
  171 glVertex3f       46 glBegin/glEnd pairs
   69 glVertex2f       35 glPushMatrix/glPopMatrix
   30 glColor3f        24 glMatrixMode
```

This is OpenGL 1.1 — a 1997 API. Apple deprecated OpenGL in 2018 and froze it at
4.1; the compatibility profile that `glBegin` requires never went past 2.1.

The complete set of GPU features Earthcall uses today:

```text
GL_DEPTH_TEST   GL_BLEND   GL_LIGHTING (one light)   GL_TEXTURE_2D
```

That is the entire ceiling. There are **no framebuffer objects anywhere** — no
shadow maps, no post-processing, no render-to-texture. No face culling. The
lighting is one positional `GL_LIGHT0` (`ShadingSystem.cpp`), placed each frame
at `cameraPos + (2, 5, 2)`, with ambient 0.2 / diffuse 0.8 / specular 1.0,
shininess 32, Gouraud interpolation, and `GL_COLOR_MATERIAL` routing vertex color
into ambient+diffuse.

A single Blinn-Phong vertex/fragment shader pair reproduces essentially the whole
renderer. This is the fact that makes the migration tractable, and it is also the
fact that dissolves the standard objection to WebGPU — see
[Phase 2](#phase-2-choose-the-rendering-backend).

### Known defects in the current renderer

Two things are wrong today, independent of which API comes next:

**1. Geometry is re-tessellated every frame.**

```cpp
// ObjectRender.cpp:100
drawTessMesh(geom::tessellateSmooth(smoothData));

// ObjectRender.cpp:113
drawTessMesh(geom::tessellatePatch(complexData.patches[i]));

// ObjectRender.cpp:273  (highlight outline)
drawShell(geom::tessellateSmooth(smoothData, 20, 12));
```

Each of these rebuilds a mesh from scratch, allocating a fresh `TessMesh`, inside
the draw path — sixty times a second, for geometry that changes maybe once a
minute. It is then handed to `glDrawArrays` through client-side pointers, which
re-uploads the whole thing to the GPU on every draw.

Only `drawFieldModel` (`ObjectRender.cpp:122`) does the right thing, caching
`_fieldMesh` and rebuilding on change. **That is the pattern the other paths need.**

The comment at `ObjectRender.cpp:81` blames "the multi-object lag" on GL call
counts, and the fix it describes (immediate mode → vertex arrays) was real. But it
treated the symptom. The disease is rebuilding unchanged geometry.

**2. `GL3Renderer` is a dead stub.**

`src/Rendering/GL/GL3Renderer.cpp` draws one triangle. It is gated behind
`USE_GL3_RENDERER`, which the `Makefile` never defines, so it compiles to nothing.
`Game.hpp:374` holds it as a concrete member; `Engine.cpp:27` and
`GameRender.cpp:44` branch on the same dead define. It is not a beachhead. It
should be deleted, not extended.

### What the WebKit dependency is

`src/Integration/RealWebView.cpp` imports Cocoa and WebKit and allocates an
`NSWindow`, compiled `-ObjC++`. **This is a test program for cross-app/web
integration, not architecture.** It does not constrain the renderer decision and
it is not a portability blocker. Earlier drafts of this document overweighted it.

## Architectural Principle

Earthcall objects must remain named and classified before they are rendered.

A normal engine tends to collapse an object into:

```text
node/entity/gameobject + transform + mesh + material + collider
```

Earthcall should instead treat those as realizations:

```text
EarthcallSpatialObject
  identity:
    Singular/Object identity

  taxonomy:
    spatial existence kind, dimensionality, manifoldness, boundary,
    orientability, smoothness, volume/surface status, topology class

  exact geometry:
    parametric, BRep, implicit, algebraic, procedural, or mesh-native form

  rendering realization:
    transient mesh, GPU buffers, materials, textures

  physics realization:
    simplified collider/proxy, mass, constraints, simulation state

  relational reality:
    Relation and Formation bindings

  lawful reality:
    Law bindings, event history, authorship, provenance

  propagation reality:
    rules for duplication, synthesis, mutation, inheritance, validation
```

The mesh is not the being. The mesh is the visible garment. The collider is not
the being. The collider is the simulation proxy. The engine node is not the being.
The engine node is a runtime handle.

## What This Migration Is Actually For

Worth stating plainly, because it is easy to mistake:

**OpenGL is not what makes Earthcall slow.** The re-tessellation defect above is
the cost, and Metal, Vulkan, and WebGPU would each re-upload the regenerated mesh
just as obediently as OpenGL does. No API fixes it.

What a modern API does is make the defect **unwritable**. None of them have a
client-side-array path to fall back on. You cannot hand WebGPU a raw pointer and
hope; geometry must live in a `Buffer`, which forces the question of *when it
changes* to be answered explicitly, once, at the place where it changes.

The migration's value is that it converts a performance mistake into a compile
error. The second value — larger, and covered below — is that it makes
Person-authored math executable on the GPU.

## Migration Strategy

### Phase 0: Preserve The Existing App

**Status: not started.** Previously listed as the first step; never built.
`GL3Renderer` was an attempt to skip it.

Do not begin by ripping out OpenGL everywhere. First define boundaries so that
OpenGL becomes an implementation detail.

Tasks:

1. Create a renderer interface around the current OpenGL calls.
2. Move raw draw calls behind methods like `drawMesh`, `drawImplicit`,
   `drawDebugLine`, `drawGrid`, and `drawSelectionOutline`.
3. Keep the existing OpenGL renderer as `OpenGLRenderer` until a replacement exists.
4. Delete `GL3Renderer` and its `USE_GL3_RENDERER` branches.
5. Keep the Earthcall object model untouched during this pass.

Target shape:

```cpp
class Renderer {
public:
    virtual void beginFrame(const RenderFrameContext& ctx) = 0;

    // The mesh path: triangle realizations. What you can touch.
    virtual void drawMesh(const RenderMesh& mesh, const Material& material,
                          const glm::mat4& transform) = 0;

    // The implicit path: exact surfaces, raymarched. What you can see.
    // Peer of drawMesh, not a special case of it. See "The Implicit Path".
    virtual void drawImplicit(const geom::SdfNode& node, const Material& material,
                              const glm::mat4& transform) = 0;

    virtual void drawDebugLine(const glm::vec3& a, const glm::vec3& b,
                               const glm::vec4& color) = 0;
    virtual void endFrame() = 0;
};
```

Three constraints on this interface, each from the manifesto:

- **`drawImplicit` is a peer, not an afterthought.** Tessellating an SDF is a lossy
  approximation; the manifesto requires exactness. See below.
- **Geometry is optional.** Laws are extra-spatial Objects; Zones may be
  extra-spatial. `Object → mesh` is not total, and `Object.hpp` already permits
  `_hasSmooth/_hasComplex/_hasField` all false as a legal state. The Renderer must
  not assume a being has a garment.
- **The view comes from Perspective.** `UserPerspective::getProjectionMatrix` already
  exists. The renderer receives the view; it does not own it.

This is the bridge out of OpenGL. After this, Earthcall can still run while
backends change.

### Phase 1: Vendor The Small Dependencies

Earthcall should be self-contained in the practical repository sense:
dependencies needed to build the app should live inside the repo, pinned to known
versions, with licenses tracked.

Recommended vendor layout:

```text
sight-cpp/
  third_party/
    glm/
    imgui/
    nlohmann_json/
    tinygltf/
    imguizmo/
    wgpu-native/        # prebuilt dylib + webgpu.h
```

Rationale:

- `glm`: already used everywhere for vector/matrix math. Also supplies
  `glm::unProject` / `glm::project`, the replacements for the GLU calls that die
  with OpenGL (`Tool.cpp:255`, `Tool.cpp:257`, `Person.cpp:156`).
- `imgui`: already central to the Creator Console. Already ships
  `backends/imgui_impl_wgpu.cpp` — the WebGPU backend is vendored and waiting.
- `nlohmann_json`: already effectively vendored as `src/json.hpp`.
- `tinygltf`: small, C++ friendly, good first step for glTF import/export.
- `ImGuizmo`: transform gizmos without hand-rolling editor manipulation.
- `wgpu-native`: ships prebuilt release binaries — a dylib and a header. No Rust
  toolchain needed to consume it.

This phase should also replace the current fragile include paths with CMake. The
current `Makefile` is serviceable, but dependency-heavy work will be cleaner in
CMake.

### Phase 2: Choose The Rendering Backend

**Decision: WebGPU, via `wgpu-native`.**

The decisive argument is not portability. It is that WGSL is text, and Earthcall
already has the compiler front-end. See
[Person-Authored Math On The GPU](#person-authored-math-on-the-gpu) — that section
is the real content of this decision, and the rest of this comparison is
bookkeeping.

#### Option A: WebGPU — chosen

Why:

- **WGSL is a text shader language compiled at runtime**
  (`wgpuDeviceCreateShaderModule` takes a source string). This is what makes
  Person-authored law compilable to GPU programs. No other option offers this
  portably.
- Modern explicit API: pipelines, bind groups, command encoders, compute shaders.
- One API and one shader language across macOS, Windows, Linux, and the browser.
- On macOS it runs on Metal underneath, so Apple Silicon performance is retained.
- `imgui_impl_wgpu.cpp` is already vendored, and targets Dawn, wgpu-native, and
  emscripten from one file via a single `#define`.
- `wgpu-native` and Dawn both implement the same `webgpu.h` C API. Choosing WebGPU
  picks an *interface*, not a vendor — the implementation can be swapped with a
  link-line change.

Tradeoffs:

- No mesh shaders, no raytracing, no MetalFX. Earthcall's feature ceiling is one
  positional light and alpha blending; the common denominator is far above what it
  asks for. This objection does not bind.
- The API is younger than Metal's and still settling at the edges.
- Local-ML interop is a real seam. See
  [The Honest Counterpoint](#the-honest-counterpoint).

Earthcall fit:

```text
Excellent. The renderer serves the ontology, and — uniquely among the options —
the ontology's own math can be compiled down into it.
```

Implementation note: start with `wgpu-native` (prebuilt binaries, drop into the
build). Dawn is a Chromium subproject and builds like one — depot_tools, gn,
ninja. Its stricter validation may be worth adopting later; the swap is cheap.

#### Option B: Direct Metal backend

Recommendation: not chosen. It would be throwaway work.

Why it was considered:

- Apple-native, best-in-class tooling (Xcode GPU frame capture), unified memory on
  Apple Silicon, compute shaders. MSL is text, so runtime codegen works.

Why it lost:

- **WebGPU on macOS is Metal underneath.** A Metal backend written now gets deleted
  the day a WebGPU backend lands, because WebGPU already covers the Mac. Metal
  would only earn its keep if Earthcall needed something WebGPU cannot express, and
  a renderer whose entire feature set is `GL_DEPTH_TEST + GL_BLEND + GL_LIGHTING +
  GL_TEXTURE_2D` is nowhere near that boundary.
- The usual argument for Metal here — that the Obj-C++ interop barrier was already
  crossed — rested on `RealWebView.cpp`, which is a throwaway test program.

Note: Xcode's GPU frame capture still works against a WebGPU app, since it captures
the underlying Metal command stream. Less legible (you see wgpu's generated calls,
not your own), but the buffers and textures are all still inspectable.

Earthcall fit:

```text
Good API, wrong layer. It would be deleted on arrival of the thing that replaces it.
```

#### Option C: Vulkan + MoltenVK

Recommendation: not chosen.

Why it was considered:

- Cross-platform, mature, explicit.

Why it lost:

- On macOS this is Vulkan *translated into Metal at runtime* — a translation layer
  costing both performance and debuggability, to reach the API you could have called
  directly.
- SPIR-V is **bytecode**. Generating shaders from Person-authored law would mean
  writing a real compiler backend, not walking a tree and emitting text. This is
  disqualifying given what Earthcall wants to do.
- Enormously verbose. For a solo author whose real work is an ontology kernel,
  Vulkan is a tar pit.

Earthcall fit:

```text
Justified only if Windows/Linux were a hard requirement AND runtime shader
generation were off the table. Neither holds.
```

#### Why Not bgfx

The previous version of this document recommended bgfx. That recommendation is
withdrawn, for a reason sharper than "it isn't frontier":

**bgfx expects shaders compiled ahead of time**, by its `shaderc` tool, from a
bespoke GLSL dialect, into a private binary format. A Person authoring a new law at
runtime and having it become a GPU program would mean shelling out to `shaderc`
mid-session or embedding it.

bgfx is architecturally opposed to the manifesto's central premise — that Persons
author the math and the substrate runs it. It is a compatibility abstraction from
the 2010s whose design (view-ordered submission, single-threaded encoding) predates
the explicit-API era it wraps, and on macOS it renders through Metal anyway. It is
a defensible choice for the goal of shipping to five platforms with hand-authored
shaders. That is not Earthcall's goal.

#### Option D: Godot/Unreal/Unity migration

Recommendation: not the migration path.

Why:

- These engines solve huge amounts of app/editor/runtime work.

Tradeoffs:

- Their native ontology becomes primary unless carefully resisted.
- Godot thinks in `Node + Scene`. Unity thinks in `GameObject + Component`. Unreal
  thinks in `Actor + Component`.
- Earthcall taxonomy would become custom metadata rather than the primary world
  grammar.

Earthcall fit:

```text
Useful only if Earthcall's ontology is wrapped as a separate kernel and the engine
is treated as a front-end host. Risky as root architecture.
```

## Person-Authored Math On The GPU

This is the section that decides the backend. Everything above is downstream of it.

### Earthcall already has two compiler front-ends

```cpp
// geom::SdfToken — Sdf.hpp:32
enum Kind { Num, X, Y, Z, Add, Sub, Mul, Div, Pow, Neg,
            Sin, Cos, Tan, Sqrt, Abs, Exp, Log };
```

That is postfix bytecode — an intermediate representation, in the compiler sense,
with an opcode enum. `Sdf.hpp` says why it exists: *"deliberately plain data (not
opaque lambdas) so it can be serialized, introspected, and edited from a math-mode
editor as well as from direct manipulation."*

`OntoMath::Expression` is the same instinct at the Law layer — signomial terms plus
`TransFactor{Sin, Cos, Exp, Ln}`, exact differentiation, *"every primitive
Person-modifiable, serializable like all law text, evaluated on demand."*

Two Person-authored, serializable, introspectable math trees. Both are one backend
away from being GPU programs.

### WGSL is text

```cpp
WGPUShaderModule module = wgpuDeviceCreateShaderModule(device, &desc);
// desc holds a WGSL source string — built at runtime, from a tree walk.
```

Walking an `SdfToken` RPN stream and emitting WGSL is an afternoon's work, because
the hard half is done: the IR exists and it is clean.

What the alternatives ask instead:

| Backend | Shader language | Runtime codegen |
|---|---|---|
| **WebGPU** | WGSL — text, specified grammar | `createShaderModule(source)`. Done. |
| Metal | MSL — text | Works, but Apple-only |
| Vulkan | SPIR-V — **bytecode** | Requires a real compiler backend |
| bgfx | bespoke GLSL dialect | Offline `shaderc` binary — fights it |

### Why this matters more than portability

If Person-authored laws only ever execute in a CPU Rete network, the size of a
world is bounded by one thread's throughput — and that thread is already contested
by law matching, the event bus, and Human Language Processing. If they compile to
WGSL, a law evaluates over a million points per frame.

The manifesto's own framing applies: *"Singularity orders much of the compute via
priority in event bus."* For Singularity to arbitrate compute, compute must be a
schedulable resource. Queues and command-buffer submission are exactly that.
Fixed-function GL offers nothing to schedule.

### The interpret-vs-native fork, one layer down

The manifesto already worked this out, for law synthesis:

> *"First and conceptually simpler is just have the system create new laws simply by
> calling the constituent sub-laws... But that eventually introduces lots of
> overhead and turns it into an interpretive system. We want it native."*

The identical fork reappears when SDF math meets the GPU:

```text
Interpreted:
  Upload the SdfToken RPN as a buffer. One shader executes it.
  No pipeline explosion, no compile stalls, unlimited shapes.
  Cost: a per-sample interpreter loop. Slow.

Native:
  Codegen WGSL per shape tree.
  Fast.
  Cost: every newly authored shape triggers a pipeline compile, which stalls.
```

The resolution is the hybrid that Earthcall's own law-creation flow already
describes — *"New arrangement created by Person... Arrangement is saved,
generalized with applicable conditions to specific referents, and logged"*:

```text
While authoring:  interpret the RPN.
                  Instant feedback, no stall, arbitrary edits.

On save:          codegen WGSL, cache the pipeline keyed by tree hash.
                  The moment of compilation is the moment of authorship.
```

This is not an aesthetic choice. It falls out of the pipeline-stall problem. It
happens to land where the law flow already pointed.

## The Implicit Path

`tessellateSdf` (`Sdf.cpp:291`) marches tetrahedra at `res = 24`. That means the
renderer currently shows the Person **a faceted approximation of their own math** —
a lie with a resolution parameter, visible on zoom.

The manifesto rules this out:

> *"mathematically precise so the user isn't just drawing or using it in an art-tool
> like way... they can also use it in a desmos-graphing/research-grade simulator
> style way."*

Raymarching the SDF in a fragment shader is exact to the pixel, with no resolution
knob and no detail ceiling. Hence `drawImplicit` as a peer of `drawMesh` in the
Renderer interface.

Two consequences:

- **The re-tessellation defect ceases to exist for SDF objects** rather than being
  fixed — there is no mesh to rebuild or upload.
- **`tessellateSdf` stays.** `ObjectCollision.cpp:160` tessellates for collision,
  and polyhedra are natively meshes.

So the architecture is a deliberate hybrid:

```text
Implicit path  ->  exact surfaces, raymarched     ->  what you can see
Mesh path      ->  triangle realizations, buffers ->  what you can touch
```

Make that split on purpose. It is the "exact geometry" vs "rendering realization"
distinction from the Architectural Principle, showing up in the render loop.

## Non-Orientable Surfaces And Pipeline State

`SmoothSurface.hpp:30` carries `bool orientable = true` — always true today, with a
comment promising "non-orientable exotics later." The manifesto wants Möbius
strips, Klein bottles, and projective planes.

A Möbius strip has **no globally consistent normal**: walk the surface and you
return inverted. Today Earthcall is accidentally correct about this — there is no
`GL_CULL_FACE` call anywhere, so everything renders two-sided by default.

WebGPU does not permit accidental correctness. Cull mode is explicit, immutable
pipeline state fixed at pipeline creation, and two-sided lighting requires
`@builtin(front_facing)` in WGSL to flip the normal per fragment.

Which means:

```text
orientable  ->  selects the render pipeline
```

A topological property of the being chooses how it is drawn. `orientable` stops
being a field nobody reads. This is the Architectural Principle enforced by the
API — objects named and classified *before* they are rendered. Fixed-function GL let
the classification be skipped and produced the right pixels anyway; WebGPU makes the
taxonomy do work.

Klein bottles additionally self-intersect, so depth handling must not assume a
manifold boundary.

## The Honest Counterpoint

One manifesto ambition genuinely favors Metal:

> *"Could train models natively omnimodal with each Earthcall primitive and law as
> modalities AND text."*

Local ML on Apple Silicon means MLX or CoreML, and those want `MTLBuffer`. WebGPU
buffers are not `MTLBuffer`s. Sharing them means dropping through wgpu's HAL to the
underlying Metal device — possible (both wgpu and Dawn expose it), but a seam with
sharp edges.

It does not flip the decision, for two reasons:

- The other stated ML path — *"link up a frontier LLM for the most sophisticated
  pure-language processing"* — is a network call to a hosted model and needs no GPU
  interop at all.
- Native omnimodal training is a far larger and later project than the renderer. If
  it arrives, it may justify its own Metal compute path *alongside* WebGPU
  rendering, which the Renderer interface already permits.

Worth knowing the seam exists. Not worth paying for it now.

## Text As A Foundational Modality

The manifesto places Human Language at Singularity level — *"a distinct dimension of
semantic meaning-representation irreducible to the raw mediary physical components
(i.e. words aren't just ink)."*

Today text is ImGui chrome plus `gluProject` nametags (`Person.cpp:156`). If Human
Language is ontologically foundational, text rendering is eventually a first-class
render path rather than the UI library's job.

Practical convergence: high-quality runtime text means MSDF glyph atlases, which is
the same signed-distance machinery as the implicit shape path. The language modality
and the form modality can share a representation rather than each getting a bespoke
one. Not urgent; worth not foreclosing.

## Geometry And Topology Libraries

Rendering migration and mathematical migration should be related but separate. The
renderer should not decide what a shape is. The renderer only receives a
mesh/implicit view of a deeper object.

### OpenCASCADE

Best for: BRep solids, CAD-style exact geometry, NURBS, analytic curves/surfaces,
shells, compounds, Boolean operations, fillets, shape healing, STEP/IGES exchange.

```text
Very strong for exact spatial object identity, especially solids and constructed
forms. Heavy dependency. Best introduced after the renderer boundary exists.
```

### CGAL

Best for: robust computational geometry, exact predicates and constructions,
triangulations, arrangements, convex hulls, polygon/polyhedron operations, mesh
generation, AABB/KD trees.

```text
Very strong for validation and generative operations.
Licensing and dependency complexity must be reviewed package by package.
```

### libigl

Best for: mesh geometry processing — curvature, remeshing, parameterization,
deformation.

```text
Strong for mesh-level operations, weaker for exact ontology.
Good middle step before larger CAD kernels.
```

### geometry-central

Best for: surface mesh algorithms, discrete differential geometry, intrinsic
triangulations, surface analysis.

```text
Strong for surface intelligence. Use as a math assistant, not as the spatial ontology.
```

### Recommended Geometry Decision

Do not start with OpenCASCADE or CGAL. They are powerful, but they will force build
and modeling decisions before Earthcall has a clean renderer boundary.

Recommended order:

1. Preserve and formalize the current `SmoothSurface` and `PolyhedronData` taxonomy.
2. Add an explicit `SpatialClassification` model.
3. Vendor `tinygltf` for import/export.
4. Add `libigl` or `geometry-central` for mesh processing experiments.
5. Add CGAL for robust computational geometry once specific operations are needed.
6. Add OpenCASCADE when Earthcall needs exact BRep/NURBS/CAD-style construction.

Note the interaction with the implicit path: raymarching reduces the pressure to
tessellate *well*, but not the pressure to tessellate *correctly* — collision still
consumes meshes.

## Physics Libraries

The current physics code is useful as an Earthcall law prototype, but long-term
physics solving should not stay entirely handmade.

### Jolt Physics

Best default candidate for game-like rigid body physics: modern C++, good
performance orientation, suitable for interactive applications.

```text
Strong for runtime physics proxies. Earthcall Laws should wrap Jolt, not disappear
into Jolt.
```

### Bullet

Mature rigid body and collision detection, broad ecosystem, permissive zlib license.

```text
Solid fallback. More mature, but older and potentially less clean to integrate.
```

### PhysX

High-performance production physics, larger game/visual simulation workflows.

```text
Technically strong, but not the first choice for a self-contained open C++ app.
```

### Recommended Physics Decision

Prototype a `PhysicsBackend` interface before adopting a physics library.

```cpp
class PhysicsBackend {
public:
    virtual void syncFromEarthcallObjects(World& world) = 0;
    virtual void step(float deltaTime) = 0;
    virtual void syncToEarthcallObjects(World& world) = 0;
};
```

Then implement:

```text
HandmadePhysicsBackend
JoltPhysicsBackend
```

Earthcall Laws remain above the backend:

```text
Earthcall Law -> backend commands -> simulation result -> event log -> Law history
```

The physics engine may solve the motion, but it must not become the author of the
Law.

## UI And Editor Tools

Keep Dear ImGui. It already fits the Creator Console and keeps the app lightweight.
The WebGPU backend (`imgui_impl_wgpu.cpp`) is already vendored; migrating means
swapping `imgui_impl_opengl2` for it and setting one define.

Recommended additions:

- `ImGuizmo` for transform gizmos.
- ImGui docking if the Creator Console grows into a multi-panel editor.

Do not migrate the whole UI to a full engine editor. Earthcall's user experience
should emerge from its own authorial grammar, not from a generic scene editor.

## Build System Migration

The current `Makefile` should become CMake.

Reasons:

- Easier vendoring of third-party source.
- Cleaner platform branches.
- Better dependency tracking, CI, and release packaging.

Target layout:

```text
sight-cpp/
  CMakeLists.txt
  cmake/
  third_party/
  src/
  docs/
  assets/
```

The first CMake version should still build the current OpenGL app. Do not combine
"new build system" and "new renderer" in one risky jump.

## Proposed Milestones

Milestones 1–4 are reversible and pay off under any backend. Only Milestone 5
commits to WebGPU.

### Milestone 1: Stop Re-Tessellating

Outcome:

```text
Geometry is rebuilt when it changes, not when it is drawn.
```

Deliverables:

- Extend the `_fieldMesh` caching pattern (`ObjectRender.cpp:122`) to
  `drawSmoothModel`, `drawComplexModel`, and `drawHighlightOutline`.
- Invalidate on geometry change — the setters at `Object.hpp:495` / `Object.hpp:498`
  already centralize the mutation points.

Risk:

- Low. Probably the largest frame-time win currently available, and true regardless
  of backend.

### Milestone 2: Renderer Boundary

Outcome:

```text
OpenGL still renders the app, but all raw GL is behind an Earthcall Renderer interface.
```

Deliverables:

- `Renderer.hpp` with both `drawMesh` and `drawImplicit`
- `OpenGLRenderer`
- `RenderMesh`, `Material`
- migrated draw paths for cubes/polyhedra/smooth surfaces
- **deletion** of `GL3Renderer` and `USE_GL3_RENDERER`

Risk:

- Medium. Touches rendering paths but should preserve behavior. Get the interface
  reviewed before building on it.

Design note: `Material` is the one piece of this that touches the ontology. Color
currently arrives via scattered `glColor3f` immediate calls; Earthcall has no
material concept. Decide where `Material` lives relative to `Object` before
Milestone 5 forces the answer.

### Milestone 3: Retire GLU

Outcome:

```text
No dependency on a library that dies with OpenGL.
```

Deliverables:

- `glm::unProject` / `glm::project` replacing `gluUnProject` (`Tool.cpp:255`,
  `Tool.cpp:257`) and `gluProject` (`Person.cpp:156`)
- hand-rolled sphere/cylinder generators replacing `gluSphere` / `gluCylinder` /
  `gluNewQuadric` (`ObjectRender.cpp:57`, `ObjectRender.cpp:66`) — these have no
  successor in any modern API

Risk:

- Low. Mechanical.

### Milestone 4: Build And Vendor Hygiene

Outcome:

```text
Dependencies are explicit, local, and buildable from repo-controlled paths.
```

Deliverables:

- `third_party/`, CMake prototype, vendored ImGui/GLM/json/tinygltf/ImGuizmo,
  license manifest

Risk:

- Medium. Build churn, low ontology risk.

### Milestone 5: WebGPU Mesh Path

Outcome:

```text
The app renders through WebGPU. Feature parity with the fixed-function pipeline.
```

Deliverables:

- `WebGpuRenderer` against the Milestone 2 interface
- GLFW in `GLFW_NO_API` mode + `CAMetalLayer` surface
- `wgpu-native` vendored
- one Blinn-Phong WGSL shader pair reproducing `ShadingSystem`: single positional
  light at `cameraPos + (2,5,2)`, ambient 0.2 / diffuse 0.8 / specular 1.0,
  shininess 32, vertex color into ambient+diffuse
- `imgui_impl_wgpu` swapped in for `imgui_impl_opengl2`

Note: the current pipeline is Gouraud (per-vertex). A fragment shader gives
per-pixel lighting — a visible improvement, not a regression, but an intentional
deviation worth recording.

Risk:

- Medium/high. First real GPU work; shader and surface setup are fiddly.

### Milestone 6: Implicit Path

Outcome:

```text
SDF objects render exactly, at pixel precision, with no tessellation.
```

Deliverables:

- raymarching WGSL shader
- `SdfToken` RPN uploaded as a buffer; one interpreter shader executes it
- `drawImplicit` implemented on `WebGpuRenderer`

Risk:

- Medium/high. This is the manifesto's "research-grade" claim becoming true.

### Milestone 7: WGSL Codegen

Outcome:

```text
Person-authored math becomes a GPU program at the moment of authorship.
```

Deliverables:

- `SdfNode` tree walk emitting WGSL
- pipeline cache keyed by tree hash
- interpret-while-authoring / compile-on-save policy
- async pipeline creation so compiles do not stall the frame

Risk:

- High. Also the highest payoff. This is where `OntoMath` eventually follows.

### Milestone 8: Spatial Classification Kernel

Outcome:

```text
Earthcall has a formal spatial taxonomy independent of rendering.
```

Deliverables:

- `SpatialClassification`, `SpatialExistenceKind`, `SpatialDimensionality`,
  `BoundaryKind`, `ManifoldKind`, `OrientabilityKind`, `SmoothnessKind`,
  `RealizationKind`, validation hooks

Risk:

- Low/medium. Mostly additive. Note that Milestone 5 already forces `orientable` to
  become load-bearing via pipeline selection, so some of this arrives early.

### Milestone 9: Physics Backend Boundary

Outcome:

```text
Current physics remains available, but Earthcall can route simulation through a
backend interface.
```

Deliverables:

- `PhysicsBackend`, `HandmadePhysicsBackend`, law-to-backend mapping, event logging

Risk:

- Medium. Must keep Law authorship above backend mechanics.

## Decision Matrix

| Area | Recommended Choice | Why |
|---|---|---|
| Immediate fix | Cache tessellations | Largest available win; backend-independent |
| Renderer path | Renderer interface over current OpenGL | Lowest risk bridge |
| **Main renderer** | **WebGPU via wgpu-native** | **WGSL is text → Person-authored law compiles to GPU** |
| WebGPU implementation | wgpu-native first, Dawn optional later | Prebuilt binaries; same `webgpu.h` either way |
| Apple-native backend | Rejected | WebGPU is Metal underneath; Metal would be deleted on arrival |
| Vulkan | Rejected | SPIR-V is bytecode; codegen needs a compiler backend |
| bgfx | Rejected | Offline shader toolchain fights runtime law authoring |
| Full game engine | Defer | Too much ontology pressure |
| Exact SDF rendering | Raymarch, don't tessellate | Tessellation is a lie with a resolution parameter |
| Shape → shader | Interpret while authoring, compile on save | The manifesto's own interpret-vs-native answer |
| Non-orientable surfaces | `orientable` selects pipeline | Taxonomy does work the API enforces |
| UI | Dear ImGui + ImGuizmo | Already present; `imgui_impl_wgpu` already vendored |
| Build | CMake | Needed for vendored dependencies |
| Asset import | tinygltf first | Small and practical |
| Physics | Jolt behind `PhysicsBackend` | Modern C++ runtime physics |
| Exact CAD geometry | OpenCASCADE later | Strong BRep/NURBS/topological modeling |
| Robust comp. geometry | CGAL later | Strong algorithms; licensing/build complexity |
| Mesh processing | libigl or geometry-central | Good bridge before heavier kernels |
| Local ML interop | Deferred; Metal seam if ever needed | Frontier-LLM path needs no GPU interop |

## Chosen Direction

```text
1. Keep Earthcall ontology custom.
2. Stop re-tessellating in the draw path. This is the real perf bug.
3. Add the renderer abstraction — with drawImplicit as a peer of drawMesh.
4. Delete the GL3Renderer stub.
5. Keep OpenGL as the first backend behind that interface.
6. Move build toward CMake and local vendoring.
7. Adopt WebGPU (wgpu-native) as the renderer target.
8. Raymarch SDFs rather than tessellating them — exactness is the point.
9. Compile Person-authored math to WGSL: interpret while authoring, compile on save.
10. Add geometry/physics libraries only behind Earthcall-owned interfaces.
11. Formalize SpatialClassification before importing heavy kernels.
```

The through-line: OpenGL's fixed-function pipeline let Earthcall skip its own
taxonomy and still get the right pixels. A modern API makes the taxonomy
load-bearing — `orientable` selects a pipeline, exact geometry raymarches instead of
approximating, and authored math becomes executable rather than interpreted.

The renderer stops being a thing Earthcall talks *past* and becomes a thing
Earthcall's categories talk *through*.

## Source Notes

Recommendations are based on the current `sight-cpp` structure, the Earthcall
Ourverse Manifesto, and the following public documentation:

- WebGPU specification: https://www.w3.org/TR/webgpu/
- WGSL specification: https://www.w3.org/TR/WGSL/
- wgpu-native: https://github.com/gfx-rs/wgpu-native
- Dawn: https://dawn.googlesource.com/dawn
- Apple Metal: https://developer.apple.com/metal/
- bgfx overview: https://bkaradzic.github.io/bgfx/overview.html
- Dear ImGui: https://github.com/ocornut/imgui
- Godot nodes and scenes: https://docs.godotengine.org/en/stable/getting_started/step_by_step/nodes_and_scenes.html
- Unity GameObjects: https://docs.unity3d.com/Manual/GameObjects.html
- Unreal Actors: https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine
- CGAL: https://www.cgal.org/
- OpenCASCADE overview: https://dev.opencascade.org/doc/overview/html/index.html
- Jolt Physics: https://github.com/jrouwe/JoltPhysics
- libigl: https://libigl.github.io/
- geometry-central: https://geometry-central.net/
