# Earthcall OpenGL Migration Plan

## Purpose

This document plans the eventual migration of `sight-cpp` away from direct legacy OpenGL calls while preserving Earthcall's own mathematical and ontological architecture.

The goal is not to replace Earthcall's object model with a normal game-engine ontology. The goal is to move commodity rendering, GPU plumbing, import/export, UI support, and selected numerical calculations onto stronger foundations while keeping Earthcall's categories as the source of truth.

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

The current build is a small custom C++ app with a hand-written `Makefile`. It links directly against GLFW, OpenGL, WebKit, and Cocoa, and compiles ImGui sources from outside the `sight-cpp` folder.

Current dependency shape:

```text
Window/input:
  GLFW

Rendering:
  direct OpenGL / GLU calls

UI:
  Dear ImGui

Math:
  GLM vectors and matrices

Serialization:
  vendored json.hpp

Geometry:
  mostly custom Earthcall code

Physics:
  mostly custom Earthcall code
```

The strongest existing custom math systems are:

- `SmoothSurface`: quadrics, parametric torus/ovoid, topology flags, raycasting, signed-distance-style queries, tessellation.
- `PolyhedronData`: vertices, faces, normals, convexity, face areas, vertex curvature, truncation, duals, topology validation.
- `Physics`: rigid body state, gravity laws, spring bonds, collision broadphase, GJK/EPA-style convex collision, and Earthcall-facing physics laws.

That means the project is not just a renderer. It already contains a first draft of an Earthcall geometry/physics substrate. The migration must not flatten that into generic `Mesh + Transform + Collider`.

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

The mesh is not the being. The mesh is the visible garment. The collider is not the being. The collider is the simulation proxy. The engine node is not the being. The engine node is a runtime handle.

## Migration Strategy

### Phase 0: Preserve The Existing App

Do not begin by ripping out OpenGL everywhere. First define boundaries so that OpenGL becomes an implementation detail.

Tasks:

1. Create a renderer interface around the current OpenGL calls.
2. Move raw draw calls behind methods like `drawMesh`, `drawSmoothSurface`, `drawPolyhedron`, `drawGrid`, `drawSelectionOutline`, and `drawDebugPrimitive`.
3. Keep the existing OpenGL renderer as `OpenGLRenderer` until a replacement exists.
4. Keep the Earthcall object model untouched during the first renderer abstraction pass.

Target shape:

```cpp
class Renderer {
public:
    virtual void beginFrame(const RenderFrameContext& ctx) = 0;
    virtual void drawMesh(const RenderMesh& mesh, const Material& material,
                          const glm::mat4& transform) = 0;
    virtual void drawDebugLine(const glm::vec3& a, const glm::vec3& b,
                               const glm::vec4& color) = 0;
    virtual void endFrame() = 0;
};
```

This is the bridge out of OpenGL. After this, Earthcall can still run while backends change.

### Phase 1: Vendor The Small Dependencies

Earthcall should be self-contained in the practical repository sense: dependencies needed to build the app should live inside the repo, pinned to known versions, with licenses tracked.

Recommended first vendor layout:

```text
sight-cpp/
  third_party/
    glm/
    imgui/
    nlohmann_json/
    tinygltf/
    imguizmo/
```

Rationale:

- `glm`: already used everywhere for vector/matrix math.
- `imgui`: already central to the Creator Console.
- `nlohmann_json`: already effectively vendored as `src/json.hpp`; move it to a clearer dependency location later.
- `tinygltf`: small, C++ friendly, good first step for glTF import/export.
- `ImGuizmo`: useful for transform gizmos without hand-rolling editor manipulation.

This phase should also replace the current fragile include paths with a clearer build system. The current `Makefile` is serviceable, but dependency-heavy work will be cleaner in CMake.

### Phase 2: Choose The Rendering Backend

There are three realistic rendering paths.

#### Option A: bgfx as the renderer layer

Recommendation: strongest default choice for `sight-cpp`.

Why:

- C/C++ friendly.
- "Bring your own engine/framework" style, so Earthcall can keep its own ontology.
- Supports multiple backends, including Metal, Vulkan, Direct3D, OpenGL, WebGL, and WebGPU/Dawn.
- Permissive BSD-2 license.
- Works with GLFW-style windowing.

Tradeoffs:

- Adds a real renderer abstraction with its own shader toolchain.
- More setup than a tiny single-header renderer.
- Not a scene engine; Earthcall still owns scene/object semantics.

Earthcall fit:

```text
Excellent. bgfx can become the rendering servant without claiming the ontology.
```

#### Option B: Direct Metal backend

Recommendation: good later if Earthcall is Mac-first and wants maximum Apple-native control.

Why:

- Apple-native, modern replacement path for legacy OpenGL on macOS.
- Strong tooling, profiling, debugging, compute, and shader support.
- Good long-term fit for Apple silicon.

Tradeoffs:

- Mac/iOS/visionOS focused.
- More engine work remains on Earthcall.
- Cross-platform strategy becomes harder unless Metal is just one backend.

Earthcall fit:

