# Adaptive Compute Moments and CPU-GPU Micro-Mastery

**Date:** 2026-09-22
**Status:** Architectural cross-check
**Related:** `docs/architecture/ADAPTIVE_COMPUTE_MOMENTS.md`, `docs/architecture/ontology/CPU_GPU_MICRO_MASTERY.md`

## The Interrelation

The CPU-GPU Micro-Mastery architecture, via the `GpuBufferPool` and `GpuMeshCache`, achieves zero-allocation driver execution per frame. Adaptive Compute Moments provides a system for budgeting finite computational time to background maintenance, bounded by foreground performance.

### How they relate

**Temporal Determinism Requires Memory Predictability**
Adaptive Compute Moments requires precise measurement of how long tasks take to execute in order to properly schedule them without causing frame drops. If VRAM allocations triggered blocking driver calls unexpectedly, the variance in execution time would break the scheduler's ability to maintain stable framerates. CPU-GPU Micro-Mastery's pre-allocated lock-free pools eliminate this latency variance, ensuring that when the `ComputeScheduler` allocates 2 milliseconds for cache building, it actually gets 2 milliseconds of compute, not a blocked thread waiting on a graphics driver.

**Heavy Zone Charge-Up and Garbage Collection**
When a Heavy Zone triggers a "charge-up" phase (`docs/architecture/ADAPTIVE_COMPUTE_MOMENTS.md` §12), the scheduler may temporarily lower the foreground target FPS to donate massive compute budgets to preparation. During this phase, the engine may rapidly construct and tear down thousands of candidate `Singular`s or partial shapes. The `GpuMeshCache`'s frame-based garbage collection ensures that this high-velocity churn doesn't exhaust the memory pool before the Person even enters the Zone, automatically releasing VRAM for intermediate representations without explicit teardown logic.

**Author**: GPT-4o-2024-11-20 (default harness)
**Session ID**: 16433160482754132820
