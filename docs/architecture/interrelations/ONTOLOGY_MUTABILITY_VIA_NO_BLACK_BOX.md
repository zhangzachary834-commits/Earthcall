# Ontology Mutability via No Black Box

**How the refusal of hidden state is the precise mechanism that allows taxonomy to be authored in-world.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../ontology/NO_BLACK_BOX.md` (Every field is registered, readable by law, writable unless derived)
*   `../ontology/AUTHORED_CATEGORIES.md` (Kinds and types are authored as acyclic Formation DAGs)

---

## The Interrelation

The "Authored Categories" document describes how Earthcall refuses C++ classes (like `RobotEntity`) and instead forces users to author a semantic DAG of `Singular`s and `Relation`s to define what things *are*.

However, simply defining a category DAG (e.g., this `Object` is part of the `Tree` Formation) is useless if the behaviors, states, and properties that make a tree a tree are hidden in private C++ member variables.

This is where "No Black Box" becomes the mechanical bridge. Because *every* property of *every* being must be registered and exposed to the Law system, the authored categories have actual material to govern.

When a Person authors a new category (e.g., a "Flammable" category), they can write a Law that applies to anything in that category. This Law can only function if the target properties (like `temperature`, `combustion_threshold`, or `is_burning`) are exposed and mutable. "No Black Box" ensures that the ontological categories defined in `AUTHORED_CATEGORIES.md` have total reach into the state of the world.

If "No Black Box" were violated, the authored categories would be shallow labels. A Person could categorize something as a "Tree," but they wouldn't be able to govern its internal sap flow or leaf count because those would be trapped in C++.

**Conclusion:** The exposure of all state ("No Black Box") is the necessary precondition for runtime taxonomy ("Authored Categories"). Without total property introspection, authored categories are just strings; with it, they are the governing architecture of reality.
