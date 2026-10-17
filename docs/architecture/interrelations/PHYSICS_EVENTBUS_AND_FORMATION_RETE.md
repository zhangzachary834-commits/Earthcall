# Physics EventBus and Formation Rete

**Date:** 2026-09-22
**Status:** Architectural cross-check
**Related:** `docs/architecture/events/PHYSICS_EVENTBUS_INTEGRATION.md`, `docs/architecture/events/PHYSICS_AND_COLLISION.md`, `docs/architecture/law/FORMATION_RETE.md`

## The Interrelation

Physics, EventBus publication, relation-state maintenance, and Formation Rete relevance are adjacent stages, but they are not one mechanism and should not be collapsed into one another.

Physics computes geometric interactions on its simulation cadence. The EventBus can publish discrete `PhysicsCollisionEvent` observations at that boundary. Integration code may then translate those observations into authored, law-visible state such as contact relations. That state transition is the architectural bridge from a physical observation to a fact that laws can reason about.

Formation Rete is downstream of that authored state: it narrows which Formations are relevant to a Law from their typed/property relationships and maintained indices. It should not be described as directly consuming collision-event tokens unless such an event-to-Rete channel is explicitly implemented and witnessed.

The cross-check is therefore a separation-of-responsibilities constraint: physics establishes the observation, EventBus transports it, integration maintains the appropriate authored relation/state, and Formation Rete accelerates relevance/evaluation over that state. Laws need not embed continuous intersection mathematics, but neither should the Rete be credited with semantics owned by the physics/event/relation boundary.
