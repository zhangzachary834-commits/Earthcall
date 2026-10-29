# Interaction-as-Law and Physics EventBus Integration

**Date:** 2026-10-28
**Status:** Architectural cross-check
**Related:** `docs/architecture/law/INTERACTION_AS_LAW.md`, `docs/architecture/events/PHYSICS_EVENTBUS_INTEGRATION.md`

## The Interrelation

Interaction-as-Law specifies that UI is not a separate domain. Controls are `Object` Singulars governed by normal Laws, eliminating the need for a separate widget tree or bespoke event system.

Physics EventBus Integration specifies that the continuous spatial simulation (the hard-wired physics loop) communicates with the rest of Earthcall by publishing discrete `PhysicsCollisionEvent`s to the EventBus.

### How they relate

These two systems interlock to completely eliminate the need for a traditional "UI event loop" (like DOM events, click bubbling, or synthetic mouse capture).

When a Person wishes to interact with a control (e.g., clicking a button or dragging a slider), they do not generate a "UI MouseEvent". Instead:

1.  **Spatial Embodiment:** The pointer or cursor is modeled as a physical `Object` (a ray or a tiny collision volume) within the `Zone`.
2.  **Physical Collision:** The pointer `Object` physically intersects with the control `Object`. The broad/narrow phase collision detection (GJK/EPA) natively recognizes this overlap.
3.  **EventBus Publication:** The hard-wired physics integration automatically publishes a `PhysicsCollisionEvent` to the EventBus.
4.  **Law Reaction:** The Interaction-as-Law system is simply a set of Laws listening to the EventBus. A Law might specify: *If `pointer` collides with `button`, set `button.activated = true`*.

By uniting Interaction-as-Law with the Physics EventBus, we achieve End-to-End Coherence. A click is just another physical event in the world, subject to the exact same temporal simulation, collision geometry, and Law-based evaluation as two rocks hitting each other. There is no "UI overlay"—there is only the continuous physics of the world producing discrete events that drive algorithmic Law progression.

**Author**: Jules (Claude 3.5 Sonnet, default harness, Session 8124)
