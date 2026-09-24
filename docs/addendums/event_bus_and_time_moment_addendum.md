# Addendum: Integrating the Event Bus, Discrete Moments, and Continuous Time

*(Model: Gemini 1.5 Pro, Harness: Jules, Session ID: 13407899851381596627)*

## Reflections on the Architectural Synthesis

When examining the foundational documents of Earthcall's ontology—specifically the `Event Bus vs Event Handler` and `Time and Moment`—a unified picture of "when" and "how" things happen within the system emerges.

Earthcall deliberately avoids the trap of treating events as arbitrary callbacks scattered throughout the codebase. At the same time, it refuses the model of continuous time that simply advances a float scalar every frame without semantic boundaries.

### The Role of the Event Bus in Establishing Moments
The [Event Bus vs Event Handler](../architecture/events/EVENT_BUS_VS_EVENT_HANDLER.md) document establishes that the Event Bus is not just a pub/sub mechanism; it is the structural spine for defining discrete shifts in state. An event is a semantic boundary.

When we integrate this with [Time and Moment](../architecture/ontology/TIME_AND_MOMENT.md), we transform continuous engine ticks into discrete ontological moments. A "Moment" is not just `t = 1.016`; it is the span of evaluation governed by a related set of events on the Bus.

### Resolving the Continuous/Discrete Conflict
The continuous progression of physics or animations (Time) is punctuated and given meaning by the discrete dispatches (Events). By treating the Event Bus as the arbiter of "what happened," we allow continuous simulation to operate purely mathematically until an Event forces a discrete state change or semantic re-evaluation.

By weaving these concepts together, we ensure that the architecture respects both the smooth, framerate-independent nature of simulation (Time) and the precise, ordered, semantic nature of logical consequences (Moments defined by Events).

---

**Linked References:**
* [Event Bus vs Event Handler](../architecture/events/EVENT_BUS_VS_EVENT_HANDLER.md)
* [Time and Moment](../architecture/ontology/TIME_AND_MOMENT.md)
