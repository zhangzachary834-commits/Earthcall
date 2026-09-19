# OntoMath as Interaction Surface

**How pure mathematical fields unify visual representation and tactile interaction, ensuring interfaces are subject to exact physical laws rather than discrete bounding boxes.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../mathematics/ONTOMATH_FRAMEWORK.md` (The exact mathematics of space, curves, and fields)
*   `../law/INTERACTION_AS_LAW.md` (Interaction via Raycast/Pick rather than UI widgets)

---

## The Interrelation

In traditional UI architectures, rendering and hit-testing are separate concerns. A button is drawn as a collection of pixels or polygons, while its interactable area is defined by a separate rectangular bounding box or discrete collider mesh. This separation allows visual shape and physical presence to diverge.

Earthcall eliminates this divergence through the interaction of OntoMath and the Interaction Law framework.

`INTERACTION_AS_LAW.md` establishes that all UI interaction (hovering, clicking) happens through raycasting into the physical 3D world against actual `Object`s, rather than via a 2D overlay widget tree.

`ONTOMATH_FRAMEWORK.md` establishes that the shape and form of objects are defined not by polygon meshes, but by continuous, closed-form mathematical functions (e.g., SDFs).

When a Person clicks their mouse to interact with a control, the `Interaction` channel raycasts into the scene. Because the shapes are defined by OntoMath, this raycast is an exact mathematical intersection against the `ScalarField` evaluating the SDF, not a check against an approximated bounding volume.

This has profound architectural implications:
1. **Total Integrity:** The visual surface of the object and its tactile interactive surface are identically the same mathematical equation. You cannot click "outside" the visual bounds of an SDF button, nor can part of the button be visually present but unclickable.
2. **Infinite Resolution:** A Person can author a complex, spiraling curve as a control. The UI hit-testing works flawlessly at any zoom level because it evaluates the curve's exact parametric equation, rather than a jagged discretized mesh.
3. **Law-Governed Shapes:** Because the interactive shape is an OntoMath AST, it can be dynamically modified by Laws (e.g., a Law that warps space via a transformation node). The UI's hit area will instantly and exactly reflect this spatial warp without requiring any bounding box recalculations.

**Conclusion:** By defining objects via OntoMath and forcing interaction through environmental raycasting, Earthcall ensures that User Interface is not a separate logical layer, but an exact, tactile manifestation of the underlying spatial mathematics.
