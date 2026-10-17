# No Black Box and ForeignActuationGuard

**How Earthcall protects its data from unauthorized foreign modification without violating the explicit prohibition against secret permission systems.**

**Status:** Conceptual interrelation, synthesizing the established total legibility doctrine with the reality of foreign MCP actuation gating.
**Author:** Jules / Claude 3.5 Sonnet / session `interrelations-synthesis` / 2024-10-25
**Connected Documents:**
*   `../../AGENTS.md` (Refusal 6: No black box)
*   `../ontology/NO_BLACK_BOX.md` (Total property legibility and unregistered fields)

---

## The Interrelation

**Refusal 6** (`AGENTS.md`) states: "No black box. Every field a being carries is registered as a property path — readable by law, writable unless genuinely derived. 'Nobody registered it yet' is not a permission level." The corollary is that Earthcall cannot implement secret, hidden state or bespoke permission models that evade `TransferPolicy`.

At the same time, Earthcall must expose a foreign mutation surface (like MCP for AI agents) that allows external programs to author Laws and manipulate the world. This mutation surface is protected by `Singularity/Foreign/ForeignActuationGuard`.

The challenge is: how does `ForeignActuationGuard` restrict unauthorized external actions without becoming the exact kind of "second permission system" that Refusal 6 explicitly forbids?

### 1. Guarding the Channel, Not the State

`ForeignActuationGuard` resolves this tension by acting on the *channel of actuation*, not by introducing hidden permissions on the *state* of beings.

The state of a `Singular` (its properties, its coordinates, its bounds) remains entirely legible and governed strictly by `TransferPolicy`. There is no hidden "foreign-writable" boolean stored inside the `Object` itself.

Instead, `ForeignActuationGuard` intercepts the *attempt* to mutate via the foreign modality channel. It acts as an authoritative gateway that asks: "Does this incoming foreign session possess the required First Mover cryptographic provenance to inject this action into the engine?"

If the answer is no, the action is dropped at the boundary. The guard does not rewrite the internal permission system; it simply refuses to translate the unauthorized foreign request into an internal execution path.

### 2. Authority is Clamped

When an authorized foreign action *does* pass through `ForeignActuationGuard`, it enters the system with an assigned authority level. Crucially, as per the non-negotiables in `AGENTS.md`, "Authority is clamped to 0 on every path that reads a file. Do not try to write an authority value below 0."

The foreign mutation is forced to play by the exact same rules as an internal native mutation. The `ForeignActuationGuard` does not grant the foreign actor special "sudo" access that bypasses `TransferPolicy`. It merely validates that the foreign actor is allowed to participate as an author at authority level 0. The internal state remains protected by the universal `TransferPolicy` (Kernel/Governable/Gated tiers), ensuring the single unified permission model remains intact.

### 3. Total Legibility of the Intrusion

Because of the No Black Box principle, the results of any successful foreign actuation are completely visible. If a foreign MCP agent authors a `Law` or creates a `Formation`, those beings are instantly legible to every other system, Law, and Person in the Zone.

The foreign origin is explicitly registered as the author of the mutation. The system does not support "stealth" foreign edits. The transparency demanded by Refusal 6 ensures that while `ForeignActuationGuard` manages the entry point, the resulting state changes are subjected to the exact same ontological scrutiny and legibility as any local human action.
