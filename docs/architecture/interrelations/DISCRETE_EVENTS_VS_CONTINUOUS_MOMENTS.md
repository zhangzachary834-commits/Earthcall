# Discrete Events vs Continuous Moments

**How Earthcall separates point-in-time state transitions from continuous time flow to maintain both deterministic algorithmic causality and reversible spatial physics.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../events/EVENT_BUS_VS_EVENT_HANDLER.md` (The delivery mechanism for discrete state changes)
*   `../ontology/TIME_AND_MOMENT.md` (The flow of continuous time and interval tracking)
*   `EVENT_BUS_AS_ALGORITHMIC_CALL_STACK.md` (How discrete events drive algorithms)

---

## The Interrelation

Time in a simulation is traditionally represented as a single ticking frame counter (`deltaTime`), driving both physics updates and game logic. Earthcall shatters this monolithic concept of time by strictly isolating **continuous flows** from **discrete state transitions**.

`TIME_AND_MOMENT.md` describes how the continuous passage of time is handled. The universe clock flows smoothly, and continuous processes (like an OntoMath `Flow` action or a `CurveModel` driven by `time`) integrate mathematically over this smooth domain. This continuous time knows nothing of logic, algorithms, or semantic state changes; it simply progresses values according to exact functions.

Conversely, `EVENT_BUS_VS_EVENT_HANDLER.md` (and `EVENT_BUS_AS_ALGORITHMIC_CALL_STACK.md`) describe a reality built on discrete, instantaneous jumps. When a Law fires and changes a property (e.g., `is_burning := true`), that is an instant, zero-duration event published to the Event Bus. The Event Bus drives the algorithmic call stack entirely independently of `deltaTime`.

This separation is crucial for architectural integrity:

1. **Deterministic Logic:** By isolating discrete logic onto the Event Bus, Earthcall guarantees that causality (Law A triggers Law B) is exact and independent of framerate fluctuations. A complex semantic inference chain does not "take time" to resolve across multiple frames; it evaluates as a discrete block of causality on the Bus.
2. **Reversible Space:** By isolating continuous changes into pure OntoMath flows driven by `Moment`s, the engine can reverse spatial changes via mathematical integration without needing to step backward through millions of discrete algorithmic ticks.

When these two systems touch, they do so explicitly. A continuous collision (tracked by continuous time and spatial bounds) results in a discrete `PhysicsCollisionEvent` being fired onto the Event Bus. A discrete event (a Person clicking a button) might trigger a Law that authors a new continuous OntoMath flow.

**Conclusion:** Earthcall does not have one "Time." It has continuous `Moment`s for space and discrete `Event`s for logic. By keeping these domains structurally distinct, it prevents the framerate-dependent bugs common to standard game engines and enables features like exact mathematical reversibility alongside complex, looping Rete causality.
