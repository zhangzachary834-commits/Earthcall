# SDF Spatial Prophetic and Adaptive Compute Moments

**Date:** 2026-10-16
**Status:** Architectural cross-check
**Related:** `docs/architecture/SDF_SPATIAL_PROPHETIC_DIRECT_PROFITABILITY_ARTIFACT.md`, `docs/architecture/ADAPTIVE_COMPUTE_MOMENTS.md`

## The Interrelation

The SDF Spatial Prophetic direct profitability artifact is a cached representation (a "first experimental artifact: profitable proof runs") compiled from a richer theorem. Its purpose is to save frame-time evaluation costs by answering whether a ray interval can cheaply skip authored field work.

However, deriving this profitability artifact—compiling the theorem into a direct representation—is a computationally intensive maintenance task.

This directly connects to the **Adaptive Compute Moments** architecture. `ADAPTIVE_COMPUTE_MOMENTS.md` explicitly calls out "geometry preparation" and "cache building" as resumable world maintenance tasks whose value increases when allowed to improve slowly over time, provided they don't cause frame drops.

### How they relate

The generation of the SDF profitability artifact is exactly the kind of process that the `ComputeScheduler` in the Adaptive Compute Moments framework is designed to manage.

Because the spatial proof theorem is resident ahead-of-time, compiling it into the narrower, faster artifact can be sliced into resumable chunks and executed using the temporal budget allocated by the `ComputeScheduler`. This ensures that the heavy work of identifying maximal axis-adjacent positive runs does not stutter the active simulation frame rate.

Thus, Adaptive Compute Moments provides the economic engine and temporal budget required to build the SDF Spatial Prophetic direct profitability artifacts, enabling the renderer to reap the frame-time savings without suffering from expensive cache-building penalties.
