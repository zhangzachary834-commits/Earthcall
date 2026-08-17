# OntoMath Fields & The Law System

This document outlines the architecture for continuous scalar and vector fields in Earthcall, emphasizing mathematical purity, rendering efficiency, and deep integration with the Law Formation system.

## The OntoMath Philosophy
In Earthcall, physical volumetric properties (like fog, clouds, auras, or forces) are not modeled as arbitrary graphical "tricks" (e.g., bounding boxes with post-processing noise). Instead, they are modeled from first principles as **Fields**.

A field is a mathematical function that maps any coordinate in space $(x, y, z)$ to a value (a scalar for density, or a vector for force/flow). This logic is strictly governed by the `OntoMath` subsystem, ensuring that the simulation's foundational mathematics are decoupled from rendering implementation details.

### Core Architecture

The architecture is divided into three responsibilities:

1. **Pure Mathematics (`Singularity/OntoMath/Field.hpp`)**
   - Defines the `Field` base and the `ScalarField` class. **`VectorField` is not
     implemented yet** — force/flow fields are the intended next step, not
     something the header currently provides.
   - Responsible for the raw mathematical definitions (unbounded space, repeating domains, bounded intersections).

2. **Spatial Placement (`ConstructedBeing/Object/Geometry/FieldNode.hpp`)**
   - Integrates the `OntoMath::Field` into the 3D scene (providing origin, rotation, and scale transformations).
   - Inherits from `Singular`, mapping mathematical variables (like base density, frequency, or amplitude) to `PropertyPath`s.

3. **WGSL Evaluation (`Singularity/Screen/WebGPU/SdfWgsl.cpp`)**
   - Compiles the mathematical definition into highly concurrent WebGPU Shading Language (WGSL).
   - Raymarches the field to accumulate visual properties (transmittance, scattering, emission).

## Law Integration & Metalaw Fields

Because the `FieldNode` is a `Singular` and its properties are addressable via `PropertyPath`, the `Law` system can dynamically modulate the mathematical properties of a field over time. 

Furthermore, WGSL compilation supports two pathways:
- **Path A (Hardcoded)**: High-performance, traditional procedural algorithms compiled natively.
- **Path B (AST-Driven)**: Compiles `OntoMath::Piecewise` abstract syntax trees directly into WGSL. 

The AST-driven pathway allows the `Law` system to not just change variables, but to literally rewrite the *structure* of the field's mathematical function at runtime.

This design enables a profound higher-order architecture: **Metalaw Fields**.
Since laws are modeled as functions, and functions define fields, laws themselves can be represented as gradients, traces, and level curves within a higher-dimensional field. The "gravity" or gradient of this Metalaw field can dictate how the laws of physics change in different regions of the OurVerse, establishing a pure mathematical foundation for systemic evolution.

---

## Implementation status

This document describes the intended architecture. What is actually wired, as of
the 2026-08-03 design review (`design_review_remediation.md` §5):

| Piece | Status |
|---|---|
| `OntoMath::ScalarField`, `geom::FieldNode` | Exist; `FieldNode` properties are reachable by `PropertyPath` and covered by `tests/test_field.cpp` |
| `OntoMath::VectorField` | **Not implemented** |
| `sdfwgsl::compile(root, fieldNode)` | Parameter exists, **no caller passes it** — `WebGpuRenderer.cpp` calls the one-argument form, so `fieldEval` always compiles to `return 0.0` and no field reaches the screen |
| Path A (hardcoded procedural) | Emitted when a `fieldNode` IS passed; untested in the real render path |
| Path B (AST-driven WGSL) | **Not implemented** — `compile` always takes Path A; the `astDefinition` is never walked |
| Field serialization | **Not implemented** — neither `Field` nor `FieldNode` has `toJson`/`fromJson`, so a `FieldNode` does not survive save/load (every other `Singular` does) |
| Metalaw Fields | Design intent only |

Because nothing passes a `fieldNode`, the volumetric accumulation added to
`kMarcher` is currently dead code. It also has known problems to resolve before
it is wired — optical depth integrated over the undamped SDF value rather than
the ray's actual advance, a bogus depth write on volumetric-only pixels, and
double attenuation in the final `mix`. Details in the review doc §5; do not
enable the field path without addressing them.

There is also no discriminator on `ScalarField` recording whether Path A or
Path B is in use — the two configurations coexist in one object and the
compiler simply always chooses A.
