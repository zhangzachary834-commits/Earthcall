# Derived-State Ledger and Prophetic Rete

*Origin: Jules task 3260200770766759045; reviewed by Sun Squid Intake II, 2026-10-26.*
**Status: documentation synthesis, not a new materialized-fact store.**

## Two different responsibilities
[The Derived-State Ledger](../architecture/law/DERIVED_STATE_LEDGER.md) is primarily an **audit inventory**: it records derived structures, their inputs, invalidation triggers, and executable witnesses. It is not itself a runtime database where every inferred proposition is persisted.

[Prophetic Rete](../architecture/law/PROPHETIC_RETE.md) is an ahead-of-time, conservative analysis of what authored Laws *may* read and write. Its governing soundness rule is to **widen on uncertainty**: an incomplete analysis must fail open rather than silence a Law.

## Interrelation
Every optimization that maintains derived execution state should declare its dependencies, invalidation behavior, and regression witness in the ledger. Prophetic Rete can help avoid irrelevant work only where its over-approximation proves exclusion safe. Neither document licenses assuming all transitive facts are materialized, that every change has a precise dependency edge, or that an unknown First-Mover input can be treated as impossible.

The shared principle is **legibility plus soundness**: a performance filter must never silently remove a semantically possible Law execution. New caching or materialization proposals need explicit ownership, invalidation, and negative/positive witnesses before being described as implemented.
