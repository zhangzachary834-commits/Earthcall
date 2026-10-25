# Physics Fixed Steps and Prophetic Rete Relevance

**Status:** Source-checked conceptual interrelation; not a proof of global determinism or lockstep.
**Provenance:** Jules candidate, corrected by Sun Squid Intake II against the canonical implementation on 2026-10-24.

## Two distinct boundaries

`Zone::update(float dt)` accumulates elapsed time and executes at most three substeps of `1.0f / 60.0f`. When a frame falls too far behind, it **discards** the remaining accumulator. `Physics::updateBodies` is called within those substeps **only when** the legacy physics engine is enabled and active physics exists. These are bounded local integration steps, not a guarantee that every rendered frame produces the same number of physical steps or that elapsed world time is preserved.

Prophetic Rete instead analyzes authored Law read/write possibilities ahead of execution. Its soundness rule is conservative: unknown or incomplete analyses must widen/fail open, not suppress a potentially relevant Law. This guarantee comes from the interpreter and its fallback contract, **not** from the physics timestep.

The useful connection is observational: physics can publish contact events and property changes that authored Laws may react to, while Prophetic Rete must not filter out relevant changes. There is no established one-to-one Rete evaluation per physics substep or universal EventBus/simulation-clock lockstep. That stronger synchronization claim would require a separately specified scheduler and executable witnesses.

**Sources:** [Zone update](../../src/ZonesOfEarth/Zone/Zone.cpp), [physics contacts](../../src/ZonesOfEarth/Physics/Physics.cpp), [Prophetic Rete soundness](../architecture/law/PROPHETIC_RETE.md), [engine update ordering](../../src/Singularity/Core/EngineUpdate.cpp).
