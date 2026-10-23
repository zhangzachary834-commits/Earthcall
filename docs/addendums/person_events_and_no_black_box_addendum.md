# Integrating Person Events and the No Black Box Principle

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../architecture/events/PERSON_EVENTS_SYSTEM.md` (The system triggering events for Person activities like `PersonLoginEvent` and `PersonJoinedEvent`)
*   `../architecture/ontology/NO_BLACK_BOX.md` (The doctrine dictating that all meaningful state governing a Person must be legible and accessible as registered property paths)

---

## The Interrelation

The `PERSON_EVENTS_SYSTEM.md` establishes a suite of discrete events that the engine automatically fires when a Person changes their operational state—such as logging in (`PersonLoginEvent`) or joining a specific zone (`PersonJoinedEvent`).

Simultaneously, the `NO_BLACK_BOX.md` refusal demands that no variable governing a Person can be hidden beneath the C++ kernel. "Nobody has registered it yet" is not a valid excuse for keeping state unaddressable by Authored Laws.

These two architectures intersect directly at the concept of **Session State**.

### Exposing Session and Zone State

When a `PersonLoginEvent` or `PersonJoinedEvent` fires on the Event Bus, it implies that the engine is tracking an active session (`sessionId`) and a location (`zoneName`). If this tracking existed solely as private C++ variables (`std::string _currentSessionId` or `std::vector<std::string> _activeZones`), it would create a Black Box. The system would know where a Person is and whether they are active, but the authored world would be blind to it.

To comply with the No Black Box refusal, the state changes announced by Person Events must correspond to registered, legible property paths on the `Person` Singular (e.g., `@person.session.active`, `@person.session.currentZone`).

Because these fields are registered properties, any First Mover or Authored Law can read them to understand the Person's current engagement context. For example, a Law can check `@person.session.currentZone` to verify presence before allowing interaction with a Zone-specific Object, all without needing backdoor C++ access. The Person Events serve as the reactive triggers, while the No Black Box principle ensures the underlying state remains a transparent, governable part of the ontology.