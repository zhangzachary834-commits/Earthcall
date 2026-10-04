# Relational Propagation and Primary Relations Addendum

**AI Model:** Jules (OpenAI)
**Harness:** Earthcall Development Harness
**Session ID:** current_session_id

This addendum synthesizes the architectural directives established in:

1.  `docs/architecture/ontology/Formation_Systemic_Maxim.md` (The requirement for an innate capacity to understand recursions, cycles, and the Relational propagation of effects).
2.  `docs/architecture/ontology/PRIMARY_AND_SUB_RELATIONS.md` (The principle that primary Relations between Singulars do not disappear, keeping history, while only sub-Relations disappear when their premises change).

## 1. Systemic Propagation and Closed vs Open Systems

Earthcall demands that changes do not happen in isolated silos. When a subsystem changes, its effects must propagate—what the architecture calls **Relational propagation**.

If a relation changes, it ripples to other relations and other Singulars. The architecture requires that any change that interlocks another system, or propagates beyond its own immediate recursion, must be **known**. It cannot be a silent update. The system must be capable of recognizing these cycles and ripples.

## 2. Primary Relations as the Enduring Record

How does the system remember these ripples without drowning in an infinite replay log? This is answered by the distinction between Primary Relations and Sub-Relations.

When a premise changes—for example, if a Law was conditioned on an object being "red" but it turns "blue"—the sub-Relation built on that premise becomes logically impossible.

Instead of erasing the connection between the involved Singulars entirely, Earthcall dictates:
-   **Sub-Relations dissolve or change kind:** The active variant is removed from the active set because it is no longer true now. It changes to a historical kind.
-   **Primary Relations endure:** The foundational "Primary" kind relation between the two Singulars remains. It keeps a record of existence and a mathematical model of its past.

## 3. Integration Thoughts: Propagation Could Leave a Relational Trace

These documents suggest a compatible future architecture, but they do not establish that Primary Relations already record every propagated occurrence. `PRIMARY_AND_SUB_RELATIONS.md` explicitly presents this Primary/Sub-Relation architecture as not yet built, and its proposed mathematical history is not an event log.

If a cyclical change or systemic ripple invalidates a premise, a future implementation could re-kind the affected Sub-Relation while preserving an enduring Primary Relation between the same Singulars. That enduring Relation could retain a mathematical model of relational history without being treated as a chronological ledger of propagation events.

Accordingly, Primary Relations may become one durable substrate through which relational continuity remains knowable after active premises change, but the Systemic Maxim's stronger requirement that propagated inter-system changes "must be known" still needs an authored mechanism. This addendum records the architectural compatibility rather than claiming that Primary Relations alone already fulfill that requirement.
