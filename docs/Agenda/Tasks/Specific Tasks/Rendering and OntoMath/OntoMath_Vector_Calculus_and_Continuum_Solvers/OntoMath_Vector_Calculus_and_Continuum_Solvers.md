# OntoMath Vector Calculus and Continuum Field Solver Substrate

**Status:** Open / Proposed  
**Agenda section:** Modalities · integration · substrate  
**Author:** Zach & Gemini Spark, 2026-09-15  

## The human thread

An architectural inquiry into whether Earthcall can natively evaluate or simulate the Navier-Stokes equations clarified the boundaries of `OntoMath` and the Law action model:
- `Map` is local algebraic assignment ($y := f(x_1, \dots, x_n)$).
- `Flow` is forward Euler integration on isolated properties of discrete entities ($\dot{y} = f \implies y_{t+1} = y_t + f \cdot \Delta t$).
- Navier-Stokes, by contrast, is a non-linear continuum PDE requiring spatial vector calculus ($\nabla, \nabla \cdot, \nabla^2$), advection schemes, and a global elliptic boundary-value solve (the pressure Poisson equation $\nabla^2 p = -\rho \nabla \cdot (\mathbf{u} \cdot \nabla \mathbf{u})$ enforcing $\nabla \cdot \mathbf{u} = 0$).

Attempting to force a global elliptic PDE solver into the pointwise `MathNode` AST risks repeating the precedent of `Raycast = 16` and `LineIntegral = 19` (`ONTOMATH_FRAMEWORK.md` §5), which were added to `MathNode::Op` but refuse to evaluate because an AST node cannot hold a marching budget, spatial memory grid, or quadrature rule.

## Architectural division of labor

To support spatial physics (fluids, heat diffusion, wave propagation) without violating Earthcall's exact calculus contract, the system must separate **local field vector calculus** from **continuum PDE solvers**:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      ONTOMATH (EXACT / LOCAL AST)                       │
│  MathNode::Op additions for spatial differential operators:             │
│  - Divergence:   VectorField -> ScalarField                             │
│  - Laplacian:    Field -> Field (diffusive operator)                    │
│  - Curl:         VectorField -> VectorField (vorticity)                 │
│  - Convection:   (u · ∇)w directional transport                         │
│  Evaluates analytically or via local finite-difference stencils.        │
├─────────────────────────────────────────────────────────────────────────┤
│                                   ▲                                     │
│                                   │ parameters, boundary SDF, AST       │
│                                   ▼                                     │
├─────────────────────────────────────────────────────────────────────────┤
│                 FIRST MOVER CONTINUUM SOLVER (SUBSTRATE)                │
│  Lives beneath Singularity/ (WGSL compute pipelines / C++ kernel):      │
│  - Spatial grids, 3D texture buffers, or SPH particle buffers           │
│  - Semi-Lagrangian advection (tracing characteristic streamlines)       │
│  - Iterative linear solver (Conjugate Gradient / Multigrid for Poisson) │
│  - Reports macro-state back to Singular properties                      │
└─────────────────────────────────────────────────────────────────────────┘
```

## Deliverables and Acceptance Criteria

1. **Formalize Spatial Vector Calculus in `MathNode::Op`:**
   - Add enum values for `Divergence`, `Laplacian`, `Curl`, and `ConvectiveDerivative`.
   - Implement analytical derivation in `ScalarForm` where closed-form solutions exist, with explicit fallback stencils for discrete sampling.
2. **First Mover Continuum Solver Architecture (`Singularity/Continuum/`):**
   - Implement the Navier-Stokes / fluid solver as a First Mover channel utilizing WebGPU compute shaders.
   - Decouple the continuum memory grid from the Rete entity network.
3. **PropertyPath Exposure:**
   - Expose the fluid's macro-measurements (e.g. `averageVelocity`, `totalKineticEnergy`, `maxVorticity`, `surfaceLevel`) as read-only properties on the continuum `Singular` for Law conditions.
4. **Parity and Safety:**
   - Ensure the continuum solver obeys `ALGORITHMS_AS_LAW.md` §8: the heavy numerical loop runs on the hardware substrate, but all boundary interactions, source inflows, and material constants are driven by authored Laws.
