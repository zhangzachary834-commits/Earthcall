# Rendering Optimization Mechanism Audit

**Date:** 2026-09-18
**Agent:** Antigravity (Session ID: 6698ec17-9fa6-4308-8a38-fb54f3ad1812)

## Goal
Audit the hardcoded rendering optimization mechanisms within Earthcall's C++ substrate that violate Refusal #6 (No Black Box) and prepare them for migration to authored Law.

## Findings

### 1. The `RenderMode` Fallback Mechanism (Sense/Decide/Act Violation)
**Location:** `src/ConstructedBeing/Singular/Object/ObjectRender.cpp`

Currently, `ObjectRender.cpp` fuses the Sense, Decide, and Act phases for rendering optimization:
```cpp
if (r.rendersImplicitExactly() && _renderMode != RenderMode::Mesh) {
    r.drawImplicit(...);
} else {
    r.drawMesh(...);
}
```
- **Sense:** `r.rendersImplicitExactly()` (a C++ only virtual method query).
- **Decide:** If the renderer supports exact marching and the mode isn't explicitly `Mesh`, choose implicit over tessellated meshes.
- **Act:** Execute the draw call.

This is a black box. A Person cannot govern this fallback logic nor inspect why the mesh was bypassed.

### 2. Field Geometry Caching Bounds
**Location:** `src/ConstructedBeing/Singular/Object/ObjectCollision.cpp`

The lazy evaluation mechanism `rebuildFieldMesh()` enforces strict resolution bounds using hidden C++ `constexpr` constants:
```cpp
static constexpr int   kMinRes   = 24;
static constexpr int   kMaxRes   = 128;
static constexpr float kMaxCells = 2200000.0f;
```
While these protect the engine from OOM conditions (and prevent lopsided plateau bugs on terrain), they are entirely opaque. A Person cannot tune these bounds based on the host platform's capabilities or the specific Zone's requirements, nor can other systems read them to predict mesh fidelity.

## Conclusion
Both of these mechanisms represent "first mover" code that successfully got the engine running but now represent a black box. Following the `LAW_MIGRATION_FRAMEWORK`, they must be dismantled:
1. Expose the "Sense" mechanisms (`rendersImplicitExactly`, `minRes`, `maxRes`, `maxCells`) as Legible properties.
2. Delegate the "Decide" mechanisms to Authored Seed Laws.
