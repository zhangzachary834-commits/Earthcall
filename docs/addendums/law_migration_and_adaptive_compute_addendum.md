# Addendum: Integrating Law Migration and Adaptive Compute Moments

*(Model: Gemini 1.5 Pro, Harness: Jules, Session ID: 596534326296339974)*

## Reflections on the Architectural Synthesis

The [Law Migration Framework](../architecture/law/LAW_MIGRATION_FRAMEWORK.md) describes Earthcall's movement from hard-coded engine behavior toward authored Law where that migration preserves the relevant kernel invariants. The goal is semantic legibility and Person authorship rather than hiding domain behavior behind opaque engine mechanisms.

That migration can increase the importance of compute economics: relevance discovery, derived structures, geometry preparation, cache building, and other maintenance work must not become accidental frame taxes merely because more of the world's behavior is authored and inspectable.

### Adaptive Compute as a Companion Architecture

[Adaptive Compute Moments](../architecture/ADAPTIVE_COMPUTE_MOMENTS.md) is an **architecture proposal** for giving expensive *maintenance work* independent temporal domains, resumable progress, bounded scheduler admission, and authorable compute policy. It begins with the Slow Adapter and deliberately generalizes beyond it.

The proposal does not currently guarantee that arbitrary Law evaluation is resumably sliced across frames. Its concrete contract is narrower: maintenance clients SHOULD be able to consume bounded compute, yield, preserve resumable state, detect invalidation, and publish only sound completed results. Slow Adapter traversal is the explicit specialization.

This matters to Law Migration because migrated behavior may depend on maintenance structures whose preparation can be decoupled from foreground rendering. Formation Rete relevance discovery, Law-candidate readiness, geometry preparation, and similar derived work can eventually receive authored temporal emphasis without equating one render frame with one unit of maintenance progress.

### The Synthesis

The two architectures therefore complement one another without making either a prerequisite for every authored Law:

1. **Law Migration exposes semantic behavior:** eligible behavior becomes legible, authorable, and inspectable while kernel invariants remain kernel-owned.
2. **Adaptive Compute proposes temporal governance for maintenance:** expensive preparatory and derived work can be admitted under bounded, observable budgets rather than becoming an unconditional frame tax.

This is the architectural meaning of the world being able to "think harder than it moves." A heavy Zone MAY eventually request preparation policy and readiness work before entry, while the kernel preserves foreground liveness and scheduler safety. Whether entry is blocked, advised, or allowed before readiness is explicitly an authorial policy question in the proposal.

The synthesis is therefore directional rather than a claim that the complete scheduler already exists: as Earthcall migrates more behavior into authored truth, Adaptive Compute provides a proposed path for keeping the maintenance that supports that truth temporally explicit and sustainable.

---

**Linked References:**
* [Law Migration Framework](../architecture/law/LAW_MIGRATION_FRAMEWORK.md)
* [Adaptive Compute Moments](../architecture/ADAPTIVE_COMPUTE_MOMENTS.md)
