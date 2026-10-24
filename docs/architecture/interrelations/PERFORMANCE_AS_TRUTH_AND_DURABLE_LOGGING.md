# Performance as Truth and Durable Logging

**Date:** 2026-09-22
**Status:** Architectural cross-check
**Related:** `docs/architecture/ontology/PERFORMANCE_AS_TRUTH.md`, `docs/architecture/PER_SINGULAR_DURABLE_LOGGING.md`

## The Interrelation

These documents share a narrower architectural discipline: representations should preserve the boundaries and truths the engine actually needs rather than forcing unrelated facts through one convenient abstraction.

*Performance as Truth* applies that discipline to rendering. Where Earthcall has an exact mathematical representation such as an SDF, replacing it with gratuitous geometric approximation can make both the model and its execution worse. The performance claim is specific to the measured rendering paths and witnesses documented there; this analogy does not imply that ontological exactness automatically makes arbitrary systems faster.

*Per-Singular Durable Logging* applies a related boundary discipline to history: durable records can be partitioned by Singular rather than requiring one undifferentiated console stream. The benefit is provenance and retrieval aligned with the entity whose history is being inspected. That is an architectural correspondence, not evidence that per-Singular logging has the same performance characteristics as SDF rendering.

The useful cross-check is therefore representational: in both domains, optimize only after identifying the truthful boundary of the thing represented, and keep empirical performance claims attached to their own witnesses.
