# Implementation Plan: Ontological Rete Architecture
    
**Origin.** The architectural foundation in this document was conceived by Zach on 2026-09-03, following an audit that exposed the relational limitations of the current C++ Rete. Zach provided the core paradigm shift: leveraging Earthcall's Category framework as "possibility receptacles," pre-computing relational joins as actual `Relation` beings, and turning the Rete network itself into an observable `Formation`. My (Antigravity's) contribution is internalizing this telos and formalizing its mechanical execution—specifically mapping how this discrete topological approach natively resolves the $O(N)$ sweep bottleneck and $O(N^2)$ continuous math explosion.


## 1. Motivation & Context
The current C++ Rete implementation (`Law.cpp`, `PropheticRete.hpp`) suffers from two fatal flaws when applied to Earthcall's continuous, dynamic simulation:
1. **Directional Blindness:** To avoid $O(N^2)$ memory explosion on continuous variables, the network drops remote state from the index, breaking declarative triggers (see `docs/audits/rete_directional_blindness.md`).
2. **The Sweep Bottleneck:** To mask the blindness, continuous laws fall back to an $O(N)$ brute-force sweep over the entire Universe every tick, setting a hard ceiling on scale.

This plan outlines the migration to an **Ontological Rete**, conceived by Zach. It rips the rule engine out of hidden C++ structs and reconstructs it natively using Earthcall's existing `Singular`, `Relation`, and `Formation` primitives. 

## 2. Layer 1: Category Filtering (Possibility Receptacles)
The $O(N)$ Universe sweep will be retired. Instead, the engine will leverage Earthcall's `Category` framework as semantic indices.

* **Instant Classification:** Upon creation, every `Singular` is filtered and placed into `Category` Formations (via `instance-of` or `composed-by` Relations).
* **Semantic Pruning:** When a Law looks for targets, it does not linear-search. It queries the relevant `Category` Formations.
* **Prophetic Integration:** The engine uses Prophetic Rete's bounds-checking to evaluate entire Categories at once. If a Category's intrinsic bounds are incompatible with the Law's conditions, the entire sub-graph is pruned instantly without inspecting a single instance.

## 3. Layer 2: Reified Condition Joins (Materialized Relations)
To solve the $O(N^2)$ join thrashing for continuous data (e.g., Distance, Line-of-Sight), Earthcall will stop calculating math inside rule engine join nodes. 

* **Topological Edges:** Continuous relationships will be evaluated by dedicated, highly-optimized subsystems (like spatial partitioning) which will emit first-class `Relation` beings (e.g., a `Near` relation between `A` and `B`).
* **Discrete Listening:** The rule engine will simply listen for discrete `relation-state` facts (e.g., "A new Near relation was formed"). 
* **The Result:** The combinatorial cross-product completely vanishes from the rule evaluator. Continuous math is converted into discrete graph topology before the Law ever sees it.

## 4. Layer 3: The Network as an Observable Formation
This architecture fulfills **Refusal #6 (No Black Box)** for the engine's own evaluation logic.

* **Reifying the Graph:** Beta nodes and Alpha filters will cease to be `std::vector<BetaNode>`. They will be represented as actual `Relation`s connecting `Category` Formations to `Law` Singulars.
* **Governable Logic:** Because the Rete network is now a standard `Formation`, it is fully legible to the rest of the engine. Laws can query the network. Meta-laws can optimize the network's category-prioritization on the fly. 
* **Dynamic Restructuring:** As new Categories are generated dynamically by First Movers, the network's shortest-path BFS graph traversals can automatically rewire to find the fastest evaluation routes.

## 5. Migration Strategy
1. **Establish Category Formations:** Ensure all `Object` and `Being` instantiations rigorously attach to their root ontological Categories via reified `Relation`s.
2. **Abstract Spatial/Continuous Joins:** Migrate continuous conditions out of raw `ConditionNode` mathematical comparisons and into discrete `Relation` states managed by localized subsystems.
3. **Deprecate C++ Beta Nodes:** Phased removal of `ReteNetwork::BetaNode` in favor of resolving condition asts via graph traversal of Category Formations.
4. **Retire the Sweep:** Once the graph provides complete coverage without directional blindness, the O(N) Universe fallback for `WhileTrue` laws can be deleted.

---
*Authored by Antigravity, session b7b980a8-6cb5-452f-a382-0c554ebd1d69, 2026-09-03T20:19:01-07:00.*
