# Systemic Propagation and Temporal Events Addendum

**AI Model:** Jules (OpenAI)
**Harness:** Earthcall Development Harness
**Session ID:** current_session_id

This addendum synthesizes the architectural directives established in:

1.  `docs/architecture/ontology/Formation_Systemic_Maxim.md` (The requirement that systemic propagation, open/closed system interlocking, and recursive changes must be "known" by the system).
2.  `docs/architecture/ontology/TIME_AND_MOMENT.md` (The ontology defining Timelines as independent domains owned by Singulars, and Events as distinguished Moments).

## 1. The Requirement to Know the Propagation

The `Formation_Systemic_Maxim` establishes a rigorous requirement for the engine: "Any change that propagates and interlocks another system must be known. Any change that propagates other systems beyond its own recursion."

This means that a subsystem's change cannot simply be a silent memory overwrite. If a change ripples across relational bounds—affecting other Singulars in an open system—the occurrence of that ripple is a first-class ontological fact that Earthcall must register.

## 2. Events as the Record of Occurrences

`TIME_AND_MOMENT.md` provides the exact mechanism for this knowledge. It defines an `Event` not as a hidden callback timestamp, but as a "distinguished Moment" (a `Singular`) carrying:
-   A transition verb.
-   Subject/object participants.
-   An author.
-   A stable occurrence identity.

Events inhabit Timelines, which can be owned by *any* Singular.

## 3. Integration Thoughts: Propagation is an Event on a Timeline

The integration of these concepts reveals how Earthcall fulfills the maxim's demand for legibility without creating a centralized, global bottleneck.

When Relational propagation occurs (a change in subsystem A rippling to affect subsystem B), the system "knows" this because the propagation generates an **Event**.

1.  **Independent Timelines:** Because any Singular (or Formation, or Zone) can own its own Timeline, the propagation does not have to be serialized onto a single global game clock. The interlocking systems can record the interaction on their respective local Timelines.
2.  **Distinguished Moments:** The Event captures exactly who participated in the change (the `subject` and `object`), the `verb` (the nature of the propagated effect), and the `author`.
3.  **Historical Snapshot:** This Event is passed as a Rete fact. It serves as a read-only historical snapshot.

Thus, the requirement "must be known" is solved: Systemic propagation is recorded as a distinguished Event Moment on the Timelines of the involved Singulars. This guarantees that cycles and interlocked state changes are always legible, auditable, and accessible to Laws, acting as the historical footprint of systemic ripples.
