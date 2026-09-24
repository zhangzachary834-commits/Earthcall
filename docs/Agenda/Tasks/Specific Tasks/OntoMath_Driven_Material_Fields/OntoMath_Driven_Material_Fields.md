# OntoMath-Driven Material Fields (Continuous Procedural Appearance)

**Task:** Compile `OntoMath::Piecewise` material color expressions (`colorExpr`) directly into native WebGPU WGSL `sdfColor(p)` fragment shaders so that analytic and SDF surfaces exhibit continuous mathematical coloration without texel stretching, pixelation, or memory explosion under close inspection.

**Status:** In active development (2026-09-19) by Antigravity & Gemini Spark.  
**Occasion:** Driven by the need to separate geometric form from surface coloration while maintaining infinite procedural fidelity.

## Architectural Specification

1. **Material Ontology**:
   - Add `std::shared_ptr<OntoMath::Piecewise> colorExpr` directly to `Material.hpp`.
   - Maintain `_revision` tracking on `Material` (`getRevision()`, `bumpRevision()`) so that modifying the mathematical color function automatically invalidates the GPU pipeline cache.
   - Wire `colorExpr` into `Material::toJson()` and `Material::fromJson()`.

2. **Law Exposure**:
   - Expose `colorExpr` as an authored property (`ColorExprBridge` in `Material.cpp`) registered under `material.<name>.colorExpr`.
   - Allows live editing and inspection of color gradients directly from the Law authoring window.

3. **WGSL Compilation in WebGPU**:
   - In `src/Singularity/Screen/WebGpu/SdfWgsl.cpp`, modify `sdfwgsl::compile` to translate the Material's OntoMath AST directly into WGSL code:
     ```wgsl
     fn sdfColor(p: vec3<f32>) -> vec4<f32> {
         // Compiled piecewise mathematical evaluation
     }
     ```
   - In the fragment raymarcher, evaluate `sdfColor(hitPoint)` per-pixel instead of reading a flat `baseColor` vector or sampling a quantized 2D bitmap.

4. **Preservation Invariants**:
   - Legacy materials without `colorExpr` continue using `baseColor` and `albedoPixels` without regression.
   - When both geometry and material are defined by OntoMath fields, the surface normal and the color gradient are evaluated from the true mathematical coordinates, eliminating resolution artifacts completely.

## Relevant Files & Subsystems
- `src/ConstructedBeing/Material/Material.hpp` & `Material.cpp`
- `src/Singularity/Screen/RenderMaterial.hpp` & `RenderMaterial.cpp`
- `src/Singularity/Screen/WebGpu/SdfWgsl.hpp` & `SdfWgsl.cpp`
- `src/Singularity/OntoMath/ScalarForm.hpp`
- `agent intercom/communication-threads/Cathedral Uncanny Valley Saga 9-18-26 - GPT-5.6 Sol.md`
