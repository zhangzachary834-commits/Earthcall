# Per-Singular Durable Logging Architecture

**Status:** Architecture vision and migration target  
**Initial proving subsystem:** Laws  
**Scope:** Cross-cutting observability and historical persistence across Earthcall

---

## 1. Why this architecture exists

Earthcall currently has no single coherent theory of runtime history.

A large amount of meaningful activity is emitted directly into the console. Refusals, diagnostics, execution details, and other runtime information can accumulate into one enormous stream. That stream is difficult to read while Earthcall is running and, more importantly, much of it is transient: once the console or process closes, the only record of that activity may disappear.

The file-backed logging that does exist is uneven. Some systems already emit dedicated files under `/logs`, while other first-class systems do not. Existing files can also collapse the histories of many distinct instances into one large subsystem-wide log.

These are different problems, but they share one cause: Earthcall currently treats logging too often as incidental output rather than as an architectural layer.

This document establishes a different model.

> **The console witnesses the world passing by. Durable logs preserve what happened to the things that inhabit it.**

The purpose is not to turn every print statement into permanent storage. The purpose is to give meaningful runtime history a durable, attributable, queryable home.

---

## 2. The central invariant

Earthcall already reasons in terms of identifiable beings.

Its logging architecture should respect that identity.

The governing principle is:

> **A being with stable identity should be capable of possessing stable history.**

That means Earthcall should distinguish three different observational surfaces:

```text
immediate observation
        ↓
console

domain-wide history
        ↓
subsystem chronicle

individual history
        ↓
per-Singular durable log
```

These answer different questions.

The console answers:

> What deserves attention right now?

A subsystem chronicle answers:

> What happened across this domain?

A per-Singular log answers:

> What happened to this particular being?

No one of these surfaces should be forced to serve all three purposes.

---

## 3. The console is not Earthcall's memory

The console is process-bound and observer-oriented.

It is useful for immediate information such as:

- boot progress,
- concise warnings,
- severe failures,
- explicit diagnostic modes,
- short refusal summaries,
- high-value execution summaries,
- pointers toward deeper records.

It is a poor place for a complete durable history.

A refusal, for example, may still deserve a concise console line:

```text
[Law] <identity> refused execution — see durable Law history
```

but the full refusal record should not exist only in that transient stream if it is semantically important.

Closing a console window must not be equivalent to erasing Earthcall's memory of meaningful events.

This architecture therefore treats stdout/stderr and equivalent transient surfaces as observation channels, not historical authorities.

---

## 4. Durability across runtime death

Durability is a required property of the new architecture.

Meaningful persisted history should survive:

- console closure,
- normal application shutdown,
- destruction of the current process,
- test-process termination,
- later application relaunch.

The ordinary lifecycle should look conceptually like:

```text
runtime activity
      ↓
semantic logging event
      ↓
identity-aware routing
      ↓
durable sink
      ↓
filesystem
      ↓
process terminates
      ↓
history remains readable
```

For histories intended to span sessions, reopening the logger must not silently truncate the prior history.

If a system eventually distinguishes between session-scoped traces and lifetime-scoped histories, that distinction must be explicit in the model. It must not emerge accidentally from whichever file-open flag a caller happened to choose.

---

## 5. Logging observes ontology; it does not define ontology

Logging is downstream of Earthcall identity.

A Singular does not become itself because a filename exists. A Law is not identified by its log path. A Relation is not defined by a logging key.

Instead:

```text
Earthcall ontology
      ↓
stable Singular identity
      ↓
logging address
```

When a stable Singular identity already exists, that identity or an existing canonical encoding of it should govern log ownership.

Per-instance history must not be keyed solely by:

- memory addresses,
- vector indices,
- process-local handles,
- transient object order,
- mutable display names,
- other accidental implementation details.

Human-readable names may supplement a record, but stable identity owns the history.

This is essential for continuity across hydration, reload, or reconstruction.

---

## 6. Per-Singular history

A per-Singular durable log contains events attributable to one identifiable Earthcall being.

Conceptually:

```text
logs/
    laws/
        <stable-law-id>.log

    zones/
        <stable-zone-id>.log

    relations/
        <stable-relation-id>.log

    persons/
        <stable-person-id>.log
```

This directory layout is illustrative, not prescriptive. Implementations must follow Earthcall's actual path, filesystem-safety, identity, and persistence conventions.

