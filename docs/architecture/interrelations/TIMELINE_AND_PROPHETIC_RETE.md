# Timeline Singulars and Prophetic Rete

**Date:** 2026-10-28  
**Status:** Architectural cross-check; cross-Timeline solver is not implemented  
**Sources:** `docs/architecture/ontology/TIME_AND_MOMENT.md`; `docs/architecture/law/PROPHETIC_RETE.md`

## The shared problem

Timelines are independent temporal domains unless authored Relations establish otherwise. Prophetic Rete is an ahead-of-time, **conservative over-approximation** of authored Law reads, writes, and possible relevance. Its critical safety rule is that uncertainty must **fail open**: it may prove an interaction impossible, but must not suppress a Law on an unproven assumption.

These facts motivate Timeline-aware *future* analysis, but do not make wall-clock overlap a necessary condition for semantic dependence. Two writes may conflict by targeting the same persistent state even when their execution windows do not overlap; a later Law may depend on an earlier write across Timeline boundaries.

## Architectural interrelation

1. **Temporal provenance:** Future relevance edges may record the Timeline/Moment context in which a Law reads or writes, provided that context has an explicit, trustworthy source.
2. **Relations before exclusion:** Independent Timeline identity alone does not prove that two effects are disjoint. A sound exclusion would require proven noninteraction of the target state, causal dependencies, and any authored synchronization or translation.
3. **Conservative fallback:** If scheduling, cross-Timeline relations, First-Mover writes, or external sources are unknown, retain the possible relevance edge. Never suppress a real Law firing merely because two clocks appear unsynchronized.
4. **Derived evidence, not authority:** The existing Prophetic relevance graph is diagnostic/derived state; incomplete analysis does not acquire hot-path narrowing authority. A future Timeline-aware solver needs separate currency, invalidation, soundness, and regression witnesses.
5. **No invented global fixpoint:** The source specification says general cross-Law least-post-fixpoint closure remains a refinement. Do not describe a deployed cross-Timeline clash solver or a proof of real-time overlap.

**Implementation boundary:** The current Timeline first rung supports independently advancing beings, not the final authored synchronization Laws. Prophetic Rete's existing soundness witnesses (including `tests/law/prophetic_rete_test.cpp` Section F) remain authoritative when extending analysis.

**Original intake:** Jules PR #812; scope and implementation claims reconciled against the source architecture.
