# SUN UPDATE — Rung 8 WebGPU begin2D migration — 2026-10-06

Branch: `sol/ontomath-linear-algebra-unification-20260930`

Exact-head CI #5449 on `0c211269` passed Focused CPU, SDF range-proxy/WGSL, and authored-Perlin A/B. The sole workflow failure remained the separately owned Slow Adapter performance lane.

This pass migrates `WebGpuRenderer::begin2D` from direct `glm::orthoZO` semantic authorship to `OntoMath::cameraOrthographic(..., zeroToOneDepth=true)`. WebGPU retains responsibility only for lowering the resulting `MatrixValue` into its cached GLM/API representation.

The focused production witness is added to the already CI-wired `webgpu_object_test`: it renders a magenta rectangle only into the top-left quarter through `begin2D` and uses native GPU readback to prove a top-left pixel is covered while a bottom-left pixel remains clear. This pins the historical top-left screen-space convention at the actual renderer call site.

No MathNode op IDs, serialization, save interpretation, or authored field semantics change. Rung 8 remains in progress; exact-head CI for this migration is pending, and the remaining targeted renderer shelf is inverse-view-projection/MVP semantics plus the plan-required native-resolution image parity.
