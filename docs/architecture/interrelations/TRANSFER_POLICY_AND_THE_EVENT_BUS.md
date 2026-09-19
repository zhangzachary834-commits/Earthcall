# TransferPolicy and the Event Bus

**How the global ACL acts as the locking mechanism *before* the Event Bus, ensuring the algorithmic call stack only branches into authorized state.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../Singularity/TransferPolicy` (The global path-based ACL dictating write authority)
*   `EVENT_BUS_AS_ALGORITHMIC_CALL_STACK.md` (How events function as control flow)
*   `../events/EVENT_BUS_VS_EVENT_HANDLER.md` (The distinction between the delivery mechanism and processors)

---

## The Interrelation

In Earthcall, the `EventBus` functions as the algorithmic call stack (`EVENT_BUS_AS_ALGORITHMIC_CALL_STACK.md`). When a Law executes, it evaluates conditions, mutates state, and emits Events onto the Bus, which in turn trigger subsequent Laws.

However, because Earthcall operates as a multi-author, highly concurrent environment, there is a severe risk of cascading unauthorized state changes. If a rogue Law or unauthorized First Mover were allowed to inject an Event that triggered a massive state-change cascade, the engine would have to unroll or rollback thousands of downstream Events to maintain security.

To prevent this, Earthcall's security model places the `TransferPolicy` gate *before* the `EventBus`.

### The Deterministic Gate

`TransferPolicy` is the global path-based Access Control List. It defines three tiers of authority (Kernel, Governable, Gated) over every `PropertyPath` in the world.

When a Law attempts to mutate a state (e.g., setting `@target.shape.r := 5`), that mutation request must pass through the `TransferPolicy` gate.
*   If the Law (or the Person/First Mover who authored it) lacks the authority to write to `shape.r`, the mutation is silently dropped or clamped.
*   Crucially, because the mutation never occurs, **no Event is emitted to the EventBus.**

### Securing the Call Stack

By acting as a hard, deterministic gate preceding the Event Bus, `TransferPolicy` ensures that the "algorithmic call stack" remains completely secure.

It guarantees that:
1.  **No Ghost Branches:** The Event Bus only ever carries Events for state changes that were *actually authorized and successfully applied*. It never carries "Attempted" state changes.
2.  **Rete Predictability:** The Prophetic Rete can statically analyze a Law, check its target properties against the `TransferPolicy`, and definitively know if the Law will be allowed to execute its downstream cascade.

**Conclusion:** `TransferPolicy` is not just a permission system; it is the structural lock on the Event Bus. It prevents unauthorized interactions from ever entering the engine's control flow, ensuring that the algorithmic consequences of a Law are always physically legal within the world.