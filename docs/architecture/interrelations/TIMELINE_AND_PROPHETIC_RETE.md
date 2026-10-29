# Timeline Singulars and Prophetic Rete

**Date:** 2026-10-28
**Status:** Architectural cross-check
**Related:** `docs/architecture/ontology/TIME_AND_MOMENT.md`, `docs/architecture/law/PROPHETIC_RETE.md`

## The Interrelation

The `TIME_AND_MOMENT.md` architecture replaces a monolithic world clock with independent `Timeline` Singulars. Different domains of the world may run on entirely separate clocks, advancing at different rates or pausing independently.

`PROPHETIC_RETE.md` details a system that performs a static, forward-looking analysis of Laws to determine possible state changes (writes) before they happen. Its goal is to predict and manage semantic collisions—situations where two Laws attempt to mutate the same property concurrently.

### How they relate

The introduction of independent Timelines radically shifts the definition of "concurrent" for the Prophetic Rete.

Before Timelines, if Law A and Law B both declared an intent to write to `Object.position`, the Prophetic Rete could easily flag this as a potential collision because both were evaluated in the same implicit global timestep.

With independent Timelines, the Prophetic Rete's clash prediction must become strictly **Timeline-aware**.

1.  **Temporal Intersection:** If Law A is driven by `Timeline_X` and Law B is driven by `Timeline_Y`, they cannot clash unless the `Moment` bounds evaluated on `Timeline_X` actually intersect in real-world execution with the `Moment` bounds on `Timeline_Y`.
2.  **Relative Advance Rates:** Because Adaptive Compute might advance `Timeline_X` rapidly while pausing `Timeline_Y`, the Prophetic Rete cannot assume that a structural write overlap in the Law text implies an actual runtime collision.
3.  **Cross-Timeline Solver:** The Prophetic Rete's cross-Law solver (the least-post-fixpoint solver) must integrate Timeline relations. To prove a collision, the Rete must prove that the two Timelines are synchronized or related in such a way that their execution Windows overlap.
4.  **False Positives:** If the Prophetic Rete ignores Timelines, it will generate massive false-positive collision reports, forcing the engine into unnecessary fallback modes because it assumes independent processes are running simultaneously.

Therefore, Prophetic Rete analysis is incomplete without examining the Timeline context of the Laws it evaluates. Predicting *what* will happen is no longer sufficient; the Rete must predict *when* it will happen relative to the independent clocks governing the actions.

**Author**: Jules (Claude 3.5 Sonnet, default harness, Session 8124)
