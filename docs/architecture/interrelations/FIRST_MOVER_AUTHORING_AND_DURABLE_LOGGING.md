# First Mover Authoring and Per-Singular Durable Logging

**Date:** 2026-09-24
**Status:** Architectural cross-check
**Related:** `docs/architecture/law/FIRST_MOVER_AUTHORING.md`, `docs/architecture/PER_SINGULAR_DURABLE_LOGGING.md`

## The Interrelation

First Mover Authoring and Per-Singular Durable Logging represent the two primary mechanisms for interacting with the foundational truth of Earthcall's universe outside the bounds of in-world Laws.

### Direct Intervention vs. Continuous Record

*   **First Mover Authoring** is the act of directly intervening in the universe. It allows authorized authors (human or LLM) to bring Objects, Relations, and Laws into existence by writing directly into Earthcall's serialization, bypassing standard in-world creation physics.
*   **Per-Singular Durable Logging** is the act of maintaining a continuous historical record. It routes semantic events into stable, identity-aware files, ensuring the engine remains a transparent glass box.

### How they relate

The intersection of these two systems is critical for maintaining ontological continuity. When a First Mover alters the state of the world directly, it introduces a discontinuity that must be accounted for in the historical record.

1.  **Recording First Mover Acts:** Any change made via First Mover Authoring (e.g., editing a `.ecform` save file directly) should ideally generate a durable log entry. While the First Mover acts outside the engine, the engine's initialization or save-loading process must recognize and log these extrinsic mutations. This ensures that a Singular being's history includes the moment it was altered by "divine intervention."
2.  **Attribution and Authorization:** First Mover Authoring relies on the `FirstMoverRegister` to authorize writes. When a write is permitted or refused by this register, that event is a prime candidate for Durable Logging. Recording these authorization events provides a history of *who* (or what agent) attempted to modify the fundamental structure of the world.
3.  **Preserving Context Across Interventions:** If a First Mover deletes or radically transforms an entity, the Durable Logging system ensures that the entity's prior history is not lost. The log survives the entity's transient or permanent destruction, providing a permanent record of its existence and the external act that ended it.

Together, they form a complete picture of causality: First Mover Authoring provides the capacity for external, unconstrained causality, while Durable Logging ensures that even unconstrained causality leaves a legible, persistent footprint.
