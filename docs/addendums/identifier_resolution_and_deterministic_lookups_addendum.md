# Integrating Identifier Resolution and Deterministic Lookups

Origin: Jules architectural interrelation proposal; corrected against Earthcall canonical `59232506f781`.

## Two distinct lookup contracts

In `src/Singularity/Network/WebSocketServer.cpp`, a foreign property-write request may address special aliases such as `@player` or `@active_zone`. For general beings, the handler first searches `Universe::instance().beings()` by `getIdentifier()`, but it **also accepts** an Object's `getObjectID()` or `getEntityName()`, and searches active-Zone-owned Objects and spatial fields. Only the later **LawManager fallback** explicitly resolves Laws by stable identifier through `lm->find(target)` or `lm->find(normTarget)`; it deliberately does not match a Law's display name, since display names need not be unique. Therefore strict identifier-only resolution is a Law fallback rule, **not** a blanket guarantee for all foreign targets.

In `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`, `LawManager::runDriveSessions` takes **one** `Universe::instance().beings()` snapshot per pass, rather than rebuilding that snapshot for each subject or event participant. It constructs a per-pass identifier index using first-insertion-wins semantics: if two distinct beings share an identifier, the **first** being in the provider's iteration order is returned, just as in the original linear scan. If no Universe being matches, it searches the active Law's target members, skipping unmade targets. Building the index costs O(number of beings) per drive pass and subsequent lookups are expected O(1); this does **not** enforce global identifier uniqueness or receive an identifier-validity guarantee from the WebSocket path.

## Interrelation and boundary

Both paths depend on stable identifiers where authored Laws are addressed, but their guarantees are intentionally different: foreign mutation has several target-addressing modes, whereas drive sessions retain subject and event identities and amortize the **snapshot construction**, not each lookup. The index must preserve first-match ordering and the Law-target fallback. `time_flow_test` now witnesses duplicate identifiers, first-match drive continuation, and Law-target fallback after the provider stops supplying the subject. Missing/unmade participants and Law display-name collisions remain separate contracts requiring their own coverage; no global uniqueness assumption is introduced.
