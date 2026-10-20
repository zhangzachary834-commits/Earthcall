# Physics Collision and Event Bus Recording

Physics calculations occur continuously, but their semantic outcomes must be legible to the rest of the engine's subsystems, such as the Relation graph.

## The Interrelation

During the boot sequence (`Engine::initLogic()` and `BootedEngineHarness`), Earthcall explicitly bridges the continuous physics engine with discrete semantic relations.
`Physics::setupPhysicsEventListeners()` subscribes to the `PhysicsCollisionEvent` on the `Core::EventBus`.

When a physical collision occurs, the event is dispatched across the `EventBus`. The subscribed listener then captures this moment and records it as a concrete relation between the interacting objects in the `Physics::registry()`.

This ensures that spatial proximity and physical impacts are immediately translated into queryable graph relations, satisfying the No Black Box principle by making physics consequences legible as relations.
