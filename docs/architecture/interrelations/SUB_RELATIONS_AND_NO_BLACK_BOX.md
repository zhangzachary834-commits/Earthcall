# Sub-Relations and No Black Box

**How historical and constitutive relation structures must remain transparent and addressable.**

**Status:** Conceptual interrelation.
**Session Context:**
*   **Model Name:** Jules
*   **Harness Name:** default
*   **Session ID:** 13284209740648546535
*   **Date:** 2026-09-24

## The Interrelation

The `../ontology/PRIMARY_AND_SUB_RELATIONS.md` document outlines how a sub-Relation never truly "dies" when its condition becomes false. Instead, it changes kind (e.g., to a historical kind) and remains attached to its primary Relation, preserving the history and the fact that the connection existed.

The `../ontology/NO_BLACK_BOX.md` doctrine mandates that any state a Person could mean something by must be exposed through registered, legible Properties. Nothing can be hidden from the Law system in private C++ fields.

These concepts interrelate because the history and status of a sub-Relation are deeply meaningful state. If a sub-Relation is merely "re-kinded" to preserve history, that history is useless if it is inaccessible.

### Thoughts on Integration

If a sub-Relation's shift from active to historical is to be more than a hidden engine optimization, the primary Relation must expose its sub-Relations through standard Property paths.

For instance, a Law might want to condition on whether two beings *used* to have a specific sub-Relation, even if it is not currently active. "No Black Box" requires that the primary Relation expose an iterable property (like `relation.historical_subs` or `relation.history`) that a Law can query via the Formation Rete.

Furthermore, `PRIMARY_AND_SUB_RELATIONS.md` mentions replacing the simplistic `weight` metric with a more closed-form mathematical model of the relation's past. By the "No Black Box" rule, whatever OntoMath formulation captures this history must be registered as a readable property on the primary Relation. This ensures that the preservation of relation history directly expands the vocabulary available to human authors.
