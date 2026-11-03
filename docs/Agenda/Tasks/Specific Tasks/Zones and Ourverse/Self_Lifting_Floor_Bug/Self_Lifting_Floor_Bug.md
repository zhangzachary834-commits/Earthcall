# Self-Lifting Floor Physics Bug

**Status**: Resolved and guarded (2026-08-14)  
**Related**: `World.cpp`, `Ourverse.cpp`, `Physics.cpp`, `scratch/probes/coincident_spawn_probe.cpp`, `tests/ground_plane_test.cpp`

## Root Causes and Fixes
1. **Unchecked List Index Fallback**:
   - `World::update` fell back to `groundIdx = 1` when no entity had `baseline=ground`. Because Zone worlds initialize empty, the second spawned entity was automatically designated as the ground. Fixed by requiring an explicitly tagged ground or defaulting to the `y=0` plane. Applied matching fix to `Ourverse::onUpdate`.
2. **Transform Origin vs. Surface Support Offset**:
   - `Physics::integrate` clamped object origins to `groundY` (floor top surface). As a result, objects buried half their height into the floor, and the floor itself was lifted half a height per substep. Fixed by referencing bottom contact via `supportOffset` (from collision zone) and marking the ground plane as kinematic.
3. **Collision Exclusion by Index**:
   - Collision loop skipped index `i == 1` assuming it was ground. Fixed by filtering on the `baseline` attribute instead of index.

## Verification
- Diagnosed via `scratch/probes/coincident_spawn_probe.cpp`.
- Guarded by `tests/ground_plane_test.cpp` (6 checks pass; verified to fail on previous codebase).

## Open Sub-issue
- In `CursorSnap` placement, `Tool::ShapeGenerator3D` retains stale `cursorHitPos` when pick ray misses rather than refusing.
