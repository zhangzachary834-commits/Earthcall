# Physics and Collision Architecture in Earthcall

**Document Status**: Authoritative Architecture Specification  
**Scope**: Physics Subsystem, Collision Detection (Broad/Narrow Phase), Fixed Timestep Loop, Geometry Caching, and Law-Governed Mechanics.

---

## 1. Philosophical Grounding & Ontological Boundaries

In Earthcall, physical laws and collision responses are not sovereign authorities over what a being *is*; they are Sense-Act mechanisms and First Mover Laws that govern continuous physical state (`velocity`, `position`) and publish relational facts (`contact-began`, `contact-ended`) for Person-authored Laws.

- **Refusal #1 & #6 Compliance**: Collision shapes are properties of `Object` beings exposed via `PropertyPath`.
- **First Mover Mechanics**: Hardcoded physics (gravity, kinematics, acoustic triggers) operate as `FirstMoverLaw` instances registered in `Physics::createDefaultPhysicsLaws()`.
- **Relational Grounding**: Contacts between beings are published as events onto the `EventBus` and registered into the active Zone's relation graph (`Formation::relations`).

---

## 2. Fixed Timestep Simulation Loop

To prevent temporal instability, step doubling, and jagged physics behavior under variable frame rates, `Zone::update` operates on a deterministic fixed-timestep accumulator:

```
Accumulator += dt
While (Accumulator >= FIXED_DT && steps < MAX_STEPS_PER_FRAME):
    Physics::updateBodies(FIXED_DT)
    Accumulator -= FIXED_DT
    steps++
If Accumulator > MAX_STEPS_PER_FRAME * FIXED_DT:
    Accumulator = 0  // Drop excess time under sustained load rather than freezing
```

- **Constants**: `FIXED_DT = 1.0f / 60.0f` (16.67 ms), `MAX_STEPS_PER_FRAME = 3`.
- **Ontological Implication**: The simulation world clock (`Universe::instance().setClock`) remains monotonic and stable, decoupling world physics integration from display render frame jitter.

---

## 3. Collision Pipeline

The collision subsystem in `src/ZonesOfEarth/Physics/` and `src/ConstructedBeing/Singular/Object/` consists of three stages:

### A. Broad-Phase: 1D Sort-and-Sweep (Sweep and Prune)
The previous naive all-pairs ($O(n^2)$) loop recomputed AABBs in the inner loop, producing quadratic scaling ($n^{1.75}$) that dominated frame time at 128+ objects.

The broad-phase pipeline now:
1. **Precomputes Extents**: Evaluates world-space AABB min/max bounds and law eligibility masks once per object.
2. **Sorts on Primary Axis**: Sorts candidate objects by `AABB.min.x`.
3. **Sweeps with Early Pruning**: Iterates through sorted pairs, immediately terminating inner evaluation when `candidate.min.x > current.max.x`.
4. **Axis Overlap Verification**: Checks Y and Z intervals before dispatching to narrow-phase algorithms.

This reduces broad-phase complexity to $O(n \log n + k)$ where $k$ is the number of overlapping pairs, achieving near-linear scaling ($n^{1.10}$) on standard scenes.

### B. Narrow-Phase: GJK & EPA
For candidate overlapping pairs:
1. **Convex Polyhedra & Smooth Quadrics**: Uses Gilbert-Johnson-Keerthi (GJK) distance computation followed by Expanding Polytope Algorithm (EPA) for penetration depth and contact normal extraction (`CollisionDispatcher.cpp`).
2. **Analytic Quadric Support**: Quadric smooth surfaces (spheres, ellipsoids) compute support points analytically via `geom::supportPoint` rather than scanning large vertex meshes.
3. **Support Cloud Decimation**: Complex SDF and patch surfaces use a strided, decimated support cloud (capped at 256 points) during narrow-phase argmax evaluation, eliminating $O(n 	imes 	ext{mesh})$ bottlenecks.

---

## 4. Tessellation Cache & Memory Stewardship

Geometry tessellation (`geom::tessellateSmooth`) is computed on shape mutation rather than during draw or physics update.

- **Content-Addressed Cache (`s_smoothCache`)**: `ObjectCollision.cpp` maintains a static map keyed on `SmoothSurfaceData`. Identical smooth primitives share a single `std::shared_ptr<geom::TessMesh>`, enabling instanced draw collapsing in the renderer.
- **Reference-Counted Garbage Collection**: `Object::gcSmoothTessellationCache()` sweeps `s_smoothCache` and evicts any entry with `use_count() <= 1` (where no active `Object` holds a reference). This prevents memory accumulation during continuous interactive shape edits.

---

## 5. Event Publishing & Law Integration

Collisions translate physical contact into legible ontological facts:
- **Edge Transitions**: Transition from non-touching to touching emits `contact-began`. Transition from touching to separated emits `contact-ended`.
- **Continuous Logic**: Continuous contact state is queried via `isTouching(other)`.
- **Acoustics & Reactions**: The `physics-acoustics` first-mover law listens to `contact-began` to spawn dynamic sound emitter concepts in-world.
