# Zone Manager Active Index and Ontological Void Safety

**How strict array bounds checking grounds Foreign Modalities in physical reality.**

**Status:** Conceptual interrelation.
**Connected Systems:**
*   `src/Singularity/Foreign/API/EarthcallAPI.cpp`
*   `src/Singularity/Network/WebSocketServer.cpp`

---

## The Interrelation

In the `WebSocketServer` and `EarthcallAPI`, all attempts to access the world—whether iterating over objects, transforming them, or building a world snapshot—are strictly guarded by `!mgr.zones().empty() && mgr.currentIndex() < mgr.zones().size() && mgr.zones()[mgr.currentIndex()]`.

This is not merely C++ memory safety; it is an enforcement of **Ontological Void Safety**. Earthcall prevents the Foreign Modality (such as WASM scripts, Web UI clients, or external API calls) from acting upon a nonexistent world. Because the foreign modality operates on standard native ontology rather than maintaining a parallel state, it is bound by the same physical constraints as native laws: if there is no active `Zone` loaded, there is no spatial or relational substrate to manipulate.

By strictly validating the active Zone index before any foreign intervention, Earthcall ensures that all foreign influence remains grounded in an instantiated physical reality, completely preventing operations in the void.
