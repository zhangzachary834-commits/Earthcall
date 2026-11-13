# No Black Box and the Second Person Framework

**Status:** Conceptual interrelation; Second Person visibility, representation, and conflict remain specified rather than implemented.

## Sources

- [No Black Box](../ontology/NO_BLACK_BOX.md)
- [The Second Person Framework](../ourverse/SECOND_PERSON_FRAMEWORK.md)
- [Prophetic Rete and TransferPolicy](PROPHETIC_RETE_AND_TRANSFER_POLICY.md) (conceptual proposal, not proof of implemented ACL coverage)

## Legibility is not authority

No Black Box requires that a being's state be registered and legible, or explicitly identified as beneath the Kernel. This does **not** grant every actor unrestricted mutation. The distinction is between **reach** (what is addressable and inspectable) and **standing** (who may act).

`TransferPolicy::canTransfer` currently gates set-to-set transfers; ordinary `PropertyPath` reads and writes do **not** yet pass through a general actor-aware authorization decision. Thus the presence of a PropertyPath or a TransferPolicy tier is not evidence that every unauthorized write is rejected. Do not add a competing permission system.

The Second Person Framework separately specifies Person-to-Person visibility, representation, and conflicting Laws. Its three mechanisms are not implemented guarantees. Constitutional Kernel guards already protect certain Person-body/location acts, but complete signed, revocable consent relations and actor attribution across every actuation channel remain unfinished. The framework distinguishes authorship, ownership, governance, dependency, and Constitution; these are not interchangeable.

## Integration proof gates

A future implementation must carry authenticated actor and target identity across each relevant read, representation, and mutation channel; define the consent/revocation proof and its persistence; enforce the constitutional guards even where ordinary property writes bypass transfer policy; and test unauthorized attempts, revoked consent, and conflicting standing. Until then, transparent state enables *auditability*, not a cryptographic proof that consent was honored.
