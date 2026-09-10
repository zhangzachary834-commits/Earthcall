# Physics as an Algorithmic Driver

**How the integration of hard-wired physics collisions into the Event Bus transforms physical interactions into the clock ticks that drive abstract algorithmic progression.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../events/PHYSICS_EVENTBUS_INTEGRATION.md` (The hybrid approach of publishing physics events)
*   `EVENT_BUS_AS_ALGORITHMIC_CALL_STACK.md` (The Event Bus providing control flow boundaries)

---

## The Interrelation

The "Event Bus as Algorithmic Call Stack" document establishes that in the absence of `while` loops, algorithms in Earthcall are driven by sequential Law executions triggered by state changes on the Event Bus. But what causes the initial state change, and how is the tempo of the algorithm maintained when relying on environmental interaction?

The "Physics EventBus Integration" provides the answer. Because the hard-wired C++ physics engine automatically publishes `PhysicsCollisionEvent`s to the Event Bus, physical interactions become the fundamental clock that can drive abstract algorithmic progression.

In a standard von Neumann architecture, an algorithm traverses a graph by iterating a counter or a pointer in memory as fast as the CPU allows. In Earthcall, an algorithm can be authored to traverse a logical graph by physically colliding objects.

Consider a Person authoring a cellular automaton or a sorting algorithm. Instead of trying to force the engine to iterate an array, the Person authors Laws that react to the `PhysicsCollisionEvent`s of small, physical "messenger" objects moving between "data" nodes.

1. Object A (the data) collides with Object B (the messenger).
2. The hard-wired physics engine detects the collision and pushes an event to the Event Bus.
3. The Rete matches a Law listening for this collision.
4. The Law fires, altering the state of Object A, and modifying the velocity of Object B toward the next node.
5. The algorithm yields back to the engine until the next physical collision occurs.

**Conclusion:** The publication of physics events to the Event Bus bridges the gap between spatial simulation and abstract computation. Physics collisions don't just trigger sound effects or damage; they act as the discrete, physical clock ticks that drive the execution of Person-authored algorithms across the Rete.
