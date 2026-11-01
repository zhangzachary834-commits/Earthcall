# Geometry Execution and Event Bus

## Interrelated Documents
- `docs/architecture/mathematics/GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md`
- `docs/architecture/events/PHYSICS_EVENTBUS_INTEGRATION.md`
- `docs/architecture/events/PHYSICS_AND_COLLISION.md`

## The Interrelation

The `GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md` establishes a strict hierarchy where mathematical forms (geometry execution substrate) exist solely to lower the authored ontology into execution. It asserts that "Execution tags must never flow upward and silently become authored meaning."

Meanwhile, the `PHYSICS_EVENTBUS_INTEGRATION.md` and `PHYSICS_AND_COLLISION.md` detail how physical collisions (determined by that very geometry execution substrate) are broadcast over the `EventBus` to notify other systems.

These concepts intersect at the boundary between mathematical simulation and relational logic. While geometry itself is a lower-level construct with no inherent ontological meaning, *contact* between geometrical bodies is intensely meaningful in the world of Laws. The EventBus acts as the translation layer. It listens to the hard-coded execution of geometry (like GJK and EPA collision detection) and transforms those mechanical overlaps into abstract events (`PhysicsCollisionEvent`).

## Thoughts on Integration and Interrelation

The translation mechanism provided by the Event Bus guarantees that the core principle of the Geometry Execution Substrate is maintained. The geometry engine does not dictate relationships; it merely calculates overlaps and emissions. It is the Event Bus and subsequent listeners (like the Relation Event System or First Mover physics laws) that capture these "dumb" geometric collisions and formally publish them as Legible, ontologically valid Relations (e.g., `contact-began`, `contact-ended`).

In this way, the Event Bus protects the No Black Box and First Mover principles. It prevents physics code from silently manipulating world meaning by forcing all physical interactions to pass through a standardized, observable event pipeline before they can become world facts.
