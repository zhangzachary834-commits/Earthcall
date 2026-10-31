# No Black Box and The Second Person Framework

**How Earthcall resolves the tension between absolute state legibility and the requirement for signed consent when actuating a Person.**

## Interrelated Documents
- `docs/architecture/ontology/NO_BLACK_BOX.md`: Mandates that all state must be legible and mutable via PropertyPath, explicitly refusing unaddressable "black box" state.
- `docs/architecture/ourverse/SECOND_PERSON_FRAMEWORK.md`: Defines the constraints on what crosses the boundary between two Persons (sight, likeness, or will), emphasizing that no Person may modify another's state without explicit permission.

## Thoughts on Integration and Interrelation

The "No Black Box" doctrine asserts that all state must be completely legible and mutably accessible. It insists that any state a Person carries must be registered and addressable via a `PropertyPath`, ensuring that there is no hidden substrate lever. However, a `PropertyPath` is inherently contextless—it provides a direct, unmediated conduit to state.

This creates an immediate structural tension when introduced to the "Second Person Framework". The Second Person framework insists on strict boundaries between Persons, specifically regarding visibility, representation, and conflict (will). If all state is universally addressable via `PropertyPath`, how does the system prevent Person A from capriciously altering the state of Person B?

Earthcall resolves this tension without compromising either doctrine through explicit authorization bridges, primarily the `TransferPolicy`.

1. **State Remains Legible:** The state itself remains fully registered and addressable via `PropertyPath`, satisfying No Black Box. There are no private, unaddressable fields hiding beneath the kernel.
2. **Mutations Require Consent:** While the path to the state exists, the *act of writing* across a Person boundary is intercepted. The Second Person Framework does not hide the state; instead, it enforces signed consent for actuation.
3. **The TransferPolicy Bridge:** Mechanisms like `TransferPolicy` serve as the gatekeepers. When a modification is attempted across boundaries, these systems evaluate the actor's context and the required permissions before allowing the `PropertyPath` assignment to execute.

In this way, legibility (No Black Box) and sovereignty (Second Person Framework) coexist. You can see the door and the lock, and you can attempt to turn the handle, but you must have the key to open it.

---
*Provenance: Authored by Gemini during Earthcall Harness session Unknown.*
