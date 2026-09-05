# Comprehensive Audit of Earthcall's Entire Rendering Pipeline

**Author**: Jules (Software Engineer Agent)
**Date**: 2026-09-03
**Timestamp**: 2026-09-03T07:10:00Z
**Scope**: Entire Earthcall Rendering Pipeline Substrate (`src/Singularity/Screen/`, `src/Singularity/Core/EngineRender.cpp`, `src/ConstructedBeing/Singular/Object/ObjectRender.cpp`, `src/ConstructedBeing/Singular/Object/Geometry/Sdf*`, `src/Singularity/FirstMoverOntology/Legacy/DesignSystem.cpp`, and associated GPU/WebGPU/GL layers)

---

## 1. Executive Summary & Philosophy Alignment

Earthcall is a Person-centered ontology that orders the engine attached to it. The rendering pipeline is not an autonomous "graphics engine" that defines domain nouns or dictates what beings are; rather, it operates as a **Modality Channel** under `Singularity/` (`ScreenChannel`), translating Person-authored OntoMath expressions, geometry properties, and material definitions into visible manifestations on the hardware substrate.

### Core Architectural Mandates & Seven Refusals Check
1. **Refusal #1 (No domain C++ class for a domain noun)**: **FULL COMPLIANCE.** There are no `TreeRender`, `RobotMesh`, or `VehicleShader` classes in C++. All visual representations are authored as properties (`shape`, `artStyle`, `materialId`, `color`, `fieldNode`, `patchData`, `polyhedronData`) attached to generic `Object` or `BodyPart` instances.
2. **Refusal #2 (No top-level subsystem folder)**: **FULL COMPLIANCE.** The graphics layer lives strictly inside `src/Singularity/Screen/` (with backend concrete implementations in `src/Singularity/Screen/GL/` and `src/Singularity/Screen/WebGPU/`).
3. **Refusal #6 (No Black Box)**: **FULL COMPLIANCE.** The GPU substrate's internal metrics and governing knobs are fully exposed through `ScreenChannel` (`@screen-channel.enabled`, `@screen-channel.backgroundColor`, `@screen-channel.wireframe`, `@screen-channel.heightGridDdaEnabled`, `@screen-channel.drawCalls`, `@screen-channel.trianglesDrawn`, `@screen-channel.vramAllocatedBytes`, `@screen-channel.uniformBytesWritten`, `@screen-channel.bufferSuballocations`, `@screen-channel.pipelineSwitches`, `@screen-channel.cachedMeshesCount`). Metrics are registered as read-only computed properties, while drive controls are writable properties governed by Laws and Persons.
4. **Refusal #7 (No hardcoded methods for variable behavior)**: **FULL COMPLIANCE.** Object visual transformations, lighting reactions, and material responses are driven by Person-authored Laws and OntoMath expressions. Render materials diverge per stroke (`ownMaterial()`) to preserve authoring isolation.

---

## 2. Core Architectural Subsystems & Pipeline Walkthrough

The rendering pipeline comprises ~8,000 lines of high-performance C++ / WGSL code across 34 core files.

```
                  +-----------------------------------+
                  |        Engine::render()           |
                  |   (src/Singularity/Core/)         |
                  +-----------------+-----------------+
                                    |
          +-------------------------+-------------------------+
          |                                                   |
          v                                                   v
+-----------------------+                           +-------------------+
|    ScreenChannel      |                           |   ShadingSystem   |
| (Law/Property Sync)   |                           | (World Light Pos) |
+-----------------------+                           +-------------------+
          |                                                   |
          +-------------------------+-------------------------+
                                    |
                                    v
                     +-----------------------------+
                     |    currentRenderer()        |
                     | (Renderer Base Interface)   |
                     +--------------+--------------+
                                    |
            +-----------------------+-----------------------+
            |                                               |
            v                                               v
+-----------------------+                       +-----------------------+
|    OpenGLRenderer     |                       |    WebGpuRenderer     |
| (Legacy / Immediate)  |                       | (wgpu-native / WGSL)  |
+-----------------------+                       +-----------+-----------+
                                                            |
                                            +---------------+---------------+
                                            |               |               |
                                            v               v               v
                                     +------------+  +------------+  +------------+
                                     |GpuBufferPool| |GpuMeshCache|  |  SdfWgsl   |
                                     |(Ring Alloc)|  |(VBO Cache) |  |(SphereTracer)|
                                     +------------+  +------------+  +------------+
```

