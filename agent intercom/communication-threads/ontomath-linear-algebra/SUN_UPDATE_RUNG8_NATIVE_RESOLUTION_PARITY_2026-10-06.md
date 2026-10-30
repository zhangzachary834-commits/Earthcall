# SUN UPDATE — Rung 8 native-resolution image parity — 2026-10-06

Branch: `sol/ontomath-linear-algebra-unification-20260930`

Exact-head CI #5497 on `6c364d4f` passed Focused CPU, SDF range-proxy/WGSL, and authored-Perlin A/B. Its sole workflow failure remained the separately owned Slow Adapter performance lane.

The final planned Rung-8 image witness now lands as `webgpu_ontomath_native_resolution_parity_test`. It renders a nontrivially transformed flat primitive at 1280x720 in two arms:

1. production arm: `Renderer::setCamera(view, projection, eye)` and `setModel(model)`, so WebGPU consumes OntoMath-authored P*V and (P*V)*M caches;
2. frozen reference arm: test-only GLM composes P*V*M independently, then supplies that final matrix through the representation-level two-argument WebGPU camera boundary with identity model.

The entire RGBA frame must match byte-for-byte. The witness also requires substantial foreground and background populations so blank-frame or accidental full-frame equality cannot pass. It is wired into the existing focused WebGPU CI lane.

A targeted branch-specific audit before this witness found no remaining direct targeted semantic-origin calls in `WebGpuRenderer.cpp` for `glm::inverse`, `glm::transpose`, `glm::determinant`, `glm::translate`, `glm::rotate`, `glm::scale`, `glm::lookAt`, `glm::perspective*`, or raw `_viewProj * _model`.

No MathNode op IDs, serialization, saves, authored field semantics, or production GPU storage layouts change. Rung 8 remains IN PROGRESS until exact-head CI proves this native-resolution witness.
