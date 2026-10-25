# Derived State Ledger and HTML Bridge

**The interrelation between the engine's internal state caches and real-time Foreign modality UI reactivity.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../law/DERIVED_STATE_LEDGER.md` (The internal system for caching the results of complex ontological queries)
*   `../Integration/HTML_LEXEME_FORMATION_BRIDGE.md` (The bidirectional Foreign modality channel for web-based UI)

---

## The Interrelation

Earthcall's web-based UI (the HTML Bridge) faces a fundamental challenge: how does a Foreign modality app accurately reflect the complex, real-time state of the engine without constantly polling it (which would be prohibitively expensive)?

The solution is found in the architectural union of the HTML Bridge and the Derived State Ledger.

The Derived State Ledger (`DERIVED_STATE_LEDGER.md`) is primarily an internal optimization. It caches the results of complex Prophetic Rete queries and relation graph traversals. Crucially, it only invalidates these caches when the underlying properties actually mutate.

The HTML Bridge leverages this exact same invalidation mechanism. Rather than querying the engine for state, the HTML Bridge subscribes to the Derived State Ledger's invalidation events.

1. **Subscription:** The Foreign web app declares its data dependencies (e.g., "I need to display the position of Object A").
2. **Ledger Binding:** The HTML Bridge translates this into a subscription to the Derived State Ledger for Object A's position property.
3. **Mutation and Invalidation:** When a Law mutates Object A's position, the Derived State Ledger invalidates its cache.
4. **Targeted Push:** The Ledger immediately broadcasts this invalidation to all subscribers, including the HTML Bridge.
5. **UI Update:** The HTML Bridge pushes the new state directly to the web app, triggering a targeted DOM update.

**Conclusion:** The interrelation ensures that the UI is both perfectly accurate and highly performant. The Foreign modality is not a blind polling system; it is directly integrated into the engine's core ontological state caching mechanism, achieving "push-based" real-time reactivity without violating the "No Black Box" boundary.
