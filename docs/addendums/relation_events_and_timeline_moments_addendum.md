# Integrating Relation Events and Timeline Moments

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../architecture/events/RELATION_EVENT_SYSTEM.md` (The system that automatically triggers `RelationCreatedEvent` when new relations are formed)
*   `../architecture/ontology/TIME_AND_MOMENT.md` (The ontology defining `Timeline` as a first-class `Singular` that contains `Moment`s, where `Event` inherits from `Moment`)

---

## The Interrelation

The `RELATION_EVENT_SYSTEM.md` defines how the creation of a new `Relation` (e.g., "friend", "owns", "at") between `entityA` and `entityB` is published to the `EventBus` as a `RelationCreatedEvent`. Crucially, this event structure contains a `timestamp` field.

Simultaneously, `TIME_AND_MOMENT.md` defines that temporal concepts are not hidden engine mechanisms but explicitly authored `Singular`s. A `Timeline` owns a collection of `Moment`s, and an `Event` is a distinguished subclass of `Moment`.

These systems interrelate because the instantaneous publication of a `RelationCreatedEvent` is the exact catalyst for minting a new `Moment` on an authored `Timeline`.

### Integrating Events into Ontological History

When a `RelationCreatedEvent` is fired, it signifies that a new fact has entered the Ourverse. Because any `Singular` can own a `Timeline`, `entityA` and `entityB` may each possess their own temporal domains.

By combining these two architectures, we establish that a `RelationCreatedEvent` on the Event Bus can be intercepted by an authored Law, translated into an ontological `Event` (which is a `Moment`), and permanently inscribed onto the `Timeline` of the involved entities.

The `timestamp` provided by the `RelationCreatedEvent` becomes the exact coordinate of that `Moment`. This ensures that a Person's history—who they befriended, what they built, where they traveled—is not an ephemeral engine log, but a durable, navigable timeline constructed explicitly from the semantic relations they have forged.
