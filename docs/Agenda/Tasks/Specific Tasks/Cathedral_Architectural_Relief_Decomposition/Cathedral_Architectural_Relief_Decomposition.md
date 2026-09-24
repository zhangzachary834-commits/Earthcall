# Cathedral Architectural Relief Decomposition (Post-Uncanny-Valley)

**Task:** Progressively migrate Cathedral of the Living Logos architectural elements (piers, fluting, baldachin relief, chancel steps, window tracery) from flat textured primitives into true constructive geometric and bounded SDF forms that reward close-range inspection.

**Status:** Proposed & specified (2026-09-18).  
**Occasion:** Resolving the "Uncanny Valley Cathedral" phenomenon by ensuring that approaching surfaces rewards the Person with genuine geometric detail and physical shadows rather than painted deception.

## Principles of Decomposition

1. **Pigment vs. Form**:
   - Flat surfaces that are physically flat in stone (Cosmati inlaid marble pavement, illuminated manuscripts, altar vestment damask) remain high-resolution flat textures ($256 \times 256$ or OntoMath appearance fields).
   - Any surface presenting three-dimensional architectural relief (column fluting, capitals, chamfers, tracery, carved oak panels) must be constructed with real geometry.

2. **Phase 1: Choir Stalls & Pew Benches**:
   - Implement the five-step witness on `cathedral.bench.nave.*`: replace the flat box + painted wave shader with a true carved panel having an authorable `foldDepth` property.

3. **Phase 2: Piers & Clerestory Columns**:
   - Decompose monolithic cylinders into bundled shafts or fluted columns with explicit toroidal torus-bases and bell capitals, so moving lights cast vertical shadow fluting down the nave.

4. **Phase 3: High Altar Baldachin & Cathedra**:
   - Replace textured cuboid blocks with stepped moldings, pediment cornices, and true architraves.

## Relevant Subsystems
- `scripts/generate_cathedral.py`
- `saves/zones/Cathedral of the Living Logos/zone.json`
- `src/ConstructedBeing/Singular/Object/Geometry/`
- `agent intercom/communication-threads/Cathedral Uncanny Valley Saga 9-18-26 - GPT-5.6 Sol.md`

## Addendum — Form and Pigment as a Unified Field

**Originating connection by:** Jules / Claude (default harness)
**Session ID:** 7602167438967080663
**Date:** 2026-09-20

The decomposition of the Cathedral from flat textures to true SDF geometry (to resolve the Uncanny Valley) is only half the solution. As described in [`../OntoMath_Driven_Material_Fields/OntoMath_Driven_Material_Fields.md`](../OntoMath_Driven_Material_Fields/OntoMath_Driven_Material_Fields.md), material coloration must also migrate from pixelated bitmaps to continuous mathematical expressions.

If we decompose a monolithic cylinder into a true fluted column, but map a low-resolution stone texture onto it, the illusion still breaks upon close inspection. The Cathedral's architectural relief must be bound directly to OntoMath-driven material fields. The depth of the carved elements or the fluting of the piers should drive the `colorExpr` mathematically—for example, accumulating procedural dirt or shadow in the deeper crevices of the SDF surface via the continuous WGSL `sdfColor(p)` evaluation.

True geometric form demands true continuous pigment.
