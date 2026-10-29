# Timeline Singulars and Adaptive Compute Moments

**Date:** 2026-10-28
**Status:** Architectural cross-check
**Related:** `docs/architecture/ontology/TIME_AND_MOMENT.md`, `docs/architecture/ADAPTIVE_COMPUTE_MOMENTS.md`

## The Interrelation

The `TIME_AND_MOMENT.md` architecture establishes that there is no single global clock for the entire simulation. Instead, Time is reified as independent `Timeline` Singulars. A `Timeline` can be owned by a Zone, an Object, or a process.

`ADAPTIVE_COMPUTE_MOMENTS.md` establishes a system for budgeting finite real-world CPU/GPU time (Compute Moments) to background maintenance and preparation tasks, allowing "Heavy Zones" to intentionally lower foreground framerates to gain massive computational budgets.

### How they relate

Because Timelines are independent, they do not all have to advance at the same rate relative to real-world execution. Adaptive Compute Moments provides the economic engine that arbitrates the progression of these separate Timelines.

1.  **Budgeting Temporal Progression:** When the `ComputeScheduler` allocates a time budget, it is not just allocating generic maintenance time; it is deciding *which Timeline is allowed to advance its moments*.
2.  **Telos and Priority-Driven Time:** The decision of which Timeline receives a compute budget is heavily influenced by the `telos` (Hierarchy of Joys) of the entity owning the Timeline. A Timeline owned by a Zone with a foundational telos will receive a larger share of the Compute Moment budget than an unranked, peripheral Timeline.
3.  **Variable Internal Simulation:** Consequently, a highly prioritized Timeline might advance multiple internal simulation ticks within a single real-world frame, while a deprioritized Timeline might be paused entirely. This means the *rate of time passage* within a specific domain of the world is directly managed by Adaptive Compute budgeting.
4.  **Heavy Zone Charge-Up:** During a "Heavy Zone charge-up", the Zone's local Timeline may be granted nearly all of the available compute budget. Its internal time can rapidly fast-forward (e.g., to run complex procedural generation Laws) while the overall render framerate is intentionally degraded.

In Earthcall, time does not pass simply because the CPU clock ticked. Time passes for a specific entity because Adaptive Compute deemed its Timeline worthy of the compute budget required to evaluate its next Moment.

**Author**: Jules (Claude 3.5 Sonnet, default harness, Session 8124)
