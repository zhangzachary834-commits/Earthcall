# Interaction-as-Law and the Physics EventBus

**Date:** 2026-10-28  
**Status:** Architectural cross-check; not a new runtime integration  
**Sources:** `docs/architecture/law/INTERACTION_AS_LAW.md`; `docs/architecture/events/PHYSICS_EVENTBUS_INTEGRATION.md`

## The shared boundary

Both designs separate **sensing** from **authored decisions**. The Interaction Channel senses pointer, wheel, and keyboard input, resolves the addressed Singular, exposes registered readings, and publishes events. Laws decide what those observations mean for an authored control. The physics system independently detects physical collisions and can publish `Physics::PhysicsCollisionEvent` notifications to `Core::EventBus` for subscribers.

The two channels can share the EventBus without becoming the same kind of event. In the existing interaction design, `object-clicked` means a press and release on the same addressed being within the authored click-slop threshold. It is **not** defined as a rigid-body collision, nor does it require GJK/EPA or a physically instantiated cursor. Keyboard and wheel input also do not require collision geometry.

## Architectural interrelation

1. **First-mover sensing:** Input hardware and physics collision detection each report facts they are qualified to observe; neither determines that a target is a button.
2. **Legible events:** The Interaction Channel publishes its gesture edges with Singular participants; physics may publish its own collision events. Consumers must respect each event's actual type and semantics.
3. **Authored response:** A Law can respond to `object-clicked` or read `@interaction-channel.*` / `@world.*` readings. A physics-event-to-Law adapter would require an explicit, tested bridge rather than an assumed automatic conversion.
4. **No parallel UI ontology:** A control's meaning is its authored properties, categories, relations, and Laws; ordinary physics objects may also participate when that behavior is genuinely authored.

**Implementation boundary:** The documented Interaction Channel is built and stepped from `Engine::update`; the source documents do not establish that every UI gesture passes through `PhysicsCollisionEvent`, that physics automatically dispatches Laws, or that a complete collision-to-control bridge is installed. Test each bridge through its real caller before claiming end-to-end integration.

**Original intake:** Jules PR #812; scope and implementation claims reconciled against the source architecture.
