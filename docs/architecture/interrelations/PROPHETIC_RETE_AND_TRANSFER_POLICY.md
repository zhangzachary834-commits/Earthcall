# Prophetic Rete and TransferPolicy

**How ahead-of-time Law evaluation statically enforces security bounds.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../law/PROPHETIC_RETE.md` (Ahead-of-time abstract interpretation)
*   `../migration/SECURITY_FEATURES.md` (TransferPolicy and ACLs)
*   `SECURITY_AS_IN_WORLD_PHYSICS.md` (In-world authorization)

---

## The Interrelation

The "TransferPolicy" enforces security globally by defining strict path-based ACLs for property writes. Because Earthcall refuses hidden state (No Black Box), all property paths and authorization rules are completely legible.

The "Prophetic Rete" performs ahead-of-time abstract interpretation of authored Laws to determine which properties they read and write. Because both the Laws and the TransferPolicy ACLs are legible, the Prophetic Rete can statically analyze a Law prior to execution and deterministically prove whether the Law will ever attempt to violate the TransferPolicy. This allows Earthcall to reject malicious or out-of-bounds Laws at authoring time, long before they enter the simulation's event loop.
