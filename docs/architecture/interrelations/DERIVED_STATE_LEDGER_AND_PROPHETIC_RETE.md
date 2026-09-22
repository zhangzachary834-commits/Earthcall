# The Derived-State Ledger and the Prophetic Rete

**The bidirectional binding of time: grounding the past dependencies and bounding the future executions.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../law/DERIVED_STATE_LEDGER.md` (Tracking what derived state depends on and what invalidates it)
*   `../law/PROPHETIC_RETE.md` (Ahead-of-time abstract interpretation of Law)

---

## The Interrelation

The Derived-State Ledger and the Prophetic Rete represent the two halves of temporal awareness in Earthcall's Law engine.

The **Derived-State Ledger** looks *backward*. It is the historical anchor. It records why a piece of derived state (such as `_vocabularyIndex`, `_relevanceEdges`, or `_prophetic` text) currently exists, what underlying facts it depends on, and precisely what changes will invalidate it. It is the engine's memory of causal dependency.

The **Prophetic Rete** looks *forward*. It is the predictive anchor. By performing ahead-of-time abstract interpretation of Laws before they fire, it determines what derived state *will* be created, mutated, or destroyed. It establishes upper bounds on memory allocation and execution time.

Their interrelation is that **they form a closed, deterministic loop of time that makes perfect reversibility and zero-allocation execution possible.**

When the Prophetic Rete predicts a Law's consequences, it feeds this information into the execution substrate to pre-allocate bounds. Once the Law fires, the consequences are recorded in the Derived-State Ledger. If the Law must be reversed (a core Earthcall capability), the Ledger provides the exact dependency chain to undo, and because the Prophetic Rete already bounded the possibility space, the reversal is guaranteed not to encounter unknown state or require unexpected deallocations.

The Ledger grounds the past; the Prophetic Rete bounds the future. Together, they eliminate the need for reactive runtime tracing, replacing it with a fully known temporal topology.
