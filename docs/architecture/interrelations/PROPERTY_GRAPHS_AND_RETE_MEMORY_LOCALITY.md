# Property-Graphs and Rete Memory Locality

**How the Prophetic Rete network maintains O(1) condition evaluation when singulars are recursively nested inside the properties of other singulars.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md` (Singulars existing as properties of other Singulars)
*   `../law/PROPHETIC_RETE.md` (Ahead-of-time law condition caching and Alpha/Beta memory)

---

## The Interrelation

The `ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md` specification formalizes a radical architectural capability: a `PropertyValue` can hold an entire `Singular` being. This creates a recursive Property-Graph. A 2D image is not a texture; it is a root `Singular` whose properties hold thousands of pixel `Singular`s.

This presents a massive theoretical problem for the `PROPHETIC_RETE.md` network. The Rete network achieves performance by caching conditions (Alpha memory). When an entity mutates, the EventBus notifies the Rete, which instantly updates the specific laws that care about that entity.

But if a pixel `Singular` (which is deeply nested inside the `image` property of a `Canvas` Singular) changes its color, how does the Rete network know to trigger a Law that says `If Canvas.image contains red`? If the EventBus only broadcasts "Pixel #4025 changed", the Rete has no way to associate that pixel with the root Canvas without performing an O(N) traversal of the entire Property-Graph every frame, instantly destroying the engine's performance.

### Structural Alpha-Memory Projection

The interrelation requires that the Rete's Alpha memory must map directly onto the recursive topology of the Property-Graph.

When a Law condition is compiled (e.g., `If A.property.sub_property == X`), the Rete network cannot just register a listener on `A`. It must inject forwarding listeners down the property chain.

When the pixel Singular is inserted into the Canvas's property, the Rete network dynamically binds the pixel's local mutation events to the Alpha memory node associated with the root Canvas. Therefore, when the pixel mutates, it fires a localized event that is structurally projected up the graph in O(1) time, directly invalidating the cached Rete condition for the root Law.

Without this interrelation, the recursive Property-Graphs of the Design spec would be completely invisible to the Laws meant to govern them, breaking the fundamental promise of Earthcall's Legibility doctrine. The topological structure of the Rete memory must exactly mirror the topological nesting of the Singulars.