The invariant is semantic:

> History belonging uniquely to Singular A must be recoverable independently from Singular B.

For distinct Singulars A and B:

```text
event attributable only to A ∈ history(A)
event attributable only to A ∉ history(B)
```

and vice versa.

No two instances should overwrite or truncate each other's history merely because they belong to the same subsystem.

---

## 7. Subsystem chronicles remain legitimate

Per-Singular logging does not require the elimination of aggregate logs.

A subsystem chronicle may still be valuable.

For Laws, for example, a domain-wide chronicle could answer:

> Which Laws fired during this interval?

while a particular Law's history answers:

> Why did this Law fire, refuse, fail, or change?

The correct architecture may therefore write one semantic event to more than one observational surface:

```text
semantic Law event
      ├──> per-Law durable history
      └──> optional Law subsystem chronicle
```

The aggregate file must no longer be the only place where an individual being's history can be reconstructed.

The two views are complementary rather than competitive.

---

## 8. What belongs in durable history

Not every runtime occurrence deserves permanent storage.

The logging system must distinguish at least three categories.

### 8.1 Semantic history

Events that help explain the meaningful life of a Singular.

Examples may include:

- creation or hydration,
- authored changes,
- execution,
- refusal,
- failure,
- meaningful state transition,
- meaningful relation changes,
- load/save lifecycle,
- identity-relevant lifecycle events.

### 8.2 Debug traces

Detailed information useful when investigating behavior but not necessarily suitable for the ordinary durable history.

### 8.3 High-frequency instrumentation

Per-frame timing, repeated evaluation probes, verbose traversal traces, and similar data that can occur at very high frequency.

These categories must not be blindly collapsed.

A system that technically records everything but produces gigabytes of repeated noise has failed observationally. Durable per-Singular history should remain legible enough that a human or agent can reconstruct what mattered.

High-frequency tracing may remain available through explicit trace/debug modes, dedicated files, sampling, or other mechanisms appropriate to the subsystem.

---

## 9. Refusals are first-class historical events

Earthcall already gives refusals substantial diagnostic importance.

The new architecture should make them attributable and durable.

If a particular Law refuses execution, that refusal should be traceable in that Law's history.

If a Relation or Zone later has an equivalent semantic refusal or failure mode, the same principle should apply there.

The console may still surface the fact immediately. But the console should no longer be the only surviving witness.

A durable refusal record should contain enough context to understand what occurred without requiring the user to have captured the original console session.

---

## 10. Shared infrastructure, subsystem semantics

The architecture should not produce one bespoke logger per subsystem.

The shared layer should own mechanical concerns. Subsystems should own semantic meaning.

Conceptually:

```text
subsystem semantic event
        ↓
logging domain
        ↓
stable identity
        ↓
identity-aware router
        ↓
durable sink
        ↓
formatted record
        ↓
filesystem
```

Shared infrastructure should converge around responsibilities such as:

- path derivation,
- filesystem-safe identity encoding,
- directory creation,
- append versus truncate semantics,
- sink ownership,
- buffering,
- flushing,
- close behavior,
- write failures,
- optional aggregate routing,
- common timestamp/session metadata where appropriate.

Subsystem code should primarily answer:

> What meaningful event happened, and to which being?

It should not repeatedly reinvent:

> How do I open a file?

Scattered ad hoc `std::ofstream`, `printf`, `std::cout`, or equivalent implementations should not proliferate as each subsystem migrates.

---

## 11. Lifecycle and persistence semantics

A durable logger needs explicit lifecycle rules.

Every implementation should be able to answer:

- When is the sink created?
- When is a file opened?
- Is it kept open or opened per write?
- When is a record flushed?
- When is the sink closed?
- What happens during graceful shutdown?
- What happens when a Singular is removed?
- What happens when Earthcall restarts?
- What happens if the target directory does not exist?
- What happens if a write fails?
- Does reopening append or truncate?
- If log rotation is later introduced, what constitutes one historical epoch?

Critical history should not depend indefinitely on buffered state that only reaches disk by accident during process teardown.

RAII or equivalent deterministic lifecycle ownership should be preferred where it fits the implementation language and existing architecture.

---

## 12. Persistence invariant

For durable semantic history, the following property must hold:

```text
write(history)
close(runtime)
read(history)
```

The committed history must still be present.

