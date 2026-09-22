# Legacy Gizmos Migration Plan

This document preserves the knowledge and behavior of the old hard-coded legacy UI gizmos (e.g., gold cubes, blend rails, facebrush UI cursors, polyhedron previews) so that they can be systematically migrated into Earthcall's Law and Entity systems.

## The Principle

In accordance with the 6 Refusals, **no subsystem defines what a thing IS**. Hardcoded OpenGL primitives used for UI cursors, blend rails, or preview bounding boxes are a violation of the Law system because they bypass the universal properties and authorities that govern all other objects.

To achieve parity without violating the architecture, these "gizmos" must be re-implemented as native **Objects**, composed into **Formations**, bound by **Relations**, and driven by **Laws**. 

### 1. Preview Objects
**Goal:** The Preview Object needs to look and behave *exactly* like the old Preview Objects (e.g., Polyhedron previews, Brush Create wireframes), but must do so via Laws.
- **Current State:** Handled via hardcoded GL calls in the C++ rendering path.
- **Migration Path:**
  - Create a designated First Mover/System `Object` representing the cursor/preview.
  - Its geometry and rendering states (like wireframe, color, opacity) must be strictly governed by `PropertyPath` properties (`material.wireframe`, `material.color`, etc.).
  - A `Law` will continuously bind the Preview Object's `transform` to the current `Cursor` intersection point in the active `World` or `Zone`.

### 2. Blend Rails & Manipulation Gizmos (Gold Cubes)
**Goal:** The spatial widgets used to scale, rotate, and morph objects must be real physical entities in the world.
- **Current State:** Hardcoded "Gold Cubes" and "Blend Rails" drawn over the world.
- **Migration Path:**
  - The UI widgets will be authored `Objects` added to a UI/Cursor-specific `Zone` or `Formation`.
  - Interaction with these widgets (clicking and dragging the gold cubes) will emit standard interaction events.
  - A `Law` will interpret these interactions and apply corresponding geometric scaling/rotation transformations to the selected `Object`.

### 3. Facebrush UI Cursors
**Goal:** 3D painting and surface interactions should be visualized through Native Objects mapping to the surface normal.
- **Current State:** Hardcoded cursors that snap to face normals during `FacePaint` mode.
- **Migration Path:**
  - Create a "Decal" or thin `Object` governed by a `Law`.
  - The `Law` will read the surface normal of the raycast intersection and orient the Facebrush cursor object to match that normal dynamically.
