# Prophetic Rete and Law Migration

**How conservative analysis of authored Law models supports migration from hardcoded decisions without making Laws go deaf.**

**Status:** Conceptual interrelation. The Law Migration Framework has staged, partly unimplemented rungs; this note does not declare those rungs complete.
**Connected documents:**
- [Prophetic Rete](../law/PROPHETIC_RETE.md)
- [Law Migration Framework](../law/LAW_MIGRATION_FRAMEWORK.md)
- [No Black Box](../ontology/NO_BLACK_BOX.md)

---

## The interrelation

The Law Migration Framework separates sensing and actuation, which remain first-mover responsibilities, from decisions that can become authored Laws. Its aim is to make decisions legible, inspectable, and governable, rather than merely move opaque code into a different interpreter.

### Ahead-of-time analysis, not the runtime interpreter

Prophetic Rete analyzes the *authored ConditionModel and ActionModel trees* ahead of execution. It derives conservative read demands, write effects, ranges, and possible relevance. Runtime Law evaluation and event delivery remain distinct machinery. This distinction matters: an ahead-of-time proof may accelerate routing, but it does not itself execute the migrated Law.

### Fail-open is a safety obligation

A sound relevance filter can discard a candidate only when it proves that candidate impossible. Unknown, stale, or incomplete analysis must widen rather than silently suppress a valid event. The existing Prophetic Rete specification describes these fallbacks and points to executable Section F witnesses in `tests/law/prophetic_rete_test.cpp`. Its guarantees apply to the audited analysis and integration paths; they must not be extrapolated to every future migrated Law or Formation-Rete consumer without new tests.

### Migration requires parity and authority boundaries

The migration ladder must preserve a recoverable old decision path until the authored replacement has passed parity, event-delivery, and persistence tests. The analysis graph is derived evidence, not an authored Relation or a new authority gate. A Law remains governable through its authored models and the existing constitutional boundaries, not by granting the optimizer authority to decide what reality may contain.
