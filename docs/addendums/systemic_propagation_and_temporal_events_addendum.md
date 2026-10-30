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

## 2. Events as a Legible Occurrence Substrate

`TIME_AND_MOMENT.md` provides temporal primitives that a future propagation architecture can use. It defines an `Event` not as a hidden callback timestamp, but as a "distinguished Moment" (a `Singular`) carrying:
-   A transition verb.
-   Subject/object participants.
-   An author.
-   A stable occurrence identity.

Any Singular may own a Timeline. Published Events can also be exposed to Rete as read-only historical snapshots. However, `TIME_AND_MOMENT.md` explicitly leaves Event-defining Relations and Timeline admission as future work; it does not yet say that every Event inhabits a Timeline or that propagation automatically creates one.

## 3. Integration Thoughts: Propagation Could Become an Event

The integration of these concepts suggests a path for fulfilling the maxim's demand for legibility without creating a centralized, global bottleneck.

A future architecture could represent a Relational propagation occurrence (a change in subsystem A rippling to affect subsystem B) as an **Event**. The rule deciding when propagation becomes an Event, which Timeline admits it, and which authored Relations connect the occurrence to participants remains to be designed.

1.  **Independent Timelines:** Because any Singular can own its own Timeline, a future propagation-recording design need not serialize every occurrence onto one global game clock. Which Timeline or Timelines admit a particular Event remains an authored architectural question.
2.  **Distinguished Moments:** The Event captures exactly who participated in the change (the `subject` and `object`), the `verb` (the nature of the propagated effect), and the `author`.
3.  **Historical Snapshot:** When an Event is published through the EventBus, its Rete fact can carry a read-only historical snapshot of that Event.

Thus, Event and Timeline supply useful ontology for a future solution to the requirement that propagation "must be known," but they do not yet solve that requirement by themselves. The remaining architecture must decide how propagation occurrences are authored as Events, how those Events enter Timelines, and how Laws lawfully reach them.
