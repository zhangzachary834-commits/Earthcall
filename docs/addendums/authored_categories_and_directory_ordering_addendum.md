# Addendum: Integrating Authored Categories and Directory Ordering

*(Model: Claude 3.5 Sonnet, Harness: Jules, Session ID: 32462617945413787)*

## Reflections on the Architectural Synthesis

When examining Earthcall's structural directives—specifically `AUTHORED_CATEGORIES.md`, `DIRECTORY_ORDERING.md`, and `NEW_KIND_FRAMEWORK.md`—it becomes clear that the physical layout of the repository is not merely a matter of housekeeping, but a direct enforcement of the underlying ontology.

A core principle of Earthcall is the rejection of hardcoded C++ domain types (e.g., `RobotEntity`, `FurnitureKind`). Instead, the system mandates that domain nouns are represented as *Authored Categories*—dynamic, legible semantic entities (like `ObjectConcepts` and `Formations`) that exist within the world's graph, where they can be governed, modified, and reasoned about by Laws and Persons.

### The Directory as Ontology

This conceptual discipline is mirrored and enforced at the repository level. Just as a `Robot` is not a C++ class, `Robotics/` is not a top-level directory. If the directory tree were to allow a `Robotics/` folder alongside core systems, it would falsely communicate that "Robotics" is a peer to the fundamental ontology of the engine, rather than an authored construct built *on top* of it.

By maintaining strict `DIRECTORY_ORDERING` (where the root directory only contains ontological regions like `Singularity/`, `ConstructedBeing/`, and `Relation/`), the codebase prevents the subtle re-introduction of rigid domains. The physical structure of the files teaches every contributor the true shape of the system: new features and domains must be built using the `NEW_KIND_FRAMEWORK`'s primitives (Objects, Properties, Relations), not by carving out new permanent C++ silos.

### Enforcing the Truth

This synergy between folder structure and category authorship ensures that the system remains an open, general-purpose substrate. It prevents the engine from silently calcifying around specific use cases, keeping the power to define "what things are" in the hands of the Persons inhabiting the world, rather than the original C++ authors.

---

**Linked References:**
* [Authored Categories](../architecture/ontology/AUTHORED_CATEGORIES.md)
* [Directory Ordering](../architecture/ontology/DIRECTORY_ORDERING.md)
* [New Kind Framework](../architecture/ontology/NEW_KIND_FRAMEWORK.md)
