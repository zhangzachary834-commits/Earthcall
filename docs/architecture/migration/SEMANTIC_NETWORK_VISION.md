# Semantic Network Vision: Implementation Plan

This document outlines the final three stages to achieve the "neural-network like" semantic mapping and behavioral influence vision. It strictly adheres to Earthcall's ontological guardrails (the 5 refusals), ensuring that the engine remains an unopinionated machine and all semantic truths reside in-world.

## 1. Synaptic Plasticity & Semantic Decay
**The Goal:** The semantic graph must act like a memory. Unused concepts should decay and sever over time, while frequently reinforced concepts should strengthen.

**Implementation (Engine Level):**
*   **Where:** `RelationManager::tick(float deltaTime)` or `LanguageSystem::tick()`.
*   **How:** 
    *   Iterate over the active Zone's undirected/unpinned `Relation` objects.
    *   Gradually decay the `weight` dynamic property by a configurable `decayRate * deltaTime`.
    *   When the Syntactic Parser parses an utterance that matches an existing relation (e.g., "Arthur picked up the sword" is spoken again), increase that Relation's `weight` toward 1.0.
    *   If a Relation's weight drops below `0.0f`, it is garbage-collected (`RelationManager::removeRelation()`).
*   **Guardrail Check:** Modifying weights and pruning edges is an algorithmic maintenance function (first movement) operating on primitives. We are not defining what memory *is* (No new C++ domain classes); we are just updating a float value on a structural `Singular`.

## 2. Subconscious Inference (The Hidden Layers)
**The Goal:** The system must infer connections dynamically (e.g., if `Sword -> belongs_to -> Arthur` and `Arthur -> is_in -> Castle`, infer `Sword -> is_in -> Castle`).

**Implementation (Authored Law Level):**
*   **How:** We will *not* hardcode inference logic (like transitive property rules) in C++. Doing so would violate the core tenet: *"No subsystem may define what a thing IS."*
*   Instead, inference rules are **authored in-world as Laws**.
*   We will inject a First Mover `Law` into the world (via JSON) with:
    *   **Condition:** An `All` node combining two `Related` condition nodes (checking if Subject has relation A, and that target has relation B).
    *   **Action:** A `Create` or `Synthesize` action that physically mints a new `Relation` (the inferred connection) into the Zone.
*   **Guardrail Check:** By delegating inference to the `Law` system, the C++ engine remains ignorant of semantics. The logic of inference is authored data, not a von Neumann algorithm carved into the type system.

## 3. Bridging Semantics to Physics (The Output Layer)
**The Goal:** The semantic graph must actively influence the physical 3D simulation.

**Implementation (Authored Law Level):**
*   **How:** Again, no C++ bridges are needed. We use the engine's existing ECA (Event-Condition-Action) framework.
*   We author a continuous `Law` that bridges the semantic gap:
    *   **Condition:** A `Related` node monitoring for a specific semantic edge (e.g., `type="has_inventory"`) where the `weight` property is `> 0.8`.
    *   **Action:** A `Set` or `Lerp` node that writes to the physical target's `transform` or `position`, snapping the 3D Object to match the semantic truth.
*   **Guardrail Check:** This entirely skips C++ domain nouns. The physical universe bends to the semantic graph purely because a Person (or First Mover) authored a Law commanding it to.

---

## Next Steps for Execution
1.  **C++:** Implement the decay and reinforcement loop in `LanguageSystem::tick`.
2.  **JSON (First Mover):** Write the `inference_law.json` and `semantic_physics_bridge.json` First Mover seeds to bring the graph to life without altering the engine binary.
