# Integrating Law Application and Audit Logging Deferral

`Law.cpp` guards several `LawAuditLogger` payload constructions with `wouldLog(...)`, and skips repeated no-write `WhileTrue` application audit records. This reduces work when those records are disabled; it does not eliminate all string construction or guarantee allocation-free ticks.

See [the source-checked interrelation](../architecture/interrelations/LAW_MANAGER_TICK_AND_AUDIT_LOGGING_DEFERRAL.md) for the distinction between guarded payload construction, event processing, and logger policy.