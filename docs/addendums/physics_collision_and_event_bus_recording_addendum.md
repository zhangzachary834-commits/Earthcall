# Integrating Physics Collision Events and Relation Recording

`Physics::setupPhysicsEventListeners()` registers an `EventBus` listener which records `PhysicsCollisionEvent` observations in `Physics::registry()` when both endpoints exist. Engine initialization installs this listener; other harnesses do not automatically do so.

The physics registry is not interchangeable with a Person-authored Formation graph. Collision-level observations can repeat during sustained overlap; contact edges are separate. See [the source-checked interrelation](../architecture/interrelations/PHYSICS_COLLISION_AND_EVENT_BUS_RECORDING.md).