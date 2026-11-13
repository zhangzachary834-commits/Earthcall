# Performance as Truth and physics EventBus integration

**Companions:** [Performance as Truth](../architecture/ontology/PERFORMANCE_AS_TRUTH.md), [Physics EventBus Integration](../architecture/events/PHYSICS_EVENTBUS_INTEGRATION.md), and [Physics EventBus and Formation Rete](../architecture/interrelations/PHYSICS_EVENTBUS_AND_FORMATION_RETE.md).

Performance as Truth asks computation to follow the actual structure and continuity of authored beings rather than duplicating work in disconnected representations. Physics illustrates a narrower engineering seam: simulation and collision detection retain their direct calculation path, while `PhysicsCollisionEvent` publishes observations to interested consumers through the EventBus.

**The event is a notification, not the collision solver or a Formation Rete fact.** Publication, relation-state maintenance, and Rete relevance are adjacent but distinct responsibilities. The EventBus reduces the need for every consumer to poll or hardwire itself into physics; it does not establish that simulation computation runs only when an event occurs, nor that an event is emitted only once per contact. A sustained overlap can publish repeatedly according to the physics update path.

**Design consequence:** optimize subscriptions and downstream work without weakening collision correctness or silently treating an event notification as durable authored state. Measure costs and preserve causality before claiming a performance gain.
