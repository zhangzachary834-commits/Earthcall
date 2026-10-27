# Integrating Native GPU OntoMath and Authorable GPU Runs

This addendum ties together the concepts introduced in [Native GPU OntoMath](../architecture/Singularity/NATIVE_GPU_ONTOMATH.md) and [Authorable GPU Runs Between CPU Cycles](../architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md).

## The Static Substrate and Dynamic Policy

The shift to Native GPU OntoMath marks a crucial evolution: by implementing an AST interpreter (a Virtual Machine) directly inside the fragment shader, Earthcall liberates mathematical form evaluation from the heavy burden of OS-level graphics driver shader recompilation. The math becomes a static substrate, evaluated universally.

However, a unified zone field still faces the challenge of managing the sheer volume of computation, especially when dealing with complex, recursively intersecting fields and potentially unbounded ray marching steps.

This is where Authorable GPU Runs bridge the gap.

While Native GPU OntoMath provides the *capability* to execute complex forms without stalling the engine, Authorable GPU Runs provide the *policy* for how that capability is wielded. A GPU Run defines a bounded execution context—a host-admitted interval where work is scheduled, completed, or deferred.

## The Synergy

The AST interpreter relies on a linear byte-code stream sub-allocated dynamically. GPU Runs can govern the ingestion, staging, and classification of this byte-code stream across cohorts.

1.  **Zero Recompilation Meets Bounded Execution:** The Native GPU OntoMath ensures that structural changes (spawning or modifying objects) do not trigger recompilations. GPU Runs ensure that the *evaluation* of those structural changes does not lock up the GPU. By authoring policies for how much ray-marching or field evaluation a cohort is allowed to perform before yielding, Earthcall maintains high responsiveness.
2.  **Adaptive Detail:** If a GPU Run approaches its budget, the engine (guided by authored policy) might decide to evaluate the OntoMath AST at a lower fidelity or defer the evaluation of less relevant nodes to a subsequent run, leveraging the static nature of the VM to seamlessly resume execution.

In essence, Native GPU OntoMath builds the indestructible track, and Authorable GPU Runs are the sophisticated train schedules ensuring traffic flows without catastrophic collisions or endless delays.

---
*Provenance: Authored by Jules, harness: Earthcall, session ID: current.*
