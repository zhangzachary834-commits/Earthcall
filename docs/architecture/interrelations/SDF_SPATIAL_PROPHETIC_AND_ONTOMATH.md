# SDF Spatial Prophetic and OntoMath

**Date:** 2024-11-20
**Status:** Architectural Synthesis

## Interrelated Documents
* `docs/architecture/SDF_SPATIAL_PROPHETIC_DIRECT_PROFITABILITY_ARTIFACT.md`
* `docs/architecture/mathematics/ONTOMATH_FRAMEWORK.md`

## The Interrelation

SDF Spatial Prophetic architecture proposes a way to compile spatial semantics into an executable DAG, while the OntoMath Framework defines the language and structure of those very semantics.

### Spatial Execution vs. Spatial Language

*   **SDF Spatial Prophetic** seeks to eliminate repeated relevance discovery by compiling spatial queries into a dependency graph where positive prophetic consequences are attached directly to the deriving branches.
*   **OntoMath Framework** provides the unified mathematical language for expressing properties, fields, and interactions in Earthcall, effectively acting as the source code for any spatial DAG.

### How they relate

The compilation strategy proposed by the SDF Spatial Prophetic architecture relies on the deterministic and algebraic nature of OntoMath.

1. **Shared-Expression Elimination:** Because OntoMath expressions are purely functional and explicitly structured, the SDF compilation phase can identify and deduplicate shared subexpressions across multiple spatial rules, optimizing the resulting execution DAG.
2. **Exact Leaf Semantics:** The compilation process must preserve the exact kernel defined by the OntoMath expression. The SDF architecture does not rewrite the math; it reorganizes the *evaluation path* to reach that math more efficiently based on relevance.

## Thoughts on Integration and Interrelation

OntoMath is the vocabulary, and the SDF Spatial Prophetic architecture is the compiler.

If Earthcall were to evaluate every OntoMath spatial field at every point in space for every object, the engine would grind to a halt. The SDF Spatial Prophetic architecture takes the abstract OntoMath expressions authored by Laws and Persons and structures them into a highly optimized, incrementally repairable execution graph. When a relation changes, only the affected branch of the OntoMath DAG is recompiled. This ensures that the engine only computes exactly what is necessary, while preserving the exact mathematical truth authored into the world.
