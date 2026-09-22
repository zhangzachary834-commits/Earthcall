# Security as In-World Physics

**How Earthcall translates authorization and security bounds into legible, governable ontological properties rather than invisible host-level isolation.**

**Status:** Conceptual interrelation. The general Property-access claims below are a design aim, not current implementation: today's `TransferPolicy::canTransfer` gates set-to-set transfer only. The Law-governed general-access and bootstrap proposal is [`Property Storage and OntoMath Binding`](../ontology/PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md) §§5a–5b. Clarified by Codex (GPT-6), session `01a0cad3-5b23-7430-b00a-0613ece86506`, 2026-09-22T13:44:32-07:00, following Zach's direction that permissions be governed by Laws and Laws themselves be treated as memory.
**Connected Documents:**
*   `../migration/SECURITY_FEATURES.md` (TransferPolicy, First Mover Register)
*   `../law/PROPHETIC_RETE.md` (Static analysis and deterministic state changes)
*   `../ontology/NO_BLACK_BOX.md` (Total legibility of state)

---

## The Interrelation

In conventional software, security is enforced through separation and isolation. Operating systems use processes and rings; web browsers use sandboxes and same-origin policies; container systems use namespaces and cgroups. These mechanisms operate entirely outside the awareness of the software running within them—they are invisible walls imposed by the host environment.

Earthcall takes a radically different approach: **Security features rely on in-world ontological properties.** It refuses aspirational capability security or host-level sandboxing, instead embedding authorization directly into the physics of the world.

### Legible Authorization
Two existing authorization precedents are the **First Mover Register** (who may inject at the substrate save path) and the **TransferPolicy** (which source Properties set-to-set creation may take). A general Law-governed memory/Property access decision is still to be built.

Crucially, neither of these is a hidden OS-level mechanism. Both are exposed as explicit data structures within the engine. Because they are part of the ontology, they adhere to the "No Black Box" doctrine.

*   When a set-to-set creation attempts to take a Property, it encounters the deterministic, Law-governable `TransferPolicy` gate. Ordinary `PropertyPath` reads and writes do not yet pass through a general access gate; extending that same authority office is the open work.
*   An active `FirstMoverSession` makes save writes pass the register's grant and scope checks; section-level `injectedBy` provenance remains open, per `FIRST_MOVER_AUTHORING.md` §8.

### The Prophetic Advantage
Because access rules are intended to be legible Law data, they can eventually participate in **Prophetic Rete** analysis.

The Rete performs ahead-of-time abstract interpretation over Laws. A future permission analysis may use known policy Properties to prove an access impossible, but a stale or incomplete analysis must not silently deny a Law whose live policy permits it.

A refused access must be named and recorded at the actual access boundary. Static analysis may help diagnose it, but it does not replace the live Law-governed decision or the First Mover bootstrap.
