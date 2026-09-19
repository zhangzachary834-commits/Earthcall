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
