# Authorable GPU Runs and Durable Logging

**Date:** 2024-11-20
**Status:** Architectural Synthesis

## Interrelated Documents
* `docs/architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md`
* `docs/architecture/PER_SINGULAR_DURABLE_LOGGING.md`

## The Interrelation

Authorable GPU Runs describe how computation is scheduled on the GPU, while Per-Singular Durable Logging describes how the history of entities is preserved.

### Execution vs Observability

*   **Authorable GPU Runs** allows a Law to specify how many passes a GPU execution cohort may take without asking the CPU what to do next. It authors the boundaries of computation, moving execution away from the CPU's direct step-by-step observation.
*   **Per-Singular Durable Logging** guarantees that the semantic history of entities is reliably recorded and persisted, maintaining Earthcall's "glass box" legibility even when processes span multiple frames or subsystem boundaries.

### How they relate

If the GPU is allowed to iterate autonomously via Authorable GPU Runs, it inherently becomes a temporary "black box" to the CPU. Durable Logging is the architectural answer to retaining legibility across this gap.

1. **Logging GPU-side state changes:** When an Authorable GPU Run performs internal re-cohorting or yields back to the CPU after bounded execution, the semantic outcomes of those passes must be captured. The Durable Logger receives these outcomes and attributes them to the correct Singular identity, ensuring that the GPU's autonomy doesn't result in unobservable history.
2. **Refusals and Bounding:** If a GPU Run hits its authored continuation limit and must yield unfinished work back to the "authoritative fallback," that fallback event is a critical semantic refusal/timeout. The Durable Logging layer guarantees this event is permanently recorded in the relevant entity's history, rather than just silently dropping frames or vanishing into a transient console message.

## Thoughts on Integration and Interrelation

The tension between these two architectures is the tension between performance and truth. Earthcall needs Authorable GPU Runs to hit high framerates and utilize modern hardware. But if the engine lets the GPU run completely wild, it violates the No Black Box principle.

Durable Logging is the bridge. It allows Earthcall to say to the GPU: "Take this state and run with it for 3 passes." And when the GPU returns, the logging layer ensures that the *story* of what the GPU did is written down. You get the speed of bounded GPU autonomy, but the legibility and historical permanence required by Earthcall's ontology.
