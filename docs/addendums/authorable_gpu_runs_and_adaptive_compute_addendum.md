# Integrating Authorable GPU Runs Between CPU Cycles and Adaptive Compute Moments

**Model Name:** GPT-4o
**Harness Name:** Earthcall Autonomous Testing Harness
**Session ID:** session_74a58b99_gpu_adaptive_addendum

This addendum synthesizes the architectural relationship between **Authorable GPU Runs Between CPU Cycles** (`docs/architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md`) and **Adaptive Compute Moments** (`docs/architecture/ADAPTIVE_COMPUTE_MOMENTS.md`). While they address different dimensions of execution scheduling, they compose into a unified theory of finite compute distribution across Earthcall.

## 1. The Intersection of Host Budget and Device Execution

Adaptive Compute Moments defines *when* and *how much* computation is admitted by the host over time. It governs the distribution of finite computational attention among background maintenance tasks, prophetic preparation, and foreground requirements. It sits at the macro-level, deciding if a subsystem gets time in the current frame and bounding the budget it may consume.

Authorable GPU Runs Between CPU Cycles governs *what happens inside* that admitted device interval. It allows Earthcall to author bounded GPU execution policies (such as internal continuation, queue compaction, and re-cohorting) without yielding back to the host CPU between every stage.

They intersect at the **admission boundary**:
1.  **Adaptive Compute** decides to admit a workload and sets the overarching temporal budget.
2.  **GPU Run** accepts the admitted work, internally reorganizes it for device profitability (e.g., separating easy cases from hard-tail cases), and loops on the GPU until completion, exact fallback, or a maximum stage count is reached.
3.  The GPU Run yields results and telemetry back to the CPU, informing the next Adaptive Compute Moment's admission decisions.

## 2. Policy Authoring vs. Scheduling Infrastructure

Both architectures share a core Earthcall philosophy: **Earthcall must be able to author its execution policy, but that policy does not write scheduling infrastructure.**

*   In Adaptive Compute Moments, a Person or Law authors the *value* of a process (e.g., Formation Rete relevance discovery) and its *temporal domain*, but the Kernel implements the resumable work queue.
*   In GPU Runs, a Law might author `recohort mode` with a `coarse quantum = 8`, but this does not place a literal GPU loop into the Law engine. The Kernel lowers these authored properties into backend execution IR.

This maintains the separation outlined in `docs/architecture/ontology/PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md`: meaning is authored; execution is lowered.

## 3. Synergy in the Hard Tail

The most powerful synergy between these two systems occurs in resolving "hard-tail" workloads, such as Northern Veil volumetric ray marching or complex Rete propagations.

Without GPU Runs, the CPU must micromanage every iteration of a difficult workload, causing frame drag. Without Adaptive Compute, an autonomous GPU Run might consume so much device time that it starves foreground rendering, leading to visual stutter.

Together:
1.  **Adaptive Compute** says: "You have 2ms of GPU time to resolve these unresolved rays."
2.  **GPU Run** says: "I will use this 2ms to perform 3 stages of bounded continuation entirely on the device. I will compact the queue internally. Whatever is unresolved after 3 stages falls back to the exact CPU queue for the next Moment."

This prevents the GPU from holding the system hostage (handled by Adaptive Compute) while maximizing the work done per host-device roundtrip (handled by GPU Runs).

## 4. Telemetry and Truth

Both architectures demand rigorous telemetry. Adaptive Compute needs to know how much work was completed to adjust future budgets. GPU Runs generate this telemetry: completion counts, queue overflows, and fallback reasons.

This telemetry is not just for performance profiling; it is a semantic output of the execution substrate, allowing Earthcall's authored Laws to react to their own execution cost, fulfilling the vision of **Performance as Truth** (`docs/architecture/ontology/PERFORMANCE_AS_TRUTH.md`).