```text
Good as a backend, not as the whole renderer strategy unless the project is intentionally Apple-first.
```

#### Option C: Godot/Unreal/Unity migration

Recommendation: not the first migration path.

Why:

- These engines solve huge amounts of app/editor/runtime work.
- They have excellent import pipelines, editor tooling, rendering, physics, animation, and deployment.

Tradeoffs:

- Their native ontology becomes primary unless carefully resisted.
- Godot thinks in `Node + Scene`.
- Unity thinks in `GameObject + Component`.
- Unreal thinks in `Actor + Component`.
- Earthcall taxonomy would likely become custom metadata/components rather than the primary world grammar.

Earthcall fit:

```text
Useful only if Earthcall's ontology is wrapped as a separate kernel and the engine is treated as a front-end host.
Risky if used as the root architecture.
```

### Recommended Rendering Decision

Use `bgfx` as the likely renderer target, while keeping the renderer interface narrow enough that a direct `MetalRenderer` can be added later.

Target backend path:

```text
Phase 0:
  Earthcall objects -> OpenGLRenderer

Phase 2:
  Earthcall objects -> Renderer interface -> bgfx

Later:
  Earthcall objects -> Renderer interface -> bgfx/Metal/Vulkan/etc.
```

This preserves optionality. bgfx can use Metal underneath on macOS, but Earthcall does not have to become a Metal-only project.

## Geometry And Topology Libraries

Rendering migration and mathematical migration should be related but separate.

The renderer should not decide what a shape is. The renderer only receives a mesh/material view of a deeper object.

### OpenCASCADE

Best for:

- BRep solids.
- CAD-style exact geometry.
- NURBS, analytic curves/surfaces, shells, solids, compounds.
- Boolean operations, fillets, shape healing, STEP/IGES-style exchange.

Earthcall fit:

```text
Very strong for exact spatial object identity, especially solids and constructed forms.
Heavy dependency. Best introduced after the renderer boundary exists.
```

### CGAL

Best for:

- Robust computational geometry.
- Exact predicates and constructions.
- Triangulations, arrangements, convex hulls, polygon/polyhedron operations.
- Mesh generation, geometry processing, AABB/KD trees, shape analysis.

Earthcall fit:

```text
Very strong for validation and generative operations.
Licensing and dependency complexity must be reviewed package by package.
```

### libigl

Best for:

- Mesh geometry processing.
- Curvature, remeshing, parameterization, deformation, geodesic-ish workflows.

Earthcall fit:

```text
Strong for mesh-level operations, weaker for exact ontology.
Good middle step before larger CAD kernels.
```

### geometry-central

Best for:

- Surface mesh algorithms.
- Discrete differential geometry.
- Intrinsic triangulations and surface analysis.

Earthcall fit:

```text
Strong for surface intelligence.
Use as a math assistant, not as the spatial ontology.
```

### Recommended Geometry Decision

Do not start with OpenCASCADE or CGAL immediately. They are powerful, but they will force build and modeling decisions before Earthcall has a clean renderer boundary.

Recommended order:

1. Preserve and formalize the current `SmoothSurface` and `PolyhedronData` taxonomy.
2. Add an explicit `SpatialClassification` model.
3. Vendor `tinygltf` for import/export.
4. Add `libigl` or `geometry-central` for mesh processing experiments.
5. Add CGAL for robust computational geometry once specific operations are needed.
6. Add OpenCASCADE when Earthcall needs exact BRep/NURBS/CAD-style construction.

## Physics Libraries

The current physics code is useful as an Earthcall law prototype, but long-term physics solving should not stay entirely handmade.

### Jolt Physics

Best default candidate for game-like rigid body physics.

Why:

- Modern C++.
- Good performance orientation.
- Suitable for games and interactive applications.
- Less historically heavy than Bullet.

Earthcall fit:

```text
Strong for runtime physics proxies.
Earthcall Laws should wrap Jolt, not disappear into Jolt.
```

### Bullet

Best for:

- Mature rigid body and collision detection.
- Broad ecosystem and long history.
- Permissive zlib license.

Earthcall fit:

```text
Solid fallback. More mature, but older and potentially less clean to integrate.
```

### PhysX

Best for:

- High-performance production physics.
- Larger game/visual simulation workflows.

Earthcall fit:

```text
Technically strong, but not the first choice for a self-contained open C++ app.
```

### Recommended Physics Decision

Prototype a `PhysicsBackend` interface before adopting a physics library.

Target shape:

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

The physics engine may solve the motion, but it must not become the author of the Law.

## UI And Editor Tools

Keep Dear ImGui for now. It already fits the Creator Console and keeps the app lightweight.

Recommended additions:

- `ImGuizmo` for transform gizmos.
- ImGui docking if the Creator Console grows into a multi-panel editor.
- A more disciplined project/file browser after save/load stabilizes.

Do not migrate the whole UI to a full engine editor yet. Earthcall's user experience should emerge from its own authorial grammar, not from a generic scene editor.

## Build System Migration

The current `Makefile` should eventually become CMake.

Reasons:

