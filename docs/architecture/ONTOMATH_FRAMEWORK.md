# OntoMath Framework

The Earthcall ontology separates the underlying mathematical rules of reality from the engine logic that executes them. `OntoMath` is the multi-class module that encapsulates pure mathematics, separating mathematical constructs from any particular rendering framework or API like WebGPU or Audio.

## 1. Why OntoMath?

In most traditional game engines, mathematical behaviors like noise fields, velocity, or volumetric densities are tightly coupled with the physics engine or the rendering shaders. In Earthcall, this coupling is an ontological violation. What something *is* mathematically must be distinct from how a *specific channel* renders it.

If a `FieldNode` in Geometry needs a procedural density map, it does not define a custom WebGPU node. Instead, it maintains a reference to an `OntoMath::ScalarField` or `OntoMath::VectorField`. The WebGPU modality compiles the WGSL shader by reading the `OntoMath` structures. The Physics modality reads the exact same `OntoMath` structures to apply wind or gravity.

## 2. Representation: The `MathNode` AST and `Piecewise` Functions

The foundation of `OntoMath` is exact symbolic math, avoiding approximations where possible.

### `ScalarForm`
`ScalarForm` represents a continuous algebraic expression: a sum of terms where a term is `coefficient * Π var^exp * Π trans(scale·var + shift)`. Within it, algebra (+, ×) and basic calculus (derivatives) are exact.

### `MathNode` (AST)
When we need complex vector calculations (Dot products, Cross products) or structured combinations of scalars, we construct a tree of `MathNode`s. 

Supported Operations:
- `ScalarLeaf`: Contains a `ScalarForm`.
- `VectorConstruct`: Builds a vector from scalar children.
- Math ops: `Add`, `Sub`, `Scale`, `Dot`, `Cross`, `Normalize`, `Length`, `Map`.

### `Piecewise`
Mathematical laws in reality often only apply under certain conditions (e.g., gravity changes beyond the atmosphere). `Piecewise` functions define the interval or *condition* bounds where a specific `MathNode` AST applies.

## 3. Shader Integration (The Dual Path)

`OntoMath` is designed to power the Law system. When generating highly parallelized code like WGSL for WebGPU, we rely on two paths based on the `Field`'s `EvaluationMode`:

1. **Path A (Procedural/Hardcoded):**
   The field's properties (like `baseDensity` or `frequency`) are bound as dynamic uniform variables in the shader. The Law system can modify these properties via `PropertyPath` in real-time without recompiling the shader. This is ideal for continuous modulation.

2. **Path B (AST Compilation):**
   If the `Field` requires a custom mathematical equation authored by a Person via a Law, the `mode` is set to `AST`. The shader compiler (`SdfWgsl.cpp`) traverses the `OntoMath::Piecewise` tree and transpiles it into literal WGSL string operations. Changing the AST triggers a shader recompile, providing infinite mathematical flexibility.

## 4. Sub-Modules of OntoMath
- `ScalarForm`: Multivariate signomial algebra and exact calculus.
- `Field`: Continuous Scalar and Vector fields.
- `CurveModel`: 1D time-based parameter curves.
- `ProbabilityForm`: Stochastic modeling.

By centralizing these in `OntoMath`, Earthcall ensures that any mathematical law authored by a `Person` can be reliably executed by any sub-system.
