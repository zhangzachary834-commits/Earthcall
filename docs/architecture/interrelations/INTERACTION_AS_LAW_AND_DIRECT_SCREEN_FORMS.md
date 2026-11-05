# Interaction as Law and Direct Screen Forms

**Status:** Conceptual interrelation; not an implemented pointer-to-screen-field hit-test contract.

## Sources

- [Interaction as Law](../law/INTERACTION_AS_LAW.md)
- [OntoMath Raster Formation and Property-Singular-Graphs](../Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md)
- [Direct Screen Forms](../../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md)

## Boundary and possible integration

The existing interaction channel senses pointer, wheel, and key input; authored Laws can respond to addressed events and properties. A control may be an Object with an authored Law; this does **not** imply that every visible screen pixel is an Object or a pointer target.

Direct Screen Forms evaluate authored mathematical color fields for screen samples. A rendered color is an observation, **not** automatically an addressable interaction target. The raster specification explicitly distinguishes bearer identity, source fields, sensed final pixels, and bitmap ingestion; none automatically converts into the others.

A future bridge would need an explicit authored mapping from screen coordinates and sampling context to a target bearer or region, deterministic hit/selection semantics for overlapping fields, event provenance, and an authorization path for resulting Law actions. Input sensing remains distinct from Law decisions and their effects. Neither raw pointer-ray casting into arbitrary fields nor automatic event-to-Law conversion is asserted to exist today.
