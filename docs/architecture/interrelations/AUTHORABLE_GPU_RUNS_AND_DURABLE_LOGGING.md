# Authorable GPU Runs and Durable Logging

**Status:** Architectural interrelation; not an implemented GPU-run/logger integration.

## Source specifications

- [Authorable GPU Runs Between CPU Cycles](../AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md)
- [Per-Singular Durable Logging](../PER_SINGULAR_DURABLE_LOGGING.md)

## The boundary

Authorable GPU Runs proposes a **host-admitted, bounded execution policy**: a workload may execute, classify unfinished work, compact continuation records, and use further GPU stages before yielding to a host boundary. The source explicitly says that a production GPU-run scheduler is **not yet wired**. Existing indirect-dispatch experiments establish feasibility, not production deployment or profitability.

Per-Singular Durable Logging is a **migration target**, initially proved subsystem by subsystem beginning with Laws. It distinguishes immediate console observations, optional subsystem chronicles, and durable semantic history attributed to stable Singular identity. It does **not** establish that every subsystem already persists such history.

The two specifications meet at the **host-visible semantic outcome**, not at an assumption that every intermediate GPU operation is an authored event. A physical continuation queue, dispatch, or re-cohorting decision is derived execution state, not a new Singular or Law. Whether a bounded run completed, fell back, or refused acceleration may be relevant diagnostic evidence; it becomes per-Singular semantic history only when an owning subsystem defines an attributable event and a verified durable route.

## Proposed integration contract

1. Admit a GPU run under validated policy, revision/validity conditions, hard resource ceilings, and an exact or certified fallback.
2. Publish terminal results and bounded diagnostic/fallback reasons at the host boundary; do not claim that host code has inspected every internal GPU stage.
3. Classify events as semantic history, debug traces, or high-frequency instrumentation. Do not durably log every ray, queue operation, or dispatch by default.
4. Route an attributable semantic event by existing stable Singular identity, never by transient GPU indices, buffer offsets, or workgroup IDs. The logger remains observational, not an authority over ontology.
5. Verify identity isolation, append/reopen persistence, write-failure behavior, and no loss of relevant refusal records before claiming the integration works.

**Implementation status:** These are requirements for a future bridge. The current documents do not prove that GPU fallback events are automatically logged, that every refusal is permanently stored, or that bounded GPU autonomy alone preserves complete causal history. Neither specification authorizes fabricating such guarantees.
