# HTML Bridge and OntoMath Raster Formations

**How Earthcall bridges discrete external web documents into continuous, natively interactive spatial fields.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../Integration/HTML_LEXEME_FORMATION_BRIDGE.md` (Translating DOM into Lexeme Formations)
*   `../Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md` (Continuous OntoMath regions governing interactive entities, avoiding atomicity explosion)
*   `../Design/Building 2D and 3D Apps with Earthcall Guide.md` (Authoring UIs through Law and OntoMath)

---

## The Interrelation

The "HTML Lexeme Formation Bridge" establishes how a rigid, external DOM hierarchy is translated into Earthcall's structural vocabulary of Lexemes, Singulars, and Relations. However, having a graph representation is not sufficient for rendering or interaction; mapping a massive DOM tree into individual 3D objects with discrete meshes would cause an immediate "atomicity explosion" in both memory and the renderer.

This is where the "OntoMath Raster Formation and Property Graphs" architecture steps in to unify the foreign structure with Earthcall's spatial reality.

Instead of treating every HTML `<div>` or `<button>` as a separate physical C++ `Object` with its own geometry and physics collider, the ingested DOM Formation is mapped onto continuous **OntoMath fields**.
1. The entire visual output of the bridged page acts as a `FaceTexture` on a macro Singular.
2. The discrete semantic boundaries of the HTML elements are translated into analytical bounding conditions (indicator functions and SDFs) within an `OntoMath::Piecewise` structure.
3. The engine elevates these bounds into logical sub-region Singulars.

Because the HTML element is now defined by continuous mathematical space rather than a rigid array of pixels, Earthcall's **Interaction as Law** acts upon it seamlessly. A Person's pointer isn't interacting with a black-box WebView or a rigid collision mesh—it is evaluating an OntoMath SDF region, which perfectly maps to the underlying semantics of the bridged HTML Formation.

**Conclusion:** The HTML Bridge gives foreign pages structural meaning, but the OntoMath Raster Formations give them physical, spatial form without breaking the Rete or the GPU. Together, they allow external web applications to exist in Earthcall not as flat projections, but as fully reactive, Law-governed objects.

**Author**: Jules (Claude 3.5 Sonnet)
**Session ID**: 5938271034
