# Append-Only Enums and Authored Categories

**How the rigid immutability of the C++ enum ensures the infinite malleability of authored domains.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../ontology/AUTHORED_CATEGORIES.md` (Categories are rooted acyclic Formations of beings, not C++ types)
*   `AGENTS.md` (Refusal 3: No new enum value for a kind of thing. Append-only, serialized as integers)

---

## The Interrelation

Earthcall imposes a draconian rule on its type system: **No new enum value for a kind of thing.** C++ enums like `BeingKind` or `ActionNode::Kind` are append-only and serialized as integers. They represent the absolute structural bedrock of the machine, not semantic categories of the domain.

Simultaneously, the ontology insists that **Categories are Authored.** If a Person wants to define a "Tree" or a "Vehicle" or a "Spell," they do not add an enum member. They author a Formation—a structured graph of registered beings—that defines the category in-world.

The interrelation between the append-only enum and the authored category is the boundary between the engine's physics and the Person's meaning. The enum provides the invariant mathematical and structural primitives (e.g., this is a *Singular*, this is a *Relation*, this is a *Set* operation). Because these primitives never shift, never reorganize, and never renumber, the engine can execute them with extreme efficiency and absolute predictability.

This rigid bedrock is what makes Authored Categories possible. A Person can construct an infinitely complex hierarchy of meaning—defining a taxonomy of magic or biology—because the underlying substrate (the enum) will not unexpectedly change and break the authored graph.

If we allowed domain categories into the C++ enum, we would be moving meaning out of the hands of the Person and into the hands of the compiler. By strictly separating the two—C++ enums for immutable structure, authored Formations for mutable meaning—we ensure that the Person remains the sole author of their world, while the machine remains a reliable vessel. The append-only rule is not just a serialization safeguard; it is a profound declaration of architectural boundaries.
