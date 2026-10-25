# Addendum: Discrete Events, Continuous Simulation, and the Event Bus

*Synthesized from the event-system architecture documents; distinguishes notification from state ownership.*

## Two time domains, one explicit boundary

Physics advances continuous state during an update. A collision detected there can publish a discrete `PhysicsCollisionEvent` to `Core::EventBus`. Subscribers may respond without being embedded in the physics solver. The event is a report of a transition; it does not replace the underlying physics state or its authoritative owner.

The EventBus is the **post office** and subscribed handlers are the **recipients**. Do not confuse decoupled producers and consumers with deferred delivery: the Person and Relation event specifications describe **synchronous** dispatch when their publishing methods are called. Whether a specific publisher queues or delivers immediately must be verified at its call site.

## Ontological events are specific, not universal

`PersonLoginEvent` is documented for a Person's login method, and `RelationCreatedEvent` for adding a new relation through `RelationManager`. Neither statement licenses the stronger claim that every mutation to a Person or Relation necessarily emits an event. Direct changes, restoration, and replay require their own explicit contracts.

An observer can use these events for diagnostics or lawful reactions while retaining the distinction between a real human Person, a Relation, and the transient notification about either. The EventBus transports facts about changes; it does not author ontological identity.

## Linked references

- [Event Bus vs Event Handler](../architecture/events/EVENT_BUS_VS_EVENT_HANDLER.md)
- [Physics EventBus Integration](../architecture/events/PHYSICS_EVENTBUS_INTEGRATION.md)
- [Person Events System](../architecture/events/PERSON_EVENTS_SYSTEM.md)
- [Relation Event System](../architecture/events/RELATION_EVENT_SYSTEM.md)
