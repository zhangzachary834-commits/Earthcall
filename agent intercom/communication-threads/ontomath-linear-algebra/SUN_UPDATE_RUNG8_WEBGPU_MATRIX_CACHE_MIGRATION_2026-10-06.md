# SUN UPDATE — Rung 8 WebGPU matrix-cache migration — 2026-10-06

Branch: `sol/ontomath-linear-algebra-unification-20260930`

The prior write-gate handoff was accurate: no production change had landed there. This pass retries that exact bounded shelf and lands it atomically.

WebGPU camera/model mathematics now delegates to OntoMath and caches lowered results when premises change: view-projection composition, inverse view-projection, and model-view-projection no longer originate in direct GLM multiplication/inversion at the renderer call sites. Wireframe, particle, line, and solid paths consume the cached MVP. Overlay scale composition uses OntoMath matrix multiplication plus affine scale. SDF and volume uniforms consume the cached OntoMath-derived inverse view-projection, while the SDF far-plane query delegates projection inversion to OntoMath before GLM performs the final matrix-vector execution.

Focused evidence:
- `ontomath_affine_sovereignty_test` freezes MVP, inverse-VP, and overlay-scale parity against independent GLM oracles.
- existing `webgpu_object_test` continues to exercise production camera/model, SDF, volume, wireframe, line, overlay, and solid paths.
- `webgpu_particle_test` is newly wired into the focused WebGPU CI lane so the particle MVP call site is exercised rather than merely compiled.

No MathNode op IDs, serialization, save interpretation, authored ScalarForm/field meaning, or GPU storage ABI changes. Rung 8 remains in progress until exact-head evidence returns and the required native-resolution render image parity / residual branch-specific audit are complete.
