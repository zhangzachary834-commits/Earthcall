# Restore Legacy 3D Create Tool & Author Law Version

**Status**: Completed and verified (2026-08-13)  
**Related**: `Tool::ShapeGenerator3D`, `CreationChannel`, `saves/tests/shape_generator_3d_law.json`, `Engine.cpp`

## Summary of Implementation
- Restored `Tool::ShapeGenerator3D` as a developer ImGui panel ("Developer: 3D Create Tool") that spawns directly through `CreationChannel` placement/shape/color fields, bypassing `Law::applyTo`.
- Spawned objects receive an `authored-by` relation marking `CreationChannel` as author via `Law::recordProvenance` (leveraging its standing as a registered First Mover, `isFirstMover() == true`).
- Authored separate Law version ("Tool: Shape Generator 3D", verified in `basic_cube_law_test.cpp`), seeded as `saves/tests/shape_generator_3d_law.json` and verified through the save/load pipeline.
- Law activation is bound to edge-triggered `L` key (`active3DMode`) rather than an ImGui button to prevent dual-firing.
- Resolved pre-existing bug: initialized `Engine::_lawManager`, `_player`, `_camera`, `_mouseHandler`, and `_keyboardHandler` in `Engine::initLogic()` to prevent null-pointer dereference at startup.
