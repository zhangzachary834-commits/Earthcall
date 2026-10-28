# Integrating Authorable GPU Runs and Per-Singular Durable Logging

**Model Name:** GPT-4o
**Harness Name:** Earthcall Autonomous Testing Harness
**Session ID:** session_74a58b99_gpu_logging_addendum

This addendum synthesizes the architectural relationship between **Authorable GPU Runs Between CPU Cycles** (`docs/architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md`) and **Per-Singular Durable Logging** (`docs/architecture/PER_SINGULAR_DURABLE_LOGGING.md`). It addresses how granular device-resident execution telemetry connects to the durable, attributable history of beings in Earthcall.

## 1. Bridging Device Execution and Semantics

GPU Runs execute at the lowest substrate level, performing bounded loops, queue compaction, and re-cohorting without CPU intervention. They produce critical telemetry: fallback reasons, continuation counts, and queue overflows.

However, a raw counter like `fallback_queue_overflows = 42` is semantically meaningless if it cannot be attributed to the authored entity that caused it. This is where Per-Singular Durable Logging integrates.

When a GPU Run yields its telemetry at the CPU admission boundary, that data must be mapped back to the Singulars (e.g., specific Zones, Laws, or materials) that authored the executed policy.

## 2. Telemetry as Attributable History

Per-Singular Durable Logging demands that meaningful runtime history has an attributable home. The performance and execution reality of a Singular is part of its history.

If a specific authored Law requesting `auto eligibility = true` consistently forces a GPU Run into its exact fallback path due to pathological complexity, this is not just a transient console warning. It is a durable fact about that Law's execution history.

The integration flow:
1.  **Device:** The GPU Run completes and yields telemetry (e.g., `NEEDS_ROOT_REFINE` overflow).
2.  **Host Boundary:** The CPU correlates the execution cohorts back to their authoring Singulars (e.g., the Northern Veil Zone).
3.  **Durable Logging:** The execution anomaly is persisted into the specific, queryable durable log for that Singular, surviving process shutdown.

## 3. Performance as Truth in the Log

This synthesis realizes the principle of **Performance as Truth** (`docs/architecture/ontology/PERFORMANCE_AS_TRUTH.md`). By logging GPU Run telemetry into the Per-Singular Durable Log, Earthcall ensures that the physical execution cost of an authored being is as legible and permanent as its explicit properties. A Person reviewing a Zone's history can see exactly when and why its authored geometry overwhelmed the GPU's internal continuation budget.
