# Rendering Optimization Migration Plan

**Date:** 2026-09-18
**Agent:** Antigravity (Session ID: 6698ec17-9fa6-4308-8a38-fb54f3ad1812)

## Goal
Implement the migration of rendering optimization mechanisms from C++ black boxes to authored Law, as identified in `docs/audits/rendering_optimization_mechanism_audit_2026-09-18.md`.

## Execution Steps

### 1. `ScreenChannel` Property Exposure (Rung 1: Legible)
Modify `src/Singularity/Screen/ScreenChannel.hpp` and `.cpp` to expose:
- `screen.rendersImplicitExactly` (boolean, computed property mapping to `currentRenderer().rendersImplicitExactly()`).
- `screen.fieldMeshMinRes` (int, default 24, mutable).
- `screen.fieldMeshMaxRes` (int, default 128, mutable).
- `screen.fieldMeshMaxCells` (double, default 2200000.0, mutable).

### 2. Displace Hardcoded Limits (Rung 3/4)
- **Geometry Bounds:** Modify `src/ConstructedBeing/Singular/Object/ObjectCollision.cpp` (`Object::rebuildFieldMesh()`). Remove the `constexpr` limits and instead read `fieldMeshMinRes`, `fieldMeshMaxRes`, and `fieldMeshMaxCells` dynamically from `ScreenChannel`.
- **Render Mode Fallback:** Modify `src/ConstructedBeing/Singular/Object/ObjectRender.cpp`. Remove the C++ decision `if (r.rendersImplicitExactly() && _renderMode != RenderMode::Mesh)`. Change it to strictly honor the `_renderMode` state: `if (_renderMode == RenderMode::Analytic) drawImplicit(); else drawMesh();`.

### 3. Establish the Seed Law (Rung 4: Displaced)
Create a first-mover Seed Law in `src/ZonesOfEarth/AuthorsOfLaw/SeedLaws.cpp` (or equivalent initial world generation code).
- The Seed Law will govern the `renderMode` responsibility for `Object`s.
- Condition: If `screen.rendersImplicitExactly` is true.
- Action: Set `renderMode` to `1` (Analytic). Otherwise, set it to `2` (Mesh).
- *Note: Since `renderMode` defaults to 0 (`Auto`), the Seed Law will explicitly move it to 1 or 2, effectively acting as the optimization decision.*

## Verification
- Compile with `cmake --build build -j8`.
- Run tests (`ctest`) to ensure no regression in `law_parity_test` or `object_render_test`.
