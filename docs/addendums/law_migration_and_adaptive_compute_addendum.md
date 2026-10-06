# Addendum: Integrating Law Migration and Adaptive Compute Moments

*(Model: Gemini 1.5 Pro, Harness: Jules, Session ID: 596534326296339974)*

## Reflections on the Architectural Synthesis

The [Law Migration Framework](../architecture/law/LAW_MIGRATION_FRAMEWORK.md) details how hard-coded functionality, previously executed in C++, is progressively migrated into authored law within Earthcall. This transformation is fundamental to the system's ontology, exposing the engine's mechanisms to Person authorship and removing opaque "black boxes".

However, this ontological victory introduces a profound computational challenge. Laws evaluated as part of the Rete network, reading properties and iterating over subjects, are intrinsically more demanding than raw C++ functions. If 10,000 entities are governed by a complex authored law evaluated every frame, the engine risks staggering frame times, violating the principle of a responsive, continuous Ourverse.

### The Necessity of Adaptive Compute

This is where [Adaptive Compute Moments](../architecture/ADAPTIVE_COMPUTE_MOMENTS.md) becomes an essential companion to Law Migration. The architecture of Adaptive Compute provides an independent temporal domain for expensive maintenance tasks, decoupling "world progression" from strict frame rendering.

By recognizing that world maintenance—such as Formation Rete relevance discovery and Law candidate readiness—does not need to happen in a single, blocking frame, Earthcall transforms an accidental lag spike into intentional preparation.

When a heavily migrated Law begins to demand significant processing, Adaptive Compute steps in. It allows the world to allocate a specific temporal budget for evaluating that Law, slicing the computation into resumable pieces across multiple frames.

### The Synthesis

Therefore, the successful migration of engine behavior into authored Law *requires* the Adaptive Compute architecture.
1. **Migration provides the semantic truth:** The behavior is now legible, authorable, and inspectable.
2. **Adaptive Compute provides the temporal reality:** The engine can sustainably evaluate that truth without breaking the physics of rendering.

Together, they allow the world to "think harder than it moves." A complex Law may require a "charge up" period when a Person approaches its Zone, but once active, it operates within an authored compute budget rather than arbitrarily tanking the framerate.

---

**Linked References:**
* [Law Migration Framework](../architecture/law/LAW_MIGRATION_FRAMEWORK.md)
* [Adaptive Compute Moments](../architecture/ADAPTIVE_COMPUTE_MOMENTS.md)
