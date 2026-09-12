# Multiplayer Taxonomy Conflict

**How modeling ontological categories as rooted DAG Formations ensures that multiple Persons can subject the same physical object to entirely different conceptual realities simultaneously.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../ontology/AUTHORED_CATEGORIES.md` (Kinds and types are authored as acyclic Formation DAGs)
*   `../ourverse/SECOND_PERSON_FRAMEWORK.md` (Multiplayer representation, visibility, and conflict)

---

## The Interrelation

In traditional software, an object's taxonomy is absolute and globally authoritative. If an entity is instantiated as a `class Tree`, it is a `Tree` for all players on the server. The C++ type system enforces a monolithic consensus of reality.

Earthcall deliberately shatters this consensus by replacing hardcoded classes with `AUTHORED_CATEGORIES.md`. Taxonomy—what a thing *is*—is not an innate trait; it is a `Formation` composed of `Relation` edges.

When combined with the `SECOND_PERSON_FRAMEWORK.md`, this yields a profound architectural consequence: **Taxonomy is subjective to the Law authored by the observer.**

Consider an `Object` placed in a shared Zone.
1. Person A constructs a `Formation` defining a "Weapon" category and adds this `Object` to it, writing a Law that enables "Weapons" to cause damage.
2. Person B constructs a `Formation` defining a "Sacred Relic" category, adds the *exact same* `Object` to it, and writes a Law that prevents "Sacred Relics" from moving or striking.

Because the category is just a DAG of relations, neither Person A nor Person B alters the innate substrate of the object. The engine does not crash due to a type conflict, because the engine does not care what the object is "called."

When Person A attempts to strike with the object, Person A's Law sees a Weapon and fires. When Person B observes the object, Person B's Law sees a Sacred Relic and attempts to hold it still. The conflict is not resolved by a compiler error, but by the physical interplay of their respective Laws competing for authority over the object's `PropertyPath`s.

**Conclusion:** By moving taxonomy out of the C++ type system and into authored graph relations, Earthcall guarantees that the entrance of the Second Person does not force an immediate, absolute consensus of meaning. The universe supports overlapping, contradictory ontologies governing the same physical matter.
