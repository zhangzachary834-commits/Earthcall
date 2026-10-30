# Interaction-as-Law and Direct Screen Forms

**Status:** Architectural cross-check
**Related:** `law/INTERACTION_AS_LAW.md`, `mathematics/ONTOMATH_FRAMEWORK.md`, `Singularity/Screen/ScreenChannel.cpp`

## The Interrelation

`INTERACTION_AS_LAW.md` dictates that Earthcall rejects traditional UI widgets. Instead, interfaces are constructed by pointer events (like `object-hover-entered` or `object-clicked`) interacting with authored Laws.

Concurrently, Direct Screen Forms allow mathematical expressions (OntoMath) to directly evaluate colors and opacities at framebuffer sample centers via the `ScreenChannel`, bypassing the need for physical Objects or Meshes.

These two mechanisms interrelate to fulfill the minimum-maximum principle for user interfaces. By projecting OntoMath color fields directly to the screen and using Interaction-as-Law to observe pointer intersections with those mathematical regions, Earthcall achieves fully interactive UIs without manufacturing bespoke UI state or importing foreign widget toolkits. The interface is purely the intersection of mathematical manifestation (`ScreenChannel`) and logical observation (`Law`), perfectly aligning the UX with Earthcall's ontological semantics.