- Easier vendoring of third-party source.
- Cleaner platform branches for OpenGL, Metal, bgfx, and future backends.
- Better dependency tracking.
- Easier CI and release packaging.
- Easier to keep libraries inside `third_party/`.

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

The first CMake version should still build the current OpenGL app. Do not combine "new build system" and "new renderer" in one risky jump.

## Proposed Milestones

### Milestone 1: Renderer Boundary

Outcome:

```text
OpenGL still renders the app, but all raw OpenGL rendering is behind an Earthcall Renderer interface.
```

Deliverables:

- `Renderer.hpp`
- `OpenGLRenderer`
- `RenderMesh`
- `Material`
- migrated object drawing path for cubes/polyhedra/smooth surfaces

Risk:

- Medium. Touches rendering paths but should preserve behavior.

### Milestone 2: Build And Vendor Hygiene

Outcome:

```text
Dependencies are explicit, local, and buildable from repo-controlled paths.
```

Deliverables:

- `third_party/`
- CMake prototype
- vendored ImGui/GLM/json/tinygltf/ImGuizmo
- license manifest

Risk:

- Medium. Build churn, but low ontology risk.

### Milestone 3: bgfx Prototype

Outcome:

```text
One small scene renders through bgfx while the existing app still works.
```

Deliverables:

- `BgfxRenderer`
- shader compilation path
- one mesh/material path
- comparison with OpenGL output

Risk:

- Medium/high. Shader and backend setup can be fiddly.

### Milestone 4: Spatial Classification Kernel

Outcome:

```text
Earthcall has a formal spatial taxonomy independent of rendering.
```

Deliverables:

- `SpatialClassification`
- `SpatialExistenceKind`
- `SpatialDimensionality`
- `BoundaryKind`
- `ManifoldKind`
- `OrientabilityKind`
- `SmoothnessKind`
- `RealizationKind`
- validation hooks

Risk:

- Low/medium. Mostly additive, but conceptually important.

### Milestone 5: Physics Backend Boundary

Outcome:

```text
Current physics remains available, but Earthcall can route simulation through a backend interface.
```

Deliverables:

- `PhysicsBackend`
- `HandmadePhysicsBackend`
- law-to-backend mapping
- event logging after simulation steps

Risk:

- Medium. Must keep Law authorship above backend mechanics.

### Milestone 6: First External Math Kernel

Outcome:

```text
One selected library performs a real Earthcall-useful operation without owning the ontology.
```

Candidate first operations:

- libigl/geometry-central: curvature or remeshing on a mesh realization.
- CGAL: robust convex hull, Boolean, triangulation, or validity check.
- OpenCASCADE: exact BRep primitive, Boolean operation, or STEP import.

Risk:

- Medium/high depending on library.

## Decision Matrix

| Area | Recommended Choice | Why |
|---|---|---|
| Immediate renderer path | Renderer interface over current OpenGL | Lowest risk bridge |
| Main future renderer | bgfx | Cross-platform, Metal-capable, does not impose scene ontology |
| Apple-native backend | Metal later | Strong backend, not a world model |
| Full game engine | Defer | Too much ontology pressure |
| UI | Dear ImGui + ImGuizmo | Already present, lightweight, C++ friendly |
| Build | CMake | Needed for vendored dependencies |
| Asset import | tinygltf first | Small and practical |
| Physics | Jolt behind `PhysicsBackend` | Modern C++ runtime physics |
| Exact CAD geometry | OpenCASCADE later | Strong BRep/NURBS/topological modeling |
| Robust computational geometry | CGAL later | Strong algorithms, licensing/build complexity |
| Mesh processing | libigl or geometry-central | Good bridge before heavier kernels |

## Chosen Direction For Now

The recommended plan is:

```text
1. Keep Earthcall ontology custom.
2. Add renderer abstraction.
3. Keep OpenGL as the first backend.
4. Move build toward CMake and local vendoring.
5. Adopt bgfx as the primary renderer migration target.
6. Treat Metal as an important backend path, not the whole architecture.
7. Add geometry/physics libraries only behind Earthcall-owned interfaces.
8. Formalize SpatialClassification before importing heavy kernels.
```

This lets Earthcall grow in mathematical strength without surrendering its naming authority.

## Source Notes

The recommendations above are based on the current `sight-cpp` structure and the following public documentation:

- Apple Metal: https://developer.apple.com/metal/
- bgfx overview: https://bkaradzic.github.io/bgfx/overview.html
- Godot nodes and scenes: https://docs.godotengine.org/en/stable/getting_started/step_by_step/nodes_and_scenes.html
- Unity GameObjects: https://docs.unity3d.com/Manual/GameObjects.html
- Unreal Actors: https://dev.epicgames.com/documentation/en-us/unreal-engine/actors-in-unreal-engine
- CGAL: https://www.cgal.org/
- OpenCASCADE overview: https://dev.opencascade.org/doc/overview/html/index.html
- Jolt Physics: https://github.com/jrouwe/JoltPhysics
- Dear ImGui: https://github.com/ocornut/imgui
- sokol: https://github.com/floooh/sokol

