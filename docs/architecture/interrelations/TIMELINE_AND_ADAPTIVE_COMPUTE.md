# Timeline Singulars and Adaptive Compute Moments

**Date:** 2026-10-28  
**Status:** Architectural cross-check; scheduler integration proposed, not established  
**Sources:** `docs/architecture/ontology/TIME_AND_MOMENT.md`; `docs/architecture/ADAPTIVE_COMPUTE_MOMENTS.md`

## The distinction

`Timeline` and `Moment` are first-class Singulars. Multiple Timelines may advance independently; a broad `world-timeline` currently supplies the compatibility clock for legacy Law execution. Ownership is expressed through Relations, not through the C++ field holding a Timeline.

Adaptive Compute Moments is an **architecture proposal** for bounded, resumable work: a scheduler would grant CPU/GPU maintenance opportunities according to foreground deadlines, policy, and available budget. Its own design explicitly separates the **clock** (when work may progress), **scheduler** (which due work receives compute), and **thread** (where it runs).

## Architectural interrelation

1. **Independent opportunity:** A render frame need not imply a maintenance advancement. Maintenance can have its own temporal domain and pending-work state.
2. **Budget versus time:** A granted compute budget permits bounded work; it does **not**, by itself, prove that an arbitrary Timeline advances or that simulation time runs faster. Such a mapping requires an explicit authored or first-mover contract.
3. **Authorable policy:** Zone readiness, desired convergence, priority, and available frame slack may inform a future scheduler. Telos-driven arbitration is a possible authored policy, not a currently implemented universal ranking rule.
4. **Heavy-Zone preparation:** Lowering a foreground FPS target could free opportunities for resumable preparation. This is not a license to fast-forward world physics or skip causal obligations.
5. **Legibility:** Requested/granted budgets, work progress, missed deadlines, and the relevant Timeline/Moment relationships should be inspectable without inventing hidden temporal ownership.

**Implementation boundary:** The Timeline substrate exists, but `TIME_AND_MOMENT.md` explicitly leaves the final Law–Timeline–Moment relations undecided. `ADAPTIVE_COMPUTE_MOMENTS.md` labels its scheduler an architecture proposal. Do not describe a `ComputeScheduler` as already controlling all Timeline rates or granting time according to a fixed telos hierarchy.

**Original intake:** Jules PR #812; scope and implementation claims reconciled against the source architecture.
