# OntoMath and The Event Bus

**Date:** 2024-05-18
**Status:** Architectural cross-check
**Related:** `docs/architecture/mathematics/ONTOMATH_FRAMEWORK.md`, `docs/architecture/events/README.md`

`OntoMath` is Earthcall's representation of continuous, authored mathematical meaning. It describes fields, bounds, geometries, and raster formations as equations and piecewise maps—continuous structures that exist independent of the discrete frame loop.

The `Event Bus` (`Core::EventBus`) is the system for discrete, programmatic transitions. It handles past-tense occurrences (e.g., `PhysicsCollisionEvent`, `HoverEvent`) that occur at a specific boundary in time and state.

The intersection of these two concepts highlights a fundamental architectural friction: bridging the continuous and the discrete.

OntoMath defines *what is*. A spatial bound or an SDF field doesn't "happen"; it simply *exists* as a mathematical truth across the space. The Event Bus, however, is entirely concerned with *what happened*.

When a Person's continuous movement intersects with a continuous OntoMath boundary (like a Zone bound or an SDF surface), Earthcall must collapse that continuous mathematical relation into a discrete event that systems like the `LawManager` or `Physics` can react to.

This collapse happens when the execution substrate evaluating the OntoMath explicitly emits an event on the bus. For example, the `ZoneManager` evaluating the continuous mathematical bounds of Zones will publish a discrete "entered-zone" event.

### Thoughts on Integration and Interrelation

The critical insight in their integration is that the Event Bus must never attempt to replace or approximate the continuous truth of OntoMath.

We do not use continuous events (like a per-frame "StillInsideZoneEvent") because an event is a transition, an edge. OntoMath holds the level (the continuous state of being inside). If a Law needs to know if someone is currently inside a bound, it queries the OntoMath structure directly. It only listens to the Event Bus if it needs to know the exact moment the threshold was crossed.

This separation ensures that complex geometric or temporal laws are evaluated mathematically rather than via a fragile, per-frame event storm. The Event Bus remains an algorithmic call stack for discrete transitions, while OntoMath remains the unbroken, resolution-independent truth of form and field.

*Model: Jules / Harness: Standard / Session ID: a4b9c2d1-e5f6-7a8b-9c0d-1e2f3a4b5c6d*
