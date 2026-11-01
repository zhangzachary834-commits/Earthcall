# Authorable GPU Runs and Micro-Mastery

**Date:** 2024-11-20
**Status:** Architectural Synthesis

## Interrelated Documents
* `docs/architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md`
* `docs/architecture/Singularity/GPU_MICRO_MASTERY_ARCHITECTURE.md`

## The Interrelation

Authorable GPU Runs dictate the *policy* of bounded GPU autonomy, while GPU Micro-Mastery provides the *mechanism* that makes such autonomy safe and performant.

### Execution Policy vs. Data Mutability

*   **Authorable GPU Runs** allows a Law to specify how many passes a GPU execution cohort may take without asking the CPU what to do next. It authors the boundaries of computation.
*   **GPU Micro-Mastery** demands that data structures mapped to the GPU remain strictly flat, lock-free, and bounded, avoiding any host-side allocation overhead.

### How they relate

You cannot grant the GPU bounded autonomy if it cannot safely mutate its own working state.

1. **Re-cohorting Requires Lock-Free Mutation:** When the GPU re-cohorts unfinished work (as specified by Authorable GPU Runs), it must write to intermediate buffers. GPU Micro-Mastery guarantees that these buffers are pre-allocated and linearly accessible, preventing the GPU from needing to wait on the CPU to allocate new memory for the next stage.
2. **Deterministic Bounding:** The policy authored by a Person (e.g., "allow 3 continuation stages") is only respected if the time taken per stage is highly predictable. Micro-Mastery's elimination of unpredictable memory operations on the GPU ensures that a "stage" is a reliable unit of work.

## Thoughts on Integration and Interrelation

The synthesis of these two architectures is what actually frees the CPU from babysitting the GPU. Without Authorable GPU Runs, the Micro-Mastery data structures are merely fast buffers. Without Micro-Mastery, the Authorable GPU Runs would immediately stall on memory allocations during re-cohorting.

By combining them, Earthcall allows a Person to author a complex computational rule (like ray marching or particle simulation), send the initial state to the GPU, and trust that the GPU will intelligently loop over the work, sorting it into coherent batches entirely on its own, before yielding back to the CPU within the prescribed budget.
