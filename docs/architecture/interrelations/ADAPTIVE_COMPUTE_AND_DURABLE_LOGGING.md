# Adaptive Compute Moments and Per-Singular Durable Logging

**Date:** 2026-09-24
**Status:** Architectural cross-check
**Related:** `docs/architecture/ADAPTIVE_COMPUTE_MOMENTS.md`, `docs/architecture/PER_SINGULAR_DURABLE_LOGGING.md`

## The Interrelation

Adaptive Compute Moments and Per-Singular Durable Logging both address the concept of "time" in Earthcall, but from complementary perspectives: Execution Time and Historical Time.

### Execution Time vs. Historical Time

*   **Adaptive Compute Moments** governs the *budgeting of present execution time*. It ensures that processes whose value increases when they are allowed to improve slowly (like relevance discovery and cache building) are given bounded computational opportunities without causing frame drops.
*   **Per-Singular Durable Logging** governs the *persistence of historical time*. It ensures that the semantic events and state transitions occurring within the engine are recorded in stable, identity-aware histories rather than being lost in a transient console stream.

### How they relate

The relationship between these two systems centers on the legibility and predictability of background work.

When Adaptive Compute Moments defers or slowly processes a task (like building a Prophetic Rete), this creates a gap between an event happening and its full consequences being realized. Durable Logging provides the necessary transparency into this process.

1.  **Logging Budget Exhaustion:** When the ComputeScheduler suspends a process because its temporal budget is exhausted, this suspension is a critical semantic event. Durable Logging should record these suspensions so that a Singular entity's history reflects *why* a particular piece of derived state was not immediately updated.
2.  **Tracking Deferred Execution:** If a Law is partially evaluated and then paused, the durable log provides the continuity. It bridges the gap between the initial trigger and the final execution, ensuring that the No Black Box principle is maintained even when work is spread across multiple frames.
3.  **Observability of Slow Adapters:** The slow adaptation processes themselves (like spatial hierarchy building) need to log their progress and state. By routing these logs through the Per-Singular Durable Logging system, we can correlate the background compute efforts with the specific entities or Zones they are serving.

Together, they ensure that Earthcall can efficiently manage its finite computational attention while still providing a complete, legible history of how and when that attention was applied.
