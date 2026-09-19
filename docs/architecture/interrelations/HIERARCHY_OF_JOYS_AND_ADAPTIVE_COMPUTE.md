# Hierarchy of Joys and Adaptive Compute Moments

**Date:** 2026-09-19
**Status:** Architectural cross-check
**Related:** `docs/architecture/ontology/HIERARCHY_OF_JOYS.md`, `docs/architecture/ADAPTIVE_COMPUTE_MOMENTS.md`

## The Interrelation

The Hierarchy of Joys structures the telos of entities within Earthcall. It defines an order of foundation, rooted in a primary Lexeme, establishing a clear ranking of importance based on depth within a Formation.

Adaptive Compute Moments is the system that decides how to allocate finite computational resources (time budget) to background tasks, maintenance, and preparation, ensuring that these processes do not interfere with foreground performance. It relies on priorities and budgets.

### How they relate

The Hierarchy of Joys provides the most fundamental, authored answer to the question: *What should Adaptive Compute prioritize?*

When the `ComputeScheduler` in Adaptive Compute Moments has a limited budget and multiple `MaintenanceClient`s requesting time (e.g., relevance discovery, route revisitation, cache building), it needs a mechanism to arbitrate.

The telos rank of the entities involved in the maintenance tasks serves as the primary heuristic for this arbitration. A task maintaining a relation or index for an entity close to the root of the Hierarchy of Joys should inherently receive a higher priority or a larger share of the compute budget than a task concerning an unranked or distant entity.

In the context of a "Heavy Zone charge-up", the preparation policy can use the Hierarchy of Joys to determine which structures must be converged first. Structures related to the foundational telos of the Zone are prioritized over peripheral details.

The Hierarchy of Joys ensures that the temporal distribution of compute in Earthcall is not an arbitrary engine decision, but a reflection of the authored semantic order of the world. It provides the "why" for the "when" and "how much" of Adaptive Compute.
