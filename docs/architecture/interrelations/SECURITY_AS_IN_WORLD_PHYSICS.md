# Security as In-World Physics

**How Earthcall translates authorization and security bounds into legible, governable ontological properties rather than invisible host-level isolation.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../migration/SECURITY_FEATURES.md` (TransferPolicy, First Mover Register)
*   `../law/PROPHETIC_RETE.md` (Static analysis and deterministic state changes)
*   `../ontology/NO_BLACK_BOX.md` (Total legibility of state)

---

## The Interrelation

In conventional software, security is enforced through separation and isolation. Operating systems use processes and rings; web browsers use sandboxes and same-origin policies; container systems use namespaces and cgroups. These mechanisms operate entirely outside the awareness of the software running within them—they are invisible walls imposed by the host environment.

Earthcall takes a radically different approach: **Security features rely on in-world ontological properties.** It refuses aspirational capability security or host-level sandboxing, instead embedding authorization directly into the physics of the world.

### Legible Authorization
The two primary security mechanisms in Earthcall are the **First Mover Register** (who authored a change) and the **TransferPolicy** (the global path-based ACL dictating who can write to what property).

Crucially, neither of these is a hidden OS-level mechanism. Both are exposed as explicit data structures within the engine. Because they are part of the ontology, they adhere to the "No Black Box" doctrine.

*   When a Law attempts to modify an Object, it doesn't hit an opaque `EACCES` permission denied error from an OS syscall. It hits a deterministic gate evaluated against the `TransferPolicy` property paths.
*   When a First Mover injects a state change (e.g., via a JSON save file), the engine performs load-time verification against the First Mover Register to ensure the cryptographic identity matches the authored claim.

### The Prophetic Advantage
Because security in Earthcall is represented as legible data and predictable rules, it integrates perfectly with the **Prophetic Rete**.

The Rete performs ahead-of-time abstract interpretation over the Laws. Because the security boundaries (like `TransferPolicy`) are known properties rather than arbitrary syscall traps, the Rete can statically verify if a Law will *ever* have the authorization to perform its intended action.

If a Law attempts to violate a security boundary, the failure is caught deterministically during analysis, not as a runtime crash or an invisible OS intervention. The security of the world is modeled, predicted, and enforced as clearly as gravity.