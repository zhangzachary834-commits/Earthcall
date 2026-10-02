# Spatial Relations and Formation Rete

**How modeling spatial positions as Relations transforms physics and spatial queries into semantic network traversal.**

**Status:** Conceptual interrelation.
**Session Context:**
*   **Model Name:** Jules
*   **Harness Name:** default
*   **Session ID:** 13284209740648546535
*   **Date:** 2026-09-24

## The Interrelation

The document `../ontology/Person's locations in Zones' bounds as Relations.md` argues that a Person's spatial position inside a Zone should not be a hardcoded property of the Person, but rather an authored property of a Relation between the Person and the Zone.

Meanwhile, `../law/FORMATION_RETE.md` describes how the Rete network efficiently computes and caches paths between Singulars across the relation graph.

These two concepts interrelate profoundly. When spatial containment and position are moved from being isolated properties (like a `vec3` inside `Person`) into explicit `Relation` objects linking the `Person` to the `Zone`, spatial queries become a native subset of semantic network queries.

### Thoughts on Integration

If a Person's location is a Relation, the Formation Rete can natively match patterns like "all Persons inside Zone X" without needing a specialized spatial acceleration structure (like an octree) for the initial query. The Rete automatically tracks the edge connecting the Person and the Zone.

Furthermore, if the distance or exact coordinates are stored as a property *on* this Relation (e.g., `relation.position`), the Prophetic Rete can observe writes to this property to detect movement, naturally triggering Laws related to boundary crossings or proximity without needing a separate physics event bus.

This unifies spatial reasoning with semantic reasoning. A Person being "near" someone or "inside" a Zone is structurally identical to a Person "owning" an Object or "belonging" to a Formation. It simplifies the engine's query logic by turning spatial physics into standard relational logic.
