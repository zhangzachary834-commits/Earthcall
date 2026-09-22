# Adaptive Compute Moments and B-Time Rete

**Date:** 2026-09-19
**Status:** Architectural cross-check
**Related:** `docs/architecture/ADAPTIVE_COMPUTE_MOMENTS.md`, `docs/architecture/law/B-time Rete.md`

## The Interrelation

Adaptive Compute Moments and B-Time Rete both concern the temporal execution of processes within Earthcall, particularly those related to the evaluation of Laws and the maintenance of the world state.

B-Time Rete proposes an optimization to the Rete algorithm where the structure of Laws (the degrees of freedom and property paths) acts as a pre-filter. By analyzing the Laws, the system can determine in advance which property changes could *possibly* trigger an action, avoiding the need to evaluate facts that fall outside these ranges.

Adaptive Compute Moments provides an architectural mechanism for allocating computational time to processes whose value increases with time (like relevance discovery and cache building), ensuring they don't cause frame drops. It introduces the concept of a "ComputeScheduler" that grants bounded compute opportunities based on priorities, budgets, and authored policies.

### How they relate

B-Time Rete's optimization directly informs and enhances Adaptive Compute Moments.

The pre-filtering and dependency analysis performed by B-Time Rete generates a concrete understanding of *what needs to be evaluated and when*. This information is exactly what the `ComputeScheduler` in Adaptive Compute Moments needs to make informed decisions about how to allocate its budget.

When a Zone requests a "Heavy Charge-Up" (as described in Adaptive Compute Moments), B-Time Rete provides the dependency graph and the specific ranges of possible property changes. The scheduler can use this to prioritize the evaluation of specific beta nodes or partial routes that are most relevant to the imminent entry, maximizing the impact of the temporary compute boost.

Furthermore, B-Time Rete's concept of an "in-advance abstract interpreter" is itself a process that requires computational time. The execution of this interpreter, the building of the pre-filters, and the linking of action nodes to beta chains are all tasks that can be managed by the `ComputeScheduler` as resumable maintenance work.

In summary, B-Time Rete defines the *logic* of efficient evaluation, while Adaptive Compute Moments provides the *economy* of its execution.
