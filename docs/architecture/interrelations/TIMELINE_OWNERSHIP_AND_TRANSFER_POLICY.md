# Timeline Ownership and Transfer Policy

**Date:** 2024-05-18
**Status:** Architectural cross-check
**Related:** `docs/architecture/ontology/TIME_AND_MOMENT.md`, `docs/architecture/law/PROPHETIC_RETE_AND_TRANSFER_POLICY.md`

Earthcall treats time not as a monolithic background engine clock, but as a first-class, authorable domain called a `Timeline`. A `Timeline` contains `Moment`s and can be owned by a `Singular`, whether that is a Person, a Zone, or a specific Formation. Because time is data, it immediately collides with the central mechanism of governance: `TransferPolicy`.

If a `Timeline` is just another owned entity, then its progression, branching, or rewinding are simply property mutations. `TransferPolicy` does not need a separate set of "temporal permission" rules. It applies its existing gating logic to temporal state.

When a Law attempts to shift the current `Moment` of a `Timeline`—perhaps initiating a localized rewind or temporal dilation—it is fundamentally a write operation. The TransferPolicy evaluates this write by asking who owns the `Timeline` and whether the acting Law's author holds a Relation that grants them authority over it.

The friction here arises when we conflate the hardware tick with authored time. The absolute progression of the substrate (the Kernel tier) is ungovernable. However, the authored `Timeline` a Person experiences is Governable. By routing temporal mutations through `TransferPolicy`, we ensure that no Law can forcefully rewrite the temporal experience of a Person without explicit relational consent. Time manipulation ceases to be engine magic; it becomes an authored, legible interaction governed by the same rules that protect physical boundaries and material state.

### Thoughts on Integration and Interrelation

The profound consequence of this interrelation is that time becomes inherently relational. We don't have "time zones" built into the C++ architecture; we have Persons and Zones owning Timelines, and TransferPolicy ensuring that one Person cannot fast-forward another Person's reality without the required constitutional Relation. It ensures the `No Black Box` principle applies to the very fabric of how events unfold, making temporal progression an auditable, governable sequence of registered property paths.

*Model: Jules / Harness: Standard / Session ID: a4b9c2d1-e5f6-7a8b-9c0d-1e2f3a4b5c6d*
