# Integrating Property Storage, OntoMath Binding, and the Law Execution Frontier

**Status:** Architectural synthesis; general checked-cell custody and Law-governed Property access remain **proposals**, not shipped execution paths.
**Session Context:** Jules / default / 13820445096022914992. Audited and clarified by Sun Squid Intake III, 2026-10-23.

## Existing mechanisms versus proposals

[Property Storage and OntoMath Binding](../architecture/ontology/PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md) distinguishes the bearer Singular, the addressed PropertyPath, and any underlying value cell. It explicitly records that general shared/unique/weak cell custody, alias topology, and access governance remain open; only narrower typed-binding/container-access rungs are implemented. `PropertyPath` already uses runtime `StringId` interned names for portions of lookup. Interning is **not** a durable `Identity::SingularId`, a value-cell identity, a path-owned bearer ID, or a completed checked-handle system.

[Law Execution Frontier](../architecture/law/LAW_EXECUTION_FRONTIER.md) describes the cost of dynamic Law evaluation and proposes CPU bytecode/JIT directions. Its projected speedups and direct memory execution are architectural aspirations, **not** measured guarantees of the current engine. Optimizing a name lookup does not settle how a bearer is resolved across Zones, how an authored dependency is invalidated, or who may read or write a Property.

## Proposed integration and non-negotiable constraints

An eventual compiled Law could reuse interned path segments and a *validated, revocable* binding to avoid repeated name parsing. But a `StringId` alone cannot be mapped directly to a permanently valid memory cell: multiple bearers can expose the same path name, a changing authored Relation can change the intended referent, and structural mutation can invalidate the storage behind a binding. Resolution must preserve bearer identity, Zone jurisdiction, path semantics, and the authored distinction between aliasing and copying.

**Never replace permission checks with a once-at-bind grant.** Law-governed authority, revocation, structural read-only restrictions, and Kernel/Person boundaries must remain effective when a Law executes. The current `TransferPolicy` tiers gate set-to-set transfers; they do **not** already implement a universal Property read/write permission system. Any future mechanism must extend the existing authority office rather than invent a competing one. Ordinary reads/writes remain open unless an explicit rule closes them, as the storage proposal specifies.

An optimization witness would compare dynamic resolution with the candidate binding under mutations, replaced containers, changing Relations, Zone crossings, and revoked permissions; prove identical values and refusal behavior; then measure cost on a named workload. No such implementation or benchmark is claimed here.

## Architectural consequence

Interned names can reduce lookup overhead, but meaning remains a relationship among authored bearers, predicates, values, and governing Laws. A fast execution path is valid only while it preserves that relationship and its revocation boundaries.