For histories intended to span multiple sessions:

```text
session 1 writes A
runtime closes
session 2 reopens same Singular history
session 2 writes B
```

should not silently become:

```text
B
```

unless explicit semantics require truncation.

Ordinarily the resulting historical record should retain both A and B.

This persistence property must be tested, not merely assumed because a file exists.

---

## 13. Initial migration strategy

This architecture should be adopted incrementally.

A giant repository-wide logging rewrite would create unnecessary integration risk and make it difficult to prove isolation and persistence one domain at a time.

Each subsystem migration should approximately follow this sequence:

1. Audit all existing console emissions related to the subsystem.
2. Audit all existing file logging related to the subsystem.
3. Identify the stable identity model for individual instances.
4. Classify current output into semantic history, debug trace, and high-frequency instrumentation.
5. Reuse or introduce the smallest shared logging abstraction needed.
6. Route semantic history into per-Singular durable logs.
7. Preserve useful aggregate chronicles where appropriate.
8. Reduce redundant console verbosity without deleting useful information.
9. Add per-instance isolation tests.
10. Add persistence/reopen tests.
11. Document intentional exceptions.
12. Only then move to the next subsystem.

The first proving subsystem is **Laws**.

Laws are a useful starting point because they already have meaningful execution semantics, refusals, existing logs/diagnostics, stable identity concerns, and potentially high-frequency activity.

The Law migration should prove the architecture without pretending that the entire repository has already migrated.

---

## 14. Required correctness properties for each migrated subsystem

A subsystem should not be considered migrated merely because files appear under `/logs`.

At minimum, the migration should prove:

### Identity isolation

Two different instances have distinct histories.

### Attribution isolation

Activity unique to one instance does not leak into another instance's file.

### Non-destructive coexistence

Writing one instance's history does not truncate or overwrite another's.

### Stable routing

The same stable identity deterministically maps to the same historical destination, subject to any explicitly defined versioning/rotation policy.

### Persistence

Committed history remains after the logging/runtime lifetime ends.

### Reopen safety

Reopening the historical sink does not accidentally erase prior history.

### Console reduction

Detailed durable information is not needlessly duplicated in the console, while concise immediate signals remain where useful.

---

## 15. Migration status

This document describes the target architecture, not current completion.

Initial intended progression:

```text
[first proving subsystem] Laws
[todo] Zones
[todo] Relations
[todo] Persons
[todo] Formations
[todo] Events
[todo] other Singular-bearing systems discovered during audit
```

A subsystem that already has some logging is not automatically considered migrated. Migration means its logging semantics conform to the shared identity, durability, isolation, and lifecycle model.

The order after Laws may change based on repository evidence.

---

## 16. Future capabilities enabled by this layer

The first purpose is observability and durable history.

But once stable histories exist, later systems could build richer capabilities above them without burdening the logger itself.

Possible future uses include:

- Creator-facing historical inspection,
- causality reconstruction,
- debugging without console archaeology,
- comparing histories of multiple Singulars,
- machine-readable historical analysis by agents,
- temporal visualization,
- selective replay tooling,
- semantic event queries,
- asking a being for its own history.

These are future possibilities, not requirements of the first migration.

The logger should remain a low-level historical substrate rather than prematurely absorbing query engines, replay systems, or UI responsibilities.

---

## 17. Architectural boundary

The logging layer must remain observational.

It must not become a second source of truth for:

- Singular identity,
- Law semantics,
- Relation semantics,
- Zone membership,
- Personhood,
- authored properties,
- world persistence,
- ontology.

If the live/world persistence model and a log disagree, the log is evidence about execution history; it does not silently replace canonical state.

This protects Earthcall from turning observability into ontology.

---

## 18. Vision

Earthcall is increasingly a world of identifiable beings rather than a collection of anonymous engine operations.

Its historical architecture should reflect that transformation.

A giant process-wide console stream sees existence from nowhere in particular. It tells us that something happened somewhere, now.

Per-Singular history gives events a home.

A Law has a history.

A Person has a history.

A Relation has a history.

A Zone has a history.

A Formation has a history.

Those histories should be legible after the moment in which they occurred has passed. They should remain after the console closes. They should remain after the process that witnessed them ends.

The console is the window through which we watch Earthcall living now.

The durable logs are the memory by which Earthcall can tell us what happened before.
