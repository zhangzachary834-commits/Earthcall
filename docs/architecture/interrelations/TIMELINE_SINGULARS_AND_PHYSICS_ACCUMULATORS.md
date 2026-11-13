# Timeline Singulars and Physics Accumulators

**How first-class relative Timelines may eventually meet fixed-timestep physics without deciding Earthcall's still-open Law ↔ Timeline semantics here.**

**Status:** Conceptual design question; no authored pause/scale/reverse physics binding is established by the current implementation.
**Connected Documents:**
* `../ontology/TIME_AND_MOMENT.md`
* `../events/PHYSICS_AND_COLLISION.md`

---

## The Interrelation

Earthcall already has two useful truths that should not be collapsed.

The physics substrate uses a fixed-timestep accumulator so numerical integration can advance in stable increments despite variable frame delivery. Separately, `Timeline` is a first-class relative `Singular`; multiple Singular-owned Timelines can advance independently.

The current Time architecture is explicit, however, that it **does not yet define** the final Laws that relate, synchronize, fork, pause, scale, or derive one Timeline from another. Legacy Law temporal execution remains compatibility machinery. Therefore examples such as “a Person authors a Law that runs a Timeline at 0.5×” are useful design probes, not descriptions of an implemented or already-decided semantic contract.

### A possible substrate boundary

A future physics/Timeline integration can preserve fixed-step numerical integration while admitting an authored temporal coordinate or delta at a boundary above the integrator. For example, a selected Timeline could eventually supply elapsed semantic time to an accumulator, while the integrator itself continues consuming fixed quanta.

That is a candidate shape, not a mandate that a Zone must have one “active Timeline,” nor a claim that Timeline pause, scaling, reversal, or locality already has defined Law semantics.

Any eventual design must answer at least:

1. how a physical process names or derives the Timeline governing it;
2. how several independently owned Timelines interact with one Zone or physical relation;
3. what reversal means for stateful integration, where merely passing a negative delta may not reconstruct prior physical state;
4. how discontinuities, pause, scaling, and Timeline switching affect accumulator state; and
5. how authored Law semantics remain inspectable rather than becoming hidden engine clock policy.

The invariant worth preserving now is narrower: **physics stability and temporal authorship are separate concerns, and the fixed-step substrate should not prematurely decide the still-open Timeline ontology.** The exact Law ↔ Timeline ↔ physics contract belongs to the future architecture pass identified by `TIME_AND_MOMENT.md`.
