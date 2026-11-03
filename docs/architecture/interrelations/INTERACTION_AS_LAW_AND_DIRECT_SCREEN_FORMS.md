# Interaction-as-Law and Direct Screen Forms

**How pointer events directly actuate math-based framebuffer expressions.**

**Status:** Conceptual interrelation.

## Interrelated Documents

* `../law/INTERACTION_AS_LAW.md`
* `../Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md`

## Thoughts on Integration and Interrelation

In Earthcall, the "Interaction-as-Law" architecture fundamentally rejects the use of traditional UI widgets or object-based input wrappers. Instead, user intent—such as pointer events and control patterns—interacts directly with authored Laws that govern the world.

This approach interrelates deeply with the concept of "Direct Screen Forms." Rather than relying on underlying physical objects or polygon meshes to construct user interfaces, Direct Screen Forms evaluate OntoMath expressions directly at framebuffer sample centers (via `ScreenChannel.cpp`).

By bridging Interaction-as-Law and Direct Screen Forms, Earthcall allows raw pointer rays to cast into pure mathematical fields, resolving interactions against mathematically rasterized UIs without any intermediate black-box UI framework. The interface is simply another domain where laws meet mathematics.
