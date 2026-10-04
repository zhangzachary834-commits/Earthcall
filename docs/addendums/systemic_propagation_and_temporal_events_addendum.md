# Systemic Propagation and Temporal Events Addendum

This addendum synthesizes the architectural directives established in:

1.  `docs/architecture/ontology/Formation_Systemic_Maxim.md` (The requirement that systemic propagation, open/closed system interlocking, and recursive changes must be "known" by the system).
2.  `docs/architecture/ontology/TIME_AND_MOMENT.md` (The ontology defining Timelines as independent domains owned by Singulars, and Events as distinguished Moments).

## 1. The Requirement to Know the Propagation

The `Formation_Systemic_Maxim` establishes a rigorous requirement for the engine: "Any change that propagates and interlocks another system must be known. Any change that propagates other systems beyond its own recursion."

This means that a subsystem's change cannot simply be a silent memory overwrite. If a change ripples across relational bounds—affecting other Singulars in an open system—the occurrence of that ripple is a first-class ontological fact that Earthcall must register.

## 2. Events as the Record of Occurrences

`TIME_AND_MOMENT.md` provides vocabulary that could make such occurrences legible, while deliberately leaving Event-defining Relations and Timeline admission as future work. It defines an `Event` not as a hidden callback timestamp, but as a "distinguished Moment" (a `Singular`) carrying:
-   A transition verb.
-   Subject/object participants.
-   An author.
-   A stable occurrence identity.

Events inhabit Timelines, which can be owned by *any* Singular.

## 3. Integration Thoughts: Propagation is an Event on a Timeline

The integration of these concepts reveals how Earthcall fulfills the maxim's demand for legibility without creating a centralized, global bottleneck.

A plausible future integration is for a Relational-propagation occurrence (a change in subsystem A rippling to affect subsystem B) to be represented by an **Event**. This is a synthesis direction, not current engine behavior.

1.  **Independent Timelines:** Because any Singular can own its own Timeline, propagation need not conceptually collapse onto one global clock. Which Timeline or Timelines should admit a propagation Event remains intentionally undecided.
2.  **Distinguished Moments:** The Event captures exactly who participated in the change (the `subject` and `object`), the `verb` (the nature of the propagated effect), and the `author`.
3.  **Historical Snapshot:** Existing published Events are passed as Rete facts and serve as read-only historical snapshots. A future propagation Event could reuse that established occurrence shape rather than inventing a second hidden timestamp or log mechanism.

Thus, the two documents expose a compatible future road: systemic propagation needs legible occurrence identity, while Event, Moment, and Timeline supply an ontology capable of carrying such identity. The remaining design work is to author when propagation constitutes an Event, which Timeline admits it, and by what Relations. Until that exists, this addendum records the architectural connection without claiming the requirement is already solved.
