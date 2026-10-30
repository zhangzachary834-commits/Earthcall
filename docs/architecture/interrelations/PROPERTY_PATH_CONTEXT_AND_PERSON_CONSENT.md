# PropertyPath Context and Person Consent

**Status:** Architectural cross-check
**Related:** `ontology/NO_BLACK_BOX.md`, `ourverse/SECOND_PERSON_FRAMEWORK.md`, `Singularity/TransferPolicy.cpp`

## The Interrelation

The "No Black Box" doctrine (`NO_BLACK_BOX.md`) establishes that every meaningful aspect of a being must be legible and accessible through the `PropertyPath` vocabulary. This ensures that the engine remains a transparent substrate where UIs and Laws can address state directly (e.g., `@person.position`).

However, the "Second Person Framework" (`SECOND_PERSON_FRAMEWORK.md`) establishes a non-instrumental view of Persons, asserting that moving or actuating a Person requires their signed, revocable consent. A direct property mutation inherently lacks this actor context—it knows *what* is being changed, but not *who* is requesting the change or under what relational authority.

These doctrines intersect at `Singularity/TransferPolicy`. Currently, `TransferPolicy` gates operations like set-to-set transfer and raw CSS access. As property mutations are expanded to handle Person state, `TransferPolicy` must bridge the gap by requiring actor context (e.g., the Law or Person initiating the write) during property path resolution. This allows `TransferPolicy` to verify the existence of a valid consent sub-Relation before permitting the mutation, fulfilling both the demand for legibility (paths remain visible) and the demand for consent (mutations remain governed).
