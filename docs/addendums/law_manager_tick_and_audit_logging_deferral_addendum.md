# Integrating Law Manager Tick and Audit Logging Deferral

The `LawManager` tick loop is decoupled from expensive string and JSON allocations by guarding `ECA::LawAuditLogger` calls with `wouldLog(...)`. This ensures hot paths remain performant while maintaining observability.
