# Time, Moment, and the Derived-State Ledger

**How the explicit ledger of derived state and cache invalidation interacts with the reality that Time is a local, relative `Singular` (Timeline) rather than a global tick.**

**Status:** Conceptual interrelation, synthesizing the caching and invalidation requirements of derived state with the shift away from global time.
**Author:** Jules / Claude 3.5 Sonnet / session `interrelations-synthesis` / 2024-10-25
**Connected Documents:**
*   `../law/DERIVED_STATE_LEDGER.md` (The explicit accounting of derived structures and their invalidations)
*   `../ontology/TIME_AND_MOMENT.md` (The separation of `Timeline`, `Moment`, and `Event` from global `Universe` ticks)

---

## The Interrelation

The `DERIVED_STATE_LEDGER.md` mandates that "a derived structure whose invalidation nobody declared is a bug." Every cached outcome—whether a bounding box, a Rete evaluation, or an OntoMath layout—must declare exactly what invalidates it.

Historically, engines rely on a global `deltaTime` or frame tick to invalidate and advance time-dependent derived state. However, `TIME_AND_MOMENT.md` completely overhauls this model: **Time is no longer a global concept.** There is no `class Time`. Instead, there are `Timeline` beings (which are standard `Singulars`). Any object, field, or zone can own its own `Timeline`.

This shift fundamentally rewires how the Derived-State Ledger accounts for temporal invalidation.

### 1. Invalidation is Relational, Not Global

Because a `Timeline` is just a `Singular` with legible properties (like `now` and `delta`), derived state cannot simply say "I invalidate every frame." It must declare *which specific Timeline* it depends on.

For example, a volume shader layout cached by the Kernel might depend on the specific time variable of a local clock authored for that volume. The Derived-State Ledger dictates that the shader layout is only invalidated when *that specific Timeline's* properties change. If a `Timeline` in an unrelated Zone advances, it has absolutely no effect on the local derived state.

This prevents global time ticks from tearing down cached proofs (like zero-proof volumetric structures or Rete memoizations) that are perfectly stable in their local temporal domain.

### 2. Events as Explicit Invalidation Triggers

`TIME_AND_MOMENT.md` defines an `Event` as a distinguished `Moment` carrying a transition edge. This is not a continuous per-frame polling mechanism.

In the Derived-State Ledger, a dependency on an `Event` is much cheaper than a dependency on continuous time. The ledger requires that temporal dependencies be classified. If a Rete network structure depends on a discrete `Event` (e.g., "when the door opens"), it does not need to re-evaluate every time the local `Timeline` advances its coordinate. It only invalidates when the `Timeline` publishes that specific, distinguished `Event` transition.

This explicit separation between continuous time advancement (`now`) and discrete occurrence (`Event`) allows the ledger to accurately report which derived states must be recalculated continuously versus reactively.

### 3. Substrate Splitting and Temporal Drift

When rendering and physics run at different intervals (as defined in the `Performance as Truth` and micro-mastery architectures), the `Timeline` ontology allows the Substrate to cleanly split.

The Derived-State Ledger can track that the GPU volume evaluator depends on `Timeline_Render`, while the collision Rete depends on `Timeline_Physics`. Because both are distinct `Singulars` with explicit property paths, the ledger correctly enforces that a `Timeline_Render` coordinate advancement does not invalidate the `Timeline_Physics` derived state. This prevents cache trashing across execution lanes and ensures that temporal validity is mathematically proven rather than heuristically guessed.
