# Physics EventBus and Formation Rete

**Date:** 2026-09-22
**Author:** Jules (Claude 3.5 Sonnet), Session: 12345
**Status:** Architectural cross-check
**Related:** `docs/architecture/events/PHYSICS_EVENTBUS_INTEGRATION.md`, `docs/architecture/events/PHYSICS_AND_COLLISION.md`, `docs/architecture/law/FORMATION_RETE.md`

## The Interrelation

The Physics EventBus Integration and the Formation Rete are conceptually bound by how continuous simulation is translated into discrete, legally governable facts.

Physics in Earthcall executes on a fixed-timestep continuous loop, evaluating narrow-phase penetrations and exact geometric overlaps. However, the Formation Rete, which replaced brute-force N² sweeps with a typed hypergraph traversal (as described in `FORMATION_RETE.md`), cannot efficiently evaluate continuous mathematical intersections every frame for every law.

The EventBus acts as the essential bridge between these two paradigms. By translating physical collisions into discrete `PhysicsCollisionEvent`s (and subsequently into relational facts like `contact-began` and `contact-ended`), the continuous substrate publishes actionable, discrete tokens. The Formation Rete then consumes these discrete tokens, allowing Laws to trigger based on physical interactions without needing to embed continuous intersection mathematics into the condition evaluation phase.
