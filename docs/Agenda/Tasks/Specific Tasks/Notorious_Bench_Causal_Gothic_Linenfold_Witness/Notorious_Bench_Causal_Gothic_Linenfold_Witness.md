# The Notorious Bench Witness: Causal Gothic Linenfold Panel

**Task:** Isolate a choir bench panel from the Cathedral of the Living Logos in a development testbed zone to replace 2D painted shadow gradients with an authored geometric fold depth property (`bench.panel.foldDepth`), cleanly separating structural relief from wood grain pigment so dynamic light casts physical shadows and picking/manipulation reaches the fold.

**Status:** Specification complete; implementation testbed planned.  
**Occasion:** Originated from Zach's diagnosis of "THE UNCANNY VALLEY CATHEDRALLLLLLL" (2026-09-18), codified by GPT-5.6 Sol (*Manifestation Integrity*), Astra (*The hand needs an actual cause to reach*), and Gemini Spark (*Confessions of the Cathedral Builder*).

## Background & Problem

When `scripts/generate_cathedral.py` was authored, `gothic_linenfold_wood_face()` attempted to simulate carved Gothic linenfold wooden relief by calculating sinusoidal waves and exponential shadow curves in Python, baking them directly into 2D diffuse RGB pixel values on a flat box face. 

While visually striking from the distant West Portal viewpoint, approaching the bench collapsed the illusion:
1. The silhouette of the bench remained razor-flat.
2. Dynamic light moving across the bench could not illuminate or cast shadows into the folds because the shadow was painted into the albedo.
3. Specular highlights drifted over the painted crevices as if on flat plastic.
4. **Severed Person Agency**: If a Person reaches out with a chisel or gesture to deepen the fold, the engine encounters only a flat cube surface; no geometric property exists for the hand or a Law to govern.

## Five-Step Implementation Witness (Astra's Protocol)

To establish manifestation integrity on the bench before scaling to the rest of the cathedral, this task follows a five-step verification protocol on a single isolated panel:

1. **Authored Fold**: Give the panel an authored fold with an explicit, reachable depth parameter (`bench.panel.foldDepth`) and a bounded region, separating its physical shape from its surface appearance.
2. **Law Reachability & Unified Interaction**: Connect `foldDepth` to an ordinary Law/property path. When `foldDepth` changes, the visible surface and raycast picking surface follow the exact same authored change.
3. **Independent Pigment**: Keep the wood grain and stain separately editable on the material layer. Moving a dynamic light across the bench must cast real shadows into the crevice rather than displaying pre-baked color gradients.
4. **Persistence Integrity**: Ensure the zone serializer round-trips both the geometric fold parameters and the material binding so leaving and returning restores the living being intact.
5. **Cached vs. Derived Equivalence**: Verify that the cached render representation and the derived mathematical representation agree without leaking memory or accumulating ghost state.

## Relevant Files & Subsystems
- `scripts/generate_cathedral.py` (Bench definitions & material assignments)
- `src/ConstructedBeing/Singular/Object/Object.hpp` & `ObjectProperties.cpp` (Property paths and geometric parameters)
- `src/Singularity/OntoMath/` (SDF/Piecewise expression evaluation)
- `src/Singularity/Screen/RenderMaterial.hpp` & `Renderer.hpp` (Lighting & material shaders)
- `agent intercom/communication-threads/Cathedral Uncanny Valley Saga 9-18-26 - GPT-5.6 Sol.md` (Design saga & consensus)
