# Law Application and Audit Logging Deferral

The relationship is a **conditional reduction of diagnostic work**, not a universal zero-allocation guarantee for the LawManager tick.

## Source-checked behavior

- [`Law::applyTo` in `Law.cpp`](../../../src/ZonesOfEarth/AuthorsOfLaw/Law.cpp) uses `LawAuditLogger::wouldLog("LAW")` before building some failure and application audit payloads. Its no-actions branch uses a verbose-level guard. A no-write `WhileTrue` application can skip an audit record entirely.
- The same file guards some EventBus audit payloads with `wouldLog("EVENT")`, but computes some event identifiers **before** the guard. Law evaluation, Rete processing, and non-audit work still occur.
- [`LawAuditLogger`](../../../src/ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp) delegates `wouldLog` to logger policy. [`ConditionModel.cpp`](../../../src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp) has other audit call sites; the Law.cpp guards must not be generalized to every logging path.

## Engineering consequence

Guarding payload construction can avoid unnecessary strings and JSON for disabled audit categories, while preserving enabled observability. Performance claims require measured hot-path witnesses with the same logger configuration and Law workload; `wouldLog` alone does not prove an allocation-free tick or deferred asynchronous logging.