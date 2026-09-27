# Authorable Bounds and The Event Bus

**Date:** 2026-09-27
**Status:** Architectural cross-check
**Related:** `Reflections on the Substrate/When_Bounds_Are_Doctrine_And_When_They_Are_Not.md`, `events/EVENT_BUS_VS_EVENT_HANDLER.md`, `ontology/PERFORMANCE_AS_TRUTH.md`

## The Interrelation

The doctrine outlined in `When_Bounds_Are_Doctrine_And_When_They_Are_Not.md` dictates that system bounds (like `kMaxChainRounds` for law cascades) should not be opaque C++ constants. Instead, they must be authorable properties governed by MetaLaws, granting Persons granular control over the machine's limits.

When this is cross-checked with the Event Bus architecture, a critical mechanism is revealed: what happens when an authored limit is reached?

If `kMaxChainRounds` is a hardcoded C++ constant, hitting it results in a silent truncation or a console warning—both of which violate the `NO_BLACK_BOX` refusal. However, if the bound is an in-world property, the act of exceeding it must be an in-world event.

When a Law cascade hits the authored `maxChainRounds`, the engine should publish a specific event (e.g., `CausalityExhaustedEvent` or `ThermodynamicLimitReached`) to the `EventBus`. Because it is on the EventBus, other authored Laws can listen for it. A Zone could author a Law that says: "When causality is exhausted, visually fracture the Object that triggered the cascade," or "When a Being violates the computational budget, strip its active Laws."

By linking authorable bounds directly to the Event Bus, hardware and computational limits are fully integrated into the physics and ontology of the world. Performance stops being an external constraint and becomes an authorable thermodynamic reality.
