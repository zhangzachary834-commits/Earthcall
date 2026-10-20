# Law Manager Tick and Audit Logging Deferral

The `LawManager` operates under strict performance requirements during its hot application loops (e.g., `tick()`, `sweepSubjects()`). Any synchronous operations must avoid unnecessary heap allocations or string manipulations.

## The Interrelation

The `ECA::LawAuditLogger` is deeply integrated with the `LawManager`. Because audit logging requires generating detailed JSON representations and string concatenation for the event bus and durable logs, it poses a risk to the `LawManager` tick performance.

To resolve this, the `LawManager` enforces a deferred evaluation strategy:
- `ECA::LawAuditLogger::instance().log()` calls are strictly guarded by `if (ECA::LawAuditLogger::instance().wouldLog(...))`.
- This ensures that C++ does not evaluate function call arguments—such as `law.getIdentifier()` virtual calls which return `std::string` by value—when audit logging is inactive.

This cross-check guarantees that the high-frequency tick of the `LawManager` can coexist with deep systemic observability.
