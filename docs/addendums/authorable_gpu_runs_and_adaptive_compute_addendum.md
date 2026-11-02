# Integrating Authorable GPU Runs Between CPU Cycles and Adaptive Compute Moments

**Status:** Proposed architectural synthesis; the production GPU Run scheduler and its integration with Adaptive Compute are not yet implemented.
**Sources:** [Authorable GPU Runs Between CPU Cycles](../architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md) and [Adaptive Compute Moments](../architecture/ADAPTIVE_COMPUTE_MOMENTS.md).

These proposals address complementary boundaries of finite compute allocation. Adaptive Compute defines temporal opportunities for resumable work and foreground safety; GPU Runs define how admitted device work can continue and re-cohort without a CPU readback between every stage. Their integration is a design direction, not an existing end-to-end scheduler.

## 1. Host admission and device continuation

Adaptive Compute's current proposal focuses on independently clocked **CPU maintenance work**, not an already-shipping unified CPU/GPU scheduler. Extending its admission logic to GPU work would require accounting for GPU queue contention, foreground rendering deadlines, measured device costs, and asynchronous completion.

At a proposed integration boundary:

1. The host decides whether a workload may run, using measured slack, priorities, and a conservative estimate of its GPU cost.
2. The Kernel validates and lowers authored GPU Run policy into bounded execution stages. On the device, unfinished work may be classified, compacted, and re-cohorted without host inspection of every intermediate queue.
3. At a completion/yield boundary, the host receives results and telemetry, checks validity, and updates later admission decisions. An invalidated continuation must be repaired or discarded rather than published as current truth.

A GPU Run is **host-admitted execution substrate**, not an independent clock, a new Singular, or a guarantee that the GPU can be preempted mid-dispatch.

## 2. Authored policy versus scheduling infrastructure

A Person or Law may author a workload's value, temporal preferences, and bounded GPU policy (for example, a coarse quantum of eight root operations and at most three continuation stages). The Kernel owns admission, hard ceilings, backend execution IR, dispatches, and exact/certified fallback.

**Authored intent is not a physical dispatch command.** A Law cannot create arbitrary GPU instructions or claim physical GPU resources merely by naming them. Backend capability, mathematical validity, and measured profitability remain separate admission tests. This follows [Property Storage and OntoMath Binding](../architecture/ontology/PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md) and the GPU Run source specification.

## 3. Hard-tail work and honest budgets

Northern Veil ray transport illustrates the potential benefit: easy rays finish, difficult rays continue in coherent cohorts, and unresolved obligations follow an exact or certified fallback. A similar pattern might eventually serve other bounded workloads, but a Rete workload is not automatically GPU-executable merely because it is expensive.

Consider a scheduler *estimating* that it can afford about 2 ms of device work. It may admit a GPU Run with three finite continuation stages and a specified fallback. **The 2 ms is a planning target, not a hard wall-clock limit enforced by a running GPU dispatch.** Operation-count and stage ceilings bound the work structurally; device timestamps and observed tail latency must determine whether future admissions remain profitable. If a backend cannot safely meet the foreground constraint, it must veto or reduce the run.

Unresolved work goes to the specified exact/certified fallback, which may be CPU or another supported executor; it must not silently be declared complete or assumed to be a CPU queue on the next Moment.

## 4. Telemetry, validity, and Performance as Truth

Useful outputs include completion counts, stage counts, queue overflow, fallback reasons, measured latency, and the validity regime (device, workload revision, and other relevant conditions). These observations can inform future admission and authored policy without converting transient scheduling classifications into ontological identity.

[Performance as Truth](../architecture/ontology/PERFORMANCE_AS_TRUTH.md) calls for computation ordered faithfully to authored meaning. It does **not** make every performance counter an authored Property or a durable historical event. The separate [GPU Runs and Per-Singular Durable Logging](gpu_runs_and_per_singular_logging_addendum.md) addendum describes when an attributable, meaningful execution anomaly may warrant durable history.
