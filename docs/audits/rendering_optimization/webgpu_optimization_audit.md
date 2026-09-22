# WebGPU Optimization Audit

We've audited Earthcall's CPU and GPU architecture to identify bottlenecks in the WebGPU renderer that explain the low FPS and jitter when loading many implicit objects (like 100 toruses) or expensive expressions (like Perlin green hills). 

None of the proposed optimizations change the ontological meaning of the world or C++ data structures; they strictly restructure how the GPU consumes and evaluates the data.

## 1. Implicit Field Instancing (CPU Bottleneck)
- **Observation:** `WebGpuRenderer::drawImplicit` is completely un-instanced. For 100 toruses, the engine dispatches 100 separate `wgpuRenderPassEncoderDraw` calls and creates 100 separate `WGPUBindGroup`s per frame.
- **Impact:** Massive CPU overhead. WebGPU is designed around command batching; creating bind groups and dispatching individual draws per object ruins CPU performance and explains the jitter.
- **Optimization:** We can batch implicit objects exactly like `drawMesh` batches them. While `prog.params` (the SDF parameters, e.g. radii) differ per object, we can pool them into a single `Storage` array. We pass a `paramOffset` in the per-instance data to the vertex shader, which passes `instance_index` to the fragment shader via `flat`. Then `sdfEval(p)` dynamically looks up its constants using the instance offset. This reduces 100 draw calls down to 1.

## 2. Iso-Surface Gradient Evaluator (GPU Bottleneck)
- **Observation:** When an authored expression is evaluated (e.g., Perlin noise for "green hills"), `prog.needsGradientStep` is set to `true`, forcing `damping = 1.0`.
- **Impact:** Inside `SdfWgsl.cpp`'s `fs` raymarcher, if `damping > 0.5`, the fragment shader computes the gradient using central differences *on every single step* of the 192-step loop to normalize the distance. For a 3D noise function, this multiplies the `cnoise3` evaluation cost by 4 on every step. This leads to hundreds of noise evaluations per raymarched pixel, tanking the GPU.
- **Optimization:** 
  - **Dynamic stepping:** Only evaluate the central difference gradient when `raw` (the iso-surface value) is close to 0. When far away, we can take a conservative fixed step or a scaled step. Newton-Raphson steps are only valid near the root anyway.
  - **Analytical gradients:** Earthcall's math AST already supports `Op::Gradient`. We could theoretically emit the true analytical derivative into WGSL, bypassing the 4x evaluation multiplier entirely.

## 3. Dynamic Offsets for Bind Groups (CPU Bottleneck)
- **Observation:** The `MeshUniforms` and instance storage buffers are dynamically suballocated from `GpuBufferPool`. However, the renderer calls `wgpuDeviceCreateBindGroup` for *each* batch and implicit draw, binding to that specific offset.
- **Impact:** Creating `WGPUBindGroup` objects during the hot render loop every frame is a major anti-pattern in WebGPU/Vulkan. 
- **Optimization:** Set `hasDynamicOffset = true` in the `WGPUBindGroupLayoutEntry`. This allows Earthcall to create a single static bind group mapping the entire backing buffer, and simply pass the offsets as `dynamicOffsets` during `wgpuRenderPassEncoderSetBindGroup`. This completely eliminates bind group creation from the per-frame render loop.

## 4. Particle Generation CPU Overhead
- **Observation:** `WebGpuRenderer::drawParticles` executes a deterministic `xorshift32` generator in a CPU loop to compute `verts` for all particles every single frame, then uploads a fresh vertex buffer.
- **Impact:** For large particle counts, this is a significant CPU bottleneck and causes unnecessary memory traffic to the GPU each frame.
- **Optimization:** Since the logic is entirely deterministic and stateless (seeded strictly by particle index), it can be rewritten directly into a Vertex Shader. The CPU would only need to issue a single draw call with `vertexCount = count`, and the GPU will instantly evaluate the positions using `@builtin(vertex_index)`.
