# Addendum: Physics EventBus and Interaction as Law

## Interrelated documents
- [Physics EventBus Integration](../architecture/events/PHYSICS_EVENTBUS_INTEGRATION.md)
- [Interaction as Law](../architecture/law/INTERACTION_AS_LAW.md)
- [Physics EventBus and Formation Rete](../architecture/interrelations/PHYSICS_EVENTBUS_AND_FORMATION_RETE.md)

## The architectural connection

Physics and pointer interaction both expose *observations* that authored Laws may respond to, without giving the sensing subsystem authority to decide what those observations mean. This is a shared **Sense → Decide → Act** principle, not a claim that all event types or processing stages are identical.

**Physics sensing:** `Physics::updateBodies` retains hard-wired collision detection and response. On detected overlap it publishes a typed `PhysicsCollisionEvent` to `Core::EventBus`, and also a string-named `ECA::Event{"collision", a, b, ...}`. The latter is a **contact level**, emitted while overlap persists; `contact-began` is the false-to-true edge, and `contact-ended` marks separation. A Law intended to act *once per contact* should use the appropriate edge, not repeatedly spawn on the level.

**Interaction sensing:** `InteractionChannel` reports pointer/keyboard readings and publishes addressed ECA edges such as `object-clicked` and `object-hover-entered` when their gesture conditions are met. The channel does not decide that an Object is a button or what clicking it should do. A Law can condition on authored categories and properties, then publish or perform an authored `ActionModel` act. An addressed interaction requires the pointing Person; a missing Person is not an anonymous click.

## The boundary that must remain visible

- A typed C++ `PhysicsCollisionEvent`, a named `ECA::Event`, a maintained contact `Relation`, and a Law trigger are **distinct representations**. The code contains explicit publication/translation seams; a common EventBus does not erase their different payloads, lifetimes, or semantics.
- Physics remains responsible for geometry, collision response, and its observation cadence. Laws govern authored reactions; they do not retroactively replace the collision solver.
- Relation maintenance can make contact facts available to authored predicates. Formation Rete narrows relevance over maintained relation/property state; it must **not** be described as directly consuming raw collision tokens without a separately implemented and tested bridge.
- Pointer hover/click edges and physics contact levels have different temporal contracts. A level is not an onset edge, and a held overlap must not be narrated as a fresh collision every frame.

**Scope of this addendum:** an architectural cross-check of existing source and documentation, not a new end-to-end integration, performance result, or assertion that every modality is already translated into the same Law event vocabulary. The No Black Box requirement is served by making the seams and their limits legible, not by claiming they have vanished.
