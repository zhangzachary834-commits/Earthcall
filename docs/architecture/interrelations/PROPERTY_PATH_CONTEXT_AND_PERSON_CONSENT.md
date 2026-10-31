# PropertyPath Context and Person Consent

**Status:** Architectural cross-check — implemented Kernel refusal and transfer gate distinguished from **unimplemented** general actor-aware PropertyPath governance and signed consent lifecycle.

**Sources:** [No Black Box](../ontology/NO_BLACK_BOX.md), [Property storage and OntoMath binding](../ontology/PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md), [Second Person Framework](../ourverse/SECOND_PERSON_FRAMEWORK.md), [TransferPolicy](../../../src/Singularity/TransferPolicy.hpp).

## Legibility is not authorization

The No Black Box refusal requires every meaningful state field of a Singular to be addressable, or explicitly identified as Kernel mechanism. A readable `PropertyPath` is not a license for every actor to change the field. Read visibility, write authority, and positive actuation of a real human Person's Body are different questions.

`TransferPolicy` already provides the one legible Kernel/Governable/Gated authority office, but **its current `canTransfer` operation gates set-to-set transfers only**. General PropertyPath reads/writes do not yet consult an actor-aware gate; there is no implemented universal consent check in `TransferPolicy`. A registered property may also be structurally read-only. Do not claim that CSS access, ordinary path writes, or all Person state are presently protected by this transfer API.

## Person's constitutional boundary

Zach's Second Person Framework distinguishes **Authorship, Ownership, Governance, Dependency, and Constitution** as five distinct Relation kinds; their precise topology/admission remains authorial work. Authorship of a Law is not itself consent to later bodily actuation. The hardcoded Kernel boundary refuses a Law's unconsented **positive body/location writes** to a Person, including qualified paths whose bearer is that Person. It does not prohibit reading location to express a restriction or warning.

The intended permission to move a Person is a **Person-signed, revocable Person–Law consent sub-Relation**. Its signed proof, matching and revocation lifecycle are **not yet implemented**; the Law motion path refuses rather than inventing authorization. Other actuation channels still require audit. This constitutional refusal precedes any general `TransferPolicy` extension and must not be weakened to an open-by-default policy.

## Proposed integration, not a shipped guarantee

Extend the **existing** authority office, not a second permission subsystem, with operation (read/write/transfer), requesting actor and provenance, bearer, and relevant Relation standing. Specify how an authorized actor's context reaches a PropertyPath mutation and how consent evidence is verified without mistaking ownership or authorship for bodily consent. The remaining ⚑ AUTHOR decisions in the Second Person Framework belong to Zach; this document does not settle them.

The invariant is **legible predicates, governed authority, and a non-bypassable Person boundary**. None of those permits the engine to silently fabricate a signature, a Relation, or a completed implementation.
