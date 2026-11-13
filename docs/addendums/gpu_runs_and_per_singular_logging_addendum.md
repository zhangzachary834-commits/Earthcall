# Integrating Authorable GPU Runs and Per-Singular Durable Logging

**Status:** Proposed architectural synthesis; neither the production GPU Run scheduler nor its per-Singular telemetry-to-log bridge is implemented by this addendum.
**Sources:** [Authorable GPU Runs Between CPU Cycles](../architecture/AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md) and [Per-Singular Durable Logging](../architecture/PER_SINGULAR_DURABLE_LOGGING.md).

This proposal connects bounded GPU execution diagnostics to meaningful, identity-attributable history without treating transient device counters as ontological facts.

## 1. Device telemetry is not automatically Singular history

A GPU Run may produce completion and continuation counts, overflow/refusal reasons, and measured device costs. These are **execution observations**. A raw counter such as `fallback_queue_overflows = 42` is not, by itself, evidence that any particular Zone or Law caused those overflows.

A device cohort may contain work from several authored sources, and its temporary work-item indices or queue positions are not durable identities. Attribution requires a validated mapping from the run's workload and policy to the actual authoring Singular(s), under the revisions and Zone context for which that mapping was valid.

If attribution is unknown, ambiguous, or shared, preserve that uncertainty in an aggregate diagnostic; **never invent a single owner**. Multiple independently established owners may receive appropriately scoped records, but a shared run must not silently charge its entire cost to each participant.

## 2. Selective durable events, not a permanent stream of counters

[Per-Singular Durable Logging](../architecture/PER_SINGULAR_DURABLE_LOGGING.md) distinguishes semantic history, debug traces, and high-frequency instrumentation. The proposed bridge must preserve those categories:

1. **Device:** A bounded GPU Run yields telemetry and any exact/certified fallback or refusal outcome at a host-visible boundary.
2. **Host:** The Kernel validates the run's revision, source attribution, and outcome. It distinguishes an expected fallback from a meaningful anomaly, such as repeated refusal or a reproducible queue-capacity violation.
3. **Routing:** A meaningful, attributable event may enter the affected Singular's durable history, while high-frequency timings and per-frame counts remain in sampled diagnostics or subsystem chronicles.
4. **Persistence:** The identity-aware durable sink writes an event under stable Singular identity and retains it across process restarts. This is a required property of the future bridge, **not a feature demonstrated by this documentation change**.

For example, a Law's repeatedly requested GPU policy may be refused under a measured profitability veto. The refusal and its valid reason could be historically meaningful; recording every per-frame measurement indefinitely would not be.

## 3. A proposed attributable event envelope

An eventual event should retain enough evidence to reconstruct its claim without relying on process-local indices:

```text
GpuRunHistoryEvent {
    stableSourceSingularIds     // only identities proven by source mapping
    workloadAndPolicyRevision   // validity regime for attribution
    runOrCorrelationId          // connects to aggregate diagnostics
    observedAt
    adapterAndExecutionRegime
    eventKind                   // refusal, overflow, fallback, etc.
    measuredCountsAndReason
    fallbackDisposition         // completed, pending, or refused
}
```

This is illustrative, not a committed serialization schema. If source identity cannot be established, an aggregate diagnostic must preserve that fact rather than manufacturing a per-Singular event. Log records should be bounded, attributable, queryable, and safe to reopen without truncation, according to the logging architecture.

## 4. Performance as Truth, without conflating measurement and being

[Performance as Truth](../architecture/ontology/PERFORMANCE_AS_TRUTH.md) demands that the physical computation be faithfully ordered after authored meaning. Device telemetry can expose where execution diverges from that intention, and an attributable durable event can preserve the explanation.

But **execution cost is not the Singular's identity**, and a transient scheduler decision is not automatically a permanent authored Property. The history records a witnessed execution event under a stable bearer; it does not turn a GPU queue, continuation record, or measurement into another Singular.
