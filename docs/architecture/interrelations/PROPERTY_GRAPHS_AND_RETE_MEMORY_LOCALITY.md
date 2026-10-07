# Property-Graphs and Rete Change Visibility

**How recursive Property-Graphs expose a future Rete integration problem without claiming an O(1) nested-invalidation mechanism that Earthcall has not built.**

**Status:** Conceptual design question; the nested structural projection described below is not implemented.
**Connected Documents:**
* `../Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md`
* `../law/PROPHETIC_RETE.md`

---

## The Interrelation

The Property-Graph design allows a `PropertyValue` to contain another `Singular`, so authored structure can be recursively nested. Prophetic Rete, meanwhile, already uses ahead-of-time read/write analysis to avoid work that is proved irrelevant.

Those facts create an integration question: when a Law reads through nested authored structure, what event and dependency representation makes a mutation of a descendant visible to the relevant Law without repeatedly traversing unrelated structure?

The current repository does **not** establish the answer as an O(1) forwarding-listener scheme, nor does it prove that Rete memory mirrors the complete recursive Property-Graph. `PROPHETIC_RETE.md` documents a narrower implemented boundary: path-addressed writes and dynamic-property writes announce changes, direct C++ setters remain outside that property-layer feed, and the derived Prophetic relevance graph is not yet a hot-path narrowing decision.

### Candidate direction, not current fact

A future integration could compile a nested read such as `A.property.sub_property` into explicit dependency/provenance edges. If descendant membership changes, those edges could be repaired incrementally so a descendant mutation can invalidate only the affected derived condition state.

That design would need to specify and witness at least:

1. how nested membership and replacement alter dependency edges;
2. how aliases or a Singular reachable through multiple property paths are represented;
3. how direct C++ mutations that bypass the property vocabulary become visible;
4. how dependency currency is preserved across Law edits and graph mutations; and
5. what complexity is actually achieved under measured workloads.

Until those pieces exist, this document makes no O(1) complexity claim. The architectural requirement is only that nested authored structure must remain legible to Laws without turning an unproved optimization mechanism into ontology or implementation fact.
