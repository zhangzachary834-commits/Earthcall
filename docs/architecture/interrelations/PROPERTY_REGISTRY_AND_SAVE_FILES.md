# Property Registry and Save Files

**How addressable authored state and durable world saves reinforce one another without treating registration as automatic persistence.**

**Status:** Conceptual interrelation; persistence completeness must be demonstrated by executable round-trip witnesses.
**Connected documents:**
- [No Black Box](../ontology/NO_BLACK_BOX.md)
- [Build and Environment](../../BUILD_AND_ENVIRONMENT.md)

---

## The interrelation

Earthcall treats authored world data as authoritative rather than making an incidental C++ layout the definition of a world. The Property Registry and `PropertyPath` make a being's meaningful properties legible to Laws and authors. Save/load machinery is a *separate* obligation: addressability does not itself serialize a field, guarantee an identity survives hydration, or prove that every authored relation and law has been persisted.

### Legibility is not serialization

The No Black Box refusal distinguishes a being's meaningful, registered state from explicitly named machine mechanisms beneath the Kernel. Computed properties, transient derived data, and caches need not all be serialized as independent values. A registered property can also lack a persistence adapter; that is a coverage gap to test, not proof that the saved world is complete.

### Authored saves as the durable authority

For authored state that is persisted, the save tree must preserve identities, relations, laws, and their intended meanings across engine versions. The engine should reconstruct its derived projections from that source, rather than silently substituting defaults or overwriting authored history. This is an architectural requirement, not an assertion that current save formats have already achieved perfect round-trip fidelity.

### The witness obligation

A change touching properties or save/load must verify the actual serializer and hydrator, including version migration, stable identity, absent/unknown fields, and authored save-tree isolation in tests. A clean C++ build or the presence of a `PropertyPath` alone cannot certify that the world survives a save/load cycle.
