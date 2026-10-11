# Integrating Identifier Resolution and Deterministic Lookups

Model: Jules
Harness: Earthcall
Session ID: sess-001

In Earthcall, resolving identity requires careful alignment between how external signals address objects and how the internal simulation queries them. Two distinct but deeply interrelated mechanisms—strict identifier resolution in the `WebSocketServer` and O(1) first-match caching in `LawManager::runDriveSessions`—work together to guarantee deterministic lookups across both foreign communication boundaries and high-frequency internal simulation frames.

## Thought on Integration and Interrelation

When considering how external agents or web interfaces target specific `Law` beings via the `WebSocketServer`, the system enforces a strict matching policy based solely on unique identifiers (e.g., `l->getIdentifier() == target || l->getIdentifier() == normTarget`). It intentionally does not fall back to fuzzy or display-name lookups (`l->name()`). This strictness ensures that multiple `Law` instances sharing identical display names cannot cause ambiguous state changes when modified from outside the simulation.

This strict identity boundary directly complements the performance-critical path inside the `LawManager`. During `runDriveSessions`, the simulation must repeatedly look up drive session subjects and event participants. Doing this naively would mean executing O(N) linear scans over `Universe::instance().beings()`, incurring virtual `getIdentifier()` string heap allocations each time. To resolve this without sacrificing the deterministic correctness guaranteed at the perimeter, the `LawManager` pre-populates an `std::unordered_map<std::string, Singular*> beingMap` using `emplace` once per pass.

Because `emplace` only inserts if the key does not already exist, it intrinsically preserves first-match semantics. The strict identifier resolution from the WebSocket layer ensures that the strings used as keys are well-formed and unique where necessary, while the caching mechanism provides O(1) lookup speeds. The result is a cohesive architecture where external determinism (strict ID matching) supports internal performance (first-match caching), preventing both ambiguous mutations from the web and frame-rate hitching in the core simulation loop.
