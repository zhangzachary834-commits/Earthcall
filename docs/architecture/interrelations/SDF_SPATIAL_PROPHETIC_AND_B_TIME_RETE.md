# SDF Spatial Prophetic and B-Time Rete

**Date:** 2026-10-16
**Status:** Architectural cross-check
**Related:** `docs/architecture/SDF_SPATIAL_PROPHETIC_DIRECT_PROFITABILITY_ARTIFACT.md`, `docs/architecture/law/B-time Rete.md`

## The Interrelation

The SDF Spatial Prophetic architecture and B-Time Rete share a profound methodological isomorphism: they both rely on deriving a fast, targeted execution artifact from a richer, more complex underlying theorem or structure to save runtime evaluation costs.

### How they relate

In the SDF Spatial Prophetic framework, a dense, generic spatial proof interpreter (the "richer theorem") is too expensive to run on the hot path (the frame-time raymarching loop). Instead, the system derives a "direct profitability artifact" (e.g., compiled maximal axis-adjacent positive runs) that cheaply answers the specific question of whether a ray interval can be skipped. The richer theorem is explicitly described as "the derivation and repair spine," not the frame-time representation.

Similarly, B-Time Rete analyzes the structure of authored Laws (degrees of freedom, property paths) to build a pre-filter. It uses an "in-advance abstract interpreter" to identify dependencies and build a targeted evaluation path. This avoids evaluating facts or branches that are mathematically guaranteed not to trigger an action based on the law's structure.

Both systems employ the same core strategy:

1.  **Analyze the complex truth ahead of time:** (The SDF spatial proof theorem; the authored Law's property paths and conditions).
2.  **Crystallize a cheap, direct artifact:** (The positive-run bitmap; the B-Time Rete pre-filter/dependency graph).
3.  **Execute the direct artifact on the hot path:** (The WGSL raymarching loop; the real-time Rete evaluation during events).
4.  **Retain the rich theorem for derivation and repair:** (The original SDF theorem; the full Law AST and Formation Rete network) to rebuild the direct artifact when the underlying world state or authored rules change.

This shared approach to "Law-Direct" and "SDF-Direct" execution demonstrates a unified philosophy across both visual rendering and semantic law evaluation in Earthcall.
