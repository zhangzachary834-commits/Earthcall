# Integrating the Unified Brush System and CPU-GPU Micro-Mastery

*Initial synthesis: Jules. Reviewed against the Earthcall implementation.*

## Separate responsibilities

The [Unified Brush System](../tools/UNIFIED_BRUSH_SYSTEM.md) defines brush presets, stroke dynamics, paint layers and compositing. The existing [`BrushSystem`](../../src/Singularity/Screen/BrushSystem.cpp) applies strokes to CPU-side RGBA pixel buffers and composites layers. Its implementation does **not** presently show a direct call from each stroke into `GpuBufferPool`.

The [CPU-GPU Micro-Mastery architecture](../architecture/Singularity/GPU_MICRO_MASTERY_ARCHITECTURE.md) addresses a different bottleneck: repeated WebGPU buffer creation for rendering data. [`GpuBufferPool`](../../src/Singularity/Screen/WebGPU/GpuBufferPool.cpp) reuses uniform, vertex and storage chunks, suballocating aligned slices. [`WebGpuRenderer`](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp) uses this pool for rendering inputs. The pool still invokes `wgpuDeviceCreateBuffer` when a new chunk is needed and `wgpuQueueWriteBuffer` for uploads; it does **not** bypass the graphics driver entirely.

## Where an integration could help

If authored brush edits cause render-relevant state or geometry to change, an explicit brush-to-renderer handoff could reuse GPU allocations rather than creating new buffers for every update. The handoff must distinguish CPU pixel compositing, texture upload, per-draw uniform/storage updates and topology invalidation: these are not interchangeable operations.

Batching and reusable buffers can reduce allocation overhead without changing what an authored stroke means. That is a potential performance benefit, **not** a guarantee that every brush change is constant-time, that driver calls disappear, or that thousands of Law-driven Objects necessarily achieve a particular frame rate.

## Evidence before claiming success

A meaningful witness should exercise actual brush strokes through the implemented render path, count driver buffer creations and GPU writes, measure upload/compositing and frame times at representative sizes, and compare emitted pixels and authored properties with the optimization disabled. The existing [micro-mastery lag probe](../../tests/singularity/webgpu_micro_mastery_lag_test.cpp) exercises the renderer's buffer behavior, but does not by itself prove an end-to-end brush integration.

**Source anchors:** [BrushSystem](../../src/Singularity/Screen/BrushSystem.cpp), [GpuBufferPool](../../src/Singularity/Screen/WebGPU/GpuBufferPool.cpp), [WebGpuRenderer](../../src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp).
