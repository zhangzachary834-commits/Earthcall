# OntoMath Rungs 7–10 inert scaffolding — 2026-10-01

Status: **SCAFFOLD ONLY — NOT WIRED, NOT COMPLETE, NOT CI EVIDENCE.**

This file prepares the already-authorized boundaries in the Linear Algebra & Transform Sovereignty ladder while Rung 6 awaits exact-head testimony. It deliberately adds no competing mathematics, changes no production call site, and does not advance rung status.

## Ground rules

- Rung 6 remains the first incomplete rung.
- No scaffold below is permission to bypass Rung 6.
- GLM may remain runtime representation/backend machinery; it may not originate mathematical meaning.
- Existing OntoMath primitives are reused before any new primitive is considered.
- Future witnesses must be wired only when their production seam is actually migrated.
- Frozen GLM expressions may appear in tests solely as independent legacy/reference oracles.

## Rung 7 — physics / collision / raycast / picking

Existing OntoMath surface already provides the core semantic primitives:
- `transformPoint`
- `transformDirection`
- `transformNormal`
- `inverseAffine`
- `matrixTranspose` where genuinely general matrix transpose is required

Planned production seams, unchanged from the authoritative ladder:
- `CollisionDispatcher.cpp`
- `ObjectCollision.cpp`
- `ObjectRaycast.cpp`
- `ObjectEvents.cpp`
- interaction/cursor unprojection paths

Witness scaffold to create when migration begins:
- `tests/singularity/ontomath_physics_transform_parity_test.cpp`

Witness sections:
1. collision normal parity;
2. raycast hit parity;
3. transformed-SDF signed-value parity;
4. picking parity;
5. non-uniform-scale collision parity;
6. explicit singular-transform refusal.

No GJK/support/collision-policy logic moves into OntoMath.

## Rung 8 — camera / renderer / shader

The ladder explicitly requires OntoMath mathematical ownership of:
- look-at/view construction;
- perspective projection;
- view-projection composition;
- inverse view-projection;
- model transforms;
- normal matrices.

Current direct camera origins confirmed in `PersonPerspective.cpp`:
- `glm::lookAt`
- `glm::perspective`

API names are intentionally **not declared yet**: clip-depth convention and projection parameter contracts must be frozen from the live renderer boundary before implementation so scaffolding does not accidentally invent semantics.

Witness scaffold to create when migration begins:
- `tests/singularity/ontomath_camera_renderer_parity_test.cpp`

Witness sections:
1. view parity;
2. projection parity;
3. world -> clip parity;
4. clip -> world parity;
5. native-resolution image parity;
6. SDF normal parity;
7. camera-motion parity.

GPU buffer layout, upload, batching, draw order, and pipeline choice remain renderer-owned representation/modality concerns.

## Rung 9 — quadric / remaining custom matrix algebra

Current semantic origin confirmed in `SmoothSurface.cpp::Quadric`:
```
Q' = transpose(M) * Q * M
```

The existing matrix-form <-> `ScalarForm` bridge must remain mathematically connected; this rung must not create a second quadric engine.

Witness scaffold to create when migration begins:
- `tests/singularity/ontomath_quadric_equivalence_test.cpp`

Witness sections:
1. sphere;
2. ellipsoid;
3. cylinder;
4. cone;
5. paraboloid;
6. translated quadric;
7. matrix-form <-> ScalarForm equivalence;
8. gradient/normal parity;
9. ray-intersection parity.

No new quadric API is declared by this scaffold because the exact ownership boundary should be derived from the existing `ScalarForm` conversion contract when Rung 9 opens.

## Rung 10 — GLM semantic quarantine

Future guard:
- `tests/singularity/ontomath_math_sovereignty_test.cpp`

The guard will inspect production source for unauthorized semantic-origin calls to:
- `glm::inverse`
- `glm::transpose`
- `glm::determinant`
- `glm::translate`
- `glm::rotate`
- `glm::scale`
- `glm::lookAt`
- `glm::perspective*`

Narrow allowlist categories only:
1. OntoMath numerical backend;
2. OntoMath <-> GLM representation adapter;
3. renderer API-layout conversion that originates no mathematical meaning;
4. independent reference-oracle tests.

The guard must not ban `glm::mat4` storage itself.

## Wiring gate

None of these future witnesses or guards is added to CMake/CI by this scaffold. Wiring occurs rung-by-rung only after the corresponding production migration exists and its exact semantic contract is frozen.
