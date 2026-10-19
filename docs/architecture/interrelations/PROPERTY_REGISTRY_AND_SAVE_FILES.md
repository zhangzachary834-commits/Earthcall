# Property Registry and Save Files

**How the Property Registry transforms save files from ephemeral caches into the definitive, canonical reality of Earthcall.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../ontology/NO_BLACK_BOX.md` (The refusal of hidden, un-introspectable state)
*   `../../BUILD_AND_ENVIRONMENT.md` (Where data lives: the sacred nature of save files)

---

## The Interrelation

In typical software architecture, a save file is a convenient snapshot—an ephemeral serialization of runtime memory used merely to restore the application state. If the engine changes, the save file format changes or breaks, because the engine's C++ state is the primary truth.

Earthcall completely inverts this relationship. The true state of the world resides in the authored data (the saves), and the engine is merely a compiler and witness to that data. This inversion is technically enforced by the **Property Registry** (as implemented via `PropertyPath`).

### No State in Opaque C++
The "No Black Box" doctrine dictates that no domain concept or state can be hidden inside opaque C++ variables. If a state exists, it must be registered and accessible via a `PropertyPath`. Unregistered state is ungoverned, which is architecturally forbidden.

Because every meaningful aspect of a Singular, Relation, or Law must be registered as a discoverable property, the entirety of a being's state is fully legible to the serialization system.

### The Save File as Canonical Reality
When the world state is persisted to the `saves/` directory, it is not writing a lossy summary; it is writing the complete, canonical reality. The serialized property graphs capture the totality of authored laws, relationships, and physical presence.

This makes the save file sacred. It allows the authored world to survive architectural changes to the Earthcall engine itself. Because the engine cannot legally hold hidden domain state, an updated engine can cleanly re-hydrate the property structures from the save file without data loss. The Property Registry acts as the hinge that guarantees the save file, rather than the C++ memory layout, remains the true source of reality.
