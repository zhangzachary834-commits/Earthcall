# CPU-GPU Micro-Mastery: The Substrate of Interaction-as-Law

## 1. The Telos of the Mastery

Earthcall is undergoing a massive architectural shift with the **Interaction-as-Law** movement: the user interface (GUI) is ceasing to be an immediate-mode overlay and is instead becoming a native ecosystem of `Singular` beings governed by `Law`s. 

In a traditional GUI, the entire interface is grouped into a handful of batched draw calls. In Earthcall, a complex menu might consist of 10,000 discrete `Object`s—every panel, button, border, and text character acts as a first-class entity evaluating hover events, collisions, and state changes via the kernel's Rete network. 

If the CPU requested memory from the graphics driver (`wgpuDeviceCreateBuffer`) for each of these 10,000 entities every frame, the driver overhead would stall the engine entirely. **CPU-GPU Micro-Mastery** is the engine's answer: a bespoke, high-performance memory substrate that decouples Earthcall's immense entity count from the graphics driver's allocation latency, making the "UI as Beings" philosophy practically viable at 60+ FPS.

## 2. GpuBufferPool: The Dynamic Sub-Allocator

The `GpuBufferPool` operates as a ring-buffer memory sub-allocator. Rather than negotiating with the WebGPU driver per object, the pool pre-allocates massive `Chunk`s of VRAM at startup. 

During the render loop, the CPU "sub-allocates" slices of this memory instantly via atomic offset increments:
- **Uniform Chunks:** 256-byte aligned slices for dynamic state (transforms, colors, shading variables).
- **Vertex Chunks:** 16-byte aligned slices for ephemeral triangle geometry.
- **Storage Chunks:** 256-byte aligned slices for high-density compute parameters.

At the end of the frame (`resetFrame`), the offset heads are set back to zero without releasing the underlying VRAM. This grants the CPU zero-cost dynamic updates. When thousands of 2D buttons change their color or size simultaneously due to Law evaluation, the `GpuBufferPool` streams those updates directly to the GPU without a single driver allocation call.

**What this actually is:** despite "ring buffer" above, `GpuBufferPool` is a per-frame bump allocator with no frame rotation and no fence — `resetFrame()` reclaims a chunk the instant the CPU-side recording of the next frame begins, not once the GPU has finished reading the last one. Under WebGPU this is safe because `wgpuQueueWriteBuffer` is ordered on the queue timeline against previously submitted command buffers: frame N+1's write cannot land before frame N's draws have read the same bytes. That safety is a property of the WebGPU API's queue ordering, not of this allocator's design — the same code ported directly to Vulkan or Metal's lower-level command-buffer APIs, without an equivalent ordering guarantee or an explicit fence/frames-in-flight scheme, would corrupt in-flight data. Say so here for whoever ports this next.

### Memory Leak Resolution (2026-08-25 Audit)
An audit revealed a severe flaw in the original allocator: if a sub-allocation exceeded the remaining capacity of the current chunk, the pool would immediately allocate an entirely new chunk and skip the remaining capacity in all previous chunks for the duration of the frame. Under heavy load, this caused a cascading VRAM leak. The fix replaced the blind chunk jump with a linear search (`suballocateFrom`) starting from the current chunk forward.

**Correction (2026-08-25, remediation review):** that search does not saturate memory within a single frame — it scans forward from `currentIdx + 1` and never revisits earlier chunks the frame has already passed, so a mid-frame allocation can still open a new chunk while an earlier one in that same frame has room. What the fix actually closes is the leak **across frames**: `resetFrame()` resets `currentIdx` to 0, so the next frame's search starts over from chunk 0 and reuses every chunk the pool has ever allocated, rather than growing without bound. That is the real leak this fixed and the more valuable claim — see `tests/singularity/gpu_mastery_test.cpp` [4a] for the regression test. A free-list for full intra-frame saturation is not warranted at the ~6 MB this pool holds for the heavy-object probe.

## 3. GpuMeshCache: Persistent Geometry and Garbage Collection

While `GpuBufferPool` handles highly dynamic state, `GpuMeshCache` serves as the persistent memory layer for static geometry (`geom::TessMesh`). 

For "Origami" (tessellated) objects, uploading the vertex buffers to the GPU every frame is wasteful. The `GpuMeshCache` uploads the geometry once and maps it to the rendering pipeline automatically on subsequent frames.

### Identity and Cache Invalidation (2026-08-25 Audit)
Originally, the cache was keyed by the raw memory address (`const geom::TessMesh*`). This created two critical failures:
1. **The Stale Cache Bug:** When an `Object` was destroyed, its address was freed, but the `GpuMeshCache` was never notified, causing it to hold VRAM indefinitely.
2. **The Reincarnation Bug:** If a new `Object`'s mesh was allocated at the exact same memory address as a deleted one, the cache falsely recognized it and drew the deleted geometry.

This was resolved by giving the raw geometry struct (`geom::TessMesh`) a globally unique, atomic `uint64_t id` at construction. The cache now checks both the pointer and the `id`. Furthermore, the cache now features a **frame-based garbage collector**. If a cached buffer is not drawn for 10 consecutive frames, `endFrame()` automatically releases the VRAM, ensuring that as UI objects are destroyed by `Law`s, the GPU cleans up the memory without requiring explicit teardown logic in the `Object` destructor.

## 4. OntoMath, Implicit Surfaces, and Storage Chunks

Earthcall does not rely solely on triangles. Through the `OntoMath` subsystem, `Kind::Field` Objects (implicit surfaces, SDFs) are rendered purely via mathematical formulas. The CPU generates a continuous algebraic AST and passes it to the GPU, which evaluates the exact surface per-pixel using raymarching.

Previously, `WebGpuRenderer::drawImplicit` negotiated a brand new `WGPUBufferUsage_Storage` buffer from the driver for every single math object, every single frame, causing significant driver stalls. 

Following the micro-mastery audit, this path was integrated into `GpuBufferPool` via the newly added `_storageChunks`. Now, the `OntoMath` program parameters are sub-allocated instantly from the persistent pool, allowing procedurally generated implicit surfaces to scale effortlessly alongside traditional meshes.

## 5. ScreenChannel: Telemetry Without Black Boxes

In accordance with **Refusal #6 (No Black Box)**, the engine's rendering state cannot be hidden in a C++ singleton. 

The `Singularity::Screen::ScreenChannel` is a First Mover `Law` (`@screen-channel`). It actively tracks the CPU-GPU micro-mastery state and registers it into the Earthcall ontology via `PropertyPath`s. 

Exposed fields include:
- `drawCalls`
- `trianglesDrawn`
- `vramAllocatedBytes`
- `uniformBytesWritten`
- `bufferSuballocations`
- `pipelineSwitches`
- `cachedMeshesCount`
- `wireframe`

The `wireframe` property is fully bi-directional. Authoring a `Law` that sets `@screen-channel.wireframe` to `true` actively binds the rasterizer to `WGPUPrimitiveTopology_LineList` at the driver level, ensuring that the Person holds ultimate authorial control over the Singularity's rendering modality.
