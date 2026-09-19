# Event Bus as Algorithmic Call Stack

**How Earthcall's Event Bus system provides the necessary control flow boundaries to compile algorithms into discrete, legal increments.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../law/ALGORITHMS_AS_LAW.md` (Compiling iterative/recursive algorithms into authored Law)
*   `../events/EVENT_BUS_VS_EVENT_HANDLER.md` (The distinction between the delivery mechanism and the discrete processors)

---

## The Interrelation

"Algorithms as Law" presents a profound challenge: how to express concepts like loops, recursion, and complex state traversal in a system that explicitly refuses C++ `while` loops or von Neumann control flow in favor of declarative, instantaneous mathematical Laws.

The answer lies in how Laws trigger one another, and this is where the `EventBus` (`EVENT_BUS_VS_EVENT_HANDLER.md`) becomes the implicit call stack.

In Earthcall, a Law cannot "loop" internally. If a Law needs to traverse a graph (like A* pathfinding), it evaluates one node, mutates a state (perhaps minting a new `Relation` or setting a property), and finishes.

This state mutation emits an Event onto the `EventBus`. That Event then triggers the *next* iteration of the Law (or a different Law entirely).

Therefore, the `EventBus` acts as the inter-Law call stack. A traditional algorithm's `while` loop is unrolled into a series of discrete Law executions, stitched together by the `EventBus` delivering "State Changed" events.

Furthermore, the strict separation defined in "Event Bus vs Event Handler"—where the Bus only delivers and the Handler (or in this case, the Rete/Law engine listening to the Bus) only reacts—ensures that the algorithm cannot run away and freeze the main thread. Because each step of the algorithm must pass back through the central Event Bus, the Engine retains total control over the simulation tick rate, capable of pausing, inspecting, or interleaving the algorithm's execution with other world events.

**Conclusion:** The Event Bus is not just for UI clicks or collisions. By serving as the delivery mechanism for state changes, it provides the fundamental control flow structure (the "call stack" and "looping" mechanism) required to execute complex algorithms purely through sequential, independent Law evaluations.