### 2.1 Top-Level Frame Orchestration (`src/Singularity/Core/EngineRender.cpp`)
- **Viewport & Perspective**: Computes framebuffer dimensions (`glfwGetFramebufferSize`), calculates aspect ratio, and constructs depth-aware projection matrices (`glm::frustumZO` for WebGPU depth range `[0,1]`, `glm::frustumNO` for OpenGL depth range `[-1,1]`). Supports FirstPerson, ThirdPerson, and SecondPerson camera modes.
- **Law-Governed Screen Parameters**: Fetches `ScreenChannel` from `LawManager` every frame. Applies `@screen-channel.backgroundColor` as the frame clear color, and toggles `@screen-channel.wireframe` and `@screen-channel.heightGridDdaEnabled` on the active `Renderer`.
- **3D Render Loop**: Iterates over all owned objects in `zone.getOwnedObjects()`. Sets model transform (`currentRenderer().setModel(obj->getTransform())`), draws object geometry (`obj->drawObject()`), and draws highlight/selection overlays (`obj->drawHighlightOutline()`).
- **Player & Nametag Overlay**: Renders Person body parts and nametag HUD when not in FirstPerson mode.
- **Screen-Space 2D Render Loop**: Filters objects where `is2D()` is true, performs stable sorting on `getZOrder2D()`, and brackets drawing with `currentRenderer().begin2D(winW, winH)` / `end2D()`. Coordinates are strictly normalized to **window points** rather than framebuffer pixels, guaranteeing Retina / HiDPI resolution independence and aligning mouse pick regions with visible UI quads.
- **Frame Telemetry Sync**: Reads `currentRenderer().frameStats()` at `endFrame()` and syncs metrics back to `ScreenChannel`.

### 2.2 Abstract Renderer Boundary (`src/Singularity/Screen/Renderer.hpp`, `Renderer.cpp`)
- Provides an explicit hardware-agnostic boundary (`Renderer` class) isolating scene logic from GPU backends.
- **Shared State**:
  - Model transform stack (`setModel`, `pushModel`, `popModel`, `currentModel()`) enabling hierarchical transform composition (used by `BodyPart` and `Formation`).
  - Recorded camera state (`setCamera`, `view()`, `proj()`, `eyePos()`, `viewport()`) for portable screen projection.
  - Directional & Positional lighting parameters (`setLight`, `setLightingEnabled`).
  - Active frame metrics (`FrameStats`).
- **Core Draw Verbs**:
  1. `drawMesh`: Explicit triangle geometry (`geom::TessMesh`).
  2. `drawImplicit`: Field evaluation via SDF (`geom::SdfNode`). WebGPU raymarches fields exactly; OpenGL falls back to tessellation.
  3. `drawSolid`: Unlit world-space geometry (gizmo handles, ghost previews, vector lines).
  4. `drawLines` / `drawOverlay`: Selection highlights, law candidate overlays, and translucent shells.
  5. `begin2D` / `end2D` / `drawTris2D` / `drawLines2D` / `drawImage2D`: Screen-space UI and pixel blitting.
  6. `uploadTexture` / `releaseTexture`: Persistent face texture management.
- **Topology Adapters (`draw::`)**: Converts legacy topologies (`GL_QUADS`, `GL_POLYGON`, `GL_LINE_LOOP`, `GL_LINE_STIPPLE`, `stb_easy_font`) into pure triangle strips and line segment pairs compatible with modern graphics APIs.

### 2.3 Legacy OpenGL Backend (`src/Singularity/Screen/GL/OpenGLRenderer.cpp/.hpp`, `GluCompat`)
- Implements the `Renderer` boundary contract over OpenGL.
- Emulates matrix stack, lighting, and immediate state through fixed-function / compatibility calls where applicable.
- `drawImplicit` tessellates SDF fields on every invocation via `geom::tessellateSdf` unless cached CPU meshes are provided.

