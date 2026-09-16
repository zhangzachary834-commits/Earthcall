# Continuum as Singular: Discrete Definition of Continua and Dynamic Internal Calculations

**Status:** Open / Proposed  
**Agenda section:** Singular · Relation · Formation  
**Author:** Zach & Gemini Spark, 2026-09-15  

## The human thread

During an architectural inquiry into fluid dynamics and whether Earthcall can natively support continuum mechanics like the Navier-Stokes equations, Zach clarified a foundational principle of Earthcall's ontology:

> *"Well my vision for Singular was never to force everything into discrete numbers but rather the philosophy of discrete definitions. Every continuum or being isn’t necessarily fixed numbers of course, a continuum or being still defined in some sense by a discrete form. That discrete form would be made up of Properties belonging to a Singular who represents the continuum itself, and fluidly moving in the continuum would be dynamic calculations."* — Zach, 2026-09-10

This corrects a subtle but persistent category error: the assumption that an ontological `Singular` must always be a 0D point mass or a collection of scalar numbers, and that modeling a continuum requires fragmenting it into millions of discrete particle beings. In Earthcall, a continuum (such as an ocean, a wind current, a nebula, an atmospheric cell, or a fluid body) is itself an individual `Singular` with an author, an identity, and a discrete ontological definition.

## The architectural core: Discrete Definition vs. Dynamic Internal Calculation

Attempting to model fluid dynamics by spawning $10^5$ to $10^6$ tiny `Object` beings and running ECA Laws over them collapses the Rete network and tick agenda, and fundamentally misapprehends what a fluid is. Conversely, attempting to hardcode fluid physics as a black-box engine bypasses the Law and PropertyPath governance that defines Earthcall.

Zach's resolution establishes a three-tiered architecture:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      1. THE SINGULAR (CONTINUUM)                        │
│  The being itself (e.g. FieldNode, FluidDomain, Lake, AtmosphericCell)   │
│  Has identity, transform, author, and ontological standing in the Zone. │
├─────────────────────────────────────────────────────────────────────────┤
│                   2. THE DISCRETE DEFINITION (FORM)                     │
│  Exposed via PropertyPath:                                              │
│  - Boundary shape / container (geom::SdfNode, transform)                │
│  - Continuum constants (viscosity, density, surface tension)            │
│  - Inflow/outflow sources, external force couplings                     │
│  - Field AST expressions (field.ast)                                    │
│  GOVERNED BY LAWS VIA Map AND Flow:                                     │
│  - Law: Temperature maps to fluid viscosity.                            │
│  - Law: Rain inflows flow into total volume.                            │
│  - Law: Submerged body velocities couple into boundary momentum.        │
├─────────────────────────────────────────────────────────────────────────┤
│               3. DYNAMIC INTERNAL CALCULATIONS (SUBSTRATE)              │
│  Evaluated on the hardware continuum substrate (WGSL compute passes):   │
│  - Navier-Stokes advection, diffusion, and pressure projection          │
│  - Fluid velocity u(x, t) and pressure p(x, t) fields                   │
│  - Completely decoupled from Rete tick sweeps and entity loops          │
└─────────────────────────────────────────────────────────────────────────┘
```

This reconciles the Law system with continuum physics:
- **`Map` and `Flow` are not flawed.** They govern the *discrete definition* of the continuum being. `Flow` integrates macroscopic state over time, and `Map` relates environmental conditions to the continuum's properties.
- **Navier-Stokes is the internal calculation.** It executes on the continuous substrate bounded by the `Singular`'s discrete form, without flooding the entity graph.

## Alignment with Earthcall Doctrine

1. **Precedent in `FieldNode.hpp`:**  
   `FieldNode` already exemplifies this pattern: it is a `Singular` that anchors an `OntoMath::Field` into the scene, exposing the mathematical field's variables to the PropertyPath system so Laws can modulate the field dynamically without turning spatial coordinates into entities.
2. **Refusal #1 & Refusal #6 (No domain noun C++ classes, no black box):**  
   Continua are configured through authored properties and geometry (`field.ast`, `boundarySdf`, `viscosity`), rather than monolithic hardcoded C++ classes.
3. **`ALGORITHMS_AS_LAW.md` §8 (*When the answer is not a law*):**  
   *"A tight inner numeric kernel that is genuinely hot and genuinely sensing or acting is first-mover code... 'It's too hot to be a law' is a reason to keep the loop in C++, never a reason to keep the decision there."* The Navier-Stokes spatial solve is a First Mover kernel; the parameters, boundary conditions, and relational couplings remain fully authorable Laws.

## Concrete Work & Deliverables

1. **Formalize Continuum Properties on `Singular` (`FieldNode` / `FluidDomain`):**
   - Register standardized PropertyPaths for continuum definitions: `continuum.boundarySdf`, `continuum.viscosity`, `continuum.density`, `continuum.inflow`, `continuum.vorticity`.
   - Ensure these properties are dynamically mutable by `ActionNode::Kind::Map` and `ActionNode::Kind::Flow`.
2. **First Mover Continuum Execution Contract:**
   - Define the interface between a `Singular` continuum's discrete definition and the underlying GPU/WGSL compute pass.
   - When a Law alters a continuum property (e.g. `viscosity` changes), the First Mover channel receives the updated uniform without stalling the simulation or breaking invariance.
3. **Boundary Coupling with Other Singulars:**
   - Define how rigid or articulated `Object` instances intersecting the continuum's boundary SDF inject momentum sources or boundary obstacles into the internal calculation.
4. **Verification & Testing:**
   - Author a test verifying that modulating a continuum `Singular`'s properties via Law updates the internal calculation parameters without generating per-particle entities or bloating the Rete network.
