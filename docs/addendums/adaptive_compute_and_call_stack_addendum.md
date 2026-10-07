# Addendum: Integrating Adaptive Compute and the Event Bus Call Stack

## Reflections on the Architectural Synthesis

The capacity for Earthcall to "think harder than it moves" relies on a delicate balance: executing complex, potentially heavy algorithms without freezing the interactive foreground loop. This capability, defined as "Adaptive Compute Moments," is inextricably linked to how Earthcall manages its internal control flow. By synthesizing [Adaptive Compute Moments](../architecture/ADAPTIVE_COMPUTE_MOMENTS.md), [Event Bus as Algorithmic Call Stack](../architecture/interrelations/EVENT_BUS_AS_ALGORITHMIC_CALL_STACK.md), and [Event Bus vs Event Handler](../architecture/events/EVENT_BUS_VS_EVENT_HANDLER.md), we can understand how algorithmic execution becomes schedule-able.

### The Problem of Heavy Computation

As described in `ADAPTIVE_COMPUTE_MOMENTS.md`, certain operations—such as deep relevance searches, cache building, or converging the "Heavy Charge-Up" of a complex Zone—require significant computational time. In a traditional game engine, invoking a heavy algorithm (like a recursive graph search or a large `while` loop) monopolizes the main thread, resulting in a frame spike or an outright stall.

### Unrolling Algorithms via the Event Bus

The solution is structural. As `EVENT_BUS_AS_ALGORITHMIC_CALL_STACK.md` details, Earthcall refuses opaque C++ `while` loops for game logic. Instead, algorithms are unrolled into discrete increments. A Law evaluates a single step and emits a "State Changed" event onto the `EventBus`. That event, rather than a tight `while` loop, triggers the next iteration.

Crucially, the `EVENT_BUS_VS_EVENT_HANDLER.md` distinction ensures the Event Bus only *delivers* these messages; it does not process them synchronously and block execution.

### Enabling the Compute Scheduler

This transformation of control flow is what makes Adaptive Compute Moments possible. Because an algorithm is broken down into discrete events, the `ComputeScheduler` can intervene. Instead of being trapped inside a monolithic loop, the scheduler can observe the pending maintenance tasks (the events on the bus) and allocate bounded time slices to them.

If the foreground frame requires 14ms and the target is 16ms, the scheduler grants exactly 2ms of budget to the pending algorithmic steps. When the 2ms expires, the algorithm can gracefully yield—its progress preserved in the form of pending events and partial states—and resume precisely where it left off during the next available Compute Moment.

Thus, the Event Bus acting as a Call Stack is the structural prerequisite that allows heavy world preparation and algorithmic depth to be scheduled dynamically, preserving the stability of the Person's immediate interactive experience.

---

*(Model: Jules, Harness: default, Session ID: 13313331426855917341)*