### 2.4 High-Performance WebGPU Backend (`src/Singularity/Screen/WebGPU/`)
- **Native Context & Swapchain (`WgpuDevice.hpp`, `WebGpuContext.mm`)**: Brings up wgpu-native v29.0.1.1 device, queue, and surface instances across Apple Metal and WebAssembly platforms.
- **GPU Buffer Sub-allocator Ring (`GpuBufferPool.hpp/.cpp`)**: High-throughput sub-allocation ring buffer with 256-byte Metal alignment compliance. Sub-allocates uniform buffers, instance buffers (`SdfInstanceData`), and dynamic vertex/index buffers without per-frame GPU driver allocations.
- **GPU Mesh Cache (`GpuMeshCache.hpp/.cpp`)**: Persists VBO/IBO vertex and index handles for static and smooth meshes, tracking memory footprint in `ScreenChannel::vramAllocatedBytes`.
- **Instanced SSBO Batching**: Groups mesh draws by `MeshBatchKey` and SDF draws by `SdfInstanceData` SSBOs on `@group(1)`. Drastically collapses draw calls across duplicate geometries.

### 2.5 OntoMath-to-WGSL Codegen & Raymarching (`src/Singularity/Screen/WebGPU/SdfWgsl.cpp/.hpp`, `src/ConstructedBeing/Singular/Object/Geometry/Sdf.cpp/.hpp`)
- **Architecture**: Translates arbitrary OntoMath expression trees (`SdfNode`, `FieldNode`, `MathNode`) into compiled WGSL shader functions (`sdfEval`).
- **Structure vs. Data Separation**: Tree topology generates WGSL pipeline code (acting as pipeline cache key), while numeric constants and transform parameters stream into a parameter storage buffer (`@group(1)`). Slider drags update uniform buffers without recompiling shaders.
- **Raymarching Sphere Tracer Engine**:
  - Keinert et al. over-relaxation step acceleration.
  - Keinert & AABB planar leaps and upward early exit.
  - Secant root refinement for sub-pixel boundary accuracy.
  - Gradient step normalization (`f / |grad f|`) for non-Lipschitz authored expressions (iso-surfaces).
  - Min/Max Heightfield Grid DDA skipping (Phase C optimization) for terrain fields.
- **Parity Lock**: Fully guarded by `tests/singularity/webgpu_sdf_parity_test.cpp`, ensuring zero silhouette divergence between CPU `geom::raycastSdf` and GPU WGSL sphere tracing across 21 complex primitive/operator configurations (including Perlin noise and non-uniform scale).

### 2.6 Object Geometry & Material Resolution (`src/ConstructedBeing/Singular/Object/`)
- `Object::drawObject` dispatches based on `ShapeKind`:
  - `SmoothSurface` / `ComplexShape`: Tessellated or analytic smooth surfaces.
  - `SdfField`: Evaluated via `drawImplicit` on WebGPU (raymarched) or tessellated mesh on GL.
  - `BezierPatch` / `Polyhedron`: Multi-face mesh rendering.
  - `Shape2D`: Screen-space orthographic rendering.
- **Material Isolation (`RenderMaterial.cpp`)**: Material properties (`albedo`, `roughness`, `metallic`, `emissive`, `opacity`, `faceAlbedo`) resolve through `resolveRenderMaterial`. Painting via `Object::ownMaterial()` / `setFaceColor()` diverges the object onto its own unique material instance (`material.<identifier>`), ensuring painting on one object never repaints shared defaults.

---

## 3. Review of Recent Campaigns & Remediation History

