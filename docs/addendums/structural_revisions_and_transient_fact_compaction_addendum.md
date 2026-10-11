# Integrating Structural Revisions and Transient Fact Compaction

Model: Jules
Harness: Earthcall
Session ID: sess-001

Earthcall's Law execution model relies on maintaining awareness of the universe's state without succumbing to the overhead of redundant processing. This balance is achieved through the integration of global structural change signals (`Universe::instance().structuralRevision()`) and highly localized, memory-efficient fact management within the Rete network (`_transientFactCount` compaction).

## Thought on Integration and Interrelation

The interrelation between structural revisions and transient fact compaction reveals a tiered approach to performance optimization, addressing both macroscopic state sweeps and microscopic fact processing.

At the macro level, `LawManager::tick()` is responsible for seeding the Rete network with state facts about every being in the universe (`seedStateFacts`). Executing this sweep unconditionally on every frame would be disastrous for performance, involving per-frame vector allocations and O(N) set lookups over all entities. To prevent this, the operation is tightly gated by `Universe::instance().structuralRevision()`. The system checks if `Universe::instance().structuralRevision() != _lastSeededStructuralRevision`. If the structural revision remains unchanged—meaning no entities have been added, removed, or structurally altered in a way that affects tracking—the sweep is skipped entirely. In these steady-state frames, all necessary beings are already cached in `_seededBeingPointers`.

When the structural revision *does* bump, the newly seeded state facts flow into the individual `Law` objects' Rete networks. Here, at the micro level, another optimization takes over. The Rete network manages both persistent state facts and fleeting, event-driven transient facts. The variable `_transientFactCount` strictly tracks the number of non-state (`!isState`) facts in the `_facts` collection.

When a structural revision triggers a re-evaluation or when temporal events decay, the network must clean up consumed or expired data via `retractFirst`. Because `_transientFactCount` is accurately maintained, if it equals zero, `retractFirst` can exit immediately in O(1) time without iterating through the fact list. When transient facts *are* present, the system employs in-place compaction using `std::move`. This shifts remaining persistent facts forward in memory, actively eliminating the need for per-frame vector reallocations and bypassing the `std::shared_ptr` ref-count overhead that would occur if new vectors were constructed.

Together, these mechanisms ensure that Earthcall's simulation scales effectively. The global structural revision prevents unnecessary high-level sweeps, while transient fact compaction ensures that when the system does need to process changing facts, it does so with minimal memory churn and optimal cache locality.
