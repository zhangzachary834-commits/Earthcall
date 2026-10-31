# Addendum: Integrating Person Events, Event Handlers, and the Second Person Framework

## Events report activity; they do not grant standing

The [Person Events System](../architecture/events/PERSON_EVENTS_SYSTEM.md) describes events for creation, login, logout, and Zone joining, including `PersonCreatedEvent`, `PersonLoginEvent`, and `PersonJoinedEvent`. Its examples subscribe **directly to the `EventBus`**. The [Event Bus vs Event Handler](../architecture/events/EVENT_BUS_VS_EVENT_HANDLER.md) document offers `EventHandler` as an optional way to organize named callbacks, not a required central authority for Person state changes.

### Delivery is not authorization

Publishing or handling `PersonJoinedEvent` can notify subscribers about a join; it does **not** itself prove that admission, consent, jurisdiction, or conflicting Laws were checked *before* the state changed. Such guarantees must be enforced at the relevant mutation/admission boundary. An after-the-fact callback cannot substitute for an authorization gate, and callback registration order must not silently decide a Person's rights.

### The Second Person boundary is partly specified, not shipped

The [Second Person Framework](../architecture/ourverse/SECOND_PERSON_FRAMEWORK.md) specifies future Person-to-Person visibility, representation, and equal-authority conflict mechanisms. It explicitly says those three mechanisms are **not yet implemented**; the at-least-one Primary Home admission invariant is a separate live guarantee. Its proposed conflict ladder is jurisdiction, authored covenant, then loud mutual refusal, with unresolved **AUTHOR** decisions. An `EventHandler` does not implement that ladder or the required signed, revocable Person–Law consent.

The integration path is therefore **validate a proposed change at its authoritative boundary, then publish an attributable event about the result, then let subscribers react**. The Person Events document describes intended event behavior; neither it nor this addendum is an independent runtime verification of every event emission. Preserve the distinction between documentation, implemented safeguards, and future constitutional design.

---

**Interrelated Documents:**
* [Person Events System](../architecture/events/PERSON_EVENTS_SYSTEM.md)
* [Event Bus vs Event Handler](../architecture/events/EVENT_BUS_VS_EVENT_HANDLER.md)
* [Second Person Framework](../architecture/ourverse/SECOND_PERSON_FRAMEWORK.md)