### 3.1 August 2026 Optimization Campaign Audit (Reviewed 2026-08-31)
- **Bottleneck Clarification**: Framerate caps in late August were proven to originate from Ourverse metalaw execution costs (20-30 ms), not rendering. Consequently, changes that sacrificed visual truth for framerate were systematically reverted, while sound structural optimizations were retained.
- **Kept Work**: Instanced SDF SSBO drawing (`SdfInstanceData`), mesh batching with per-instance `baseColor`, program memoization on `(memoId, fieldRevision)`, lazy field mesh generation (`_fieldMeshDirty`), local 8-corner AABB collision zone memos, tetrahedral SDF normals (4 taps vs 6), GPU particle generation, and `PerformanceMetricsWindow` (F3).
- **Reverted Defect Substitutions**:
  1. Restored true 3D Perlin noise (`cnoise3`) in WGSL to match CPU physics/collision noise.
  2. Restored 192-iteration raymarch budget and gradient normalization (`d = f / |grad f|`) to prevent thin geometries from clipping or disappearing.
  3. Restored bounding diagonal horizon distance (`maxDim * 8.0f`) preventing terrain popping.
  4. Restored per-axis collision cell resolution floored at 2.5 units, resolving the "sliding on invisible hovering platforms" issue (Bugs.md #12 / #15).
  5. Corrected Perlin noise interval bound to $2.2 \cdot \sqrt{3}/2 = 1.905$ in `geom::evalRange`.
  6. Fixed 4.8-unit clipping distance bug on analytic SDF objects (Bugs.md #20) where `maxDist` was incorrectly clamped against eye distance rather than bounding volume depth.

---

## 4. Test Suite Coverage & Verification Matrix

The rendering substrate is guarded by 9 dedicated test suites in `tests/`:

| Test Suite | Path | Primary Subsystem Covered | Status |
|---|---|---|---|
| `webgpu_sdf_parity_test` | `tests/singularity/webgpu_sdf_parity_test.cpp` | WGSL vs CPU SDF evaluation parity (21 shapes) | **PASS** |
| `webgpu_object_test` | `tests/singularity/webgpu_object_test.cpp` | `Object` draw dispatch, face albedo, field rendering | **PASS** |
| `webgpu_sdf_distance_test` | `tests/singularity/webgpu_sdf_distance_test.cpp` | Distance-based field raymarching & maxDist clipping | **PASS** |
| `gpu_mastery_test` | `tests/singularity/gpu_mastery_test.cpp` | Sub-allocator ring buffer & uniform streaming | **PASS** |
| `webgpu_particle_test` | `tests/singularity/webgpu_particle_test.cpp` | Shader-based vertex xorshift particle pipeline | **PASS** |
| `webgpu_heightfield_sweep_test` | `tests/singularity/webgpu_heightfield_sweep_test.cpp` | Min/Max DDA grid skipping acceleration | **PASS** |
| `webgpu_micro_mastery_lag_test` | `tests/singularity/webgpu_micro_mastery_lag_test.cpp` | Buffer pool allocations under sustained load | **PASS** |
| `primitive_render_test` | `tests/constructed-being/primitive_render_test.cpp` | Mesh generation & primitive tessellation | **PASS** |
| `material_render_test` | `tests/constructed-being/material_render_test.cpp` | Material resolution, face textures, paint divergence | **PASS** |

All tests compile cleanly and execute green under `ctest`.

---

## 5. Identified Open Debt, Risks, and Future Roadmap

### 5.1 Open Debt & Risks
1. **Unbound `_programCache` Growth (`WebGpuRenderer.cpp`)**: Compiled WGSL render pipelines are stored in `_programCache` keyed by `memoId`. While pipeline keys are memoized per unique topology, destroyed objects leave compiled pipelines in memory for the session duration. Recommended fix: Implement LRU eviction or reference counting tied to object destruction.
2. **2D/3D Layering Mismatch Between Draw Order and Pick Order**: `EngineRender.cpp` sorts 2D objects by `getZOrder2D()` for drawing, whereas `InteractionChannel` picks 2D elements using `pickPriority` properties. Aligning draw order and pick order across 2D/3D boundaries guarantees visual overlay matches cursor interaction in every edge case.
3. **Process Teardown Static Destruction Order (Metal / wgpu-native)**: On macOS, destroying global C++ objects during process shutdown after `main()` exits can trigger a `SIGABRT` if wgpu-native thread pools are torn down out of sequence. `webgpu_object_test` currently uses `std::_Exit(0)` to bypass static cleanup.
4. **WebGPU Line Width Limit**: Native WebGPU / Metal pipelines restrict line widths to 1px. Multi-pixel vector strokes and line glow rely on quad expansion via `drawImage2D` or `drawSolid` quads rather than GPU line rasterization width parameters.

### 5.2 Strategic Roadmap
1. **Finalize Full WebGPU Switch (Phase F / Flip)**: Complete transition away from legacy OpenGL binaries, making `earthcall_webgpu` the sole production driver across desktop and WASM web targets.
2. **OntoMath WGSL JIT Acceleration**: Expand `SdfWgsl` codegen into a full OntoMath shader compiler capable of executing Person-authored procedural materials, animated surface displacements, and dynamic light fields directly on GPU compute units.
3. **Hi-Z Occlusion Culling Pre-Pass**: Implement hierarchical z-buffer (Hi-Z) occlusion culling for complex zones with dense interior geometry, skipping fragment evaluation for occluded objects prior to the main render pass.

---

**Audit Sign-off**:
Jules (Software Engineer Agent)
Session: `2026-09-03_rendering_pipeline_audit`
Earthcall Codebase Master Branch `HEAD`
