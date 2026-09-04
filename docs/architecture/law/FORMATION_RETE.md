# Formation Rete

*The Ontological, Graph-Routed Successor to Standard Rete*

**Origin.** The architectural foundation was forged on 2026-09-03 in a collaborative synthesis between Zach, Claude Opus 5, and Antigravity. It began with Antigravity’s audit exposing that the existing C++ Rete was a "dishonest" single-subject index suffering from Directional Blindness. Zach proposed replacing the C++ structs with Earthcall's native `Relation` and `Category` Formations, creating a dynamic, continuous path-finding network. Opus 5 rigorously grounded this telos in frontier computer science—mapping it to Formal Concept Analysis (FCA), Hierarchical Navigable Small World graphs (HNSW), and Magic Sets, while providing critical corrections regarding graph weights and the necessity of the Sweep. This document formalizes that unified architecture.

---

## 1. The Core Paradigm: Ontological Rule Evaluation
The classical Rete algorithm (Forgy, 1979) relies on Beta nodes to maintain the cross-product of multi-subject joins. In a continuous simulation (e.g., 60fps positions), this combinatorial explosion causes $O(N^2)$ memory thrashing. Earthcall previously dodged this by dropping remote state from the index, sacrificing relational logic to maintain performance.

**Formation Rete** abandons hidden C++ structs (`AlphaNode`, `BetaNode`). Instead, the rule engine is reified into the world itself using Earthcall’s existing ontology (`Singular`, `Relation`, `Formation`). It transforms the rule engine from a discrete cross-product table into a **bidirectional, typed hypergraph traversal**.

This rigorously satisfies **Refusal #6 (No Black Box)**: The evaluation index is just a Formation. It can be read, governed, and optimized by other Laws. It requires zero new C++ classes.

## 2. The Three Theoretical Pillars

### A. Category Lattices via Formal Concept Analysis (FCA)
Calculating overlap between *instances* is a trap—it recreates the $O(N^2)$ cross-product as persistent memory. Instead, overlap holds between **Categories** ($O(C^2)$, where $C$ is small and static until redefined). 
* **The Structure:** Category overlap forms a concept lattice (the mathematical meet of shared properties). 
* **Why it works here:** FCA requires a total object-attribute incidence relation. Most systems fail at this because objects have hidden fields. Refusal #6 guarantees complete legibility, making the derived Category lattice trustworthy.
* **Iceberg Lattices:** To prevent exponential worst-case scaling in the concept lattice, Earthcall uses "iceberg lattices" (bounded by a support threshold), capping the relational breadth.

### B. The Slow Adapter via HNSW (Hierarchical Navigable Small World)
The network does not instantly track every possible overlap at every moment. It uses an independent, continuous "slow adapter" to maintain shortest-path routing between Singulars and Categories.
* This maps precisely to the **HNSW** algorithm.
* **Parameters:** It respects max neighbors ($M$), build effort on an independent clock (`efConstruction`), and query effort (`efSearch`).
* **Hub Labeling:** Root Categories act as the natural hubs. A being's index entry is effectively its distance to the categories it participates in.

### C. Forward-Backward Hybrid via Magic Sets
Standard Rete is forward-chaining (data flows to rules). A Law traversing the graph to find its subjects is backward-chaining (goal-driven). 
* **Formation Rete uses both:** Adopting the "Magic Sets" Datalog technique, Earthcall uses the Law's position in the graph (backward chaining) to restrict the forward-chaining engine to *only* the facts that the goal could ever reach.

## 3. Routing Objective: Telos vs. Selectivity
In a standard graph search, Dijkstra seeks the "shortest" path (hop count). In Formation Rete, shortest $\neq$ cheapest, and cheapest $\neq$ best.

A 2-hop path through a category with 10,000 members costs more than a 5-hop path through narrow categories. The engine evaluates paths as a **Best-First Search over Value per Unit Cost**:
* **Cost (Fan-out / Selectivity):** The expected branching factor of traversing the relation.
* **Value (Telos / Joy-Rank):** The authored weight/strength of the bond (as defined by the Hierarchy of Joys).
* **Execution:** The engine looks first where the *telos* of the world intends it to look, scaled by the computational expense of the traversal.

## 4. The Sweep as the Correctness Floor (Bloom Filter Discipline)
**The $O(N)$ Universe Sweep is not deleted.** 
An approximate index (like HNSW) that misses a candidate *narrows* the result set. This violates `PROPHETIC_RETE.md` §2 ("Widen where uncertain, never narrow"), causing laws to silently go deaf.

The overlap index acts purely as a **scheduling optimization**. Its job is to make the sweep rare, never to make it unnecessary. Operating on the discipline of a Bloom filter (one-sided error), the $O(N)$ sequential scan remains the correctness floor that licenses the optimizer to be heuristic.

## 5. The Grounded Tower (Stratification)
Because the index is a Formation of Relations, the overlap adapter will inevitably index its own index. 
To prevent infinite meta-recursion (where traversal descends endlessly into index edges while traversing), the architecture adopts a **Grounded Tower**:
* The meta-levels are stratified (Layer $N$ may read but not walk into edges of Layer $N+1$).
* The reflection bottoms out entirely in the non-reflective C++ Universe Sweep. 

## 6. Open Operational Design Parameters
*(Pending resolution by Zach)*

1. **The Distance Function:** What is the precise metric distance function across the three overlap subkinds (property, kind, quantitative)? Is it a unified function or three layered indices?
2. **The Sweep Schedule:** Now that the Universe Sweep is a correctness backstop rather than the main 60fps loop, what dictates its execution frequency?

---
*Authored by Antigravity, session b7b980a8-6cb5-452f-a382-0c554ebd1d69, 2026-09-03T22:42:13-07:00.*
