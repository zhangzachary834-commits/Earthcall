# Authorable GPU Runs and CPU-GPU Micro-Mastery

**Status:** Architectural interrelation; no claim of a shipped GPU continuation scheduler.

## Source specifications

- [Authorable GPU Runs Between CPU Cycles](../AUTHORABLE_GPU_RUNS_BETWEEN_CPU_CYCLES.md)
- [CPU-GPU Micro-Mastery Memory Substrate](../Singularity/GPU_MICRO_MASTERY_ARCHITECTURE.md)

## Policy is not the allocation mechanism

Authorable GPU Runs proposes a finite, kernel-owned vocabulary for work quanta, continuation stages, classification, compaction, admission, and exact fallback. A Person or Law may author **policy intent**; it does not thereby author arbitrary GPU commands or a new ontological kind of execution being. The production scheduler is not yet wired.

The Micro-Mastery document describes host-side GpuBufferPool suballocation from reusable GPU buffers, mesh caching, and renderer upload/draw economics. Its ring-buffer strategy reduces repeated driver allocations for supported workloads. **CPU-managed buffer suballocation is not GPU-resident continuation, device-side queue allocation, or proof that GPU work can autonomously re-cohort.**

## Engineering connection

A future bounded GPU-run executor can benefit from reused resident resources, but it must separately establish:

1. Capacity and overflow rules for continuation records and indirect-dispatch argument buffers.
2. Correct ordering, synchronization, and bounded atomic/compaction behavior between stages.
3. A hard stage/work budget and a sound path for unfinished or invalidated work to reach exact fallback.
4. Explicit separation of derived physical queue identities from authored Singular/Relation/Law meaning.
5. Device-measured admission economics: compaction, queue traffic, and dispatch overhead can make a theoretically valid continuation path **slower** than direct execution.

A bound of three continuation stages is a **count**, not a deterministic wall-clock duration. Flat buffers and CPU-side reuse alone cannot guarantee predictable latency or zero GPU-side synchronization cost.

## Evidence and status

The GPU-run source reports experimental indirect continuation and a compact-indirect ray-transport candidate that lost on native economics. It explicitly distinguishes feasible dispatch machinery from a profitable, shipping scheduler. Micro-Mastery supplies a useful resource-management precedent, **not** an implementation witness for autonomous GPU continuation. Any production claim needs a real end-to-end witness for capacity, correctness, fallback, and measured profitability.
