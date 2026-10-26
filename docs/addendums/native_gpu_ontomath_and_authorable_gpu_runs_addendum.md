# Native GPU OntoMath and Authorable GPU Runs

**Status:** Proposed architectural integration; neither the general GPU AST interpreter nor the production GPU-run scheduler is established by these documents as deployed.

Read [Native GPU OntoMath](../architecture/Singularity/NATIVE_GPU_ONTOMATH.md) alongside [Authorable GPU Runs Between CPU Cycles](../architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md).

## Representation is not scheduling

Native GPU OntoMath proposes serializing a mathematical expression tree into data consumed by a static GPU interpreter, instead of generating fresh WGSL for each structural edit. Its motivating bottleneck is topology-driven shader compilation. This is an **architecture proposal**, not a measured zero-recompilation result on current Earthcall.

Authorable GPU Runs proposes a different boundary: a host-admitted, bounded interval in which GPU work can execute, classify unfinished work, compact continuation records, and possibly re-cohort before a host synchronization boundary. Its source explicitly states that the **production GPU-run scheduler is not yet wired**.

The connection is a possible division of responsibility, not an existing pipeline:

1. **Expression representation:** a future GPU OntoMath evaluator could supply a bounded instruction/data stream for mathematical field evaluation.
2. **Execution policy:** a future GPU Run could admit bounded cohorts of evaluations, determine continuation stages, and route unresolved work to a defined exact or certified fallback.
3. **Backend realization:** the Kernel/backend—not an authored Law—would choose and validate actual dispatches, storage, synchronization, and hardware capabilities.

A static interpreter alone does not guarantee bounded GPU work, zero overdraw, or infinite scene complexity. A GPU-run policy alone does not make a fragment shader preemptible, permit arbitrary mid-invocation yielding, or allow unbounded loops. Re-cohorting and continuation require explicit backend support and state representation.

**Correctness boundary:** do not silently lower field fidelity or drop low-relevance work when a budget expires. Any approximation or deferral must have an explicit authored/Kernel contract, defined observation semantics, and a certified fallback. Preserve the distinction from the source document: authored scheduling intent ≠ policy representation ≠ physical cohort ≠ backend command.

**Evidence required before claiming integration:** a bounded interpreter witness, an actual host-admitted GPU-run path, continuation/fallback correctness, parity against the reference evaluator, bounded resource use, and measurements on supported backends. The current documents specify the destination, not that this joint system has shipped.

*Jules-originated architectural linkage; corrected to distinguish proposals from implemented runtime.*
