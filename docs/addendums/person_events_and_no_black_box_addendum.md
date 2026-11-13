# Integrating Person Events and the No Black Box Principle

**Status:** Conceptual integration; proposed property paths below are **not** implemented API.
**Connected documents:** [Person Events System](../architecture/events/PERSON_EVENTS_SYSTEM.md) and [No Black Box](../architecture/ontology/NO_BLACK_BOX.md).

## Existing behavior and its boundary

`Person::login`, `logout`, `joinZone`, and `leaveZone` publish typed C++ Person events and corresponding string-typed ECA events (`person-logged-in`, `person-logged-out`, `person-joined-zone`, `person-left-zone`). These are notifications about transitions, not proof that their payloads are already available as registered PropertyPaths or persisted Moments.

The current `Person::buildProperties` registers `position`, `velocity`, `name`, `cameraPos`, `cameraForward`, and `joys`. It does **not** register `session.active` or `session.currentZone`. Session and joined-Zone state also have different meanings: membership in a Zone is not, by itself, an authoritative physical location or the active Zone.

## Proposed legible state

The No Black Box refusal requires meaningful Person-governing state to be addressable, or explicitly classified as kernel mechanism. A future design could expose an appropriately authorized session-active boolean and a separately defined Zone-membership or derived-location view. Names such as `@person.session.active` and `@person.session.currentZone` are **illustrative proposals only**; no Law should rely on them until their registration, identity, read semantics, and tests exist.

Legibility is not unrestricted disclosure. A raw session identifier may be an authentication credential and must not become publicly readable merely to satisfy the refusal. Define a safe, non-secret observable projection, the authority controlling access, and the distinction between a Person's own view and other observers' views. Do not invent a second permission gate: use the documented constitutional and TransferPolicy boundaries.

A complete implementation needs property registration, transition-to-property consistency tests, Law-facing event witnesses, and an explicit persistence contract where durability is intended. Event publication alone proves none of these.
